#pragma once

#include <stdint.h>

#include "config.h"

// BLE peripheral. GATT layout, bonding, and writes are in docs/ble.md.
class BleLink {
 public:
  using MaxWriteHandler = bool (*)(uint16_t requestedMax);
  using NameWriteHandler = bool (*)(const uint8_t* bytes, uint8_t length);
  using LocateHandler = void (*)();
  using PairingClosedHandler = void (*)();

  void begin(uint16_t count, uint16_t maximum, const char* name, uint8_t nameLen);
  void poll();

  void notifyCount(uint16_t count);
  void notifyMax(uint16_t maximum);
  void notifyName(const char* name, uint8_t nameLen);

  void onMaxWrite(MaxWriteHandler handler);
  void onNameWrite(NameWriteHandler handler);
  void onLocate(LocateHandler handler);
  void onPairingClosed(PairingClosedHandler handler);

  void openPairingWindow();
  void closePairingWindow();
  bool pairingOpen() const { return pairingOpen_; }
  void clearBonds();

  // True when the stack has been polled and can wait for the next radio
  // or GPIO event. A connected phone does not by itself make this false.
  bool canSleep() const { return canSleep_; }

  // Used by SoftDevice callbacks in ble_link.cpp.
  bool handleMaxWrite(const uint8_t* data, uint16_t len);
  bool handleNameWrite(const uint8_t* data, uint16_t len);
  bool handleLocateWrite(const uint8_t* data, uint16_t len);
  void handleConnect(uint16_t connHandle);
  void handleDisconnect();
  void handlePairComplete(uint16_t connHandle, uint8_t authStatus);
  void handleSecured(uint16_t connHandle);

 private:
  void startAdvertising();
  void refreshAdvertising();
  void updateBatteryLevel();
  void buildLocalName();

  MaxWriteHandler onMaxWrite_ = nullptr;
  NameWriteHandler onNameWrite_ = nullptr;
  LocateHandler onLocate_ = nullptr;
  PairingClosedHandler onPairingClosed_ = nullptr;

  bool canSleep_ = true;
  bool pairingOpen_ = false;
  uint32_t pairingDeadlineMs_ = 0;
  char localName_[8] = {0};  // "CC-XXXX"
};
