/**
* This file is part of the UKHAS 2014 Badge project
*
* Jon Sowman 2014
* <jon@jonsowman.com>
*/

#ifndef __UKHAS_LCD_H__
#define __UKHAS_LCD_H__

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>

#define LCD_PORT        GPIOA
#define LCD_CS          GPIO2
#define LCD_RESET       GPIO1
#define LCD_MOSI        GPIO7
#define LCD_SCK         GPIO5

void lcd_init(void);

#endif /* __UKHAS_LCD_H__ */
