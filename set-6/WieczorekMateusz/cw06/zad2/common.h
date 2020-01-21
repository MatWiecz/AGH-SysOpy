//
// Created by matthew on 24.04.19.
//

#ifndef ZAD1_COMMON_H
#define ZAD1_COMMON_H

#include <stdlib.h>
#include <sys/msg.h>
#include <stdio.h>
#include <zconf.h>
#include <memory.h>
#include <signal.h>
#include <time.h>
#include <mqueue.h>

//#define DEBUG
//#define IPCS_DEBUG

#define ALLOC_TABLE_SIZE 0xFFFFFF

#define USLEEP_TIME 1000*0

#define PROJECT_IDENTIFIER 0x13C7FE6D
#define QUEUE_MAX_SIZE_IN_MESSAGES 10
#define MESSAGE_SIZE sizeof(struct chat_message)

#define MESSAGE_DATA_SIZE 242
#define TEXT_MAX_LENGTH 200
#define COMMANDS_MAX_COUNT 10000

#define MESSAGE_TYPE_STOP 70
#define MESSAGE_TYPE_INIT 60
#define MESSAGE_TYPE_LIST 50
#define MESSAGE_TYPE_FRIENDS 40
#define MESSAGE_TYPE_SEND 30
#define MESSAGE_TYPE_ECHO 20
#define MESSAGE_TYPE_RESPONSE 10

#define FRIENDS_SET 1
#define FRIENDS_ADD 2
#define FRIENDS_DEL 3

#define SEND_2ALL ~((__uint64_t) 0)
#define SEND_2FRIENDS ((__uint64_t) 0)
#define SEND_2ONE(USER_ID) ((__uint64_t)(1 << (USER_ID-1)))

struct alloc_table_struct
{
	int pointers_num;
	void *pointers[ALLOC_TABLE_SIZE];
};

extern struct alloc_table_struct alloc_table;
extern int chat_instance_id;
extern int send_messages_count;

struct chat_message
{
	unsigned int message_type;
	int sender_id;
	int message_id;
	char data[MESSAGE_DATA_SIZE];
};

struct mt_response
{
	int message_id;
	int status;
};

struct mt_init
{
	char queue_path[64];
};

struct mt_echo
{
	char text[TEXT_MAX_LENGTH];
};

struct mt_friends
{
	int op_type;
	__uint64_t friends_mask;
};

struct mt_send
{
	__uint64_t target;
	char text[TEXT_MAX_LENGTH];
};


int prepare_message(struct chat_message *message_ptr,
					unsigned int type, void *data);

int send_chat_message(mqd_t message_queue, struct chat_message *message);

int receive_chat_message(mqd_t message_queue, struct chat_message *message);

struct mt_response *alloc_mt_response(struct chat_message *request,
									  int status);

struct mt_init *alloc_mt_init(char *queue_path);

struct mt_echo *alloc_mt_echo(char *text);

struct mt_friends *alloc_mt_friends(char *text);

struct mt_send *alloc_mt_send(__uint64_t target, char *text);

void *alloc_mem(size_t mem_size);

void free_whole_mem();

#endif //ZAD1_COMMON_H
