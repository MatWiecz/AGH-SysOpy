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

void register_client(char *, int);

void unregister_client(char *);

void cleanup();

void *ping_routine(void *);

void *commander_routine(void *);

void handle_connection(int);

void delete_client_from_list(int);

void delete_socket(int);

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
int request_id;

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
				if (write(clients[i].fd, &message_type, 1) != 1)
					throw_error("Cannot send ping to client!");
				clients[i].pings_sent++;
			}
		}
		pthread_mutex_unlock(&mutex);
		sleep(1);
	}
	return NULL;
}


void send_message(int type, int len, request_t *request, int i)
{
	if (write(clients[i].fd, &type, 1) != 1)
	{
		throw_error("Cannot send message to client!");
	}
	if (write(clients[i].fd, &len, 2) != 2)
	{
		throw_error("Cannot send message to client!");
	}
	if (write(clients[i].fd, request, (size_t) len) != len)
	{
		throw_error("Cannot send message to client!");
	}
	
}


void *commander_routine(void *arg)
{
	char *file_path;
	request_t *request;
	while (!finish_threads)
	{
		char buffer[256];
		printf("Enter command:\n");
		fgets(buffer, 256, stdin);
		file_path = (char *) calloc(sizeof(char), MAX_FILE_SIZE);
		request = (request_t *) calloc(sizeof(request_t), 1);
		memset(file_path, '\0', sizeof(char) * MAX_FILE_SIZE);
		sscanf(buffer, "%s", file_path);
		request_id++;
		printf("Request id: %d \nFile: %s\n", request_id, file_path);
		request->request_id = request_id;
		int status = (int) read_whole_file(file_path, request->data);
		if (strlen(file_path) <= 0)
		{
			printf("Please provide file path!\n");
			free(request);
			free(file_path);
			continue;
		}
		if (status < 0)
		{
			printf("An error occurred during opening file!\n");
			free(request);
			free(file_path);
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
				clients[client_id].busy = 1;
				send_message(REQUEST, sizeof(request_t), request, client_id);
				sent = 1;
				break;
			}
		}
		if (!sent)
		{
			client_id = 0;
			if (clients[client_id].busy > -1)
			{
				printf("Request has been sent to %s \n",
					   clients[client_id].name);
				clients[client_id].busy++;
				send_message(REQUEST, sizeof(request_t), request, client_id);
			}
		}
		free(request);
		free(file_path);
	}
	return NULL;
}


void handle_connection(int socket)
{
	int client = accept(socket, NULL, NULL);
	if (client == -1)
		throw_error("Cannot accept new client");
	
	struct epoll_event event;
	event.events = EPOLLIN | EPOLLPRI;
	event.data.fd = client;
	
	if (epoll_ctl(epoll, EPOLL_CTL_ADD, client, &event) == -1)
		throw_error("Cannot add new client to epoll");
}

void handle_message(int socket)
{
	uint8_t message_type;
	uint16_t message_size;
	
	if (read(socket, &message_type, MSG_TYPE_FIELD_SIZE) != MSG_TYPE_FIELD_SIZE)
		throw_error("Cannot read message type!\n");
	if (read(socket, &message_size, MSG_LEN_FIELD_SIZE) != MSG_LEN_FIELD_SIZE)
		throw_error("Cannot read message size!\n");
	char *client_name = malloc(message_size);
	
	switch (message_type)
	{
		case REGISTER:
		{
			if (read(socket, client_name, message_size) != message_size)
				throw_error("Cannot read register message\n");
			else
				register_client(client_name, socket);
			break;
		}
		case UNREGISTER:
		{
			if (read(socket, client_name, message_size) != message_size)
				throw_error("Cannot read unregister message\n");
			else
				unregister_client(client_name);
			break;
		}
		case RESULT:
		{
			printf("\nRESULT REPORT\n");
			char result[MAX_FILE_SIZE];
			if (read(socket, client_name, message_size) < 0)
				printf("An error occurred during receiving result!\n");
			int size;
			if (read(socket, &size, sizeof(int)) < 0)
				printf("An error occurred during receiving result!\n");
			if (read(socket, result, (size_t) size) < 0)
				printf("An error occurred during receiving result!\n");
			
			printf("From: %s \n", client_name);
			printf("Result:\n%s\n", result);
			
			int client_id;
			for (client_id = 0; client_id < clients_num; client_id++)
			{
				if (clients[client_id].busy > 0 &&
					strcmp(client_name, clients[client_id].name) == 0)
				{
					clients[client_id].busy--;
					clients[client_id].pings_sent = 0;
					printf("Client %s is ready for new tasks!\n",
						   client_name);
				}
			}
			break;
		}
		case PONG:
		{
			if (read(socket, client_name, message_size) != message_size)
				throw_error("Cannot read ping return message\n");
			pthread_mutex_lock(&mutex);
			int i = get_client_id(clients, clients_num, client_name);
			if (i >= 0)
				clients[i].pings_sent =
					clients[i].pings_sent == 0 ? 0 : clients[i].pings_sent - 1;
			pthread_mutex_unlock(&mutex);
			break;
		}
		default:
			break;
	}
	free(client_name);
}

void register_client(char *client_name, int socket)
{
	uint8_t message_type;
	pthread_mutex_lock(&mutex);
	if (clients_num == CLIENT_CONN_MAX_NUM)
	{
		message_type = MAX_CONN_NUM_GAINED;
		if (write(socket, &message_type, 1) != 1)
			throw_error(
				"Cannot send MAX_CONN_NUM_GAINED message to client "
					"\"%s\"\n");
		delete_socket(socket);
	}
	else
	{
		int exists = get_client_id(clients, clients_num, client_name);
		if (exists != -1)
		{
			message_type = CLIENT_NAME_ALREADY_USED;
			if (write(socket, &message_type, 1) != 1)
				throw_error(
					"Cannot send CLIENT_NAME_ALREADY_USED message to client "
						"\"%s\"\n");
			delete_socket(socket);
		}
		else
		{
			clients[clients_num].fd = socket;
			clients[clients_num].name = malloc(strlen(client_name) + 1);
			clients[clients_num].pings_sent = 0;
			clients[clients_num].busy = 0;
			strcpy(clients[clients_num++].name, client_name);
			message_type = SUCCESS;
			if (write(socket, &message_type, 1) != 1)
				throw_error(
					"Cannot send SUCCESS message to client \"%s\"\n");
			printf("Client with name \"%s\" registered successfully!\n",
				   client_name);
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
	delete_socket(clients[client_id].fd);
	free(clients[client_id].name);
	
	clients_num--;
	for (int j = client_id; j < clients_num; ++j)
		clients[j] = clients[j + 1];
}

void delete_socket(int socket)
{
	if (epoll_ctl(epoll, EPOLL_CTL_DEL, socket, NULL) == -1)
		throw_error("Cannot remove client's socket from epoll!\n");
	
	if (shutdown(socket, SHUT_RDWR) == -1)
		throw_error("Cannot shutdown client's socket!\n");
	
	if (close(socket) == -1)
		throw_error("Cannot close client's socket!\n");
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
	
	if ((internet_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		throw_error("Cannot create internet socket!\n");
	
	if (bind(internet_socket, (const struct sockaddr *) &inter_socket_address,
			 sizeof(inter_socket_address)))
		throw_error("Cannot bind internet socket!\n");
	
	if (listen(internet_socket, 64) == -1)
		throw_error("Cannot listen internet socket!\n");
	
	struct sockaddr_un unix_socket_address;
	unix_socket_address.sun_family = AF_UNIX;
	sprintf(unix_socket_address.sun_path, "%s", unix_socket_path);
	
	if ((unix_socket = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		throw_error("Cannot create UNIX socket!\n");
	if (bind(unix_socket, (const struct sockaddr *) &unix_socket_address,
			 sizeof(unix_socket_address)))
		throw_error("Cannot bind UNIX socket!\n");
	if (listen(unix_socket, 64) == -1)
		throw_error("Cannot listen UNIX socket!\n");
	
	struct epoll_event event;
	event.events = EPOLLIN | EPOLLPRI;
	if ((epoll = epoll_create1(0)) == -1)
		throw_error("Cannot create epoll!\n");
	event.data.fd = -internet_socket;
	if (epoll_ctl(epoll, EPOLL_CTL_ADD, internet_socket, &event) == -1)
		throw_error("Cannot add internet socket to epoll!\n");
	event.data.fd = -unix_socket;
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
		fprintf(stderr, "Wrong number of arguments! Expected 2: TDP port "
			"number, UNIX socket path.\n");
	atexit(cleanup);
	
	init(argv[1], argv[2]);
	
	struct epoll_event event;
	while (!finish_threads)
	{
		if (epoll_wait(epoll, &event, 1, -1) == -1)
			throw_error("epoll_wait failed!\n");
		if (event.data.fd < 0)
			handle_connection(-event.data.fd);
		else
			handle_message(event.data.fd);
	}
	return 0;
}