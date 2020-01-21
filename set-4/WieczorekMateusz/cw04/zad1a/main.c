#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include "time.h"
#include "unistd.h"

void sigtstop_handler_pause (int);
void sigtstop_handler_continue (int);
void sigint_handler_terminate (int);
sigset_t old_signal_set;
struct sigaction sigaction_pause;
struct sigaction sigaction_continue;

void sigtstop_handler_pause (int signal_id)
{
	printf("\nSIGTSTP received, so process stopped!\n"
			   "\tWaiting for: \n"
			   "\tCTRL+Z - continuation\n"
			   "\tCTRL+C - termination\n");
	sigset_t new_signal_set;
	sigfillset(&new_signal_set);
	sigdelset(&new_signal_set, SIGTSTP);
	sigdelset(&new_signal_set, SIGINT);
	sigprocmask(SIG_SETMASK, &new_signal_set, &old_signal_set);
	sigaction(SIGTSTP, &sigaction_continue, NULL);
	pause();
}

void sigtstop_handler_continue (int signal_id)
{
	printf("\nSIGTSTP received, so process resumed!\n");
	sigprocmask(SIG_SETMASK, &old_signal_set, NULL);
	sigaction(SIGTSTP, &sigaction_pause, NULL);
}

void sigint_handler_terminate (int signal_id)
{
	printf("\nSIGINT received, so process terminated!\n");
	exit(0);
}

int main(int argc, char *argv[])
{
	signal (SIGINT, sigint_handler_terminate);
	sigaction_pause.sa_handler = sigtstop_handler_pause;
	sigemptyset(&sigaction_pause.sa_mask);
	sigaction_pause.sa_flags = 0;
	sigaction_continue.sa_handler = sigtstop_handler_continue;
	sigemptyset(&sigaction_continue.sa_mask);
	sigaction_continue.sa_flags = 0;
	sigaction(SIGTSTP, &sigaction_pause, NULL);
	time_t sys_time;
	struct tm *time_info;
	int counter = 0;
	char time_str[80] = "";
	while (counter < 1000000000)
	{
		sys_time = time(NULL);
		time_info = localtime(&sys_time);
		strftime(time_str, 79, "%d.%m.%Yr. %H:%M:%S", time_info);
		printf("%s\n", time_str);
		sleep(1);
		counter++;
	}
	return 0;
}