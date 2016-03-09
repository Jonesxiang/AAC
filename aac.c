#include<stdio.h>
#include<stdlib.h>
#include"aac.h"

#define AAC_CODE_VALUE_BITS 16

#define AAC_TOP_VALUE       (((unsigned long)1<<AAC_CODE_VALUE_BITS)-1)

#define AAC_FIRST_QTR       (AAC_TOP_VALUE/4+1)
#define AAC_HALF            (2*AAC_FIRST_QTR)
#define AAC_THIRD           (3*AAC_FIRST_QTR)

#define AAC_NO_OF_CHARS     256
#define AAC_EOF_SYMBOL      AAC_NO_OF_CHARS
#define AAC_NO_OF_SYMBOLS   (AAC_NO_OF_CHARS+1)

#define AAC_MAX_FREQUENCY   (AAC_TOP_VALUE>>2)
#define AAC_DOWN_SCALE      2

void (*aac_output_char)(char);

int aac_cum_freq[AAC_NO_OF_SYMBOLS+1];
int aac_freq[AAC_NO_OF_SYMBOLS];
int aac_buffer;
int aac_bits_to_go;
int aac_bits_to_follow;
unsigned long aac_low;
unsigned long aac_high;

unsigned long aac_value;
int aac_garbage_bits;
char * aac_code;
int aac_decode_index;
int aac_code_len;

void aac_update_cum_freq()
{
	int i;
	aac_cum_freq[AAC_NO_OF_SYMBOLS] = 0;
	for (i = AAC_NO_OF_SYMBOLS; i > 0; i--) {
		aac_cum_freq[i-1] = aac_cum_freq[i] + aac_freq[i-1];
	}
}

void aac_start_model()
{
	int i;
	for (i = 0; i < AAC_NO_OF_SYMBOLS; i++) {
		aac_freq[i] = 1;
	}
	aac_update_cum_freq();
}

void aac_update_model(int symbol)
{
	if (symbol >= AAC_NO_OF_SYMBOLS) {
		fprintf(stderr, "Bad symbol:%d for aac_update_cum_freq()\n", symbol);
		exit(1);
	}
	aac_freq[symbol]++;
	if (aac_cum_freq[0] >= AAC_MAX_FREQUENCY) {
		int i;
		for (i = 0; i < AAC_NO_OF_SYMBOLS; i++)
			aac_freq[i] = aac_freq[i]/AAC_DOWN_SCALE + 1;
	}
	aac_update_cum_freq();
}

/******************** encode **************************/
void aac_start_outputing_bits()
{
	aac_buffer = 0;
	aac_bits_to_go = 8;                          
}


void aac_output_bit(int bit)
{
	aac_buffer >>= 1;
	if (bit) aac_buffer |= 0x80;
	aac_bits_to_go -= 1;
	if (aac_bits_to_go==0) {
		aac_output_char((char) aac_buffer);
		aac_bits_to_go = 8;
	}
}

void aac_done_outputing_bits()
{
	aac_buffer >>= aac_bits_to_go;
	aac_output_char((char) aac_buffer);
}

void aac_start_encoding()
{
	aac_low = 0;
	aac_high = AAC_TOP_VALUE;
	aac_bits_to_follow = 0;
}

void aac_bit_plus_follow(int bit)
{
	aac_output_bit(bit);
	while (aac_bits_to_follow > 0) {
		aac_output_bit(!bit);
		aac_bits_to_follow -= 1;
	}
}

void aac_done_encoding()
{
	aac_bits_to_follow += 1;
	if (aac_low < AAC_FIRST_QTR) aac_bit_plus_follow(0);
	else aac_bit_plus_follow(1);
}

void aac_encode_symbol(int symbol)
{
	unsigned long range = (unsigned long)(aac_high - aac_low) + 1;

	aac_high = aac_low + (range*aac_cum_freq[symbol])/aac_cum_freq[0]-1;
	aac_low = aac_low + (range*aac_cum_freq[symbol+1])/aac_cum_freq[0];

	for (;;) {
		if (aac_high < AAC_HALF) {
			aac_bit_plus_follow(0);
		} else if (aac_low >= AAC_HALF) {
			aac_bit_plus_follow(1);
			aac_low -= AAC_HALF;
			aac_high -= AAC_HALF;
		} else if (aac_low >= AAC_FIRST_QTR && aac_high < AAC_THIRD) {
			aac_bits_to_follow += 1;
			aac_low -= AAC_FIRST_QTR;
			aac_high -= AAC_FIRST_QTR;
		} else break;
		aac_low = 2*aac_low;
		aac_high = 2*aac_high+1;
	}
}

void adaptive_arithmetic_encode(char * chs, int chs_len, void (*output_char)(char))
{
	aac_output_char = output_char;
	aac_start_model();
	aac_start_outputing_bits();
	aac_start_encoding();
	int i;
	for (i = 0; i < chs_len; i++) {
		int symbol = 0xff & chs[i];
		aac_encode_symbol(symbol);
		aac_update_model(symbol);
	}
	aac_encode_symbol(AAC_EOF_SYMBOL);
	aac_done_encoding();
	aac_done_outputing_bits();
}


/******************** decode **************************/
void aac_start_inputing_bits()
{
	aac_bits_to_go = 0;
	aac_garbage_bits = 0;
}

int aac_input_bit()
{
	int t;
	if (aac_bits_to_go == 0) {
		aac_buffer = aac_code[aac_decode_index];
		aac_decode_index++;
		if(aac_decode_index > aac_code_len ) {
			aac_garbage_bits += 1;
			if (aac_garbage_bits > AAC_CODE_VALUE_BITS - 2) {
				fprintf(stderr, "Bad input file\n");
				exit(1);
			}
		}
		aac_bits_to_go = 8;
	}
	t = aac_buffer&1;
	aac_buffer >>= 1;
	aac_bits_to_go -= 1;
	return t;
}

void aac_start_decoding()
{
	aac_decode_index = 0;
	int i;
	aac_value = 0;
	for (i = 1; i<= AAC_CODE_VALUE_BITS; i++) {
		aac_value = 2*aac_value+aac_input_bit();
	}
	aac_low = 0;
	aac_high = AAC_TOP_VALUE;
}


int aac_decode_symbol()
{
	unsigned long range = (unsigned long)(aac_high - aac_low) + 1;
	int cum = (((unsigned long)(aac_value - aac_low) + 1) * aac_cum_freq[0] - 1)/range;

	int symbol;
	for (symbol = 0; aac_cum_freq[symbol+1]>cum; symbol++);
	aac_high = aac_low + (range*aac_cum_freq[symbol])/aac_cum_freq[0]-1;
	aac_low = aac_low + (range*aac_cum_freq[symbol+1])/aac_cum_freq[0];
	for (;;) {
		if (aac_high < AAC_HALF) {
		
		} else if (aac_low >= AAC_HALF) {
			aac_value -= AAC_HALF;
			aac_low -= AAC_HALF;
			aac_high -= AAC_HALF; 
		} else if (aac_low >= AAC_FIRST_QTR && aac_high <AAC_THIRD) {
			aac_value -= AAC_FIRST_QTR;
			aac_low -= AAC_FIRST_QTR;
			aac_high -= AAC_FIRST_QTR;
		} else break;
		aac_low = 2*aac_low;
		aac_high = 2*aac_high+1;
		aac_value = 2*aac_value+aac_input_bit();
	}
	return symbol;
}

void adaptive_arithmetic_decode(char * chs, int chs_len, void (*output_char)(char))
{
	aac_code = chs;
	aac_code_len = chs_len;
	aac_start_model();
	aac_start_inputing_bits();
	aac_start_decoding();
	for (;;) {
		int symbol = aac_decode_symbol();
		if (symbol==AAC_EOF_SYMBOL) break;
		char ch = (char) (0xff & symbol);
		output_char(ch);
		aac_update_model(symbol);
	}
}
