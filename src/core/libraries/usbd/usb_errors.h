// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/libraries/error_codes.h"

// Pad library
constexpr int SCE_USBD_ERROR_IO = 0x80240001;
constexpr int SCE_USBD_ERROR_INVALID_ARG = 0x80240002;
constexpr int SCE_USBD_ERROR_ACCESS = 0x80240003;
constexpr int SCE_USBD_ERROR_NO_DEVICE = 0x80240004;
constexpr int SCE_USBD_ERROR_NOT_FOUND = 0x80240005;
constexpr int SCE_USBD_ERROR_BUSY = 0x80240006;
constexpr int SCE_USBD_ERROR_TIMEOUT = 0x80240007;
constexpr int SCE_USBD_ERROR_OVERFLOW = 0x80240008;
constexpr int SCE_USBD_ERROR_PIPE = 0x80240009;
constexpr int SCE_USBD_ERROR_INTERRUPTED = 0x8024000A;
constexpr int SCE_USBD_ERROR_NO_MEM = 0x8024000B;
constexpr int SCE_USBD_ERROR_NOT_SUPPORTED = 0x8024000C;
constexpr int SCE_USBD_ERROR_FATAL = 0x802400FF;
