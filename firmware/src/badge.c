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
#include <libopencm3/stm32/common/timer_common_all.h>

#include "badge.h"
#include "lcd.h"

void init(void);

int main(void)
{
    init();
    lcd_init();
    lcd_clear();
    lcd_set_display_ptr(2,0,3,127);
    lcd_write_string("SUSF / APEX / ASTRA");
    lcd_write_string_medium("Matt Brejza", 0, 0);
    /* Blink the LED (PC8) on the board. */
    while (1){
        _delay_ms(100);

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
    gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO1);
              
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
