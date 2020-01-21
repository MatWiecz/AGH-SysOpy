#include "common.h"

void my_exit(int exit_value)
{
	detach_belt();
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

int try_put_pack_on_belt(unsigned int loader_strength)
{
	if (belt_data == (struct belt *) -1)
		return -1;
	if (belt_data->belt_slots[0].loader_pid != 0 ||
		loader_strength > belt_data->free_weight)
	{
		print_log("I can't put my pack on the belt!");
		return 0;
	}
	belt_data->belt_slots[0].loader_pid = getpid();
	get_timestamp(&belt_data->belt_slots[0].place_time);
	belt_data->belt_slots[0].pack_weight = loader_strength;
	belt_data->free_weight -= loader_strength;
	belt_data->free_slots--;
	{
		char temp_text[256] = "";
		char place_time_str[64] = "";
		append_timestamp_string(&belt_data->belt_slots[0].place_time,
								place_time_str);
		sprintf(temp_text, "I've just put my pack with weight %d on the belt.\n"
					"(Place time: %s; Belt load: %d/%d slots, %d/%d units)",
				loader_strength, place_time_str, belt_data->belt_length -
												 belt_data->free_slots,
				belt_data->belt_length,
				belt_data->belt_weight_capacity - belt_data->free_weight,
				belt_data->belt_weight_capacity);
		print_log(temp_text);
	}
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc != 2 && argc != 3)
	{
		fprintf(stderr, "Wrong number of arguments! Expected: loader's "
			"strength, [optional] loader's cycles number.\n");
		my_exit(-1);
	}
	unsigned int loader_strength = (unsigned int) strtol(argv[1], NULL, 10);
	unsigned int loader_cycles_number = (unsigned int) -1;
	if (argc == 3)
		loader_cycles_number = (unsigned int) strtol(argv[2], NULL, 10);
	if (loader_strength < 1 || loader_cycles_number < 1)
	{
		fprintf(stderr, "Arguments should be positive integer numbers!\n");
		my_exit(-2);
	}
	program_type = PROGRAM_TYPE_LOADER;
	set_signal_handler(SIGINT, sigint_handler);
	if (get_belt_op_permitter())
		my_exit(-3);
	if (get_belt())
		my_exit(-4);
	if (attach_belt())
		my_exit(-5);
	{
		char temp_text[256] = "";
		sprintf(temp_text, "Loader's ID: %d", getpid());
		print_log(temp_text);
	}
	unsigned int cycle_counter = 0;
	while (!end_program && cycle_counter++ < loader_cycles_number)
	{
		if (set_belt_op_permitter(BELT_OP_PERMITTER_START_OP))
			break;
		if (try_put_pack_on_belt(loader_strength))
			end_program = 1;
		if (set_belt_op_permitter(BELT_OP_PERMITTER_STOP_OP))
			break;
		usleep(1);
#ifdef DEBUG
		usleep(USLEEP_TIME);
#endif
	}
	if (detach_belt())
		my_exit(-6);
	my_exit(0);
}