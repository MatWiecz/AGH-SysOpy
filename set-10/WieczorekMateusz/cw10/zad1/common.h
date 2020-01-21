#ifndef SET10_COMMON_H
#define SET10_COMMON_H

#include <stdint.h>
#include <unistd.h>
#include <errno.h>

#define UNIX_SOCK_PATH_MAX_LEN 108
#define CLIENT_CONN_MAX_NUM 12
#define MSG_TYPE_FIELD_SIZE 1
#define MSG_LEN_FIELD_SIZE 2
#define MAX_FILE_SIZE 10240

typedef enum message_type
{
	REGISTER = 0,
	UNREGISTER = 1,
	SUCCESS = 2,
	MAX_CONN_NUM_GAINED = 3,
	CLIENT_NAME_ALREADY_USED = 4,
	REQUEST = 5,
	RESULT = 6,
	PING = 7,
	PONG = 8,
} message_type;

typedef enum connection_type
{
	UNIX_SOCKET,
	INTERNET_SOCKET
} connection_type;

typedef struct client_info_t
{
	int fd;
	char *name;
	int pings_sent;
	int busy;
} client_info_t;

typedef struct request_t
{
	char data[MAX_FILE_SIZE];
	int request_id;
} request_t;

void throw_error(char *message)
{
	fprintf(stderr, "%s (%s)\n", message, strerror(errno));
	exit(0);
}

#endif //SET10_COMMON_H