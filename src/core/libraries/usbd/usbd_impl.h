#pragma once

#include <map>
#include <shared_mutex>
#include <string_view>
#include <unordered_map>
#include <queue>
#include "common/singleton.h"
#include "common/types.h"
#include "libusb.h"
#include "usb_device.h"

namespace Libraries::Usbd {

#define MAX_SYS_USBD_TRANSFERS 0x44

struct UsbStringHash {
    using hash_type = std::hash<std::string_view>;
    using is_transparent = void;

    std::size_t operator()(const char* str) const {
        return hash_type{}(str);
    }
    std::size_t operator()(std::string_view str) const {
        return hash_type{}(str);
    }
    std::size_t operator()(std::string const& str) const {
        return hash_type{}(str);
    }
};

struct UsbPipe {
    std::shared_ptr<UsbDevice> device = nullptr;

    u8 endpoint = 0;
};

struct DiscoveredDevices {
    size_t length;
    size_t capacity;
    std::vector<libusb_device*> devices;
};

class UsbHandler {
public:
    void LIBUSB_CALL callback_transfer(struct libusb_transfer* transfer);

    void initialize();
    void deinitialize();

    int operate(timeval lusb_tv);

    void transfer_complete(libusb_transfer* transfer);

    std::pair<u32, UsbTransfer&> get_free_transfer();
    std::pair<u32, u32> get_transfer_status(u32 transfer_id);
    std::pair<u32, UsbDeviceIsoRequest> get_isochronous_transfer_status(u32 transfer_id);
    void push_fake_transfer(UsbTransfer* transfer);

    const std::array<u8, 7>& get_new_location();
    int open_usb_device(std::shared_ptr<UsbDevice> dev);
    void disconnect_usb_device(std::shared_ptr<UsbDevice> dev, bool update_usb_devices);

    unsigned long next_session_id = 0;
    ssize_t get_device_list(libusb_device*** list);

    int open_device(libusb_device* dev, libusb_device_handle** dev_handle);
    libusb_device_handle* open_device_with_ids(u16 vendor_id, u16 product_id);

    libusb_device* find_device_from_ids(struct libusb_device** devs, u16 vendor_id, u16 product_id);
    std::shared_ptr<UsbDevice>* UsbHandler::find_device_from_bus_number(int bus_number);

    std::vector<std::shared_ptr<UsbDevice>> open_devices;

    std::shared_mutex mutex;
    std::atomic<bool> is_init = false;

private:
    // Lock free functions for internal use(ie make sure to lock before using those)
    UsbTransfer& get_transfer(u32 transfer_id);
    u32 get_free_transfer_id();

private:
    // Transfers infos
    u32 transfer_counter = 0;
    std::shared_mutex mutex_transfers;
    std::array<UsbTransfer, MAX_SYS_USBD_TRANSFERS> transfers;
    std::vector<UsbTransfer*> fake_transfers;

    // List of devices "connected" to the ps3
    std::vector<std::shared_ptr<UsbDevice>> usb_devices;

    libusb_context* ctx = nullptr;
};

using UsbImplementation = Common::Singleton<UsbHandler>;
} // namespace Libraries::Usbd