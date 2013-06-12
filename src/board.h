/*
 * This file is part of the superbitrf project.
 *
 * Copyright (C) 2013 Freek van Tienen <freek.v.tienen@gmail.com>
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

#ifndef BOARD_H_
#define BOARD_H_

/* Define the LEDS (optional) */
#define USE_LED_POWER				1								/**< If the board has a Power led */
#define LED_POWER_GPIO_PORT			GPIOB							/**< The Power led GPIO port */
#define LED_POWER_GPIO_PIN			GPIO2							/**< The Power led GPIO pin */
#define LED_POWER_GPIO_CLK			RCC_APB2ENR_IOPBEN				/**< The Power led GPIO clock */

#define USE_LED_1					1
#define LED_1_GPIO_PORT				GPIOB
#define LED_1_GPIO_PIN				GPIO10
#define LED_1_GPIO_CLK				RCC_APB2ENR_IOPBEN

#define USE_LED_2					1
#define LED_2_GPIO_PORT				GPIOB
#define LED_2_GPIO_PIN				GPIO11
#define LED_2_GPIO_CLK				RCC_APB2ENR_IOPBEN

/* Define the BIND button (optional) */
#define USE_BTN_BIND				1								/**< If the board has a bind button */
#define BTN_BIND_GPIO_PORT			GPIOB							/**< The Bind button GPIO port */
#define BTN_BIND_GPIO_PIN			GPIO12							/**< The Bind button GPIO pin */
#define BTN_BIND_GPIO_CLK			RCC_APB2ENR_IOPBEN				/**< The Bind button GPIO clock */
#define BTN_BIND_EXTI				EXTI12							/**< The Bind button EXTI for the interrupt */
#define BTN_BIND_ISR				exti15_10_isr					/**< The Bind button ISR function for the interrupt */
#define BTN_BIND_NVIC				NVIC_EXTI15_10_IRQ				/**< The Bind button NVIC for the interrupt */

/* Define the USB connection */
#define USB_DETACH_PORT				GPIOA							/**< The USB Detach GPIO base address */
#define USB_DETACH_PIN				GPIO8							/**< The USB Detach GPIO pin */
#define USB_DETACH_CLK				RCC_APB2ENR_IOPAEN				/**< The USB Detach GPIO clock */

/* Define the CYRF6936 chip */
#define CYRF_DEV_SPI				SPI1							/**< The SPI connection number */
#define CYRF_DEV_SPI_CLK			RCC_APB2ENR_SPI1EN				/**< The SPI clock */
#define CYRF_DEV_SS_PORT			GPIOA							/**< The SPI SS port */
#define CYRF_DEV_SS_PIN				GPIO4							/**< The SPI SS pin */
#define CYRF_DEV_SCK_PORT			GPIOA							/**< The SPI SCK port */
#define CYRF_DEV_SCK_PIN			GPIO5							/**< The SPI SCK pin */
#define CYRF_DEV_MISO_PORT			GPIOA							/**< The SPI MISO port */
#define CYRF_DEV_MISO_PIN			GPIO6							/**< The SPI MISO pin */
#define CYRF_DEV_MOSI_PORT			GPIOA							/**< The SPI MOSI port */
#define CYRF_DEV_MOSI_PIN			GPIO7							/**< The SPI MOSI pin */
#define CYRF_DEV_RST_PORT			GPIOB							/**< The RST GPIO port*/
#define CYRF_DEV_RST_PIN			GPIO0							/**< The RST GPIO pin */
#define CYRF_DEV_RST_CLK			RCC_APB2ENR_IOPBEN				/**< The RST GPIO clock */
#define CYRF_DEV_IRQ_PORT			GPIOA							/**< The IRQ GPIO port*/
#define CYRF_DEV_IRQ_PIN			GPIO3							/**< The IRQ GPIO pin */
#define CYRF_DEV_IRQ_CLK			RCC_APB2ENR_IOPAEN				/**< The IRQ GPIO clock */
#define CYRF_DEV_IRQ_EXTI			EXTI3							/**< The IRQ EXTI for the interrupt */
#define CYRF_DEV_IRQ_ISR			exti3_isr						/**< The IRQ ISR function for the interrupt */
#define CYRF_DEV_IRQ_NVIC			NVIC_EXTI3_IRQ					/**< The IRQ NVIC for the interrupt */

/* Define the DSM timer */
#define TIMER_DSM					TIM2							/**< The DSM timer */
#define TIMER_DSM_NVIC				NVIC_TIM2_IRQ					/**< The DSM timer NVIC */
#define TIMER_DSM_IRQ				tim2_isr						/**< The DSM timer function interrupt */


#endif /* BOARD_H_ */
