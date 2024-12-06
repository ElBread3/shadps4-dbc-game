// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"
#include "libusb.h"

namespace Core::Loader {
class SymbolsResolver;
}
namespace Libraries::Usbd {

libusb_transfer* PS4_SYSV_ABI sceUsbdAllocTransfer(s32 iso_packets);
int PS4_SYSV_ABI sceUsbdAttachKernelDriver(libusb_device_handle* dev_handle, s32 interface_num);
int PS4_SYSV_ABI sceUsbdBulkTransfer(libusb_device_handle* dev_handle, unsigned char endpoint,
                                     unsigned char* data, s32 length, s32* transferred,
                                     u32 timeout);
int PS4_SYSV_ABI sceUsbdCancelTransfer(struct libusb_transfer* transfer);
int PS4_SYSV_ABI sceUsbdCheckConnected(libusb_device_handle* dev_handle);
int PS4_SYSV_ABI sceUsbdClaimInterface(libusb_device_handle* dev_handle, s32 interface_num);
void PS4_SYSV_ABI sceUsbdClearHalt();
void PS4_SYSV_ABI sceUsbdClose(libusb_device_handle* dev_handle);
int PS4_SYSV_ABI sceUsbdControlTransfer(libusb_device_handle* dev_handle, u8 request_type,
                                        u8 request, u16 value, u16 index, unsigned char* data,
                                        u16 length, u32 timeout);
unsigned char* PS4_SYSV_ABI sceUsbdControlTransferGetData(struct libusb_transfer* transfer);
struct libusb_control_setup* PS4_SYSV_ABI
sceUsbdControlTransferGetSetup(struct libusb_transfer* transfer);
int PS4_SYSV_ABI sceUsbdDetachKernelDriver(libusb_device_handle* dev_handle, s32 interface_num);
void PS4_SYSV_ABI sceUsbdEventHandlerActive();
void PS4_SYSV_ABI sceUsbdEventHandlingOk();
int PS4_SYSV_ABI sceUsbdExit();
void PS4_SYSV_ABI sceUsbdFillBulkTransfer(struct libusb_transfer* transfer,
                                          libusb_device_handle* dev_handle, unsigned char endpoint,
                                          unsigned char* buffer, u32 length,
                                          libusb_transfer_cb_fn callback, void* user_data,
                                          u32 timeout);
void PS4_SYSV_ABI sceUsbdFillControlSetup(unsigned char* buffer, u8 request_type, u8 request,
                                          u16 value, u16 index, u16 length);
void PS4_SYSV_ABI sceUsbdFillControlTransfer(struct libusb_transfer* transfer,
                                             libusb_device_handle* dev_handle,
                                             unsigned char* buffer, libusb_transfer_cb_fn callback,
                                             void* user_data, u32 timeout);
void PS4_SYSV_ABI sceUsbdFillInterruptTransfer(struct libusb_transfer* transfer,
                                               libusb_device_handle* dev_handle,
                                               unsigned char endpoint, unsigned char* buffer,
                                               s32 length, libusb_transfer_cb_fn callback,
                                               void* user_data, u32 timeout);
void PS4_SYSV_ABI sceUsbdFillIsoTransfer(struct libusb_transfer* transfer,
                                         libusb_device_handle* dev_handle, unsigned char endpoint,
                                         unsigned char* buffer, s32 length,
                                         libusb_transfer_cb_fn callback, void* user_data,
                                         u32 timeout);
void PS4_SYSV_ABI sceUsbdFreeConfigDescriptor(struct libusb_config_descriptor* config);
void PS4_SYSV_ABI sceUsbdFreeDeviceList(libusb_device** list);
void PS4_SYSV_ABI sceUsbdFreeTransfer(struct libusb_transfer* transfer);
int PS4_SYSV_ABI sceUsbdGetActiveConfigDescriptor(libusb_device* dev,
                                                  struct libusb_config_descriptor** config);
int PS4_SYSV_ABI sceUsbdGetBusNumber(libusb_device* dev);
int PS4_SYSV_ABI sceUsbdGetConfigDescriptor(libusb_device* dev, u8 config_index,
                                            struct libusb_config_descriptor** config);
int PS4_SYSV_ABI sceUsbdGetConfigDescriptorByValue(libusb_device* dev, u8 config_value,
                                                   struct libusb_config_descriptor** config);
int PS4_SYSV_ABI sceUsbdGetConfiguration(libusb_device_handle* dev_handle, s32* config);
int PS4_SYSV_ABI sceUsbdGetDescriptor(libusb_device_handle* dev_handle, u8 desc_type, u8 desc_index,
                                      unsigned char* data, s32 length);
libusb_device* PS4_SYSV_ABI sceUsbdGetDevice();
int PS4_SYSV_ABI sceUsbdGetDeviceAddress(libusb_device* dev);
int PS4_SYSV_ABI sceUsbdGetDeviceDescriptor(libusb_device* dev,
                                            struct libusb_device_descriptor* config);
int PS4_SYSV_ABI sceUsbdGetDeviceList(libusb_device*** list);
int PS4_SYSV_ABI sceUsbdGetDeviceSpeed(libusb_device* dev);
unsigned char* PS4_SYSV_ABI sceUsbdGetIsoPacketBuffer(struct libusb_transfer* transfer, u32 packet);
int PS4_SYSV_ABI sceUsbdGetMaxIsoPacketSize(libusb_device* dev, unsigned char endpoint);
int PS4_SYSV_ABI sceUsbdGetMaxPacketSize(libusb_device* dev, unsigned char endpoint);
int PS4_SYSV_ABI sceUsbdGetStringDescriptor(libusb_device_handle* dev_handle, u8 desc_index,
                                            u16 lang_id, unsigned char* data, s32 length);
int PS4_SYSV_ABI sceUsbdGetStringDescriptorAscii(libusb_device_handle* dev_handle, u8 desc_index,
                                                 u16 lang_id, unsigned char* data, s32 length);
int PS4_SYSV_ABI sceUsbdHandleEvents();
int PS4_SYSV_ABI sceUsbdHandleEventsLocked();
int PS4_SYSV_ABI sceUsbdHandleEventsTimeout(int* time_value);
int PS4_SYSV_ABI sceUsbdInit();
int PS4_SYSV_ABI sceUsbdInterruptTransfer(libusb_device_handle* dev_handle, unsigned char endpoint,
                                          unsigned char* data, s32 length, s32* transferred,
                                          u32 timeout);
int PS4_SYSV_ABI sceUsbdKernelDriverActive(libusb_device_handle* dev_handle, s32 interface_num);
void PS4_SYSV_ABI sceUsbdLockEvents();
void PS4_SYSV_ABI sceUsbdLockEventWaiters();
int PS4_SYSV_ABI sceUsbdOpen(libusb_device* dev, libusb_device_handle** dev_handle);
libusb_device_handle* PS4_SYSV_ABI sceUsbdOpenDeviceWithVidPid(u16 vendor_id, u16 product_id);
void PS4_SYSV_ABI sceUsbdRefDevice();
int PS4_SYSV_ABI sceUsbdReleaseInterface(libusb_device_handle* dev_handle, s32 interface_num);
int PS4_SYSV_ABI sceUsbdResetDevice(libusb_device_handle* dev_handle);
int PS4_SYSV_ABI sceUsbdSetConfiguration(libusb_device_handle* dev_handle, s32 config);
int PS4_SYSV_ABI sceUsbdSetInterfaceAltSetting(libusb_device_handle* dev_handle, s32 interface_num,
                                               s32 alt_setting);
void PS4_SYSV_ABI sceUsbdSetIsoPacketLengths(struct libusb_transfer* transfer, u32 length);
int PS4_SYSV_ABI sceUsbdSubmitTransfer(struct libusb_transfer* transfer);
void PS4_SYSV_ABI sceUsbdTryLockEvents();
void PS4_SYSV_ABI sceUsbdUnlockEvents();
void PS4_SYSV_ABI sceUsbdUnlockEventWaiters();
void PS4_SYSV_ABI sceUsbdUnrefDevice();
void PS4_SYSV_ABI sceUsbdWaitForEvent();
int PS4_SYSV_ABI Func_65F6EF33E38FFF50();
int PS4_SYSV_ABI Func_97F056BAD90AADE7();
int PS4_SYSV_ABI Func_C55104A33B35B264();
int PS4_SYSV_ABI Func_D56B43060720B1E0();

void RegisterlibSceUsbd(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Usbd