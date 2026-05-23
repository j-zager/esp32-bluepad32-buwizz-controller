# Bluepad32 for ESP32

Supports the different ESP32 chips:

* ESP32
* ESP32-S3
* ESP32-C3 / ESP32-C6
* ESP32-H2

## Install dependencies

1. Install ESP-IDF

   Install the ESP32 toolchain. Use version **5.3**. Might work on newer / older
   ones, but not tested.

    * <https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/>

2. Integrate BTstack into ESP32

   ```sh
   cd ${BLUEPAD32}/external/btstack/port/esp32
   # This will install BTstack as a component inside Bluepad32 source code (recommended).
   # Remove "IDF_PATH=../../../../src" if you want it installed in the ESP-IDF folder
   IDF_PATH=../../../../src ./integrate_btstack.py
   ```

3. Compile Bluepad32

    ```sh
    # Possible options: esp32, esp32-s3, esp32-c3, esp32-c6 or esp32-h2
    idf.py set-target esp32
    ```

   And compile it:

    ```sh
    idf.py build
    ```

4. Flash it

    ```sh
    idf.py flash monitor
    ```

## Debugging

In case you need to debug an ESP32-S3 (or ESP32-C3 / C6) using JTAG, follow these steps: (or
read [ESP32 JTAG Debugging][esp32-gdb]).

*Note: If you have a standard ESP32, you can do it with an [ESP-PROG][esp-prog] module.*

TL;DR: Open 3 terminals, and do:

### Terminal 1

```shell
idf.py openocd
```

Or if you prefer the verbose way (but not both):

```shell
# Open OpenOCD
# Valid for ESP32-S3. Change it to "esp32c3-builtin.cfg" for ESP32-C3
sudo openocd -f board/esp32s3-builtin.cfg  -c "adapter speed 5000"
```

### Terminal 2

```shell
idf.py gdb
```

Or if you prefer the verbose way (but not both):

```shell
xtensa-esp32s3-elf-gdb bluepad32_esp32_example_app.elf
> target remote :3333
> set remote hardware-watchpoint-limit 2
> mon reset halt
> maintenance flush register-cache
> thb app_main
> c
```

### Terminal 3

```shell
# macOS
tio /dev/tty.usbmodem21202

# Linux
tio /dev/ttyACM0
```

[esp32-gdb]: https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/jtag-debugging/index.html

[esp-prog]: https://docs.espressif.com/projects/esp-iot-solution/en/latest/hw-reference/ESP-Prog_guide.html

## License

- Example code: licensed under Public Domain.
- Bluepad32: licensed under Apache 2.
- BTstack:
    - Free to use for open source projects.
    - Paid for commercial projects.
    - <https://github.com/bluekitchen/btstack/blob/master/LICENSE>

## Introduction to the Buwizz-Gamepad-ESP32 project
A  ESP‑IDF project that combines Bluepad32 (PS4 controller support) with a fully modular BuWizz BLE client for LEGO robotics.
The project provides a dedicated controller class implementing press, long‑press, double‑press, trigger, and analog stick events, while maintaining parallel BLE communication with one or more BuWizz devices.

The architecture emphasizes:

Separation of concerns (Controller module, BuWizz module, Application layer)

Event handling via a custom PS4 input state machine

BLE operation through queued GATT writes and explicit service/characteristic discovery

Scalability for multi‑BuWizz setups, via adding Buwizz in adress 



### ToDo's

#### High Prio

- stabilize connect process(quicker)(✔) -> could be little bit faster (✔)
- verify esp32 connects to the right brick, even in inverse brick activation (✔)

#### Middle Prio
- create config file for buwizz adresses, assignment of controller commands and pin settings
- analyse min time between motor/ mode commands  to react smoother (✔) -> Is further interval timing reduction possible?
#### Low Prio
- implement/ rework Buwizz battery request


