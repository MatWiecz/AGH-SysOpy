#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <wait.h>
#include <time.h>
#include <sys/times.h>
#include <sys/stat.h>

int sigstop_off = 1;
int sigkill_off = 1;
int proc_num;
char *file_path;
pid_t *pid_array;

void sigtstp_handler(int signal_id)
{
	sigstop_off = 0;
}

void sigcont_handler(int signal_id)
{
	sigstop_off = 1;
}

void sigint_handler(int signal_id)
{
	sigkill_off = 0;
}

void sigint2_handler(int signal_id)
{
	sigkill_off = 0;
	for (int i = 0; i < proc_num; i++)
		kill(pid_array[i], SIGINT);
	printf("\n");
	for (int i = 0; i < proc_num; i++)
	{
		int proc_return_value;
		waitpid(pid_array[i], &proc_return_value, 0);
		printf("Child process with id %d created %d copies of his file.\n",
			   pid_array[i], (proc_return_value / 256));
	}
	free(file_path);
	free(pid_array);
	exit(0);
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

int load_original(char *file_path, char **file_buffer_ptr, size_t *
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


int child_bundled_copier(char *file_path, int file_interval)
{
	int copies_num = 0;
	time_t modify_time = 0;
	struct stat file_status;
	size_t file_size = 0;
	char *file_buffer = NULL;
	stat(file_path, &file_status);
	modify_time = file_status.st_mtime;
	load_original(file_path, &file_buffer, &file_size);
	if (save_backup(file_path, &file_buffer, &file_size, modify_time, 0) == 0)
		copies_num++;
	while (sigkill_off)
	{
		while (sigkill_off && sigstop_off)
		{
			stat(file_path, &file_status);
			if (modify_time != file_status.st_mtime)
			{
				if (save_backup(file_path, &file_buffer, &file_size,
								modify_time,
								1) == 0)
					copies_num++;
				modify_time = file_status.st_mtime;
				load_original(file_path, &file_buffer, &file_size);
			}
			sleep((unsigned int) file_interval);
		}
		if (sigkill_off)
			pause();
	}
	if (save_backup(file_path, &file_buffer, &file_size, modify_time, 1) == 0)
		copies_num++;
	if (file_buffer != NULL)
		free(file_buffer);
	return copies_num;
}


int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf("1 program argument expected: task file path. "
				   "Provided %d! Monitor aborted.\n",
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
	int file_interval = 0;
	proc_num = 0;
	size_t pid_array_size = 16;
	file_path = (char *) calloc(pid_array_size * 256, sizeof(char));
	pid_array = malloc(pid_array_size * sizeof(pid_t));
	while (fscanf(task_file, "%s %d", file_path + (256 * proc_num),
				  &file_interval) == 2)
	{
		if (proc_num == pid_array_size)
		{
			pid_array_size *= 2;
			file_path = realloc(file_path, (pid_array_size * 256 * sizeof
				(char)));
			pid_array = realloc(pid_array, (pid_array_size * sizeof(pid_t)));
		}
		pid_array[proc_num] = fork();
		if (pid_array[proc_num] == 0)
		{
			signal(SIGTSTP, sigtstp_handler);
			signal(SIGCONT, sigcont_handler);
			signal(SIGINT, sigint_handler);
			int copies_num;
			copies_num = child_bundled_copier(file_path + (256 * proc_num),
											  file_interval);
			fclose(task_file);
			free(file_path);
			free(pid_array);
			exit(copies_num);
		}
		printf("Process with id %d monitoring file \"%s\".\n",
			   pid_array[proc_num], file_path + (256 * proc_num));
		proc_num++;
	}
	fclose(task_file);
	signal(SIGINT, sigint2_handler);
	char command_str[16] = "";
	char pid_str[16] = "";
	while (sigkill_off)
	{
		printf("> ");
		scanf("%s", command_str);
		if (strcmp(command_str, "LIST") == 0)
		{
			for (int i = 0; i < proc_num; i++)
				printf("Process with id %d monitoring file \"%s\".\n",
					   pid_array[i], file_path + (256 * i));
		}
		if (strcmp(command_str, "STOP") == 0)
		{
			scanf("%s", pid_str);
			if (strcmp(pid_str, "ALL") == 0)
			{
				for (int i = 0; i < proc_num; i++)
					kill(pid_array[i], SIGTSTP);
			}
			else
			{
				pid_t child_pid = (pid_t) strtol(pid_str, NULL, 10);
				for (int i = 0; i < proc_num; i++)
				{
					if (pid_array[i] == child_pid)
					{
						kill(child_pid, SIGTSTP);
						break;
					}
					if (i + 1 == proc_num)
						printf("Process with id %d isn't a child of "
								   "monitor!\n", child_pid);
				}
			}
		}
		if (strcmp(command_str, "START") == 0)
		{
			scanf("%s", pid_str);
			if (strcmp(pid_str, "ALL") == 0)
			{
				for (int i = 0; i < proc_num; i++)
					kill(pid_array[i], SIGCONT);
			}
			else
			{
				pid_t child_pid = (pid_t) strtol(pid_str, NULL, 10);
				for (int i = 0; i < proc_num; i++)
				{
					if (pid_array[i] == child_pid)
					{
						kill(child_pid, SIGCONT);
						break;
					}
					if (i + 1 == proc_num)
						printf("Process with id %d isn't a child of "
								   "monitor!\n", child_pid);
				}
			}
		}
		if (strcmp(command_str, "END") == 0)
			sigint2_handler(SIGINT);
	}
	return 0;
}