// Host-side check of the touch calibration/mapping round trip.
//
// A panel can report either axis direction, and the firmware used to hardcode
// "non-CYD inverts Y" - which mapped the EVRAS3 panel upside down. The mapping
// and the calibration now read TOUCH_INVERT_X / TOUCH_INVERT_Y, so this test
// proves the two stay consistent: take raw corner samples from a simulated
// panel of the direction this board declares, run them through the same
// arithmetic TouchCalib uses when saving, then map the corners back and require
// every one to land in the right place.
//
// Build with -DUSE_BOARD_<BOARD>, like pinmap.cpp.
#include "BoardConfig.h"
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
#include <cstdlib>

// Arduino's map(): linear interpolation, and it handles in_min > in_max.
static long amap(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static int failures = 0;

static void expectNear(const char* what, long got, long want) {
  const long tolerance = 2;  // integer division slack
  if (labs(got - want) > tolerance) {
    std::printf("   FAIL %-28s got %ld, want %ld\n", what, got, want);
    failures++;
  }
}

int main() {
  const long maxX = TFT_WIDTH - 1;
  const long maxY = TFT_HEIGHT - 1;

  // A simulated panel: the raw reading at each screen edge, in the direction
  // this board's TOUCH_INVERT_* settings say the controller runs.
  const uint16_t rawLeft   = TOUCH_INVERT_X ? 3800 : 200;
  const uint16_t rawRight  = TOUCH_INVERT_X ? 200 : 3800;
  const uint16_t rawTop    = TOUCH_INVERT_Y ? 3800 : 220;
  const uint16_t rawBottom = TOUCH_INVERT_Y ? 220 : 3800;

  // What TouchCalib samples: the four targets, two per edge.
  const uint16_t xs[4] = {rawLeft, rawRight, rawRight, rawLeft};   // TL, TR, BR, BL
  const uint16_t ys[4] = {rawTop, rawTop, rawBottom, rawBottom};

  // Same arithmetic as TouchCalib::loop() when it stores the result.
  const uint16_t storedXMin = TOUCH_INVERT_X ? (uint16_t)(((uint32_t)xs[1] + xs[2]) / 2)
                                             : (uint16_t)(((uint32_t)xs[0] + xs[3]) / 2);
  const uint16_t storedXMax = TOUCH_INVERT_X ? (uint16_t)(((uint32_t)xs[0] + xs[3]) / 2)
                                             : (uint16_t)(((uint32_t)xs[1] + xs[2]) / 2);
  const uint16_t storedYMin = TOUCH_INVERT_Y ? (uint16_t)(((uint32_t)ys[2] + ys[3]) / 2)
                                             : (uint16_t)(((uint32_t)ys[0] + ys[1]) / 2);
  const uint16_t storedYMax = TOUCH_INVERT_Y ? (uint16_t)(((uint32_t)ys[0] + ys[1]) / 2)
                                             : (uint16_t)(((uint32_t)ys[2] + ys[3]) / 2);

  // ...and the mapping in Touchscreen.cpp, reading those stored values.
  auto mapX = [&](long raw) {
#if TOUCH_INVERT_X
    return amap(raw, storedXMax, storedXMin, 0, maxX);
#else
    return amap(raw, storedXMin, storedXMax, 0, maxX);
#endif
  };
  auto mapY = [&](long raw) {
#if TOUCH_INVERT_Y
    return amap(raw, storedYMax, storedYMin, 0, maxY);
#else
    return amap(raw, storedYMin, storedYMax, 0, maxY);
#endif
  };

  std::printf("   TOUCH_INVERT_X=%d TOUCH_INVERT_Y=%d\n", TOUCH_INVERT_X, TOUCH_INVERT_Y);

  // After a calibration, every corner has to land on its own corner.
  expectNear("left edge -> x 0",         mapX(rawLeft), 0);
  expectNear("right edge -> x max",      mapX(rawRight), maxX);
  expectNear("top edge -> y 0",          mapY(rawTop), 0);
  expectNear("bottom edge -> y max",     mapY(rawBottom), maxY);

  // And with no calibration at all, the compiled-in defaults must at least be
  // the right way up: top smaller than bottom on screen.
  const long defTop    = amap(rawTop, TOUCH_INVERT_Y ? TOUCH_Y_MAX : TOUCH_Y_MIN,
                                      TOUCH_INVERT_Y ? TOUCH_Y_MIN : TOUCH_Y_MAX, 0, maxY);
  const long defBottom = amap(rawBottom, TOUCH_INVERT_Y ? TOUCH_Y_MAX : TOUCH_Y_MIN,
                                          TOUCH_INVERT_Y ? TOUCH_Y_MIN : TOUCH_Y_MAX, 0, maxY);
  const long defLeft   = amap(rawLeft, TOUCH_INVERT_X ? TOUCH_X_MAX : TOUCH_X_MIN,
                                        TOUCH_INVERT_X ? TOUCH_X_MIN : TOUCH_X_MAX, 0, maxX);
  const long defRight  = amap(rawRight, TOUCH_INVERT_X ? TOUCH_X_MAX : TOUCH_X_MIN,
                                          TOUCH_INVERT_X ? TOUCH_X_MIN : TOUCH_X_MAX, 0, maxX);
  if (!(defTop < defBottom)) {
    std::printf("   FAIL default profile is upside down: top %ld >= bottom %ld\n", defTop, defBottom);
    failures++;
  }
  if (!(defLeft < defRight)) {
    std::printf("   FAIL default profile is mirrored: left %ld >= right %ld\n", defLeft, defRight);
    failures++;
  }
  std::printf("   defaults on an uncalibrated panel: x %ld..%ld  y %ld..%ld\n",
              defLeft, defRight, defTop, defBottom);

  if (failures) {
    std::printf("   %d touch mapping check(s) failed\n", failures);
    return 1;
  }
  std::printf("   ok\n");
  return 0;
}
