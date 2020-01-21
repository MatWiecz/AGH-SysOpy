#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "memory-manager.h"

struct find_command_info
{
	char * directory;
	char * file;
};

struct find_command_info * create_find_command_info ( char * directory_path,
													  char * file_name )
{
	struct find_command_info * new_fci = malloc ( sizeof ( struct
		find_command_info ) );
	new_fci->directory = NULL;
	new_fci->file = NULL;
	update_string ( directory_path, & new_fci->directory );
	update_string ( file_name, & new_fci->file );
	return new_fci;
}

void update_string ( char * source, char ** target )
{
	if ( * target != NULL )
		free ( * target );
	if ( source == NULL )
		* target = NULL;
	else
	{
		unsigned int source_length = ( unsigned int ) strlen ( source );
		* target = calloc ( source_length + 1, sizeof ( char ) );
		strcpy ( * target, source );
	}
}

void update_directory_to_explore ( struct find_command_info * info,
								   char * directory_path )
{
	if ( info == NULL )
	{
		fprintf ( stderr, "Null pointer to info structure provided!\n" );
		return;
	}
	update_string ( directory_path, & info->directory );
}

void update_file_to_find ( struct find_command_info * info, char * file_name )
{
	if ( info == NULL )
	{
		fprintf ( stderr, "Null pointer to info structure provided!\n" );
		return;
	}
	update_string ( file_name, & info->file );
}

void delete_find_command_info ( struct find_command_info * info_to_delete )
{
	if ( info_to_delete == NULL )
	{
		fprintf ( stderr, "Null pointer to info structure provided!\n" );
		return;
	}
	update_string ( NULL, & info_to_delete->directory );
	update_string ( NULL, & info_to_delete->file );
	free ( info_to_delete );
}

void run_find_command ( struct find_command_info * info_for_command,
						char * log_file_path )
{
	if ( info_for_command == NULL )
	{
		fprintf ( stderr, "Null pointer to info structure provided!\n" );
		return;
	}
	if ( info_for_command->directory == NULL || info_for_command->file == NULL )
	{
		fprintf ( stdout, "Nothing to do. Find command won't be run.\n" );
		return;
	}
	char console_out = 0;
	if ( log_file_path == NULL )
	{
		fprintf ( stdout, "No log file path provided. Output will be "
			"directed to console.\n" );
		console_out = 1;
	}
	char command[256];
	strcpy ( command, "find \"" );
	strcat ( command, info_for_command->directory );
	strcat ( command, "\" -name \"" );
	strcat ( command, info_for_command->file );
	strcat ( command, "\" " );
	if ( console_out == 0 )
	{
		strcat ( command, "> \"" );
		strcat ( command, log_file_path );
		strcat ( command, "\" " );
	}
	strcat ( command, "2> /dev/null" );
	system ( command );
}

struct memory_blocks_list
{
	int list_length;
	char ** memory_blocks_pointers;
};

struct memory_blocks_list * create_memory_blocks_list ( int list_length )
{
	if ( list_length <= 0 )
	{
		fprintf ( stderr, "Length of memory blocks list must be positive "
			"integer number!\n" );
		return NULL;
	}
	struct memory_blocks_list
		* new_mbl = malloc ( sizeof ( struct memory_blocks_list ) );
	new_mbl->list_length = list_length;
	new_mbl->memory_blocks_pointers =
		calloc ( ( size_t ) list_length, sizeof ( char * ) );
	return new_mbl;
}

int load_file_to_memory_block ( struct memory_blocks_list * list,
								char * file_path )
{
	if ( list == NULL )
	{
		fprintf ( stderr, "Null pointer to memory blocks list provided!\n" );
		return -1;
	}
	if ( file_path == NULL )
	{
		fprintf ( stdout, "File path not provided. Loading aborted.\n" );
		return -1;
	}
	FILE * file = fopen ( file_path, "r" );
	if ( file == NULL )
	{
		fprintf ( stdout, "File with provided path doesn't exist. "
			"Loading aborted.\n" );
		return -2;
	}
	fseek ( file, 0, SEEK_END );
	long file_size = ftell ( file );
	if ( file_size > 0xFFFFFFFF )
	{
		fprintf ( stdout, "File with provided path is too large.\n" );
		fclose ( file );
		return -2;
	}
	rewind ( file );
	int memory_block_index = 0;
	while ( memory_block_index < list->list_length )
	{
		if ( list->memory_blocks_pointers[memory_block_index] == NULL )
			break;
		memory_block_index++;
	}
	if ( memory_block_index == list->list_length )
	{
		fprintf ( stdout, "Memory blocks list is full. Cannot allocate new "
			"memory block for this list.\n" );
		fclose ( file );
		return -3;
	}
	list->memory_blocks_pointers[memory_block_index] =
		malloc ( sizeof ( char ) * file_size );
	fread ( list->memory_blocks_pointers[memory_block_index],
			( size_t ) file_size, 1, file );
	fclose ( file );
	return memory_block_index;
}

void delete_memory_block_from_list ( struct memory_blocks_list * list,
									 int index )
{
	if ( list == NULL )
	{
		fprintf ( stderr, "Null pointer to memory blocks list provided!\n" );
		return;
	}
	if ( index >= list->list_length || list->memory_blocks_pointers[index]
									   == NULL )
	{
		fprintf ( stdout, "Index doesn't exists in this list or there is no "
			"memory block allocated under this index.\n" );
		return;
	}
	free ( list->memory_blocks_pointers[index] );
	list->memory_blocks_pointers[index] = NULL;
}

void delete_memory_blocks_list ( struct memory_blocks_list * list_to_delete )
{
	if ( list_to_delete == NULL )
	{
		fprintf ( stderr, "Null pointer to memory blocks list provided!\n" );
		return;
	}
	for ( int i = 0; i < list_to_delete->list_length; i++ )
	{
		free ( list_to_delete->memory_blocks_pointers[i] );
	}
	free ( list_to_delete->memory_blocks_pointers );
	free ( list_to_delete );
}