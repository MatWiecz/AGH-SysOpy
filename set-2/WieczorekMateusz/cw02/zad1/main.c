#include <memory.h>
#include <stdlib.h>
#include <zconf.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

double get_operation_time(clock_t start, clock_t stop)
{
	return ((double) (stop - start)) / sysconf(_SC_CLK_TCK);
}

enum TASK_TYPE
{
	UNDEFINED_TASK,
	GENERATE,
	SORT,
	COPY
};

enum TOOLS_TYPE
{
	UNDEFINED_TOOLS,
	SYS,
	LIB
};

void *sys_open_file(char *file_path, int rec_size, int recs_num, int force_open)
{
	int file_descr;
	if (!force_open)
	{
		file_descr = open(file_path, O_RDWR);
		if (file_descr == -1)
		{
			printf("Cannot open file \"%s\"!\n", file_path);
			return NULL;
		}
	}
	else
	{
		file_descr = open(file_path, O_RDWR | O_CREAT | O_TRUNC, 0664);
		if (file_descr == -1)
		{
			printf("Cannot open/create file \"%s\"!\n", file_path);
			return NULL;
		}
	}
	if (!force_open && lseek(file_descr, 0, SEEK_END) != rec_size * recs_num)
	{
		printf("Opened file doesn't have %d record with size %d bytes!\n",
			   recs_num, rec_size);
		close(file_descr);
		return NULL;
	}
	lseek(file_descr, 0, SEEK_SET);
	int *file_descr_ptr = malloc(sizeof(int));
	*file_descr_ptr = file_descr;
	return file_descr_ptr;
}

void sys_close_file(void *file_descr_ptr)
{
	int *file_descr = (int *) file_descr_ptr;
	close(*file_descr);
	free(file_descr);
}

int sys_read_file_record(void *file_descr_ptr, int rec_size, int rec_id,
						 void *dest_buffer, int bytes_to_read)
{
	int *file_descr = (int *) file_descr_ptr;
	lseek(*file_descr, rec_id * rec_size, SEEK_SET);
	int bytes_to_process = rec_size;
	if (bytes_to_read > 0 && bytes_to_read < rec_size)
		bytes_to_process = bytes_to_read;
	if (read(*file_descr, dest_buffer, bytes_to_process) != bytes_to_process)
	{
		printf("Some errors occurred during reading record with id %d.\n",
			   rec_id);
		return -1;
	}
	return 0;
}

int sys_write_file_record(void *file_descr_ptr, int rec_size, int rec_id,
						  void *src_buffer, int bytes_to_write)
{
	int *file_descr = (int *) file_descr_ptr;
	lseek(*file_descr, rec_id * rec_size, SEEK_SET);
	int bytes_to_process = rec_size;
	if (bytes_to_write > 0 && bytes_to_write < rec_size)
		bytes_to_process = bytes_to_write;
	if (write(*file_descr, src_buffer, bytes_to_process) != bytes_to_process)
	{
		printf("Some errors occurred during writing record with id %d.\n",
			   rec_id);
		return -1;
	}
	return 0;
}

void *lib_open_file(char *file_path, int rec_size, int recs_num, int force_open)
{
	FILE *file;
	if (!force_open)
	{
		file = fopen(file_path, "r+b");
		if (file == NULL)
		{
			printf("Cannot open file \"%s\"!\n", file_path);
			return NULL;
		}
	}
	else
	{
		file = fopen(file_path, "w+b");
		if (file == NULL)
		{
			printf("Cannot open/create file \"%s\"!\n", file_path);
			return NULL;
		}
	}
	fseek(file, 0, SEEK_END);
	if (!force_open && ftell(file) != rec_size * recs_num)
	{
		printf("Opened file doesn't have %d record with size %d bytes!\n",
			   recs_num, rec_size);
		fclose(file);
		return NULL;
	}
	fseek(file, 0, SEEK_SET);
	return file;
}

void lib_close_file(void *file_ptr)
{
	FILE *file = (FILE *) file_ptr;
	fclose(file);
}

int lib_read_file_record(void *file_ptr, int rec_size, int rec_id,
						 void *dest_buffer, int bytes_to_read)
{
	FILE *file = (FILE *) file_ptr;
	fseek(file, rec_id * rec_size, SEEK_SET);
	int bytes_to_process = rec_size;
	if (bytes_to_read > 0 && bytes_to_read < rec_size)
		bytes_to_process = bytes_to_read;
	if (fread(dest_buffer, bytes_to_process, 1, file) != 1)
	{
		printf("Some errors occurred during reading record with id %d.\n",
			   rec_id);
		return -1;
	}
	return 0;
}

int lib_write_file_record(void *file_ptr, int rec_size, int rec_id,
						  void *src_buffer, int bytes_to_write)
{
	FILE *file = (FILE *) file_ptr;
	fseek(file, rec_id * rec_size, SEEK_SET);
	int bytes_to_process = rec_size;
	if (bytes_to_write > 0 && bytes_to_write < rec_size)
		bytes_to_process = bytes_to_write;
	if (fwrite(src_buffer, bytes_to_process, 1, file) != 1)
	{
		printf("Some errors occurred during writing record with id %d.\n",
			   rec_id);
		return -1;
	}
	return 0;
}

int generate(char *file_path, int recs_num, int rec_size)
{
	int random_descr = open("/dev/urandom", O_RDONLY);
	int file_descr = open(file_path, O_WRONLY | O_CREAT | O_TRUNC, 0664);
	if (file_descr == -1)
	{
		printf("Cannot open/create file \"%s\"!\n", file_path);
		return -1;
	}
	unsigned char *rec_buffer = malloc(rec_size);
	for (int rec_id = 0; rec_id < recs_num; rec_id++)
	{
		if (read(random_descr, rec_buffer, rec_size) != rec_size)
		{
			printf("System error occurred during reading random data!\n");
			return -2;
		}
		if (write(file_descr, rec_buffer, rec_size) != rec_size)
		{
			printf("An error occurred during writing random data to file!\n");
			return -3;
		}
	}
	free(rec_buffer);
	close(random_descr);
	close(file_descr);
	return 0;
}

int sort(char *file_path, int recs_num, int rec_size,
		 enum TOOLS_TYPE tools_type)
{
	void *(*open_file)(char *, int, int, int);
	void (*close_file)(void *);
	int (*read_file_record)(void *, int, int, void *, int);
	int (*write_file_record)(void *, int, int, void *, int);
	switch (tools_type)
	{
		case UNDEFINED_TOOLS:
			break;
		case SYS:
		{
			open_file = sys_open_file;
			close_file = sys_close_file;
			read_file_record = sys_read_file_record;
			write_file_record = sys_write_file_record;
			break;
		}
		case LIB:
		{
			open_file = lib_open_file;
			close_file = lib_close_file;
			read_file_record = lib_read_file_record;
			write_file_record = lib_write_file_record;
			break;
		}
	}
	void *file_struct_ptr = open_file(file_path, rec_size, recs_num, 0);
	if (!file_struct_ptr)
	{
		printf("Cannot open file \"%s\"!\n", file_path);
		return -1;
	}
	unsigned char *buffers[2];
	buffers[0] = malloc(rec_size);
	buffers[1] = malloc(rec_size);
	int recs_to_sort = recs_num;
	int max_in;
	int max_rec;
	while (recs_to_sort > 1)
	{
		max_in = 0;
		max_rec = -1;
		for (int rec_id = 0; rec_id < recs_to_sort; rec_id++)
		{
			if (max_rec == -1)
			{
				read_file_record(file_struct_ptr, rec_size, rec_id,
								 buffers[max_in], 1);
				max_rec = rec_id;
			}
			else
			{
				read_file_record(file_struct_ptr, rec_size, rec_id,
								 buffers[1 - max_in], 1);
				if (buffers[1 - max_in][0] > buffers[max_in][0])
				{
					max_rec = rec_id;
					max_in = 1 - max_in;
				}
			}
		}
		recs_to_sort--;
		if (recs_to_sort != max_rec)
		{
			read_file_record(file_struct_ptr, rec_size, recs_to_sort,
							 buffers[0], 0);
			read_file_record(file_struct_ptr, rec_size, max_rec,
							 buffers[1], 0);
			write_file_record(file_struct_ptr, rec_size, recs_to_sort,
							  buffers[1], 0);
			write_file_record(file_struct_ptr, rec_size, max_rec,
							  buffers[0], 0);
		}
	}
	close_file(file_struct_ptr);
	free(buffers[0]);
	free(buffers[1]);
	return 0;
}

int copy(char *source_file_path, char *dest_file_path, int recs_num,
		 int rec_size, enum TOOLS_TYPE tools_type)
{
	void *(*open_file)(char *, int, int, int);
	void (*close_file)(void *);
	int (*read_file_record)(void *, int, int, void *, int);
	int (*write_file_record)(void *, int, int, void *, int);
	switch (tools_type)
	{
		case UNDEFINED_TOOLS:
			break;
		case SYS:
		{
			open_file = sys_open_file;
			close_file = sys_close_file;
			read_file_record = sys_read_file_record;
			write_file_record = sys_write_file_record;
			break;
		}
		case LIB:
		{
			open_file = lib_open_file;
			close_file = lib_close_file;
			read_file_record = lib_read_file_record;
			write_file_record = lib_write_file_record;
			break;
		}
	}
	void *source_file_struct_ptr = open_file(source_file_path, rec_size,
											 recs_num, 0);
	if (!source_file_struct_ptr)
	{
		printf("Cannot open source file \"%s\"!\n", source_file_path);
		return -1;
	}
	void *dest_file_struct_ptr = open_file(dest_file_path, rec_size,
										   recs_num, 1);
	if (!dest_file_struct_ptr)
	{
		close_file(source_file_struct_ptr);
		printf("Cannot open/create destination file \"%s\"!\n",
			   dest_file_path);
		return -1;
	}
	unsigned char *buffer = malloc(rec_size);
	for (int rec_id = 0; rec_id < recs_num; rec_id++)
	{
		read_file_record(source_file_struct_ptr, rec_size, rec_id,
						 buffer, 0);
		write_file_record(dest_file_struct_ptr, rec_size, rec_id,
						  buffer, 0);
	}
	close_file(source_file_struct_ptr);
	close_file(dest_file_struct_ptr);
	free(buffer);
	return 0;
}

int main(int argc, char *argv[])
{
//	int number;
//	scanf("%d", &number);
	int arg_ptr = 1;
	int arg_no = 0;
	enum TASK_TYPE task_type;
	char *file1_path;
	char *file2_path;
	int recs_num;
	int rec_size;
	enum TOOLS_TYPE tools_type;
	struct tms *global_start_time = malloc(sizeof(struct tms));
	struct tms *global_stop_time = malloc(sizeof(struct tms));
	struct tms *start_time = malloc(sizeof(struct tms));
	struct tms *stop_time = malloc(sizeof(struct tms));
	clock_t real_global_start_time;
	clock_t real_global_stop_time;
	clock_t real_start_time;
	clock_t real_stop_time;
	printf("Op\\Time\t\t\t\t\tReal\t\tUser\t\tSystem\n");
	real_global_start_time = times(global_start_time);
	while (arg_no != 0 || arg_ptr < argc)
	{
		switch (arg_no)
		{
			case 0:
			{
				task_type = UNDEFINED_TASK;
				tools_type = UNDEFINED_TOOLS;
				if (strcmp(argv[arg_ptr], "generate") == 0)
					task_type = GENERATE;
				if (!task_type && strcmp(argv[arg_ptr], "sort") == 0)
					task_type = SORT;
				if (!task_type && strcmp(argv[arg_ptr], "copy") == 0)
					task_type = COPY;
				if (!task_type)
				{
					printf("Command \"%s\" not found! Executing aborted.!\n",
						   argv[arg_ptr]);
					exit(-1);
				}
				arg_ptr++;
				break;
			}
			case 1:
			{
				file1_path = argv[arg_ptr++];
				break;
			}
			case 2:
			{
				if (task_type == COPY)
					file2_path = argv[arg_ptr++];
				break;
			}
			case 3:
			{
				recs_num = strtol(argv[arg_ptr++], NULL, 10);
				if (!recs_num)
				{
					printf("Segments number must be positive integer "
							   "number!\n");
					exit(-2);
				}
				break;
			}
			case 4:
			{
				rec_size = strtol(argv[arg_ptr++], NULL, 10);
				if (!rec_size)
				{
					printf("Segment size must be positive integer number!\n");
					exit(-2);
				}
				break;
			}
			case 5:
			{
				if (task_type > GENERATE)
				{
					if (strcmp(argv[arg_ptr], "sys") == 0)
						tools_type = SYS;
					if (strcmp(argv[arg_ptr], "lib") == 0)
						tools_type = LIB;
					arg_ptr++;
				}
				break;
			}
		}
		arg_no++;
		if (arg_no == 6)
		{
			real_start_time = times(start_time);
			switch (task_type)
			{
				case UNDEFINED_TASK:
					break;
				case GENERATE:
				{
					generate(file1_path, recs_num, rec_size);
					printf("Generate\t\t\t\t");
					break;
				}
				case SORT:
				{
					sort(file1_path, recs_num, rec_size, tools_type);
					printf("Sort\t\t\t\t\t");
					break;
				}
				case COPY:
				{
					copy(file1_path, file2_path, recs_num, rec_size,
						 tools_type);
					printf("Copy\t\t\t\t\t");
					break;
				}
			}
			real_stop_time = times(stop_time);
			printf("%lf\t",
				   get_operation_time(real_start_time,
									  real_stop_time));
			printf("%lf\t",
				   get_operation_time(start_time->tms_utime,
									  stop_time->tms_utime));
			printf("%lf\n",
				   get_operation_time(start_time->tms_stime,
									  stop_time->tms_stime));
			if (task_type > GENERATE)
			{
				if (tools_type == SYS)
					printf("Tools type: System\n");
				else
					printf("Tools type: Library\n");
			}
			printf("Records number: %d\n", recs_num);
			printf("Record size: %d\n", rec_size);
			printf("File1's path: %s\n", file1_path);
			if (task_type==COPY)
				printf("File2's path: %s\n", file2_path);
			printf("\n");
			arg_no = 0;
		}
	}
	real_global_stop_time = times(global_stop_time);
	printf("Total\t\t\t\t\t");
	printf("%lf\t", get_operation_time(real_global_start_time,
									   real_global_stop_time));
	printf("%lf\t", get_operation_time(global_start_time->tms_utime,
									   global_stop_time->tms_utime));
	printf("%lf\n\n\n",
		   get_operation_time(global_start_time->tms_stime,
							  global_stop_time->tms_stime));
	free(global_start_time);
	free(global_stop_time);
	free(start_time);
	free(stop_time);
	return 0;
}