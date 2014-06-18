/**
* This file is part of the UKHAS 2014 Badge project
*
* Jon Sowman 2014
* <jon@jonsowman.com>
*/

#include "badge.h"
#include "lcd.h"

/* This is a temporary bodge covering a missing define in locm3 */
#define LCD_SPI SPI1_I2S1_BASE

/* Private prototypes */
static void lcd_write_instruction(const uint8_t cmd);

/**
 * Initialise the LCD following the init routine given in the datasheet. See
 * this for more details.
 */
void lcd_init(void)
{
    // Clock the SPI periph and associated hardware pins
    rcc_periph_clock_enable(RCC_SPI1);
    rcc_periph_clock_enable(RCC_GPIOA);

    // Configure pins for RESET and CS
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO1 | GPIO2);

    // Enable AF0 (LCD_SPI periph) on these pins
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE,
            GPIO5 | GPIO6 | GPIO7);
    gpio_set_af(GPIOA, GPIO_AF0, GPIO5 | GPIO6 | GPIO7);

    // Reset and enable the SPI periph
    spi_reset(LCD_SPI);
    spi_init_master(LCD_SPI, SPI_CR1_BAUDRATE_FPCLK_DIV_64,
            SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
            SPI_CR1_CPHA_CLK_TRANSITION_1,
            SPI_CR1_CRCL_8BIT,
            SPI_CR1_MSBFIRST);

    // Trigger an RXNE event when we have 8 bits (one byte) in the buffer
    spi_fifo_reception_threshold_8bit(LCD_SPI);

    // NSS must be set high for the peripheral to function
    spi_enable_software_slave_management(LCD_SPI);
    spi_set_nss_high(LCD_SPI);
    gpio_set(GPIOA, GPIO2);

    // Enable
    spi_enable(LCD_SPI);

    // Reset the panel
    gpio_set(GPIOA, GPIO1);
    _delay_ms(20);
    gpio_clear(GPIOA, GPIO1);
    _delay_ms(200);
    gpio_set(GPIOA, GPIO1);
    _delay_ms(200);

    // Run through the init routine given in the datasheet
    lcd_write_instruction(0xae);//--turn off oled panel

    lcd_write_instruction(0xd5);//--set display clock divide ratio/oscillator frequency
    lcd_write_instruction(0x80);//--set divide ratio

    lcd_write_instruction(0xa8);//--set multiplex ratio(1 to 64)
    lcd_write_instruction(0x1f);//--1/32 duty

    lcd_write_instruction(0xd3);//-set display offset
    lcd_write_instruction(0x00);//-not offset


    lcd_write_instruction(0x8d);//--set Charge Pump enable/disable
    lcd_write_instruction(0x14);//--set(0x10) disable

    lcd_write_instruction(0x40);//--set start line address

    lcd_write_instruction(0xa6);//--set normal display

    lcd_write_instruction(0xa4);//Disable Entire Display On

    lcd_write_instruction(0xa1);//--set segment re-map 128 to 0

    lcd_write_instruction(0xC8);//--Set COM Output Scan Direction 64 to 0

    lcd_write_instruction(0xda);//--set com pins hardware configuration
    lcd_write_instruction(0x42);

    lcd_write_instruction(0x81);//--set contrast control register
    lcd_write_instruction(0xcf);


    lcd_write_instruction(0xd9);//--set pre-charge period
    lcd_write_instruction(0xf1);

    lcd_write_instruction(0xdb);//--set vcomh
    lcd_write_instruction(0x40);

    lcd_write_instruction(0xaf);//--turn on oled panel
}

/**
 * Send a single instruction byte to the LCD panel
 * @param cmd The command to be send to the LCD panel
 */
static void lcd_write_instruction(const uint8_t cmd)
{
    // Assert CS
    gpio_clear(GPIOA, GPIO2);

    // Send the byte
    spi_send8(LCD_SPI, cmd);
    spi_read8(LCD_SPI);

    // Wait until send FIFO is empty
    while(SPI_SR(LCD_SPI) & SPI_SR_BSY);

    // Deassert CS
    gpio_set(GPIOA, GPIO2);
}
