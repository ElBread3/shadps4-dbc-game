#pragma once
#include <libusb.h>

#include <vector>

namespace Libraries::Usbd {

struct UsbTransfer;

// Usb descriptors
enum : u8 {
    USB_DESCRIPTOR_DEVICE = 0x01,
    USB_DESCRIPTOR_CONFIG = 0x02,
    USB_DESCRIPTOR_STRING = 0x03,
    USB_DESCRIPTOR_INTERFACE = 0x04,
    USB_DESCRIPTOR_ENDPOINT = 0x05,
    USB_DESCRIPTOR_HID = 0x21,
    USB_DESCRIPTOR_ACI = 0x24,
    USB_DESCRIPTOR_ENDPOINT_ASI = 0x25,
};

struct UsbDeviceDescriptor {
    u16 bcdUSB;
    u8 bDeviceClass;
    u8 bDeviceSubClass;
    u8 bDeviceProtocol;
    u8 bMaxPacketSize0;
    u16 idVendor;
    u16 idProduct;
    u16 bcdDevice;
    u8 iManufacturer;
    u8 iProduct;
    u8 iSerialNumber;
    u8 bNumConfigurations;
};

struct UsbDeviceConfiguration {
    u16 wTotalLength;
    u8 bNumInterfaces;
    u8 bConfigurationValue;
    u8 iConfiguration;
    u8 bmAttributes;
    u8 bMaxPower;
};

struct UsbDeviceInterface {
    u8 bInterfaceNumber;
    u8 bAlternateSetting;
    u8 bNumEndpoints;
    u8 bInterfaceClass;
    u8 bInterfaceSubClass;
    u8 bInterfaceProtocol;
    u8 iInterface;
};

struct UsbDeviceEndpoint {
    u8 bEndpointAddress;
    u8 bmAttributes;
    u16 wMaxPacketSize;
    u8 bInterval;
};

struct UsbDeviceHID {
    u16 bcdHID;
    u8 bCountryCode;
    u8 bNumDescriptors;
    u8 bDescriptorType;
    u16 wDescriptorLength;
};

struct UsbDeviceRequest {
    u8 bmRequestType;
    u8 bRequest;
    u16 wValue;
    u16 wIndex;
    u16 wLength;
};

struct UsbDeviceIsoRequest {
    void* buf;
    u32 start_frame;
    u32 num_packets;
    u16 packets[8];
};

struct UsbTransfer {
    u32 assigned_number = 0;
    u32 transfer_id = 0;

    s32 result = 0;
    u32 count = 0;
    UsbDeviceIsoRequest iso_request{};

    std::vector<u8> setup_buf;
    libusb_transfer* transfer = nullptr;
    bool busy = false;

    // For control transfers
    u8* control_destbuf = nullptr;

    // For fake transfers
    bool fake = false;
    u64 expected_time = 0;
    s32 expected_result = 0;
    u32 expected_count = 0;
};

// Usb descriptor helper
struct UsbDescriptorNode {
    u8 bLength{};
    u8 bDescriptorType{};

    union {
        UsbDeviceDescriptor _device;
        UsbDeviceConfiguration _configuration;
        UsbDeviceInterface _interface;
        UsbDeviceEndpoint _endpoint;
        UsbDeviceHID _hid;
        u8 data[0xFF]{};
    };

    std::vector<UsbDescriptorNode> subnodes;

    UsbDescriptorNode() {}
    template <typename T>
    UsbDescriptorNode(u8 _bDescriptorType, const T& _data)
        : bLength(sizeof(T) + 2), bDescriptorType(_bDescriptorType) {
        memcpy(data, &_data, sizeof(T));
    }
    UsbDescriptorNode(u8 _bLength, u8 _bDescriptorType, u8* _data)
        : bLength(_bLength), bDescriptorType(_bDescriptorType) {
        memcpy(data, _data, _bLength - 2);
    }

    UsbDescriptorNode& add_node(const UsbDescriptorNode& newnode) {
        subnodes.push_back(newnode);
        return subnodes.back();
    }

    u32 get_size() const {
        u32 nodesize = bLength;
        for (const auto& node : subnodes) {
            nodesize += node.get_size();
        }
        return nodesize;
    }

    u32 write_data(u8* ptr, u32 max_size) const {
        u32 size = std::min<u32>(bLength, max_size);
        memcpy(ptr, this, size);
        for (const auto& node : subnodes) {
            const u32 remaining = max_size - size;
            if (remaining == 0)
                break;
            size += node.write_data(ptr + size, remaining);
        }
        return size;
    }
};

class UsbDevice {
public:
    UsbDevice(const std::array<u8, 7>& location);
    virtual ~UsbDevice() = default;

    virtual int open_device() = 0;

    void get_location(u8* location) const;
    virtual void read_descriptors();

    virtual u32 get_configuration(u8* buf);
    virtual bool set_configuration(u8 cfg_num);
    virtual bool set_interface(u8 int_num);

    virtual void control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex,
                                  u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer) = 0;
    virtual void interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer) = 0;
    virtual void isochronous_transfer(UsbTransfer* transfer) = 0;

public:
    // base device descriptor, every other descriptor is a subnode
    UsbDescriptorNode device;

protected:
    u8 current_config = 1;
    u8 current_interface = 0;
    std::array<u8, 7> location{};

protected:
    static u64 get_timestamp();
};

class UsbDevicePassthrough : public UsbDevice {
public:
    UsbDevicePassthrough(libusb_device* _device, libusb_device_descriptor& desc,
                         const std::array<u8, 7>& location);
    ~UsbDevicePassthrough();

    int open_device() override;
    void read_descriptors() override;
    u32 get_configuration(u8* buf) override;
    bool set_configuration(u8 cfg_num) override;
    bool set_interface(u8 int_num) override;
    void control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength,
                          u32 buf_size, u8* buf, UsbTransfer* transfer) override;
    void interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer) override;
    void isochronous_transfer(UsbTransfer* transfer) override;

protected:
    void send_libusb_transfer(libusb_transfer* transfer);

protected:
    libusb_device* lusb_device = nullptr;
    libusb_device_handle* lusb_handle = nullptr;
};

class UsbDeviceEmulated : public UsbDevice {
public:
    UsbDeviceEmulated(const std::array<u8, 7>& location);
    UsbDeviceEmulated(const UsbDeviceDescriptor& _device, const std::array<u8, 7>& location);

    int open_device() override;
    void control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength,
                          u32 buf_size, u8* buf, UsbTransfer* transfer) override;
    void interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer) override;
    void isochronous_transfer(UsbTransfer* transfer) override;

    // Emulated specific functions
    void add_string(std::string str);
    u32 get_descriptor(u8 type, u8 index, u8* buf, u32 buf_size);
    u32 get_status(bool self_powered, bool remote_wakeup, u8* buf, u32 buf_size);

protected:
    std::vector<std::string> strings;
};

} // namespace Libraries::Usbd