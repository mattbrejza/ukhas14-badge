/**
 * This file is part of the UKHAS 2014 Badge project
 *
 * Jon Sowman 2014
 * <jon@jonsowman.com>
 */
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/dma.h>



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "badge.h"
#include "lcd.h"
#include "demodulator.h"

void init(void);
static void my_usart_print_int(uint32_t usart, int32_t value);

#define BUFF_LEN 128

//volatile int16_t atest[] = {0, 119, 227, 295, 259, 189, 30, -34, -25, 98, 216, 324, 291, 245, 110, 5, -1, 55, 182, 275, 340, 255, 212, 120, 7, 47, 139, 243, 273, 241, 183, 31, -37, 22, 135, 247, 303, 294, 219, 124, 15, -16, 86, 179, 284, 292, 252, 207, 34, -10, 38, 152, 257, 286, 249, 179, 43, -9, 36, 133, 263, 292, 271, 211, 79, -7, -3, 95, 235, 312, 284, 242, 153, 36, 3, 63, 199, 265, 287, 198, 104, 20, -48, 44, 166, 275};

volatile int16_t adc_buffer[BUFF_LEN*2] = {0};
//int16_t adc_buffer[] = {-333, -506, -436, -157, 200, 462, 488, 236, -176, -481, -440, -78, 332, 512, 354, -45, -418, -496, -228, 191, 482, 447, 104, -311, -511, -369, 18, 395, 505, 272, -143, -462, -471, -160, 260, 502, 409, 41, -357, -511, -319, 87, 435, 490, 213, -209, -488, -439, -95, 315, 511, 361, -31, -403, -503, -262, 156, 468, 466, 148, -269, -505, -399, -25, 367, 511, 309, -101, -443, -486, -201, 221, 492, 432, 81, -326, -512, -352, 46, 412, 501, 251, -169, -474, -459, -135, 281, 507, 390, 11, -376, -509, -298, 115, 450, 481, 188, -233, -496, -424, -66, 336, 512, 342, -60, -420, -498, -239, 181, 479, 453, 121, -292, -509, -381, 4, 386, 508, 287, -128, -456, -476, -175, 245, 499, 416, 52, -347};
int32_t buffer1[BUFF_LEN];
int32_t buffer2[BUFF_LEN];   //make smaller
int32_t buffer3[BUFF_LEN/4];
int32_t dl_cic[32] = {0};
int32_t dl_fir[60] = {0};
char textout[5] = {0};

const int32_t coeff[] = {-3, 13, 34, 56, 78, 98, 114, 125, 128, 125, 114, 98, 78, 56, 34, 13, -3};

//l=8
const int16_t sin_lut_1k[] = {0, 707, 1000, 707, 0, -707, -1000, -707};
const int16_t cos_lut_1k[]  = {1000, 707, 0, -707, -1000, -707, 0, 707};
//l=32
const int16_t sin_lut_750[] = {0, 556, 924, 981, 707, 195, -383, -831, -1000, -831, -383, 195, 707, 981, 924, 556, 0, -556, -924, -981, -707, -195, 383, 831, 1000, 831, 383, -195, -707, -981, -924, -556};
const int16_t cos_lut_750[] = {1000, 831, 383, -195, -707, -981, -924, -556, 0, 556, 924, 981, 707, 195, -383, -831, -1000, -831, -383, 195, 707, 981, 924, 556, 0, -556, -924, -981, -707, -195, 383, 831};

t_cic_state cs1 = {.delay_line_in = &dl_cic[0], .delay_line_out = &dl_cic[4], .stages = 4, .rate = 10};
t_cic_state cs2 = {.delay_line_in = &dl_cic[8], .delay_line_out = &dl_cic[12], .stages = 4, .rate = 10};
t_cic_state cs3 = {.delay_line_in = &dl_cic[16], .delay_line_out = &dl_cic[20], .stages = 4, .rate = 10};
t_cic_state cs4 = {.delay_line_in = &dl_cic[24], .delay_line_out = &dl_cic[28], .stages = 4, .rate = 10};
t_fir_state fs1 = {.coeff = coeff, .delay_ptr = 0, .delay_line = &dl_fir[0], .length = 16};
t_fir_state fs2 = {.coeff = coeff, .delay_ptr = 0, .delay_line = &dl_fir[15], .length = 16};
t_fir_state fs3 = {.coeff = coeff, .delay_ptr = 0, .delay_line = &dl_fir[30], .length = 16};
t_fir_state fs4 = {.coeff = coeff, .delay_ptr = 0, .delay_line = &dl_fir[45], .length = 16};
t_bit_sync_state s2;
t_char_sync_state s3;
t_char_count_state s4;

volatile uint8_t half_complete_flag = 0;
volatile uint8_t full_complete_flag = 0;

int32_t last = 0;

int main(void)
{
    init();
    lcd_init();
    lcd_clear();
    lcd_set_display_ptr(2,0,3,127);
  //  lcd_write_string("SUSF / APEX / ASTRA");
  //  lcd_write_string_medium("Matt Brejza", 0, 0);
    /* Blink the LED (PC8) on the board. */

    char buff[20];




	gpio_mode_setup(GPIOF, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);
	gpio_mode_setup(GPIOF, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO4);
/*
	while(1)
	{
		//gpio_set(GPIOF, GPIO5);
		//adc_start_conversion_regular(ADC1);
		//while (!(adc_eoc(ADC1)));
		//gpio_clear(GPIOF, GPIO5);

		//uint16_t ar = adc_read_regular(ADC1);
		//uint32_t tv = DMA1_CMAR1;
		//my_usart_print_int(USART1, ADC1_DR);//adc_buffer[2]);
		my_usart_print_int(USART1, adc_buffer[0]);
		my_usart_print_int(USART1, ADC1_DR);
		_delay_ms(100);


	}*/







	uint8_t lo1_p = 0,lo2_p = 0;
	usart_send_blocking(USART1,"H");



	while(1)
	{
		uint8_t lo;
		uint16_t i,j;
		lo = lo1_p;


		uint16_t buff_end,buff_start;
		while ((half_complete_flag == 0) && (full_complete_flag == 0));
		if (half_complete_flag){
			buff_end = BUFF_LEN;
			buff_start = 0;
		}else {
			buff_end = BUFF_LEN<<1;
			buff_start = BUFF_LEN;
		}

		half_complete_flag = 0;
		full_complete_flag = 0;

		//nvic_disable_irq(NVIC_DMA1_CHANNEL1_IRQ);

		gpio_set(GPIOF, GPIO5);


		//process sin 1k
		j=0;
		for (i = buff_start; i < buff_end; i++)
		{
			buffer1[j] = (((int32_t)adc_buffer[i]-2000) * ((int32_t)(sin_lut_1k[lo]))) >> 12;   //!!!!!!!!11
			lo++;
			j++;
			if (lo >= 8)
				lo = 0;
		}
		uint8_t c1 = cic_filter(&cs1, buffer1, buffer2,  BUFF_LEN);
		for (i = 0; i < c1; i++){
			buffer2[i] = buffer2[i] >> 2;
			//my_usart_print_int(USART1, buffer2[i]);
		}
		fir_filter(&fs1, buffer2, buffer1,  c1);
		for (i = 0; i < c1; i++)
		{
			buffer1[i] = buffer1[i] >> 18;
			buffer3[i] = +(buffer1[i] * buffer1[i]);
//			my_usart_print_int(USART1, buffer1[i]);
		}

		//process cos 1k
		lo = lo1_p;
		j=0;
		for (i = buff_start; i < buff_end; i++)
		{
			buffer1[j] = ((adc_buffer[i]-2000) * cos_lut_1k[lo]) >> 12;   //!!!!!!!!11
			lo++;
			j++;
			if (lo >= 8)
				lo = 0;
		}
		c1 = cic_filter(&cs2, buffer1, buffer2,  BUFF_LEN);
		for (i = 0; i < c1; i++)
					buffer2[i] = buffer2[i] >> 2;
		fir_filter(&fs2, buffer2, buffer1,  c1);
		for (i = 0; i < c1; i++)
		{
			buffer1[i] = buffer1[i] >> 18;
			buffer3[i] += buffer1[i] * buffer1[i];
		}

		lo1_p = lo;

		//process sin 750
		lo = lo2_p;
		j=0;
		for (i = buff_start; i < buff_end; i++)
		{
			buffer1[j] = ((adc_buffer[i]-2000) * sin_lut_750[lo]) >> 12;   //!!!!!!!!11
			lo++;
			j++;
			if (lo >= 32)
				lo = 0;
		}
		c1 = cic_filter(&cs3, buffer1, buffer2,  BUFF_LEN);
		for (i = 0; i < c1; i++)
					buffer2[i] = buffer2[i] >> 2;
		fir_filter(&fs3, buffer2, buffer1,  c1);
		for (i = 0; i < c1; i++)
		{
			buffer1[i] = buffer1[i] >> 18;
			buffer3[i] -= buffer1[i] * buffer1[i];
		}

		//process cos 750
		lo = lo2_p;
		j=0;
		for (i = buff_start; i < buff_end; i++)
		{
			buffer1[j] = ((adc_buffer[i]-2000) * cos_lut_750[lo]) >> 12;   //!!!!!!!!11
			lo++;
			j++;
			if (lo >= 32)
				lo = 0;
		}
		c1 = cic_filter(&cs4, buffer1, buffer2,  BUFF_LEN);
		for (i = 0; i < c1; i++)
					buffer2[i] = buffer2[i] >> 2;
		fir_filter(&fs4, buffer2, buffer1,  c1);
		for (i = 0; i < c1; i++)
		{
			buffer1[i] = buffer1[i] >> 18;
			buffer3[i] -= buffer1[i] * buffer1[i];
		//	my_usart_print_int(USART1, buffer3[i]);

		}
		lo2_p = lo;


	//	uint8_t c2 =  bit_sync(&s2, buffer3, buffer1, c1);

		//my_usart_print_int(USART1, s2.error_i);
		//for (i = 0; i < c2; i++)
		//{
		//	if (buffer1[i] > 0)
		//		my_usart_print_int(USART1, 1);
		//	else
		//		my_usart_print_int(USART1, 0);
		//}

	//	uint8_t c3 = char_sync(&s3, buffer1, textout, c2, 7);
		uint8_t c3 = char_sync_count(&s4, buffer3, textout, c1, 7);
		textout[1] = 0;
		if (c3 > 0)
			lcd_write_string(&textout[0]);
			//usart_send_blocking(USART1, textout[0]);

		/*

		for (i = buff_start; i < buff_end; i++)
		{
			my_usart_print_int(USART1, adc_buffer[i]-last);
			last  = adc_buffer[i];
		}*/

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

void dma1_channel1_isr(void)
{

	if (dma_get_interrupt_flag(DMA1,1,DMA_HTIF)){
		dma_clear_interrupt_flags(DMA1,1,DMA_HTIF);
		half_complete_flag = 1;
		gpio_set(GPIOF, GPIO4);
	}

	if (dma_get_interrupt_flag(DMA1,1,DMA_TCIF)){
		dma_clear_interrupt_flags(DMA1,1,DMA_TCIF);
		full_complete_flag = 1;
		gpio_clear(GPIOF, GPIO4);
	}
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

    //adc
	rcc_periph_clock_enable(RCC_ADC);
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO0);
	uint8_t channel_array[] = { ADC_CHANNEL0};
	adc_power_off(ADC1);
	adc_calibrate_start(ADC1);
	adc_calibrate_wait_finish(ADC1);
	//adc_set_operation_mode(ADC1, ADC_MODE_SCAN); //adc_set_operation_mode(ADC1, ADC_MODE_SCAN_INFINITE);
	adc_set_operation_mode(ADC1, ADC_MODE_SCAN);//ADC_MODE_SCAN_INFINITE);
	//adc_disable_external_trigger_regular(ADC1); //adc_enable_external_trigger_regular(ADC1, 1<<6, 1<<10);
//	adc_set_single_conversion_mode(ADC1);
	adc_enable_external_trigger_regular(ADC1, 0<<6, 1<<10);//fucking libopencm3
	adc_set_right_aligned(ADC1);
	ADC1_CFGR1 |= ADC_CFGR1_DMACFG;
	adc_enable_dma(ADC1);   //!!!!!!!!!!!!!!!!!111
	adc_set_sample_time_on_all_channels(ADC1, ADC_SMPTIME_013DOT5);
	adc_set_regular_sequence(ADC1, 1, channel_array);
	adc_set_resolution(ADC1, ADC_RESOLUTION_12BIT);

	adc_disable_analog_watchdog(ADC1);
	adc_power_on(ADC1);


    //dma
    rcc_periph_clock_enable(RCC_DMA);
	dma_channel_reset(DMA1,1);
	nvic_enable_irq(NVIC_DMA1_CHANNEL1_IRQ);
	dma_set_peripheral_address(DMA1,1,(0x40000000U) + 0x12400 + 0x40);
	dma_set_memory_address(DMA1,1,(uint32_t)adc_buffer);
	dma_set_number_of_data(DMA1,1,BUFF_LEN*2);
	dma_enable_circular_mode(DMA1,1);
	dma_set_read_from_peripheral(DMA1,1);
	dma_enable_memory_increment_mode(DMA1,1);
//	dma_disable_peripheral_increment_mode(DMA1,1);
	dma_set_peripheral_size(DMA1,1,DMA_CCR_PSIZE_16BIT);
	dma_set_memory_size(DMA1,1,DMA_CCR_MSIZE_16BIT);
	//dma_disable_mem2mem_mode(DMA1,1);
	dma_set_priority(DMA1,1,DMA_CCR_PL_HIGH);
	dma_enable_half_transfer_interrupt(DMA1,1);
	dma_enable_transfer_complete_interrupt(DMA1,1);
	//dma_clear_interrupt_flags(DMA1,1,DMA_HTIF | DMA_TCIF);



	/*
	adc_power_off(ADC1);
	adc_set_operation_mode(ADC1, ADC_MODE_SCAN);
	adc_set_single_conversion_mode(ADC1);
	adc_enable_external_trigger_regular(ADC1,0,1<<10);
	//adc_enable_eoc_interrupt_injected(ADC1);
	adc_set_right_aligned(ADC1);
	adc_set_sample_time_on_all_channels(ADC1, ADC_SMPTIME_013DOT5);
	adc_power_on(ADC1);
*/
	_delay_ms(100);

	//tim1 (adc tigger)
	rcc_periph_clock_enable(RCC_TIM1);
	timer_reset(TIM1);
	//timer_set_mode(TIM1, TIM_CR1_CKD_CK_INT_MUL_2,
	//               TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
	//timer_set_oc_mode(TIM1, TIM_OC4, TIM_OCM_PWM2);
	//timer_enable_oc_output(TIM1, TIM_OC4);
	//timer_set_oc_value(TIM1, TIM_OC4, 200);
	timer_set_prescaler(TIM1, 0);
	timer_set_period(TIM1, 1000);//937);
	timer_set_master_mode(TIM1,TIM_CR2_MMS_UPDATE);
	timer_enable_counter(TIM1);

	ADC_CR(ADC1) |= ADC_CR_ADSTART;
	dma_enable_channel(DMA1,1);              //!!!!!!!!!!!!!!!!!111



	//usart
	rcc_periph_clock_enable(RCC_USART1);
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9);
	gpio_set_af(GPIOA, GPIO_AF1, GPIO9);
	usart_set_baudrate(USART1, 500000 );
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_CR2_STOP_1_0BIT);
	usart_set_mode(USART1, USART_MODE_TX);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);

	/* Finally enable the USART. */
	usart_enable(USART1);

}

static void my_usart_print_int(uint32_t usart, int32_t value)
{
	int8_t i;
	int8_t nr_digits = 0;
	char buffer[25];

	if (value < 0) {
		usart_send_blocking(usart, '-');
		value = value * -1;
	}

	if (value == 0) {
		usart_send_blocking(usart, '0');
	}

	while (value > 0) {
		buffer[nr_digits++] = "0123456789"[value % 10];
		value /= 10;
	}

	for (i = nr_digits-1; i >= 0; i--) {
		usart_send_blocking(usart, buffer[i]);
	}

	usart_send_blocking(usart, ',');
	//usart_send_blocking(usart, '\n');
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
