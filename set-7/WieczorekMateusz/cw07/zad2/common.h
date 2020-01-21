//
// Created by matthew on 24.04.19.
//

#ifndef ZAD1_COMMON_H
#define ZAD1_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/time.h>
#include <memory.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <semaphore.h>

//#define DEBUG
#define USLEEP_TIME 1*1000*100

#define PROJECT_IDENTIFIER 0x13C7FE6D

#define PROGRAM_TYPE_TRUCKER 1
#define PROGRAM_TYPE_LOADER 2

#define BELT_OP_PERMITTER_START_OP 1
#define BELT_OP_PERMITTER_STOP_OP 2

union semun
{
	int val;    /* Value for SETVAL */
	struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
	unsigned short *array;  /* Array for GETALL, SETALL */
	struct seminfo *__buf;  /* Buffer for IPC_INFO
                                           (Linux-specific) */
};

struct timestamp
{
	time_t sys_time;
	struct timeval time_value;
};

struct belt_slot
{
	pid_t loader_pid;
	unsigned int pack_weight;
	struct timestamp place_time;
};

struct belt
{
	size_t memory_size;
	unsigned int belt_length;
	unsigned int belt_weight_capacity;
	unsigned int free_weight;
	unsigned int free_slots;
	struct belt_slot belt_slots[0];
};

void set_signal_handler(int signal, void (*handler)(int));

void print_to_stream(FILE *stream, char *message);

void print_log(char *log_message);

void print_system_error(char *error_message);

void get_timestamp(struct timestamp *timestamp_struct);

void append_timestamp_string(struct timestamp *timestamp_struct,
							 char *str_buffer);

int create_belt_op_permitter();

int delete_belt_op_permitter();

int get_belt_op_permitter();

int set_belt_op_permitter(unsigned char mode);

int create_belt(unsigned int packs_capacity, unsigned int weight_capacity);

int delete_belt();

int get_belt();

int attach_belt();

int detach_belt();

void move_belt();

extern int end_program;
sem_t *belt_op_permitter;
extern int belt_id;
extern struct belt *belt_data;
extern int program_type;
extern char sem_name[16];
extern char shm_name[16];
extern size_t belt_data_size;

#endif //ZAD1_COMMON_H
