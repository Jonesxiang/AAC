#include "aac.h"
#include "file.h"
#include <unistd.h>
#include <malloc.h>
#include <memory.h>

const char * __progname = "aac";
const char * __version = "0.1";
const char * __authors = "Jones XiangTao";

static void usage(void)
{
	printf("\naac version %s\n", __version);
	printf("Copyright (C) 2016 %s\n", __authors);
	printf("\nUsage: %s <command> <file>\n", __progname);
	printf("\n<commands>\n");
	printf(" -e  encode file\n");
	printf(" -d  decode file\n");
	printf(" -h  Show this help message\n");
}

FILE * fout;
void output_char(char c);

int main(int argc, char* argv[])
{
	int i;
	int eflag = 0;
	int dflag = 0;
	int hflag = 0;
	char * fin_name = NULL;
	while ((i = getopt(argc, argv, "e:d:h")) != -1) {
		switch(i) {
			case 'e':
				eflag = 1;
                fin_name = optarg;
				break;

			case 'd':
				dflag = 1;
                fin_name = optarg;
				break;

			default:
				usage();
				exit(1);
		}
	}

	if (eflag ^ dflag && fin_name != NULL) {
		int len;
		char * buf = read_file(fin_name, &len);
		char fout_name[100];
		if (eflag) {
			sprintf(fout_name, "%s.ac", fin_name);
			fout = open_file(fout_name, "wb");
			adaptive_arithmetic_encode(buf, len, output_char);
		} else {
			sprintf(fout_name, "%s.txt", fin_name);
			fout = open_file(fout_name, "wb");
			adaptive_arithmetic_decode(buf, len, output_char);
		}
		fclose(fout);
		free(buf);
	} else {
		usage();
		exit(1);
	}
	return 0;
}

void output_char(char c)
{
	fwrite(&c, sizeof(char), 1, fout);
}
