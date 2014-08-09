/**
 * This file is part of the UKHAS 2014 Badge project
 *
 * Jon Sowman 2014
 * <jon@jonsowman.com>
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/timer.h>



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "badge.h"
#include "lcd.h"
#include "demodulator.h"

void init(void);

#define BUFF_LEN 128

int16_t adc_buffer[BUFF_LEN*2];
int16_t adc_buffer[] = {-333, -506, -436, -157, 200, 462, 488, 236, -176, -481, -440, -78, 332, 512, 354, -45, -418, -496, -228, 191, 482, 447, 104, -311, -511, -369, 18, 395, 505, 272, -143, -462, -471, -160, 260, 502, 409, 41, -357, -511, -319, 87, 435, 490, 213, -209, -488, -439, -95, 315, 511, 361, -31, -403, -503, -262, 156, 468, 466, 148, -269, -505, -399, -25, 367, 511, 309, -101, -443, -486, -201, 221, 492, 432, 81, -326, -512, -352, 46, 412, 501, 251, -169, -474, -459, -135, 281, 507, 390, 11, -376, -509, -298, 115, 450, 481, 188, -233, -496, -424, -66, 336, 512, 342, -60, -420, -498, -239, 181, 479, 453, 121, -292, -509, -381, 4, 386, 508, 287, -128, -456, -476, -175, 245, 499, 416, 52, -347};
int32_t buffer1[BUFF_LEN];
int32_t buffer2[BUFF_LEN];   //make smaller
int32_t buffer3[BUFF_LEN/4];
int32_t dl_cic[32] = {0};
int32_t dl_fir[60] = {0};
char textout[5] = {0};

const int32_t coeff[] = {-3, 13, 34, 56, 78, 98, 114, 125, 128, 125, 114, 98, 78, 56, 34, 13, -3};

//l=8
const int16_t sin_lut_1k[] = {0, 707, 1000, 707, 0, -707, -1, -707};
const int16_t cos_lut_1k[]  = {1000, 707, 0, -707, -1, -707, 0, 707};
//l=32
const int16_t sin_lut_750[] = {0, 556, 924, 981, 707, 195, -383, -831, -1000, -831, -383, 195, 707, 981, 924, 556, 0, -556, -924, -981, -707, -195, 383, 831, 1000, 831, 383, -195, -707, -981, -924, -556};
const int16_t cos_lut_750[] = {1000, 831, 383, -195, -707, -981, -924, -556, 0, 556, 924, 981, 707, 195, -383, -831, -1000, -831, -383, 195, 707, 981, 924, 556, 0, -556, -924, -981, -707, -195, 383, 831};

t_cic_state cs1 = {.delay_line_in = &dl_cic[0], .delay_line_out = &dl_cic[4], .stages = 4, .rate = 5};
t_cic_state cs2 = {.delay_line_in = &dl_cic[8], .delay_line_out = &dl_cic[12], .stages = 4, .rate = 5};
t_cic_state cs3 = {.delay_line_in = &dl_cic[16], .delay_line_out = &dl_cic[20], .stages = 4, .rate = 5};
t_cic_state cs4 = {.delay_line_in = &dl_cic[24], .delay_line_out = &dl_cic[28], .stages = 4, .rate = 5};
t_fir_state fs1 = {.coeff = coeff, .delay_ptr = 0, .delay_line = &dl_fir[0], .length = 16};
t_fir_state fs2 = {.coeff = coeff, .delay_ptr = 0, .delay_line = &dl_fir[15], .length = 16};
t_fir_state fs3 = {.coeff = coeff, .delay_ptr = 0, .delay_line = &dl_fir[30], .length = 16};
t_fir_state fs4 = {.coeff = coeff, .delay_ptr = 0, .delay_line = &dl_fir[45], .length = 16};
t_bit_sync_state s2;
t_char_sync_state s3;

int main(void)
{
    init();
    lcd_init();
    lcd_clear();
    lcd_set_display_ptr(2,0,3,127);
    lcd_write_string("SUSF / APEX / ASTRA");
    lcd_write_string_medium("Matt Brejza", 0, 0);
    /* Blink the LED (PC8) on the board. */

    char buff[20];




	gpio_mode_setup(GPIOF, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);









	uint8_t lo1_p = 0,lo2_p = 0;



	while(1)
	{
		uint8_t i,lo;
		lo = lo1_p;

		_delay_ms(100);
		gpio_set(GPIOF, GPIO5);
		//process sin 1k
		for (i = 0; i < BUFF_LEN; i++)
		{
			buffer1[i] = (adc_buffer[i] * sin_lut_1k[lo]) >> 12;   //!!!!!!!!11
			lo++;
			if (lo >= 8)
				lo = 0;
		}
		uint8_t c1 = cic_filter(cs1, buffer1, buffer2,  BUFF_LEN);
		fir_filter(fs1, buffer2, buffer1,  c1);
		for (i = 0; i < c1; i++)
		{
			buffer1[i] = buffer1[i] >> 18;
			buffer3[i] = buffer1[i] * buffer1[i];
		}

		//process cos 1k
		lo = lo1_p;
		for (i = 0; i < BUFF_LEN; i++)
		{
			buffer1[i] = (adc_buffer[i] * cos_lut_1k[lo]) >> 12;   //!!!!!!!!11
			lo++;
			if (lo >= 8)
				lo = 0;
		}
		c1 = cic_filter(cs2, buffer1, buffer2,  BUFF_LEN);
		fir_filter(fs2, buffer2, buffer1,  c1);
		for (i = 0; i < c1; i++)
		{
			buffer1[i] = buffer1[i] >> 18;
			buffer3[i] += buffer1[i] * buffer1[i];
		}
		lo1_p = lo;

		//process sin 750
		lo = lo2_p;
		for (i = 0; i < BUFF_LEN; i++)
		{
			buffer1[i] = (adc_buffer[i] * sin_lut_750[lo]) >> 12;   //!!!!!!!!11
			lo++;
			if (lo >= 32)
				lo = 0;
		}
		c1 = cic_filter(cs3, buffer1, buffer2,  BUFF_LEN);
		fir_filter(fs3, buffer2, buffer1,  c1);
		for (i = 0; i < c1; i++)
		{
			buffer1[i] = buffer1[i] >> 18;
			buffer3[i] -= buffer1[i] * buffer1[i];
		}

		//process cos 750
		lo = lo2_p;
		for (i = 0; i < BUFF_LEN; i++)
		{
			buffer1[i] = (adc_buffer[i] * cos_lut_750[lo]) >> 12;   //!!!!!!!!11
			lo++;
			if (lo >= 32)
				lo = 0;
		}
		c1 = cic_filter(cs4, buffer1, buffer2,  BUFF_LEN);
		fir_filter(fs4, buffer2, buffer1,  c1);
		for (i = 0; i < c1; i++)
		{
			buffer1[i] = buffer1[i] >> 18;
			buffer3[i] -= buffer1[i] * buffer1[i];
		}
		lo2_p = lo;


		uint8_t c2 =  bit_sync(s2, buffer3, buffer1, c1);
		uint8_t c3 = char_sync(s3, buffer1, textout, c2, 7);
		gpio_clear(GPIOF, GPIO5);

	}







/*
    int32_t input[100];
    int32_t output[100] = {0};
	int32_t delay_line1[4] = {0};
	int32_t delay_line2[4] = {0};
    const int32_t coeff[] = {0, -2, -4, 7, 17, 0, -36, -31, 41, 90, 0, -166, -146, 230, 768, 1024, 768, 230, -146, -166, 0, 90, 41, -31, -36, 0, 17, 7, -4, -2, 0};
	int32_t delay_line[30] = {0};
	t_fir_state s;
	s.coeff = coeff;
	s.delay_ptr = 0;
	s.delay_line = delay_line;
	s.length = 31;

	t_cic_state s1 = {.delay_line_in = delay_line1, .delay_line_out = delay_line2, .stages = 4, .rate = 5};




    while(1)
    {



		gpio_set(GPIOF, GPIO5);
		fir_filter(s, input, output,  100);
		gpio_clear(GPIOF, GPIO5);

		_delay_ms(60);

		//gpio_set(GPIOF, GPIO5);
		//cic_filter(s1, input, output,  100);
		//gpio_clear(GPIOF, GPIO5);
		_delay_ms(90);

    } */
    while (1){
        _delay_ms(1000);

        sprintf(buff,"t: %u     ",(unsigned int)TIM3_CNT);
       // itostr(buff,TIM3_CNT);
        lcd_set_display_ptr(2,0,3,127);
        lcd_write_string(buff);

        //gpio_set(GPIOF, GPIO1);
        //_delay_ms(1000);

       //gpio_clear(GPIOF, GPIO1);
        //}
        
        if (!(GPIOF_IDR & GPIO1))
        	lcd_write_string("A");
		
		}

    return 0;
}

void init(void)
{
    rcc_periph_clock_enable(RCC_GPIOF);
    rcc_periph_clock_enable(RCC_GPIOB);
    gpio_mode_setup(GPIOF, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO1);
    
    rcc_periph_clock_enable(RCC_TIM3);
    timer_reset(TIM3);
    timer_set_mode(TIM3, TIM_CR1_CKD_CK_INT_MUL_2,
               TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    gpio_set_af(GPIOB, GPIO_AF1, GPIO1);
    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO1);
              
    timer_set_oc_mode(TIM3, TIM_OC4, TIM_OCM_PWM2);
    timer_enable_oc_output(TIM3, TIM_OC4);
    timer_set_oc_value(TIM3, TIM_OC4, 200);
    timer_set_period(TIM3, 1000);
    timer_enable_counter(TIM3);

}

/**
 * Approximately delay for the given time period
 * @param delay The number of milliseconds to delay
 */
void _delay_ms(const uint32_t delay)
{
    uint32_t i, j;

    for(i=0; i< delay; i++)
        for(j=0; j<1000; j++)
            __asm__("nop");
}
