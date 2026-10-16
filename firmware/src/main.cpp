#include <Arduino.h>
#include <string.h>

#include "ble_link.h"
#include "button_gestures.h"
#include "config.h"
#include "counter_store.h"
#include "led_display.h"
#include "power_mgmt.h"

static CounterStore store;
static LedDisplay display;
static ButtonGestures gestures;
static BleLink ble;
static PowerMgmt power;

static uint32_t displayStartedMs = 0;
static bool gestureWasBusy = false;

static void syncAtMax() {
  gestures.setAtMax(store.count() >= store.maximum());
}

static void noteShown(uint32_t nowMs) {
  display.show(store.count());
  displayStartedMs = nowMs;
}

static bool handleMaxWrite(uint16_t requested) {
  const uint16_t previousCount = store.count();
  const uint16_t previousMax = store.maximum();
  if (!store.setMax(requested)) {
    return false;
  }
  if (store.maximum() != previousMax) {
    ble.notifyMax(store.maximum());
  }
  if (store.count() != previousCount) {
    ble.notifyCount(store.count());
    if (display.isOn()) {
      noteShown(millis());
    }
  }
  if (store.maximum() != previousMax || store.count() != previousCount) {
    store.commit();
  }
  syncAtMax();
  return true;
}

static bool handleNameWrite(const uint8_t* bytes, uint8_t length) {
  const uint8_t previousLen = store.nameLength();
  char previous[NAME_MAX_BYTES + 1];
  memcpy(previous, store.name(), previousLen);
  previous[previousLen] = '\0';

  if (!store.setName(bytes, length)) {
    return false;
  }

  if (store.nameLength() != previousLen ||
      memcmp(store.name(), previous, store.nameLength()) != 0) {
    ble.notifyName(store.name(), store.nameLength());
    store.commit();
  }
  return true;
}

static void handleLocate() {
  display.startLocate();
}

static void handlePairingClosed() {
  display.stopPairingBlink();
  gestures.endPairing();
}

static void handleGesture(GestureEvent event, uint32_t nowMs) {
  switch (event) {
    case GestureEvent::View:
      noteShown(nowMs);
      break;

    case GestureEvent::SubtractOne:
      if (store.subtractOne()) {
        ble.notifyCount(store.count());
      }
      syncAtMax();
      noteShown(nowMs);
      break;

    case GestureEvent::AddOne:
      if (store.addOne()) {
        ble.notifyCount(store.count());
      }
      syncAtMax();
      noteShown(nowMs);
      break;

    case GestureEvent::OpenPairing:
      store.commit();
      ble.openPairingWindow();
      display.startPairingBlink();
      break;

    case GestureEvent::CancelPairing:
      ble.closePairingWindow();
      display.stopPairingBlink();
      gestures.endPairing();
      break;

    case GestureEvent::ForgetBonds:
      ble.clearBonds();
      break;

    case GestureEvent::None:
      break;
  }
}

void setup() {
  store.begin();
  display.begin();
  gestures.begin();
  syncAtMax();

  ble.onMaxWrite(handleMaxWrite);
  ble.onNameWrite(handleNameWrite);
  ble.onLocate(handleLocate);
  ble.onPairingClosed(handlePairingClosed);
  ble.begin(store.count(), store.maximum(), store.name(), store.nameLength());

  power.begin();
}

void loop() {
  const uint32_t now = millis();

  handleGesture(gestures.poll(now), now);

  const bool busy = gestures.isBusy();
  if (gestureWasBusy && !busy) {
    // Gesture finished: flush once, rather than on every repeat tick.
    store.commit();
  }
  gestureWasBusy = busy;

  display.poll(now);

  if (display.isOn() && !busy && !display.isAnimating() &&
      (uint32_t)(now - displayStartedMs) >= DISPLAY_MS) {
    display.clear();
  }

  ble.poll();

  if (!busy && !display.isOn() && !display.isAnimating() && ble.canSleep()) {
    power.idle();
  }
}
