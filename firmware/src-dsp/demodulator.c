#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include "demodulator.h"



//t_fir_state init_fir_filter(const uint8_t length, const int32_t *coeff )
//{
//
//	t_fir_state out;
//	out.coeff = coeff;
//	out.delay_line
//}

void fir_filter(t_fir_state state, int32_t *input, int32_t *output, uint16_t len)
{

	uint16_t i,j;

	for (i = 0; i < len; i++)
	{

		*output = state.coeff[0] * *input;			//first tap


		for (j = 1; j < state.length-1; j++)
		{
			*output = *output + (state.delay_line[state.delay_ptr] * state.coeff[j]);
			if (state.delay_ptr == 0)
				state.delay_ptr = state.length - 2;
			else
				state.delay_ptr--;
		}

		*output = *output + (state.delay_line[state.delay_ptr] * state.coeff[state.length -1]);
		state.delay_line[state.delay_ptr] = *input;

		input++;
		output++;
	}
}


uint16_t cic_filter(t_cic_state state, int32_t *input, int32_t *output, uint16_t len)
{

	uint16_t i,j,out_count;
	out_count = 0;
	int32_t prev,next;

	for (i = 0; i < len; i++)
	{
		prev = *input;
		input++;
		//update all the input delay lines
		for (j = 0; j < state.stages; j++)
		{
			state.delay_line_in[j] = state.delay_line_in[j] + prev;
			prev = state.delay_line_in[j];
		}

		state.rate_count++;
		if (state.rate_count >= state.rate)
		{
			state.rate_count=0;
			for (j = 0; j < state.stages; j++)
			{
				next = prev - state.delay_line_out[j];
				state.delay_line_out[j] = prev;
				prev = next;
			}
			*output = prev;
			output++;
			out_count++;
		}
	}

	return out_count;
}


