#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/times.h>
#include <zconf.h>
#include <string.h>
#include "memory-manager.h"

double get_operation_time ( clock_t start, clock_t stop )
{
	return ( ( double ) ( stop - start ) ) / sysconf ( _SC_CLK_TCK );
}

int main ( int argc, char ** argv )
{
	struct memory_blocks_list * list = NULL;
//	char * log_file_path;
	struct tms * global_start_time = malloc ( sizeof ( struct tms ) );
	struct tms * global_stop_time = malloc ( sizeof ( struct tms ) );
	struct tms * start_time = malloc ( sizeof ( struct tms ) );
	struct tms * stop_time = malloc ( sizeof ( struct tms ) );
	clock_t real_global_start_time;
	clock_t real_global_stop_time;
	clock_t real_start_time;
	clock_t real_stop_time;
	printf ( "Op\\Time\t\t\tReal\t\tUser\t\tSystem\n" );
	int arg_id = 1;
	real_global_start_time = times ( global_start_time );
	while ( arg_id < argc )
	{
		real_start_time = times ( start_time );
		if ( strcmp ( argv[arg_id], "create_table" ) == 0 && arg_id + 1 < argc )
		{
			int table_size = atoi ( argv[arg_id + 1] );
			if ( list != NULL )
				delete_memory_blocks_list ( list );
			list = create_memory_blocks_list ( table_size );
			printf ( "Create table\t" );
			arg_id += 2;
		}
		else
			if ( strcmp ( argv[arg_id], "run_find" ) == 0 &&
				 arg_id + 3 <
				 argc )
			{
				struct find_command_info * info = create_find_command_info
					( argv[arg_id + 1], argv[arg_id + 2] );
				run_find_command ( info, argv[arg_id + 3] );
				delete_find_command_info ( info );
				printf ( "Run find\t\t" );
				arg_id += 4;
			}
			else
				if ( strcmp ( argv[arg_id], "load_file" ) == 0 &&
					 arg_id + 1 < argc )
				{
					load_file_to_memory_block ( list, argv[arg_id + 1] );
					printf ( "Load file\t\t" );
					arg_id += 2;
				}
				else
					if ( strcmp ( argv[arg_id], "remove_block" ) == 0 &&
						 arg_id + 1 < argc )
					{
						char * end_ptr;
						int index = ( int ) strtol ( argv[arg_id + 1],
													 & end_ptr, 10 );
						if ( * end_ptr )
							printf ( "Wrong index provided as argument of "
										 "command \"remove_block\"!\n" );
						else
							delete_memory_block_from_list ( list, index );
						printf ( "Remove block\t" );
						arg_id += 2;
					}
					else
					{
						printf (
							"\"%s\" is not a valid command name \n"
								"Program aborted.",
							argv[arg_id + 1] );
						arg_id = argc;
					}
		real_stop_time = times ( stop_time );
		printf ( "%lf\t",
				 get_operation_time ( real_start_time,
									  real_stop_time ) );
		printf ( "%lf\t",
				 get_operation_time ( start_time->tms_cutime,
									  stop_time->tms_cutime ) );
		printf ( "%lf\n",
				 get_operation_time ( start_time->tms_cstime,
									  stop_time->tms_cstime ) );
	}
	real_global_stop_time = times ( global_stop_time );
	printf ( "Total\t\t\t" );
	printf ( "%lf\t", get_operation_time ( real_global_start_time,
										   real_global_stop_time ) );
	printf ( "%lf\t", get_operation_time ( global_start_time->tms_cutime,
										   global_stop_time->tms_cutime ) );
	printf ( "%lf\n\n\n",
			 get_operation_time ( global_start_time->tms_cstime,
								  global_stop_time->tms_cstime ) );
	delete_memory_blocks_list ( list );
	free ( global_start_time );
	free ( global_stop_time );
	free ( start_time );
	free ( stop_time );
	return 0;
}