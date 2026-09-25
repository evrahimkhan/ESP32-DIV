# Evrahim S3 board profile (`BOARD_EVRAS3`)

Firmware target for a custom **ESP32-S3 N16R8** (16 MB flash, 8 MB **octal** PSRAM) with a
2.8" 240x320 **ST7789** + **XPT2046** touch panel, one **CC1101** sub-GHz radio and one
**NRF24L01+** (E01-MLO1DP5) 2.4 GHz radio. Touch-only UI — there are no physical buttons.

Selected in [`ESP32-DIV/BoardConfig.h`](ESP32-DIV/BoardConfig.h); every pin lives in
[`ESP32-DIV/shared.h`](ESP32-DIV/shared.h) under `#if defined(BOARD_EVRAS3)`.

---

## Pin map

| Function | GPIO | Notes |
|---|---|---|
| **Shared data bus** SCK / MOSI / MISO | 12 / 11 / 13 | TFT + SD + CC1101 + NRF24 |
| TFT ST7789 CS / DC / RST | 10 / 9 / 14 | RST driven by `tft.init()` |
| TFT backlight BL | 7 | PWM 5 kHz 8-bit, **HIGH = on** |
| Touch XPT2046 SCK / MOSI / MISO | 38 / 39 / 40 | own SPI controller |
| Touch CS / IRQ | 41 / 42 | |
| SD card CS | 5 | no card-detect switch |
| CC1101 CSN | 21 | |
| CC1101 GDO0 (TX) / GDO2 (RX) | 4 / 6 | `setGDO(TX, RX)` |
| NRF24 CE / CSN | 47 / 48 | single module |
| Grove I2C SDA / SCL | 15 / 16 | free, nothing auto-initialises it |
| Legacy 433 MHz RX / TX (`RX_PIN`/`TX_PIN`) | 8 / 8 | nothing wired — see below |
| UART0 TX / RX | 43 / 44 | |
| Native USB | 19 / 20 | flashing + serial monitor (see build flags) |
| BOOT / deep-sleep wake | 0 | active LOW |

Not populated, so these are pinned off: PCF8574 buttons, IR, buzzer, NeoPixel/RGB, GPS,
PN532 RFID/NFC, battery voltage divider.

`RX_PIN`/`TX_PIN` come from older hardware that carried a separate 433 MHz module. Nothing
reads `RX_PIN`, but `subghz.cpp` still toggles `TX_PIN` when the jammer starts and stops.
They stay on unconnected GPIO 8 — pointing `TX_PIN` at GPIO 4 would make the ESP32 drive
against the CC1101, because GDO0 is the radio's own output. The pin map check below asserts
that.

**Reserved by the module:** N16R8 has octal PSRAM, so **GPIO 33-37 are not usable**.
GPIO 3/45/46 are strapping pins and 19/20 are the native USB pair — none of them are
assigned to anything above.

## SPI topology (why the pins are split this way)

The ESP32-S3 exposes exactly **two** general-purpose SPI controllers, and this wiring needs
three buses' worth of devices — so the split is forced:

| Controller | Devices | Pins |
|---|---|---|
| **FSPI / SPI2** — the Arduino `SPI` object, which TFT_eSPI also uses because `USE_HSPI_PORT` is *not* defined | ST7789, SD, CC1101, NRF24 | 12 / 11 / 13 + 4 chip selects |
| **HSPI / SPI3** — `touchscreenSPI` in `Touchscreen.cpp` | XPT2046 | 38 / 39 / 40, CS 41 |

Consequence worth knowing: the CC1101 driver issues bare `SPI.transfer()` calls with no SPI
transaction, so it inherits whatever clock the last display write left on the bus. The CC1101
tops out at 10 MHz SPI, so `SPI_FREQUENCY` in `Libraries/User_Setup evras3.h` is capped at
**10 MHz**. If you build without the sub-GHz radio you can raise it to 27/40 MHz for a
snappier UI.

## Build and test

One script does everything the CI workflow does. It reads the selected board from
`BoardConfig.h`, checks the pin map, puts the matching TFT_eSPI `User_Setup.h` in place,
then compiles:

```bash
tools/build-and-test.sh                        # test + build
tools/build-and-test.sh --test-only            # pin map checks only (no toolchain needed)
tools/build-and-test.sh --install              # install the esp32 core + libraries first
tools/build-and-test.sh --dry-run              # print the commands, run nothing
tools/build-and-test.sh --upload --port /dev/ttyACM0
tools/build-and-test.sh --board v2             # build a different board profile
```

Add `--dry-run` to see the exact `arduino-cli` command without running it.

### What the test phase checks

`shared.h` only needs `<stdint.h>` and `BoardConfig.h`, so the resolved pin map compiles on
a desktop — no xtensa toolchain, no hardware. `tools/board-pins/run.sh` (called by the
script, and by CI before it compiles) prints the resolved settings for all four board
profiles and fails to build if `Libraries/User_Setup evras3.h` and `shared.h` disagree about
a pin, if two chip selects collide on the shared bus, if `TX_PIN` lands on a CC1101 GDO net,
or if anything is assigned to GPIO 33-37.

### Triggering CI

The GitHub workflow (`ESPForge firmware build`) only runs on a dispatch — a push alone
never builds anything. `tools/ci-run.sh` dispatches it for a branch and watches the result:

```bash
tools/ci-run.sh                 # dispatch for the current branch, watch, report
tools/ci-run.sh --no-watch      # dispatch and return
tools/ci-run.sh --list          # recent runs
tools/ci-run.sh --logs          # log of the latest run
tools/ci-run.sh --cancel        # cancel the latest in-progress run
tools/ci-run.sh --watch         # attach to a run already in progress
```

It needs `gh` authenticated as *you* — creating a dispatch needs the `actions: write`
permission. Artifacts from a run: `gh run download <id>`.

### What the build phase does

1. Copies `Libraries/User_Setup evras3.h` over the installed
   `~/Arduino/libraries/TFT_eSPI/User_Setup.h` (backing up the old one). A stale
   `User_Setup.h` compiles fine and drives the wrong panel, so this is not optional —
   pass `--no-sync-usersetup` only if you manage that file yourself.
2. Compiles with the board's FQBN and prints the resulting image size plus the matching
   `esptool.py` command.

Required libraries, if you install them by hand instead of using `--install`:

```bash
arduino-cli core install esp32:esp32@2.0.10
arduino-cli lib install 'PCF8574 library@2.3.7' 'Adafruit PN532@1.3.4' 'ArduinoJson@6.18.2' \
  'TFT_eSPI@2.5.43' 'XPT2046_Touchscreen@1.4' 'RF24@1.5.0' 'rc-switch@2.6.4' \
  'NimBLE-Arduino@1.4.2' 'IRremoteESP8266@2.8.6' 'arduinoFFT@1.6.2'
arduino-cli lib install --zip-path "Libraries/SmartRC-CC1101-Driver-Lib-master.zip"
```

The FQBN the script uses for this board, and why each option is there:

```
esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB,FlashMode=dio,CDCOnBoot=cdc
```

| Option | Why |
|---|---|
| `FlashSize=16M` | N16R8 has 16 MB flash |
| `PSRAM=opi` | the R8 PSRAM is **octal** — `PSRAM=enabled` (QSPI) will not initialise |
| `PartitionScheme=app3M_fat9M_16MB` | 3 MB app + OTA slot, fits the 16 MB flash |
| `CDCOnBoot=cdc` | `Serial` on the native USB port |

Override with `--fqbn "..."`; drop `CDCOnBoot=cdc` if you would rather have `Serial` on
UART0 (GPIO 43/44).

## Flashing

`tools/build-and-test.sh --upload --port /dev/ttyACM0`, or `arduino-cli upload -p
/dev/ttyACM0 --fqbn "<same fqbn>" ESP32-DIV`, or esptool directly. Flash offsets for the
ESP32-S3:

```bash
esptool.py --chip esp32s3 --port /dev/ttyACM0 write_flash \
  0x0     build/ESP32-DIV.ino.bootloader.bin \
  0x8000  build/ESP32-DIV.ino.partitions.bin \
  0xe000  ~/.arduino15/packages/esp32/hardware/esp32/2.0.10/tools/partitions/boot_app0.bin \
  0x10000 build/ESP32-DIV.ino.bin
```

If the port does not appear, hold **BOOT (GPIO 0)** while tapping **RESET** to enter
download mode.

## First boot

1. The panel is portrait (`TFT_ROTATION 0`). Flip it in `BoardConfig.h` only if you also
   adapt the menus — the main menu needs the full 320 px of height.
2. Run **Tools → Touch Calibrate** once. The saved values are keyed to profile `EVRAS3` in
   `/config/settings.json` on the SD card.
3. Format the SD card as FAT32. The firmware creates `/config`, `/logs` and `/captures`.

## Behaviour with missing hardware

| Menu entry | What happens on this board |
|---|---|
| IR Remote | Talks to unused GPIO 1/2. Harmless — no hang, nothing to capture. |
| RFID/NFC | `PN532` pins are `-1`; `getFirmwareVersion()` fails in ~100 ms and the feature reports no hardware. |
| GPS | UART RX/TX are `-1`, so the port keeps its default pins and never receives NMEA. |
| Battery icon | No voltage divider, so the status bar reads 0 %. Cosmetic. |
| Buttons | None. All navigation uses the on-screen touch nav bar. |

## nRF24 power

The E01-MLO1DP5 is a high-PA module. Power it from a **dedicated 3.3 V regulator** with a
common ground and 10-100 µF across its VCC/GND — not from the ESP32's 3V3 pin, or you get
brownouts and collapsed range. Only one module is wired here, so `CE_PIN_2/3` and
`CSN_PIN_2/3` are `-1`; features that spread work across three radios (Proto Kill, Scanner)
run on the single module instead.
