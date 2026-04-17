#pragma once

extern "C" {
#include "uni.h"
}

extern uni_hid_device_t* deviceForSlot[4];

int findFreeSlot();
int findSlotForDevice(uni_hid_device_t* d);
