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



BuWizz::BuWizz() : _con_handle(HCI_CON_HANDLE_INVALID), _connected(false), _motor_handle(0x0003) {}

void BuWizz::init() {
    gatt_client_init();
    // NEU: Dem Stack sagen, dass wir Pairing unterstützen (No Input No Output)
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(SM_AUTHREQ_BONDING);

        // Das Objekt mit dem Handler verknüpfen
    _hci_event_callback_registration.callback = &BuWizz::packetHandler;
    
    // Jetzt das Objekt (nicht nur die Funktion) registrieren
    hci_add_event_handler(&_hci_event_callback_registration);
}

void BuWizz::connect() {
    if (_connected) return;
    printf("BuWizz: Verbindungsversuch...\n");
    //gap_connect(_addr, (bd_addr_type_t)0);  
    uni_bt_le_scan_start();
}

// void BuWizz::packetHandler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
//     if (packet_type != HCI_EVENT_PACKET) return;

//     // TEST-PRINT: Wenn das nicht erscheint, lässt Bluepad32 dich nicht durch!
//     if (packet_type == HCI_EVENT_PACKET) {
//           printf("DEBUG: HCI Event %02x empfangen\n", hci_event_packet_get_type(packet));
//     }

//     uint8_t event = hci_event_packet_get_type(packet);

//     switch (event) {
//         // case HCI_EVENT_LE_META: {
//         //     uint8_t subevent = hci_event_le_meta_get_subevent_code(packet);
            
//         //     // A) Falls wir ein Gerät im Scan finden (0x02 = Advertising Report)
//         //     if (subevent == HCI_SUBEVENT_LE_ADVERTISING_REPORT) {
//         //         // Adresse auslesen: Im LE Meta Event Typ 0x02 liegt die Adresse ab Byte 6
//         //         // Byte 5 ist der Adress-Typ (00 = Public, 01 = Random)
//         //         uint8_t addr_type = packet[5];

//         //         // Wir lesen die Adresse ab Byte 5
//         //         printf("Scan-Paket gefunden! Adresse: %02x:%02x:%02x:%02x:%02x:%02x\n", 
//         //                 packet[6], packet[7], packet[8], packet[9], packet[10], packet[11]);
                
//         //         // Test: Vergleiche nur die ersten 3 Bytes deiner BuWizz-MAC (50:FA:AB)
//         //         if (packet[6] == 0x4C && packet[7] == 0x03 && packet[8] == 0x6D) { // Probier es mal "rückwärts"
//         //             printf("BuWizz: Treffer (Rückwärts-Check)!\n");
//         //             printf("BuWizz: GEFUNDEN! Verbinde mit Typ %d...\n", addr_type);
        
//         //             uni_bt_le_scan_stop();
                    
//         //             // WICHTIG: Wir nutzen den gefundenen addr_type (0 oder 1)
//         //             bd_addr_t found_addr;
//         //             //memcpy(found_addr, &packet[6], 6);

//         //             // Wir drehen die 6 Bytes beim Kopieren von Little Endian zu Big Endian
//         //             for (int i = 0; i < 6; i++) {
//         //                 found_addr[i] = packet[11 - i]; 
//         //             }

//         //             printf("Sende Connect an: %02x:%02x:%02x:%02x:%02x:%02x\n", 
//         //                     found_addr[0], found_addr[1], found_addr[2], 
//         //                     found_addr[3], found_addr[4], found_addr[5]);
//         //             gap_connect(found_addr, (bd_addr_type_t)addr_type);
//         //         }
//         //     }
            
//         //     // B) Falls die Verbindung bereits hergestellt wurde (0x01 oder 0x0a)
//         //     else if (subevent == HCI_SUBEVENT_LE_CONNECTION_COMPLETE || subevent == 0x0a) {

//         //         uint8_t status = packet[3]; 
//         //         if (status == 0) {
//         //             // Wir lesen das Handle von Position 14 (aus deinem Dump: 18 00)
//         //             uint16_t handle = little_endian_read_16(packet, 14);
                    
//         //             // Falls 14 auch 0 ist, probieren wir zur Sicherheit 4
//         //             // if (handle == 0) handle = little_endian_read_16(packet, 4);

//         //             if (handle != 0) {
//         //                 buwizz._con_handle = handle;
//         //                 buwizz._connected = true;
//         //                 printf("BuWizz: ✔ ECHTES HANDLE GEFUNDEN: 0x%04x\n", buwizz._con_handle);

//         //                 // // 1. Suche ALLE Services (ohne UUID-Filter, um zu sehen, was da ist)
//         //                 // gatt_client_discover_primary_services(&BuWizz::packetHandler, buwizz._con_handle);
//         //                     // WICHTIG: MTU Exchange anfordern. In manchen BTstack-Versionen heißt es:
//         //                 gatt_client_send_mtu_negotiation(&BuWizz::packetHandler, buwizz._con_handle);
                        
//         //                 // // Jetzt die Service-Suche starten
//         //                 // gatt_client_discover_primary_services_by_uuid128(
//         //                 //     &BuWizz::packetHandler, buwizz._con_handle, BUWIZZ_SERVICE_UUID128);
//         //             }
//         //         }
//         //      }
    
//         // } break;

//         case ATT_EVENT_MTU_EXCHANGE_COMPLETE:
//             printf("BuWizz: MTU Exchange abgeschlossen! Starte jetzt Service-Suche...\n");
//             // Jetzt ist die Leitung "bereit" für GATT-Anfragen
//             gatt_client_discover_primary_services(&BuWizz::packetHandler, buwizz._con_handle);
//             break;

//         // // --- GATT EBENE: MTU FERTIG ---
//         // case GATT_EVENT_MTU:
//         //     printf("BuWizz: MTU verhandelt auf %d. Suche Service...\n", gatt_event_mtu_get_MTU(packet));
//         //     // Schritt 2: Service suchen
//         //     gatt_client_discover_primary_services_by_uuid128(
//         //         &BuWizz::packetHandler, buwizz._con_handle, BUWIZZ_SERVICE_UUID128);
//         //     break;

//         // --- GATT EBENE: SERVICE GEFUNDEN ---
//         // case GATT_EVENT_SERVICE_QUERY_RESULT: {
//         //     gatt_client_service_t service;
//         //     gatt_event_service_query_result_get_service(packet, &service);
//         //     printf("BuWizz: Service gefunden! Bereich: 0x%04x-0x%04x. Suche ALLE Chars...\n", 
//         //             service.start_group_handle, service.end_group_handle);
            
//         //     // Schritt 3: ALLE Characteristics in diesem Service auflisten
//         //     gatt_client_discover_characteristics_for_service(
//         //         &BuWizz::packetHandler, buwizz._con_handle, &service);
//         // } break;

//         case GATT_EVENT_SERVICE_QUERY_RESULT: {
//             // Hier kommen die Antworten vom BuWizz nacheinander an!
//             gatt_client_service_t service;
//             gatt_event_service_query_result_get_service(packet, &service);
            
//             // JETZT PRINTEN WIR:
//             printf("BuWizz: Service gefunden! UUID128: ");
//             for (int i = 0; i < 16; i++) {
//                 // Wir nutzen den UUID-Speicher aus der gefundenen Service-Struktur
//                 // (In manchen BTstack Versionen muss man hier service.uuid128 nutzen)
//                 printf("%02x", packet[i+8]); // Offset 8 ist oft der Start der UUID im Event-Paket
//             }
//             printf(" | Handles: 0x%04x - 0x%04x\n", service.start_group_handle, service.end_group_handle);
//             } break;



//         // --- GATT EBENE: CHARACTERISTIC GEFUNDEN ---
//         case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT: {
//             gatt_client_characteristic_t characteristic;
//             gatt_event_characteristic_query_result_get_characteristic(packet, &characteristic);
            
//             // Wir loggen jedes Handle und die UUID (für den Vergleich mit 92d1...)
//             printf("BuWizz: Char gefunden! Handle: 0x%04x, UUID: ", characteristic.value_handle);
//             for (int i = 0; i < 16; i++) printf("%02x", characteristic.uuid128[i]);
//             printf("\n");

//             // Automatischer Check auf die Motor-UUID (92d1)
//             // Je nach Endianness liegt 92 d1 an Index 2/3 oder 12/13
//             if ((characteristic.uuid128[2] == 0x92 && characteristic.uuid128[3] == 0xd1) ||
//                 (characteristic.uuid128[12] == 0x92 && characteristic.uuid128[13] == 0xd1)) {
//                 buwizz._motor_handle = characteristic.value_handle;
//                 printf("BuWizz: ⭐ MOTOR-HANDLE IDENTIFIZIERT: 0x%04x\n", buwizz._motor_handle);
//             }
//         } break;

//         // --- GATT EBENE: SCHRITT ABGESCHLOSSEN ---
//         case GATT_EVENT_QUERY_COMPLETE:
//             printf("BuWizz: Discovery Schritt abgeschlossen.\n");
//             break;

//         case HCI_EVENT_DISCONNECTION_COMPLETE: {
//             uint8_t reason = packet[5];
//             printf("BuWizz: Verbindung verloren! Grund-Code: 0x%02x\n", reason);
//             // Grund 0x13: Remote User Terminated (BuWizz hat dich rausgeworfen)
//             // Grund 0x22: LMP Response Timeout (Funkstörung/Protokollfehler)
//             buwizz._connected = false;
//             } break;

//         default:
//             break;
//     }
// }

void BuWizz::packetHandler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    if (packet_type != HCI_EVENT_PACKET) return;
    uint8_t event = hci_event_packet_get_type(packet);

    switch (event) {
        case HCI_EVENT_LE_META: {
            uint8_t subevent = hci_event_le_meta_get_subevent_code(packet);
            
            // SCHRITT 1: BuWizz im Scan finden
            if (subevent == HCI_SUBEVENT_LE_ADVERTISING_REPORT) { // Advertising Report
                bd_addr_t found_addr;
                // MAC Adresse liegt ab Byte 6 (rückwärts)
                for (int i = 0; i < 6; i++) found_addr[i] = packet[11 - i];
                uint8_t addr_type = packet[5];

                if (packet[6] == 0x4c && packet[7] == 0x03) { // 4C:03... Treffer!
                    printf("BuWizz: Gefunden! Verbinde...\n");
                    uni_bt_le_scan_stop();
                    gap_connect(found_addr, (bd_addr_type_t)addr_type);
                }
            }
            // SCHRITT 2: Verbindung erfolgreich
            else if (subevent == HCI_SUBEVENT_LE_CONNECTION_COMPLETE || subevent == HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE_V1) {
                if (packet[3] == 0) { // Status OK
                    // buwizz._con_handle = little_endian_read_16(packet, 14); // Deine 0x0018 Position
                    buwizz._con_handle = little_endian_read_16(packet, 4); // Deine 0x0000 Position
                    buwizz._connected = true;
                    // DAS ERGEBNIS AUS DEM DATENBLATT:
                     buwizz._motor_handle = 0x0003; 
                    //buwizz._motor_handle = 0x0004;
                    printf("BuWizz: ✔ KONSTANT GRÜN! Handle: 0x%04x\n", buwizz._con_handle);
                    // SCHRITT A: Benachrichtigungen aktivieren (Handle 0x0005 laut Datenblatt)
                    // // 0x0002 bedeutet "Indications einschalten"
                    // uint16_t enable_indicate = 0x0002; 
                    // gatt_client_write_value_of_characteristic_without_response(
                    //     buwizz._con_handle, 0x0005, 2, (uint8_t*)&enable_indicate);
                            // EINMALIG: Subscription / Notify aktivieren (Handle 0x0005)
                    uint16_t enable_notify = 0x0001; 
                    uint8_t err = gatt_client_write_value_of_characteristic_without_response(
                        buwizz._con_handle, 0x0005, 2, (uint8_t*)&enable_notify);
                    printf("Stack-Status CCCD (0x05): %d\n", err);
                    
                }
            }
        } break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            buwizz._connected = false;
            printf("BuWizz: Verbindung verloren.\n");
            break;
    }
}



void BuWizz::setMode(uint8_t level) {
    if (!_connected) return;
    uint8_t cmd[] = { 0x11, level };
    printf("BuWizz: set Mode:%d.\n",level);
    // Wir schreiben direkt auf das vermutete Handle 0x001b
    uint8_t err = gatt_client_write_value_of_characteristic_without_response(_con_handle, _motor_handle, 2, cmd);
    printf("Stack-Status Mode (0x03): %d\n", err);
    // gatt_client_write_value_of_characteristic(_con_handle, _motor_handle, 2, cmd); 
}

void BuWizz::setMotors(int8_t m1, int8_t m2, int8_t m3, int8_t m4) {
    if (!_connected) return;
    uint8_t payload[] = { 0x10, (uint8_t)m1, (uint8_t)m2, (uint8_t)m3, (uint8_t)m4, 0x00 };
    printf("BuWizz: setMotors:cmd %d,m0:%d,m1:%d,m2:%d,m3:%d.\n",payload[0],payload[1],payload[2],payload[3],payload[4]);
    gatt_client_write_value_of_characteristic_without_response(_con_handle, _motor_handle, 6, payload);
}
// // Hilfsfunktion für den Modus
// void BuWizz::setMode(uint8_t mode) {
//     if (!_connected || _motor_handle == 0) return;
//     // Beim BuWizz 2 ist der Modus oft Teil desselben Handles oder ein Byte-Befehl
//     uint8_t mode_cmd[] = { 0x11, mode }; // Beispiel: 0x11 ist oft das Mode-Register
//     gatt_client_write_value_of_characteristic_without_response(_con_handle, _motor_handle, 2, mode_cmd);
//     printf("BuWizz: Modus %d gesetzt.\n", mode);
// }

// void BuWizz::setMotors(int8_t m1, int8_t m2, int8_t m3, int8_t m4) {
//     if (!_connected || _con_handle == HCI_CON_HANDLE_INVALID || _motor_handle == 0) return;
//     uint8_t data[] = {(uint8_t)m1, (uint8_t)m2, (uint8_t)m3, (uint8_t)m4};
//     gatt_client_write_value_of_characteristic_without_response(_con_handle, _motor_handle, 4, data);
// }

// // void BuWizz::setMode(uint8_t level) {
// //     if (!_connected || _motor_handle == 0) return;
// //     uint8_t cmd[2] = { 0x11, level };
// //     gatt_client_write_value_of_characteristic_without_response(
// //         _con_handle, _motor_handle, 2, cmd);
// // }

// // void BuWizz::setMotors(int8_t m1, int8_t m2, int8_t m3, int8_t m4) {
// //     if (!_connected || _motor_handle == 0) return;
    
// //     uint8_t payload[6] = {
// //         0x10, // Command: Set motor data
// //         (uint8_t)m1, (uint8_t)m2, (uint8_t)m3, (uint8_t)m4,
// //         0x00  // BrakeMask (0 = kein Bremsen)
// //     };
    
// //     gatt_client_write_value_of_characteristic_without_response(
// //         _con_handle, _motor_handle, 6, payload);
// // }



void BuWizz::process() {
    // Hier kannst du später Logik einbauen, die in jedem Loop-Durchlauf laufen soll
}
