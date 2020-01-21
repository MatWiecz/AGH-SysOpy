#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <time.h>
#include "unistd.h"

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		printf("2 program argument expected: path of pipe file to open, "
				   "count of messages. "
				   "Provided %d! Slave aborted.\n",
			   argc - 1);
		return -1;
	}
	int messages_num = (int) strtol (argv[2], NULL, 10);
	if (!messages_num)
	{
		printf("Messages count must be positive integer number!"
				   " Slave aborted.\n");
		return -1;
	}
	int fifo_desc = open(argv[1], O_WRONLY);
	if(fifo_desc == -1 )
	{
		fprintf(stderr, "FATAL ERROR! Cannot open pipe file in write mode with "
			"provided path! Slave aborted.\n");
		return -2;
	}
	size_t buffer_size = 64*sizeof(char);
	char * buffer = malloc (buffer_size);
	srand((unsigned int)time(NULL));
	FILE * date_cmd_pipe;
	printf("Slave's PID: %d\n", getpid());
	while (messages_num--)
	{
		if((date_cmd_pipe = popen("date", "r"))==NULL)
		{
			fprintf(stderr, "ERROR! An error occurred during"
				" calling popen.\n");
			break;
		}
		sprintf(buffer, "%5d: ", getpid());
		fread(buffer+7, 1, buffer_size-7, date_cmd_pipe);
		write(fifo_desc, buffer, buffer_size);
		if(pclose(date_cmd_pipe)!=0)
		{
			fprintf(stderr, "ERROR! An error occurred during"
				" calling pclose.\n");
			break;
		}
		sleep((unsigned int)(rand()%4)+2);
	}
	close(fifo_desc);
	free(buffer);
	return 0;
}