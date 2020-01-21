#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <wait.h>
#include "unistd.h"

# define COMMAND_MAX_SIZE 1024
# define PROCESSES_MAX_NUM 16

void convert_command(char *command, char **cmd_bricks_ptr, int *bricks_num)
{
	int buf_cur = 0;
	int proc_args_ptr = *bricks_num - 1;
	int arg_ptr = 0;
	int last_pipe = 1;
	int last_white = 1;
	while (command[buf_cur] != '\0' && command[buf_cur] != '\n')
	{
		if (command[buf_cur] == ' ' || command[buf_cur] == '\t')
		{
			command[buf_cur] = '\0';
			last_white = 1;
		}
		else
		{
			if (command[buf_cur] == '|')
			{
				command[buf_cur] = '\0';
				last_pipe = 1;
			}
			else
			{
				if (last_pipe)
				{
					cmd_bricks_ptr[arg_ptr] = NULL;
					arg_ptr++;
					cmd_bricks_ptr[proc_args_ptr] =
						(char *) &cmd_bricks_ptr[arg_ptr];
					proc_args_ptr--;
					cmd_bricks_ptr[arg_ptr] = &command[buf_cur];
					arg_ptr++;
					last_pipe = 0;
					last_white = 0;
				}
				if (last_white)
				{
					cmd_bricks_ptr[arg_ptr] = &command[buf_cur];
					arg_ptr++;
					last_white = 0;
				}
			}
		}
		buf_cur++;
	}
	*bricks_num = *bricks_num - 1 - proc_args_ptr;
	command[buf_cur] = '\0';
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("1 program argument expected: path of file with commands. "
				   "Provided %d! Interpreter aborted.\n",
			   argc - 1);
		return -1;
	}
	FILE *commands_file = NULL;
	{
		char *task_file_path = argv[1];
		commands_file = fopen(task_file_path, "r");
		if (commands_file == NULL)
		{
			printf("Cannot open file with provided path!"
					   " Interpreter aborted.\n");
			return -1;
		}
	}
	char row_buffer[COMMAND_MAX_SIZE] = "";
	while (1)
	{
		if (fgets(row_buffer, COMMAND_MAX_SIZE * sizeof(char),
				  commands_file) == NULL)
		{
			printf("All commands from provided file have been executed!\n");
			break;
		}
		printf("my-interpreter: Command: %s\n", row_buffer);
		char *cmd_bricks[256];
		int procs_num = 256;
		for (int i = 0; i < procs_num; i++)
			cmd_bricks[i] = NULL;
		convert_command(row_buffer, cmd_bricks, &procs_num);
		int cur_pipe[2];
		int prev_pipe[2];
		pid_t procs[PROCESSES_MAX_NUM];
		for (int i = 0; i < procs_num; i++)
		{
//			int j = 0;
//			while (((char **) cmd_bricks[255 - i])[j] != NULL)
//			{
//				printf("%s ", ((char **) cmd_bricks[255 - i])[j]);
//				j++;
//			}
//			printf("NULL \n");
			if (pipe(cur_pipe) == -1)
			{
				fprintf(stderr, "FATAL ERROR! Cannot create new pipe!"
					" Interpreter aborted.\n");
				exit(-2);
			}
			procs[i] = fork();
			if (procs[i] == 0)
			{
				if (i > 0)
				{
					dup2(prev_pipe[0], STDIN_FILENO);
					close(prev_pipe[0]);
					close(prev_pipe[1]);
				}
				if (i + 1 < procs_num)
				{
					dup2(cur_pipe[1], STDOUT_FILENO);
					close(cur_pipe[0]);
					close(cur_pipe[1]);
				}
				execvp(((char **) cmd_bricks[255 - i])[0],
					   (char **) cmd_bricks[255 - i]);
				exit(-3);
			}
			else
				if (procs[i] > 0)
				{
					if (i > 0)
					{
						close(prev_pipe[0]);
						close(prev_pipe[1]);
					}
					if (i + 1 < procs_num)
					{
						prev_pipe[0] = cur_pipe[0];
						prev_pipe[1] = cur_pipe[1];
					}
					else
					{
						close(cur_pipe[0]);
						close(cur_pipe[1]);
					}
				}
		}
		int return_value;
		for (int i = 0; i < procs_num; i++)
			waitpid(procs[i], &return_value, 0);
	}
	fclose(commands_file);
	return 0;
}