#include "file.h"

FILE * open_file(char * file_name, char * mode)
{
	FILE * file = fopen (file_name, mode );
	if (file==NULL) {
		fprintf(stderr, "Open file:%s failed!\n", file_name);
		exit(1);
	}
	return file;
}

char * read_file(char * file_name, int * buf_len)
{
	FILE * file = open_file(file_name, "rb");

	/* obtain the number of char */
	fseek(file , 0 , SEEK_END);
	int size = ftell(file);
	rewind(file);

	/* allocate */
	char * buffer = (char*) malloc (sizeof(char) * size);
	if (buffer == NULL) {
		fprintf(stderr, "Malloc memory error\n");
		exit(1);
	}

	/* read */
	size_t result = fread(buffer, sizeof(char), size, file);
	if (result != size) {
		fprintf(stderr, "Reading file:%s error", file_name);
		exit(1);
	}

	fclose (file);
	*buf_len = size;
	return buffer;
}
