#define _XOPEN_SOURCE 500

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <memory.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <ftw.h>
#include <sys/wait.h>

time_t system_time;
char operator;

void run_ls(const char *directory_path)
{
	execlp ("ls", "ls", "-l", directory_path, NULL);
}

char *get_file_type(int st_mode)
{
	if (S_ISDIR(st_mode) != 0)
		return "      DIR";
	if (S_ISCHR(st_mode) != 0)
		return " CHAR DEV";
	if (S_ISBLK(st_mode) != 0)
		return "BLOCK DEV";
	if (S_ISFIFO(st_mode) != 0)
		return "     FIFO";
	if (S_ISLNK(st_mode) != 0)
		return "    SLINK";
	if (S_ISSOCK(st_mode) != 0)
		return "     SOCK";
	if (S_ISREG(st_mode) != 0)
		return "     FILE";
	return "???";
}

int get_time_string(time_t time, char *buffer)
{
	struct tm *time_info = localtime(&time);
	strftime(buffer, 256, "%d.%m.%Y %H:%M:%S", time_info);
	return 0;
}

int get_file_path(char *dir_path, char *file_name, char *buffer)
{
	if (realpath(dir_path, buffer) == NULL)
		return -1;
	strcat(buffer, "/");
	strcat(buffer, file_name);
	return 0;
}

void explore_directory(char *dir_path)
{
	DIR *directory = opendir(dir_path);
	struct dirent *dir_file = NULL;
	struct stat *file_status = malloc(sizeof(struct stat));
	int file_matches;
	char last_modify_date[256];
	char last_access_date[256];
	char file_path[256];
	if (directory == NULL)
		return;
	chdir(dir_path);
	errno = 0;
	dir_file = readdir(directory);
	while (dir_file != NULL)
	{
		if (strcmp(dir_file->d_name, ".") == 0 ||
			strcmp(dir_file->d_name, "..") == 0)
		{
			dir_file = readdir(directory);
			continue;
		}
		if (lstat(dir_file->d_name, file_status) != 0)
			break;
		file_matches = 0;
		switch (operator)
		{
			case '>':
				if (file_status->st_mtime > system_time)
					file_matches = 1;
				break;
			case '<':
				if (file_status->st_mtime < system_time)
					file_matches = 1;
				break;
			case '=':
				if (file_status->st_mtime > system_time - 60
					&& file_status->st_mtime < system_time + 60)
					file_matches = 1;
				break;
			default:
				break;
		}
		get_time_string(file_status->st_mtime, last_modify_date);
		get_time_string(file_status->st_atime, last_access_date);
		get_file_path(dir_path, dir_file->d_name, file_path);
		if (file_matches == 1)
		{
			printf("%s    %10d    %s    %s    %s\n",
				   get_file_type(file_status->st_mode),
				   (int) file_status->st_size,
				   last_access_date,
				   last_modify_date,
				   file_path
			);
		}
		if (S_ISDIR(file_status->st_mode) != 0 &&
			S_ISLNK(file_status->st_mode) == 0)
		{
			pid_t process_id = fork();
			if (process_id != 0)
			{
				explore_directory(file_path);
			}
			else
			{
				printf("PID: %5d, PATH: %s\n",
					   getpid(),file_path);
				run_ls(file_path);
			}
		}
		chdir(dir_path);
		dir_file = readdir(directory);
	}
	free(file_status);
	closedir(directory);
}

static int nftw_function(const char *file_path, const struct stat *file_status,
						 int type_flag, struct FTW *ftw_info)
{
	if (ftw_info->level == 0)
		return 0;
	char last_modify_date[256];
	char last_access_date[256];
	int file_matches = 0;
	switch (operator)
	{
		case '>':
			if (file_status->st_mtime > system_time)
				file_matches = 1;
			break;
		case '<':
			if (file_status->st_mtime < system_time)
				file_matches = 1;
			break;
		case '=':
			if (file_status->st_mtime > system_time - 60
				&& file_status->st_mtime < system_time + 60)
				file_matches = 1;
			break;
		default:
			break;
	}
	get_time_string(file_status->st_mtime, last_modify_date);
	get_time_string(file_status->st_atime, last_access_date);
	if (file_matches == 1)
	{
		printf("%s    %10d    %s    %s    %s\n",
			   get_file_type(file_status->st_mode),
			   (int) file_status->st_size,
			   last_access_date,
			   last_modify_date,
			   file_path
		);
	}
	if (S_ISDIR(file_status->st_mode) != 0 &&
		S_ISLNK(file_status->st_mode) == 0)
	{
		pid_t process_id = fork();
		if (process_id == 0)
		{
			printf("PID: %5d, PATH: %s\n",
				   getpid(),file_path);
			run_ls(file_path);
		}
	}
	return 0;
}

int main(int argc, char **argv)
{
	char *dir_path;
	if (argc != 5)
	{
		printf(
			"4 arguments expected: directory path, compare operator, time "
				"and operation mode! %i provided\n", argc - 1);
		return -1;
	}
	dir_path = argv[1];
	if (strlen(argv[2]) != 1 || (argv[2][0] != '>' && argv[2][0] != '<' &&
								 argv[2][0] != '='))
	{
		printf("Wrong compare operator provided! Should be: '<', '=', '>'.\n");
		return -1;
	}
	operator = argv[2][0];
	struct tm *time_to_cmp = malloc(sizeof(struct tm));
	if (strptime(argv[3], "%d.%m.%Y;%H:%M:%S", time_to_cmp) == NULL)
	{
		printf("Provide date in format DD.MM.YYYY;hh:mm:ss.\n");
		free(time_to_cmp);
		return -1;
	}
	system_time = mktime(time_to_cmp);
	dir_path = realpath(dir_path, NULL);
	if (strcmp(argv[4], "1") == 0)
		explore_directory(dir_path);
	if (strcmp(argv[4], "2") == 0)
		nftw(dir_path, nftw_function, 10, FTW_PHYS);
	free(time_to_cmp);
	wait(0);
	return 0;
}