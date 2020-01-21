#include "common.h"

void set_signal_handler(int signal, void (*handler)(int))
{
	struct sigaction sigint_action;
	sigint_action.sa_handler = handler;
	sigemptyset(&sigint_action.sa_mask);
	sigaddset(&sigint_action.sa_mask, signal);
	sigint_action.sa_flags = 0;
	sigaction(signal, &sigint_action, NULL);
}

void print_to_stream(FILE *stream, char *message)
{
	struct timestamp timestamp_struct;
	get_timestamp(&timestamp_struct);
	char log_prefix[64] = "\r";
	switch (program_type)
	{
		case PROGRAM_TYPE_TRUCKER:
		{
			strcat(log_prefix, "TRUCKER: ");
			break;
		}
		case PROGRAM_TYPE_LOADER:
		{
			char temp_text[32] = "";
			sprintf(temp_text, "LOADER[%5d]: ", getpid());
			strcat(log_prefix, temp_text);
			break;
		}
		default:
			break;
	}
	append_timestamp_string(&timestamp_struct, log_prefix);
	fprintf(stream, "%s: %s\n", log_prefix, message);
}

void print_log(char *log_message)
{
	print_to_stream(stdout, log_message);
}

void print_system_error(char *error_message)
{
	print_to_stream(stderr, error_message);
}

void get_timestamp(struct timestamp *timestamp_struct)
{
	timestamp_struct->sys_time = time(NULL);
	gettimeofday(&timestamp_struct->time_value, NULL);
}

void append_timestamp_string(struct timestamp *timestamp_struct,
							 char *str_buffer)
{
	char time_str[32] = "";
	struct tm *time_info;
	time_info = localtime(&timestamp_struct->sys_time);
	strftime(time_str, 31, "%H:%M:%S.", time_info);
	char us_str[16] = "";
	sprintf(us_str, "%06d", (int) timestamp_struct->time_value.tv_usec);
	strcat(time_str, us_str);
	strcat(str_buffer, time_str);
}

int create_belt_op_permitter()
{
	if (belt_op_permitter != SEM_FAILED)
		return 0;
	char *home_dir_path = getenv("HOME");
	key_t unique_key = ftok(home_dir_path, PROJECT_IDENTIFIER);
	sprintf(sem_name, "%xSEM", (int) unique_key);
	if ((belt_op_permitter = sem_open(sem_name, O_RDWR | O_CREAT | O_EXCL,
									  0666, 1)) == SEM_FAILED)
	{
		print_system_error("Cannot create belt operation permitter "
							   "(semaphore)!");
		return -1;
	}
	return 0;
}

int delete_belt_op_permitter()
{
	if (belt_op_permitter == SEM_FAILED)
		return 0;
	if (sem_close(belt_op_permitter) == -1)
	{
		print_system_error("Cannot delete belt operation permitter "
							   "(semaphore)!");
		return -1;
	}
	sem_unlink(sem_name);
	belt_op_permitter = SEM_FAILED;
	return 0;
}

int get_belt_op_permitter()
{
	if (belt_op_permitter != SEM_FAILED)
		return 0;
	char *home_dir_path = getenv("HOME");
	key_t unique_key = ftok(home_dir_path, PROJECT_IDENTIFIER);
	sprintf(sem_name, "%xSEM", (int) unique_key);
	if ((belt_op_permitter = sem_open(sem_name, O_RDWR)) == SEM_FAILED)
	{
		print_system_error("Cannot get belt operation permitter "
							   "(semaphore)!");
		return -1;
	}
	return 0;
}

int set_belt_op_permitter(unsigned char mode)
{
	if (belt_op_permitter == SEM_FAILED)
		return 0;
	if (mode == BELT_OP_PERMITTER_START_OP)
		if (sem_wait(belt_op_permitter))
		{
			print_system_error("Cannot set belt operation permitter "
								   "(semaphore)!");
			return -1;
		}
	if (mode == BELT_OP_PERMITTER_STOP_OP)
		if (sem_post(belt_op_permitter))
		{
			print_system_error("Cannot set belt operation permitter "
								   "(semaphore)!");
			return -1;
		}
	return 0;
}

int create_belt(unsigned int packs_capacity, unsigned int weight_capacity)
{
	if (belt_id != -1)
		return 0;
	char *home_dir_path = getenv("HOME");
	key_t unique_key = ftok(home_dir_path, PROJECT_IDENTIFIER);
	sprintf(shm_name, "%xSHM", (int) unique_key);
	belt_data_size = sizeof(struct belt) +
					 packs_capacity * sizeof(struct belt_slot);
	if ((belt_id = shm_open(shm_name, O_RDWR | O_CREAT | O_EXCL, 0666)) == -1)
	{
		print_system_error("Cannot create belt (shared memory)!");
		return -1;
	}
	if (ftruncate(belt_id, belt_data_size) == -1)
	{
		print_system_error("Cannot ftruncate belt (shared memory)!");
		return -1;
	}
	if (attach_belt())
	{
		delete_belt();
		return -2;
	}
	memset(belt_data, 0, belt_data_size);
	belt_data->memory_size = belt_data_size;
	belt_data->belt_length = packs_capacity;
	belt_data->free_weight = weight_capacity;
	belt_data->free_slots = packs_capacity;
	belt_data->belt_weight_capacity = weight_capacity;
	return 0;
}

int delete_belt()
{
	if (belt_id == -1)
		return 0;
	if (shm_unlink(shm_name) == -1)
	{
		print_system_error("Cannot delete belt (shared memory)!");
		return -1;
	}
	belt_id = -1;
	return 0;
}

int get_belt()
{
	if (belt_id != -1)
		return 0;
	char *home_dir_path = getenv("HOME");
	key_t unique_key = ftok(home_dir_path, PROJECT_IDENTIFIER);
	sprintf(shm_name, "%xSHM", (int) unique_key);
	if ((belt_id = shm_open(shm_name, O_RDWR, 0666)) == -1)
	{
		print_system_error("Cannot get belt (shared memory)!");
		return -1;
	}
	belt_data_size = sizeof(struct belt);
	if (ftruncate(belt_id, belt_data_size) == -1)
	{
		print_system_error("Cannot ftruncate belt (shared memory)!");
		return -1;
	}
	return 0;
}

int attach_belt()
{
	if (belt_data != (struct belt *) -1)
		return 0;
	if ((belt_data = mmap(NULL, belt_data_size, PROT_READ | PROT_WRITE,
						  MAP_SHARED, belt_id, 0)) == (struct belt *) -1)
	{
		print_system_error("Cannot attach belt (shared memory)!");
		return -1;
	}
	if (program_type == PROGRAM_TYPE_LOADER)
	{
		size_t right_belt_data_size = belt_data->memory_size;
		if (detach_belt())
			return -2;
		belt_data_size = right_belt_data_size;
		if ((belt_data = mmap(NULL, belt_data_size, PROT_READ | PROT_WRITE,
							  MAP_SHARED, belt_id, 0)) == (struct belt *) -1)
		{
			print_system_error("Cannot attach belt (shared memory)!");
			return -3;
		}
	}
	fprintf(stdout, "%d\n", (int)belt_data_size);
	return 0;
}

int detach_belt()
{
	if (belt_data == (struct belt *) -1)
		return 0;
	if (munmap(belt_data, belt_data_size) == -1)
	{
		print_system_error("Cannot detach belt (shared memory)!");
		return -1;
	}
	belt_data = (struct belt *) -1;
	return 0;
}

int end_program = 0;
sem_t *belt_op_permitter = SEM_FAILED;
int belt_id = -1;
struct belt *belt_data = (void *) -1;
int program_type = 0;
char sem_name[16] = "";
char shm_name[16] = "";
size_t belt_data_size = 0;