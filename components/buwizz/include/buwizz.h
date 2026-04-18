#ifndef BUWIZZ_H
#define BUWIZZ_H

#include <stdint.h>

extern "C" {
    #include "btstack.h"
    #include "gatt_client.h"
    #include "bt/uni_bt_le.h"
}

class BuWizz {
public:
    BuWizz();
    void init();
    void connect();
    void process();
    void setMotors(int8_t m1, int8_t m2, int8_t m3, int8_t m4);
    void setMode(uint8_t mode);
    void requestBattery();
    
    bool isConnected() const { return _connected; }

    // Der Handler muss statisch sein für BTstack
    static void packetHandler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

private:
        // Dieses Objekt muss dauerhaft im RAM bleiben (nicht lokal in init!)
    btstack_packet_callback_registration_t _hci_event_callback_registration;

    hci_con_handle_t _con_handle;
    bd_addr_t _addr = {0x50, 0xFA, 0xAB, 0x6D, 0x03, 0x4C};
    bool _connected;
    uint16_t _motor_handle; 

    gatt_client_service_t buwizz_service;
    bool service_found;

    uint8_t _motor_payload[6]; // Hier ist der Speicher sicher
    uint8_t _mode_payload[2];
};

// Das macht das Objekt für alle Dateien (platform.cpp, main.cpp etc.) sichtbar
extern BuWizz buwizz; 

#endif
