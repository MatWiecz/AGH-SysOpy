#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include "common.h"

void sigint_handler(int);

void init(char *, char *);

void handle_message(int);

void register_client(int socket, message_t message, struct sockaddr *sockaddr,
					 socklen_t socklen);

void unregister_client(char *);

void cleanup();

void *ping_routine(void *);

void *commander_routine(void *);

void delete_client_from_list(int);

size_t get_file_size(const char *file_path)
{
	int fd;
	if ((fd = open(file_path, O_RDONLY)) == -1)
	{
		fprintf(stderr, "Unable to open file for getting size.\n");
		return (size_t) -1;
	}
	struct stat stat_struct;
	fstat(fd, &stat_struct);
	size_t size = (size_t) stat_struct.st_size;
	close(fd);
	return size;
}

size_t read_whole_file(const char *file_path, char *buffer)
{
	size_t size = get_file_size(file_path);
	if (size == -1 || size >= MAX_FILE_SIZE)
	{
		fprintf(stderr, "Unable to open file or file too large.\n");
		return (size_t) -1;
	}
	FILE *file = fopen(file_path, "r");
	size_t read_bytes;
	if ((read_bytes = fread(buffer, sizeof(char), size, file)) != size)
	{
		fprintf(stderr, "Unable to read file.\n");
		return (size_t) -1;
	}
	fclose(file);
	return read_bytes;
}

int get_client_id(client_info_t *clients, int clients_num, char *client_name)
{
	for (int i = 0; i < clients_num; i++)
		if (strcmp(clients[i].name, client_name) == 0)
			return i;
	return -1;
}


int internet_socket;
int unix_socket;
int epoll;
char *unix_socket_path;
int request_id = 0;
pthread_t ping_thread;
pthread_t commander_thread;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
client_info_t clients[CLIENT_CONN_MAX_NUM];
int clients_num = 0;
int finish_threads = 0;


void *ping_routine(void *arg)
{
	uint8_t message_type = PING;
	while (!finish_threads)
	{
		pthread_mutex_lock(&mutex);
		for (int i = 0; i < clients_num; ++i)
		{
			if (clients[i].pings_sent != 0)
			{
				printf("Client \"%s\" is not responding. Removing...\n",
					   clients[i].name);
				delete_client_from_list(i--);
			}
			else
			{
				int socket = clients[i].connection_type == INTERNET_SOCKET
							 ? internet_socket : unix_socket;
				if (sendto(socket, &message_type, 1, 0, clients[i].sockaddr,
						   clients[i].socklen) != 1)
					throw_error("Cannot send ping to client!");
				clients[i].pings_sent++;
			}
		}
		pthread_mutex_unlock(&mutex);
		sleep(1);
	}
	return NULL;
}


void *commander_routine(void *arg)
{
	request_t request;
	while (!finish_threads)
	{
		char file_path[256];
		printf("Enter command:\n");
		fgets(file_path, 256, stdin);
		memset(request.data, 0, sizeof(request.data));
		sscanf(file_path, "%s", file_path);
		uint8_t message_type = REQUEST;
		request_id++;
		printf("Request id: %d \nFile: %s\n", request_id, file_path);
		request.request_id = request_id;
		int status = (int) read_whole_file(file_path, request.data);
		if (strlen(file_path) <= 0)
		{
			printf("Please provide file path!\n");
			continue;
		}
		if (status < 0)
		{
			printf("An error occurred during opening file!\n");
			continue;
		}
		int client_id = 0;
		int sent = 0;
		for (client_id = 0; client_id < clients_num; client_id++)
		{
			if (clients[client_id].busy == 0)
			{
				printf("Request has been sent to %s \n",
					   clients[client_id].name);
				int socket =
					clients[client_id].connection_type == INTERNET_SOCKET
					? internet_socket : unix_socket;
				if (sendto(socket, &message_type, 1, 0,
						   clients[client_id].sockaddr,
						   clients[client_id].socklen) != 1)
				{
					printf("An error occurred during sending!\n");
				}
				if (sendto(socket, &request, sizeof(request_t), 0,
						   clients[client_id].sockaddr,
						   clients[client_id].socklen) !=
					sizeof(request_t))
				{
					printf("An error occurred during sending!\n");
				}
				sent = 1;
				clients[client_id].busy++;
				break;
			}
		}
		if (!sent)
		{
			client_id = 0;
			if (clients[client_id].busy > -1)
			{
				int socket =
					clients[client_id].connection_type == INTERNET_SOCKET
					? internet_socket : unix_socket;
				if (sendto(socket, &message_type, 1, 0,
						   clients[client_id].sockaddr,
						   clients[client_id].socklen) != 1)
				{
					printf("An error occurred during sending!\n");
				}
				if (sendto(socket, &request, sizeof(request_t), 0,
						   clients[client_id].sockaddr,
						   clients[client_id].socklen) !=
					sizeof(request_t))
				{
					printf("An error occurred during sending!\n");
				}
				clients[client_id].busy++;
			}
		}
	}
	return NULL;
}


void handle_message(int socket)
{
	struct sockaddr *sockaddr = malloc(sizeof(struct sockaddr));
	socklen_t socklen = sizeof(struct sockaddr);
	message_t message;
	
	if (recvfrom(socket, &message, sizeof(message_t), 0, sockaddr, &socklen) !=
		sizeof(message_t))
		throw_error("Cannot receive message\n");
	
	switch (message.message_type)
	{
		case REGISTER:
		{
			register_client(socket, message, sockaddr, socklen);
			break;
		}
		case UNREGISTER:
		{
			unregister_client(message.name);
			break;
		}
		case RESULT:
		{
			printf("\nRESULT REPORT\n");
			
			printf("From: %s \n", message.name);
			printf("Result:\n%s\n", message.data);
			
			int client_id;
			for (client_id = 0; client_id < clients_num; client_id++)
			{
				if (clients[client_id].busy > 0 &&
					strcmp(clients[client_id].name, message.name) == 0)
				{
					clients[client_id].busy--;
					clients[client_id].pings_sent = 0;
					printf("Client %s is ready for new tasks\n",
						   message.name);
				}
			}
			
			break;
		}
		case PONG:
		{
			pthread_mutex_lock(&mutex);
			int i = get_client_id(clients, clients_num, message.name);
			if (i >= 0)
				clients[i].pings_sent =
					clients[i].pings_sent == 0 ? 0 : clients[i].pings_sent - 1;
			pthread_mutex_unlock(&mutex);
			break;
		}
		default:
			break;
	}
	
}


void register_client(int socket, message_t message, struct sockaddr *sockaddr,
					 socklen_t socklen)
{
	uint8_t message_type;
	pthread_mutex_lock(&mutex);
	if (clients_num == CLIENT_CONN_MAX_NUM)
	{
		message_type = MAX_CONN_NUM_GAINED;
		if (sendto(socket, &message_type, 1, 0, sockaddr, socklen) != 1)
			throw_error(
				"Cannot send MAX_CONN_NUM_GAINED message to client "
					"\"%s\"\n");
		free(sockaddr);
	}
	else
	{
		int exists = get_client_id(clients, clients_num, message.name);
		if (exists != -1)
		{
			message_type = CLIENT_NAME_ALREADY_USED;
			if (sendto(socket, &message_type, 1, 0, sockaddr, socklen) != 1)
				throw_error(
					"Cannot send CLIENT_NAME_ALREADY_USED message to client "
						"\"%s\"\n");
			free(sockaddr);
		}
		else
		{
			clients[clients_num].name = malloc(strlen(message.name) + 1);
			clients[clients_num].connection_type = message.connection_type;
			clients[clients_num].pings_sent = 0;
			clients[clients_num].busy = 0;
			clients[clients_num].socklen = socklen;
			clients[clients_num].sockaddr = malloc(sizeof(struct sockaddr_un));
			memcpy(clients[clients_num].sockaddr, sockaddr, socklen);
			message_type = SUCCESS;
			
			strcpy(clients[clients_num++].name, message.name);
			if (sendto(socket, &message_type, 1, 0, sockaddr, socklen) != 1)
				throw_error(
					"Cannot send SUCCESS message to client \"%s\"\n");
			printf("Client with name \"%s\" registered successfully!\n",
				   message.name);
		}
	}
	pthread_mutex_unlock(&mutex);
}


void unregister_client(char *client_name)
{
	pthread_mutex_lock(&mutex);
	int client_id = get_client_id(clients, clients_num, client_name);
	if (client_id >= 0)
	{
		delete_client_from_list(client_id);
		printf("Client with name \"%s\" unregistered successfully!\n",
			   client_name);
	}
	pthread_mutex_unlock(&mutex);
}


void delete_client_from_list(int client_id)
{
	free(clients[client_id].sockaddr);
	free(clients[client_id].name);
	
	clients_num--;
	for (int j = client_id; j < clients_num; ++j)
		clients[j] = clients[j + 1];
}


void sigint_handler(int signo)
{
	printf("\nSIGINT received!\n");
	exit(0);
}


void init(char *port, char *path)
{
	struct sigaction act;
	act.sa_handler = sigint_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGINT, &act, NULL);
	
	int client_id;
	for (client_id = 0; client_id < CLIENT_CONN_MAX_NUM; client_id++)
	{
		clients[client_id].busy = -1;
	}
	
	uint16_t port_num = (uint16_t) atoi(port);
	unix_socket_path = path;
	
	struct sockaddr_in inter_socket_address;
	memset(&inter_socket_address, 0, sizeof(struct sockaddr_in));
	inter_socket_address.sin_family = AF_INET;
	inter_socket_address.sin_addr.s_addr = htonl(INADDR_ANY);
	inter_socket_address.sin_port = htons(port_num);
	
	if ((internet_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		throw_error("Cannot create internet socket!\n");
	
	if (bind(internet_socket, (const struct sockaddr *) &inter_socket_address,
			 sizeof(inter_socket_address)))
		throw_error("Cannot bind internet socket\n");
	
	struct sockaddr_un unix_socket_address;
	unix_socket_address.sun_family = AF_UNIX;
	sprintf(unix_socket_address.sun_path, "%s", unix_socket_path);
	
	if ((unix_socket = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
		throw_error("Cannot create UNIX socket!\n");
	
	if (bind(unix_socket, (const struct sockaddr *) &unix_socket_address,
			 sizeof(unix_socket_address)))
		throw_error("Cannot bind UNIX socket!\n");
	
	struct epoll_event event;
	event.events = EPOLLIN | EPOLLPRI;
	if ((epoll = epoll_create1(0)) == -1)
		throw_error("Cannot create epoll!\n");
	event.data.fd = internet_socket;
	if (epoll_ctl(epoll, EPOLL_CTL_ADD, internet_socket, &event) == -1)
		throw_error("Cannot add internet socket to epoll!\n");
	event.data.fd = unix_socket;
	if (epoll_ctl(epoll, EPOLL_CTL_ADD, unix_socket, &event) == -1)
		throw_error("Cannot add UNIX socket to epoll!\n");
	
	if (pthread_create(&ping_thread, NULL, ping_routine, NULL) != 0)
		throw_error("Cannot create pinger thread");
	if (pthread_create(&commander_thread, NULL, commander_routine, NULL) != 0)
		throw_error("Cannot create commander thread");
}


void cleanup()
{
	printf("Cleaning up... \n");
	pthread_cancel(ping_thread);
	pthread_cancel(commander_thread);
	int client_id;
	for (client_id = 0; client_id < clients_num; client_id++)
	{
		if (clients[client_id].busy >= 0)
		{
			printf("Request has been sent to %s \n",
				   clients[client_id].name);
			uint8_t message_type = END;
			int socket =
				clients[client_id].connection_type == INTERNET_SOCKET
				? internet_socket
				: unix_socket;
			if (sendto(socket, &message_type, 1, 0, clients[client_id].sockaddr,
					   clients[client_id].socklen) != 1)
			{
				printf("An error occurred during sending!\n");
			}
		}
	}
	
	if (close(internet_socket) == -1)
		fprintf(stderr, "Cannot close internet socket\n");
	if (close(unix_socket) == -1)
		fprintf(stderr, "Cannot close UNIX socket\n");
	if (unlink(unix_socket_path) == -1)
		fprintf(stderr, "Cannot unlink UNIX socket\n");
	if (close(epoll) == -1)
		fprintf(stderr, "Cannot close epoll\n");
}


int main(int argc, char *argv[])
{
	if (argc != 3)
		fprintf(stderr, "Wrong number of arguments! Expected 2: UDP port "
			"number, UNIX socket path.\n");
	atexit(cleanup);
	
	init(argv[1], argv[2]);
	
	struct epoll_event event;
	while (!finish_threads)
	{
		if (epoll_wait(epoll, &event, 1, -1) == -1)
			throw_error("epoll_wait failed\n");
		handle_message(event.data.fd);
	}
	return 0;
}