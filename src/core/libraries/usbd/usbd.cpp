// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <magic_enum.hpp>
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "libusb.h"
#include "usb_device.h"
#include "usbd.h"
#include "usbd_impl.h"

namespace Libraries::Usbd {

static int LibusbErrToOrbis(const int error_code) {
    ASSERT_MSG(error_code < 1, "Passed an invalid error code!");

    switch (error_code) {
    case LIBUSB_SUCCESS:
        return ORBIS_OK;
    case LIBUSB_ERROR_IO:
        return SCE_USBD_ERROR_IO;
    case LIBUSB_ERROR_INVALID_PARAM:
        return SCE_USBD_ERROR_INVALID_ARG;
    case LIBUSB_ERROR_ACCESS:
        return SCE_USBD_ERROR_ACCESS;
    case LIBUSB_ERROR_NO_DEVICE:
        return SCE_USBD_ERROR_NO_DEVICE;
    case LIBUSB_ERROR_NOT_FOUND:
        return SCE_USBD_ERROR_NOT_FOUND;
    case LIBUSB_ERROR_BUSY:
        return SCE_USBD_ERROR_BUSY;
    case LIBUSB_ERROR_TIMEOUT:
        return SCE_USBD_ERROR_TIMEOUT;
    case LIBUSB_ERROR_OVERFLOW:
        return SCE_USBD_ERROR_OVERFLOW;
    case LIBUSB_ERROR_PIPE:
        return SCE_USBD_ERROR_PIPE;
    case LIBUSB_ERROR_INTERRUPTED:
        return SCE_USBD_ERROR_INTERRUPTED;
    case LIBUSB_ERROR_NO_MEM:
        return SCE_USBD_ERROR_NO_MEM;
    case LIBUSB_ERROR_NOT_SUPPORTED:
        return SCE_USBD_ERROR_NOT_SUPPORTED;
    case LIBUSB_ERROR_OTHER:
    default:
        return SCE_USBD_ERROR_FATAL;
    }
}

libusb_transfer* PS4_SYSV_ABI sceUsbdAllocTransfer(s32 iso_packets) {
    LOG_INFO(Lib_Usbd, "called");
    return libusb_alloc_transfer(iso_packets);
}

int PS4_SYSV_ABI sceUsbdAttachKernelDriver(libusb_device_handle* dev_handle, s32 interface_num) {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    if (dev_handle == nullptr) {
        return SCE_USBD_ERROR_INVALID_ARG;
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdBulkTransfer() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdCancelTransfer(struct libusb_transfer* transfer) {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdCheckConnected(libusb_device_handle* dev_handle) {
    LOG_INFO(Lib_Usbd, "called");
    if (dev_handle == nullptr) {
        return SCE_USBD_ERROR_INVALID_ARG;
    }

    // Libusb doesn't have this, so I guess we'll check if we can still get the device's info
    struct libusb_device_descriptor desc;
    libusb_device* dev = sceUsbdGetDevice(dev_handle);
    if (!dev) {
        return SCE_USBD_ERROR_NO_DEVICE; //???
    }

    int err = sceUsbdGetDeviceDescriptor(dev, &desc);
    if (err < 0 || !desc.idProduct) {
        return SCE_USBD_ERROR_NO_DEVICE;
    }
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdClaimInterface(libusb_device_handle* dev_handle, s32 interface_num) {
    LOG_INFO(Lib_Usbd, "called");
    if (dev_handle == nullptr || interface_num < 32) {
        return SCE_USBD_ERROR_INVALID_ARG;
    }
    UNREACHABLE_MSG("not yet");
    int err = libusb_claim_interface(dev_handle, interface_num);
    return LibusbErrToOrbis(err);
}

void PS4_SYSV_ABI sceUsbdClearHalt() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    UNREACHABLE_MSG("unimplemented");
}

void PS4_SYSV_ABI sceUsbdClose(libusb_device_handle* dev_handle) {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    libusb_close(dev_handle);
}

int PS4_SYSV_ABI sceUsbdControlTransfer(libusb_device_handle* dev_handle, u8 request_type,
                                        u8 request, u16 value, u16 index, unsigned char* data,
                                        u16 length, u32 timeout) {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    int bytes = libusb_control_transfer(dev_handle, request_type, request, value, index, data,
                                        length, timeout);
    if (bytes < 0) {
        return LibusbErrToOrbis(bytes);
    }
    return bytes;
}

unsigned char* PS4_SYSV_ABI sceUsbdControlTransferGetData(struct libusb_transfer* transfer) {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    UNREACHABLE_MSG("unimplemented");
    return 0;
}

struct libusb_control_setup* PS4_SYSV_ABI
sceUsbdControlTransferGetSetup(libusb_transfer* transfer) {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    UNREACHABLE_MSG("unimplemented");
    return nullptr;
}

int PS4_SYSV_ABI sceUsbdDetachKernelDriver(libusb_device_handle* dev_handle, s32 interface_num) {
    LOG_INFO(Lib_Usbd, "called");
    if (dev_handle == nullptr) {
        return SCE_USBD_ERROR_INVALID_ARG;
    }
    return ORBIS_OK;
}

void PS4_SYSV_ABI sceUsbdEventHandlerActive() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    UNREACHABLE_MSG("unimplemented");
}

void PS4_SYSV_ABI sceUsbdEventHandlingOk() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    UNREACHABLE_MSG("unimplemented");
}

int PS4_SYSV_ABI sceUsbdExit() {
    LOG_INFO(Lib_Usbd, "called");
    UsbImplementation::Instance()->deinitialize();
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdFillBulkTransfer() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

void PS4_SYSV_ABI sceUsbdFillControlSetup(unsigned char* buffer, u8 request_type, u8 request,
                                          u16 value, u16 index, u16 length) {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    UNREACHABLE_MSG("unimplemented");
}

void PS4_SYSV_ABI sceUsbdFillControlTransfer(libusb_transfer* transfer,
                                             libusb_device_handle* dev_handle,
                                             unsigned char* buffer, libusb_transfer_cb_fn callback,
                                             void* user_data, u32 timeout) {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    UNREACHABLE_MSG("unimplemented");
}

void PS4_SYSV_ABI sceUsbdFillInterruptTransfer(struct libusb_transfer* transfer,
                                               libusb_device_handle* dev_handle,
                                               unsigned char endpoint, unsigned char* buffer,
                                               s32 length, libusb_transfer_cb_fn callback,
                                               void* user_data, u32 timeout) {
    LOG_INFO(Lib_Usbd, "called");
    libusb_fill_interrupt_transfer(transfer, dev_handle, endpoint, buffer, length, callback,
                                   user_data, timeout);
}

void PS4_SYSV_ABI sceUsbdFillIsoTransfer(struct libusb_transfer* transfer,
                                         libusb_device_handle* dev_handle, unsigned char endpoint,
                                         unsigned char* buffer, s32 length,
                                         libusb_transfer_cb_fn callback, void* user_data,
                                         u32 timeout) {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    UNREACHABLE_MSG("unimplemented");
}

void PS4_SYSV_ABI sceUsbdFreeConfigDescriptor(struct libusb_config_descriptor* config) {
    LOG_INFO(Lib_Usbd, "called");
    libusb_free_config_descriptor(config);
}

void PS4_SYSV_ABI sceUsbdFreeDeviceList(libusb_device** list) {
    LOG_INFO(Lib_Usbd, "called");
    ASSERT(list != nullptr);
    libusb_free_device_list(list, 0);
}

void PS4_SYSV_ABI sceUsbdFreeTransfer(struct libusb_transfer* transfer) {
    LOG_INFO(Lib_Usbd, "called");
    libusb_free_transfer(transfer);
}

int PS4_SYSV_ABI sceUsbdGetActiveConfigDescriptor(libusb_device* dev,
                                                  struct libusb_config_descriptor** config) {
    LOG_INFO(Lib_Usbd, "called");
    if (dev == nullptr || config == nullptr) {
        return SCE_USBD_ERROR_INVALID_ARG;
    }
    int err = libusb_get_active_config_descriptor(dev, config);
    return LibusbErrToOrbis(err);
}

int PS4_SYSV_ABI sceUsbdGetBusNumber(libusb_device* dev) {
    LOG_INFO(Lib_Usbd, "called");
    if (dev == nullptr) {
        return 0;
    }
    return libusb_get_bus_number(dev);
}

int PS4_SYSV_ABI sceUsbdGetConfigDescriptor(libusb_device* dev, u8 config_index,
                                            libusb_config_descriptor** config) {
    LOG_INFO(Lib_Usbd, "called");
    if (dev == nullptr || config == nullptr) {
        return SCE_USBD_ERROR_INVALID_ARG;
    }
    UNREACHABLE_MSG("not yet");
    int err = libusb_get_config_descriptor(dev, config_index, config);
    return LibusbErrToOrbis(err);
}

int PS4_SYSV_ABI sceUsbdGetConfigDescriptorByValue(libusb_device* dev, u8 config_value,
                                                   struct libusb_config_descriptor** config) {
    LOG_INFO(Lib_Usbd, "called");
    if (dev == nullptr || config == nullptr) {
        return SCE_USBD_ERROR_INVALID_ARG;
    }
    UNREACHABLE_MSG("not yet");
    int err = libusb_get_config_descriptor_by_value(dev, config_value, config);
    return LibusbErrToOrbis(err);
}

int PS4_SYSV_ABI sceUsbdGetConfiguration(libusb_device_handle* dev_handle, s32* config) {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdGetDescriptor(libusb_device_handle* dev_handle, u8 desc_type, u8 desc_index,
                                      unsigned char* data, s32 length) {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

libusb_device* PS4_SYSV_ABI sceUsbdGetDevice(libusb_device_handle* dev_handle) {
    LOG_INFO(Lib_Usbd, "called");
    if (dev_handle == nullptr) {
        return 0;
    }
    return libusb_get_device(dev_handle);
}

int PS4_SYSV_ABI sceUsbdGetDeviceAddress(libusb_device* dev) {
    LOG_INFO(Lib_Usbd, "called");
    if (dev == nullptr) {
        return 0;
    }
    return libusb_get_device_address(dev);
}

int PS4_SYSV_ABI sceUsbdGetDeviceDescriptor(libusb_device* dev,
                                            struct libusb_device_descriptor* config) {
    LOG_INFO(Lib_Usbd, "called");
    if (dev == nullptr || config == nullptr) {
        return SCE_USBD_ERROR_INVALID_ARG;
    }
    int err = libusb_get_device_descriptor(dev, config);
    return LibusbErrToOrbis(err);
}

int PS4_SYSV_ABI sceUsbdGetDeviceList(libusb_device*** list) {
    LOG_INFO(Lib_Usbd, "called");
    ssize_t count = UsbImplementation::Instance()->get_device_list(list);
    if (count <= 0) {
        return LibusbErrToOrbis(count);
    }
    return count;
}

int PS4_SYSV_ABI sceUsbdGetDeviceSpeed(libusb_device* dev) {
    LOG_INFO(Lib_Usbd, "called");
    return libusb_get_device_speed(dev);
}

unsigned char* PS4_SYSV_ABI sceUsbdGetIsoPacketBuffer(struct libusb_transfer* transfer,
                                                      u32 packet) {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    UNREACHABLE_MSG("unimplemented");
    return 0;
}

int PS4_SYSV_ABI sceUsbdGetMaxIsoPacketSize(libusb_device* dev, unsigned char endpoint) {
    LOG_INFO(Lib_Usbd, "called");
    if (dev == nullptr) {
        return SCE_USBD_ERROR_NO_DEVICE;
    }
    return libusb_get_max_iso_packet_size(dev, endpoint);
}

int PS4_SYSV_ABI sceUsbdGetMaxPacketSize(libusb_device* dev, unsigned char endpoint) {
    LOG_INFO(Lib_Usbd, "called");
    if (dev == nullptr) {
        return SCE_USBD_ERROR_NO_DEVICE;
    }
    return libusb_get_max_packet_size(dev, endpoint);
}

int PS4_SYSV_ABI sceUsbdGetStringDescriptor(libusb_device_handle* dev_handle, u8 desc_index,
                                            u16 lang_id, unsigned char* data, s32 length) {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdGetStringDescriptorAscii(libusb_device_handle* dev_handle, u8 desc_index,
                                                 u16 lang_id, unsigned char* data, s32 length) {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdHandleEvents(int* seconds) {
    LOG_INFO(Lib_Usbd, "redirecting to HandleEventsTimeout...");
    return sceUsbdHandleEventsTimeout(seconds);
}

int PS4_SYSV_ABI sceUsbdHandleEventsLocked() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdHandleEventsTimeout(int* seconds) {
    LOG_INFO(Lib_Usbd, "called");
    int err = UsbImplementation::Instance()->operate({*seconds, 0});
    return LibusbErrToOrbis(err);
}

int PS4_SYSV_ABI sceUsbdInit() {
    LOG_INFO(Lib_Usbd, "called");
    UsbImplementation::Instance()->initialize();
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdInterruptTransfer(libusb_device_handle* dev_handle, unsigned char endpoint,
                                          unsigned char* data, s32 length, s32* transferred,
                                          u32 timeout) {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdKernelDriverActive(libusb_device_handle* dev_handle, s32 interface_num) {
    LOG_INFO(Lib_Usbd, "called");
    if (dev_handle == nullptr) {
        return SCE_USBD_ERROR_INVALID_ARG;
    }
    if (interface_num > 256) {
        return 0;
    }
    int err = libusb_kernel_driver_active(dev_handle, interface_num);
    return (err == 1 || err == LIBUSB_ERROR_NOT_SUPPORTED);
}

void PS4_SYSV_ABI sceUsbdLockEvents() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    UNREACHABLE_MSG("unimplemented");
}

void PS4_SYSV_ABI sceUsbdLockEventWaiters() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    UNREACHABLE_MSG("unimplemented");
}

int PS4_SYSV_ABI sceUsbdOpen(libusb_device* dev, libusb_device_handle** dev_handle) {
    LOG_INFO(Lib_Usbd, "called");
    if (dev == nullptr || dev_handle == nullptr) {
        return SCE_USBD_ERROR_INVALID_ARG; //???
    }
    int err = UsbImplementation::Instance()->open_device(dev, dev_handle);
    return LibusbErrToOrbis(err);
}

libusb_device_handle* PS4_SYSV_ABI sceUsbdOpenDeviceWithVidPid(u16 vendor_id, u16 product_id) {
    LOG_INFO(Lib_Usbd, "called");

    struct libusb_device** devs;
    struct libusb_device* found = nullptr;
    struct libusb_device* dev;
    struct libusb_device_handle* dev_handle = nullptr;
    size_t i = 0;

    if (sceUsbdGetDeviceList(&devs) < 0) {
        return nullptr;
    }

    while ((dev = devs[i++]) != nullptr) {
        struct libusb_device_descriptor desc;
        int r = sceUsbdGetDeviceDescriptor(dev, &desc);
        if (r < 0) {
            return nullptr;
        }
        if (desc.idVendor == vendor_id && desc.idProduct == product_id) {
            found = dev;
            break;
        }
    }

    if (found) {
        return nullptr;
    }

    return dev_handle;
}

void PS4_SYSV_ABI sceUsbdRefDevice() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    UNREACHABLE_MSG("unimplemented");
}

int PS4_SYSV_ABI sceUsbdReleaseInterface(libusb_device_handle* dev_handle, s32 interface_num) {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdResetDevice(libusb_device_handle* dev_handle) {
    LOG_INFO(Lib_Usbd, "called");
    int err = libusb_reset_device(dev_handle);
    return LibusbErrToOrbis(err);
}

int PS4_SYSV_ABI sceUsbdSetConfiguration(libusb_device_handle* dev_handle, s32 config) {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI sceUsbdSetInterfaceAltSetting(libusb_device_handle* dev_handle, s32 interface_num,
                                               s32 alt_setting) {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

void PS4_SYSV_ABI sceUsbdSetIsoPacketLengths(struct libusb_transfer* transfer, u32 length) {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    UNREACHABLE_MSG("unimplemented");
}

int PS4_SYSV_ABI sceUsbdSubmitTransfer(struct libusb_transfer* transfer) {
    LOG_INFO(Lib_Usbd, "called");
    int err = libusb_submit_transfer(transfer);
    return LibusbErrToOrbis(err);
}

void PS4_SYSV_ABI sceUsbdTryLockEvents() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    UNREACHABLE_MSG("unimplemented");
}

void PS4_SYSV_ABI sceUsbdUnlockEvents() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    UNREACHABLE_MSG("unimplemented");
}

void PS4_SYSV_ABI sceUsbdUnlockEventWaiters() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    UNREACHABLE_MSG("unimplemented");
}

void PS4_SYSV_ABI sceUsbdUnrefDevice() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    UNREACHABLE_MSG("unimplemented");
}

void PS4_SYSV_ABI sceUsbdWaitForEvent() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    UNREACHABLE_MSG("unimplemented");
}

int PS4_SYSV_ABI Func_65F6EF33E38FFF50() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_97F056BAD90AADE7() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_C55104A33B35B264() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

int PS4_SYSV_ABI Func_D56B43060720B1E0() {
    LOG_ERROR(Lib_Usbd, "(STUBBED)called");
    return ORBIS_OK;
}

void RegisterlibSceUsbd(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("0ktE1PhzGFU", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdAllocTransfer);
    LIB_FUNCTION("BKMEGvfCPyU", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdAttachKernelDriver);
    LIB_FUNCTION("fotb7DzeHYw", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdBulkTransfer);
    LIB_FUNCTION("-KNh1VFIzlM", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdCancelTransfer);
    LIB_FUNCTION("MlW6deWfPp0", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdCheckConnected);
    LIB_FUNCTION("AE+mHBHneyk", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdClaimInterface);
    LIB_FUNCTION("3tPPMo4QRdY", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdClearHalt);
    LIB_FUNCTION("HarYYlaFGJY", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdClose);
    LIB_FUNCTION("RRKFcKQ1Ka4", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdControlTransfer);
    LIB_FUNCTION("XUWtxI31YEY", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdControlTransferGetData);
    LIB_FUNCTION("SEdQo8CFmus", "libSceUsbd", 1, "libSceUsbd", 1, 1,
                 sceUsbdControlTransferGetSetup);
    LIB_FUNCTION("Y5go+ha6eDs", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdDetachKernelDriver);
    LIB_FUNCTION("Vw8Hg1CN028", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdEventHandlerActive);
    LIB_FUNCTION("e7gp1xhu6RI", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdEventHandlingOk);
    LIB_FUNCTION("Fq6+0Fm55xU", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdExit);
    LIB_FUNCTION("oHCade-0qQ0", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdFillBulkTransfer);
    LIB_FUNCTION("8KrqbaaPkE0", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdFillControlSetup);
    LIB_FUNCTION("7VGfMerK6m0", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdFillControlTransfer);
    LIB_FUNCTION("t3J5pXxhJlI", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdFillInterruptTransfer);
    LIB_FUNCTION("xqmkjHCEOSY", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdFillIsoTransfer);
    LIB_FUNCTION("Hvd3S--n25w", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdFreeConfigDescriptor);
    LIB_FUNCTION("EQ6SCLMqzkM", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdFreeDeviceList);
    LIB_FUNCTION("-sgi7EeLSO8", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdFreeTransfer);
    LIB_FUNCTION("S1o1C6yOt5g", "libSceUsbd", 1, "libSceUsbd", 1, 1,
                 sceUsbdGetActiveConfigDescriptor);
    LIB_FUNCTION("t7WE9mb1TB8", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdGetBusNumber);
    LIB_FUNCTION("Dkm5qe8j3XE", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdGetConfigDescriptor);
    LIB_FUNCTION("GQsAVJuy8gM", "libSceUsbd", 1, "libSceUsbd", 1, 1,
                 sceUsbdGetConfigDescriptorByValue);
    LIB_FUNCTION("L7FoTZp3bZs", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdGetConfiguration);
    LIB_FUNCTION("-JBoEtvTxvA", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdGetDescriptor);
    LIB_FUNCTION("rsl9KQ-agyA", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdGetDevice);
    LIB_FUNCTION("GjlCrU4GcIY", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdGetDeviceAddress);
    LIB_FUNCTION("bhomgbiQgeo", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdGetDeviceDescriptor);
    LIB_FUNCTION("8qB9Ar4P5nc", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdGetDeviceList);
    LIB_FUNCTION("e1UWb8cWPJM", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdGetDeviceSpeed);
    LIB_FUNCTION("vokkJ0aDf54", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdGetIsoPacketBuffer);
    LIB_FUNCTION("nuIRlpbxauM", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdGetMaxIsoPacketSize);
    LIB_FUNCTION("YJ0cMAlLuxQ", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdGetMaxPacketSize);
    LIB_FUNCTION("g2oYm1DitDg", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdGetStringDescriptor);
    LIB_FUNCTION("t4gUfGsjk+g", "libSceUsbd", 1, "libSceUsbd", 1, 1,
                 sceUsbdGetStringDescriptorAscii);
    LIB_FUNCTION("EkqGLxWC-S0", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdHandleEvents);
    LIB_FUNCTION("rt-WeUGibfg", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdHandleEventsLocked);
    LIB_FUNCTION("+wU6CGuZcWk", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdHandleEventsTimeout);
    LIB_FUNCTION("TOhg7P6kTH4", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdInit);
    LIB_FUNCTION("rxi1nCOKWc8", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdInterruptTransfer);
    LIB_FUNCTION("RLf56F-WjKQ", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdKernelDriverActive);
    LIB_FUNCTION("u9yKks02-rA", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdLockEvents);
    LIB_FUNCTION("AeGaY8JrAV4", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdLockEventWaiters);
    LIB_FUNCTION("VJ6oMq-Di2U", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdOpen);
    LIB_FUNCTION("vrQXYRo1Gwk", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdOpenDeviceWithVidPid);
    LIB_FUNCTION("U1t1SoJvV-A", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdRefDevice);
    LIB_FUNCTION("REfUTmTchMw", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdReleaseInterface);
    LIB_FUNCTION("hvMn0QJXj5g", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdResetDevice);
    LIB_FUNCTION("FhU9oYrbXoA", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdSetConfiguration);
    LIB_FUNCTION("DVCQW9o+ki0", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdSetInterfaceAltSetting);
    LIB_FUNCTION("dJxro8Nzcjk", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdSetIsoPacketLengths);
    LIB_FUNCTION("L0EHgZZNVas", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdSubmitTransfer);
    LIB_FUNCTION("TcXVGc-LPbQ", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdTryLockEvents);
    LIB_FUNCTION("RA2D9rFH-Uw", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdUnlockEvents);
    LIB_FUNCTION("1DkGvUQYFKI", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdUnlockEventWaiters);
    LIB_FUNCTION("OULgIo1zAsA", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdUnrefDevice);
    LIB_FUNCTION("ys2e9VRBPrY", "libSceUsbd", 1, "libSceUsbd", 1, 1, sceUsbdWaitForEvent);
    LIB_FUNCTION("ZfbvM+OP-1A", "libSceUsbd", 1, "libSceUsbd", 1, 1, Func_65F6EF33E38FFF50);
    LIB_FUNCTION("l-BWutkKrec", "libSceUsbd", 1, "libSceUsbd", 1, 1, Func_97F056BAD90AADE7);
    LIB_FUNCTION("xVEEozs1smQ", "libSceUsbd", 1, "libSceUsbd", 1, 1, Func_C55104A33B35B264);
    LIB_FUNCTION("1WtDBgcgseA", "libSceUsbd", 1, "libSceUsbd", 1, 1, Func_D56B43060720B1E0);
};

} // namespace Libraries::Usbd
