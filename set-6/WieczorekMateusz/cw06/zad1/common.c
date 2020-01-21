#include "common.h"


struct alloc_table_struct alloc_table = {0};
int chat_instance_id = -1;
int send_messages_count = 0;

int prepare_message(struct chat_message *message_ptr,
					long type, void *data)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	message_ptr->message_type = type;
	message_ptr->sender_id = chat_instance_id;
	message_ptr->message_id = send_messages_count++;
	size_t data_size = 0;
	switch (type)
	{
		case MESSAGE_TYPE_RESPONSE:
		{
			data_size = sizeof(struct mt_response);
			break;
		}
		case MESSAGE_TYPE_INIT:
		{
			data_size = sizeof(struct mt_init);
			break;
		}
		case MESSAGE_TYPE_ECHO:
		{
			data_size = sizeof(struct mt_echo);
			break;
		}
		case MESSAGE_TYPE_FRIENDS:
		{
			data_size = sizeof(struct mt_friends);
			break;
		}
		case MESSAGE_TYPE_SEND:
		{
			data_size = sizeof(struct mt_send);
			break;
		}
		default:
		{
			data_size = 0;
			break;
		}
	}
	if (!data_size)
		return 0;
	memcpy(message_ptr->data, data, data_size);
	return 0;
}

int send_chat_message(int message_queue, struct chat_message *message)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	if (message_queue == -1)
		return -1;
	if (msgsnd(message_queue, message,
			   sizeof(struct chat_message) - sizeof(long), 0) == -1)
		return -2;
	return 0;
}

int receive_chat_message(int message_queue, struct chat_message *message)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	if (message_queue == -1)
		return -1;
	if (msgrcv(message_queue, message, sizeof(struct chat_message) -
									   sizeof(long), -1000, IPC_NOWAIT) == -1)
		return -2;
	return 0;
}

struct mt_response *alloc_mt_response(struct chat_message *request,
									  int status)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	struct mt_response *new_struct = alloc_mem(sizeof(struct mt_response));
	new_struct->message_id = request->message_id;
	new_struct->status = status;
	return new_struct;
}

struct mt_init *alloc_mt_init(key_t ipc_key)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	struct mt_init *new_struct = alloc_mem(sizeof(struct mt_init));
	new_struct->ipc_key = ipc_key;
	return new_struct;
}

struct mt_echo *alloc_mt_echo(char * text)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	struct mt_echo *new_struct = alloc_mem(sizeof(struct mt_echo));
	strcpy(new_struct->text, text);
	return new_struct;
}

struct mt_friends *alloc_mt_friends(char * text)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	struct mt_friends *new_struct = alloc_mem(sizeof(struct mt_friends));
	new_struct->op_type = FRIENDS_SET;
	new_struct->friends_mask = 0;
	int list_size = (int) strlen(text);
	int first = 0;
	for (int i = 0; i < list_size;)
	{
		while (text [i] == ' ' || text[i] == '\t')
			i++;
		int cur_id = atoi(&text[i]);
		if (!first)
		{
			char temp_str [4] ="";
			memcpy(temp_str,&text[i],3);
			if(strcmp(temp_str, "ADD") == 0)
				new_struct->op_type = FRIENDS_ADD;
			if(strcmp(temp_str, "DEL") == 0)
				new_struct->op_type = FRIENDS_DEL;
			first = 1;
		}
		if ( cur_id > 0 )
			new_struct->friends_mask |= 1 << (cur_id-1);
		while(text[i] != ' ' && text[i] != '\t')
			i++;
	}
	return new_struct;
}

struct mt_send *alloc_mt_send(__uint64_t target, char * text)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	struct mt_send *new_struct = alloc_mem(sizeof(struct mt_send));
	new_struct->target = target;
	strcpy(new_struct->text, text);
	return new_struct;
}

void *alloc_mem(size_t mem_size)
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	return alloc_table.pointers[alloc_table.pointers_num++] =
			   calloc(1, mem_size);;
}

void free_whole_mem()
{
#ifdef DEBUG
	printf("%d:%s()\n", getpid(), __FUNCTION__);
#endif
	for (int i = 0; i < alloc_table.pointers_num; i++)
		free(alloc_table.pointers[i]);
}