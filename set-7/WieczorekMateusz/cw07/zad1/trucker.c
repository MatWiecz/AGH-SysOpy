#include "common.h"

struct truck
{
	unsigned int max_capacity;
	unsigned int free_space;
};

void my_exit(int exit_value)
{
	delete_belt_op_permitter();
	delete_belt();
	if (exit_value == 0)
		fprintf(stdout, "Process terminated normally.\n");
	else
		fprintf(stdout, "Process terminated with error with code %d.\n",
				exit_value);
	exit(exit_value);
}

void sigint_handler(int signal)
{
	print_log("SIGINT received, so program is ending...");
	end_program = 1;
}

void move_belt()
{
	if ((belt_data == (struct belt *) -1))
		return;
	for (int i = belt_data->belt_length - 1; i > 0; i--)
		belt_data->belt_slots[i] = belt_data->belt_slots[i - 1];
	belt_data->belt_slots[0].loader_pid = 0;
	belt_data->belt_slots[0].pack_weight = 0;
	memset(&belt_data->belt_slots[0].place_time, 0, sizeof(struct timestamp));
}

int try_get_pack_from_belt(struct truck *truck_info)
{
	if (belt_data == (struct belt *) -1)
		return -1;
	move_belt();
	if (belt_data->belt_slots[belt_data->belt_length - 1].loader_pid == 0)
	{
		print_log("Waiting for pack to load on track...");
		return 0;
	}
	if (belt_data->belt_slots[belt_data->belt_length - 1].pack_weight >
		truck_info->free_space)
	{
		print_log("Truck is full and ready to drive away!");
		truck_info->free_space = truck_info->max_capacity;
		print_log("Empty truck draw in!");
	}
	truck_info->free_space -=
		belt_data->belt_slots[belt_data->belt_length - 1].pack_weight;
	belt_data->free_weight +=
		belt_data->belt_slots[belt_data->belt_length - 1].pack_weight;
	belt_data->free_slots++;
	{
		char temp_text[256] = "";
		char delta_time_str[64] = "";
		struct timestamp current_time;
		get_timestamp(&current_time);
		unsigned long long delta_time = (unsigned long long)
											current_time.sys_time * 1000000 +
										current_time.time_value.tv_usec;
		delta_time -= 1000000 *
					  belt_data->belt_slots[belt_data->belt_length - 1]
						  .place_time.sys_time;
		delta_time -=
			belt_data->belt_slots[belt_data->belt_length - 1].place_time.
				time_value.tv_usec;
		sprintf(delta_time_str, "%lld", delta_time);
		char place_time_str[64] = "";
		append_timestamp_string(&belt_data->belt_slots
								[belt_data->belt_length - 1].place_time,
								place_time_str);
		sprintf(temp_text, "Pack with weight %d, of loader with id %d loaded "
					"on track.\n(Truck's free space: %d/%d; "
					"Time of pack on belt: %s us; "
					"Pack put at %s;\nBelt load: %d/%d slots, %d/%d units)",
				belt_data->belt_slots[belt_data->belt_length - 1].pack_weight,
				belt_data->belt_slots[belt_data->belt_length - 1].loader_pid,
				truck_info->free_space, truck_info->max_capacity,
				delta_time_str,
				place_time_str, belt_data->belt_length -
								belt_data->free_slots, belt_data->belt_length,
				belt_data->belt_weight_capacity - belt_data->free_weight,
				belt_data->belt_weight_capacity);
		print_log(temp_text);
	}
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc != 4)
	{
		fprintf(stderr, "Wrong number of arguments! Expected 3: truck's "
			"capacity, belt's packs capacity, belt's weight capacity.\n");
		my_exit(-1);
	}
	unsigned int truck_capacity = (unsigned int) strtol(argv[1], NULL, 10);
	unsigned int belt_packs_capacity =
		(unsigned int) strtol(argv[2], NULL, 10);
	unsigned int belt_weight_capacity =
		(unsigned int) strtol(argv[3], NULL, 10);
	if (truck_capacity < 1 || belt_packs_capacity < 1 ||
		belt_weight_capacity < 1)
	{
		fprintf(stderr, "Arguments should be positive integer numbers!\n");
		my_exit(-2);
	}
	program_type = PROGRAM_TYPE_TRUCKER;
	set_signal_handler(SIGINT, sigint_handler);
	if (create_belt_op_permitter())
		my_exit(-3);
	if (set_belt_op_permitter(BELT_OP_PERMITTER_START_OP))
		my_exit(-4);
	if (create_belt(belt_packs_capacity, belt_weight_capacity))
		my_exit(-5);
	struct truck truck_info;
	truck_info.max_capacity = truck_capacity;
	truck_info.free_space = truck_capacity;
	print_log("Empty truck draw in!");
	if (set_belt_op_permitter(BELT_OP_PERMITTER_STOP_OP))
		my_exit(-6);
	//sleep(10);
	while (!end_program)
	{
		if (set_belt_op_permitter(BELT_OP_PERMITTER_START_OP))
			break;
		if (try_get_pack_from_belt(&truck_info))
			end_program = 1;
		if (set_belt_op_permitter(BELT_OP_PERMITTER_STOP_OP))
			my_exit(-13);
#ifdef DEBUG
		usleep(USLEEP_TIME);
#endif
	}
	if (set_belt_op_permitter(BELT_OP_PERMITTER_START_OP))
		my_exit(-7);
	if (delete_belt_op_permitter())
		my_exit(-11);
	for (int i = 0; i < belt_packs_capacity; i++)
		try_get_pack_from_belt(&truck_info);
	if (detach_belt())
		my_exit(-8);
	if (delete_belt())
		my_exit(-9);
	my_exit(0);
}