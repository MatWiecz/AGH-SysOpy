#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

struct find_command_info;

struct find_command_info * create_find_command_info ( char *, char * );

void update_string ( char *, char ** );

void update_directory_to_explore ( struct find_command_info *, char * );

void update_file_to_find ( struct find_command_info *, char * );

void delete_find_command_info ( struct find_command_info * );

void run_find_command ( struct find_command_info *, char * );

struct memory_blocks_list;

struct memory_blocks_list * create_memory_blocks_list ( int );

int load_file_to_memory_block ( struct memory_blocks_list * list,
								char * file_path );

void delete_memory_block_from_list ( struct memory_blocks_list *, int );

void delete_memory_blocks_list ( struct memory_blocks_list * );

#endif