#pragma once

#include <stdint.h>

// Gesture recognition for the single button (docs/behavior.md).

enum class GestureEvent : uint8_t {
  None = 0,
  View,           // Short press, and the gap closed with no second press.
  SubtractOne,    // Second press of a double-press, or a later press in that chain.
  AddOne,         // Long-press threshold, then again every ADD_REPEAT_MS while held.
  OpenPairing,    // Held at max for PAIR_AT_MAX_MS.
  CancelPairing,  // Short press while the pairing window is open.
  ForgetBonds,    // Hold of PAIR_FORGET_MS after the finger lifted in pairing.
};

class ButtonGestures {
 public:
  void begin();

  // Main updates this after each store change so the pairing timer knows
  // when the count sits at max.
  void setAtMax(bool atMax);

  // Window closed by timeout or a completed bond. Leaves the recognizer idle.
  void endPairing();

  // After this returns, isBusy() is the state that follows the event.
  // isBusy() stays true through a subtract chain, including the gap spent
  // waiting for another press. It is false only while idle.
  GestureEvent poll(uint32_t nowMs);
  bool isBusy() const { return busy_; }

 private:
  enum class State : uint8_t {
    Idle,
    Held,
    Pending,
    Subtracting,
    Adding,
    Pairing,
    PairingHeld,
  };

  bool updateDebounced(uint32_t nowMs);
  GestureEvent onPressed(uint32_t nowMs);
  GestureEvent onReleased(uint32_t nowMs);
  GestureEvent pollWhileHeld(uint32_t nowMs);
  GestureEvent pollWhileUp(uint32_t nowMs);

  State state_ = State::Idle;
  bool busy_ = false;
  bool atMax_ = false;

  bool stablePressed_ = false;
  bool candidatePressed_ = false;
  uint32_t candidateSinceMs_ = 0;

  uint32_t pressAcceptedMs_ = 0;
  uint32_t releaseAcceptedMs_ = 0;
  uint32_t nextAddMs_ = 0;
  uint32_t pairDeadlineMs_ = 0;
  bool pairTimerArmed_ = false;
  bool openedPairingThisHold_ = false;
  bool needArmPairTimer_ = false;
};
