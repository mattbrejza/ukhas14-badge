/**
* This file is part of the UKHAS 2014 Badge project
*
* Jon Sowman 2014
* <jon@jonsowman.com>
*/

#include "badge.h"
#include "lcd.h"
#include "font.h"

/* This is a temporary bodge covering a missing define in locm3 */
#define LCD_SPI SPI1_I2S1_BASE

/* Private prototypes */
static void lcd_write_instruction(const uint8_t cmd);
static uint8_t stretch_up(uint8_t in);
static uint8_t stretch_down(uint8_t in);

/**
 * Initialise the LCD following the init routine given in the datasheet. See
 * this for more details.
 */
void lcd_init(void)
{
    // Clock the SPI periph and associated hardware pins
    rcc_periph_clock_enable(RCC_SPI1);
    rcc_periph_clock_enable(RCC_GPIOA);

    // Configure pins for DC and CS
    gpio_mode_setup(LCD_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LCD_CS | LCD_DC);

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

    // Lower DC to enter instruction mode
    gpio_clear(LCD_PORT, LCD_DC);
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
    
    //put into line-by-line address increment mode
    lcd_write_instruction(0x20);
    lcd_write_instruction(0x00);

    // Finally, turn the display on
    lcd_write_instruction(LCD_DISPLAY_ON);
}

void lcd_set_display_ptr(uint8_t start_row, uint8_t start_col, uint8_t end_row, uint8_t end_col)
{
    lcd_write_instruction(LCD_COL_ADDRESS);
	lcd_write_instruction(start_col);
	lcd_write_instruction(end_col);
	lcd_write_instruction(LCD_PAGE_ADDRESS);
	lcd_write_instruction(start_row);
	lcd_write_instruction(end_row);

}

void lcd_clear(void)
{
    lcd_set_display_ptr(0,0,LCD_HEIGHT/8,LCD_WIDTH-1);
    gpio_set(LCD_PORT, LCD_DC);
	gpio_clear(GPIOA, GPIO2);
    uint16_t i = 0;
	for (i = 0; i < (LCD_HEIGHT/8)*LCD_WIDTH; i++) {
        
        spi_send8(LCD_SPI, 0x00);        
		spi_read8(LCD_SPI);
		// Wait until send FIFO is empty
		while(SPI_SR(LCD_SPI) & SPI_SR_BSY);
    }

	gpio_clear(LCD_PORT, LCD_DC);
	gpio_set(GPIOA, GPIO2);
}

void lcd_write_string(char *string)
{
    gpio_set(LCD_PORT, LCD_DC);
	gpio_clear(GPIOA, GPIO2);
    uint8_t i,j;
    i = 0;
    j = 0;
	while (string[i]) {
        
        if (j >= 5)
        {
            j = 0;
            i++;
            spi_send8(LCD_SPI, 0x00);
        }
        else{
            spi_send8(LCD_SPI, font5x7[(string[i]-32)*5+j]);
            j++;
        }
		spi_read8(LCD_SPI);

		// Wait until send FIFO is empty
		while(SPI_SR(LCD_SPI) & SPI_SR_BSY);
    }

	gpio_clear(LCD_PORT, LCD_DC);
	gpio_set(GPIOA, GPIO2);
}

void lcd_write_string_medium(char *string, uint8_t start_row, uint8_t start_col)
{

    lcd_set_display_ptr(start_row,start_col,start_row,LCD_WIDTH-1);

    gpio_set(LCD_PORT, LCD_DC);
	gpio_clear(GPIOA, GPIO2);
    uint8_t i,j;
    i = 0;
    j = 0;
	while (string[i]) {        
        if (j >= 10)
        {
            j = 0;
            i++;
            spi_send8(LCD_SPI, 0x00);
        }
        else{
            spi_send8(LCD_SPI, stretch_up(font5x7[(string[i]-32)*5+(j>>1)]));
            j++;
        }
		spi_read8(LCD_SPI);

		// Wait until send FIFO is empty
		while(SPI_SR(LCD_SPI) & SPI_SR_BSY);
    }

	gpio_clear(LCD_PORT, LCD_DC);
	gpio_set(GPIOA, GPIO2);
    
    lcd_set_display_ptr(start_row+1,start_col,start_row+1,LCD_WIDTH-1);

    gpio_set(LCD_PORT, LCD_DC);
	gpio_clear(GPIOA, GPIO2);
    i = 0;
    j = 0;
	while (string[i]) {        
        if (j >= 10)
        {
            j = 0;
            i++;
            spi_send8(LCD_SPI, 0x00);
        }
        else{
            spi_send8(LCD_SPI, stretch_down(font5x7[(string[i]-32)*5+(j>>1)]));
            j++;
        }
		spi_read8(LCD_SPI);

		// Wait until send FIFO is empty
		while(SPI_SR(LCD_SPI) & SPI_SR_BSY);
    }

	gpio_clear(LCD_PORT, LCD_DC);
	gpio_set(GPIOA, GPIO2);
}

void lcd_test(void)
{
lcd_write_instruction(0x20);
lcd_write_instruction(0x00);
	lcd_write_instruction(LCD_COL_ADDRESS);
	lcd_write_instruction(0x00);
	lcd_write_instruction(LCD_WIDTH-1);
	lcd_write_instruction(LCD_PAGE_ADDRESS);
	lcd_write_instruction(0x00);
	lcd_write_instruction(0x03);
	
	uint16_t i,j,k;
    j = 0;
    k = 0;
	gpio_set(LCD_PORT, LCD_DC);
	gpio_clear(GPIOA, GPIO2);
	for (i=0; i<(LCD_WIDTH*LCD_HEIGHT/8); i++) {
        j++;
        if (j > 5)
        {
            j = 0;
            spi_send8(LCD_SPI, 0x00);
        }
        else{
            spi_send8(LCD_SPI, font5x7[k]);
            k++;
            }
		spi_read8(LCD_SPI);

		// Wait until send FIFO is empty
		while(SPI_SR(LCD_SPI) & SPI_SR_BSY);
    }

	gpio_clear(LCD_PORT, LCD_DC);
	gpio_set(GPIOA, GPIO2);
}

static uint8_t stretch_up(uint8_t in)
{
//0b 0000abcd into 0b aabbccdd

    return ((in << 0) & 0x01) | ((in << 1) & 0x02) |
           ((in << 1) & 0x04) | ((in << 2) & 0x08) |
           ((in << 2) & 0x10) | ((in << 3) & 0x20) |
           ((in << 3) & 0x40) | ((in << 4) & 0x80);

}
static uint8_t stretch_down(uint8_t in)
{
//0b abcd0000 into 0b aabbccdd
    return ((in >> 4) & 0x01) | ((in >> 3) & 0x02) |
           ((in >> 3) & 0x04) | ((in >> 2) & 0x08) |
           ((in >> 2) & 0x10) | ((in >> 1) & 0x20) |
           ((in >> 1) & 0x40) | ((in >> 0) & 0x80);

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
