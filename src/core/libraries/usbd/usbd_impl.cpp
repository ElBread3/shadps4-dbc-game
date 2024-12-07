#include <atomic>
#include <map>
#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <queue>
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/types.h"
#include "fmt/std.h"
#include "libusb.h"
#include "libusbi.h"
#include "usbd_impl.h"

namespace Libraries::Usbd {

void LIBUSB_CALL UsbHandler::callback_transfer(struct libusb_transfer* transfer) {
    LOG_INFO(Lib_Usbd, "Called");
    auto usbh = UsbImplementation::Instance();

    if (!usbh->is_init)
        return;

    usbh->transfer_complete(transfer);
}

#if LIBUSB_API_VERSION >= 0x0100010A
static void LIBUSB_CALL printlog_callback(libusb_context* /*ctx*/, enum libusb_log_level level,
                                          const char* str) {
    if (!str) {
        return;
    }

    const std::string str_copy = str;
    std::string msg = {};
    const size_t begin = str_copy.find_first_not_of(" \t\n");

    if (begin != str_copy.npos) {
        msg = str_copy.substr(begin, str_copy.find_last_not_of(" \t\n") + 1);
    }
    switch (level) {
    case LIBUSB_LOG_LEVEL_ERROR:
        LOG_WARNING(Lib_Usbd, "{}", msg);
        break;
    case LIBUSB_LOG_LEVEL_INFO:
        LOG_INFO(Lib_Usbd, "{}", msg);
    case LIBUSB_LOG_LEVEL_DEBUG:
        LOG_DEBUG(Lib_Usbd, "{}", msg);
    default:
        break;
    }
}
#endif

void UsbHandler::initialize() {
#if LIBUSB_API_VERSION >= 0x0100010A
    libusb_init_option log_lv_opt{};
    log_lv_opt.option = LIBUSB_OPTION_LOG_LEVEL;
    log_lv_opt.value.ival =
        LIBUSB_LOG_LEVEL_WARNING; // You can also set the LIBUSB_DEBUG env variable instead

    libusb_init_option log_cb_opt{};
    log_cb_opt.option = LIBUSB_OPTION_LOG_CB;
    log_cb_opt.value.log_cbval = &printlog_callback;

    std::vector<libusb_init_option> options = {std::move(log_lv_opt), std::move(log_cb_opt)};

    if (int res = libusb_init_context(&ctx, options.data(), static_cast<int>(options.size()));
        res < 0)
#else
    if (int res = libusb_init(&ctx); res < 0)
#endif
    {
        LOG_ERROR(Lib_Usbd, "Failed to initialize: {}", libusb_error_name(res));
        return;
    }

    for (u32 index = 0; index < MAX_SYS_USBD_TRANSFERS; index++) {
        transfers[index].transfer = libusb_alloc_transfer(8);
        transfers[index].transfer_id = index;
    }

    // look if any device which we could be interested in is actually connected
    libusb_device** list = nullptr;
    ssize_t ndev = libusb_get_device_list(ctx, &list);

    if (ndev < 0) {
        LOG_ERROR(Lib_Usbd, "Failed to get device list: %s",
                  libusb_error_name(static_cast<s32>(ndev)));
        return;
    }

    bool found_skylander = false;
    bool found_infinity = false;
    bool found_dimension = false;

    for (ssize_t index = 0; index < ndev; index++) {
        libusb_device_descriptor desc{};
        if (int res = libusb_get_device_descriptor(list[index], &desc); res < 0) {
            LOG_ERROR(Lib_Usbd, "Failed to get device descriptor: %s", libusb_error_name(res));
            continue;
        }

        auto check_device = [&](const u16 id_vendor, const u16 id_product,
                                const char* s_name) -> bool {
            if (desc.idVendor == id_vendor && desc.idProduct == id_product && true) {
                LOG_INFO(Lib_Usbd, "Found device: %s", s_name);
                libusb_ref_device(list[index]);
                std::shared_ptr<UsbDevicePassthrough> usb_dev =
                    std::make_shared<UsbDevicePassthrough>(list[index], desc, get_new_location());
                usb_devices.push_back(usb_dev);
                return true;
            }
            return false;
        };

        found_skylander = check_device(0x1430, 0x0150, "Skylanders Portal");
        found_infinity = check_device(0x0E6F, 0x0129, "Disney Infinity Base");
        found_dimension = check_device(0x0E6F, 0x0241, "Lego Dimensions Portal");
    }

    libusb_free_device_list(list, 1);

    /*if (!found_skylander) {
        LOG_INFO(Lib_Usbd, "Adding emulated skylander");
        usb_devices.push_back(std::make_shared<usb_device_skylander>(get_new_location()));
    }

    if (!found_infinity) {
        LOG_INFO(Lib_Usbd, "Adding emulated infinity base");
        usb_devices.push_back(std::make_shared<usb_device_infinity>(get_new_location()));
    }

    if (!found_dimension) {
        LOG_INFO(Lib_Usbd, "Adding emulated dimension toypad");
        usb_devices.push_back(std::make_shared<usb_device_dimensions>(get_new_location()));
    }*/
}

UsbHandler::~UsbHandler() {
    // deinitialize();
}

void UsbHandler::deinitialize() {
    // Ensures shared_ptr<usb_device> are all cleared before terminating libusb
    open_devices.clear();
    usb_devices.clear();

    for (u32 index = 0; index < MAX_SYS_USBD_TRANSFERS; index++) {
        if (transfers[index].transfer)
            libusb_free_transfer(transfers[index].transfer);
    }

    if (ctx)
        libusb_exit(ctx);
}

int UsbHandler::operate(timeval lusb_tv) {
    if (ctx) {
        // Todo: Hotplug here?

        // Process asynchronous requests that are pending
        libusb_handle_events_timeout_completed(ctx, &lusb_tv, nullptr);

        // Process fake transfers
        if (!fake_transfers.empty()) {
            std::lock_guard lock_tf(mutex_transfers);
            u64 timestamp = 0 /*get_system_time() - Emu.GetPauseTime()*/;

            for (auto it = fake_transfers.begin(); it != fake_transfers.end();) {
                auto transfer = *it;

                ASSERT(transfer->busy && transfer->fake);

                if (transfer->expected_time > timestamp) {
                    ++it;
                    continue;
                }

                transfer->result = transfer->expected_result;
                transfer->count = transfer->expected_count;
                transfer->fake = false;
                transfer->busy = false;

                it = fake_transfers.erase(it); // if we've processed this, then we erase this entry
                                               // (replacing the iterator with the new reference)
            }
        }

        //// If there is no handled devices usb thread is not actively needed
        // if (handled_devices.empty())
        //     thread_ctrl::wait_for(500'000);
        // else
        //     thread_ctrl::wait_for(1'000);
    }
    return 0;
}

void UsbHandler::transfer_complete(struct libusb_transfer* transfer) {
    std::lock_guard lock_tf(mutex_transfers);

    UsbTransfer* usbd_transfer = static_cast<UsbTransfer*>(transfer->user_data);

    if (transfer->status != 0) {
        LOG_ERROR(Lib_Usbd, "Transfer Error: %d", +transfer->status);
    }

    usbd_transfer->result = transfer->status;

    if (transfer->status == LIBUSB_TRANSFER_NO_DEVICE) {
        for (const auto& dev : usb_devices) {
            // if (dev->assigned_number == usbd_transfer->assigned_number) {
            disconnect_usb_device(dev, true);
            //    break;
            //}
        }
    }

    usbd_transfer->count = transfer->actual_length;

    for (s32 index = 0; index < transfer->num_iso_packets; index++) {
        u8 iso_status = transfer->iso_packet_desc[index].status;
        usbd_transfer->iso_request.packets[index] =
            ((iso_status & 0xF) << 12 | (transfer->iso_packet_desc[index].actual_length & 0xFFF));
    }

    if (transfer->type == LIBUSB_TRANSFER_TYPE_CONTROL && usbd_transfer->control_destbuf) {
        memcpy(usbd_transfer->control_destbuf, transfer->buffer + LIBUSB_CONTROL_SETUP_SIZE,
               transfer->actual_length);
        usbd_transfer->control_destbuf = nullptr;
    }

    usbd_transfer->busy = false;

    LOG_TRACE(Lib_Usbd, "Transfer complete(0x%x): %s", usbd_transfer->transfer_id, *transfer);
}

u32 UsbHandler::get_free_transfer_id() {
    u32 num_loops = 0;
    do {
        num_loops++;
        transfer_counter++;

        if (transfer_counter >= MAX_SYS_USBD_TRANSFERS) {
            transfer_counter = 0;
        }

        if (num_loops > MAX_SYS_USBD_TRANSFERS) {
            LOG_ERROR(Lib_Usbd, "Usb transfers are saturated!");
        }
    } while (transfers[transfer_counter].busy);

    return transfer_counter;
}

UsbTransfer& UsbHandler::get_transfer(u32 transfer_id) {
    return transfers[transfer_id];
}

std::pair<u32, UsbTransfer&> UsbHandler::get_free_transfer() {
    std::lock_guard lock_tf(mutex_transfers);

    u32 transfer_id = get_free_transfer_id();
    auto& transfer = get_transfer(transfer_id);
    transfer.busy = true;

    return {transfer_id, transfer};
}

std::pair<u32, u32> UsbHandler::get_transfer_status(u32 transfer_id) {
    std::lock_guard lock_tf(mutex_transfers);

    const auto& transfer = get_transfer(transfer_id);

    return {transfer.result, transfer.count};
}

std::pair<u32, UsbDeviceIsoRequest> UsbHandler::get_isochronous_transfer_status(
    u32 transfer_id) {
    std::lock_guard lock_tf(mutex_transfers);

    const auto& transfer = get_transfer(transfer_id);

    return {transfer.result, transfer.iso_request};
}

void UsbHandler::push_fake_transfer(UsbTransfer* transfer) {
    std::lock_guard lock_tf(mutex_transfers);
    fake_transfers.push_back(transfer);
}

int UsbHandler::open_usb_device(std::shared_ptr<UsbDevice> dev) {
    size_t size = sizeof(libusb_device_handle) + usbi_backend.device_handle_priv_size;
    void* aligned_new = ::operator new(size, std::align_val_t(sizeof(void*)));

    int err = dev->open_device();
    if (err != LIBUSB_SUCCESS) {
        LOG_ERROR(Lib_Usbd, "Failed to open USB device(VID=0x%04x, PID=0x%04x)",
                  dev->device._device.idVendor, dev->device._device.idProduct);
        return err;
    }

    open_devices.push_back(dev);
    LOG_INFO(Lib_Usbd, "USB device(VID=0x%04x, PID=0x%04x)", dev->device._device.idVendor,
             dev->device._device.idProduct);
    return err;
}

void UsbHandler::disconnect_usb_device(std::shared_ptr<UsbDevice> dev,
                                               bool update_usb_devices) {
    auto it = find(open_devices.begin(), open_devices.end(), dev);
    if (it != open_devices.end()) {
        open_devices.erase(it);
        LOG_INFO(Lib_Usbd, "USB device(VID={}, PID={}) unassigned", dev->device._device.idVendor,
                 dev->device._device.idProduct);
    }

    if (update_usb_devices) {
        auto it = find(usb_devices.begin(), usb_devices.end(), dev);
        if (it != usb_devices.end()) {
            usb_devices.erase(it);
        }
    }
}

ssize_t UsbHandler::get_device_list(libusb_device*** list) {
    // discovered_devs_alloc()
    auto discdevs = std::unique_ptr<DiscoveredDevices>(new DiscoveredDevices{0, 16, {}});
    discdevs->devices.reserve(16);

    size_t size = sizeof(libusb_device) + usbi_backend.device_priv_size;
    void* aligned_new = ::operator new(size, std::align_val_t(sizeof(void*)));

    for (int i = 0; i < usb_devices.size(); i++) {
        auto session_id = next_session_id;
        next_session_id++;

        // usbi_alloc_device()
        libusb_device* dev = new (aligned_new) libusb_device;

        usbi_atomic_store(&dev->refcnt, 1);
        dev->session_data = session_id;
        dev->speed = LIBUSB_SPEED_UNKNOWN;

        // We don't have real buses in WebUSB, just pretend everything
        // is on bus 1.
        dev->bus_number = i + 1;
        // This can wrap around but it's the best approximation of a stable
        // device address and port number we can provide.
        dev->device_address = dev->port_number = (u8)session_id;

        auto usb_dev = usb_devices[i].get();
        usb_dev->read_descriptors();

        UsbDeviceDescriptor usb_dev_desc = usb_dev->device._device;

        dev->device_descriptor.bcdDevice = usb_dev_desc.bcdDevice;
        dev->device_descriptor.bcdUSB = usb_dev_desc.bcdUSB;
        dev->device_descriptor.bDescriptorType = LIBUSB_DT_DEVICE;
        dev->device_descriptor.bDeviceClass = usb_dev_desc.bDeviceClass;
        dev->device_descriptor.bDeviceProtocol = usb_dev_desc.bDeviceProtocol;
        dev->device_descriptor.bDeviceSubClass = usb_dev_desc.bDeviceSubClass;
        dev->device_descriptor.bLength = LIBUSB_DT_DEVICE_SIZE;
        dev->device_descriptor.bMaxPacketSize0 = usb_dev_desc.bMaxPacketSize0;
        dev->device_descriptor.bNumConfigurations = usb_dev_desc.bNumConfigurations;
        dev->device_descriptor.idProduct = usb_dev_desc.idProduct;
        dev->device_descriptor.idVendor = usb_dev_desc.idVendor;
        dev->device_descriptor.iManufacturer = usb_dev_desc.iManufacturer;
        dev->device_descriptor.iProduct = usb_dev_desc.iProduct;
        dev->device_descriptor.iSerialNumber = usb_dev_desc.iSerialNumber;

        dev->attached = 1;

        // Infer the device speed from the descriptor.
        if (usb_dev_desc.bMaxPacketSize0 == 9) {
            dev->speed = dev->device_descriptor.bcdUSB >= 0x0310 ? LIBUSB_SPEED_SUPER_PLUS
                                                                 : LIBUSB_SPEED_SUPER;
        } else if (dev->device_descriptor.bcdUSB >= 0x0200) {
            dev->speed = LIBUSB_SPEED_HIGH;
        } else if (dev->device_descriptor.bMaxPacketSize0 > 8) {
            dev->speed = LIBUSB_SPEED_FULL;
        } else {
            dev->speed = LIBUSB_SPEED_LOW;
        }
        // discovered_devs_append()
        if (discdevs->length > discdevs->capacity) {
            discdevs->capacity += 16;
            discdevs->devices.reserve(discdevs->capacity);
        }
        discdevs->devices.push_back(libusb_ref_device(dev));
        discdevs->length++;
        libusb_unref_device(dev);
    }

    ssize_t count = static_cast<ssize_t>(discdevs->length);

    auto list_copy = new libusb_device*[count + 1]; // +1 for the null terminator
    if (!list_copy) {
        count = LIBUSB_ERROR_NO_MEM;
        goto out;
    }

    for (ssize_t i = 0; i < count; ++i) {
        libusb_device* dev = discdevs->devices[i];
        list_copy[i] = libusb_ref_device(dev);
    }
    list_copy[count] = nullptr;
    *list = list_copy;

out:
    // discovered_devs_free()
    for (size_t i = 0; i < discdevs->length; ++i) {
        libusb_unref_device(discdevs->devices[i]);
    }
    // discdevs frees itself
    return count;
}

int UsbHandler::open_device(libusb_device* dev, libusb_device_handle** dev_handle) {
    struct libusb_device_handle* _dev_handle;

    auto* dev2 = this->find_device_from_bus_number(libusb_get_bus_number(dev));
    if (dev2 == nullptr) {
        return LIBUSB_ERROR_NO_DEVICE;
    }

    size_t size = sizeof(*_dev_handle) + usbi_backend.device_handle_priv_size;
    void* aligned_new = ::operator new(size, std::align_val_t(sizeof(void*)));

    _dev_handle = new (aligned_new) libusb_device_handle;
    if (!_dev_handle) {
        return LIBUSB_ERROR_NO_MEM;
    }

    _dev_handle->dev = libusb_ref_device(dev);

    int err = this->open_usb_device(*dev2);
    if (err < 0) {
        libusb_unref_device(dev);
        free(_dev_handle);
        return err;
    }

    *dev_handle = _dev_handle;

    return 0;
}

libusb_device* UsbHandler::find_device_from_ids(struct libusb_device** devs, u16 vendor_id,
                                                u16 product_id) {
    struct libusb_device* dev;
    size_t i = 0;

    while ((dev = devs[i++]) != nullptr) {
        struct libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(dev, &desc) < 0) {
            return nullptr;
        }
        if (desc.idVendor == vendor_id && desc.idProduct == product_id) {
            return dev;
        }
    }
    return nullptr;
}

std::shared_ptr<UsbDevice>* UsbHandler::find_device_from_bus_number(int bus_number) {
    if (bus_number < 0 || bus_number >= usb_devices.size()) {
        return nullptr;
    }
    return &usb_devices[bus_number];
}

libusb_device_handle* UsbHandler::open_device_with_ids(u16 vendor_id, u16 product_id) {
    struct libusb_device** devs;
    struct libusb_device_handle* dev_handle = nullptr;

    if (this->get_device_list(&devs) < 0) {
        return nullptr;
    }

    struct libusb_device* dev = this->find_device_from_ids(devs, vendor_id, product_id);
    if (dev && this->open_device(dev, &dev_handle) < 0) {
        return nullptr;
    }

    libusb_free_device_list(devs, 1);
    return dev_handle;
}
} // namespace Libraries::Usbd