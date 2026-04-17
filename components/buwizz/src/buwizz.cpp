#include "buwizz.h"
#include <stdio.h>

// UUIDs von String in Byte-Arrays umwandeln (BTstack braucht Little Endian)
// Service: 4e050000-74fb-4481-88b3-9919b1676e93
static const uint8_t BUWIZZ_SERVICE_UUID128[] = { 
    0x4e, 0x05, 0x00, 0x00, 0x74, 0xfb, 0x44, 0x81, 
    0x88, 0xb3, 0x99, 0x19, 0xb1, 0x67, 0x6e, 0x93 
};

// // // Characteristic: 000092d1-0000-1000-8000-00805f9b34fb
// static const uint8_t BUWIZZ_CHAR_UUID128[] = { 
//     0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 
//     0x00, 0x10, 0x00, 0x00, 0xd1, 0x92, 0x00, 0x00 
// };

// Characteristic: 000092d1-0000-1000-8000-00805f9b34fb
static const uint8_t BUWIZZ_CHAR_UUID128[] = { 
    0x00, 0x00, 0x92, 0xd1, 0x00, 0x00, 0x10, 0x00, 
    0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb 
};



BuWizz::BuWizz() : 
    _con_handle(HCI_CON_HANDLE_INVALID), 
    _connected(false), 
    _motor_handle(0x0000), // Auf 0 setzen, damit Discovery beweisbar ist //_motor_handle(0x0003),
    service_found(false) 
    {}

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
                    printf("BuWizz: ✔ KONSTANT GRÜN! Handle: 0x%04x\n", buwizz._con_handle);

                     printf("BuWizz: ✔ Verbunden!-KONSTANT GRÜN! Handle: 0x%04x. Starte MTU-Verhandlung...\n", buwizz._con_handle);
                    
                    // Sofortige Service-Suche (UUID128 vorwärts, wie es bei dir klappte)
                    gatt_client_discover_primary_services_by_uuid128(
                        &BuWizz::packetHandler, buwizz._con_handle, BUWIZZ_SERVICE_UUID128);


                    //----------------------------------------------------
                    // DAS ERGEBNIS AUS DEM DATENBLATT:
                    //  buwizz._motor_handle = 0x0003; 

                    // uint16_t enable_notify = 0x0001; 
                    // uint8_t err = gatt_client_write_value_of_characteristic_without_response(
                    //     buwizz._con_handle, 0x0005, 2, (uint8_t*)&enable_notify);
                    // printf("Stack-Status CCCD (0x05): %d\n", err);
                    //-----------------------------------------------------
                    
                }
            }
        } break;


        case GATT_EVENT_SERVICE_QUERY_RESULT: {
            // Wir speichern den Service nur zwischen!
            gatt_event_service_query_result_get_service(packet, &buwizz.buwizz_service);
            buwizz.service_found = true;
            printf("BuWizz: Service in Reichweite (0x%04x-0x%04x).\n", 
            buwizz.buwizz_service.start_group_handle, buwizz.buwizz_service.end_group_handle);
            // // gatt_client_service_t service;
            // static gatt_client_service_t service;
            // gatt_event_service_query_result_get_service(packet, &service);
            // // printf("BuWizz: Service gefunden! Suche jetzt Characteristic...\n");

            // // // SCHRITT 2: In diesem Service die Motor-Characteristic suchen
            // // gatt_client_discover_characteristics_for_service_by_uuid128(
            // //     &BuWizz::packetHandler, buwizz._con_handle, &service, BUWIZZ_CHAR_UUID128);


            // // printf("BuWizz: Service gefunden (Bereich 0x%04x-0x%04x). Suche ALLE Characteristics...\n", 
            // // service.start_group_handle, service.end_group_handle);


            // // printf("BuWizz: Service gefunden! Suche Characteristic 0x92D1...\n");

            // // // Wir suchen jetzt nach der 16-Bit UUID aus dem Datenblatt
            // // gatt_client_discover_characteristics_for_service_by_uuid16(
            // //     &BuWizz::packetHandler, buwizz._con_handle, &service, 0x92D1);

            // printf("BuWizz: Service gefunden (0x%04x-0x%04x).\n", 
            // service.start_group_handle, service.end_group_handle);
            
            // // Wir starten die Char-Suche JETZT (Brute-Force: Liste ALLES auf)
            // gatt_client_discover_characteristics_for_service(
            //     &BuWizz::packetHandler, buwizz._con_handle, &service);

        } break;

        // 4. EINE SUCHE IST ABGESCHLOSSEN (Die Brücke)
        case GATT_EVENT_QUERY_COMPLETE: {
            if (buwizz._connected && buwizz.service_found && buwizz._motor_handle == 0) {
                printf("BuWizz: Service-Suche fertig. Suche jetzt Characteristics...\n");
                // SCHRITT B: Suche Characteristics (16-Bit UUID aus Datenblatt)
                gatt_client_discover_characteristics_for_service_by_uuid16(
                    &BuWizz::packetHandler, buwizz._con_handle, &buwizz.buwizz_service, 0x92D1);
            }
        } break;

        // 5. CHARACTERISTIC GEFUNDEN
        case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT: {
            gatt_client_characteristic_t characteristic;
            gatt_event_characteristic_query_result_get_characteristic(packet, &characteristic);
            
            if (characteristic.uuid16 == 0x92D1) {
                buwizz._motor_handle = characteristic.value_handle;
                printf("BuWizz: ⭐ MOTOR-HANDLE AUTOMATISCH: 0x%04x\n", buwizz._motor_handle);

                // SCHRITT C: Notifications aktivieren (Handle 0x0005 laut Landkarte)
                uint16_t enable_notify = 0x0001; 
                gatt_client_write_value_of_characteristic_without_response(
                    buwizz._con_handle, 0x0005, 2, (uint8_t*)&enable_notify);
                
                // SOFORT Ludicrous Modus setzen (Level 4)
                buwizz.setMode(4);
            }
            else{
                printf("BuWizz: characteristic.uuid16 -not 0x92D1: 0x%04x\n", characteristic.uuid16);
            }
        } break;
        // case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT: {
        //     // gatt_client_characteristic_t characteristic;
        //     // gatt_event_characteristic_query_result_get_characteristic(packet, &characteristic);
            
        //     // // SCHRITT 3: Handle speichern!
        //     // buwizz._motor_handle = characteristic.value_handle;
        //     // printf("BuWizz: ⭐ Motor-Handle gefunden: 0x%04x\n", buwizz._motor_handle);

        //     // // Jetzt, wo wir das Handle haben, können wir den Modus setzen
        //     // buwizz.setMode(2);
        //     gatt_client_characteristic_t characteristic;
        //     gatt_event_characteristic_query_result_get_characteristic(packet, &characteristic);
            
        //     // buwizz._motor_handle = characteristic.value_handle;
        //     // printf("BuWizz: ⭐ MOTOR-HANDLE GEFUNDEN: 0x%04x\n", buwizz._motor_handle);

        //     // // EINMALIG: Notifications aktivieren (Laut Datenblatt Handle 0x0005)
        //     // uint16_t enable_notify = 0x0001; 
        //     // gatt_client_write_value_of_characteristic_without_response(
        //     //     buwizz._con_handle, 0x0005, 2, (uint8_t*)&enable_notify);
            
        //     // // Jetzt ist der BuWizz bereit für Befehle aus der Main-Task

        //                 // Wir prüfen auf die 16-bit UUID 0x92D1 aus dem Datenblatt
        //     if (characteristic.uuid16 == 0x92D1) {
        //         buwizz._motor_handle = characteristic.value_handle;
        //         printf("BuWizz: ⭐ MOTOR-HANDLE GEFUNDEN: 0x%04x\n", buwizz._motor_handle);

        //         // JETZT: Notifications aktivieren (Handle 0x0005 laut Datenblatt)
        //         uint16_t enable_notify = 0x0001; 
        //         gatt_client_write_value_of_characteristic_without_response(
        //             buwizz._con_handle, 0x0005, 2, (uint8_t*)&enable_notify);
        //     }
        //     else{
        //         printf("BuWizz: characteristic.uuid16 0x%04x\n", characteristic.uuid16);
        //     }

        // }break;

        case GATT_EVENT_NOTIFICATION: {
            // Das Paket enthält: [Event_Type, Size, Handle(2), Value_Length(2), Data(...)]
            // Die eigentlichen BuWizz-Daten starten bei Index 8 (je nach BTstack Version)
            uint8_t* data = &packet[8]; 
            float battV = 3.0f + data[2] * 0.01f;
            printf("BuWizz Batterie: %.2f V\n", battV);
        } break;

        

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            buwizz._connected = false;
            printf("BuWizz: Verbindung verloren.\n");
            break;
    }
}



void BuWizz::setMode(uint8_t mode) {
    if (!isConnected()) return;
    uint8_t cmd[] = { 0x11, mode };
    printf("BuWizz: set Mode:%d.\n", mode);
    // Wir schreiben direkt auf das vermutete Handle 0x001b
    uint8_t err = gatt_client_write_value_of_characteristic_without_response(_con_handle, _motor_handle, 2, cmd);
    printf("Stack-Status Mode (0x03): %d\n", err);
}

void BuWizz::setMotors(int8_t m1, int8_t m2, int8_t m3, int8_t m4) {
    if (!_connected) return;
    uint8_t payload[] = { 0x10, (uint8_t)m1, (uint8_t)m2, (uint8_t)m3, (uint8_t)m4, 0x00 };
    printf("BuWizz: setMotors:cmd %d,m0:%d,m1:%d,m2:%d,m3:%d.\n",payload[0],payload[1],payload[2],payload[3],payload[4]);
    gatt_client_write_value_of_characteristic_without_response(_con_handle, _motor_handle, 6, payload);
}


void BuWizz::process() {
    // Hier kannst du später Logik einbauen, die in jedem Loop-Durchlauf laufen soll
}
