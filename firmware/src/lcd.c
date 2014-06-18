/**
* This file is part of the UKHAS 2014 Badge project
*
* Jon Sowman 2014
* <jon@jonsowman.com>
*/

#include "badge.h"
#include "lcd.h"

#define tiny_wait() do{__asm__("nop");__asm__("nop");__asm__("nop");__asm__("nop");__asm__("nop"); \
                    } while(0)

/* Private prototypes */
static void lcd_write_instruction(const uint8_t cmd);

/**
 * Initialise the LCD following the init routine given in the datasheet. See
 * this for more details.
 */
void lcd_init(void)
{
    // Clock the SPI periph and associated hardware pins
    rcc_periph_clock_enable(RCC_GPIOA);

    // Configure pins for RESET and CS
    gpio_mode_setup(LCD_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, 
            LCD_CS | LCD_RESET | LCD_MOSI | LCD_SCK);

    // SCK idles low
    gpio_clear(LCD_PORT, LCD_SCK);

    // Reset the panel
    gpio_set(GPIOA, GPIO1);
    _delay_ms(20);
    gpio_clear(GPIOA, GPIO1);
    _delay_ms(200);
    gpio_set(GPIOA, GPIO1);
    _delay_ms(200);

    // Run through the init routine given in the datasheet
    lcd_write_instruction(LCD_DISPLAY_OFF);

    // Set oscillator frequency to 0b1000 and divider to 0b0000
    lcd_write_instruction(LCD_CKDIV_OSC_FREQ);
    lcd_write_instruction(0x80);

    // Set multiplex ratio to 1/32 duty (0x1F)
    lcd_write_instruction(LCD_MUX_RATIO);
    lcd_write_instruction(0x1f);

    // Set LCD to no offset
    lcd_write_instruction(LCD_DISPLAY_OFFSET);
    lcd_write_instruction(0x00);

    // Enable the charge pump to generate Vcc internally
    lcd_write_instruction(LCD_SET_CHARGEPUMP);
    lcd_write_instruction(LCD_CHARGEPUMP_ON);

    // Set start line address (0b01xxxxxx where 6 LSBs are the line 0-63)
    lcd_write_instruction(0x40);

    // Use normal mode (rather than inverted display)
    lcd_write_instruction(LCD_SET_NORMAL_MODE);

    // Disable ENTIRE ON (overrides the RAM buffer and turns whole screen on)
    lcd_write_instruction(LCD_ENTIRE_DISPLAY_ON_DISABLE);

    // Remap column 127 to segment 0
    lcd_write_instruction(LCD_SEG_REMAP_COL127_SEG0);

    // Change the output scan direction to scan from COM[N-1] to COM0
    lcd_write_instruction(LCD_COM_SCAN_DIR_REVERSE);

    // Set COM pins hardware configuration to SEQUENTIAL
    lcd_write_instruction(LCD_COM_PINS_CONFIG);
    lcd_write_instruction(0x42); //why this number? should be 0x02

    // Change the contrast
    lcd_write_instruction(LCD_SET_CONTRAST);
    lcd_write_instruction(0xcf); // this number came from the datasheet and appears to look ok

    // Set pre-charge period (upper nibble is 0b1111, lower is 0b0001) - from
    // datasheet
    lcd_write_instruction(LCD_PRECHARGE_PERIOD);
    lcd_write_instruction(0xf1);

    // Sert deselect level. 0x40 comes from datasheet and appears to be an
    // invalid setting! (Valid are 0x00, 0x20 or 0x30)
    lcd_write_instruction(LCD_VCOMH_DESEL_LVL);
    lcd_write_instruction(0x40);

    // Finally, turn the display on
    lcd_write_instruction(LCD_DISPLAY_ON);
}

/**
 * Send a single instruction byte to the LCD panel
 * @param cmd The command to be sent to the LCD panel
 */
static void lcd_write_instruction(const uint8_t cmd)
{
    int8_t i;

    // Assert CS
    gpio_clear(GPIOA, GPIO2);
    tiny_wait();

    // Send 9 bit SPI with MSB as 0 (for command)
    // Send the MSB (9th bit)
    gpio_clear(LCD_PORT, LCD_SCK);
    gpio_clear(LCD_PORT, LCD_MOSI);
    tiny_wait();
    gpio_set(LCD_PORT, LCD_SCK);
    tiny_wait();

    // Now send the remaining 8 bits
    for(i=7; i>=0; i--)
    {
        // Drop sck
        gpio_clear(LCD_PORT, LCD_SCK);
        
        // Change data bit
        if(cmd & (1<<i))
            gpio_set(LCD_PORT, LCD_MOSI);
        else
            gpio_clear(LCD_PORT, LCD_MOSI);

        tiny_wait();
        // Raise SCK
        gpio_set(LCD_PORT, LCD_SCK);
        tiny_wait();
    }

    // SCK idles low
    gpio_clear(LCD_PORT, LCD_SCK);

    // Deassert CS
    tiny_wait();
    gpio_set(GPIOA, GPIO2);
}
