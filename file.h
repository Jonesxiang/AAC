#ifndef _FILE_H
#define _FILE_H

#include <stdio.h>
#include <stdlib.h>

char * read_file(char * file_name, int * buf_len);

FILE * open_file(char * file_name, char * mode);

#endif /* _FILE_H */