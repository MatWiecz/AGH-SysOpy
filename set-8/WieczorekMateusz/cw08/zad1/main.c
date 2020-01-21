#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <pthread.h>
#include <sys/time.h>

#define PROCESS_METHOD_BLOCK 1
#define PROCESS_METHOD_INTERLEAVED 2

long long my_round(double value)
{
	long long integer = (long long) value;
	double after_coma = value - integer;
	if (after_coma < 0.5)
		return integer;
	else
		return integer + 1;
}

long long my_ceil(double value)
{
	long long integer = (long long) value;
	if (integer == value)
		return integer;
	else
		return integer + 1;
}

struct image_area
{
	unsigned short left;
	unsigned short top;
	unsigned short right;
	unsigned short bottom;
};

struct image_filter
{
	unsigned short filter_size;
	double *filter_data;
};

int create_image_filter(struct image_filter *image_filter_struct,
						unsigned short size)
{
	if (image_filter_struct == NULL || size == 0)
		return -1;
	image_filter_struct->filter_size = size;
	image_filter_struct->filter_data = calloc(size * size, sizeof(double));
	memset(image_filter_struct->filter_data, 0, size * size * sizeof(double));
	return 0;
}

int destroy_image_filter(struct image_filter *image_filter_struct)
{
	if (image_filter_struct == NULL || image_filter_struct->filter_size == 0)
		return -1;
	free(image_filter_struct->filter_data);
	image_filter_struct->filter_size = 0;
	return 0;
}

int load_image_filter(struct image_filter *image_filter_struct,
					  char *file_path)
{
	if (image_filter_struct == NULL || file_path == NULL)
		return -1;
	FILE *filter_file = fopen(file_path, "r");
	if (filter_file == NULL)
	{
		fprintf(stderr, "Cannot open filter file with path \"%s\"!\n",
				file_path);
		return -2;
	}
	unsigned short filter_size;
	fscanf(filter_file, "%hd", &filter_size);
	if (create_image_filter(image_filter_struct, filter_size))
	{
		fclose(filter_file);
		return -3;
	}
	for (unsigned short row = 0; row < filter_size; row++)
		for (unsigned short column = 0; column < filter_size; column++)
			fscanf(filter_file, "%lf", &(image_filter_struct->filter_data[
				row * image_filter_struct->filter_size + column]));
	fclose(filter_file);
	return 0;
}

int save_image_filter(struct image_filter *image_filter_struct,
					  char *file_path)
{
	if (image_filter_struct == NULL || file_path == NULL
		|| image_filter_struct->filter_size == 0)
		return -1;
	FILE *filter_file = fopen(file_path, "w");
	if (filter_file == NULL)
	{
		fprintf(stderr, "Cannot open/create filter file with path \"%s\"!\n",
				file_path);
		return -2;
	}
	unsigned short filter_size = image_filter_struct->filter_size;
	fprintf(filter_file, "%hd\n", filter_size);
	for (unsigned short row = 0; row < filter_size; row++)
	{
		for (unsigned short column = 0; column < filter_size; column++)
			fprintf(filter_file, "%lf ", image_filter_struct->filter_data[
				row * image_filter_struct->filter_size + column]);
		fprintf(filter_file, "\n");
	}
	fclose(filter_file);
	return 0;
}

void set_image_filter_factor(struct image_filter *image_filter_struct,
							 unsigned short row, unsigned short column,
							 double factor)
{
	image_filter_struct->filter_data[row * image_filter_struct->filter_size +
									 column] = factor;
}

double get_image_filter_factor(struct image_filter *image_filter_struct,
							   unsigned short row, unsigned short column)
{
	return image_filter_struct->filter_data[
		row * image_filter_struct->filter_size + column];
}

struct pgm_image
{
	unsigned short width;
	unsigned short height;
	unsigned short max_value;
	unsigned char *data;
};

int create_pgm_image(struct pgm_image *pgm_image_struct,
					 unsigned short width, unsigned short height,
					 unsigned short max_value)
{
	if (pgm_image_struct == NULL || width == 0 || height == 0 || max_value == 0)
		return -1;
	pgm_image_struct->width = width;
	pgm_image_struct->height = height;
	pgm_image_struct->max_value = max_value;
	pgm_image_struct->data = calloc(width * height, sizeof(unsigned char));
	memset(pgm_image_struct->data, 0, width * height * sizeof(unsigned char));
	return 0;
}

int destroy_pgm_image(struct pgm_image *pgm_image_struct)
{
	if (pgm_image_struct == NULL || pgm_image_struct->max_value == 0)
		return -1;
	free(pgm_image_struct->data);
	pgm_image_struct->width = 0;
	pgm_image_struct->height = 0;
	pgm_image_struct->max_value = 0;
	return 0;
}

void get_non_comment(FILE *file, char *string_buffer)
{
	fscanf(file, "%s", string_buffer);
	while (string_buffer[0] == '#')
	{
		fscanf(file, "%[^\n]%c", &string_buffer[0]);
		fscanf(file, "%s", string_buffer);
	}
}

int load_pgm_image(struct pgm_image *pgm_image_struct, char *file_path)
{
	if (pgm_image_struct == NULL || file_path == NULL)
		return -1;
	FILE *pgm_image_file = fopen(file_path, "r");
	if (pgm_image_file == NULL)
	{
		fprintf(stderr, "Cannot open PGM image file with path \"%s\"!\n",
				file_path);
		return -2;
	}
	char pgm_watermark[32] = "";
	fscanf(pgm_image_file, "%s", pgm_watermark);
	if (strcmp(pgm_watermark, "P2") != 0)
	{
		fprintf(stderr, "Not valid format of PGM image file!\n");
		fclose(pgm_image_file);
		return -3;
	}
	unsigned short width;
	unsigned short height;
	unsigned short max_value;
	char string_buffer[128] = "";
	get_non_comment(pgm_image_file, string_buffer);
	width = (unsigned short) strtol(string_buffer, NULL, 10);
	get_non_comment(pgm_image_file, string_buffer);
	height = (unsigned short) strtol(string_buffer, NULL, 10);
	get_non_comment(pgm_image_file, string_buffer);
	max_value = (unsigned short) strtol(string_buffer, NULL, 10);
	if (width == 0 || height == 0 || max_value == 0)
	{
		fprintf(stderr, "Not valid or supported format of PGM image file!\n");
		fclose(pgm_image_file);
		return -4;
	}
	if (create_pgm_image(pgm_image_struct, width, height, max_value))
	{
		fclose(pgm_image_file);
		return -5;
	}
	for (unsigned short row = 0; row < height; row++)
		for (unsigned short column = 0; column < width; column++)
		{
			fscanf(pgm_image_file, "%hhd", &(pgm_image_struct->data[
				row * pgm_image_struct->width + column]));
		}
	fclose(pgm_image_file);
	return 0;
}

int save_pgm_image(struct pgm_image *pgm_image_struct, char *file_path)
{
	if (pgm_image_struct == NULL || file_path == NULL
		|| pgm_image_struct->width == 0)
		return -1;
	FILE *pgm_image_file = fopen(file_path, "w");
	if (pgm_image_file == NULL)
	{
		fprintf(stderr, "Cannot open/create PGM image file with path \"%s\"!\n",
				file_path);
		return -2;
	}
	fprintf(pgm_image_file, "P2\n%hd %hd\n%hd\n",
			pgm_image_struct->width, pgm_image_struct->height,
			pgm_image_struct->max_value);
	for (unsigned short row = 0; row < pgm_image_struct->height; row++)
	{
		for (unsigned short column = 0; column < pgm_image_struct->width;
			 column++)
		{
			fprintf(pgm_image_file, "%hd ", pgm_image_struct->data[
				row * pgm_image_struct->width + column]);
		}
		fprintf(pgm_image_file, "\n");
	}
	fclose(pgm_image_file);
	return 0;
}

void set_pixel(struct pgm_image *pgm_image_struct,
			   unsigned short row, unsigned short column,
			   unsigned char value)
{
	pgm_image_struct->data[row * pgm_image_struct->width + column] = value;
}

unsigned char get_pixel(struct pgm_image *pgm_image_struct,
						unsigned short row, unsigned short column)
{
	return pgm_image_struct->data[row * pgm_image_struct->width + column];
}

int process_pgm_image(struct pgm_image *input_image,
					  struct pgm_image *output_image,
					  struct image_filter *filter, struct image_area *area)
{
	if (input_image == NULL || input_image->max_value == 0
		|| output_image == NULL || output_image->max_value == 0
		|| input_image->width != output_image->width
		|| input_image->height != output_image->height
		|| input_image->max_value != output_image->max_value
		|| filter == NULL || filter->filter_size == 0 || area == NULL
		|| area->left >= area->right || area->top >= area->bottom
		|| area->right > input_image->width
		|| area->bottom > input_image->height)
		return -1;
	double element_1 = my_ceil(filter->filter_size / 2);
	for (unsigned short row = area->top; row < area->bottom; row++)
		for (unsigned short column = area->left; column < area->right; column++)
		{
			double new_pixel_value = 0.0;
			for (unsigned short y = 0; y < filter->filter_size; y++)
				for (unsigned short x = 0; x < filter->filter_size; x++)
				{
					int source_row = (int) (row - element_1 + y);
					if (source_row < 0)
						source_row = 0;
					else
						if (source_row > input_image->height - 1)
							source_row = input_image->height - 1;
					int source_column = (int) (column - element_1 + x);
					if (source_column < 0)
						source_column = 0;
					else
						if (source_column > input_image->width - 1)
							source_column = input_image->width - 1;
					new_pixel_value += get_image_filter_factor(filter, y, x)
									   * get_pixel(input_image,
												   (unsigned short) source_row,
												   (unsigned short) source_column);
				}
			new_pixel_value = my_round(new_pixel_value);
			if (new_pixel_value < 0)
				new_pixel_value = 0;
			if (new_pixel_value > input_image->max_value)
				new_pixel_value = input_image->max_value;
			set_pixel(output_image, row, column,
					  (unsigned char) new_pixel_value);
		}
	return 0;
}

struct image_filterer_thread_common_arg
{
	struct pgm_image *input_image;
	struct pgm_image *output_image;
	struct image_filter *filter;
	unsigned short threads_num;
	unsigned char process_method;
};

struct image_filterer_thread_arg
{
	struct image_filterer_thread_common_arg *common_arg;
	unsigned short thread_id;
};

void *image_filterer_thread(void *argument)
{
	struct timeval start_time;
	gettimeofday(&start_time, NULL);
	struct image_filterer_thread_arg *arg =
		(struct image_filterer_thread_arg *) argument;
	struct image_area area;
	area.top = 0;
	area.bottom = arg->common_arg->input_image->height;
	switch (arg->common_arg->process_method)
	{
		case PROCESS_METHOD_BLOCK:
		{
			area.left = (unsigned short) (arg->thread_id * my_ceil(((double)
				arg->common_arg->input_image->width) / arg->common_arg->
				threads_num));
			area.right =
				(unsigned short) ((arg->thread_id + 1) * my_ceil(((double)
					arg->common_arg->input_image->width) / arg->common_arg->
					threads_num));
			process_pgm_image(arg->common_arg->input_image, arg->common_arg->
								  output_image, arg->common_arg->filter, &area);
			break;
		}
		case PROCESS_METHOD_INTERLEAVED:
		{
			unsigned short cur_x = arg->thread_id;
			while (cur_x < arg->common_arg->input_image->width)
			{
				area.left = cur_x;
				area.right = (unsigned short) (cur_x + 1);
				process_pgm_image(arg->common_arg->input_image,
								  arg->common_arg->output_image,
								  arg->common_arg->filter, &area);
				cur_x += arg->common_arg->threads_num;
			}
			break;
		}
		default:
			break;
	}
	struct timeval stop_time;
	gettimeofday(&stop_time, NULL);
	long long delta_time = stop_time.tv_sec*1000000+stop_time.tv_usec;
	delta_time -= start_time.tv_sec*1000000+start_time.tv_usec;
	pthread_exit((void*)delta_time);
}

int main(int argc, char *argv[])
{
	if (argc != 6)
	{
		fprintf(stderr, "Wrong number of arguments! Expected 5: threads "
			"number, process method, input input_image's path, input filter's path,"
			" output input_image's path!\n");
		exit(-1);
	}
	unsigned short threads_num = (unsigned short) strtol(argv[1], NULL, 10);
	if(threads_num == 0)
	{
		fprintf(stderr, "Threads number must be positive integer number!\n");
		exit(-1);
	}
	unsigned char process_method = 0;
	if (strcmp(argv[2], "block") == 0)
		process_method = PROCESS_METHOD_BLOCK;
	if (strcmp(argv[2], "interleaved") == 0)
		process_method = PROCESS_METHOD_INTERLEAVED;
	if (!process_method)
	{
		fprintf(stderr, "Unrecognised process method! Expected \"block\" "
			"or \"interleaved\"!\n");
		exit(-1);
	}
	char *input_image_path = argv[3];
	char *filter_path = argv[4];
	char *output_image_path = argv[5];
	struct pgm_image input_image;
	if(load_pgm_image(&input_image, input_image_path))
		exit(-2);
	struct image_filter filter;
	if(load_image_filter(&filter, filter_path))
		exit(-3);
	struct pgm_image output_image;
	create_pgm_image(&output_image, input_image.width, input_image.height,
					 input_image.max_value);
	pthread_t *threads = calloc(threads_num, sizeof(pthread_t));
	struct image_filterer_thread_arg *threads_args =
		calloc(threads_num, sizeof(struct image_filterer_thread_arg));
	struct image_filterer_thread_common_arg common_arg;
	common_arg.threads_num = threads_num;
	common_arg.process_method = process_method;
	common_arg.input_image = &input_image;
	common_arg.filter = &filter;
	common_arg.output_image = &output_image;
	struct timeval start_time;
	gettimeofday(&start_time, NULL);
	for (unsigned short thread_id = 0; thread_id < threads_num; thread_id++)
	{
		threads_args[thread_id].common_arg = &common_arg;
		threads_args[thread_id].thread_id = thread_id;
		pthread_create(threads + thread_id, NULL, image_filterer_thread,
					   threads_args + thread_id);
	}
	for (unsigned short thread_id = 0; thread_id < threads_num; thread_id++)
	{
		void *delta_time;
		pthread_join(threads[thread_id], &delta_time);
		fprintf(stdout, "Thread with id %ld exited after %lld us of working.\n",
				(long) threads[thread_id], (long long) delta_time);
	}
	struct timeval stop_time;
	gettimeofday(&stop_time, NULL);
	long long delta_time = stop_time.tv_sec*1000000+stop_time.tv_usec;
	delta_time -= start_time.tv_sec*1000000+start_time.tv_usec;
	fprintf(stdout, "The image has been filtered in %lld us!\n", delta_time);
	free(threads_args);
	free(threads);
	save_pgm_image(&output_image, output_image_path);
	destroy_pgm_image(&input_image);
	destroy_pgm_image(&output_image);
	destroy_image_filter(&filter);
	exit(0);
}