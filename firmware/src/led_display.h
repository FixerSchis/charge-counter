#pragma once

#include <stdint.h>

// Eight addressable RGB lamps. Encoding is in docs/behavior.md.
class LedDisplay {
 public:
  void begin();

  // Blue ACK flash, then the red-then-green count pattern. Restarts the
  // reading timer basis in the caller. Pairing blink yields to this, and
  // resumes after clear() only if startPairingBlink() is called again.
  void show(uint16_t count);
  void clear();

  void startPairingBlink();
  void stopPairingBlink();
  void startLocate();

  // Drive pairing blink and locate chase. Call every loop.
  void poll(uint32_t nowMs);

  // True while a count reading is on the lamps (after show, before clear).
  bool isOn() const { return on_; }
  uint16_t shown() const { return shown_; }

  // True while pairing blink or locate is running. Blocks idle sleep.
  bool isAnimating() const;

 private:
  enum class Mode : uint8_t {
    Off,
    Reading,
    Pairing,
    Locate,
  };

  void powerOn();
  void powerOff();
  void sendFrame();
  void fillOff();
  void fillBlueAll();
  void fillCount(uint16_t count);
  void fillLocateStep(uint8_t index);

  Mode mode_ = Mode::Off;
  Mode resumeAfterLocate_ = Mode::Off;
  bool on_ = false;
  uint16_t shown_ = 0;

  uint32_t animNextMs_ = 0;
  bool pairingLit_ = false;
  uint8_t locateIndex_ = 0;
  uint8_t locateCycle_ = 0;
};
