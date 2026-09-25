#pragma once
// Touch calibration maths that does not need the Arduino core, so the host
// tests can exercise it (tools/board-pins/check_touch_map.cpp).
//
// Convention used everywhere in this firmware: touchXMin is the raw value
// measured where the screen's x=0 edge is, touchXMax where x=max is, and the
// same for Y. The raw values may be in either order - a panel can report
// either direction, and Arduino's map() already interpolates a reversed pair -
// so nothing else in the firmware needs to know which way the panel runs.
#include <stdint.h>

struct TouchLimits {
  uint16_t xMin;
  uint16_t xMax;
  uint16_t yMin;
  uint16_t yMax;
};

static inline uint16_t touchClampRaw(long v) {
  if (v < 0) return 0;
  if (v > 4095) return 4095;
  return (uint16_t)v;
}

/** Raw limits from two taps: one near the top-left, one near the bottom-right.
 *
 *  Each tap gives a raw reading and the screen position it was aimed at, which
 *  is enough to extrapolate the raw value at both screen edges - and the sign
 *  of each axis tells us which way the panel runs, so no direction has to be
 *  configured. Used by the two-tap first-run setup, so a fresh board is
 *  correct without anyone guessing its wiring.
 */
static inline TouchLimits touchLimitsFromTwoPoints(int16_t rawX1, int16_t rawY1, int x1, int y1,
                                                  int16_t rawX2, int16_t rawY2, int x2, int y2,
                                                  int maxX, int maxY) {
  TouchLimits r = {0, 4095, 0, 4095};

  const int screenSpanX = x2 - x1;   // targets are ordered left to right
  const int screenSpanY = y2 - y1;   // ...and top to bottom
  if (screenSpanX == 0 || screenSpanY == 0) {
    return r;
  }

  // Raw units per screen pixel, then walk out to both screen edges.
  const long rawPerPxX = (long)(rawX2 - rawX1) / screenSpanX;
  const long rawPerPxY = (long)(rawY2 - rawY1) / screenSpanY;

  r.xMin = touchClampRaw((long)rawX1 - rawPerPxX * x1);
  r.xMax = touchClampRaw((long)rawX2 + rawPerPxX * (maxX - x2));
  r.yMin = touchClampRaw((long)rawY1 - rawPerPxY * y1);
  r.yMax = touchClampRaw((long)rawY2 + rawPerPxY * (maxY - y2));
  return r;
}

/** Screen position for a raw reading, by the same convention. */
static inline long touchMapRaw(int raw, uint16_t storedMin, uint16_t storedMax, long outMax) {
  const long inSpan = (long)storedMax - (long)storedMin;
  if (inSpan == 0) {
    return 0;
  }
  return ((long)raw - (long)storedMin) * outMax / inSpan;
}
