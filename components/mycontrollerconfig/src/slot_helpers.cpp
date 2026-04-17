#include "slot_helpers.h"

// Zuordnung: welcher Slot gehört welchem Device
uni_hid_device_t* deviceForSlot[4] = { nullptr };

int findFreeSlot() {
    for (int i = 0; i < 4; i++) {
        if (deviceForSlot[i] == nullptr)
            return i;
    }
    return -1;
}

int findSlotForDevice(uni_hid_device_t* d) {
    for (int i = 0; i < 4; i++) {
        if (deviceForSlot[i] == d)
            return i;
    }
    return -1;
}