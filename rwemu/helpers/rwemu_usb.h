#pragma once

#include <storage/storage.h>
#include "rwemu_scsi.h"

typedef struct RWEmuUsb RWEmuUsb;
typedef void (*RWEmuUsbConnectionStatusCallback)(bool connected, void* context);

RWEmuUsb* rwemu_usb_start(const char* filename, SCSIDeviceFunc fn);
void rwemu_usb_stop(RWEmuUsb* mass);
void rwemu_usb_set_connection_status_callback(
    RWEmuUsb* mass,
    RWEmuUsbConnectionStatusCallback cb,
    void* context);
