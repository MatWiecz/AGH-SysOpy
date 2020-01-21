#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <wait.h>
#include <bits/types/siginfo_t.h>

int handled_signals_num;
int sigusr2_received;
int ready_to_send;
struct sigaction sigaction_for_main2_signal;
struct sigaction sigaction_for_ready_signal;

void main2_signal_handler(int signal_id, siginfo_t *extra_info, void *context)
{
	handled_signals_num++;
	sigval_t signal_value = extra_info->si_value;
	if (extra_info->si_code == SI_QUEUE)
		printf("Signal order id = %d!\n", signal_value.sival_int);
	ready_to_send = 1;
}

void ready_signal_handler(int signal_id, siginfo_t *extra_info, void *context)
{
	sigaction(SIGUSR1, &sigaction_for_main2_signal, NULL);
	ready_to_send = 1;
}

void end_signal_handler(int signal_id, siginfo_t *extra_info, void *context)
{
	sigusr2_received = 1;
}

void set_unready_state ()
{
	sigaction(SIGUSR1, &sigaction_for_ready_signal, NULL);
	ready_to_send = 0;
}

int main(int argc, char **argv)
{
	if (argc != 4)
	{
		printf("3 program arguments expected: catcher's PID, signals count,"
				   " signal sending mode. Provided %d! Sender aborted.\n",
			   argc - 1);
		return -1;
	}
	pid_t catcher_pid = (pid_t) strtol(argv[1], NULL, 10);
	if (catcher_pid == 0)
	{
		printf("Catcher's PID should be a PID of Catcher process! "
				   "Sender aborted.\n");
		return -1;
	}
	int signals_to_send = (int) strtol(argv[2], NULL, 10);
	if (signals_to_send == 0)
	{
		printf("Signals count should be positive integer number! "
				   "Sender aborted.\n");
		return -1;
	}
	int send_mode = 0;
	if (strcmp(argv[3], "KILL") == 0)
		send_mode = 1;
	if (strcmp(argv[3], "SIGQUEUE") == 0)
		send_mode = 2;
	if (strcmp(argv[3], "SIGRT") == 0)
		send_mode = 3;
	if (send_mode == 0)
	{
		printf("Signal sending mode should be one of these options: "
				   "KILL, SIGQUEUE, SIGRT! Sender aborted.\n");
		return -1;
	}
	handled_signals_num = 0;
	sigusr2_received = 0;
	ready_to_send = 1;
	sigset_t old_signal_set;
	sigset_t signal_set;
	sigfillset(&signal_set);
	sigdelset(&signal_set, SIGUSR1);
	sigdelset(&signal_set, SIGUSR2);
	sigdelset(&signal_set, SIGRTMIN);
	sigdelset(&signal_set, SIGRTMIN + 1);
	sigprocmask(SIG_SETMASK, &signal_set, &old_signal_set);
	struct sigaction sigaction_for_end_signal;
	sigaction_for_end_signal.sa_sigaction = end_signal_handler;
	sigfillset(&sigaction_for_end_signal.sa_mask);
	sigaction_for_end_signal.sa_flags = SA_SIGINFO;
	sigaction_for_main2_signal.sa_sigaction = main2_signal_handler;
	sigfillset(&sigaction_for_main2_signal.sa_mask);
	sigaction_for_main2_signal.sa_flags = SA_SIGINFO;
	sigaction_for_ready_signal.sa_sigaction = ready_signal_handler;
	sigfillset(&sigaction_for_ready_signal.sa_mask);
	sigaction_for_ready_signal.sa_flags = SA_SIGINFO;
	sigaction(SIGUSR1, &sigaction_for_main2_signal, NULL);
	sigaction(SIGUSR2, &sigaction_for_end_signal, NULL);
	sigaction(SIGRTMIN, &sigaction_for_main2_signal, NULL);
	sigaction(SIGRTMIN + 1, &sigaction_for_end_signal, NULL);
	printf("Sender's PID: %d.\n", getpid());
	switch (send_mode)
	{
		case 1:
		{
			for (int i = 0; i < signals_to_send; i++)
			{
				kill(catcher_pid, SIGUSR1);
				set_unready_state();
				while(!ready_to_send)
					pause();
			}
			kill(catcher_pid, SIGUSR2);
			set_unready_state();
			while(!ready_to_send)
				pause();
			break;
		}
		case 2:
		{
			sigval_t signal_value;
			signal_value.sival_ptr = (void *) 0x14B039CE75A28FD6;
			for (int i = 0; i < signals_to_send; i++)
			{
				sigqueue(catcher_pid, SIGUSR1, signal_value);
				set_unready_state();
				while(!ready_to_send)
					pause();
			}
			sigqueue(catcher_pid, SIGUSR2, signal_value);
			set_unready_state();
			while(!ready_to_send)
				pause();
			break;
		}
		case 3:
		{
			for (int i = 0; i < signals_to_send; i++)
			{
				kill(catcher_pid, SIGRTMIN);
				set_unready_state();
				while(!ready_to_send)
					pause();
			}
			kill(catcher_pid, SIGRTMIN + 1);
			set_unready_state();
			while(!ready_to_send)
				pause();
			break;
		}
		default:
			break;
	}
	while (!sigusr2_received)
	{
		pause();
		kill(catcher_pid, SIGUSR1);
	}
	printf("Sender: I've received %d signals SIGUSR1!\n"
			   "I should receive %d signals from Catcher!\n",
		   handled_signals_num, signals_to_send);
	return 0;
}