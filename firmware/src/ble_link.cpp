#include "ble_link.h"

void BleLink::begin(const char* deviceName) {
  (void)deviceName;
  canSleep_ = true;
}

void BleLink::poll() {}

void BleLink::notifyCount(uint16_t count) {
  (void)count;
}

void BleLink::notifyMax(uint16_t maximum) {
  (void)maximum;
}

void BleLink::onMaxWrite(MaxWriteHandler handler) {
  onMaxWrite_ = handler;
}
