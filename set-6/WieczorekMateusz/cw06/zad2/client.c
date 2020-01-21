#include "common.h"

char private_queue_path [64] = "";
mqd_t private_message_queue = -1;
mqd_t server_request_queue = -1;
int client_initiated = 0;
int loop_on = 1;
int wait_for_response = 0;
int is_sender = -1;

void send_request(struct chat_message *request)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	if (send_chat_message(server_request_queue, request) == -1)
	{
		fprintf(stderr, "ERROR! Cannot put chat message in server's request "
			"queue!\n");
	}
	else
		wait_for_response = 1;
}

void init_client(char * self_queue_path)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	struct mt_init *temp_mt_init = alloc_mt_init(self_queue_path);
	struct chat_message temp_message;
	prepare_message(&temp_message, MESSAGE_TYPE_INIT, temp_mt_init);
	send_request(&temp_message);
}

int finish_init_client(struct chat_message *message)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	struct mt_response *temp_mt_response = (struct mt_response *) message->data;
	if (temp_mt_response->message_id != 0 || temp_mt_response->status == 0)
	{
		fprintf(stderr, "ERROR! Cannot register client in local sever!\n"
					"messageId = %d, status = %d\n",
				temp_mt_response->message_id, temp_mt_response->status);
		client_initiated = -1;
		return -1;
	}
	chat_instance_id = temp_mt_response->status;
	client_initiated = 1;
	fprintf(stdout, "User's identifier: %d\n", chat_instance_id);
	return 0;
}

int process_server_response(struct chat_message *message)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	struct mt_response *temp_mt_response = (struct mt_response *) message->data;
	fprintf(stdout, "\rSERVER: Command #%d processed with "
		"status %d.\n", temp_mt_response->message_id, temp_mt_response->status);
	kill(getppid(), SIGUSR1);
	return 0;
}

int process_stop_command()
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	kill(getppid(), SIGINT);
	loop_on = 0;
	return 0;
}

int process_echo_command(struct chat_message *message)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	struct mt_echo *temp_mt_echo = (struct mt_echo *) message->data;
	fprintf(stdout, "\rSERVER: %s\n", temp_mt_echo->text);
	kill(getppid(),SIGUSR2);
	return 0;
}

void message_receiver(int one_time)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	struct chat_message cur_message;
	int loop_on2 = 1;
	while (loop_on && loop_on2)
	{
		usleep(USLEEP_TIME);
		if (receive_chat_message(private_message_queue, &cur_message) == 0)
		{
			switch (cur_message.message_type)
			{
				case MESSAGE_TYPE_RESPONSE:
				{
					if (!client_initiated)
						finish_init_client(&cur_message);
					else
						process_server_response(&cur_message);
					break;
				}
				case MESSAGE_TYPE_STOP:
				{
					process_stop_command();
					break;
				}
				case MESSAGE_TYPE_ECHO:
				{
					process_echo_command(&cur_message);
					break;
				}
				default:
					loop_on = 0;
					break;
			}
			if (one_time)
				loop_on2 = 0;
		}
	}
}

int send_echo()
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	char text[TEXT_MAX_LENGTH] = "";
	fgets(text, 2, stdin);
	fgets(text, TEXT_MAX_LENGTH - 1, stdin);
	struct mt_echo *temp_mt_echo = alloc_mt_echo(text);
	struct chat_message temp_message;
	prepare_message(&temp_message, MESSAGE_TYPE_ECHO,
					temp_mt_echo);
	send_request(&temp_message);
	return 0;
}

int send_list()
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	struct chat_message temp_message;
	prepare_message(&temp_message, MESSAGE_TYPE_LIST,
					NULL);
	send_request(&temp_message);
	return 0;
}

int send_friends()
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	char text[TEXT_MAX_LENGTH] = "";
	fgets(text, 1, stdin);
	fgets(text, TEXT_MAX_LENGTH - 1, stdin);
	struct mt_friends *temp_mt_friends = alloc_mt_friends(text);
	struct chat_message temp_message;
	prepare_message(&temp_message, MESSAGE_TYPE_FRIENDS,
					temp_mt_friends);
	send_request(&temp_message);
	return 0;
}

int send_send(__uint64_t pretarget)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	int one_client = 0;
	if(pretarget == SEND_2ONE(1))
	{
		fscanf(stdin, "%d", &one_client);
		if (one_client == 0)
		{
			fprintf(stdout, "WARNING! Target user identifier must be "
				"positive integer number.\n");
			return -1;
		}
		pretarget = SEND_2ONE(one_client);
	}
	char text[TEXT_MAX_LENGTH] = "";
	fgets(text, 2, stdin);
	fgets(text, TEXT_MAX_LENGTH - 1, stdin);
	struct mt_send *temp_mt_send = alloc_mt_send(pretarget, text);
	struct chat_message temp_message;
	prepare_message(&temp_message, MESSAGE_TYPE_SEND,
					temp_mt_send);
	send_request(&temp_message);
	return 0;
}

int send_stop()
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	struct chat_message temp_message;
	prepare_message(&temp_message, MESSAGE_TYPE_STOP,
					NULL);
	send_request(&temp_message);
	loop_on = 0;
	return 0;
}

int get_user_input();

int execute_file_commands()
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	char file_path [256] = "";
	fscanf(stdin, "%s", file_path);
	FILE * command_file = fopen(file_path, "r");
	if(command_file == NULL)
	{
		fprintf(stdout, "Cannot open command file \"%s\"!\n", file_path);
		return -1;
	}
	FILE * old_stdin = stdin;
	stdin = command_file;
	while(!get_user_input());
	stdin = old_stdin;
	fclose(command_file);
	return 0;
}

int execute_command(char *cmd_str)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	if(send_messages_count>=COMMANDS_MAX_COUNT)
		return -2;
	if (strcmp(cmd_str, "ECHO") == 0)
	{
		send_echo();
		return 0;
	}
	if (strcmp(cmd_str, "LIST") == 0)
	{
		send_list();
		return 0;
	}
	if (strcmp(cmd_str, "FRIENDS") == 0)
	{
		send_friends();
		return 0;
	}
	if (strcmp(cmd_str, "2ALL") == 0)
	{
		send_send(SEND_2ALL);
		return 0;
	}
	if (strcmp(cmd_str, "2FRIENDS") == 0)
	{
		send_send(SEND_2FRIENDS);
		return 0;
	}
	if (strcmp(cmd_str, "2ONE") == 0)
	{
		send_send(SEND_2ONE(1));
		return 0;
	}
	if (strcmp(cmd_str, "STOP") == 0)
	{
		send_stop();
		return 0;
	}
	fprintf(stdout, "%s: Unrecognized command.\n", cmd_str);
	return -1;
}

int get_user_input()
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	char cmd_str[64] = "";
	if (fscanf(stdin, "%s", cmd_str) != 1)
		return -1;
	if (strcmp(cmd_str, "READ") == 0)
		execute_file_commands();
	else
		execute_command(cmd_str);
	return 0;
}

void message_sender()
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	wait_for_response = 0;
	while (send_messages_count<COMMANDS_MAX_COUNT)
	{
		fprintf(stdout, "%d> ", send_messages_count);
		fflush(stdout);
		get_user_input();
		while(wait_for_response)
			usleep(USLEEP_TIME);
	}
	fprintf(stdout, "Limit of %d commands reached!\n", COMMANDS_MAX_COUNT);
	send_stop();
}

void atexit_action()
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	if(is_sender)
	{
		fprintf(stdout, "\nClient terminated.\n");
		fflush(stdout);
	}
	//TODOok:
	mq_close(private_message_queue);
	if(is_sender)
		while(mq_unlink(private_queue_path));
	free_whole_mem();
}

void sigint_parent_handler(int signal_id)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	exit(0);
}

void sigint_child_handler(int signal_id)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	send_stop();
}

void sigusr1_handler(int signal_id)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	wait_for_response = 0;
}

void sigusr2_handler(int signal_id)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	fflush(stdout);
}

int main(int argc, char *argv[])
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	//TODOok:
	struct mq_attr message_queue_attr;
	message_queue_attr.mq_flags = O_NONBLOCK;
	message_queue_attr.mq_maxmsg = QUEUE_MAX_SIZE_IN_MESSAGES;
	message_queue_attr.mq_msgsize = MESSAGE_SIZE;
	message_queue_attr.mq_curmsgs = 0;
	sprintf(private_queue_path, "/%08x", PROJECT_IDENTIFIER ^ getpid());
	private_message_queue = mq_open(private_queue_path,
							O_CREAT | O_EXCL | O_RDONLY | O_NONBLOCK, 0666,
							& message_queue_attr);
	if (private_message_queue == -1)
	{
		perror("mq_open()");
		fprintf(stderr, "FATAL ERROR! Cannot create message queue for "
			"messages from server! Chat Client aborted.\n");
		free_whole_mem();
		exit(-1);
	}
	atexit(atexit_action);
	//TODOok:
	char queue_path [64] = "";
	sprintf(queue_path, "/%08x", PROJECT_IDENTIFIER);
	server_request_queue = mq_open(queue_path, O_WRONLY | O_NONBLOCK);
	if (server_request_queue == -1)
	{
		fprintf(stderr, "FATAL ERROR! Cannot get server's message queue for "
			"requests! Chat Client aborted.\n");
		exit(0);
	}
	init_client(private_queue_path);
	message_receiver(1);
	if (client_initiated != 1)
		exit(0);
	pid_t pid = fork();
	if (pid != 0)
	{
		{
			struct sigaction action_struct;
			action_struct.sa_handler = sigint_parent_handler;
			action_struct.sa_flags = 0;
			sigaction(SIGINT, &action_struct, NULL);
		}
		{
			struct sigaction action_struct;
			action_struct.sa_handler = sigusr1_handler;
			action_struct.sa_flags = 0;
			sigaction(SIGUSR1, &action_struct, NULL);
		}
		{
			struct sigaction action_struct;
			action_struct.sa_handler = sigusr2_handler;
			action_struct.sa_flags = 0;
			sigaction(SIGUSR2, &action_struct, NULL);
		}
		is_sender = 1;
		message_sender();
	}
	else
	{
		struct sigaction action_struct;
		action_struct.sa_handler = sigint_child_handler;
		sigemptyset(&action_struct.sa_mask);
		sigaddset(&action_struct.sa_mask, SIGINT);
		action_struct.sa_flags = 0;
		sigaction(SIGINT, &action_struct, NULL);
		is_sender = 0;
		message_receiver(0);
	}
	exit(0);
}