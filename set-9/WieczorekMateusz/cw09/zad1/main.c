#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <memory.h>

struct timestamp
{
	time_t sys_time;
	struct timeval time_value;
};

void get_timestamp(struct timestamp *timestamp_struct)
{
	timestamp_struct->sys_time = time(NULL);
	gettimeofday(&timestamp_struct->time_value, NULL);
}

void get_timestamp_string(char *str_buffer)
{
	struct timestamp timestamp_struct;
	get_timestamp(&timestamp_struct);
	struct tm *time_info;
	time_info = localtime(&timestamp_struct.sys_time);
	strftime(str_buffer, 31, "%H:%M:%S.", time_info);
	char us_str[16] = "";
	sprintf(us_str, "%06d:", (int) timestamp_struct.time_value.tv_usec);
	strcat(str_buffer, us_str);
}

struct Carriage
{
	pthread_mutex_t access_mutex;
	pthread_cond_t status_cond;
	pthread_cond_t status_change_cond;
	int id;
	int passengers_num;
	int carriage_status;
	int tours_to_do;
};

int passengers_num;
int carriages_num;
int carriage_capacity;
int tours_count;
int active_carriages;
int current_carriage_id = 0;
int finish_carriage_id = 0;
struct Carriage *cur_carriage = NULL;
struct Carriage **carriages_array;
pthread_t **carriages_threads;
pthread_t **passengers_threads;
int *passengers_ids;
pthread_cond_t *carriage_order_cond;
pthread_cond_t *carriage_present_cond;
pthread_cond_t *carriage_finish_order_cond;
pthread_mutex_t *carriage_access_mutex;
pthread_mutex_t *carriage_stop_access_mutex;
pthread_mutex_t *carriage_finish_mutex;

struct Carriage *create_carriage(int carriage_id)
{
	struct Carriage *carriage = malloc(sizeof(struct Carriage));
	carriage->id = carriage_id;
	carriage->passengers_num = 0;
	carriage->carriage_status = 0;
	carriage->tours_to_do = tours_count;
	pthread_mutex_init(&carriage->access_mutex, NULL);
	pthread_cond_init(&carriage->status_cond, NULL);
	pthread_cond_init(&carriage->status_change_cond, NULL);
	return carriage;
}

void destroy_carriage(struct Carriage *carriage)
{
	pthread_cond_destroy(&carriage->status_cond);
	pthread_cond_destroy(&carriage->status_change_cond);
	pthread_mutex_destroy(&carriage->access_mutex);
	free(carriage);
}

void *carriage_routine(void *args)
{
	srand((unsigned int)(time(NULL)+(unsigned long)(args)));
	struct Carriage * self = (struct Carriage *) args;
	char timestamp_string [32] = "";
	while (1)
	{
		pthread_mutex_lock(carriage_stop_access_mutex); // 1
		while (current_carriage_id != self->id)
		{
			pthread_cond_wait(carriage_order_cond,
							  carriage_stop_access_mutex);
		}
		pthread_mutex_lock(carriage_access_mutex); // 2
		cur_carriage = self;
		pthread_mutex_lock(&self->access_mutex); // 3
		get_timestamp_string(timestamp_string);
		fprintf(stdout, "%s Carriage %4d: Carriage arrives to start!\n",
			timestamp_string, self->id);
		self->carriage_status = 3;
		pthread_cond_broadcast(&self->status_cond);
		pthread_mutex_unlock(&self->access_mutex); // 3
		pthread_mutex_lock(&self->access_mutex); // 4
		while (self->passengers_num != 0)
			pthread_cond_wait(&self->status_change_cond, &self->access_mutex);
		self->tours_to_do--;
		if (self->tours_to_do < 0)
			break;
		self->carriage_status = 0;
		pthread_cond_broadcast(carriage_present_cond);
		pthread_mutex_unlock(&self->access_mutex); // 4
		pthread_mutex_unlock(carriage_access_mutex); // 2
		pthread_mutex_lock(&self->access_mutex); // 5
		while (self->passengers_num != carriage_capacity)
			pthread_cond_wait(&self->status_change_cond, &self->access_mutex);
		self->carriage_status = 1;
		pthread_cond_broadcast(&self->status_cond);
		pthread_mutex_unlock(&self->access_mutex); // 5
		pthread_mutex_lock(&self->access_mutex); // 6
		while (self->carriage_status != 2)
			pthread_cond_wait(&self->status_change_cond, &self->access_mutex);
		unsigned int usleep_time = (unsigned int)(rand()%8001)+2000;
		get_timestamp_string(timestamp_string);
		fprintf(stdout, "%s Carriage %4d: Carriage starts ride (%dus)!\n",
			timestamp_string, self->id, usleep_time);
		current_carriage_id = (current_carriage_id + 1) % carriages_num;
		pthread_cond_broadcast(carriage_order_cond);
		pthread_mutex_unlock(carriage_stop_access_mutex); // 1
		usleep(usleep_time);
		pthread_mutex_unlock(&self->access_mutex); // 6
		pthread_mutex_lock(carriage_finish_mutex); // 7
		while (finish_carriage_id != self->id)
			pthread_cond_wait(carriage_finish_order_cond,
							  carriage_finish_mutex);
		get_timestamp_string(timestamp_string);
		fprintf(stdout, "%s Carriage %4d: Carriage finishes ride!\n",
			timestamp_string, self->id);
		finish_carriage_id = (finish_carriage_id + 1) % carriages_num;
		pthread_cond_broadcast(carriage_finish_order_cond);
		pthread_mutex_unlock(carriage_finish_mutex); // 7
	}
	get_timestamp_string(timestamp_string);
	fprintf(stdout, "%s Carriage %4d: Carriage ends work!\n",
		timestamp_string, self->id);
	current_carriage_id = (current_carriage_id + 1) % carriages_num;
	pthread_cond_broadcast(carriage_order_cond);
	pthread_mutex_unlock(carriage_stop_access_mutex);
	active_carriages--;
	pthread_mutex_unlock(carriage_access_mutex);
	if (active_carriages == 0)
		pthread_cond_broadcast(carriage_present_cond);
	int ret = 0;
	pthread_exit((void *) &ret);
}

void *passenger_routine(void *args)
{
	int self_id = * ((int *) args);
	char timestamp_string [32] = "";
	struct Carriage *my_carriage;
	while (active_carriages != 0)
	{
		pthread_mutex_lock(carriage_access_mutex); // 1
		while (active_carriages != 0 &&
			   (cur_carriage == NULL || cur_carriage->carriage_status != 0 ||
				cur_carriage->passengers_num == carriage_capacity))
			pthread_cond_wait(carriage_present_cond,
							  carriage_access_mutex);
		if (active_carriages == 0)
		{
			pthread_mutex_unlock(carriage_access_mutex); // 1
			break;
		}
		my_carriage = cur_carriage;
		pthread_mutex_lock(&my_carriage->access_mutex); // 2
		while (cur_carriage->carriage_status != 0 ||
			   cur_carriage->passengers_num == carriage_capacity)
			pthread_cond_wait(carriage_present_cond, &my_carriage->access_mutex);
		my_carriage->passengers_num++;
		get_timestamp_string(timestamp_string);
		fprintf(stdout, "%s Passenger %3d: "
				   "Passenger entries to Carriage %4d!     (busy seats: %3d)\n",
			   timestamp_string, self_id, my_carriage->id,
			   my_carriage->passengers_num);
		if (my_carriage->passengers_num == carriage_capacity)
			pthread_cond_broadcast(&my_carriage->status_change_cond);
		pthread_mutex_unlock(&my_carriage->access_mutex); // 2
		pthread_mutex_unlock(carriage_access_mutex); // 1
		pthread_mutex_lock(&my_carriage->access_mutex); // 3
		while ((my_carriage->carriage_status == 0))
			pthread_cond_wait(&my_carriage->status_cond, &my_carriage->access_mutex);
		if (cur_carriage->carriage_status == 1)
		{
			my_carriage->carriage_status = 2;
			get_timestamp_string(timestamp_string);
			fprintf(stdout, "%s Passenger %3d: "
					   "Pressed start in Carriage   %4d!\n",
				   timestamp_string, self_id, my_carriage->id);
			pthread_cond_broadcast(&my_carriage->status_change_cond);
		}
		pthread_mutex_unlock(&my_carriage->access_mutex); // 3
		pthread_mutex_lock(&my_carriage->access_mutex); // 4
		while (my_carriage->carriage_status <= 2)
			pthread_cond_wait(&my_carriage->status_cond, &my_carriage->access_mutex);
		my_carriage->passengers_num--;
		get_timestamp_string(timestamp_string);
		fprintf(stdout, "%s Passenger %3d: "
				   "Passenger leaves Carriage %4d!         (busy seats: %3d)\n",
			   timestamp_string, self_id, my_carriage->id,
			   my_carriage->passengers_num);
		pthread_cond_broadcast(&my_carriage->status_change_cond);
		pthread_mutex_unlock(&my_carriage->access_mutex); // 4
	}
	get_timestamp_string(timestamp_string);
	fprintf(stdout, "%s Passenger %3d: Passenger thanks fun!\n",
		timestamp_string, self_id);
	int ret = 0;
	pthread_exit((void *) &ret);
}

void free_all()
{
	for (int i = 0; i < carriages_num; i++)
	{
		destroy_carriage(carriages_array[i]);
		free(carriages_threads[i]);
	}
	for (int i = 0; i < passengers_num; i++)
		free(passengers_threads[i]);
	free(carriages_array);
	free(carriages_threads);
	free(passengers_threads);
	free(passengers_ids);
	
	if (pthread_cond_destroy(carriage_order_cond) ||
		pthread_cond_destroy(carriage_present_cond) ||
		pthread_cond_destroy(carriage_finish_order_cond))
		fprintf(stderr, "Condition variables destroy error!\n");
	if (pthread_mutex_destroy(carriage_access_mutex) ||
		pthread_mutex_destroy(carriage_stop_access_mutex) ||
		pthread_mutex_destroy(carriage_finish_mutex))
		fprintf(stderr, "Mutexes destroy error!\n");
	free(carriage_order_cond);
	free(carriage_present_cond);
	free(carriage_finish_order_cond);
	free(carriage_access_mutex);
	free(carriage_stop_access_mutex);
	free(carriage_finish_mutex);
}

int main(int argc, char **argv)
{
	if (argc != 5)
	{
		fprintf(stderr, "Wrong number of arguments! Expected 4: number of "
			"passengers, number of carriages, carriage capacity, number"
			" of tours for each carriage.\n");
		return -1;
	}
	passengers_num = (int) strtol(argv[1], NULL, 0);
	if (passengers_num < 1)
	{
		fprintf(stderr, "Number of passengers must by positive "
			"integer number!\n");
		return -1;
	}
	carriages_num = (int) strtol(argv[2], NULL, 0);
	if (carriages_num < 1)
	{
		fprintf(stderr, "Number of carriages must by positive "
			"integer number!\n");
		return -1;
	}
	carriage_capacity = (int) strtol(argv[3], NULL, 0);
	if (carriage_capacity < 1)
	{
		fprintf(stderr, "struct Carriage capacity must by positive "
			"integer number!\n");
		return -1;
	}
	tours_count = (int) strtol(argv[4], NULL, 0);
	if (tours_count < 1)
	{
		fprintf(stderr, "struct Carriage tours number must by positive "
			"integer number!\n");
		return -1;
	}
	if (passengers_num < carriages_num * carriage_capacity)
	{
		fprintf(stderr, "Not enough passengers to start all carriages!\n");
		return -2;
	}
	active_carriages = carriages_num;
	carriage_order_cond = malloc(sizeof(pthread_cond_t));
	carriage_present_cond = malloc(sizeof(pthread_cond_t));
	carriage_finish_order_cond = malloc(sizeof(pthread_cond_t));
	carriage_access_mutex = malloc(sizeof(pthread_mutex_t));
	carriage_stop_access_mutex = malloc(sizeof(pthread_mutex_t));
	carriage_finish_mutex = malloc(sizeof(pthread_mutex_t));
	if (pthread_cond_init(carriage_order_cond, NULL) ||
		pthread_cond_init(carriage_present_cond, NULL) ||
		pthread_cond_init(carriage_finish_order_cond, NULL))
	{
		fprintf(stderr, "Condition variables initialization error!\n");
		return -3;
	}
	if (pthread_mutex_init(carriage_access_mutex, NULL) ||
		pthread_mutex_init(carriage_stop_access_mutex, NULL) ||
		pthread_mutex_init(carriage_finish_mutex, NULL))
	{
		fprintf(stderr, "Mutexes initialization error!\n");
		return -4;
	}
	carriages_threads = malloc(carriages_num * sizeof(pthread_t *));
	passengers_threads = malloc(passengers_num * sizeof(pthread_t *));
	carriages_array = malloc(carriages_num * sizeof(struct Carriage *));
	passengers_ids = malloc(passengers_num * sizeof(int));
	atexit(free_all);
	for (int i = 0; i < carriages_num; i++)
	{
		carriages_array[i] = create_carriage(i);
		carriages_threads[i] = malloc(sizeof(pthread_t));
		pthread_create(carriages_threads[i], NULL, carriage_routine,
					   (void *) carriages_array[i]);
	}
	for (int i = 0; i < passengers_num; i++)
	{
		passengers_ids[i] = i;
		passengers_threads[i] = malloc(sizeof(pthread_t));
		pthread_create(passengers_threads[i], NULL, passenger_routine,
					   (void *) (&passengers_ids[i]));
	}
	for (int i = 0; i < carriages_num; i++)
		pthread_join(*carriages_threads[i], NULL);
	for (int i = 0; i < passengers_num; i++)
		pthread_join(*passengers_threads[i], NULL);
	return 0;
}