#pragma once

#include <stdint.h>

// Gesture recognition for the single button. The state machine, timings,
// and scenarios are in docs/behavior.md. poll() does not implement them yet.

enum class GestureEvent : uint8_t {
  None = 0,
  View,          // Short press, and the gap closed with no second press.
  SubtractOne,   // Second press of a double-press, or a later press in that chain.
  AddOne,        // Long-press threshold, then again every ADD_REPEAT_MS while held.
};

class ButtonGestures {
 public:
  void begin();

  // After this returns, isBusy() is the state that follows the event.
  // isBusy() stays true through a subtract chain, including the gap spent
  // waiting for another press. It is false only while idle.
  GestureEvent poll(uint32_t nowMs);
  bool isBusy() const { return busy_; }

 private:
  bool busy_ = false;
};
