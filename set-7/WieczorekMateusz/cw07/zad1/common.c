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
	if (belt_op_permitter != -1)
		return 0;
	char *home_dir_path = getenv("HOME");
	key_t unique_key = ftok(home_dir_path, PROJECT_IDENTIFIER);
	if ((belt_op_permitter = semget(unique_key, 1, IPC_CREAT | IPC_EXCL | 0666))
		== -1)
	{
		print_system_error("Cannot create belt operation permitter "
							   "(semaphore)!");
		return -1;
	}
	union semun semctl_arg;
	semctl_arg.val = 1;
	if (semctl(belt_op_permitter, 0, SETVAL, semctl_arg) == -1)
	{
		print_system_error("Cannot init belt operation permitter "
							   "(semaphore)!");
		delete_belt_op_permitter();
		return -2;
	}
	return 0;
}

int delete_belt_op_permitter()
{
	if (belt_op_permitter == -1)
		return 0;
	if (semctl(belt_op_permitter, 0, IPC_RMID) == -1)
	{
		print_system_error("Cannot delete belt operation permitter "
							   "(semaphore)!");
		return -1;
	}
	belt_op_permitter = -1;
	return 0;
}

int get_belt_op_permitter()
{
	if (belt_op_permitter != -1)
		return 0;
	char *home_dir_path = getenv("HOME");
	key_t unique_key = ftok(home_dir_path, PROJECT_IDENTIFIER);
	if ((belt_op_permitter = semget(unique_key, 0, 0)) == -1)
	{
		print_system_error("Cannot get belt operation permitter "
							   "(semaphore)!");
		return -1;
	}
	return 0;
}

int set_belt_op_permitter(unsigned char mode)
{
	if (belt_op_permitter == -1)
		return 0;
	struct sembuf sem_op;
	sem_op.sem_num = 0;
	sem_op.sem_flg = 0;
	if (mode == BELT_OP_PERMITTER_START_OP)
		sem_op.sem_op = -1;
	if (mode == BELT_OP_PERMITTER_STOP_OP)
		sem_op.sem_op = 1;
	
	if (semop(belt_op_permitter, &sem_op, 1) == -1)
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
	size_t belt_data_size = sizeof(struct belt) +
							packs_capacity * sizeof(struct belt_slot);
	if ((belt_id = shmget(unique_key, belt_data_size,
						  IPC_CREAT | IPC_EXCL | 0666)) == -1)
	{
		print_system_error("Cannot create belt (shared memory)!");
		return -1;
	}
	if (attach_belt())
	{
		delete_belt();
		return -2;
	}
	memset(belt_data, 0, belt_data_size);
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
	if (shmctl(belt_id, IPC_RMID, NULL) == -1)
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
	if ((belt_id = shmget(unique_key, 0, 0)) == -1)
	{
		print_system_error("Cannot get belt (shared memory)!");
		return -1;
	}
	return 0;
}

int attach_belt()
{
	if (belt_data != (struct belt *) -1)
		return 0;
	if ((belt_data = shmat(belt_id, NULL, 0)) == (struct belt *) -1)
	{
		print_system_error("Cannot attach belt (shared memory)!");
		return -1;
	}
	return 0;
}

int detach_belt()
{
	if (belt_data == (struct belt *) -1)
		return 0;
	if (shmdt(belt_data) == -1)
	{
		print_system_error("Cannot detach belt (shared memory)!");
		return -1;
	}
	belt_data = (struct belt *) -1;
	return 0;
}

int end_program = 0;
int belt_op_permitter = -1;
int belt_id = -1;
struct belt *belt_data = (void *) -1;
int program_type = 0;