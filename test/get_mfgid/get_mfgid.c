/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2013 Stephen Dwyer <scdwyer@ualberta.ca>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/spi.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

void initialise_monitor_handles(void);
void CYRF_WriteRegister(u8 address, u8 data);
void Delay(u32 x);
void CYRF_Reset(void);
void CYRF_GetMfgData(u8 data[]);

#define CS_HI() gpio_set(GPIOA, GPIO4)
#define CS_LO() gpio_clear(GPIOA, GPIO4)
#define RS_HI() gpio_set(GPIOB, GPIO0)
#define RS_LO() gpio_clear(GPIOB, GPIO0)

void CYRF_WriteRegister(u8 address, u8 data)
{
    CS_LO();
    spi_xfer(SPI1, 0x80 | address);
    spi_xfer(SPI1, data);
    CS_HI();
}

static void ReadRegisterMulti(u8 address, u8 data[], u8 length)
{
    unsigned char i;

    CS_LO();
    spi_xfer(SPI1, address);
    for(i = 0; i < length; i++)
    {
        data[i] = spi_xfer(SPI1, 0);
    }
    CS_HI();
}

void Delay(u32 x)
{
    (void)x;
    __asm ("mov r1, #24;"
         "mul r0, r0, r1;"
         "b _delaycmp;"
         "_delayloop:"
         "subs r0, r0, #1;"
         "_delaycmp:;"
         "cmp r0, #0;"
         " bne _delayloop;");
}

void CYRF_Reset(void)
{
    /* Reset the CYRF chip */
    RS_HI();
    Delay(100);
    RS_LO();
    Delay(100);
}

void CYRF_GetMfgData(u8 data[])
{
    /* Fuses power on */
    CYRF_WriteRegister(0x25, 0xFF);

    ReadRegisterMulti(0x25, data, 6);

    /* Fuses power off */
    CYRF_WriteRegister(0x25, 0x00);
}

static void clock_setup(void)
{
	rcc_clock_setup_in_hse_12mhz_out_72mhz();

	/* Enable GPIOA, GPIOB, GPIOC clock. */
	rcc_peripheral_enable_clock(&RCC_APB2ENR,
				    RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN |
				    RCC_APB2ENR_IOPCEN);

	/* Enable clocks for GPIO port A (for GPIO_USART2_TX) and USART2. */
	rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN |
				    RCC_APB2ENR_AFIOEN);

	/* Enable SPI1 Periph and gpio clocks */
	rcc_peripheral_enable_clock(&RCC_APB2ENR,
				    RCC_APB2ENR_SPI1EN);

}

static void spi_setup(void) {

  /* Radio RST */
  gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
                GPIO_CNF_OUTPUT_PUSHPULL, GPIO0);
  /* Configure GPIOs: SS=PA4, SCK=PA5, MISO=PA6 and MOSI=PA7 */
  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
                GPIO_CNF_OUTPUT_PUSHPULL, GPIO4);

  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
                GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO5 |
                                                GPIO7 );
  
  gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,
                GPIO6);

  /* Reset SPI, SPI_CR1 register cleared, SPI is disabled */
  spi_reset(SPI1);

  /* Set up SPI in Master mode with:
   * Clock baud rate: 1/64 of peripheral clock frequency
   * Clock polarity: Idle High
   * Clock phase: Data valid on 2nd clock pulse
   * Data frame format: 8-bit
   * Frame format: MSB First
   */
  spi_init_master(SPI1,
		  SPI_CR1_BAUDRATE_FPCLK_DIV_64,
		  SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
                  SPI_CR1_CPHA_CLK_TRANSITION_1,
		  SPI_CR1_DFF_8BIT,
		  SPI_CR1_MSBFIRST);

  /*
   * Set NSS management to software.
   *
   * Note:
   * Setting nss high is very important, even if we are controlling the GPIO
   * ourselves this bit needs to be at least set to 1, otherwise the spi
   * peripheral will not send any data out.
   */
  spi_enable_software_slave_management(SPI1);
  spi_set_nss_high(SPI1);

  /* Enable SPI1 periph. */
  spi_enable(SPI1);
}

static void gpio_setup(void)
{
	/* Set GPIO10 (in GPIO port B) to 'output push-pull'. */
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL, GPIO10);
}

int main(void)
{
	int counter = 0;
	//u16 rx_value = 0x42;
	u8 cyrfmfg_id[7];

	clock_setup();
	gpio_setup();
	spi_setup();

	CYRF_Reset();
#if SEMIHOSTING
	    initialise_monitor_handles();
#endif

	cyrfmfg_id[0] = 0;
	cyrfmfg_id[1] = 0;
	cyrfmfg_id[2] = 0;
	cyrfmfg_id[3] = 0;
	cyrfmfg_id[4] = 0;
	cyrfmfg_id[5] = 0;

	/* Blink the LED (PA8) on the board with every transmitted byte. */
	while (1) {
		/* LED on/off */
		gpio_toggle(GPIOB, GPIO10);

		/* printf the value that SPI should send */
		printf("Counter: %i  SPI receiving mfgid: ... ", counter);
		/* blocking send of the byte out SPI1 */
		CYRF_GetMfgData(cyrfmfg_id);
		cyrfmfg_id[6]=0;
		/* printf the byte just received */
		printf("    SPI Received Byte: '0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X'\r\n", cyrfmfg_id[0], cyrfmfg_id[1], cyrfmfg_id[2], cyrfmfg_id[3], cyrfmfg_id[4], cyrfmfg_id[5]);

		counter++;
		
	}

	return 0;
}
