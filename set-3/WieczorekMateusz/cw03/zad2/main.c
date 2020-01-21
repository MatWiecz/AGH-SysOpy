#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <wait.h>
#include <time.h>
#include <sys/times.h>
#include <sys/stat.h>


double get_operation_time(clock_t start, clock_t stop)
{
	return ((double) (stop - start)) / sysconf(_SC_CLK_TCK);
}

void get_backup_file_path(char *file_path, time_t modify_time,
						  char *backup_file_path)
{
	strcat(backup_file_path, "./archive/");
	mkdir(backup_file_path, 0775);
	strcat(backup_file_path, "backup_");
	size_t path_len = strlen(file_path);
	char temp_file_path[256] = "";
	strcat(temp_file_path, file_path);
	for (int i = 0; i < path_len; i++)
	{
		if (temp_file_path[i] == '/' || temp_file_path[i] == '.')
			temp_file_path[i] = '_';
	}
	strcat(backup_file_path, temp_file_path);
	char time_suffix[256];
	struct tm *time_info = localtime(&modify_time);
	strftime(time_suffix, 256, "_%Y-%m-%d_%H-%M-%S", time_info);
	strcat(backup_file_path, time_suffix);
}

int load_oryginal(char *file_path, char **file_buffer_ptr, size_t *
file_size_ptr)
{
	FILE *file = fopen(file_path, "rb");
	if (file == NULL)
	{
		printf("%d: An error occurred during opening original file "
				   "\"%s\"!\n", getpid(), file_path);
		return -1;
	}
	fseek(file, 0, SEEK_END);
	*file_size_ptr = (size_t) ftell(file);
	*file_buffer_ptr = malloc(*file_size_ptr);
	fseek(file, 0, SEEK_SET);
	if (fread(*file_buffer_ptr, 1, *file_size_ptr, file) != *file_size_ptr)
	{
		printf("%d: An error occurred during loading original file "
				   "\"%s\"!\n", getpid(), file_path);
		fclose(file);
		return -1;
	}
	fclose(file);
	return 0;
}

int save_backup(char *file_path, char **file_buffer_ptr, size_t *
file_size_ptr, time_t modify_time, int free_buffer)
{
	char backup_file_path[256] = "";
	get_backup_file_path(file_path, modify_time, backup_file_path);
	FILE *backup_file = fopen(backup_file_path, "rb");
	if (backup_file != NULL)
	{
		fclose(backup_file);
		return -2;
	}
	backup_file = fopen(backup_file_path, "wb");
	if (backup_file == NULL)
	{
		printf("%d: An error occurred during creating backup file "
				   "\"%s\"!\n", getpid(), backup_file_path);
		return -1;
	}
	if (fwrite(*file_buffer_ptr, 1, *file_size_ptr, backup_file) !=
		*file_size_ptr)
	{
		printf("%d: An error occurred during saving backup file "
				   "\"%s\"!\n", getpid(), backup_file_path);
		fclose(backup_file);
		return -1;
	}
	fclose(backup_file);
	if (free_buffer)
	{
		free(*file_buffer_ptr);
		*file_buffer_ptr = NULL;
		*file_size_ptr = 0;
	}
	return 0;
}


int child_bundled_copier(int timeout, char *file_path, int file_interval)
{
	int copies_num = 0;
	struct tms *start_time = malloc(sizeof(struct tms));
	struct tms *current_time = malloc(sizeof(struct tms));
	clock_t real_start_time = times(start_time);
	clock_t real_current_time = times(current_time);
	time_t modify_time = 0;
	struct stat file_status;
	size_t file_size = 0;
	char *file_buffer = NULL;
	stat(file_path, &file_status);
	modify_time = file_status.st_mtime;
	load_oryginal(file_path, &file_buffer, &file_size);
	if (save_backup(file_path, &file_buffer, &file_size, modify_time, 0) == 0)
		copies_num++;
	while (get_operation_time(real_start_time, real_current_time) <
		   timeout)
	{
		stat(file_path, &file_status);
		if (modify_time != file_status.st_mtime)
		{
			modify_time = file_status.st_mtime;
			if (save_backup(file_path, &file_buffer, &file_size, modify_time,
							1) == 0)
				copies_num++;
			load_oryginal(file_path, &file_buffer, &file_size);
		}
		sleep((unsigned int) file_interval);
		real_current_time = times(current_time);
	}
	if (save_backup(file_path, &file_buffer, &file_size, modify_time, 1) == 0)
		copies_num++;
	free(start_time);
	free(current_time);
	if ( file_buffer != NULL )
		free ( file_buffer );
	return copies_num;
}

int run_cp (char * source_file_path, char * destination_file_path)
{
	FILE *backup_file = fopen(destination_file_path, "rb");
	if (backup_file != NULL)
	{
		fclose(backup_file);
		return -2;
	}
	pid_t process_id = fork();
	if (process_id == 0)
		execlp("cp", "cp", source_file_path, destination_file_path, NULL );
	return 0;
}

int child_system_copier(int timeout, char *file_path, int file_interval)
{
	int copies_num = 0;
	struct tms *start_time = malloc(sizeof(struct tms));
	struct tms *current_time = malloc(sizeof(struct tms));
	clock_t real_start_time = times(start_time);
	clock_t real_current_time = times(current_time);
	time_t modify_time = 0;
	struct stat file_status;
	stat(file_path, &file_status);
	modify_time = file_status.st_mtime;
	{
		char backup_file_path[256] = "";
		get_backup_file_path(file_path, modify_time, backup_file_path);
		if (run_cp(file_path, backup_file_path) == 0)
			copies_num++;
	}
	while (get_operation_time(real_start_time, real_current_time) <
		   timeout)
	{
		stat(file_path, &file_status);
		if (modify_time != file_status.st_mtime)
		{
			modify_time = file_status.st_mtime;
			{
				char backup_file_path[256] = "";
				get_backup_file_path(file_path, modify_time, backup_file_path);
				if (run_cp(file_path, backup_file_path) == 0)
					copies_num++;
			}
		}
		sleep((unsigned int) file_interval);
		real_current_time = times(current_time);
	}
	{
		char backup_file_path[256] = "";
		get_backup_file_path(file_path, modify_time, backup_file_path);
		if (run_cp(file_path, backup_file_path) == 0)
			copies_num++;
	}
	free(start_time);
	free(current_time);
	return copies_num;
}

int main(int argc, char **argv)
{
	if (argc != 4)
	{
		printf("3 program argument expected: task file path, timeout,"
				   " backup module mode. Provided %d! Monitor aborted.\n",
			   argc - 1);
		return -1;
	}
	FILE *task_file = NULL;
	{
		char *task_file_path = argv[1];
		task_file = fopen(task_file_path, "r");
		if (task_file == NULL)
		{
			printf("Cannot open file with provided path! Monitor aborted.\n");
			return -1;
		}
	}
	int timeout = (int) strtol(argv[2], NULL, 10);
	if (timeout <= 0)
	{
		fclose(task_file);
		printf("Timeout parameter should be positive integer number!"
				   " Monitor aborted.\n");
		return -1;
	}
	int backup_mode = 0;
	if (strcmp(argv[3], "bundled") == 0)
		backup_mode = 1;
	if (strcmp(argv[3], "system") == 0)
		backup_mode = 2;
	if (!backup_mode)
	{
		fclose(task_file);
		printf("Backup module mode should be \"bundled\" or \"system\"!"
				   " Monitor aborted.\n");
		return -1;
	}
	char *file_path = (char *) calloc(256, sizeof(char));
	int file_interval = 0;
	int proc_num = 0;
	size_t pid_array_size = 16;
	pid_t *pid_array = malloc(pid_array_size * sizeof(pid_t));
	while (fscanf(task_file, "%s %d", file_path, &file_interval) == 2)
	{
		if (proc_num == pid_array_size)
		{
			pid_array_size *= 2;
			pid_array = realloc(pid_array, (pid_array_size * sizeof(pid_t)));
		}
		pid_array[proc_num] = fork();
		if (pid_array[proc_num] == 0)
		{
			int copies_num;
			if (backup_mode == 1)
				copies_num = child_bundled_copier(timeout, file_path,
												  file_interval);
			else
				copies_num = child_system_copier(timeout, file_path,
												 file_interval);
			fclose(task_file);
			free(file_path);
			free(pid_array);
			exit(copies_num);
		}
		proc_num++;
	}
	fclose(task_file);
	free(file_path);
	sleep((unsigned int) timeout);
	for (int i = 0; i < proc_num; i++)
	{
		int proc_return_value;
		waitpid(pid_array[i], &proc_return_value, 0);
		printf("Child process with id %d created %d copies of his file.\n",
			   pid_array[i], (proc_return_value/256));
	}
	free(pid_array);
	return 0;
}