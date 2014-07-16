// rfm69.c
//
// Ported to Arduino 2014 James Coxon
//
// Copyright (C) 2014 Phil Crump
//
// Based on RF22 Copyright (C) 2011 Mike McCauley ported to mbed by Karl Zweimueller
// Based on RFM69 LowPowerLabs (https://github.com/LowPowerLab/RFM69/)

#include "rfm69.h"
#include "rfm69config.h"
#include <string.h>

static uint8_t     rfm69_mode;

/*static uint8_t     rfm69_slaveSelectPin;*/
static float       rfm69_temperatureFudge;

static uint8_t     rfm69_bufLen;
static uint8_t     rfm69_buf[RFM69_MAX_MESSAGE_LEN];

static int16_t     rfm69_lastRssi;

void rfm69_init(float tempFudge)
{
    uint8_t i;
    rfm69_mode = RFM69_MODE_RX;
    rfm69_temperatureFudge = tempFudge;

    // Set up CS
    gpio_mode_setup(RFM69_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, RFM69_CS);
    
    // Set up device
    for (i = 0; CONFIG[i][0] != 255; i++)
        rfm69_spiWrite(CONFIG[i][0], CONFIG[i][1]);
    
    rfm69_setMode(rfm69_mode);

    // Clear TX/RX Buffer
    rfm69_bufLen = 0;
}

uint8_t rfm69_spiRead(uint8_t reg)
{
    // Assert CS
    gpio_clear(RFM69_PORT, RFM69_CS);
    
    spi_xfer(RFM69_SPI, reg & ~RFM69_SPI_WRITE_MASK); // Send the address with the write mask off
    uint8_t val = spi_xfer(RFM69_SPI, 0); // The written value is ignored, reg value is read
    
    // Deassert CS
    gpio_set(RFM69_PORT, RFM69_CS);
    return val;
}

void rfm69_spiWrite(uint8_t reg, uint8_t val)
{
    // Assert CS
    gpio_clear(RFM69_PORT, RFM69_CS);
    
    spi_xfer(RFM69_SPI, reg | RFM69_SPI_WRITE_MASK); // Send the address with the write mask on
    spi_xfer(RFM69_SPI, val); // New value follows

    // Deassert CS
    gpio_set(RFM69_PORT, RFM69_CS);
}

void rfm69_spiBurstRead(uint8_t reg, uint8_t* dest, uint8_t len)
{
    // Assert CS
    gpio_clear(RFM69_PORT, RFM69_CS);
    
    spi_xfer(RFM69_SPI, reg & ~RFM69_SPI_WRITE_MASK); // Send the start address with the write mask off
    while (len--)
        *dest++ = spi_xfer(RFM69_SPI, 0);

    // Deassert CS
    gpio_set(RFM69_PORT, RFM69_CS);
}

void rfm69_spiBurstWrite(uint8_t reg, const uint8_t* src, uint8_t len)
{
    // Assert CS
    gpio_clear(RFM69_PORT, RFM69_CS);
    
    spi_xfer(RFM69_SPI, reg | RFM69_SPI_WRITE_MASK); // Send the start address with the write mask on
    while (len--)
        spi_xfer(RFM69_SPI, *src++);
        
    // Deassert CS
    gpio_set(RFM69_PORT, RFM69_CS);
}

void rfm69_spiFifoWrite(const uint8_t* src, uint8_t len)
{
    // Assert CS
    gpio_clear(RFM69_PORT, RFM69_CS);
    
    // Send the start address with the write mask on
    spi_xfer(RFM69_SPI, RFM69_REG_00_FIFO | RFM69_SPI_WRITE_MASK); // Send the start address with the write mask on
    // First byte is packet length
    spi_xfer(RFM69_SPI, len);
    // Then write the packet
    while (len--)
        spi_xfer(RFM69_SPI, *src++);
    	
    // Deassert CS
    gpio_set(RFM69_PORT, RFM69_CS);
}

void rfm69_setMode(uint8_t newMode)
{
    rfm69_spiWrite(RFM69_REG_01_OPMODE, (rfm69_spiRead(RFM69_REG_01_OPMODE) & 0xE3) | newMode);
    rfm69_mode = newMode;
}

bool rfm69_checkRx(void)
{
    // Check IRQ register for payloadready flag (indicates RXed packet waiting in FIFO)
    if(rfm69_spiRead(RFM69_REG_28_IRQ_FLAGS2) & RF_IRQFLAGS2_PAYLOADREADY) {
        // Get packet length from first byte of FIFO
        rfm69_bufLen = rfm69_spiRead(RFM69_REG_00_FIFO)+1;
        // Read FIFO into our Buffer
        rfm69_spiBurstRead(RFM69_REG_00_FIFO, rfm69_buf, RFM69_FIFO_SIZE);
        // Read RSSI register (should be of the packet? - TEST THIS)
        rfm69_lastRssi = -(rfm69_spiRead(RFM69_REG_24_RSSI_VALUE)/2);
        // Clear the radio FIFO (found in HopeRF demo code)
        rfm69_clearFifo();
        return true;
    }
    return false;
}

void rfm69_recv(uint8_t* buf, uint8_t* len)
{
    // Copy RX Buffer to byref Buffer
    memcpy(buf, rfm69_buf, rfm69_bufLen);
    *len = rfm69_bufLen;
    // Clear RX Buffer
    rfm69_bufLen = 0;
}

void rfm69_send(const uint8_t* data, uint8_t len, uint8_t power)
{
    // power is TX Power in dBmW (valid values are 2dBmW-20dBmW)
    if(power<2 || power >20) {
        // Could be dangerous, so let's check this
        return;
    }
    uint8_t oldMode = rfm69_mode;
    // Copy data into TX Buffer
    memcpy(rfm69_buf, data, len);
    // Update TX Buffer Size
    rfm69_bufLen = len;
    // Start Transmitter
    rfm69_setMode(RFM69_MODE_TX);
    // Set up PA
    if(power<=17) {
        // Set PA Level
        uint8_t paLevel = power + 14;
	rfm69_spiWrite(RFM69_REG_11_PA_LEVEL, RF_PALEVEL_PA0_OFF | RF_PALEVEL_PA1_ON | RF_PALEVEL_PA2_ON | paLevel);        
    } else {
        // Disable Over Current Protection
        rfm69_spiWrite(RFM69_REG_13_OCP, RF_OCP_OFF);
        // Enable High Power Registers
        rfm69_spiWrite(RFM69_REG_5A_TEST_PA1, 0x5D);
        rfm69_spiWrite(RFM69_REG_5C_TEST_PA2, 0x7C);
        // Set PA Level
        uint8_t paLevel = power + 11;
	    rfm69_spiWrite(RFM69_REG_11_PA_LEVEL, RF_PALEVEL_PA0_OFF | RF_PALEVEL_PA1_ON | RF_PALEVEL_PA2_ON | paLevel);
    }
    // Wait for PA ramp-up
    while(!(rfm69_spiRead(RFM69_REG_27_IRQ_FLAGS1) & RF_IRQFLAGS1_TXREADY)) { };
    // Throw Buffer into FIFO, packet transmission will start automatically
    rfm69_spiFifoWrite(rfm69_buf, rfm69_bufLen);
    // Clear MCU TX Buffer
    rfm69_bufLen = 0;
    // Wait for packet to be sent
    while(!(rfm69_spiRead(RFM69_REG_28_IRQ_FLAGS2) & RF_IRQFLAGS2_PACKETSENT)) { };
    // Return Transceiver to original mode
    rfm69_setMode(oldMode);
    // If we were in high power, switch off High Power Registers
    if(power>17) {
        // Disable High Power Registers
        rfm69_spiWrite(RFM69_REG_5A_TEST_PA1, 0x55);
        rfm69_spiWrite(RFM69_REG_5C_TEST_PA2, 0x70);
        // Enable Over Current Protection
        rfm69_spiWrite(RFM69_REG_13_OCP, RF_OCP_ON | RF_OCP_TRIM_95);
    }
}

void rfm69_SetLnaMode(uint8_t lnaMode)
{
    // RF_TESTLNA_NORMAL (default)
    // RF_TESTLNA_SENSITIVE
    rfm69_spiWrite(RFM69_REG_58_TEST_LNA, lnaMode);
}

void rfm69_clearFifo(void)
{
    // Must only be called in RX Mode
    // Apparently this works... found in HopeRF demo code
    rfm69_setMode(RFM69_MODE_STDBY);
    rfm69_setMode(RFM69_MODE_RX);
}

float rfm69_readTemp(void)
{
    // Store current transceiver mode
    uint8_t oldMode = rfm69_mode;
    // Set mode into Standby (required for temperature measurement)
    rfm69_setMode(RFM69_MODE_STDBY);
	
    // Trigger Temperature Measurement
    rfm69_spiWrite(RFM69_REG_4E_TEMP1, RF_TEMP1_MEAS_START);
    // Check Temperature Measurement has started
    if(!(RF_TEMP1_MEAS_RUNNING && rfm69_spiRead(RFM69_REG_4E_TEMP1))){
        return 255.0;
    }
    // Wait for Measurement to complete
    while(RF_TEMP1_MEAS_RUNNING && rfm69_spiRead(RFM69_REG_4E_TEMP1)) { };
    // Read raw ADC value
    uint8_t rawTemp = rfm69_spiRead(RFM69_REG_4F_TEMP2);
	
    // Set transceiver back to original mode
    rfm69_setMode(oldMode);
    // Return processed temperature value
    return (168.3+rfm69_temperatureFudge)-(float)rawTemp;
}

int16_t rfm69_sampleRssi(void)
{
    // Must only be called in RX mode
    if(rfm69_mode!=RFM69_MODE_RX) {
        // Not sure what happens otherwise, so check this
        return 0;
    }
    // Trigger RSSI Measurement
    rfm69_spiWrite(RFM69_REG_23_RSSI_CONFIG, RF_RSSI_START);
    // Wait for Measurement to complete
    while(!(RF_RSSI_DONE && rfm69_spiRead(RFM69_REG_23_RSSI_CONFIG))) { };
    // Read, store in _lastRssi and return RSSI Value
    rfm69_lastRssi = -(rfm69_spiRead(RFM69_REG_24_RSSI_VALUE)/2);
    return rfm69_lastRssi;
}
