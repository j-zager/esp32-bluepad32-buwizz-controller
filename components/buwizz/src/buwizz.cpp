#include "buwizz.h"
#include <stdio.h>


// UUIDs von String in Byte-Arrays umwandeln (BTstack braucht Little Endian)
// Service: 4e050000-74fb-4481-88b3-9919b1676e93
static const uint8_t BUWIZZ_SERVICE_UUID128[] = { 
    0x4e, 0x05, 0x00, 0x00, 0x74, 0xfb, 0x44, 0x81, 
    0x88, 0xb3, 0x99, 0x19, 0xb1, 0x67, 0x6e, 0x93 
};

// In der BuWizz.cpp oben
static const uint16_t BUWIZZ_CHAR_UUID16 = 0x92D1;// 0x92D1


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
    printf("BuWizz: Starte Suche nach BuWizz Stein...\n"); 
    uni_bt_le_scan_start();
}


void BuWizz::packetHandler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    // ABSOLUTER DEBUG - Zeigt JEDES Byte vom Funkchip
    // printf("P:%02x E:%02x\n", packet_type, packet[0]);

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
            else if (subevent == HCI_SUBEVENT_LE_CONNECTION_COMPLETE || 
                subevent == HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE_V1) {
                if (packet[3] == 0) { // Status OK
                    buwizz._con_handle = little_endian_read_16(packet, 4); // Deine 0x0000 Position
                    buwizz._connected = true;

                     printf("BuWizz: ✔ Verbunden!-KONSTANT GRÜN! Handle: 0x%04x.\n", buwizz._con_handle);
                    
                    // Sofortige Service-Suche (UUID128 vorwärts)
                    gatt_client_discover_primary_services_by_uuid128(
                        &BuWizz::packetHandler, buwizz._con_handle, BUWIZZ_SERVICE_UUID128);                    
                }
            }
        } break;


        case GATT_EVENT_SERVICE_QUERY_RESULT: {
            // Wir speichern den Service nur zwischen!
            gatt_event_service_query_result_get_service(packet, &buwizz.buwizz_service);
            buwizz.service_found = true;
            // printf("BuWizz: Service in Reichweite (0x%04x-0x%04x).\n", 
            // buwizz.buwizz_service.start_group_handle, buwizz.buwizz_service.end_group_handle);
        } break;

        // 4. EINE SUCHE IST ABGESCHLOSSEN (Die Brücke)
        case GATT_EVENT_QUERY_COMPLETE: {
        //     if (buwizz._connected && buwizz.service_found && buwizz._motor_handle == 0) {
        //         printf("BuWizz: Service-Suche fertig. Suche jetzt Characteristics...\n");
        //         // SCHRITT B: Suche Characteristics (16-Bit UUID aus Datenblatt)
        //         gatt_client_discover_characteristics_for_service_by_uuid16(
        //             &BuWizz::packetHandler, buwizz._con_handle, &buwizz.buwizz_service, BUWIZZ_CHAR_UUID16);
        //     }
        // } break;
            // Wenn Services fertig sind -> Chars suchen
            if (buwizz.service_found && buwizz._motor_handle == 0) {
                printf("BuWizz: Service-Suche fertig. Suche jetzt Characteristics...\n");
                gatt_client_discover_characteristics_for_service_by_uuid16(
                    &BuWizz::packetHandler, buwizz._con_handle, &buwizz.buwizz_service, BUWIZZ_CHAR_UUID16);
                buwizz.service_found = false; 
            } 
            // Wenn Chars fertig sind -> Nur noch Status-Request als finaler Anstoß
            else if (buwizz._motor_handle != 0) {
                printf("BuWizz: Discovery Pipeline abgeschlossen. Sende ersten Status-Check.\n");
                static uint8_t status_cmd = 0x00;
                gatt_client_write_value_of_characteristic_without_response(
                    buwizz._con_handle, buwizz._motor_handle, 1, &status_cmd);
            }
        } break;

        // 5. CHARACTERISTIC GEFUNDEN
        case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT: {
            gatt_client_characteristic_t characteristic;
            gatt_event_characteristic_query_result_get_characteristic(packet, &characteristic);
            
            if (characteristic.uuid16 == BUWIZZ_CHAR_UUID16) {
                buwizz._motor_handle = characteristic.value_handle;
                printf("BuWizz: ⭐ Motor-Handle identifiziert: 0x%04x\n", buwizz._motor_handle);

                // DIREKT HIER: Den "Datenhahn" aufdrehen (CCCD)
                // Das gehört kausal zum Finden der Characteristic dazu!
                static uint16_t enable_notify = 0x0001; 
                // gatt_client_write_value_of_characteristic_without_response(
                //     buwizz._con_handle, 0x0005, 2, (uint8_t*)&enable_notify);


                // Statt gatt_client_write_value_of_characteristic_without_response nutzen wir:
                gatt_client_write_value_of_characteristic(
                    &BuWizz::packetHandler, 
                    buwizz._con_handle, 
                    0x0005, // CCCD Handle
                    2, 
                    (uint8_t*)&enable_notify // Wert 0x0001
                );

                
                printf("BuWizz: CCCD (0x0005) aktiviert.\n");
                // SOFORT slow Modus setzen (Level 1)
                buwizz.setMode(1);
            }
            else{
                printf("BuWizz: characteristic.uuid16 -not 0x92D1: 0x%04x\n", characteristic.uuid16);
            }
        } break;
        //     gatt_client_characteristic_t characteristic;
        //     gatt_event_characteristic_query_result_get_characteristic(packet, &characteristic);
            
        //     if (characteristic.uuid16 == BUWIZZ_CHAR_UUID16) {
        //         buwizz._motor_handle = characteristic.value_handle;
        //         printf("BuWizz: ⭐ MOTOR-HANDLE AUTOMATISCH: 0x%04x\n", buwizz._motor_handle);

        //         // SCHRITT C: Notifications aktivieren (Handle 0x0005 laut Manual)
        //         uint16_t enable_notify = 0x0001; 
        //         gatt_client_write_value_of_characteristic_without_response(
        //             buwizz._con_handle, 0x0005, 2, (uint8_t*)&enable_notify);
                
        //         // SOFORT slow Modus setzen (Level 1)
        //         buwizz.setMode(1);
        //     }
        //     else{
        //         printf("BuWizz: characteristic.uuid16 -not 0x92D1: 0x%04x\n", characteristic.uuid16);
        //     }
        // } break;

        case GATT_EVENT_NOTIFICATION: {
            // Das Paket enthält: [Event_Type, Size, Handle(2), Value_Length(2), Data(...)]
            // Die eigentlichen BuWizz-Daten starten bei Index 8 (je nach BTstack Version)
            uint8_t* data = &packet[8]; 
            float battV = 3.0f + data[2] * 0.01f;
            printf("BuWizz Batterie: %.2f V\n", battV);
        } break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            buwizz._motor_handle = 0;
            buwizz._connected = false;
            printf("BuWizz: Verbindung verloren.\n");
            break;
    }
}



void BuWizz::setMode(uint8_t mode) {
    if (!isConnected()) return;
    // Wir füllen das Array in der Klasse (fester Speicherplatz)
    _mode_payload[0] = 0x11; // Command: Set Power Level
    _mode_payload[1] = mode;

    printf("BuWizz: set Mode:%d.\n", mode);

    // Wir übergeben den Zeiger auf den Klassenspeicher
    uint8_t err = gatt_client_write_value_of_characteristic_without_response(
        _con_handle, _motor_handle, 2, _mode_payload);
    
    if (err) printf("Stack-Status Mode Error: 0x%02x\n", err);
}

void BuWizz::setMotors(int8_t m1, int8_t m2, int8_t m3, int8_t m4) {
    if (!_connected || _motor_handle == 0) return;

    // Wir füllen das Array in der Klasse
    _motor_payload[0] = 0x10; // Command: Set Motor Data
    _motor_payload[1] = (uint8_t)m1;
    _motor_payload[2] = (uint8_t)m2;
    _motor_payload[3] = (uint8_t)m3;
    _motor_payload[4] = (uint8_t)m4;
    _motor_payload[5] = 0x00; // Brake Mask (0 = Ausrollen)

    // Optionaler Debug-Print (kann bei hoher Frequenz das Log fluten)
    // printf("BuWizz Motors: %d, %d, %d, %d\n", m1, m2, m3, m4);

    uint8_t err = gatt_client_write_value_of_characteristic_without_response(
        _con_handle, _motor_handle, 6, _motor_payload);

    if (err) printf("Stack-Status Motor Error: 0x%02x\n", err);
}

void BuWizz::requestBattery() {
    if (!_connected || _motor_handle == 0) return;
    
    // Befehl 0x00 laut Manual: "Device status report"
    static uint8_t cmd = 0x00; 
    
    // Wir senden an das Motor-Handle (0x0003)
    // Wir nutzen without_response, damit es schnell geht
    gatt_client_write_value_of_characteristic_without_response(
        _con_handle, _motor_handle, 1, &cmd);
        
    printf("BuWizz: Status angefordert...\n");
}

void BuWizz::process() {
    // Hier kannst du später Logik einbauen, die in jedem Loop-Durchlauf laufen soll
}


