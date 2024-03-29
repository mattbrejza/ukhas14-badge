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


#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include "demodulator.h"


inline int8_t sign(int32_t in){if (in > 0){return 1;}else{return -1;}}
inline int8_t max(int8_t in1, int8_t in2){if (in1 > in2){return in1;}else{return in2;}}
inline int8_t min(int8_t in1, int8_t in2){if (in1 > in2){return in2;}else{return in1;}}

void fir_filter(fir_state_t *state, int32_t *input, int32_t *output, uint16_t len)
{

	uint16_t i,j;

	for (i = 0; i < len; i++)
	{

		*output = state->coeff[0] * *input;			//first tap


		for (j = 1; j < state->length-1; j++)
		{
			*output = *output + (state->delay_line[state->delay_ptr] * state->coeff[j]);
			if (state->delay_ptr == 0)
				state->delay_ptr = state->length - 2;
			else
				state->delay_ptr--;
		}

		*output = *output + (state->delay_line[state->delay_ptr] * state->coeff[state->length -1]);
		state->delay_line[state->delay_ptr] = *input;

		input++;
		output++;
	}
}

//be careful of bit growth
uint16_t cic_filter(cic_state_t *state, int32_t *input, int32_t *output, uint16_t len)
{

	uint16_t i,j,out_count;
	out_count = 0;
	int32_t prev,next;

	for (i = 0; i < len; i++)
	{
		prev = *input;
		input++;
		//update all the input delay lines
		for (j = 0; j < state->stages; j++)
		{
			state->delay_line_in[j] = state->delay_line_in[j] + prev;
			prev = state->delay_line_in[j];
		}

		state->rate_count++;
		if (state->rate_count >= state->rate)
		{
			state->rate_count=0;
			for (j = 0; j < state->stages; j++)
			{
				next = prev - state->delay_line_out[j];
				state->delay_line_out[j] = prev;
				prev = next;
			}
			*output = prev;
			output++;
			out_count++;
		}
	}

	return out_count;
}

//early-late bit sync (currently a bit broken)
uint16_t bit_sync(bit_sync_state_t *state, int32_t *input, int32_t *output, uint16_t len)
{

	uint16_t i,count;
	count = 0;

	for (i = 0; i < len; i++)
	{
		switch (state->pos)
		{
		case 0:
			state->late = sign(*input);
			break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			state->late += sign(*input);
			break;

		case 6:
			state->late += sign(*input);
			state->error = abs(state->late) - abs(state->early);
			state->error_i += state->error;
			state->error_i = max(state->error_i, -2);
			state->error_i = min(state->error_i, 2);
			break;

		case 7:
			if (state->vco < 103)
				state->pos = 6;
			//else if (state->vco > 137)
			//	state->pos = 9;
			else if (state->vco > 122)
				state->pos = 8;
			break;

		case 8:  //sample
			*output = *input;
			output++;
			count++;
			break;

		case 9:
			state->early = sign(*input);
			break;
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			state->early += sign(*input);
			break;
		}

		state->pos++;
		if (state->pos >= 16)
			state->pos = 0;

		//                 P=0.5               I=0.06
		state->vco += 16 + (state->error>>1)  + (state->error_i>>1);

		input++;
	}
	return count;
}


//uses the method which looks for a start bit and counts samples
uint16_t char_sync_count(char_count_state_t *state, int32_t *input, char *output, uint16_t len, uint8_t data_bits)
{
	uint16_t i,out_count;
	out_count = 0;
	for (i = 0; i < len; i++)
	{

		if (state->bit_counter == 0)  //idle state
		{
			state->mask = 1;
			state->current_char = 0;
			if (*input < 0){
				state->bit_counter = data_bits+1;
				state->sample_counter = 16+8-1;
			}
		}
		else
		{
			state->sample_counter--;
			if (state->sample_counter == 0)
			{
				state->sample_counter = 15;
				if (*input > 0)
					state->current_char |= state->mask;
				state->mask = state->mask << 1;

				state->bit_counter--;
				if (state->bit_counter == 1){
					*output = state->current_char;
					out_count++;
					output++;
				}
			}
		}

		input++;

	}
	return out_count;
}

uint16_t char_sync(char_sync_state_t *state, int32_t *input, char *output, uint16_t len, uint8_t data_bits)
{
	uint16_t i,out_count;
	out_count = 0;
	for (i = 0; i < len; i++)
	{

		if (state->bit_counter == 0)  //idle state
		{
			state->mask = 1;
			state->current_char = 0;
			if (*input < 0)
				state->bit_counter = data_bits + 1;
		}
		else
		{
			if (*input > 0)
				state->current_char |= state->mask;
			state->mask = state->mask << 1;

			state->bit_counter--;
			if (state->bit_counter == 1){
				*output = state->current_char;
				out_count++;
				output++;
			}
		}

		input++;

	}
	return out_count;
}

