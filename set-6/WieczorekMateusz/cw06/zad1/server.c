#include "common.h"

#define MAX_CLIENT_CONNECTS 64

struct client_info
{
	int client_id;
	int private_queue;
	__uint64_t friends;
};

int request_queue = -1;
struct clients_connections_struct
{
	int clients_num;
	__uint64_t active_clients_mask;
	struct client_info clients[MAX_CLIENT_CONNECTS];
};
struct clients_connections_struct client_connections = {0};

int send_message_to_user(int user_id, struct chat_message *message)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	user_id--;
	if (user_id < 0 || user_id >= MAX_CLIENT_CONNECTS)
		return -3;
	if (client_connections.clients[user_id].client_id == 0)
		return -4;
	return send_chat_message(client_connections.clients[user_id].private_queue,
							 message);
}

int send_response(int user_id, struct chat_message *request, int status)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	if (user_id <= MAX_CLIENT_CONNECTS)
	{
		user_id--;
		if (user_id < 0 || user_id >= MAX_CLIENT_CONNECTS)
			return -3;
		if (client_connections.clients[user_id].client_id == 0)
			return -4;
	}
	struct mt_response *temp_mt_response = alloc_mt_response(request,
															 status);
	struct chat_message temp_message;
	prepare_message(&temp_message, MESSAGE_TYPE_RESPONSE,
					temp_mt_response);
	int target_queue;
	if (user_id <= MAX_CLIENT_CONNECTS)
		target_queue = client_connections.clients[user_id].private_queue;
	else
		target_queue = user_id;
	if (send_chat_message(target_queue, &temp_message) == -1)
	{
		fprintf(stderr, "ERROR! Cannot put chat message in user's private "
			"queue! UserId: %d\n", user_id + 1);
		return -1;
	}
	return 0;
}

int register_new_user(struct chat_message *message)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	struct mt_init *temp_mt_init = (struct mt_init *) message->data;
	key_t private_key = temp_mt_init->ipc_key;
	int cur_id = 0;
	while (cur_id < client_connections.clients_num
		   && client_connections.clients[cur_id].client_id != 0)
		cur_id++;
	if (cur_id == MAX_CLIENT_CONNECTS)
	{
		fprintf(stderr, "ERROR! List for client's connections is full, so "
			"cannot register new user with key %d!\n", private_key);
		int temp_queue = msgget(private_key, 0);
		if (temp_queue != -1)
			send_response(temp_queue, message, 0);
		return -1;
	}
	if ((client_connections.clients[cur_id].private_queue =
			 msgget(private_key, 0)) == -1)
	{
		fprintf(stderr, "ERROR! Cannot register new user with private key "
			"%d!\n", private_key);
		return -2;
	}
	client_connections.clients[cur_id].client_id = cur_id + 1;
	client_connections.clients[cur_id].friends = 0;
	client_connections.active_clients_mask |= SEND_2ONE(cur_id+1);
	cur_id++;
	if (cur_id > client_connections.clients_num)
		client_connections.clients_num = cur_id;
	send_response(cur_id, message, cur_id);
	return cur_id;
}

int unregister_user(struct chat_message *message)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	int user_id = message->sender_id;
	struct chat_message temp_message;
	prepare_message(&temp_message, MESSAGE_TYPE_STOP,
					NULL);
	send_message_to_user(user_id, &temp_message);
	send_response(user_id, message, 200);
	user_id--;
	if (client_connections.clients[user_id].client_id == 0 )
		return -1;
	client_connections.clients[user_id].client_id = 0;
	client_connections.clients[user_id].friends = 0;
	client_connections.clients[user_id].private_queue = 0;
	client_connections.active_clients_mask &= ~SEND_2ONE(user_id+1);
	for(int i = client_connections.clients_num - 1; i >= 0; i--)
		if(client_connections.clients[i].client_id == 0)
			client_connections.clients_num = i;
		else
			break;
	return 0;
}

int unregister_all ()
{
	struct chat_message temp_message;
	prepare_message(&temp_message, MESSAGE_TYPE_STOP,
					NULL);
	for (int i = 0; i < client_connections.clients_num; i++)
		if (client_connections.clients[i].client_id != 0)
			send_message_to_user(i+1, &temp_message);
	client_connections.clients_num = 0;
	return 0;
}

int process_stop_request(struct chat_message *message)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	if (unregister_user(message) < 0)
		return -1;
	return 0;
}

int process_init_request(struct chat_message *message)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	if (register_new_user(message) < 0)
		return -1;
	return 0;
}

int send_echo(int user_id, char *text)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	struct mt_echo *temp_mt_echo = alloc_mt_echo(text);
	struct chat_message temp_message;
	prepare_message(&temp_message, MESSAGE_TYPE_ECHO,
					temp_mt_echo);
	send_message_to_user(user_id, &temp_message);
	return 0;
}

int process_echo_request(struct chat_message *message)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	time_t sys_time;
	struct tm *time_info;
	sys_time = time(NULL);
	time_info = localtime(&sys_time);
	char text[TEXT_MAX_LENGTH] = "";
	strftime(text, TEXT_MAX_LENGTH-1, "%d.%m.%Yr. %H:%M:%S: ", time_info);
	struct mt_echo *temp_mt_echo = (struct mt_echo *) message->data;
	strcat(text, temp_mt_echo->text);
	text[strlen(text) - 1] = '\0';
	send_echo(message->sender_id, text);
	send_response(message->sender_id, message, 200);
	return 0;
}

int process_list_request(struct chat_message *message)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	char text[TEXT_MAX_LENGTH] = "List of active users:";
	int coma = 0;
	for (int i = 0; i < client_connections.clients_num; i++)
		if (client_connections.clients[i].client_id != 0)
		{
			if (coma)
				strcat(text, ",");
			char id_str[8] = "";
			sprintf(id_str, " %d", client_connections.clients[i].client_id);
			strcat(text, id_str);
			coma = 1;
		}
	strcat(text, ".");
	send_echo(message->sender_id, text);
	send_response(message->sender_id, message, 200);
	return 0;
}

int process_friends_request(struct chat_message *message)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	struct mt_friends *temp_mt_friends = (struct mt_friends *) message->data;
	int user_id = message->sender_id;
	user_id--;
	switch (temp_mt_friends->op_type)
	{
		case FRIENDS_SET:
		{
			client_connections.clients[user_id].friends =
				temp_mt_friends->friends_mask;
			break;
		}
		case FRIENDS_ADD:
		{
			client_connections.clients[user_id].friends |=
				temp_mt_friends->friends_mask;
			break;
		}
		case FRIENDS_DEL:
		{
			client_connections.clients[user_id].friends &=
				~temp_mt_friends->friends_mask;
			break;
		}
		default:
			break;
	}
	client_connections.clients[user_id].friends &=
		client_connections.active_clients_mask;
	send_response(message->sender_id, message, 200);
	return 0;
}

int process_send_request(struct chat_message *message)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	struct mt_send *temp_mt_send = (struct mt_send *) message->data;
	
	int user_id = message->sender_id;
	user_id--;
	time_t sys_time;
	struct tm *time_info;
	sys_time = time(NULL);
	time_info = localtime(&sys_time);
	char text[TEXT_MAX_LENGTH] = "";
	strftime(text, TEXT_MAX_LENGTH-1, "%d.%m.%Yr. %H:%M:%S: Message from "
		"user", time_info);
	char id_str[8] = "";
	sprintf(id_str, " %d", user_id+1);
	strcat(text, id_str);
	strcat(text, ": ");
	strcat(text, temp_mt_send->text);
	text[strlen(text) - 1] = '\0';
	int message_sent = 0;
	switch(temp_mt_send->target)
	{
		case SEND_2ALL:
		{
			for (int i = 0; i < client_connections.clients_num; i++)
				if (client_connections.clients[i].client_id != 0
					&& i != user_id)
					send_echo(i+1, text);
			message_sent = 1;
			break;
		}
		case SEND_2FRIENDS:
		{
			for (int i = 0; i < client_connections.clients_num; i++)
				if (client_connections.clients[i].client_id != 0
					&& i != user_id)
				{
					__uint64_t cur_user_mask = SEND_2ONE(i + 1);
					if (client_connections.clients[user_id].friends &
						cur_user_mask)
						send_echo(i + 1, text);
				}
			message_sent = 1;
			break;
		}
		default:
		{
			for (int i = 0; i < client_connections.clients_num; i++)
				if (client_connections.clients[i].client_id != 0
					&& i != user_id)
				{
					__uint64_t cur_user_mask = SEND_2ONE(i + 1);
					if (temp_mt_send->target == cur_user_mask)
					{
						send_echo(i + 1, text);
						message_sent = 1;
						break;
					}
				}
			break;
		}
	}
	if(message_sent)
		send_response(message->sender_id, message, 200);
	else
		send_response(message->sender_id, message, 404);
	return 0;
}

void server_engine()
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	int loop_on = 1;
	while (loop_on)
	{
		usleep(USLEEP_TIME);
		struct chat_message cur_message;
		if (receive_chat_message(request_queue, &cur_message) != 0)
			continue;
		switch (cur_message.message_type)
		{
			case MESSAGE_TYPE_STOP:
			{
				process_stop_request(&cur_message);
				break;
			}
			case MESSAGE_TYPE_INIT:
			{
				process_init_request(&cur_message);
				break;
			}
			case MESSAGE_TYPE_ECHO:
			{
				process_echo_request(&cur_message);
				break;
			}
			case MESSAGE_TYPE_LIST:
			{
				process_list_request(&cur_message);
				break;
			}
			case MESSAGE_TYPE_FRIENDS:
			{
				process_friends_request(&cur_message);
				break;
			}
			case MESSAGE_TYPE_SEND:
			{
				process_send_request(&cur_message);
				break;
			}
			default:
				loop_on = 0;
		}
	}
}

void atexit_action()
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	fprintf(stdout, "\nServer terminated.\n");
	fflush(stdout);
	msgctl(request_queue, IPC_RMID, NULL);
	free_whole_mem();
}

void sigint_handler(int signal_id)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	unregister_all();
	exit(0);
}

int main(int argc, char *argv[])
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	
	chat_instance_id = 0;
	char *home_dir_path = getenv("HOME");
	key_t unique_key = ftok(home_dir_path, PROJECT_IDENTIFIER);
	request_queue = msgget(unique_key, IPC_CREAT | IPC_EXCL | 0666);
	if (request_queue == -1)
	{
		fprintf(stderr, "FATAL ERROR! Cannot create message queue for "
			"requests! Chat Server aborted.\n");
		free_whole_mem();
		exit(-1);
	}
	atexit(atexit_action);
	signal(SIGINT, sigint_handler);
	server_engine();
	exit(0);
}