#include "buwizz.h"
#include <stdio.h>


// UUIDs von String in Byte-Arrays umwandeln (BTstack braucht Little Endian)
// Service: 4e050000-74fb-4481-88b3-9919b1676e93
static const uint8_t BUWIZZ_SERVICE_UUID128[] = { 
    0x4e, 0x05, 0x00, 0x00, 0x74, 0xfb, 0x44, 0x81, 
    0x88, 0xb3, 0x99, 0x19, 0xb1, 0x67, 0x6e, 0x93 
};

// 
static const uint16_t BUWIZZ_CHAR_UUID16 = 0x92D1;// 0x92D1


// Characteristic: 000092d1-0000-1000-8000-00805f9b34fb
static const uint8_t BUWIZZ_CHAR_UUID128[] = { 
    0x00, 0x00, 0x92, 0xd1, 0x00, 0x00, 0x10, 0x00, 
    0x80, 0x00, 0x00, 0x80, 0x5f, 0x9b, 0x34, 0xfb 
};



BuWizz::BuWizz(const uint8_t* addr) : 
    _con_handle(HCI_CON_HANDLE_INVALID), 
    _connected(false), 
    _motor_handle(0x0000), // Auf 0 setzen, damit Discovery beweisbar ist //_motor_handle(0x0003),
    service_found(false) 
    {
        memcpy(_addr, addr, 6);
    }

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
    printf("[DEBUG][Connect]BuWizz: Starte Suche nach BuWizz Stein...\n"); 
    uni_bt_le_scan_start();
}

void BuWizz::packetHandler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    if (packet_type != HCI_EVENT_PACKET) return;

    uint8_t event = hci_event_packet_get_type(packet);
    BuWizz* current = nullptr;
    uint16_t con_handle = 0xFFFF;

    //     // --- DEBUG: JEDES HCI PAKET ANZEIGEN ---
    // if (event == HCI_EVENT_LE_META) {
    //     uint8_t sub = hci_event_le_meta_get_subevent_code(packet);
    //     // printf("[DEBUG] LE_META Sub: 0x%02X empfangen\n", sub);
    // } else {
    //     // printf("[DEBUG] HCI Event: 0x%02X empfangen\n", event);
    // }

    if (event == HCI_EVENT_COMMAND_COMPLETE) {
        return; // Command Complete hat keine Relevanz für das Stein-Mapping!
    }

    // --- A) IDENTIFIZIERUNG ÜBER MAC (Bei Erstkontakt) ---
    // --- 1. WER IST DER ABSENDER? ---
    if (event == HCI_EVENT_LE_META) {
        uint8_t subevent = hci_event_le_meta_get_subevent_code(packet);
        // printf("status ok=0 -> check status :%d\n",packet[3]);

        //  Beim Scan-Report (0x02) liegt die MAC ab Index 6 rückwärts
        if (subevent == HCI_SUBEVENT_LE_ADVERTISING_REPORT) { // SCAN: Über MAC finden
            bd_addr_t scan_mac;
            for (int i = 0; i < 6; i++) scan_mac[i] = packet[11 - i];
            for (int i = 0; i < NUM_BRICKS; i++) {
                if (memcmp(scan_mac, mulBuWizz[i]->_addr, 6) == 0) { 
                    current = mulBuWizz[i]; 
                    printf("[IDENT] Scan-Treffer für Stein: %02X <> id: %d <> via scan_mac\n", current->_addr[5], i);
                    break; 
                }
            }
        } 

        // 2. CONNECT: Echte Makros extrahieren die MAC fehlerfrei aus dem Event-Buffer
        else if (subevent == HCI_SUBEVENT_LE_CONNECTION_COMPLETE) {
            bd_addr_t connect_mac;
            hci_subevent_le_connection_complete_get_peer_address(packet, connect_mac);
            for (int i = 0; i < NUM_BRICKS; i++) {
                if (memcmp(connect_mac, mulBuWizz[i]->_addr, 6) == 0) { 
                    current = mulBuWizz[i]; 
                    printf("[IDENT] [CON] Scan-Treffer für Stein: %02X <> id%d <> via connnect_mac\n", connect_mac[5], i);
                    break; 
                }
            }
        } 
        else if (subevent == HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE_V1) {
            bd_addr_t connect_mac;
            hci_subevent_le_enhanced_connection_complete_v1_get_peer_addresss(packet,connect_mac);
            for (int i = 0; i < NUM_BRICKS; i++) {
                if (memcmp(connect_mac, mulBuWizz[i]->_addr, 6) == 0) { 
                    current = mulBuWizz[i]; 
                    printf("[IDENT] [CON_V1] Scan-Treffer für Stein: %02X <> id%d <> via connnect_mac\n", connect_mac[5], i);
                    break; 
                }
            }
        }

        if (!current && (subevent == HCI_SUBEVENT_LE_CONNECTION_COMPLETE || subevent == HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE_V1)) {
            uint8_t status = 0;
            
            // BTstack-Makros holen den Status automatisch an der richtigen Byte-Schnittstelle:
            if (subevent == HCI_SUBEVENT_LE_CONNECTION_COMPLETE) {
                status = hci_subevent_le_connection_complete_get_status(packet);
            } else {
                status = hci_subevent_le_enhanced_connection_complete_v1_get_status(packet);
            }

            // Wenn der Status ungleich 0 (Erfolg) ist, handelt es sich GARANTIERT um einen Fehler!
            if (status != 0) {
                for (int i = 0; i < NUM_BRICKS; i++) {
                    if (mulBuWizz[i]->_connect_triggered && !mulBuWizz[i]->_connected) {
                        current = mulBuWizz[i];
                        printf("[IDENT] >>> Fehlgeschlagener Handshake (Status 0x%02X) wurde Stein %02X (ArrayId: %d) zugeordnet! <<<\n", 
                            status, current->_addr[5], i);
                        break;
                    }
                }
            }
        }
        // bd_addr_t errorAdress ;

        // Falls wir das Objekt über die MAC bei LE_META Events (0x3E) nicht direkt haben,
        // lesen wir das Handle an der offiziellen HCI-Meta-Stelle (Byte 4) aus.
        if (!current && subevent != HCI_SUBEVENT_LE_ADVERTISING_REPORT) {
            con_handle = little_endian_read_16(packet, 4);
            con_handle = hci_subevent_le_connection_complete_get_connection_handle(packet); // 0x01 HCI_SUBEVENT_LE_CONNECTION_COMPLETE
            con_handle = hci_subevent_le_connection_update_complete_get_connection_handle(packet);// 0x03 HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE
            printf("[IDENT] Scan-Treffer für Stein: <> non HCI_SUBEVENT_LE_ADVERTISING_REPORT <> via con_handle:%02X in subevent: %02X\n", con_handle,subevent); 
        }

    }
    // --- B) IDENTIFIZIERUNG ÜBER HANDLE (Bei laufender Kommunikation) ---
    else if (event == HCI_EVENT_DISCONNECTION_COMPLETE) {
        con_handle = hci_event_disconnection_complete_get_connection_handle(packet);
    } 
    else if (event == GATT_EVENT_SERVICE_QUERY_RESULT){
        con_handle = gatt_event_service_query_result_get_handle(packet);
    }
    else if (event == GATT_EVENT_CHARACTERISTIC_QUERY_RESULT){
        con_handle = gatt_event_characteristic_query_result_get_handle(packet);
    }
    else if (event == GATT_EVENT_QUERY_COMPLETE){
        con_handle = gatt_event_query_complete_get_handle(packet);
    }
    else if (event == GATT_EVENT_NOTIFICATION){
        con_handle = gatt_event_notification_get_handle(packet);
    }

    // --- C) FLOTTEN-ZUORDNUNG ---
    if (!current && con_handle != HCI_CON_HANDLE_INVALID) {
        for (int i = 0; i < NUM_BRICKS; i++) {
            if (mulBuWizz[i]->_con_handle == con_handle) {
                current = mulBuWizz[i];
                printf("[IDENT] Scan-Treffer für Stein: %02X <> id%d <> via con_handle:%02X in event: %02X\n", mulBuWizz[i]->_addr[5], i,con_handle,event); 
                break;
            }
        }
    }        // --- DER ENTSCHEIDENDE RETTUNGS-ANKER FÜR DEN 0xFFFF FEHLERFALL ---
    else if (!current && con_handle == HCI_CON_HANDLE_INVALID ){//&& event == HCI_EVENT_LE_META) {
        // uint8_t subevent = hci_event_le_meta_get_subevent_code(packet);
        // if (subevent == HCI_SUBEVENT_LE_CONNECTION_COMPLETE || subevent == HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE_V1) {
            
            for (int i = 0; i < NUM_BRICKS; i++) {
                // Wir suchen den EINEN Stein, der gerade die Antenne blockiert (triggered), aber noch nicht online ist
                //check which brick has a connect trigger, no established connection and only if one trigger is active to be sure allow blind connecting
                if (mulBuWizz[i]->_connect_triggered && !mulBuWizz[i]->_connected && mulBuWizz[i]->triggerActive() == 1) {
                    // current = mulBuWizz[i];
                    mulBuWizz[i]->_connect_triggered = false;
                    printf("[IDENT] >>> HARDWARE-FEHLER (Handle 0xFFFF): %02X <> id%d <> via leftover connect_trigger handle:%02X in event: %02X\n", mulBuWizz[i]->_addr[5], i,con_handle,event);
                    break;
                }
            }
        // }
    }


    if (!current) {
        // printf("[WARN] Paket konnte keinem Stein zugeordnet werden! (Event 0x%02X)\n", event);
        return;
    }

    // --- 2. LOGIK ---
    switch (event) {
        case HCI_EVENT_LE_META: {
            uint8_t subevent = hci_event_le_meta_get_subevent_code(packet);

            // SCHRITT 1: BuWizz im Scan finden
            if (subevent == HCI_SUBEVENT_LE_ADVERTISING_REPORT) {
                //  Sofortige harte Sperre: Wenn getriggert oder verbunden -> ABBRUCH
                if (current->_connected || current->_connect_triggered) {
                    return; 
                }
                if (!current->_connected && !current->_connect_triggered) {
                    printf("Scan - Current Brick MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", 
                        current->_addr[0], current->_addr[1], current->_addr[2], 
                        current->_addr[3], current->_addr[4], current->_addr[5]);
                    printf("[INFO] BuWizz: Gefunden! Verbinde...\n");
                    current->_connect_triggered = true;
                    uni_bt_le_scan_stop();
                    printf("gap connect packet type:%02X\n",(bd_addr_type_t)packet[5]);
                    gap_connect(current->_addr, (bd_addr_type_t)packet[5]);
                }
            }
            // SCHRITT 2: Verbindung erfolgreich
            else if (subevent == HCI_SUBEVENT_LE_CONNECTION_COMPLETE || 
                subevent == HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE_V1) {
                // --- DIE DOPPEL-SPERRE (VERRIEGELUNG) ---
                // Wenn dieser Stein bereits als "verbunden" markiert ist, 
                // ignorieren wir das zweite, doppelte Event komplett!
                if (current->_connected) {
                    return; 
                }

                uint8_t status = 0;
                hci_con_handle_t temp_handle = HCI_CON_HANDLE_INVALID;
                if (subevent == HCI_SUBEVENT_LE_CONNECTION_COMPLETE) {
                    status = hci_subevent_le_connection_complete_get_status(packet);
                    temp_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
                } else if(subevent == HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE_V1) {
                    status = hci_subevent_le_enhanced_connection_complete_v1_get_status(packet);
                    temp_handle = hci_subevent_le_enhanced_connection_complete_v1_get_connection_handle(packet);
                }

                if (status == 0 && temp_handle != 0xFFFF) { // Status OK
                    // current->_con_handle = little_endian_read_16(packet, 4);
                    current->_con_handle = temp_handle;
                    current->_connected = true;
                    // printf("BuWizz [0x%04x]: ✔ Verbunden!\n", current->_con_handle);
                    printf("[INFO] Connected - Current Brick MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", 
                        current->_addr[0], current->_addr[1], current->_addr[2], 
                        current->_addr[3], current->_addr[4], current->_addr[5]);
                    printf("BuWizz: ✅ Verbunden!-KONSTANT GRÜN! Handle: 0x%04x.\n", current->_con_handle);
                    // connect trigger freigeben wenn verbunden
                    current->_connect_triggered = false;
                    current->_was_connected = true;
                    // Wichtig hier nicht Scan aktivieren
                    // Sofortige Service-Suche (UUID128 vorwärts)
                    gatt_client_discover_primary_services_by_uuid128(
                        &BuWizz::packetHandler, current->_con_handle, BUWIZZ_SERVICE_UUID128);
                }
                else {
                    // Fehler-Fall: Reset, damit die Main-Task es neu versuchen kann
                    current->_connect_triggered = false;
                    current->_connected = false;
                    current->_mode_set = false;
                    current->_char_found = false;
                    current->_motor_handle = 0;
                    current->_con_handle = HCI_CON_HANDLE_INVALID;
                    current->_connected_at = 0;

                    printf("BuWizz: Connect Fehler 0x%02X. Scan Restart.\n", status);
                    uni_bt_le_scan_start();
                }



            }
        } break;

        case GATT_EVENT_SERVICE_QUERY_RESULT:
            // Wir speichern den Service nur zwischen!
            printf("[DEBUG] [GATT_EVENT_SERVICE_QUERY_RESULT] - Current Brick MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", 
                current->_addr[0], current->_addr[1], current->_addr[2], 
                current->_addr[3], current->_addr[4], current->_addr[5]);
            gatt_event_service_query_result_get_service(packet, &current->buwizz_service);
            current->service_found = true;
            break;


        // 4. EINE SUCHE IST ABGESCHLOSSEN (Die Brücke)
        case GATT_EVENT_QUERY_COMPLETE:
            printf("[DEBUG] [GATT_EVENT_QUERY_COMPLETE] - Current Brick MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", 
                current->_addr[0], current->_addr[1], current->_addr[2], 
                current->_addr[3], current->_addr[4], current->_addr[5]);

            // PHASE 1: Service-Suche beendet, starte Characteristic-Suche
            if (current->service_found && current->_motor_handle == 0) {
                printf("BuWizz: Service-Suche fertig. Suche jetzt Characteristics...\n");
                gatt_client_discover_characteristics_for_service_by_uuid16(
                    &BuWizz::packetHandler, current->_con_handle, &current->buwizz_service, BUWIZZ_CHAR_UUID16);
                current->service_found = false;
            }
            // PHASE 2: Characteristic-Suche beendet (Motor-Handle ist nun bekannt)
            else if (current->_motor_handle != 0) {
                printf("BuWizz [0x%04X]: Discovery vollständig abgeschlossen.\n", current->_con_handle);
                
                // --- INTELLIGENTER SCAN-STOP ---
                bool all_bricks_ready = true;
                for (int i = 0; i < NUM_BRICKS; i++) {
                    // Ein Stein ist erst "ready", wenn er verbunden ist UND sein Motor-Handle (Zimmernummer) kennt
                    if (!mulBuWizz[i]->_connected || mulBuWizz[i]->_motor_handle == 0) {
                        all_bricks_ready = false;
                        break;
                    }
                }

                if (all_bricks_ready) {
                    printf("[DEBUG] >>> Alle BuWizz-Steine fahrbereit! Stoppe Scan. <<<\n");
                    //uni_bt_le_scan_stop();
                }
            }  
            // --- DIE ENTSCHEIDENDE RETTUNG BEI KRYPTO-HÄNGERN ---
            else {
                // Suche beendet, aber weder Service gefunden noch Motor-Handle da!
                printf("[WARN] BuWizz [0x%02X]: Discovery fehlgeschlagen (Krypto-Blockade). Erzwinge Disconnect handle [0x%04X] ...\n", current->_addr[5],current->_con_handle);
                
                // Wir kappen die fehlerhafte Geister-Verbindung aktiv auf Hardware-Ebene!
                gap_disconnect(current->_con_handle); 
                
                // Alle Flags im Objekt säubern, damit der nächste Versuch frisch startet
                current->_connected = false;
                current->_connect_triggered = false;
                current->_mode_set = false;
                current->_char_found = false;
                current->_motor_handle = 0;
                current->_connected_at = 0;
            }
            break;

        // 5. CHARACTERISTIC GEFUNDEN
        case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT: {
            printf("[DEBUG][GATT_EVENT_CHARACTERISTIC_QUERY_RESULT] - Current Brick MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", 
                current->_addr[0], current->_addr[1], current->_addr[2], 
                current->_addr[3], current->_addr[4], current->_addr[5]);

            gatt_client_characteristic_t characteristic;
            gatt_event_characteristic_query_result_get_characteristic(packet, &characteristic);

            if (characteristic.uuid16 == BUWIZZ_CHAR_UUID16) {
                current->_motor_handle = characteristic.value_handle;
                printf("BuWizz [0x%04x]: ⭐ Handle 0x%04x\n", current->_con_handle, current->_motor_handle);
                current->_char_found =true;
                static uint16_t notify = 0x0001;
                gatt_client_write_value_of_characteristic(
                    &BuWizz::packetHandler, current->_con_handle, 0x0005, 2, (uint8_t*)&notify);
            }
            else{
                 printf("BuWizz: characteristic.uuid16 -not 0x92D1: 0x%04x\n", characteristic.uuid16);
             }
        } break;

        case GATT_EVENT_NOTIFICATION: {
            float battV = 3.0f + packet[10] * 0.01f;
            // Wir printen die Addresse mit, um die Steine im Log zu unterscheiden!
            printf("BuWizz [0x%04x] Batterie: %.2f V\n", current->_addr[5], battV);
        } break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            printf("HCI_EVENT_DISCONNECTION_COMPLETE - Current Brick MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", 
                current->_addr[0], current->_addr[1], current->_addr[2], 
                current->_addr[3], current->_addr[4], current->_addr[5]);

            // Altes Pairing komplett im Stack löschen ---
            gap_drop_link_key_for_bd_addr(current->_addr);

            printf("BuWizz [0x%04x]: ❌ Getrennt.\n", current->_con_handle);
            current->_connected = false;
            current->_motor_handle = 0;
            current->_con_handle = HCI_CON_HANDLE_INVALID;
            current->_char_found = false;
            current->_connect_triggered = false; // Zur Sicherheit freischalten
            current->_mode_set = false;
            current->_start_connecting_time = 0;
            current->_connected_at = 0;
            // Wenn alle getrennt sind, Scan wieder starten
            uni_bt_le_scan_start(); 
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

    // _mode_counter++;
    // if (_mode_counter >= 50) {
    //     this->setMode(3);
    //     _mode_counter = 0;
    // }

    // DEBUG PRINT (nur alle 2 Sekunden, sonst Log-Spam)
    static uint64_t last_print = 0;
    if (esp_timer_get_time() - last_print > 2000000) {
        printf("[SEND] Brick %02X -> Handle: 0x%04X | Zimmer: 0x%04X\n | m1:%d m2:%d m3:%d m4:%d \n", 
               _addr[5], _con_handle, _motor_handle, m1, m2, m3, m4);
        last_print = esp_timer_get_time();
    }


    uint8_t err = gatt_client_write_value_of_characteristic_without_response(
        _con_handle, _motor_handle, 6, _motor_payload);
    //Fehler 0x94 (Busy) siehst, ist das bei Bluetooth normal, wenn Pakete zu schnell kommen
    if (err && err != 0x94) printf("Motor Error: 0x%02x\n", err);
}


void BuWizz::useMotors() {
    if (!_connected || _motor_handle == 0) return;


    // DEBUG PRINT (nur alle 2 Sekunden, sonst Log-Spam)
    static uint64_t last_print = 0;
    if (esp_timer_get_time() - last_print > 2000000) {
        printf("[SEND] Brick %02X -> Handle: 0x%04X | Zimmer: 0x%04X\n | m1:%d m2:%d m3:%d m4:%d \n", 
               _addr[5], _con_handle, _motor_handle, _motor_payload[1], _motor_payload[2],
                _motor_payload[3], _motor_payload[4]);
        last_print = esp_timer_get_time();
    }


    uint8_t err = gatt_client_write_value_of_characteristic_without_response(
        _con_handle, _motor_handle, 6, _motor_payload);
    //Fehler 0x94 (Busy) siehst, ist das bei Bluetooth normal, wenn Pakete zu schnell kommen
    if (err && err != 0x94) printf("Motor Error: 0x%02x\n", err);
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


void BuWizz::triggerConnect(uint64_t now) {
    if (_init_time == 0) _init_time = now;

    if (_connect_triggered && !_connected) {
        if(_start_connecting_time == 0){
            _start_connecting_time = now;
        } 

        if (now - _start_connecting_time > 3900000) { // 10 -> 4.5Sek Timeout
            printf("[WARN] Stein %02X Handshake Timeout. Resetting...\n",_addr[5]);
            gap_connect_cancel();
            _connect_triggered = false; // Reset um neuen Trigger zu erlauben
            _start_connecting_time = 0;
            // WICHTIG: Nach einem Timeout sofort den Scan wieder anwerfen!
            _motor_handle = 0;
            _con_handle = HCI_CON_HANDLE_INVALID;
            _char_found = false;
            _mode_set = false;
            _connected_at = 0;
            //uni_bt_le_scan_start(); 
        }
    } else if(_connected){
        // Wenn verbunden, Timeout-Timer sauber schlafen legen
        _start_connecting_time = 0;
    }


    // A) Wenn nicht verbunden: Alles zurücksetzen und warten
    if (!_connected) {
        _connected_at = 0;
        _mode_set = false;
        return; // Im unverbundenen Zustand hier abbrechen
    }

    // B) Ab hier: Verbindung steht
    if (_connected_at == 0) _connected_at = now;

    // C) Einmaliges Scharfschalten (Modus 3)
    // Nur wenn Discovery fertig (Stern ⭐ da) und Modus noch nicht gesetzt
    if (!_mode_set && _motor_handle != 0) {
        // Wir geben dem Stein nach dem "Stern" 500ms Zeit zum Atmen
        if (now - _connected_at > 200000) {//200ms
            this->setMode(3); 
            _mode_set = true;
            printf("BuWizz [0x%04X]: Setup fertig (Modus 3).\n", _con_handle);
        }
    }
}


void BuWizz::saveMotor(int8_t m, int8_t idx) {
    if (idx<0 || idx> 5) return ;

    // Wir füllen das Array in der Klasse
    _motor_payload[idx] = m; // Command: Set Motor Data
    return;
}
uint8_t BuWizz::getMotor( int8_t idx) {
    if (idx < 0 || idx > 5) return 0;
    return _motor_payload[idx];
}

void BuWizz::saveMotorAll(int8_t m1,int8_t m2,int8_t m3,int8_t m4,int8_t brakeMask ) {
    _motor_payload[0] = 0x10; // Command: Set Motor Data
    _motor_payload[1] = (uint8_t)m1;
    _motor_payload[2] = (uint8_t)m2;
    _motor_payload[3] = (uint8_t)m3;
    _motor_payload[4] = (uint8_t)m4;
    _motor_payload[5] = brakeMask; // Brake Mask (0 = Ausrollen)
    return;
}
uint8_t* BuWizz::getMotorAll() {
    return _motor_payload;
}


uint16_t BuWizz::getMotorHandle() { 
    return _motor_handle; 
}

hci_con_handle_t BuWizz::getConnectHandle() { 
    return _con_handle; 
}

bool BuWizz::isConnected() { 
    return _connected; 
}

bool BuWizz::wasConnected() { 
    return _was_connected; 
}

bool BuWizz::isConnectTriggered() { 
    return _connect_triggered; 
}

bool BuWizz::isCharFound(){
    return _char_found;
}

uint8_t BuWizz::getAddr(int id){
    return _addr[id];
}


bool BuWizz::isReady() { 
    return _connected && (_motor_handle != 0) && _mode_set; 
    }

uint8_t BuWizz::triggerActive(){
    uint8_t activeBricks = 0;
    for(int i=0; i<NUM_BRICKS;i++){
        if(mulBuWizz[i]->_connect_triggered){
            activeBricks+=1;
        }
    }
    return activeBricks;
}


void BuWizz::process() {
    // Hier kannst du später Logik einbauen, die in jedem Loop-Durchlauf laufen soll
}




//     // 7. Alle 5 Sekunden: Batterie-Status anfordern
//     if (now - _last_battery_request > 5000000) {
//         this->requestBattery();
//         _last_battery_request = now;
//     }
// }



// void BuWizz::packetHandler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
//     // ABSOLUTER DEBUG - Zeigt JEDES Byte vom Funkchip
//     // printf("P:%02x E:%02x\n", packet_type, packet[0]);

//     if (packet_type != HCI_EVENT_PACKET) return;
//     uint8_t event = hci_event_packet_get_type(packet);

//     switch (event) {
//         case HCI_EVENT_LE_META: {
//             uint8_t subevent = hci_event_le_meta_get_subevent_code(packet);
            
//             // SCHRITT 1: BuWizz im Scan finden
//             if (subevent == HCI_SUBEVENT_LE_ADVERTISING_REPORT) { // Advertising Report
//                 bd_addr_t found_addr;
//                 // MAC Adresse liegt ab Byte 6 (rückwärts)
//                 for (int i = 0; i < 6; i++) found_addr[i] = packet[11 - i];
//                 uint8_t addr_type = packet[5];

//                 if (packet[6] == 0x4c && packet[7] == 0x03) { // 4C:03... Treffer!
//                     printf("BuWizz: Gefunden! Verbinde...\n");
//                     uni_bt_le_scan_stop();
//                     gap_connect(found_addr, (bd_addr_type_t)addr_type);
//                 }
//             }
//             // SCHRITT 2: Verbindung erfolgreich
//             else if (subevent == HCI_SUBEVENT_LE_CONNECTION_COMPLETE || 
//                 subevent == HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE_V1) {
//                 if (packet[3] == 0) { // Status OK
//                     buwizz._con_handle = little_endian_read_16(packet, 4); // Deine 0x0000 Position
//                     buwizz._connected = true;

//                      printf("BuWizz: ✔ Verbunden!-KONSTANT GRÜN! Handle: 0x%04x.\n", buwizz._con_handle);
                    
//                     // Sofortige Service-Suche (UUID128 vorwärts)
//                     gatt_client_discover_primary_services_by_uuid128(
//                         &BuWizz::packetHandler, buwizz._con_handle, BUWIZZ_SERVICE_UUID128);                    
//                 }
//             }
//         } break;


//         case GATT_EVENT_SERVICE_QUERY_RESULT: {
//             // Wir speichern den Service nur zwischen!
//             gatt_event_service_query_result_get_service(packet, &buwizz.buwizz_service);
//             buwizz.service_found = true;
//             // printf("BuWizz: Service in Reichweite (0x%04x-0x%04x).\n", 
//             // buwizz.buwizz_service.start_group_handle, buwizz.buwizz_service.end_group_handle);
//         } break;

//         // 4. EINE SUCHE IST ABGESCHLOSSEN (Die Brücke)
//         case GATT_EVENT_QUERY_COMPLETE: {
//         //     if (buwizz._connected && buwizz.service_found && buwizz._motor_handle == 0) {
//         //         printf("BuWizz: Service-Suche fertig. Suche jetzt Characteristics...\n");
//         //         // SCHRITT B: Suche Characteristics (16-Bit UUID aus Datenblatt)
//         //         gatt_client_discover_characteristics_for_service_by_uuid16(
//         //             &BuWizz::packetHandler, buwizz._con_handle, &buwizz.buwizz_service, BUWIZZ_CHAR_UUID16);
//         //     }
//         // } break;
//             // Wenn Services fertig sind -> Chars suchen
//             if (buwizz.service_found && buwizz._motor_handle == 0) {
//                 printf("BuWizz: Service-Suche fertig. Suche jetzt Characteristics...\n");
//                 gatt_client_discover_characteristics_for_service_by_uuid16(
//                     &BuWizz::packetHandler, buwizz._con_handle, &buwizz.buwizz_service, BUWIZZ_CHAR_UUID16);
//                 buwizz.service_found = false; 
//             } 
//             // Wenn Chars fertig sind -> Nur noch Status-Request als finaler Anstoß
//             else if (buwizz._motor_handle != 0) {
//                 printf("BuWizz: Discovery Pipeline abgeschlossen. Sende ersten Status-Check.\n");
//                 static uint8_t status_cmd = 0x00;
//                 gatt_client_write_value_of_characteristic_without_response(
//                     buwizz._con_handle, buwizz._motor_handle, 1, &status_cmd);
//             }
//         } break;

//         // 5. CHARACTERISTIC GEFUNDEN
//         case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT: {
//             gatt_client_characteristic_t characteristic;
//             gatt_event_characteristic_query_result_get_characteristic(packet, &characteristic);
            
//             if (characteristic.uuid16 == BUWIZZ_CHAR_UUID16) {
//                 buwizz._motor_handle = characteristic.value_handle;
//                 printf("BuWizz: ⭐ Motor-Handle identifiziert: 0x%04x\n", buwizz._motor_handle);

//                 // DIREKT HIER: Den "Datenhahn" aufdrehen (CCCD)
//                 // Das gehört kausal zum Finden der Characteristic dazu!
//                 static uint16_t enable_notify = 0x0001; 
//                 // gatt_client_write_value_of_characteristic_without_response(
//                 //     buwizz._con_handle, 0x0005, 2, (uint8_t*)&enable_notify);


//                 // Statt gatt_client_write_value_of_characteristic_without_response nutzen wir:
//                 gatt_client_write_value_of_characteristic(
//                     &BuWizz::packetHandler, 
//                     buwizz._con_handle, 
//                     0x0005, // CCCD Handle
//                     2, 
//                     (uint8_t*)&enable_notify // Wert 0x0001
//                 );

                
//                 printf("BuWizz: CCCD (0x0005) aktiviert.\n");
//                 // SOFORT slow Modus setzen (Level 1)
//                 buwizz.setMode(1);
//             }
//             else{
//                 printf("BuWizz: characteristic.uuid16 -not 0x92D1: 0x%04x\n", characteristic.uuid16);
//             }
//         } break;
//         //     gatt_client_characteristic_t characteristic;
//         //     gatt_event_characteristic_query_result_get_characteristic(packet, &characteristic);
            
//         //     if (characteristic.uuid16 == BUWIZZ_CHAR_UUID16) {
//         //         buwizz._motor_handle = characteristic.value_handle;
//         //         printf("BuWizz: ⭐ MOTOR-HANDLE AUTOMATISCH: 0x%04x\n", buwizz._motor_handle);

//         //         // SCHRITT C: Notifications aktivieren (Handle 0x0005 laut Manual)
//         //         uint16_t enable_notify = 0x0001; 
//         //         gatt_client_write_value_of_characteristic_without_response(
//         //             buwizz._con_handle, 0x0005, 2, (uint8_t*)&enable_notify);
                
//         //         // SOFORT slow Modus setzen (Level 1)
//         //         buwizz.setMode(1);
//         //     }
//         //     else{
//         //         printf("BuWizz: characteristic.uuid16 -not 0x92D1: 0x%04x\n", characteristic.uuid16);
//         //     }
//         // } break;

//         case GATT_EVENT_NOTIFICATION: {
//             // Das Paket enthält: [Event_Type, Size, Handle(2), Value_Length(2), Data(...)]
//             // Die eigentlichen BuWizz-Daten starten bei Index 8 (je nach BTstack Version)
//             uint8_t* data = &packet[8]; 
//             float battV = 3.0f + data[2] * 0.01f;
//             printf("BuWizz Batterie: %.2f V\n", battV);
//         } break;

//         case HCI_EVENT_DISCONNECTION_COMPLETE:
//             buwizz._motor_handle = 0;
//             buwizz._connected = false;
//             printf("BuWizz: Verbindung verloren.\n");
//             break;
//     }
// }