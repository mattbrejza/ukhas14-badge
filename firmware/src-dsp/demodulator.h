/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Matthew Brejza
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


typedef struct s_fir_state {
	const int32_t *coeff;
	int32_t *delay_line;
    uint8_t length;
    uint8_t delay_ptr;
} fir_state_t;

typedef struct s_cic_state {
	int32_t *delay_line_in;
	int32_t *delay_line_out;
    uint8_t stages;
    uint8_t rate;
    uint8_t rate_count;
} cic_state_t;

typedef struct s_bit_sync_state {
	uint8_t pos;
	int8_t late;
	int8_t early;
	int8_t error;
	int8_t error_i;
	uint8_t vco;	//full bit period = 16*16 = 256
} bit_sync_state_t;

typedef struct s_char_count_state {
	uint8_t bit_counter;
	char current_char;
	uint8_t mask;
	uint8_t sample_counter;
} char_count_state_t;


typedef struct s_char_sync_state{
	uint8_t bit_counter;
	char current_char;
	uint8_t mask;
} char_sync_state_t;



void fir_filter(fir_state_t *state, int32_t *input, int32_t *output, uint16_t len);
uint16_t cic_filter(cic_state_t *state, int32_t *input, int32_t *output, uint16_t len);
uint16_t bit_sync(bit_sync_state_t *state, int32_t *input, int32_t *output, uint16_t len);
uint16_t char_sync(char_sync_state_t *state, int32_t *input, char *output, uint16_t len, uint8_t data_bits);
uint16_t char_sync_count(char_count_state_t *state, int32_t *input, char *output, uint16_t len, uint8_t data_bits);

