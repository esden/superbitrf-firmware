/*
 * This file is part of the superbitrf project.
 *
 * Copyright (C) 2013 Freek van Tienen <freek.v.tienen@gmail.com>
 * Copyright (C) 2014 Piotr Esden-Tempski <piotr@esden.net>
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

#include <stdlib.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/usb/dfu.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/usb.h>

#include "cdcacm.h"
#include "ring.h"
#include "../helper/usb_struct_templates.h"


// The usbd device
usbd_device *cdcacm_usbd_dev = NULL;

// The usbd console buffer
uint8_t cdacm_usbd_console_buffer[256];

static int configured;
static int cdcacm_data_dtr = 1;
static int cdcacm_console_dtr = 1;

/* CDCACM status struct. It stores error counts on the interfaces. */
struct cdcacm_status {
	uint32_t data_rx_ring_full;
	uint32_t console_rx_ring_full;
} cdcacm_status;

/* Input and output ring buffers for the two virtual serial ports. */
#define CDCACM_IO_BUFFER_SIZE 256
uint8_t cdcacm_data_tx_buffer[CDCACM_IO_BUFFER_SIZE];
uint8_t cdcacm_data_rx_buffer[CDCACM_IO_BUFFER_SIZE];
struct ring cdcacm_data_tx;
struct ring cdcacm_data_rx;
uint8_t cdcacm_console_tx_buffer[CDCACM_IO_BUFFER_SIZE];
uint8_t cdcacm_console_rx_buffer[CDCACM_IO_BUFFER_SIZE];
struct ring cdcacm_console_tx;
struct ring cdcacm_console_rx;

/**
 * Misc device descriptor with most of the settings preset. Only adjustment we
 * do is to provide the variable name and the vendor and product id, as this is
 * the most common change between implementations...
 */
USB_DEVICE_DESCRIPTOR_MISC(dev, 0x0484, 0x5741);

/** Data CDCACM **/
USB_CDCACM_COMMAND_EP_DESCRIPTOR(data_comm_endp, 0x82);

USB_CDCACM_DATA_EP_DESCRIPTOR(data_data_endp, 0x01, 0x81);

USB_CDCACM_FUNCTIONAL_DESCRIPTORS(data_cdcacm_functional_descriptors, 1, 0, 1);

USB_CDCACM_COMMAND_INTERFACE(data_comm_iface, 0, 4, data_comm_endp, data_cdcacm_functional_descriptors);

USB_CDCACM_DATA_INTERFACE(data_data_iface, 1, data_data_endp);

USB_CDCACM_ASSOCIATION_DESCRIPTOR(data_assoc, 0);

/** console CDCACM **/
USB_CDCACM_COMMAND_EP_DESCRIPTOR(console_comm_endp, 0x84);

USB_CDCACM_DATA_EP_DESCRIPTOR(console_data_endp, 0x03, 0x83);

/* This is probably redundant with the other previously defined
 * data_cdcacm_functional_descriptors.
 */
USB_CDCACM_FUNCTIONAL_DESCRIPTORS(console_cdcacm_functional_descriptors, 3, 2, 3);

USB_CDCACM_COMMAND_INTERFACE(console_comm_iface, 2, 5, console_comm_endp, console_cdcacm_functional_descriptors);

USB_CDCACM_DATA_INTERFACE(console_data_iface, 3, console_data_endp);

USB_CDCACM_ASSOCIATION_DESCRIPTOR(console_assoc, 2);

/** DFU CDCACM **/

static const struct usb_dfu_descriptor dfu_function = {
	.bLength = sizeof(struct usb_dfu_descriptor),
	.bDescriptorType = DFU_FUNCTIONAL,
	.bmAttributes = USB_DFU_CAN_DOWNLOAD | USB_DFU_WILL_DETACH,
	.wDetachTimeout = 255,
	.wTransferSize = 1024,
	.bcdDFUVersion = 0x011A,
};

static const struct usb_interface_descriptor dfu_iface = {
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 4,
	.bAlternateSetting = 0,
	.bNumEndpoints = 0,
	.bInterfaceClass = 0xFE,
	.bInterfaceSubClass = 1,
	.bInterfaceProtocol = 1,
	.iInterface = 6,

	.extra = &dfu_function,
	.extralen = sizeof(dfu_function),
};

static const struct usb_iface_assoc_descriptor dfu_assoc = {
	.bLength = USB_DT_INTERFACE_ASSOCIATION_SIZE,
	.bDescriptorType = USB_DT_INTERFACE_ASSOCIATION,
	.bFirstInterface = 4,
	.bInterfaceCount = 1,
	.bFunctionClass = 0xFE,
	.bFunctionSubClass = 1,
	.bFunctionProtocol = 1,
	.iFunction = 6,
};

/** Main CDCACM **/

static const struct usb_interface ifaces[] = {{
	.num_altsetting = 1,
	.iface_assoc = &data_assoc,
	.altsetting = data_comm_iface,
}, {
	.num_altsetting = 1,
	.altsetting = data_data_iface,
}, {
	.num_altsetting = 1,
	.iface_assoc = &console_assoc,
	.altsetting = console_comm_iface,
}, {
	.num_altsetting = 1,
	.altsetting = console_data_iface,
}, {
	.num_altsetting = 1,
	.iface_assoc = &dfu_assoc,
	.altsetting = &dfu_iface,
}};

static const struct usb_config_descriptor config = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 5,
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0x80,
	.bMaxPower = 0x32,

	.interface = ifaces,
};

// The usb strings
static const char *usb_strings[] = {
	"1 BIT SQUARED",
	"Superbit USBRF",
	(const char *)0x8001FF0, /* The serial number is stored in the bootloader. */
	"SuperbitRF data port",
	"SuperbitRF console interface",
	"SuperbitRF DFU",
};

/**
 * When there is a successfull DFU detach
 */
static void dfu_detach_complete(usbd_device *usbd_dev, struct usb_setup_data *req) {
	(void)usbd_dev;
	(void)req;

	/* Disconnect USB cable */
	gpio_set_mode(USB_DETACH_PORT, GPIO_MODE_INPUT, 0, USB_DETACH_PIN);

	/* Assert boot-request pin */
  //assert_boot_pin();???

	/* Reset core to enter bootloader */
	scb_reset_core();
}


/**
 * CDCACM console request received
 */
static int cdcacm_console_request(usbd_device *usbd_dev,
		struct usb_setup_data *req, uint8_t **buf, uint16_t *len,
		void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req)) {
	(void)usbd_dev;

	switch(req->bRequest) {
	case USB_CDC_REQ_SET_CONTROL_LINE_STATE:
		/* Ignore if not for GDB interface */
		if(req->wIndex == 0)
			cdcacm_data_dtr = req->wValue & 1;
		else if(req->wIndex == 2)
			cdcacm_console_dtr = req->wValue & 1;

		return 1;
	case USB_CDC_REQ_SET_LINE_CODING:
		if(*len < sizeof(struct usb_cdc_line_coding))
			return 0;

		return 1;
	case DFU_GETSTATUS:
		if(req->wIndex == 4) {
			(*buf)[0] = DFU_STATUS_OK;
			(*buf)[1] = 0;
			(*buf)[2] = 0;
			(*buf)[3] = 0;
			(*buf)[4] = STATE_APP_IDLE;
			(*buf)[5] = 0;	/* iString not used here */
			*len = 6;

			return 1;
		}
	case DFU_DETACH:
		if(req->wIndex == 4) {
			*complete = dfu_detach_complete;
			return 1;
		}
		return 0;
	}
	return 0;
}

/**
 * CDCACM data recieve callback
 */
static void cdcacm_data_rx_cb(usbd_device *usbd_dev, uint8_t ep) {
	usbd_ep_nak_set(usbd_dev, ep, 1);

	char buf[65];
	int len = usbd_ep_read_packet(usbd_dev, ep, buf, 64);

	if (len) {
		int rx_len = ring_write(&cdcacm_data_rx, (uint8_t *)buf, len);

		/* Record rx buffer overflow event. */
		if (rx_len <= 0) {
			cdcacm_status.data_rx_ring_full++;
		}
	}

	usbd_ep_nak_set(usbd_dev, ep, 0);
}

/**
 * CDCACM console recieve callback
 */

static void cdcacm_console_rx_cb(usbd_device *usbd_dev, uint8_t ep) {
	usbd_ep_nak_set(usbd_dev, ep, 1);

	char buf[65];
	int len = usbd_ep_read_packet(usbd_dev, ep, buf, 64);

	if (len) {
		int rx_len = ring_write(&cdcacm_console_rx, (uint8_t *)buf, len);

		/* Record rx buffer overflow event. */
		if (rx_len <= 0) {
			cdcacm_status.console_rx_ring_full++;
		}
	}

	usbd_ep_nak_set(usbd_dev, ep, 0);
}

/**
 * CDCACM set config
 */
static void cdcacm_set_config_callback(usbd_device *usbd_dev, uint16_t wValue) {
	configured = wValue;

	/* Data interface */
	usbd_ep_setup(usbd_dev, 0x01, USB_ENDPOINT_ATTR_BULK,
					64, cdcacm_data_rx_cb);
	usbd_ep_setup(usbd_dev, 0x81, USB_ENDPOINT_ATTR_BULK,
					64, NULL);
	usbd_ep_setup(usbd_dev, 0x82, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

	/* Control interface */
	usbd_ep_setup(usbd_dev, 0x03, USB_ENDPOINT_ATTR_BULK,
					64, cdcacm_console_rx_cb);
	usbd_ep_setup(usbd_dev, 0x83, USB_ENDPOINT_ATTR_BULK,
					64, NULL);
	usbd_ep_setup(usbd_dev, 0x84, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

	usbd_register_control_callback(usbd_dev,
			USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
			USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
			cdcacm_console_request);

	/* Notify the host that DCD is asserted.
	 * Allows the use of /dev/tty* devices on *BSD/MacOS
	 */
	char buf[10];
	struct usb_cdc_notification *notif = (void*)buf;
	/* We echo signals back to host as notification */
	notif->bmRequestType = 0xA1;
	notif->bNotification = USB_CDC_NOTIFY_SERIAL_STATE;
	notif->wValue = 0;
	notif->wIndex = 0;
	notif->wLength = 2;
	buf[8] = 3; /* DCD | DSR */
	buf[9] = 0;
	usbd_ep_write_packet(usbd_dev, 0x82, buf, 10);
	notif->wIndex = 2;
	usbd_ep_write_packet(usbd_dev, 0x84, buf, 10);
}

void exti15_10_isr(void)
{
	if (gpio_get(USB_VBUS_PORT, USB_VBUS_PIN)) {
		/* Drive pull-up high if VBUS connected */
		gpio_set_mode(USB_DETACH_PORT, GPIO_MODE_OUTPUT_10_MHZ,
				GPIO_CNF_OUTPUT_PUSHPULL, USB_DETACH_PIN);
	} else {
		/* Allow pull-up to float if VBUS disconnected */
		gpio_set_mode(USB_DETACH_PORT, GPIO_MODE_INPUT,
				GPIO_CNF_INPUT_FLOAT, USB_DETACH_PIN);
	}

	exti_reset_request(USB_VBUS_PIN);
}

/**
 * Initialize the VBUS
 */
static void cdcacm_init_vbus(void) {
	rcc_peripheral_enable_clock(&RCC_APB2ENR, USB_DETACH_CLK);
	rcc_peripheral_enable_clock(&RCC_APB2ENR, USB_VBUS_CLK);

	nvic_set_priority(USB_VBUS_IRQ, (14 << 4));
	nvic_enable_irq(USB_VBUS_IRQ);

	gpio_set(USB_VBUS_PORT, USB_VBUS_PIN);
	gpio_set(USB_DETACH_PORT, USB_DETACH_PIN);

	gpio_set_mode(USB_VBUS_PORT, GPIO_MODE_INPUT,
			GPIO_CNF_INPUT_PULL_UPDOWN, USB_VBUS_PIN);

	/* Configure EXTI for USB VBUS monitor */
	exti_select_source(USB_VBUS_PIN, USB_VBUS_PORT);
	exti_set_trigger(USB_VBUS_PIN, EXTI_TRIGGER_BOTH);
	exti_enable_request(USB_VBUS_PIN);

	exti15_10_isr();
}

/**
 * Initialize the CDCACM
 */
void cdcacm_init(void) {

	/* Initialize IO ring buffers. */
	ring_init(&cdcacm_data_rx, cdcacm_data_rx_buffer, CDCACM_IO_BUFFER_SIZE);
	ring_init(&cdcacm_data_tx, cdcacm_data_tx_buffer, CDCACM_IO_BUFFER_SIZE);
	ring_init(&cdcacm_console_rx, cdcacm_console_rx_buffer, CDCACM_IO_BUFFER_SIZE);
	ring_init(&cdcacm_console_tx, cdcacm_console_tx_buffer, CDCACM_IO_BUFFER_SIZE);

	/* Reset cdcacm_status struct entries. */
	cdcacm_status.data_rx_ring_full = 0;
	cdcacm_status.console_rx_ring_full = 0;

	/* Initialize the USB stack. */
	cdcacm_usbd_dev = usbd_init(&stm32f103_usb_driver, &dev, &config, usb_strings, 6,
			    cdacm_usbd_console_buffer, sizeof(cdacm_usbd_console_buffer));

	usbd_register_set_config_callback(cdcacm_usbd_dev, cdcacm_set_config_callback);

	nvic_set_priority(NVIC_USB_LP_CAN_RX0_IRQ, (2 << 4));
	nvic_enable_irq(NVIC_USB_LP_CAN_RX0_IRQ);

	cdcacm_init_vbus();
}

/**
 * Run the CDCACM ISR
 */
void usb_lp_can_rx0_isr(void) {
	usbd_poll(cdcacm_usbd_dev);
}

/**
 * Process the content of the output ring buffers and send the data out whenever
 * possible.
 */
void cdcacm_process(void)
{
	uint8_t buf[64];
	int tx_len;

	/* Check if the EP is active, meaning it is busy and we can't send data at
	 * the moment.
	 * This implementation is non portable and should be implemented in the
	 * libopencm3 usb stack.
	 */
	if ((*USB_EP_REG(0x81 & 0x7F) & USB_EP_TX_STAT) != USB_EP_TX_STAT_VALID) {
		/* Handle data channel. */
		tx_len = ring_read(&cdcacm_data_tx, buf, 64);

		/* Get rid of the not enough data information. */
		if (tx_len < 0) tx_len = -tx_len;

		/* Send data out if we have something to send. DUH! */
		if (tx_len > 0) {
			usbd_ep_write_packet(cdcacm_usbd_dev, 0x81, buf, tx_len);
		}
	}

	if ((*USB_EP_REG(0x83 & 0x7F) & USB_EP_TX_STAT) != USB_EP_TX_STAT_VALID) {
		/* Handle data channel. */
		tx_len = ring_read(&cdcacm_console_tx, buf, 64);

		/* Get rid of the not enough data information. */
		if (tx_len < 0) tx_len = -tx_len;

		/* Send data out if we have something to send. DUH! */
		if (tx_len > 0) {
			usbd_ep_write_packet(cdcacm_usbd_dev, 0x83, buf, tx_len);
		}
	}

}
