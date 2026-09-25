#!/usr/bin/env bash
# Compile-and-run check for the ESP32-DIV board pin maps.
#
# ESP32-DIV/shared.h only needs <stdint.h> + BoardConfig.h, so the resolved pin
# map for every board can be compiled and asserted on the host with a plain C++
# compiler - no ESP32 toolchain, no hardware. Wiring mistakes fail here.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(cd "$here/../.." && pwd)"
src="$root/ESP32-DIV"
libs="$root/Libraries"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

CXX="${CXX:-g++}"
flags="-std=gnu++17 -Wall -Wextra -I$src"

echo "== static wiring checks (BOARD_EVRAS3 pin map vs User_Setup evras3.h)"
$CXX $flags -I"$libs" -o "$tmp/check" "$here/check_evras3.cpp"
"$tmp/check"
echo "   ok"

for board in EVRAS3 V2 V1 CYD; do
  $CXX $flags -DUSE_BOARD_$board -o "$tmp/pinmap_$board" "$here/pinmap.cpp"
  "$tmp/pinmap_$board" > "$tmp/out_$board.txt"
  echo "== resolved BOARD_$board pin map ($(wc -l < "$tmp/out_$board.txt") settings)"
done

echo "== BOARD_EVRAS3 must match the Evrahim S3 wiring"
diff -u /dev/stdin "$tmp/out_EVRAS3.txt" <<'EXPECT'
ESP32DIV_BOARD_NAME      = Evrahim S3
BOARD_HAS_ESP32S3        = 1
HAS_PCF8574_BUTTONS      = 0
TFT_ROTATION             = 0
TOUCH_SHARES_TFT_SPI     = 0
TOUCH_PROFILE_ID         = EVRAS3
TOUCH_X_MIN              = 300
TOUCH_X_MAX              = 3800
TOUCH_Y_MIN              = 300
TOUCH_Y_MAX              = 3800
TFT_WIDTH                = 240
TFT_HEIGHT               = 320
PCF8574_AUTO_DETECT      = 1
PCF8574_I2C_ADDR         = 32
BTN_UP                   = 7
BTN_DOWN                 = 5
BTN_LEFT                 = 3
BTN_RIGHT                = 4
BTN_SELECT               = 6
BACKLIGHT_PIN            = 7
PWM_CHANNEL              = 0
PWM_FREQ                 = 5000
PWM_RESOLUTION           = 8
XPT2046_CS               = 41
XPT2046_MOSI             = 39
XPT2046_MISO             = 40
XPT2046_CLK              = 38
XPT2046_IRQ              = 42
SD_CS                    = 5
SD_MOSI                  = 11
SD_MISO                  = 13
SD_SCLK                  = 12
SD_CS_PIN                = 5
SD_CD                    = (undefined)
PN532_SCK                = -1
PN532_MISO               = -1
PN532_MOSI               = -1
PN532_SS                 = -1
RX_PIN                   = 8
TX_PIN                   = 8
GPS_UART_RX              = -1
GPS_UART_TX              = -1
CC1101_SCK               = 12
CC1101_MISO              = 13
CC1101_MOSI              = 11
CC1101_CS                = 21
SUBGHZ_RX_PIN            = 6
SUBGHZ_TX_PIN            = 4
CC1101_GDO0              = 4
CC1101_GDO2              = 6
CE_PIN_1                 = 47
CSN_PIN_1                = 48
CE_PIN_2                 = -1
CSN_PIN_2                = -1
CE_PIN_3                 = -1
CSN_PIN_3                = -1
NRF24_SCAN_CE            = 47
NRF24_SCAN_CSN           = 48
NRF24_SPI_SCK            = 12
NRF24_SPI_MISO           = 13
NRF24_SPI_MOSI           = 11
NRF24_SPI_SS             = 48
IR_RX_PIN                = 1
IR_TX_PIN                = 2
IR_DEFAULT_KHZ           = 38
BUZZER_PIN               = -1
BATTERY_ADC_PIN          = -1
ESP32DIV_MAX_WIFI_NETWORKS = 50
ESP32DIV_RFID_SRC_PAGES  = 256
FEATURE_BLE_DUCKY        = 1
EXPECT
echo "   ok"

echo
echo "All board pin-map checks passed."
