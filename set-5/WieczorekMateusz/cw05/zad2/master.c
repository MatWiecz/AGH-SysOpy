#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include "unistd.h"

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("1 program argument expected: path of pipe file to create "
				   "Provided %d! Master aborted.\n",
			   argc - 1);
		return -1;
	}
	if(mkfifo(argv[1],S_IRWXU) == -1)
	{
		fprintf(stderr, "FATAL ERROR! Cannot create pipe file with provided "
			"path! Master aborted.\n");
		return -2;
	}
	int fifo_desc = open(argv[1], O_RDONLY);
	if(fifo_desc == -1 )
	{
		fprintf(stderr, "FATAL ERROR! Cannot open pipe file in read mode with "
			"provided path! Master aborted.\n");
		return -2;
	}
	size_t buffer_size = PIPE_BUF*sizeof(char);
	char * buffer = malloc (buffer_size);
	int parts_to_read = 10000;
	while (parts_to_read--)
	{
		if (read(fifo_desc, buffer, buffer_size) != 0 )
			printf("%s", buffer);
		else
			break;
	}
	close(fifo_desc);
	remove(argv[1]);
	free(buffer);
	printf("All writers disconnected! Master also terminated.\n");
	return 0;
}