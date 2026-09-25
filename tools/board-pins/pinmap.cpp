// Host-side dump of every pin/config macro that ESP32-DIV/shared.h resolves.
//
// shared.h only needs <stdint.h> and BoardConfig.h, so it compiles on the host.
// That makes the whole board pin map checkable without an xtensa toolchain.
// Pass -DUSE_BOARD_V1 / _V2 / _CYD / _EVRAS3 to force a board; see run.sh.
#include "BoardConfig.h"
// Force one board regardless of what BoardConfig.h selects.
#if defined(USE_BOARD_V1) || defined(USE_BOARD_V2) || defined(USE_BOARD_CYD) || defined(USE_BOARD_EVRAS3)
#undef BOARD_CYD
#undef BOARD_ESP32_DIV_V1
#undef BOARD_ESP32_DIV_V2
#undef BOARD_EVRAS3
#endif
#ifdef USE_BOARD_V1
#define BOARD_ESP32_DIV_V1
#endif
#ifdef USE_BOARD_V2
#define BOARD_ESP32_DIV_V2
#endif
#ifdef USE_BOARD_CYD
#define BOARD_CYD
#endif
#ifdef USE_BOARD_EVRAS3
#define BOARD_EVRAS3
#endif
#include "shared.h"
#include <cstdio>

#define P_INT(n)  std::printf("%-24s = %d\n", #n, (int)(n))
#define P_STR(n)  std::printf("%-24s = %s\n", #n, (n))

int main() {
  P_STR(ESP32DIV_BOARD_NAME);
  P_INT(BOARD_HAS_ESP32S3);
  P_INT(HAS_PCF8574_BUTTONS);
  P_INT(TFT_ROTATION);
  P_INT(TOUCH_SHARES_TFT_SPI);
  P_STR(TOUCH_PROFILE_ID);
  P_INT(TOUCH_X_MIN); P_INT(TOUCH_X_MAX); P_INT(TOUCH_Y_MIN); P_INT(TOUCH_Y_MAX);
  P_INT(TOUCH_INVERT_X); P_INT(TOUCH_INVERT_Y);
  P_INT(TFT_WIDTH); P_INT(TFT_HEIGHT);
  P_INT(PCF8574_AUTO_DETECT); P_INT(PCF8574_I2C_ADDR);
  P_INT(BTN_UP); P_INT(BTN_DOWN); P_INT(BTN_LEFT); P_INT(BTN_RIGHT); P_INT(BTN_SELECT);
  P_INT(BACKLIGHT_PIN); P_INT(PWM_CHANNEL); P_INT(PWM_FREQ); P_INT(PWM_RESOLUTION);
  P_INT(XPT2046_CS); P_INT(XPT2046_MOSI); P_INT(XPT2046_MISO); P_INT(XPT2046_CLK); P_INT(XPT2046_IRQ);
  P_INT(SD_CS); P_INT(SD_MOSI); P_INT(SD_MISO); P_INT(SD_SCLK); P_INT(SD_CS_PIN);
#ifdef SD_CD
  P_INT(SD_CD);
#else
  std::printf("%-24s = (undefined)\n", "SD_CD");
#endif
  P_INT(PN532_SCK); P_INT(PN532_MISO); P_INT(PN532_MOSI); P_INT(PN532_SS);
  P_INT(RX_PIN); P_INT(TX_PIN);
  P_INT(GPS_UART_RX); P_INT(GPS_UART_TX);
  P_INT(CC1101_SCK); P_INT(CC1101_MISO); P_INT(CC1101_MOSI); P_INT(CC1101_CS);
  P_INT(SUBGHZ_RX_PIN); P_INT(SUBGHZ_TX_PIN);
  P_INT(CC1101_GDO0); P_INT(CC1101_GDO2);
  P_INT(CE_PIN_1); P_INT(CSN_PIN_1); P_INT(CE_PIN_2); P_INT(CSN_PIN_2);
  P_INT(CE_PIN_3); P_INT(CSN_PIN_3);
  P_INT(NRF24_SCAN_CE); P_INT(NRF24_SCAN_CSN);
  P_INT(NRF24_SPI_SCK); P_INT(NRF24_SPI_MISO); P_INT(NRF24_SPI_MOSI); P_INT(NRF24_SPI_SS);
  P_INT(IR_RX_PIN); P_INT(IR_TX_PIN); P_INT(IR_DEFAULT_KHZ);
  P_INT(BUZZER_PIN); P_INT(BATTERY_ADC_PIN);
  P_INT(ESP32DIV_MAX_WIFI_NETWORKS); P_INT(ESP32DIV_RFID_SRC_PAGES);
  P_INT(FEATURE_BLE_DUCKY);
  return 0;
}
