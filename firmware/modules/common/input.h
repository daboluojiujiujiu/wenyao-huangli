#pragma once
#include <stdint.h>

namespace Navigation {
enum Event { NONE, A_SHORT, A_LONG, B_SHORT, B_LONG };

class Reader {
 public:
  static constexpr uint32_t B_HOLD_MS = 1000;
  void suppressUntilReleased() { blocked = true; active = 0; stable = 0; }

  Event poll(uint32_t now, bool a, bool b) {
    const uint8_t raw = (a ? 1 : 0) | (b ? 2 : 0);
    if (!initialized) {
      initialized = true;
      lastRaw = raw;
      changedAt = now;
      blocked = raw != 0;
    }
    if (raw != lastRaw) { lastRaw = raw; changedAt = now; }
    // Overlapping keys never execute two different actions after release.
    if (raw == 3 || (active != 0 && raw != 0 && raw != active)) blocked = true;
    if (blocked) {
      if (raw == 0 && uint32_t(now - changedAt) >= 30) {
        blocked = false;
        active = stable = 0;
      }
      return NONE;
    }
    // Deliver B while held, then consume the entire gesture including release.
    if (active == 2 && raw == 2 && stable == 2 &&
        uint32_t(now - changedAt) >= 30 && uint32_t(now - pressedAt) >= B_HOLD_MS) {
      blocked = true;
      return B_LONG;
    }
    if (raw == stable || uint32_t(now - changedAt) < 30) return NONE;
    stable = raw;
    if (raw != 0) { active = raw; pressedAt = changedAt; return NONE; }
    const uint8_t key = active;
    active = 0;
    const bool held = uint32_t(changedAt - pressedAt) >= (key == 2 ? B_HOLD_MS : 800);
    if (key == 1) return held ? A_LONG : A_SHORT;
    if (key == 2) return held ? B_LONG : B_SHORT;
    return NONE;
  }

 private:
  bool initialized = false, blocked = false;
  uint8_t lastRaw = 0, stable = 0, active = 0;
  uint32_t changedAt = 0, pressedAt = 0;
};
}
