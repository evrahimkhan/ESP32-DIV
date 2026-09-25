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

### CI

`.github/workflows/build-test.yml` builds and tests on GitHub on **every push** to any
branch and on pull requests to `main`, so pushing is enough to get a compile:

| job | what it does |
| --- | --- |
| `pinmap` | host-side pin map checks for all four board profiles; no toolchain, fails in seconds |
| `firmware` (matrix: `evras3`) | installs arduino-cli + ESP32 core 2.0.10, selects the board macro, drops in that board's `User_Setup.h`, runs the pin map check, compiles the merged image, uploads the artifacts |

The matrix builds **this board only** — the ESP32-DIV v2 job was dropped from it. Adding a
board back is one entry in the matrix (`board`, `macro`, `sketch`, `fqbn`; all four are in
`tools/build-and-test.sh`). The `pinmap` job still checks every board profile's pin map on
the host, which costs about a second and needs no toolchain, so a change that breaks
another board's wiring is still caught before it compiles.

A compile failure is published as annotations on the run and in the job summary, so the
compiler errors are visible without opening the raw log.

`tools/ci-run.sh` starts a run and reports the result — nothing is compiled on your
machine:

```bash
tools/ci-run.sh                 # start a run for the current branch, watch, report
tools/ci-run.sh --no-watch      # start it and print the run URL
tools/ci-run.sh --via push      # start it via the ci-run-* tag trigger
tools/ci-run.sh --download out  # fetch the artifacts when it succeeds
tools/ci-run.sh --list          # recent runs
tools/ci-run.sh --logs          # log of the latest run
tools/ci-run.sh --cancel        # cancel the latest in-progress run
tools/ci-run.sh --watch         # attach to a run already in progress
tools/ci-run.sh --run 123456    # report on one specific run
tools/ci-run.sh --workflow espforge-build.yml   # the ESPForge workflow instead
tools/ci-run.sh --local [...]   # compile here instead (build-and-test.sh)
```

There are two ways to start a run and the script picks one:

| trigger | needs | notes |
| --- | --- | --- |
| `workflow_dispatch` | a token with `actions: write` | what `gh auth login` as yourself gives you |
| the `ci-run-*` tag | only the right to push | used automatically when the dispatch is refused with 403; the tag is deleted again on exit |

The exit status is the run's: `0` when it succeeds, `1` when it fails or times out.
Artifacts from a run: `gh run download <id>`.

The older `ESPForge firmware build` workflow is still there, but it only runs on a
`workflow_dispatch`/`repository_dispatch` from ESPForge.

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

## Source changes this board profile needed

Three edits to the shared firmware. None of them changes behaviour on the other boards —
the first two only matter when the ILI9341 macros or a second nRF24 are absent:

| file | change | why |
| --- | --- | --- |
| `ESP32-DIV/utils.cpp` | `ILI9341_VSCRDEF`/`ILI9341_VSCRSADD` → `LCD_CMD_VSCRDEF`/`LCD_CMD_VSCRSADD` (`0x33`/`0x37`) | TFT_eSPI only defines those names for the controller in use, so an ST7789 build never sees the `ILI9341_*` spelling and did not compile. The MIPI DCS values are the same for ILI9341, ST7789 and ST7796. |
| `ESP32-DIV/utils.cpp` | `#if defined(CE_PIN_3)` → `#if defined(CE_PIN_3) && (CE_PIN_3 >= 0)` | this board has a single nRF24, so the second module's pins are `-1`; the old test treated them as present |
| `ESP32-DIV/Touchscreen.cpp` | map raw touch onto `tft.width()`/`tft.height()` instead of `TFT_WIDTH`/`TFT_HEIGHT` | identical in portrait; still correct if `TFT_ROTATION` is set to 1/3, where the panel reports 320x240 |

## Flash settings

The build and the flash have to agree, so these are the same settings CI compiles with:

```
esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB,FlashMode=dio,CDCOnBoot=cdc
```

In the Arduino IDE: **Board = "ESP32S3 Dev Module"**, then under Tools —

| Tools entry | Value | Why |
| --- | --- | --- |
| USB CDC On Boot | **Enabled** | native USB (GPIO19/20) is used for flashing, the serial console and BadUSB |
| Flash Size | 16MB (128Mb) | the N16 in N16R8 |
| PSRAM | **OPI PSRAM** | the R8 is octal PSRAM; QSPI leaves it uninitialised |
| Partition Scheme | 3M App/9.9MB FATFS | `app3M_fat9M_16MB`: 3 MB per app slot, room for the 1.7 MB image with OTA |
| Flash Mode | DIO | what the FQBN pins; DIO is safe on this module regardless of strapping |
| Upload Speed | 921600 (drop to 460800 if it is unreliable) | |
| USB Mode | Hardware CDC and JTAG | default, and what the native port needs |
| Flash Frequency | 80MHz | default |
| CPU Frequency | 240MHz (WiFi/BT) | default |
| Erase All Flash Before Sketch Upload | **Enabled for the first flash**, Disabled afterwards | the first flash of a board (or any flash after changing the partition scheme, flash size, PSRAM mode or CDC setting) must erase: a partition table, NVS block or PHY calibration left over from a different layout makes the WiFi/BLE stack abort during boot |

The 9.9 MB FATFS partition is unused (the firmware talks to the SD card itself) and is
harmless. Do **not** pick the `ESP32-S3-USB-OTG` board variant.

To flash:

```bash
tools/build-and-test.sh --board evras3 --upload --port /dev/ttyACM0   # build + upload
arduino-cli upload -p /dev/ttyACM0 --input-dir firmware-output \
  --fqbn "esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB,FlashMode=dio,CDCOnBoot=cdc" \
  ESP32-DIV
```

#### One-shot image (what to use when flashing by hand)

Every CI build also produces a **merged image** — bootloader + partition table +
`boot_app0` + application in one file, so one write is the whole firmware:

```
firmware-output/ESP32-DIV-evras3-merged.bin
```

Flash it at offset **`0x0`**:

```bash
esptool.py --chip esp32s3 --port /dev/ttyACM0 write_flash 0x0 ESP32-DIV-evras3-merged.bin
```

`0x0` is right for the ESP32-S3 because that is where its bootloader lives (the classic
ESP32 puts it at `0x1000`). The file is the same layout the four separate writes below
produce: `0x0` bootloader, `0x8000` partitions, `0xe000` `boot_app0`, `0x10000` app,
`0xff` in between. To build it locally instead of downloading it:

```bash
tools/build-and-test.sh --board evras3 --merged      # writes build/evras3/ESP32-DIV-evras3-merged.bin
```

#### Flashing by hand, piece by piece

`arduino-cli upload` runs esptool for you, which is what you want: it also writes
`boot_app0.bin`, a file that is **not** in the CI artifact. Only if you flash by hand:

```bash
esptool.py --chip esp32s3 --port /dev/ttyACM0 write_flash \
  0x0     build/evras3/ESP32-DIV.ino.bootloader.bin \
  0x8000  build/evras3/ESP32-DIV.ino.partitions.bin \
  0xe000  ~/.arduino15/packages/esp32/hardware/esp32/2.0.10/tools/partitions/boot_app0.bin \
  0x10000 build/evras3/ESP32-DIV.ino.bin
```

(Or skip all four writes and use the merged image above.)

If the port does not appear, hold **BOOT (GPIO 0)** while tapping **RESET** to enter
download mode; it then shows up as `/dev/ttyACM0` (Linux), `/dev/cu.usbmodem*` (macOS) or
`COMx` (Windows). GPIO 0 is also the deep-sleep wake pin, so holding BOOT at power-on is
normal and does not disturb the firmware. The CDC port re-enumerates on the first reset
after flashing, and the boot log runs at 115200.

## First boot

1. The panel is portrait (`TFT_ROTATION 0`). Flip it in `BoardConfig.h` only if you also
   adapt the menus — the main menu needs the full 320 px of height.
2. Run **Tools → Touch Calibrate** once. The saved values are keyed to profile `EVRAS3` in
   `/config/settings.json` on the SD card.
3. Format the SD card as FAT32. The firmware creates `/config`, `/logs` and `/captures`.

### First flash of a new board

If the board does not come up, erase it before flashing the firmware. `esptool.py erase_flash`
(or tick *Erase All Flash Before Sketch Upload* once) clears the partition table, NVS **and**
the RF calibration that the WiFi/BLE stack reads at start-up — leftovers there are a common
cause of a boot loop that looks like a panic, and they survive every ordinary re-flash:

```bash
esptool.py --chip esp32s3 --port /dev/ttyACM0 erase_flash
tools/build-and-test.sh --board evras3 --upload --port /dev/ttyACM0
```

Keep the tick box off for all later flashes so NVS and your touch calibration survive.

## If it reboots instead of showing the menu

A board that resets while booting shows the logo again and says nothing else, so the
firmware now reports its own failure. `setup()` records the stage it is in to RTC memory
(that survives a panic, watchdog and usually a brownout) and the next boot prints the
reset reason and the stage the last one stopped at — on the panel as well as on USB
serial:

```
BOOT DIAGNOSTIC
reason: BROWNOUT (supply sagged)
last boot stopped at: BLE stack
```

Reading it:

| what it says | what it means |
| --- | --- |
| `reason: BROWNOUT` | the 3V3 rail sagged below ~2.8 V. Power the board from a supply that can actually deliver the load, check the nRF24 is on its own regulator, and see whether the panel's backlight is being driven straight from GPIO 7. |
| `reason: crash (panic)` | a real bug or a bad pin. The stage names the guilty subsystem. |
| `reason: task watchdog` | something blocked for seconds — usually an SD card that is absent or wired wrong. |
| `no PSRAM found` | the module's PSRAM is not initialising. On an N16R8 the setting has to be **OPI PSRAM**; if the chip is silently running without PSRAM, the BLE stack can then die from the memory loss. |
| `last boot stopped at: …` | the last stage that ran. The crash is in the stage *after* it. |

The same information goes to the USB serial console, at 115200:

```bash
arduino-cli monitor -p /dev/ttyACM0 -c baudrate=115200
# or: screen /dev/cu.usbmodem* 115200        (macOS)
# or: python3 -m serial.tools.miniterm COM5 115200   (Windows)
```

Serial is on the native USB port because the build sets `CDCOnBoot=cdc`. If nothing appears
there, the ROM bootloader still reports the reset reason on UART0 (GPIO 43/44):

```
rst:0xf (BROWN_OUT_RST),boot:0x8 (SPI_FAST_FLASH_BOOT)
```

**Safe mode.** Hold the touch panel down while powering on (the XPT2046 pulls its IRQ line
low when touched) and this boot skips the BLE stack, the WiFi/BLE scan tasks and Ducky.
The panel then says `SAFE MODE - radios skipped`. If the board reaches the menu that way,
the radio stack is where it dies — and in a pinch you still get a usable device.

## Touch screen

The XPT2046 sits on its own bus (SCK 38, MOSI 39, MISO 40, CS 41, IRQ 42), so the
display's SPI traffic never disturbs it. Two firmware details decide whether touch works at
all, and both are visible on screen:

1. **`Tools → Touch Test`** shows, live: the IRQ pin state, the raw controller values and
   the mapped screen position, with a marker where the touch landed. It saves nothing, so
   it is safe to open before calibrating.
2. **`Tools → Touch Calibrate`** reads the four corners and stores the result. It prints the
   same live values under each target, and says where the calibration went.

Reading the numbers:

| what you see on Touch Test | what it means |
| --- | --- |
| `irq pin 42 : LOW - pressed` and raw values that change | the controller is being read; if taps land in the wrong place, calibrate |
| `irq pin 42 : high - idle` even while pressing | the IRQ line is not reaching GPIO 42. The XPT2046 library only reads the panel from a falling-edge **interrupt** on that line, so nothing works until it is wired. |
| `irq pin -1` | the build has no IRQ pin for this board |
| raw values stuck at 0 while pressing | same as above — no sample was taken |
| values move but the marker appears mirrored or swapped | the mapping needs calibrating; run Touch Calibrate |

**Calibration is kept in NVS**, not only on the SD card, so it survives a reboot with no
card in the slot — historically `Save FAILED` meant exactly that, because the settings file
lives at `/config/settings.json` on the card. With a card present it is written to both.

### If touch is mirrored ("upside down")

A panel can report either direction on either axis, and the firmware used to hardcode the
direction, which mapped this panel vertically flipped: touching the top-left landed on the
bottom-left, and top-right landed on bottom-right. The direction is now a per-board setting
in `ESP32-DIV/shared.h`:

| macro | EVRAS3 | v1 / v2 | meaning |
| --- | --- | --- | --- |
| `TOUCH_INVERT_X` | 0 | 0 | 1 = raw X grows towards the right of the screen |
| `TOUCH_INVERT_Y` | **0** | **1** | 1 = raw Y grows towards the **bottom** of the screen |

EVRAS3 now defaults to the usual "raw grows downwards/rightwards" panel. If your panel is
wired the other way (touch mirrored vertically, or horizontally), flip the matching macro —
and **run Touch Calibrate again**, because the stored limits are read in that order. The
calibration itself measures each edge, so it adapts to either direction; the macro decides
which way round the pair is stored and what an uncalibrated board does.

`tools/board-pins/run.sh` now checks the round trip for every board: it simulates a panel of
the declared direction, runs the calibration arithmetic, and fails if any corner maps to the
wrong one — so a mapping that disagrees with the settings cannot ship.

```bash
# what the panel reports, over USB serial, while you press it
arduino-cli monitor -p /dev/ttyACM0 -c baudrate=115200
```

## Behaviour with missing hardware

This board has **no SD card-detect switch** (GPIO 38 is the touch clock), unlike the V2
board which senses card presence on GPIO 38. A missing card is therefore found by probing
the bus, and each probe ends by tearing the shared SPI bus down and rebuilding it — so the
firmware probes **once per boot** and then stops, printing `[sd] no card - not probing
again this boot`. Open an SD-backed feature (Ducky, captures, logs) to retry.

That deferral matters: the classic-ESP32 build does not mount SD at boot at all, for the
same reason, and its own comment records that boot mounting on a card-less board *"was
rebooting right after the intro"*.

Because there is no switch to say a card is missing, this board also skips the *destructive*
part of the mount — the one that calls `SD.end()` and `SPI.end()`/`SPI.begin()` on the bus
TFT_eSPI is using. At boot that bus is not reclaimed at all; `settingsLoad()` mounts the
card through a soft remount a moment later, so a card in the slot still loads your
settings and touch calibration. Without one:

```
[sd] mount failed - not probing again this boot
```

and the display, touch and radios are unaffected. Open an SD-backed feature to retry.

**Two firmware bugs this board found outright**, both of which reset it during boot:

| bug | why it only bit this board |
| --- | --- |
| `gpio_reset_pin((gpio_num_t)PN532_SS)` and the three other PN532 pins were called with values of **-1** | the board has no PN532, so `shared.h` defines those pins as `-1` — and `#if defined(PN532_SCK)` is *true* for a defined-as-minus-1 macro. IDF's `gpio_reset_pin()` validates with `GPIO_IS_VALID_GPIO()`, a plain comparison that `-1` passes, then writes the pin matrix at that offset. Every other board has a real PN532 header (v1/v2/cyd) or does not compile the block, so only EVRAS3 reached it. All such calls now go through `sdResetPin()`, which skips negative pins — and the test suite fails the build if a raw `gpio_reset_pin()` comes back. |
| the boot mount probed a card-less bus three times and reclaimed the shared SPI bus each time | no card-detect switch, so the `no card → return` short circuit never fired. Fixed by probing once and never reclaiming without a switch. |


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
