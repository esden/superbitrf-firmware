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

#ifndef MODULES_USB_STRUCT_TEMPLATES_H_
#define MODULES_USB_STRUCT_TEMPLATES_H_

/**
 * The usb device descriptor preset for a misc device with composite
 * functions.
 */
#define USB_DEVICE_DESCRIPTOR_MISC(DEV_NAME, ID_VENDOR, ID_PRODUCT)			\
static const struct usb_device_descriptor DEV_NAME = {						\
	.bLength = USB_DT_DEVICE_SIZE,											\
	.bDescriptorType = USB_DT_DEVICE,										\
	.bcdUSB = 0x0200,														\
	.bDeviceClass = 0xEF, /* Miscellaneous */								\
	.bDeviceSubClass = 2,													\
	.bDeviceProtocol = 1,													\
	/* ^ Protocol: Together with Class and SubClass results in: 			\
	 * Interface Association Descriptor.									\
     */																		\
	.bMaxPacketSize0 = 64,													\
	.idVendor = (ID_VENDOR), /*0x0484,*/									\
	.idProduct = (ID_PRODUCT), /*0x5741,*/									\
	.bcdDevice = 0x0200,													\
	.iManufacturer = 1,														\
	.iProduct = 2,															\
	.iSerialNumber = 3,														\
	.bNumConfigurations = 1,												\
};


/***************************************************************
 * CDCACM structures
 ***************************************************************/
/**
 * Endpoint description struct preset for CDCACM command endpoint.
 */
#define USB_CDCACM_COMMAND_EP_DESCRIPTOR(EP_NAME, EP_ADDRESS)				\
static const struct usb_endpoint_descriptor EP_NAME[] = {{					\
	.bLength = USB_DT_ENDPOINT_SIZE,										\
	.bDescriptorType = USB_DT_ENDPOINT,										\
	.bEndpointAddress = (EP_ADDRESS),										\
	.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,							\
	.wMaxPacketSize = 16,													\
	.bInterval = 255,														\
}};

#define USB_CDCACM_DATA_EP_DESCRIPTOR(EP_NAME, EP_RX_ADDRESS, EP_TX_ADDRESS) \
static const struct usb_endpoint_descriptor EP_NAME[] = {{					\
	.bLength = USB_DT_ENDPOINT_SIZE,										\
	.bDescriptorType = USB_DT_ENDPOINT,										\
	.bEndpointAddress = (EP_RX_ADDRESS),									\
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,									\
	.wMaxPacketSize = 64,													\
	.bInterval = 1,															\
}, {																		\
	.bLength = USB_DT_ENDPOINT_SIZE,										\
	.bDescriptorType = USB_DT_ENDPOINT,										\
	.bEndpointAddress = (EP_TX_ADDRESS),									\
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,									\
	.wMaxPacketSize = 64,													\
	.bInterval = 1,															\
}};

#define USB_CDCACM_FUNCTIONAL_DESCRIPTORS(NAME, DATA_IFACE, CTRL_IFACE, CTRL_SUB_IFACE)	\
static const struct {														\
	struct usb_cdc_header_descriptor header;								\
	struct usb_cdc_call_management_descriptor call_mgmt;					\
	struct usb_cdc_acm_descriptor acm;										\
	struct usb_cdc_union_descriptor cdc_union;								\
} __attribute__((packed)) NAME = {											\
	.header = {																\
		.bFunctionLength = sizeof(struct usb_cdc_header_descriptor),		\
		.bDescriptorType = CS_INTERFACE,									\
		.bDescriptorSubtype = USB_CDC_TYPE_HEADER,							\
		.bcdCDC = 0x0110,													\
	},																		\
	.call_mgmt = {															\
		.bFunctionLength =													\
			sizeof(struct usb_cdc_call_management_descriptor),				\
		.bDescriptorType = CS_INTERFACE,									\
		.bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,					\
		.bmCapabilities = 0,												\
		.bDataInterface = (DATA_IFACE),										\
	},																		\
	.acm = {																\
		.bFunctionLength = sizeof(struct usb_cdc_acm_descriptor),			\
		.bDescriptorType = CS_INTERFACE,									\
		.bDescriptorSubtype = USB_CDC_TYPE_ACM,								\
		.bmCapabilities = 2, /* SET_LINE_CODING supported */				\
	},																		\
	.cdc_union = {															\
		.bFunctionLength = sizeof(struct usb_cdc_union_descriptor),			\
		.bDescriptorType = CS_INTERFACE,									\
		.bDescriptorSubtype = USB_CDC_TYPE_UNION,							\
		.bControlInterface = (CTRL_IFACE),									\
		.bSubordinateInterface0 = (CTRL_SUB_IFACE),							\
	 }																		\
};

#define USB_CDCACM_COMMAND_INTERFACE(NAME, IFACE_NR, IFACE, COMM_EP_NAME, FUNC_DESC_NAME)	\
static const struct usb_interface_descriptor NAME[] = {{					\
	.bLength = USB_DT_INTERFACE_SIZE,										\
	.bDescriptorType = USB_DT_INTERFACE,									\
	.bInterfaceNumber = (IFACE_NR),											\
	.bAlternateSetting = 0,													\
	.bNumEndpoints = 1,														\
	.bInterfaceClass = USB_CLASS_CDC,										\
	.bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,								\
	.bInterfaceProtocol = USB_CDC_PROTOCOL_AT,								\
	.iInterface = (IFACE),													\
																			\
	.endpoint = COMM_EP_NAME,												\
																			\
	.extra = &FUNC_DESC_NAME,												\
	.extralen = sizeof(FUNC_DESC_NAME)										\
}};

#define USB_CDCACM_DATA_INTERFACE(NAME, IFACE_NR, DATA_EP_NAME)				\
static const struct usb_interface_descriptor NAME[] = {{					\
	.bLength = USB_DT_INTERFACE_SIZE,										\
	.bDescriptorType = USB_DT_INTERFACE,									\
	.bInterfaceNumber = (IFACE_NR),											\
	.bAlternateSetting = 0,													\
	.bNumEndpoints = 2,														\
	.bInterfaceClass = USB_CLASS_DATA,										\
	.bInterfaceSubClass = 0,												\
	.bInterfaceProtocol = 0,												\
	.iInterface = 0,														\
																			\
	.endpoint = DATA_EP_NAME,												\
}};

#define USB_CDCACM_ASSOCIATION_DESCRIPTOR(NAME, FIRST_IFACE)				\
static const struct usb_iface_assoc_descriptor NAME = {						\
	.bLength = USB_DT_INTERFACE_ASSOCIATION_SIZE,							\
	.bDescriptorType = USB_DT_INTERFACE_ASSOCIATION,						\
	.bFirstInterface = (FIRST_IFACE),										\
	.bInterfaceCount = 2,													\
	.bFunctionClass = USB_CLASS_CDC,										\
	.bFunctionSubClass = USB_CDC_SUBCLASS_ACM,								\
	.bFunctionProtocol = USB_CDC_PROTOCOL_AT,								\
	.iFunction = 0,															\
};

#endif /* MODULES_USB_STRUCT_TEMPLATES_H_ */