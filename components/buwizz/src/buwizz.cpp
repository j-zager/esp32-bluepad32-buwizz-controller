#include "buwizz.h"
#include <stdio.h>

// UUIDs von String in Byte-Arrays umwandeln (BTstack braucht Little Endian)
// Service: 4e050000-74fb-4481-88b3-9919b1676e93
static const uint8_t BUWIZZ_SERVICE_UUID128[] = { 
    0x4e, 0x05, 0x00, 0x00, 0x74, 0xfb, 0x44, 0x81, 
    0x88, 0xb3, 0x99, 0x19, 0xb1, 0x67, 0x6e, 0x93 
};

// // Characteristic: 000092d1-0000-1000-8000-00805f9b34fb
static const uint8_t BUWIZZ_CHAR_UUID128[] = { 
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 
    0x00, 0x10, 0x00, 0x00, 0xd1, 0x92, 0x00, 0x00 
};

// // Characteristic: 000092d1-0000-1000-8000-00805f9b34fb
// static const uint8_t BUWIZZ_CHAR_UUID128[] = { 
//     0x00, 0x00, 0x92, 0xd1, 0x00, 0x00, 0x10, 0x00, 
//     0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb 
// };



BuWizz::BuWizz() : _con_handle(HCI_CON_HANDLE_INVALID), _connected(false), _motor_handle(0) {}

void BuWizz::init() {
    gatt_client_init();

        // Das Objekt mit dem Handler verknüpfen
    _hci_event_callback_registration.callback = &BuWizz::packetHandler;
    
    // Jetzt das Objekt (nicht nur die Funktion) registrieren
    hci_add_event_handler(&_hci_event_callback_registration);
}

void BuWizz::connect() {
    if (_connected) return;
    printf("BuWizz: Verbindungsversuch...\n");
    gap_connect(_addr, (bd_addr_type_t)0); 
}


void BuWizz::packetHandler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    if (packet_type != HCI_EVENT_PACKET) return;

    uint8_t event = hci_event_packet_get_type(packet);
    
    // 1. Verbindungsebene (GAP)
    if (event == HCI_EVENT_LE_META) {
        if (hci_event_le_meta_get_subevent_code(packet) == HCI_SUBEVENT_LE_CONNECTION_COMPLETE) {
            buwizz._con_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
            buwizz._connected = true;
            printf("BuWizz: ✔ Verbunden! Starte Service-Suche...\n");

            // JETZT: Discovery starten! Wir suchen den Haupt-Service
            gatt_client_discover_primary_services_by_uuid128(
                &BuWizz::packetHandler, buwizz._con_handle, BUWIZZ_SERVICE_UUID128);
        }
    } 
    
    // 2. GATT-Ebene (Hier kommen die Antworten auf Discovery)
    else if (event == GATT_EVENT_QUERY_COMPLETE) {
        printf("BuWizz: Discovery Schritt abgeschlossen.\n");
    }
    
    // else if (event == GATT_EVENT_SERVICE_QUERY_RESULT) {
    //     // Service gefunden! Jetzt suchen wir die Characteristic dadrin
    //     gatt_client_service_t service;
    //     gatt_event_service_query_result_get_service(packet, &service);
    //     printf("BuWizz: Service gefunden! Suche Characteristic...\n");
        
    //     gatt_client_discover_characteristics_for_service_by_uuid128(
    //         &BuWizz::packetHandler, buwizz._con_handle, &service, BUWIZZ_CHAR_UUID128);
    // }

    // else if (event == GATT_EVENT_CHARACTERISTIC_QUERY_RESULT) {
    //     // Characteristic gefunden! Handle speichern
    //     gatt_client_characteristic_t characteristic;
    //     gatt_event_characteristic_query_result_get_characteristic(packet, &characteristic);
    //     buwizz._motor_handle = characteristic.value_handle;
    //     printf("BuWizz: ✔ Motor-Handle gefunden: %04x. Bereit!\n", buwizz._motor_handle);
        
    //     // OPTIONAL: Hier jetzt sofort den Ludicrous-Mode (4) senden
    //     buwizz.setMode(2);// fast
    // }
    // else if (event == GATT_EVENT_SERVICE_QUERY_RESULT) {
    //     gatt_client_service_t service;
    //     gatt_event_service_query_result_get_service(packet, &service);
    //     printf("BuWizz: Service gefunden! Suche JETZT ALLE Characteristics...\n");
        
    //     // Wir suchen NICHT nach einer UUID, sondern lassen uns ALLE zeigen:
    //     gatt_client_discover_characteristics_for_service(
    //         &BuWizz::packetHandler, buwizz._con_handle, &service);
    // }
    // else if (event == GATT_EVENT_CHARACTERISTIC_QUERY_RESULT) {
    //     gatt_client_characteristic_t characteristic;
    //     gatt_event_characteristic_query_result_get_characteristic(packet, &characteristic);
        
    //     // Wir prüfen, ob es eine 16-Bit oder 128-Bit UUID ist
    //     if (characteristic.uuid16 != 0) {
    //         printf("BuWizz: Char gefunden! Handle: 0x%04x, UUID16: 0x%04x\n", 
    //                 characteristic.value_handle, characteristic.uuid16);
    //     } else {
    //         // Wir geben die 128-Bit UUID in lesbarer Form aus (Big Endian für das Log)
    //         printf("BuWizz: Char gefunden! Handle: 0x%04x, UUID128: ", characteristic.value_handle);
    //         for (int i = 0; i < 16; i++) {
    //             printf("%02x", characteristic.uuid128[i]);
    //         }
    //         printf("\n");
    //     }

    //     // Wenn die UUID mit "000092d1" beginnt (oder endet, je nach Byte-Order), 
    //     // haben wir unser Motor-Handle!
    //     if (characteristic.uuid128[12] == 0x92 && characteristic.uuid128[13] == 0xd1) {
    //         buwizz._motor_handle = characteristic.value_handle;
    //         printf("BuWizz: ✔ MOTOR-HANDLE IDENTIFIZIERT: 0x%04x\n", buwizz._motor_handle);
    //     }
    // }
    else if (event == GATT_EVENT_SERVICE_QUERY_RESULT) {
        gatt_client_service_t service;
        gatt_event_service_query_result_get_service(packet, &service);
        printf("BuWizz: Service gefunden! Suche Characteristic...\n");
        
        // // Suche gezielt nach der 92d1... UUID in diesem Service
        // gatt_client_discover_characteristics_for_service_by_uuid128(
        //     &BuWizz::packetHandler, buwizz._con_handle, &service, BUWIZZ_CHAR_UUID128);

        // Wir suchen NICHT nach der 92d1... UUID, sondern listen ALLES in diesem Service auf:
        gatt_client_discover_characteristics_for_service(
            &BuWizz::packetHandler, buwizz._con_handle, &service);

            
    }

    else if (event == GATT_EVENT_CHARACTERISTIC_QUERY_RESULT) {
        gatt_client_characteristic_t characteristic;
        gatt_event_characteristic_query_result_get_characteristic(packet, &characteristic);

        // Wir lassen uns das Handle und die UUID anzeigen, die der BuWizz schickt
        printf("BuWizz: Char gefunden! Handle: 0x%04x, UUID128: ", characteristic.value_handle);
        for (int i = 0; i < 16; i++) {
            printf("%02x", characteristic.uuid128[i]);
        }
        printf("\n");

        // Wir speichern das Handle einfach mal, um zu sehen ob wir es treffen
        buwizz._motor_handle = characteristic.value_handle;


        // buwizz._motor_handle = characteristic.value_handle;
        // printf("BuWizz: ✔ Motor-Handle gefunden: 0x%04x\n", buwizz._motor_handle);
        
        // // Sobald das Handle da ist: Modus setzen wie im NimBLE-Code (FAST = 2)
        // buwizz.setMode(2); 
    }


}

// Hilfsfunktion für den Modus
void BuWizz::setMode(uint8_t mode) {
    if (!_connected || _motor_handle == 0) return;
    // Beim BuWizz 2 ist der Modus oft Teil desselben Handles oder ein Byte-Befehl
    uint8_t mode_cmd[] = { 0x11, mode }; // Beispiel: 0x11 ist oft das Mode-Register
    gatt_client_write_value_of_characteristic_without_response(_con_handle, _motor_handle, 2, mode_cmd);
    printf("BuWizz: Modus %d gesetzt.\n", mode);
}

void BuWizz::setMotors(int8_t m1, int8_t m2, int8_t m3, int8_t m4) {
    if (!_connected || _con_handle == HCI_CON_HANDLE_INVALID || _motor_handle == 0) return;
    uint8_t data[] = {(uint8_t)m1, (uint8_t)m2, (uint8_t)m3, (uint8_t)m4};
    gatt_client_write_value_of_characteristic_without_response(_con_handle, _motor_handle, 4, data);
}

// void BuWizz::setMode(uint8_t level) {
//     if (!_connected || _motor_handle == 0) return;
//     uint8_t cmd[2] = { 0x11, level };
//     gatt_client_write_value_of_characteristic_without_response(
//         _con_handle, _motor_handle, 2, cmd);
// }

// void BuWizz::setMotors(int8_t m1, int8_t m2, int8_t m3, int8_t m4) {
//     if (!_connected || _motor_handle == 0) return;
    
//     uint8_t payload[6] = {
//         0x10, // Command: Set motor data
//         (uint8_t)m1, (uint8_t)m2, (uint8_t)m3, (uint8_t)m4,
//         0x00  // BrakeMask (0 = kein Bremsen)
//     };
    
//     gatt_client_write_value_of_characteristic_without_response(
//         _con_handle, _motor_handle, 6, payload);
// }



void BuWizz::process() {
    // Hier kannst du später Logik einbauen, die in jedem Loop-Durchlauf laufen soll
}
