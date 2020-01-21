#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <libgen.h>
#include <time.h>

int main(int argc, char ** argv) {
	if(argc != 5)
	{
		printf("4 program argument expected: file_path, pmin, pmax, bytes. "
				   "Provided %d! Tester aborted.\n",
			   argc - 1);
		return -1;
	}
	FILE * file;
	file = fopen(argv[1], "a+");
	if(file == NULL) {
		printf("Cannot open file with path \"%s\"! Tester aborted.\n", argv[1]);
		return -2;
	}
	srand(time(NULL));
	int pmin, pmax, bytes;
	sscanf(argv[2], "%d", &pmin);
	sscanf(argv[3], "%d", &pmax);
	sscanf(argv[4], "%d", &bytes);
	char destination[257];
	char seconds[128];
	char time_string[128];
	char process_id[15];
	int cur_secs;
	struct tm *local_time;
	int i;
	int j = 0;
	
	while(j++ < 1000) {
		cur_secs = rand()%(pmax - pmin + 1) + pmin;
		sleep(cur_secs);
		time_t cur_time = time((time_t*)0);
		local_time = localtime(&cur_time);
		strftime(time_string, 128, "%c", local_time);
		sprintf(seconds, "%d", cur_secs);
		sprintf(process_id, "%d", getpid());
		strcpy(destination, process_id);
		strcat(destination, " ");
		strcat(destination, seconds);
		strcat(destination, " ");
		strcat(destination, time_string);
		fseek(file, 0, SEEK_END);
		fputs(destination, file);
		i = 0;
		fputs(" ",file);
		while(i < bytes){
			fputs("?",file);
			i++;
		}
		fputs("\n",file);
	}
	fclose(file);
	return 0;
}