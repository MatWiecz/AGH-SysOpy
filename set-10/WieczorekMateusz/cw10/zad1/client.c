#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <signal.h>
#include <string.h>
#include "common.h"
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <pthread.h>


void init(char *connection_type, char *server_ip_path, char *port);

void handle_message();

void connect_to_server();

void send_message(uint8_t message_type);

void cleanup();

char *client_name;
int client_socket;
int finish_threads = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *request_handler_routine(void *arg)
{
	
	request_t *temp_request = (request_t *) arg;
	request_t request;
	strcpy(request.data, temp_request->data);
	int request_id = temp_request->request_id;
	
	char *buffer = calloc(sizeof(char), 100 + 2 * strlen(request.data));
	char *buffer_result = calloc(sizeof(char), 100 + 2 * strlen(request.data));
	sprintf(buffer,
			"echo '%s' | awk '{for(x=1;$x;++x)print $x}' | sort | uniq -c",
			(char *) request.data);
	FILE *result = popen(buffer, "r");
	int received_bytes = (int) fread(buffer, 1, 99 + 2 * strlen(request.data),
									 result);
	pclose(result);
	buffer[received_bytes] = '\0';
	
	int words_count = 1;
	char *res = strtok(request.data, " ");
	while (strtok(NULL, " ") && res)
	{
		words_count++;
	}
	
	sprintf(buffer_result, "Request id: %d: Words count: %d\n%s", request_id,
			words_count, buffer);
	printf("%s\n", buffer_result);
	
	pthread_mutex_lock(&mutex);
	send_message(RESULT);
	int len = (int) strlen(buffer_result);
	if (write(client_socket, &len, sizeof(int)) != sizeof(int))
		throw_error("Cannot write message type!\n");
	if (write(client_socket, buffer_result, (size_t) len) != len)
		throw_error("Cannot write message data!\n");
	printf("Result has been sent to server!\n");
	pthread_mutex_unlock(&mutex);
	free(buffer);
	free(buffer_result);
	return NULL;
}

void send_message(uint8_t message_type)
{
	uint16_t message_size = (uint16_t) (strlen(client_name) + 1);
	if (write(client_socket, &message_type, MSG_TYPE_FIELD_SIZE) !=
		MSG_TYPE_FIELD_SIZE)
		throw_error("Cannot write message type!\n");
	if (write(client_socket, &message_size, MSG_LEN_FIELD_SIZE) !=
		MSG_LEN_FIELD_SIZE)
		throw_error("Cannot write message size!\n");
	if (write(client_socket, client_name, message_size) != message_size)
		throw_error("Cannot write message data!\n");
}

void connect_to_server()
{
	send_message(REGISTER);
	
	uint8_t message_type;
	if (read(client_socket, &message_type, 1) != 1)
		throw_error("Cannot read response message type!\n");
	
	switch (message_type)
	{
		case CLIENT_NAME_ALREADY_USED:
			throw_error("Name already used!\n");
		case MAX_CONN_NUM_GAINED:
			throw_error("Max connections number gained by server!\n");
		case SUCCESS:
			printf("Connection established successfully\n");
			break;
		default:
			throw_error("Undefined error occurred!\n");
	}
}

void handle_message()
{
	uint8_t message_type;
	pthread_t thread;
	request_t *request;
	while (!finish_threads)
	{
		if (read(client_socket, &message_type, MSG_TYPE_FIELD_SIZE) !=
			MSG_TYPE_FIELD_SIZE)
			throw_error("Cannot read message type!\n");
		switch (message_type)
		{
			case REQUEST:
				request = (request_t *) calloc(1, sizeof(request_t));
				uint16_t request_size;
				if (read(client_socket, &request_size, 2) <= 0)
				{
					throw_error("Cannot read message size!\n");
				}
				if (read(client_socket, request->data, request_size) < 0)
				{
					throw_error("Cannot read whole data");
				}
				printf("Processing request with id %d \n", request->request_id);
				pthread_create(&thread, NULL, request_handler_routine, request);
				pthread_detach(thread);
				break;
			case PING:
				pthread_mutex_lock(&mutex);
				send_message(PONG);
				pthread_mutex_unlock(&mutex);
				break;
			default:
				break;
		}
	}
}

void sigint_handler(int signo)
{
	printf("\nSIGINT received!\n");
	send_message(UNREGISTER);
	exit(0);
}

void init(char *connection_type, char *server_ip_path, char *port)
{
	
	struct sigaction act;
	act.sa_handler = sigint_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGINT, &act, NULL);
	
	if (strcmp("INTERNET", connection_type) == 0)
	{
		uint16_t port_num = (uint16_t) atoi(port);
		if (port_num < 1024 || port_num > 65535)
		{
			throw_error("Wrong port number provided");
		}
		
		struct sockaddr_in socket_address;
		memset(&socket_address, 0, sizeof(struct sockaddr_in));
		socket_address.sin_family = AF_INET;
		socket_address.sin_addr.s_addr = htonl(INADDR_ANY);
		socket_address.sin_port = htons(port_num);
		if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			throw_error("Cannot create socket!\n");
		}
		if (connect(client_socket, (const struct sockaddr *) &socket_address,
					sizeof(socket_address)) == -1)
		{
			throw_error("Cannot establish connection with server!\n");
		}
		printf("Connected to internet socket!\n");
		
	}
	else
		if (strcmp("UNIX", connection_type) == 0)
		{
			
			char *unix_path = server_ip_path;
			struct sockaddr_un socket_address;
			socket_address.sun_family = AF_UNIX;
			snprintf(socket_address.sun_path, UNIX_SOCK_PATH_MAX_LEN, "%s",
					 unix_path);
			if ((client_socket = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
				throw_error("Cannot create socket!\n");
			
			if (connect(client_socket,
						(const struct sockaddr *) &socket_address,
						sizeof(socket_address)) == -1)
				throw_error("Cannot establish connection with server!\n");
			printf("Connected to UNIX socket!\n");
		}
		else
		{
			throw_error("Undefined connection mode! Use INTERNET or UNIX!\n");
		}
}

void cleanup()
{
	printf("Cleaning up... \n");
	send_message(UNREGISTER);
	if (shutdown(client_socket, SHUT_RDWR) == -1)
		fprintf(stderr, "Cannot shutdown client's socket!\n");
	if (close(client_socket) == -1)
		fprintf(stderr, "Cannot close client's socket!\n");
}

int main(int argc, char *argv[])
{
	
	if (argc != 4 && argc != 5)
		fprintf(stderr, "Wrong number of arguments! Expected 3 or 4: client "
			"name, connection mode, [server ip, server port]/UNIX socket path."
			"\n");
	client_name = argv[1];
	char *connection_type = argv[2];
	char *server_ip = argv[3];
	char *port = NULL;
	if (argc == 5)
	{
		port = argv[4];
	}
	
	init(connection_type, server_ip, port);
	atexit(cleanup);
	connect_to_server();
	handle_message();
	return 0;
}