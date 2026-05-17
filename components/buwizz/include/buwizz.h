#ifndef BUWIZZ_H
#define BUWIZZ_H

#include <stdint.h>

extern "C" {
    #include "btstack.h"
    #include "gatt_client.h"
    #include "bt/uni_bt_le.h"
    #include "esp_timer.h"
}

class BuWizz {
public:
    BuWizz(const uint8_t* addr);
    void init();
    void connect();
    void process();
    void setMotors(int8_t m1, int8_t m2, int8_t m3, int8_t m4);
    void useMotors();
    void setMode(uint8_t mode);
    void requestBattery();
    // void update(uint64_t now);
    void triggerConnect(uint64_t now);
    
    bool isConnected();
    bool wasConnected();
    bool isConnectTriggered(); 
    bool isCharFound();
    uint16_t getMotorHandle();
    hci_con_handle_t getConnectHandle();
    bool isReady();
    uint8_t getAddr(int id);
    void saveMotor(int8_t m, int8_t idx);
    uint8_t getMotor( int8_t idx);
    void saveMotorAll(int8_t m1,int8_t m2,int8_t m3,int8_t m4,int8_t brakeMask);
    uint8_t* getMotorAll();
    // Der Handler muss statisch sein für BTstack
    static void packetHandler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

private:
        // Dieses Objekt muss dauerhaft im RAM bleiben (nicht lokal in init!)
    btstack_packet_callback_registration_t _hci_event_callback_registration;

    hci_con_handle_t _con_handle = HCI_CON_HANDLE_INVALID;
    // bd_addr_t _addr = {0x50, 0xFA, 0xAB, 0x6D, 0x03, 0x4C};
    bd_addr_t _addr;
    bool _connected = false;
    bool _was_connected = false;        // nutzen zum reconnect nach Stein schonmal erfolgreich gefunden wurde
    uint16_t _motor_handle = 0; 

    gatt_client_service_t buwizz_service;
    bool service_found;

    uint8_t _motor_payload[6]; // Hier ist der Speicher sicher
    uint8_t _mode_payload[2];

        // Neue Variablen für die State-Machine:
    uint64_t _init_time = 0;           // Wann wurde das Programm gestartet
    uint64_t _connected_at = 0;        // Wann wurde die BT-Verbindung stabil
    uint64_t _last_battery_request = 0;// Wann wurde zuletzt der Akku abgefragt
    uint64_t _start_connecting_time = 0;// Wann wurde die connect Prozess gestartet
    uint64_t _force_gap_connect = 0;    // Timer um ein gap connect zu forcieren, wenn ein Stein zu lnage blinkt  
    
    bool _connect_triggered = false;   // Wurde connect() schon einmal gerufen Wirkt als Sperre für gap_connect
    bool _char_found =false;            // Wurde charcteristic gefunden, letzte step bevor verbindung komplett erfolgreich ist
    bool _mode_set = false;            // Wurde der Speed-Mode schon gesetzt
    bool _test_drive_done = false;     // Wurde der Motor-Test abgeschlossen
    uint32_t _mode_counter = 0;
};

// Das macht das Objekt für alle Dateien (platform.cpp, main.cpp etc.) sichtbar
// extern BuWizz buwizz; 
extern BuWizz* mulBuWizz[];
extern const int NUM_BRICKS;

#endif
