#include "ble_link.h"

#include <string.h>

#include <bluefruit.h>

#include "board_gpio.h"
#include "board_pins.h"
#include "config.h"

namespace {

// SoftDevice wants 128-bit UUIDs in little-endian byte order.
constexpr uint8_t kServiceUuid[] = {
    0xc0, 0xbc, 0xce, 0xa8, 0x97, 0x65, 0x83, 0xa9,
    0xec, 0x4c, 0xab, 0x7e, 0x8b, 0xf3, 0xdd, 0x8a};
constexpr uint8_t kCountUuid[] = {
    0x05, 0x3c, 0xbc, 0xd2, 0x0a, 0xe3, 0x69, 0xae,
    0x6f, 0x4a, 0xc7, 0xe2, 0x73, 0x7c, 0x03, 0x3f};
constexpr uint8_t kMaxUuid[] = {
    0x1b, 0x63, 0xce, 0x76, 0x8a, 0x32, 0xbe, 0xbb,
    0xb2, 0x43, 0x5c, 0x00, 0x1f, 0x6a, 0xc4, 0xdd};
constexpr uint8_t kNameUuid[] = {
    0xea, 0xfc, 0xa8, 0x69, 0x5a, 0x15, 0x80, 0xb5,
    0x58, 0x46, 0x47, 0xfe, 0xc7, 0x48, 0xaf, 0xf4};
constexpr uint8_t kLocateUuid[] = {
    0x58, 0x05, 0x80, 0x9c, 0x08, 0x5a, 0xc3, 0x8f,
    0x61, 0x4f, 0xac, 0x04, 0x3f, 0x8c, 0xda, 0x64};

BleLink* g_ble = nullptr;

BLEService g_svc(kServiceUuid);
BLECharacteristic g_count(kCountUuid);
BLECharacteristic g_max(kMaxUuid);
BLECharacteristic g_name(kNameUuid);
BLECharacteristic g_locate(kLocateUuid);

// Own battery service so the level characteristic can require encryption.
// BLEBas hard-codes SECMODE_OPEN.
BLEService g_batt(UUID16_SVC_BATTERY);
BLECharacteristic g_battLevel(UUID16_CHR_BATTERY_LEVEL);
BLEDis g_dis;

uint16_t g_storedMax = DEFAULT_MAX;
char g_storedName[NAME_MAX_BYTES + 1] = {0};
uint8_t g_storedNameLen = 0;

void connect_cb(uint16_t conn_handle) {
  if (g_ble) {
    g_ble->handleConnect(conn_handle);
  }
}

void disconnect_cb(uint16_t conn_handle, uint8_t reason) {
  (void)conn_handle;
  (void)reason;
  if (g_ble) {
    g_ble->handleDisconnect();
  }
}

void pair_complete_cb(uint16_t conn_handle, uint8_t auth_status) {
  if (g_ble) {
    g_ble->handlePairComplete(conn_handle, auth_status);
  }
}

void secured_cb(uint16_t conn_handle) {
  if (g_ble) {
    g_ble->handleSecured(conn_handle);
  }
}

void max_write_cb(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  (void)conn_hdl;
  (void)chr;
  if (!g_ble || !g_ble->handleMaxWrite(data, len)) {
    // Rejection is expressed by not updating the characteristic value.
  }
}

void name_write_cb(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  (void)conn_hdl;
  (void)chr;
  if (g_ble) {
    g_ble->handleNameWrite(data, len);
  }
}

void locate_write_cb(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  (void)conn_hdl;
  (void)chr;
  if (g_ble) {
    g_ble->handleLocateWrite(data, len);
  }
}

uint8_t voltageToPercent(float volts) {
  // Straight line from an empty cell to a full cell. Replace after measuring.
  constexpr float kEmpty = 3.4f;
  constexpr float kFull = 4.2f;
  if (volts <= kEmpty) {
    return 0;
  }
  if (volts >= kFull) {
    return 100;
  }
  return static_cast<uint8_t>(((volts - kEmpty) / (kFull - kEmpty)) * 100.0f + 0.5f);
}

}  // namespace

void BleLink::buildLocalName() {
  ble_gap_addr_t addr = Bluefruit.getAddr();
  // Lower 16 bits of the device address, four uppercase hex digits.
  const uint16_t suffix =
      static_cast<uint16_t>((addr.addr[1] << 8) | addr.addr[0]);
  snprintf(localName_, sizeof(localName_), "CC-%04X", suffix);
}

void BleLink::updateBatteryLevel() {
  board_pin_write(PIN_VBAT_EN, true);
  delay(2);

  // 12-bit ADC against the 0.6 V internal ref with 1/6 gain => 3.6 V full scale.
  analogReference(AR_INTERNAL);
  analogReadResolution(12);
  const int raw = analogRead(PIN_VBAT_AIN);
  board_pin_write(PIN_VBAT_EN, false);

  // Divider is 1:1, so cell voltage is twice the AIN voltage.
  const float ainVolts = (static_cast<float>(raw) / 4095.0f) * 3.6f;
  const float cellVolts = ainVolts * 2.0f;
  g_battLevel.write8(voltageToPercent(cellVolts));
}

void BleLink::startAdvertising() {
  Bluefruit.Advertising.clearData();
  Bluefruit.ScanResponse.clearData();

  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addUuid(BLEUuid(kServiceUuid));

  Bluefruit.setName(localName_);
  Bluefruit.ScanResponse.addName();

  if (pairingOpen_) {
    // 128-bit UUID service data with one byte 0x01.
    uint8_t svcData[17];
    memcpy(svcData, kServiceUuid, 16);
    svcData[16] = 0x01;
    Bluefruit.ScanResponse.addData(BLE_GAP_AD_TYPE_SERVICE_DATA_128BIT_UUID, svcData,
                                   sizeof(svcData));
  }

  Bluefruit.Advertising.restartOnDisconnect(true);
  // 1000 ms = 1600 units of 0.625 ms. Stay at that interval.
  Bluefruit.Advertising.setInterval(1600, 1600);
  Bluefruit.Advertising.setFastTimeout(0);
  Bluefruit.Advertising.start(0);
}

void BleLink::refreshAdvertising() {
  const bool wasRunning = Bluefruit.Advertising.isRunning();
  if (wasRunning) {
    Bluefruit.Advertising.stop();
  }
  startAdvertising();
}

void BleLink::begin(uint16_t count, uint16_t maximum, const char* name, uint8_t nameLen) {
  g_ble = this;
  canSleep_ = true;
  pairingOpen_ = false;

  board_pin_output_low(PIN_VBAT_EN);
  board_pin_input_nopull(PIN_VBUS_SENSE);

  // Feather initVariant drives P0.17 as an LED. Restore it as VBUS sense.
  board_pin_input_nopull(PIN_VBUS_SENSE);

  Bluefruit.autoConnLed(false);
  Bluefruit.configUuid128Count(5);
  Bluefruit.begin();
  Bluefruit.setTxPower(4);
  Bluefruit.Periph.setConnIntervalMS(80, 150);
  Bluefruit.Periph.setConnSlaveLatency(4);
  Bluefruit.Periph.setConnSupervisionTimeoutMS(4000);

  // Just Works, LE Secure Connections, bonding.
  Bluefruit.Security.setIOCaps(false, false, false);
  Bluefruit.Security.setMITM(false);
  Bluefruit.Security.setPairCompleteCallback(pair_complete_cb);
  Bluefruit.Security.setSecuredCallback(secured_cb);

  Bluefruit.Periph.setConnectCallback(connect_cb);
  Bluefruit.Periph.setDisconnectCallback(disconnect_cb);

  g_storedMax = maximum;
  g_storedNameLen = nameLen > NAME_MAX_BYTES ? NAME_MAX_BYTES : nameLen;
  memset(g_storedName, 0, sizeof(g_storedName));
  if (g_storedNameLen > 0 && name != nullptr) {
    memcpy(g_storedName, name, g_storedNameLen);
  }

  g_dis.setModel(MODEL_NAME);
  g_dis.setFirmwareRev(FW_VERSION);
  g_dis.begin();

  g_batt.begin();
  g_battLevel.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
  g_battLevel.setPermission(SECMODE_ENC_NO_MITM, SECMODE_NO_ACCESS);
  g_battLevel.setFixedLen(1);
  g_battLevel.begin();
  updateBatteryLevel();

  g_svc.begin();

  g_count.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
  g_count.setPermission(SECMODE_ENC_NO_MITM, SECMODE_NO_ACCESS);
  g_count.setFixedLen(2);
  g_count.begin();
  uint8_t countBytes[2] = {static_cast<uint8_t>(count & 0xFF),
                           static_cast<uint8_t>((count >> 8) & 0xFF)};
  g_count.write(countBytes, 2);

  g_max.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE | CHR_PROPS_NOTIFY);
  g_max.setPermission(SECMODE_ENC_NO_MITM, SECMODE_ENC_NO_MITM);
  g_max.setFixedLen(2);
  g_max.setWriteCallback(max_write_cb);
  g_max.begin();
  uint8_t maxBytes[2] = {static_cast<uint8_t>(maximum & 0xFF),
                         static_cast<uint8_t>((maximum >> 8) & 0xFF)};
  g_max.write(maxBytes, 2);

  g_name.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE | CHR_PROPS_NOTIFY);
  g_name.setPermission(SECMODE_ENC_NO_MITM, SECMODE_ENC_NO_MITM);
  g_name.setMaxLen(NAME_MAX_BYTES);
  g_name.setWriteCallback(name_write_cb);
  g_name.begin();
  if (g_storedNameLen > 0) {
    g_name.write(reinterpret_cast<const uint8_t*>(g_storedName), g_storedNameLen);
  } else {
    g_name.write(reinterpret_cast<const uint8_t*>(""), 0);
  }

  g_locate.setProperties(CHR_PROPS_WRITE);
  g_locate.setPermission(SECMODE_NO_ACCESS, SECMODE_ENC_NO_MITM);
  g_locate.setFixedLen(1);
  g_locate.setWriteCallback(locate_write_cb);
  g_locate.begin();

  buildLocalName();
  startAdvertising();
}

void BleLink::poll() {
  if (pairingOpen_ && (int32_t)(millis() - pairingDeadlineMs_) >= 0) {
    closePairingWindow();
  }

  // Keep UART pins disconnected from the unpowered CH340 while on battery.
  if (!board_pin_read(PIN_VBUS_SENSE)) {
    board_pin_input_nopull(PIN_UART_TX);
    board_pin_input_nopull(PIN_UART_RX);
  }

  canSleep_ = true;
}

void BleLink::notifyCount(uint16_t count) {
  uint8_t bytes[2] = {static_cast<uint8_t>(count & 0xFF),
                      static_cast<uint8_t>((count >> 8) & 0xFF)};
  g_count.write(bytes, 2);
  g_count.notify(bytes, 2);
}

void BleLink::notifyMax(uint16_t maximum) {
  g_storedMax = maximum;
  uint8_t bytes[2] = {static_cast<uint8_t>(maximum & 0xFF),
                      static_cast<uint8_t>((maximum >> 8) & 0xFF)};
  g_max.write(bytes, 2);
  g_max.notify(bytes, 2);
}

void BleLink::notifyName(const char* name, uint8_t nameLen) {
  g_storedNameLen = nameLen > NAME_MAX_BYTES ? NAME_MAX_BYTES : nameLen;
  memset(g_storedName, 0, sizeof(g_storedName));
  if (g_storedNameLen > 0 && name != nullptr) {
    memcpy(g_storedName, name, g_storedNameLen);
    g_name.write(reinterpret_cast<const uint8_t*>(g_storedName), g_storedNameLen);
    g_name.notify(reinterpret_cast<const uint8_t*>(g_storedName), g_storedNameLen);
  } else {
    g_name.write(reinterpret_cast<const uint8_t*>(""), 0);
    g_name.notify(reinterpret_cast<const uint8_t*>(""), 0);
  }
}

void BleLink::onMaxWrite(MaxWriteHandler handler) { onMaxWrite_ = handler; }
void BleLink::onNameWrite(NameWriteHandler handler) { onNameWrite_ = handler; }
void BleLink::onLocate(LocateHandler handler) { onLocate_ = handler; }
void BleLink::onPairingClosed(PairingClosedHandler handler) {
  onPairingClosed_ = handler;
}

void BleLink::openPairingWindow() {
  pairingOpen_ = true;
  pairingDeadlineMs_ = millis() + PAIR_OPEN_MS;
  refreshAdvertising();
}

void BleLink::closePairingWindow() {
  if (!pairingOpen_) {
    return;
  }
  pairingOpen_ = false;
  refreshAdvertising();
  if (onPairingClosed_) {
    onPairingClosed_();
  }
}

void BleLink::clearBonds() {
  bond_clear_prph();
}

bool BleLink::handleMaxWrite(const uint8_t* data, uint16_t len) {
  auto restoreMax = []() {
    uint8_t bytes[2] = {static_cast<uint8_t>(g_storedMax & 0xFF),
                        static_cast<uint8_t>((g_storedMax >> 8) & 0xFF)};
    g_max.write(bytes, 2);
  };

  if (len != 2 || data == nullptr || onMaxWrite_ == nullptr) {
    restoreMax();
    return false;
  }
  const uint16_t requested =
      static_cast<uint16_t>(data[0] | (static_cast<uint16_t>(data[1]) << 8));
  if (!onMaxWrite_(requested)) {
    restoreMax();
    return false;
  }
  // Handler notifies on change. Keep the cached value in sync.
  g_storedMax = requested;
  uint8_t bytes[2] = {data[0], data[1]};
  g_max.write(bytes, 2);
  return true;
}

bool BleLink::handleNameWrite(const uint8_t* data, uint16_t len) {
  auto restoreName = []() {
    if (g_storedNameLen > 0) {
      g_name.write(reinterpret_cast<const uint8_t*>(g_storedName), g_storedNameLen);
    } else {
      g_name.write(reinterpret_cast<const uint8_t*>(""), 0);
    }
  };

  if (len > NAME_MAX_BYTES || onNameWrite_ == nullptr) {
    restoreName();
    return false;
  }
  if (!onNameWrite_(data, static_cast<uint8_t>(len))) {
    restoreName();
    return false;
  }
  g_storedNameLen = static_cast<uint8_t>(len);
  memset(g_storedName, 0, sizeof(g_storedName));
  if (len > 0) {
    memcpy(g_storedName, data, len);
    g_name.write(data, len);
  } else {
    g_name.write(reinterpret_cast<const uint8_t*>(""), 0);
  }
  return true;
}

bool BleLink::handleLocateWrite(const uint8_t* data, uint16_t len) {
  if (len != 1 || data == nullptr || data[0] != 0x01) {
    return false;
  }
  if (onLocate_) {
    onLocate_();
  }
  return true;
}

void BleLink::handleConnect(uint16_t connHandle) {
  BLEConnection* conn = Bluefruit.Connection(connHandle);
  if (conn == nullptr) {
    return;
  }

  if (conn->bonded()) {
    updateBatteryLevel();
    return;
  }

  if (pairingOpen_) {
    conn->requestPairing();
    return;
  }

  // Unbonded and not pairing: drop the link.
  conn->disconnect();
}

void BleLink::handleDisconnect() {
  // Advertising restarts via restartOnDisconnect.
}

void BleLink::handlePairComplete(uint16_t connHandle, uint8_t authStatus) {
  (void)connHandle;
  if (authStatus == BLE_GAP_SEC_STATUS_SUCCESS && pairingOpen_) {
    // One bond per window.
    closePairingWindow();
  }
}

void BleLink::handleSecured(uint16_t connHandle) {
  (void)connHandle;
  updateBatteryLevel();
}
