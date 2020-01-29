#pragma once

// This is the GUID for the USB device class
const GUID GUID_DEVINTERFACE_USB_HUB = { 0xF18A0E88L, 0xC30C, 0x11D0,{ 0x88, 0x15, 0x00, 0xA0, 0xC9, 0x06, 0xBE, 0xD8 } };
// {F18A0E88-C30C-11D0-8815-00A0C906BED8}


#define USB_GET_HUB_CAPABILITIES                    271
#define USB_GET_HUB_CAPABILITIES_EX                 276
#define USB_GET_HUB_INFORMATION_EX                  277

#define IOCTL_USB_GET_HUB_CAPABILITIES      CTL_CODE(FILE_DEVICE_USB, USB_GET_HUB_CAPABILITIES, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_USB_GET_HUB_CAPABILITIES_EX   CTL_CODE(FILE_DEVICE_USB, USB_GET_HUB_CAPABILITIES_EX, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_USB_GET_HUB_INFORMATION_EX    CTL_CODE(FILE_DEVICE_USB, USB_GET_HUB_INFORMATION_EX, METHOD_BUFFERED, FILE_ANY_ACCESS)

#pragma warning( disable : 4201 )
typedef union _USB_HUB_CAP_FLAGS {
    ULONG ul;
    struct {
        ULONG HubIsHighSpeedCapable : 1;
        ULONG HubIsHighSpeed : 1;
        ULONG HubIsMultiTtCapable : 1;
        ULONG HubIsMultiTt : 1;
        ULONG HubIsRoot : 1;
        ULONG HubIsArmedWakeOnConnect : 1;
        ULONG HubIsBusPowered : 1;
        ULONG ReservedMBZ : 25;
    };

} USB_HUB_CAP_FLAGS, *PUSB_HUB_CAP_FLAGS;

#pragma warning( default : 4201 )

typedef struct _USB_HUB_CAPABILITIES {
    ULONG HubIs2xCapable : 1;
} USB_HUB_CAPABILITIES, *PUSB_HUB_CAPABILITIES;

typedef struct _USB_HUB_CAPABILITIES_EX {
    USB_HUB_CAP_FLAGS CapabilityFlags;
} USB_HUB_CAPABILITIES_EX, *PUSB_HUB_CAPABILITIES_EX;


typedef enum _USB_HUB_TYPE {
    UsbRootHub = 1,
    Usb20Hub = 2,
    Usb30Hub = 3
} USB_HUB_TYPE;

typedef struct _USB_HUB_DESCRIPTOR {
    UCHAR   bDescriptorLength;
    UCHAR   bDescriptorType;
    UCHAR   bNumberOfPorts;
    USHORT  wHubCharacteristics;
    UCHAR   bPowerOnToPowerGood;
    UCHAR   bHubControlCurrent;
    UCHAR   bRemoveAndPowerMask[64];
} USB_HUB_DESCRIPTOR, *PUSB_HUB_DESCRIPTOR;

#define USB_20_HUB_DESCRIPTOR_TYPE      0x29

//
// USB 3.0: 10.13.2.1 Hub Descriptor, Table 10-3. SuperSpeed Hub Descriptor
//
typedef struct _USB_30_HUB_DESCRIPTOR {
    UCHAR   bLength;
    UCHAR   bDescriptorType;
    UCHAR   bNumberOfPorts;
    USHORT  wHubCharacteristics;
    UCHAR   bPowerOnToPowerGood;
    UCHAR   bHubControlCurrent;
    UCHAR   bHubHdrDecLat;
    USHORT  wHubDelay;
    USHORT  DeviceRemovable;
} USB_30_HUB_DESCRIPTOR, *PUSB_30_HUB_DESCRIPTOR;

#define USB_30_HUB_DESCRIPTOR_TYPE      0x2A

typedef struct _USB_HUB_INFORMATION_EX {
    USB_HUB_TYPE             HubType;

    // The higest valid port number on the hub
    USHORT   HighestPortNumber;

    union {
        USB_HUB_DESCRIPTOR  UsbHubDescriptor;
        USB_30_HUB_DESCRIPTOR  Usb30HubDescriptor;
    } u;

} USB_HUB_INFORMATION_EX, *PUSB_HUB_INFORMATION_EX;