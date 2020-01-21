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

void send_message(uint8_t message_type, char *data);

void handle_message();

void cleanup();

void connect_to_server();

int signal_status = 0;

char *client_name;
enum connection_type c_type;
int client_socket;
int finish_threads = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t request1_mutex = PTHREAD_MUTEX_INITIALIZER;

void *request_handler_routine(void *arg)
{
	
	struct request_t *temp_request = (struct request_t *) arg;
	struct request_t request;
	strcpy(request.data, temp_request->data);
	request.request_id = temp_request->request_id;
	
	char *buffer = malloc(100 + 2 * strlen(request.data));
	char *buffer_result = malloc(100 + 2 * strlen(request.data));
	
	sprintf(buffer,
			"echo '%s' | awk '{for(x=1;$x;++x)print $x}' | sort | uniq -c",
			(char *) request.data);
	FILE *result = popen(buffer, "r");
	
	int received_bytes = (int) fread(buffer, 1, 99 + 2 * strlen(request.data),
									 result);
	buffer[received_bytes] = '\0';
	pclose(result);
	
	int words_count = 1;
	char *res = strtok(request.data, " ");
	while (strtok(NULL, " ") && res)
	{
		words_count++;
	}
	sprintf(buffer_result, "Request id: %d: Words count: %d\n%s", request
		.request_id, words_count, buffer);
	printf("%s\n", buffer_result);
	
	pthread_mutex_lock(&request1_mutex);
	send_message(RESULT, buffer_result);
	pthread_mutex_unlock(&request1_mutex);
	printf("Result has been sent to server!\n");
	free(buffer);
	free(buffer_result);
	return NULL;
	
}

void send_message(uint8_t message_type, char *data)
{
	message_t message;
	message.message_type = (enum message_type) message_type;
	snprintf(message.name, 64, "%s", client_name);
	message.connection_type = c_type;
	if (data)
	{
		snprintf(message.data, strlen(data), "%s", data);
	}
	pthread_mutex_lock(&mutex);
	if (write(client_socket, &message, sizeof(message_t)) != sizeof(message_t))
		throw_error("Cannot send message to server!\n");
	pthread_mutex_unlock(&mutex);
}

void connect_to_server()
{
	send_message(REGISTER, NULL);
	
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
	request_t request;
	while (!finish_threads)
	{
		if (read(client_socket, &message_type, sizeof(uint8_t)) !=
			sizeof(uint8_t))
			throw_error("Cannot read message type!\n");
		switch (message_type)
		{
			case REQUEST:
				if (read(client_socket, &request, sizeof(request_t)) <= 0)
				{
					throw_error("Cannot read message size!\n");
				}
				printf("Processing request with id %d \n", request.request_id);
				pthread_create(&thread, NULL, request_handler_routine,
							   &request);
				pthread_detach(thread);
				break;
			case PING:
				pthread_mutex_lock(&request1_mutex);
				send_message(PONG, NULL);
				pthread_mutex_unlock(&request1_mutex);
				break;
			case END:
				raise(SIGINT);
			default:
				break;
		}
	}
}

void sigint_handler(int signo)
{
	printf("\nSIGINT received!\n");
	signal_status++;
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
		
		c_type = INTERNET_SOCKET;
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
		if ((client_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
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
			c_type = UNIX_SOCKET;
			
			if ((client_socket = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
				throw_error("Cannot create socket!\n");
			
			struct sockaddr_un socket_address_to_bind;
			socket_address_to_bind.sun_family = AF_UNIX;
			snprintf(socket_address_to_bind.sun_path, UNIX_SOCK_PATH_MAX_LEN,
					 "%s", client_name);
			
			if (bind(client_socket,
					 (const struct sockaddr *) &socket_address_to_bind,
					 sizeof(socket_address_to_bind)))
				throw_error("Cannot bind socket!\n");
			
			struct sockaddr_un socket_address;
			socket_address.sun_family = AF_UNIX;
			snprintf(socket_address.sun_path, UNIX_SOCK_PATH_MAX_LEN, "%s",
					 server_ip_path);
			
			if (connect(client_socket,
						(const struct sockaddr *) &socket_address,
						sizeof(socket_address)) == -1)
				printf("Connected to UNIX socket!\n");
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
	if (signal_status > 0)
	{
		send_message(UNREGISTER, NULL);
		unlink(client_name);
	}
	
	if (close(client_socket) == -1)
		throw_error("Cannot close client's socket!\n");
	unlink(client_name);
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