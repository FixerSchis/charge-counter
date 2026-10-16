#include "led_display.h"

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#include "board_gpio.h"
#include "board_pins.h"
#include "config.h"

namespace {

constexpr uint8_t kLampCount = 8;
constexpr uint32_t kRailSettleMs = 1;
constexpr uint32_t kPairHalfPeriodMs = 250;

// Feather variant maps Arduino pin N to P0.N, so PIN_LED_DATA is correct.
Adafruit_NeoPixel pixels(kLampCount, PIN_LED_DATA, NEO_GRB + NEO_KHZ800);

uint32_t colorLevel(uint8_t r, uint8_t g, uint8_t b) {
  return pixels.Color(r, g, b);
}

}  // namespace

void LedDisplay::powerOn() {
  board_pin_output_low(PIN_LED_DATA);
  board_pin_write(PIN_LED_EN, true);
  delay(kRailSettleMs);
}

void LedDisplay::powerOff() {
  board_pin_output_low(PIN_LED_DATA);
  board_pin_write(PIN_LED_EN, false);
}

void LedDisplay::sendFrame() {
  pixels.show();
  board_pin_output_low(PIN_LED_DATA);
}

void LedDisplay::fillOff() {
  for (uint8_t i = 0; i < kLampCount; i++) {
    pixels.setPixelColor(i, 0);
  }
}

void LedDisplay::fillBlueAll() {
  const uint32_t blue = colorLevel(0, 0, LED_LEVEL);
  for (uint8_t i = 0; i < kLampCount; i++) {
    pixels.setPixelColor(i, blue);
  }
}

void LedDisplay::fillCount(uint16_t count) {
  fillOff();
  if (count == 0) {
    return;
  }
  if (count > COUNT_HARD_CAP) {
    count = COUNT_HARD_CAP;
  }

  const uint32_t red = colorLevel(LED_LEVEL, 0, 0);
  const uint32_t green = colorLevel(0, LED_LEVEL, 0);

  if (count <= 8) {
    for (uint8_t i = 0; i < count; i++) {
      pixels.setPixelColor(i, red);
    }
    return;
  }

  // First pass all red, second pass turns lamps green from the start.
  for (uint8_t i = 0; i < 8; i++) {
    pixels.setPixelColor(i, red);
  }
  const uint8_t greenCount = static_cast<uint8_t>(count - 8);
  for (uint8_t i = 0; i < greenCount; i++) {
    pixels.setPixelColor(i, green);
  }
}

void LedDisplay::fillLocateStep(uint8_t index) {
  fillOff();
  pixels.setPixelColor(index, colorLevel(0, 0, LED_LEVEL));
}

void LedDisplay::begin() {
  board_pin_output_low(PIN_LED_EN);
  board_pin_output_low(PIN_LED_DATA);
  pixels.begin();
  pixels.setBrightness(255);
  fillOff();
  // Do not power the chain or send a frame at boot.
  powerOff();
  mode_ = Mode::Off;
  on_ = false;
  shown_ = 0;
}

void LedDisplay::show(uint16_t count) {
  resumeAfterLocate_ = Mode::Off;
  mode_ = Mode::Reading;
  shown_ = count;
  on_ = true;

  powerOn();
  fillBlueAll();
  sendFrame();
  delay(ACK_FLASH_MS);

  fillCount(count);
  sendFrame();
}

void LedDisplay::clear() {
  fillOff();
  if (mode_ == Mode::Reading || mode_ == Mode::Off) {
    // Drop a frame only when the rail is on.
    if (mode_ == Mode::Reading) {
      powerOn();
      sendFrame();
    }
    powerOff();
    mode_ = Mode::Off;
  }
  on_ = false;
  shown_ = 0;
}

void LedDisplay::startPairingBlink() {
  if (mode_ == Mode::Locate) {
    resumeAfterLocate_ = Mode::Pairing;
    return;
  }
  mode_ = Mode::Pairing;
  on_ = false;
  pairingLit_ = true;
  animNextMs_ = millis();
  powerOn();
  fillBlueAll();
  sendFrame();
}

void LedDisplay::stopPairingBlink() {
  if (mode_ == Mode::Locate && resumeAfterLocate_ == Mode::Pairing) {
    resumeAfterLocate_ = Mode::Off;
    return;
  }
  if (mode_ != Mode::Pairing) {
    return;
  }
  fillOff();
  sendFrame();
  powerOff();
  mode_ = Mode::Off;
}

void LedDisplay::startLocate() {
  if (mode_ == Mode::Pairing) {
    resumeAfterLocate_ = Mode::Pairing;
  } else if (mode_ == Mode::Reading) {
    resumeAfterLocate_ = Mode::Reading;
  } else {
    resumeAfterLocate_ = Mode::Off;
  }

  mode_ = Mode::Locate;
  locateIndex_ = 0;
  locateCycle_ = 0;
  animNextMs_ = millis();
  powerOn();
  fillLocateStep(0);
  sendFrame();
}

bool LedDisplay::isAnimating() const {
  return mode_ == Mode::Pairing || mode_ == Mode::Locate;
}

void LedDisplay::poll(uint32_t nowMs) {
  if (mode_ == Mode::Pairing) {
    if ((int32_t)(nowMs - animNextMs_) < 0) {
      return;
    }
    animNextMs_ = nowMs + kPairHalfPeriodMs;
    pairingLit_ = !pairingLit_;
    if (pairingLit_) {
      fillBlueAll();
    } else {
      fillOff();
    }
    sendFrame();
    return;
  }

  if (mode_ == Mode::Locate) {
    if ((int32_t)(nowMs - animNextMs_) < 0) {
      return;
    }
    animNextMs_ = nowMs + LOCATE_STEP_MS;
    locateIndex_++;
    if (locateIndex_ >= kLampCount) {
      locateIndex_ = 0;
      locateCycle_++;
      if (locateCycle_ >= LOCATE_CYCLES) {
        const Mode resume = resumeAfterLocate_;
        resumeAfterLocate_ = Mode::Off;
        if (resume == Mode::Pairing) {
          mode_ = Mode::Pairing;
          pairingLit_ = true;
          animNextMs_ = nowMs;
          fillBlueAll();
          sendFrame();
        } else if (resume == Mode::Reading) {
          mode_ = Mode::Reading;
          on_ = true;
          fillCount(shown_);
          sendFrame();
        } else {
          fillOff();
          sendFrame();
          powerOff();
          mode_ = Mode::Off;
        }
        return;
      }
    }
    fillLocateStep(locateIndex_);
    sendFrame();
  }
}
