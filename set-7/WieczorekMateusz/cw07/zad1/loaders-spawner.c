#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>


int main(int argc, char *argv[])
{
	
	if (argc != 4)
	{
		fprintf(stderr, "Wrong number of arguments! Expected 3: loaders "
			"number, min loader's strength, max loader's strength.\n");
		exit(-1);
	}
	unsigned int loaders_num = (unsigned int) strtol(argv[1], NULL, 10);
	unsigned int min_loader_strength =
		(unsigned int) strtol(argv[2], NULL, 10);
	unsigned int max_loader_strength =
		(unsigned int) strtol(argv[3], NULL, 10);
	if (loaders_num < 1 || min_loader_strength < 1 ||
		max_loader_strength < 1)
	{
		fprintf(stderr, "Arguments should be positive integer numbers!\n");
		exit(-2);
	}
	time_t current_time = time(NULL);
	pid_t *loaders_table = calloc(loaders_num, sizeof(pid_t));
	for (unsigned int i = 0; i < loaders_num; i++)
	{
		loaders_table[i] = fork();
		if (loaders_table[i] == 0)
		{
			free(loaders_table);
			srand((unsigned int) current_time + i);
			unsigned int loader_strength = rand() % (max_loader_strength -
													 min_loader_strength + 1) +
										   min_loader_strength;
			char strength_str[16] = "";
			sprintf(strength_str, "%ud", loader_strength);
			execl("./sv-loader", "./sv-loader", strength_str, NULL);
		}
	}
	int loader_return_value = 0;
	for (unsigned int i = 0; i < loaders_num; i++)
		waitpid(loaders_table[i], &loader_return_value, NULL);
	free(loaders_table);
	exit(0);
}