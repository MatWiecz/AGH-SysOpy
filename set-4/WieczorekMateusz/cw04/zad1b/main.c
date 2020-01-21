#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "time.h"
#include "unistd.h"

pid_t fork_ret_val;
void sigtstop_handler_pause (int);
void sigint_handler_terminate (int);
struct sigaction sigaction_pause;

void sigtstop_handler_pause (int signal_id)
{
	printf("\nSIGTSTP received, so script ");
	if ( fork_ret_val != 0)
	{
		printf("stopped!\n");
		kill(fork_ret_val, SIGKILL);
		int child_ret_val = 0;
		waitpid(fork_ret_val, &child_ret_val, 0);
		fork_ret_val = 0;
	}
	else
	{
		printf("resumed!\n");
		fork_ret_val = fork ();
		if (fork_ret_val == 0)
		{
			execlp ("bash", "bash", "./child-process.sh", NULL);
		}
	}
	fflush(stdout);
}

void sigint_handler_terminate (int signal_id)
{
	printf("\nSIGINT received, so process and script terminated!\n");
	if ( fork_ret_val != 0 )
	{
		kill(fork_ret_val, SIGKILL);
		int child_ret_val = 0;
		waitpid(fork_ret_val, &child_ret_val, 0);
		fork_ret_val = 0;
	}
	exit(0);
}

int main(int argc, char *argv[])
{
	fork_ret_val = fork ();
	if (fork_ret_val == 0)
	{
		execlp ("bash", "bash", "./child-process.sh", NULL);
	}
	signal (SIGINT, sigint_handler_terminate);
	sigaction_pause.sa_handler = sigtstop_handler_pause;
	sigemptyset(&sigaction_pause.sa_mask);
	sigaction_pause.sa_flags = 0;
	sigaction(SIGTSTP, &sigaction_pause, NULL);
	int counter = 0;
	while (counter < 1000000000)
	{
		sleep(1);
		counter++;
	}
	return 0;
}