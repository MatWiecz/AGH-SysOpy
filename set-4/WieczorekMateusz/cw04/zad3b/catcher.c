#include <stdio.h>
#include <unistd.h>
#include <wait.h>

int handled_signals_num;
int sigusr2_received;
pid_t sender_pid;
int send_mode;
int ready_to_send;
struct sigaction sigaction_for_main2_signal;
struct sigaction sigaction_for_ready_signal;

void main2_signal_handler(int signal_id, siginfo_t *extra_info, void *context)
{
	handled_signals_num++;
	sender_pid = extra_info->si_pid;
}

void ready_signal_handler(int signal_id, siginfo_t *extra_info, void *context)
{
	sigaction(SIGUSR1, &sigaction_for_main2_signal, NULL);
	ready_to_send = 1;
}

void end_signal_handler(int signal_id, siginfo_t *extra_info, void *context)
{
	sigusr2_received = 1;
	sigval_t signal_value = extra_info->si_value;
	if (signal_value.sival_ptr == (void *) 0x14B039CE75A28FD6)
		send_mode = 2;
}

void end2_signal_handler(int signal_id, siginfo_t *extra_info, void *context)
{
	sigusr2_received = 1;
	sender_pid = extra_info->si_pid;
	send_mode = 3;
}

void set_unready_state ()
{
	sigaction(SIGUSR1, &sigaction_for_ready_signal, NULL);
	ready_to_send = 0;
}

int main(int argc, char **argv)
{
	handled_signals_num = 0;
	sigusr2_received = 0;
	sender_pid = 0;
	send_mode = 1;
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
	struct sigaction sigaction_for_end2_signal;
	sigaction_for_end2_signal.sa_sigaction = end2_signal_handler;
	sigfillset(&sigaction_for_end2_signal.sa_mask);
	sigaction_for_end2_signal.sa_flags = SA_SIGINFO;
	sigaction_for_ready_signal.sa_sigaction = ready_signal_handler;
	sigfillset(&sigaction_for_ready_signal.sa_mask);
	sigaction_for_ready_signal.sa_flags = SA_SIGINFO;
	sigaction(SIGUSR1, &sigaction_for_main2_signal, NULL);
	sigaction(SIGUSR2, &sigaction_for_end_signal, NULL);
	sigaction(SIGRTMIN, &sigaction_for_main2_signal, NULL);
	sigaction(SIGRTMIN + 1, &sigaction_for_end2_signal, NULL);
	printf("Catcher's PID: %d.\n", getpid());
	while (!sigusr2_received)
	{
		pause();
		kill(sender_pid, SIGUSR1);
	}
	usleep(1000);
	switch (send_mode)
	{
		case 1:
		{
			for (int i = 0; i < handled_signals_num; i++)
			{
				kill(sender_pid, SIGUSR1);
				set_unready_state();
				while(!ready_to_send)
					pause();
			}
			kill(sender_pid, SIGUSR2);
			set_unready_state();
			while(!ready_to_send)
				pause();
			break;
		}
		case 2:
		{
			int i = 0;
			sigval_t signal_value;
			signal_value.sival_ptr = (void *) 0x14B039CE75A28FD6;
			for (; i < handled_signals_num; i++)
			{
				signal_value.sival_int = i;
				sigqueue(sender_pid, SIGUSR1, signal_value);
				set_unready_state();
				while(!ready_to_send)
					pause();
			}
			sigqueue(sender_pid, SIGUSR2, signal_value);
			set_unready_state();
			while(!ready_to_send)
				pause();
			break;
		}
		case 3:
		{
			for (int i = 0; i < handled_signals_num; i++)
			{
				kill(sender_pid, SIGRTMIN);
				set_unready_state();
				while(!ready_to_send)
					pause();
			}
			kill(sender_pid, SIGRTMIN + 1);
			set_unready_state();
			while(!ready_to_send)
				pause();
			break;
		}
		default:
			break;
	}
	printf("Catcher: I've received %d signals SIGUSR1!\n",
		   handled_signals_num);
	return 0;
}