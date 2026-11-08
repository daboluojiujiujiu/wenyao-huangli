#pragma once
#include <stdint.h>

namespace Battery {
static constexpr uint32_t SAMPLE_MS = 2000;
enum class State { UNKNOWN, BATTERY, LOW_INPUT, CHARGE_DISABLED, CHARGING, NOT_CHARGING };

struct Reading {
  int batteryMv = -1;
  int inputMv = -1;
  int gpioBits = -1;
  int enabled = -1;
};

// StickS3 PM1: GPIO0 is active-low CHRG. DONE is not connected to PM1.
// Use read APIs with success flags: a failed I2C read must not mean charging.
template<class PM1> Reading read(PM1& pm1) {
  Reading r;
  uint16_t mv = 0;
  uint8_t input[2] = {}, bits = 0;
  bool enabled = false;
  if (pm1.getBatteryVoltage(&mv)) r.batteryMv = mv;
  if (pm1.readRegister(0x24, input, 2)) r.inputMv = (input[1] << 8) | input[0];
  if (pm1.getGPIOInputBits(&bits)) r.gpioBits = bits;
  if (pm1.getBatteryCharge(&enabled)) r.enabled = enabled ? 1 : 0;
  return r;
}

inline bool validBattery(int mv) { return mv >= 2500 && mv <= 4350; }
inline bool validInput(int mv) { return mv >= 0 && mv <= 6000; }
inline State classify(const Reading& r) {
  if (!validInput(r.inputMv)) return State::UNKNOWN;
  if (r.inputMv < 1000) return State::BATTERY;
  if (r.inputMv <= 4400) return State::LOW_INPUT;
  if (r.enabled < 0 || r.gpioBits < 0 || !validBattery(r.batteryMv)) return State::UNKNOWN;
  if (!r.enabled) return State::CHARGE_DISABLED;
  return (r.gpioBits & 1) ? State::NOT_CHARGING : State::CHARGING;
}

inline const char* label(State s) {
  switch (s) {
    case State::BATTERY: return "电池供电";
    case State::LOW_INPUT: return "电源偏低";
    case State::CHARGE_DISABLED: return "充电关闭";
    case State::CHARGING: return "正在充电";
    case State::NOT_CHARGING: return "未在充电";
    default: return "状态未知";
  }
}
inline const char* code(State s) {
  switch (s) {
    case State::BATTERY: return "BATTERY";
    case State::LOW_INPUT: return "LOW_INPUT";
    case State::CHARGE_DISABLED: return "DISABLED";
    case State::CHARGING: return "CHARGING";
    case State::NOT_CHARGING: return "NOT_CHARGING";
    default: return "UNKNOWN";
  }
}

inline int estimate(float mv) {
  struct Point { int mv; int pct; };
  static constexpr Point curve[] = {
    {3400,0},{3550,10},{3650,20},{3700,30},{3740,40},{3790,50},
    {3870,60},{3950,70},{4030,80},{4110,90},{4200,100}
  };
  if (mv <= 3400) return 0;
  if (mv >= 4200) return 100;
  for (int i=1; i<11; ++i) {
    if (mv <= curve[i].mv)
      return int(curve[i-1].pct + (mv-curve[i-1].mv)*10/(curve[i].mv-curve[i-1].mv));
  }
  return -1;
}

struct Monitor {
  Reading raw;
  State state = State::UNKNOWN;
  int percent = -1;
  float filteredMv = -1;
  uint32_t sampledAt = 0;
  bool sampled = false;
  bool due(uint32_t now) const { return !sampled || uint32_t(now-sampledAt) >= SAMPLE_MS; }
  void update(uint32_t now, const Reading& r) {
    const bool sourceChanged = (raw.inputMv > 4400) != (r.inputMv > 4400);
    raw = r;
    state = classify(r);
    sampledAt = now;
    sampled = true;
    if (!validBattery(r.batteryMv)) {
      filteredMv = -1;
      percent = -1;
    } else {
      if (filteredMv < 0 || sourceChanged) filteredMv = r.batteryMv;
      else filteredMv += (r.batteryMv-filteredMv)*0.25f;
      percent = estimate(filteredMv);
    }
  }
};
}
