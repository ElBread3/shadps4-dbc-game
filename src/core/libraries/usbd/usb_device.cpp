#include <algorithm>
#include <codecvt>
#include <locale>
#include <string>
#include <libusb.h>
#include "common/assert.h"
#include "common/logging/log.h"
#include "usb_device.h"
#include "usbd_impl.h"

namespace Libraries::Usbd {

extern void LIBUSB_CALL callback_transfer(struct libusb_transfer* transfer);

//////////////////////////////////////////////////////////////////
// ALL DEVICES ///////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

UsbDevice::UsbDevice(const std::array<u8, 7>& location) {
    this->location = location;
}

void UsbDevice::get_location(u8* location) const {
    memcpy(location, this->location.data(), 7);
}

void UsbDevice::read_descriptors() {}

u32 UsbDevice::get_configuration(u8* buf) {
    *buf = current_config;
    return sizeof(u8);
}

bool UsbDevice::set_configuration(u8 cfg_num) {
    current_config = cfg_num;
    return true;
}

bool UsbDevice::set_interface(u8 int_num) {
    current_interface = int_num;
    return true;
}

u64 UsbDevice::get_timestamp() {
    return 0; /*(get_system_time() - Emu.GetPauseTime())*/
}

UsbDevicePassthrough::UsbDevicePassthrough(libusb_device* _device, libusb_device_descriptor& desc,
                                           const std::array<u8, 7>& location)
    : UsbDevice(location), lusb_device(_device) {
    device = UsbDescriptorNode(
        USB_DESCRIPTOR_DEVICE,
        UsbDeviceDescriptor{desc.bcdUSB, desc.bDeviceClass, desc.bDeviceSubClass,
                            desc.bDeviceProtocol, desc.bMaxPacketSize0, desc.idVendor,
                            desc.idProduct, desc.bcdDevice, desc.iManufacturer, desc.iProduct,
                            desc.iSerialNumber, desc.bNumConfigurations});
}

UsbDevicePassthrough::~UsbDevicePassthrough() {
    if (lusb_handle) {
        libusb_release_interface(lusb_handle, 0);
        libusb_close(lusb_handle);
    }

    if (lusb_device) {
        libusb_unref_device(lusb_device);
    }
}

void UsbDevicePassthrough::send_libusb_transfer(libusb_transfer* transfer) {
    while (true) {
        auto res = libusb_submit_transfer(transfer);
        switch (res) {
        case LIBUSB_SUCCESS:
            return;
        case LIBUSB_ERROR_BUSY:
            continue;
        default: {
            LOG_ERROR(Lib_Usbd, "Unexpected error from libusb_submit_transfer: %d(%s)", res,
                      libusb_error_name(res));
            return;
        }
        }
    }
}

int UsbDevicePassthrough::open_device() {
    int err = libusb_open(lusb_device, &lusb_handle);
    if (err != LIBUSB_SUCCESS) {
        return err;
    }
#ifdef __linux__
    libusb_set_auto_detach_kernel_driver(lusb_handle, true);
#endif
    return LIBUSB_SUCCESS;
}

void UsbDevicePassthrough::read_descriptors() {
    // Directly getting configuration descriptors from the device instead of going through libusb
    // parsing functions as they're not needed
    for (u8 index = 0; index < device._device.bNumConfigurations; index++) {
        u8 buf[1000];
        int ssize = libusb_control_transfer(
            lusb_handle,
            +LIBUSB_ENDPOINT_IN | +LIBUSB_REQUEST_TYPE_STANDARD | +LIBUSB_RECIPIENT_DEVICE,
            LIBUSB_REQUEST_GET_DESCRIPTOR, 0x0200 | index, 0, buf, 1000, 0);
        if (ssize < 0) {
            LOG_ERROR(Lib_Usbd, "Couldn't get the config from the device: %d(%s)", ssize,
                      libusb_error_name(ssize));
            continue;
        }

        // Minimalistic parse
        auto& conf = device.add_node(UsbDescriptorNode(buf[0], buf[1], &buf[2]));

        for (int index = buf[0]; index < ssize;) {
            conf.add_node(UsbDescriptorNode(buf[index], buf[index + 1], &buf[index + 2]));
            index += buf[index];
        }
    }
}

u32 UsbDevicePassthrough::get_configuration(u8* buf) {
    return (libusb_get_configuration(lusb_handle, reinterpret_cast<int*>(buf)) == LIBUSB_SUCCESS)
               ? sizeof(u8)
               : 0;
};

bool UsbDevicePassthrough::set_configuration(u8 cfg_num) {
    UsbDevice::set_configuration(cfg_num);
    return (libusb_set_configuration(lusb_handle, cfg_num) == LIBUSB_SUCCESS);
};

bool UsbDevicePassthrough::set_interface(u8 int_num) {
    UsbDevice::set_interface(int_num);
    return (libusb_claim_interface(lusb_handle, int_num) == LIBUSB_SUCCESS);
}

void UsbDevicePassthrough::control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex,
                                            [[maybe_unused]] u16 wLength, u32 buf_size, u8* buf,
                                            UsbTransfer* transfer) {
    if (transfer->setup_buf.size() < buf_size + LIBUSB_CONTROL_SETUP_SIZE)
        transfer->setup_buf.resize(buf_size + LIBUSB_CONTROL_SETUP_SIZE);

    transfer->control_destbuf = (bmRequestType & LIBUSB_ENDPOINT_IN) ? buf : nullptr;

    libusb_fill_control_setup(transfer->setup_buf.data(), bmRequestType, bRequest, wValue, wIndex,
                              buf_size);
    memcpy(transfer->setup_buf.data() + LIBUSB_CONTROL_SETUP_SIZE, buf, buf_size);
    libusb_fill_control_transfer(transfer->transfer, lusb_handle, transfer->setup_buf.data(),
                                 nullptr, transfer, 0);
    send_libusb_transfer(transfer->transfer);
}

void UsbDevicePassthrough::interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint,
                                              UsbTransfer* transfer) {
    libusb_fill_interrupt_transfer(transfer->transfer, lusb_handle, endpoint, buf, buf_size,
                                   nullptr, transfer, 0);
    send_libusb_transfer(transfer->transfer);
}

void UsbDevicePassthrough::isochronous_transfer(UsbTransfer* transfer) {
    // TODO actual endpoint
    // TODO actual size?
    libusb_fill_iso_transfer(transfer->transfer, lusb_handle, 0x81,
                             static_cast<u8*>(transfer->iso_request.buf), 0xFFFF,
                             transfer->iso_request.num_packets, callback_transfer, transfer, 0);

    for (u32 index = 0; index < transfer->iso_request.num_packets; index++) {
        transfer->transfer->iso_packet_desc[index].length = transfer->iso_request.packets[index];
    }

    send_libusb_transfer(transfer->transfer);
}

UsbDeviceEmulated::UsbDeviceEmulated(const std::array<u8, 7>& location) : UsbDevice(location) {}

UsbDeviceEmulated::UsbDeviceEmulated(const UsbDeviceDescriptor& _device,
                                     const std::array<u8, 7>& location)
    : UsbDevice(location) {
    device = UsbDescriptorNode(USB_DESCRIPTOR_DEVICE, _device);
}

int UsbDeviceEmulated::open_device() {
    return LIBUSB_SUCCESS;
}

u32 UsbDeviceEmulated::get_descriptor(u8 type, u8 index, u8* buf, u32 buf_size) {
    if (!buf) {
        return 0;
    }

    std::array<u8, 2> header;
    header = {static_cast<u8>(header.size()), type};

    u32 expected_count = std::min<u32>(static_cast<u32>(header.size()), buf_size);
    std::memcpy(buf, header.data(), expected_count);

    if (expected_count < header.size())
        return expected_count;

    switch (type) {
    case USB_DESCRIPTOR_DEVICE: {
        buf[0] = device.bLength;
        expected_count = std::min(device.bLength, static_cast<u8>(buf_size));
        std::memcpy(buf + header.size(), device.data, expected_count - header.size());
        break;
    }
    case USB_DESCRIPTOR_CONFIG: {
        if (index < device.subnodes.size()) {
            buf[0] = device.subnodes[index].bLength;
            expected_count = std::min(device.subnodes[index].bLength, static_cast<u8>(buf_size));
            std::memcpy(buf + header.size(), device.subnodes[index].data,
                        expected_count - header.size());
        }
        break;
    }
    case USB_DESCRIPTOR_STRING: {
        if (index < strings.size() + 1) {
            if (index == 0) {
                constexpr u8 len = static_cast<u8>(sizeof(u16) + header.size());
                buf[0] = len;
                expected_count = std::min(len, static_cast<u8>(buf_size));
                constexpr u16 langid = 0x0409; // English (United States)
                std::memcpy(buf + header.size(), &langid, expected_count - header.size());
            } else {
                std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
                const std::u16string u16str = converter.from_bytes(strings[index - 1]);
                const u8 len = static_cast<u8>(std::min(u16str.size() * sizeof(u16) + header.size(),
                                                        static_cast<size_t>(0xFF)));
                buf[0] = len;
                expected_count = std::min(len, static_cast<u8>(std::min<u32>(255, buf_size)));
                std::memcpy(buf + header.size(), u16str.data(), expected_count - header.size());
            }
        }
        break;
    }
    default:
        LOG_ERROR(Lib_Usbd, "Unhandled DescriptorType");
        break;
    }

    return expected_count;
}

u32 UsbDeviceEmulated::get_status(bool self_powered, bool remote_wakeup, u8* buf, u32 buf_size) {
    const u32 expected_count = buf ? std::min<u32>(sizeof(u16), buf_size) : 0;
    const u16 device_status = static_cast<int>(self_powered) | static_cast<int>(remote_wakeup) << 1;
    std::memcpy(buf, &device_status, expected_count);
    return expected_count;
}

void UsbDeviceEmulated::control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex,
                                         u16 /*wLength*/, u32 buf_size, u8* buf,
                                         UsbTransfer* transfer) {
    transfer->fake = true;
    transfer->expected_count = buf_size;
    transfer->expected_result = LIBUSB_SUCCESS;
    transfer->expected_time = UsbDevice::get_timestamp() + 100;

    switch (bmRequestType) {
    case 0U /*silences warning*/ | LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_STANDARD |
        LIBUSB_RECIPIENT_DEVICE: // 0x00
        switch (bRequest) {
        case LIBUSB_REQUEST_SET_CONFIGURATION:
            UsbDevice::set_configuration(static_cast<u8>(wValue));
            break;
        default:
            LOG_ERROR(Lib_Usbd, "Unhandled control transfer({}): {}", bmRequestType, bRequest);
            break;
        }
        break;
    case 0U /*silences warning*/ | LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_STANDARD |
        LIBUSB_RECIPIENT_INTERFACE: // 0x01
        switch (bRequest) {
        case LIBUSB_REQUEST_SET_INTERFACE:
            UsbDevice::set_interface(static_cast<u8>(wIndex));
            break;
        default:
            LOG_ERROR(Lib_Usbd, "Unhandled control transfer({}): {}", bmRequestType, bRequest);
            break;
        }
        break;
    case 0U /*silences warning*/ | LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD |
        LIBUSB_RECIPIENT_DEVICE: // 0x80
        switch (bRequest) {
        case LIBUSB_REQUEST_GET_STATUS:
            transfer->expected_count = get_status(false, false, buf, buf_size);
            break;
        case LIBUSB_REQUEST_GET_DESCRIPTOR:
            transfer->expected_count = get_descriptor(wValue >> 8, wValue & 0xFF, buf, buf_size);
            break;
        case LIBUSB_REQUEST_GET_CONFIGURATION:
            transfer->expected_count = get_configuration(buf);
            break;
        default:
            LOG_ERROR(Lib_Usbd, "Unhandled control transfer({}): {}", bmRequestType, bRequest);
            break;
        }
        break;
    default:
        LOG_ERROR(Lib_Usbd, "Unhandled control transfer: {}", bmRequestType);
        break;
    }
}

// Temporarily
#ifndef _MSC_VER
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

void UsbDeviceEmulated::interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint,
                                           UsbTransfer* transfer) {}

void UsbDeviceEmulated::isochronous_transfer(UsbTransfer* transfer) {}

void UsbDeviceEmulated::add_string(std::string str) {
    strings.emplace_back(std::move(str));
}
} // namespace Libraries::Usbd