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

#include <stdlib.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/usb/dfu.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/exti.h>

#include "cdcacm.h"

// The recieve callback
cdcacm_receive_callback _cdcacm_receive_callback = NULL;

// The usbd device
usbd_device *cdacm_usbd_dev = NULL;

// The usbd control buffer
uint8_t cdacm_usbd_control_buffer[256];

static int configured;
static int cdcacm_data_dtr = 1;
static int cdcacm_control_dtr = 1;

// The usb device descriptor
static const struct usb_device_descriptor dev = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = 0xEF,
	.bDeviceSubClass = 2,
	.bDeviceProtocol = 1,
	.bMaxPacketSize0 = 64,
	.idVendor = 0x0484,
	.idProduct = 0x5741,
	.bcdDevice = 0x0200,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

/** Data CDCACM **/

static const struct usb_endpoint_descriptor data_comm_endp[] = {{
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x82,
	.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
	.wMaxPacketSize = 16,
	.bInterval = 255,
}};

static const struct usb_endpoint_descriptor data_data_endp[] = {{
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x01,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 1,
}, {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x81,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 1,
}};

static const struct {
	struct usb_cdc_header_descriptor header;
	struct usb_cdc_call_management_descriptor call_mgmt;
	struct usb_cdc_acm_descriptor acm;
	struct usb_cdc_union_descriptor cdc_union;
} __attribute__((packed)) data_cdcacm_functional_descriptors = {
	.header = {
		.bFunctionLength = sizeof(struct usb_cdc_header_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_HEADER,
		.bcdCDC = 0x0110,
	},
	.call_mgmt = {
		.bFunctionLength =
			sizeof(struct usb_cdc_call_management_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
		.bmCapabilities = 0,
		.bDataInterface = 1,
	},
	.acm = {
		.bFunctionLength = sizeof(struct usb_cdc_acm_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_ACM,
		.bmCapabilities = 2, /* SET_LINE_CODING supported */
	},
	.cdc_union = {
		.bFunctionLength = sizeof(struct usb_cdc_union_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_UNION,
		.bControlInterface = 0,
		.bSubordinateInterface0 = 1,
	 }
};

static const struct usb_interface_descriptor data_comm_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 1,
	.bInterfaceClass = USB_CLASS_CDC,
	.bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
	.bInterfaceProtocol = USB_CDC_PROTOCOL_AT,
	.iInterface = 4,

	.endpoint = data_comm_endp,

	.extra = &data_cdcacm_functional_descriptors,
	.extralen = sizeof(data_cdcacm_functional_descriptors)
}};

static const struct usb_interface_descriptor data_data_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 1,
	.bAlternateSetting = 0,
	.bNumEndpoints = 2,
	.bInterfaceClass = USB_CLASS_DATA,
	.bInterfaceSubClass = 0,
	.bInterfaceProtocol = 0,
	.iInterface = 0,

	.endpoint = data_data_endp,
}};

static const struct usb_iface_assoc_descriptor data_assoc = {
	.bLength = USB_DT_INTERFACE_ASSOCIATION_SIZE,
	.bDescriptorType = USB_DT_INTERFACE_ASSOCIATION,
	.bFirstInterface = 0,
	.bInterfaceCount = 2,
	.bFunctionClass = USB_CLASS_CDC,
	.bFunctionSubClass = USB_CDC_SUBCLASS_ACM,
	.bFunctionProtocol = USB_CDC_PROTOCOL_AT,
	.iFunction = 0,
};

/** control CDCACM **/

static const struct usb_endpoint_descriptor control_comm_endp[] = {{
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x84,
	.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
	.wMaxPacketSize = 16,
	.bInterval = 255,
}};

static const struct usb_endpoint_descriptor control_data_endp[] = {{
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x03,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 1,
}, {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x83,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 1,
}};

static const struct {
	struct usb_cdc_header_descriptor header;
	struct usb_cdc_call_management_descriptor call_mgmt;
	struct usb_cdc_acm_descriptor acm;
	struct usb_cdc_union_descriptor cdc_union;
} __attribute__((packed)) control_cdcacm_functional_descriptors = {
	.header = {
		.bFunctionLength = sizeof(struct usb_cdc_header_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_HEADER,
		.bcdCDC = 0x0110,
	},
	.call_mgmt = {
		.bFunctionLength =
			sizeof(struct usb_cdc_call_management_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
		.bmCapabilities = 0,
		.bDataInterface = 3,
	},
	.acm = {
		.bFunctionLength = sizeof(struct usb_cdc_acm_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_ACM,
		.bmCapabilities = 2, /* SET_LINE_CODING supported*/
	},
	.cdc_union = {
		.bFunctionLength = sizeof(struct usb_cdc_union_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_UNION,
		.bControlInterface = 2,
		.bSubordinateInterface0 = 3,
	 }
};

static const struct usb_interface_descriptor control_comm_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 2,
	.bAlternateSetting = 0,
	.bNumEndpoints = 1,
	.bInterfaceClass = USB_CLASS_CDC,
	.bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
	.bInterfaceProtocol = USB_CDC_PROTOCOL_AT,
	.iInterface = 5,

	.endpoint = control_comm_endp,

	.extra = &control_cdcacm_functional_descriptors,
	.extralen = sizeof(control_cdcacm_functional_descriptors)
}};

static const struct usb_interface_descriptor control_data_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 3,
	.bAlternateSetting = 0,
	.bNumEndpoints = 2,
	.bInterfaceClass = USB_CLASS_DATA,
	.bInterfaceSubClass = 0,
	.bInterfaceProtocol = 0,
	.iInterface = 0,

	.endpoint = control_data_endp,
}};

static const struct usb_iface_assoc_descriptor control_assoc = {
	.bLength = USB_DT_INTERFACE_ASSOCIATION_SIZE,
	.bDescriptorType = USB_DT_INTERFACE_ASSOCIATION,
	.bFirstInterface = 2,
	.bInterfaceCount = 2,
	.bFunctionClass = USB_CLASS_CDC,
	.bFunctionSubClass = USB_CDC_SUBCLASS_ACM,
	.bFunctionProtocol = USB_CDC_PROTOCOL_AT,
	.iFunction = 0,
};

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
	.iface_assoc = &control_assoc,
	.altsetting = control_comm_iface,
}, {
	.num_altsetting = 1,
	.altsetting = control_data_iface,
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
	(const char *)0x8001FF0,
	"SuperbitRF data port",
	"SuperbitRF control interface",
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
 * CDCACM control request received
 */
static int cdcacm_control_request(usbd_device *usbd_dev,
		struct usb_setup_data *req, uint8_t **buf, uint16_t *len,
		void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req)) {
	(void)usbd_dev;
	(void)complete;
	(void)buf;
	(void)len;

	switch(req->bRequest) {
	case USB_CDC_REQ_SET_CONTROL_LINE_STATE:
		/* Ignore if not for GDB interface */
		if(req->wIndex == 0)
			cdcacm_data_dtr = req->wValue & 1;
		else if(req->wIndex == 2)
			cdcacm_control_dtr = req->wValue & 1;

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
	(void) ep;
	(void) usbd_dev;
	usbd_ep_nak_set(usbd_dev, 0x01, 1);

	char buf[64];
	int len = usbd_ep_read_packet(usbd_dev, 0x01, buf, 64);

	if (len) {
		buf[len] = 0;

		if (_cdcacm_receive_callback != NULL) {
			_cdcacm_receive_callback(buf, len);
		}
	}

	usbd_ep_nak_set(usbd_dev, 0x01, 0);
}

/**
 * CDCACM control recieve callback
 */

static void cdcacm_control_rx_cb(usbd_device *usbd_dev, uint8_t ep) {
	(void) ep;
	(void) usbd_dev;
	usbd_ep_nak_set(usbd_dev, 0x03, 1);

	char buf[64];
	int len = usbd_ep_read_packet(usbd_dev, 0x03, buf, 64);

	if (len) {
		buf[len] = 0;

		// TODO FIX CONSOLE
	}

	usbd_ep_nak_set(usbd_dev, 0x03, 0);
}

/**
 * CDCACM set config
 */
 #include "led.h"
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
					64, cdcacm_control_rx_cb);
	usbd_ep_setup(usbd_dev, 0x83, USB_ENDPOINT_ATTR_BULK,
					64, NULL);
	usbd_ep_setup(usbd_dev, 0x84, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

	usbd_register_control_callback(usbd_dev,
			USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
			USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
			cdcacm_control_request);

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
	cdacm_usbd_dev = usbd_init(&stm32f103_usb_driver, &dev, &config, usb_strings, 6,
			    cdacm_usbd_control_buffer, sizeof(cdacm_usbd_control_buffer));

	usbd_register_set_config_callback(cdacm_usbd_dev, cdcacm_set_config_callback);

	nvic_set_priority(NVIC_USB_LP_CAN_RX0_IRQ, (2 << 4));
	nvic_enable_irq(NVIC_USB_LP_CAN_RX0_IRQ);

	cdcacm_init_vbus();
}

/**
 * Run the CDCACM ISR
 */
void usb_lp_can_rx0_isr(void) {
	usbd_poll(cdacm_usbd_dev);
}

/**
 * Register CDCACM receive callback
 * @param[in] callback The function for the receive callback
 */
void cdcacm_register_receive_callback(cdcacm_receive_callback callback) {
	_cdcacm_receive_callback = callback;
}

/**
 * Send data trough the CDCACM
 * @param[in] data The data that needs to be send
 * @param[in] size The size of the data in bytes
 */
bool cdcacm_send(const char *data, const int size) {
	int i = 0;

	// Check if really wanna send and someone is listening
	if(size == 0 || configured != 1 || !cdcacm_data_dtr)
		return true;

	while ((size - (i * 64)) > 64) {
		while (usbd_ep_write_packet(cdacm_usbd_dev, 0x82, (data + (i * 64)), 64) == 0);
		i++;
	}

	while (usbd_ep_write_packet(cdacm_usbd_dev, 0x82, (data + (i * 64)), size - (i * 64)) == 0);

	return true;
}
