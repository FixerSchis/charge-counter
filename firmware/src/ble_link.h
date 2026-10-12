#pragma once

#include <stdint.h>

// BLE peripheral. The GATT layout, bonding rules, and write rules are in
// docs/ble.md. None of that is implemented yet.
class BleLink {
 public:
  // Return false to reject the write (out of range or wrong length is
  // handled inside the characteristic callback before this runs).
  using MaxWriteHandler = bool (*)(uint16_t requestedMax);

  void begin(const char* deviceName);
  void poll();

  void notifyCount(uint16_t count);
  void notifyMax(uint16_t maximum);
  void onMaxWrite(MaxWriteHandler handler);

  // True when the stack has been polled and can wait for the next radio
  // or GPIO event. A connected phone does not by itself make this false.
  bool canSleep() const { return canSleep_; }

 private:
  MaxWriteHandler onMaxWrite_ = nullptr;
  bool canSleep_ = true;
};
