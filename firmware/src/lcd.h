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
#define LCD_DC          GPIO1
#define LCD_MOSI        GPIO7
#define LCD_SCK         GPIO5

/* Fundamental commands */
#define LCD_SET_CONTRAST                    0x81
#define LCD_ENTIRE_DISPLAY_ON_ENABLE        0xA5
#define LCD_ENTIRE_DISPLAY_ON_DISABLE       0xA4
#define LCD_SET_NORMAL_MODE                 0xA6
#define LCD_SET_INVERSE_MODE                0xA7
#define LCD_DISPLAY_OFF                     0xAE
#define LCD_DISPLAY_ON                      0xAF

/* TODO: Scrolling command table */

/* Addressing setting commands */
#define LCD_MEM_ADDRESSING_MODE             0x20
#define LCD_COL_ADDRESS                     0x21
#define LCD_PAGE_ADDRESS                    0x22

/* Hardware configuration commands */
#define LCD_SEG_REMAP_COL0_SEG0             0xA0
#define LCD_SEG_REMAP_COL127_SEG0           0xA1
#define LCD_MUX_RATIO                       0xA8
#define LCD_COM_SCAN_DIR_NORMAL             0xC0
#define LCD_COM_SCAN_DIR_REVERSE            0xC8
#define LCD_DISPLAY_OFFSET                  0xD3
#define LCD_COM_PINS_CONFIG                 0xDA

/* Timing and driving scheme setting commands */
#define LCD_CKDIV_OSC_FREQ                  0xD5
#define LCD_PRECHARGE_PERIOD                0xD9
#define LCD_VCOMH_DESEL_LVL                 0xDB
#define LCD_NOP                             0xE3

/* TODO: Advanced graphic command table */

/* Charge pump commands */
#define LCD_SET_CHARGEPUMP                  0x8D
#define LCD_CHARGEPUMP_OFF                  0x10
#define LCD_CHARGEPUMP_ON                   0x14

void lcd_init(void);

#endif /* __UKHAS_LCD_H__ */
