#include "counter_store.h"

#include <string.h>

#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>

using namespace Adafruit_LittleFS_Namespace;

namespace {

constexpr size_t kRecordSize = 43;  // 41 payload bytes + 2 CRC

uint16_t crc16CcittFalse(const uint8_t* data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= static_cast<uint16_t>(data[i]) << 8;
    for (uint8_t bit = 0; bit < 8; bit++) {
      if (crc & 0x8000) {
        crc = static_cast<uint16_t>((crc << 1) ^ 0x1021);
      } else {
        crc = static_cast<uint16_t>(crc << 1);
      }
    }
  }
  return crc;
}

bool isValidUtf8(const uint8_t* bytes, uint8_t length) {
  size_t i = 0;
  while (i < length) {
    const uint8_t c = bytes[i];
    if (c <= 0x7F) {
      i += 1;
      continue;
    }
    uint8_t need = 0;
    if ((c & 0xE0) == 0xC0) {
      need = 1;
      if (c < 0xC2) {
        return false;  // overlong
      }
    } else if ((c & 0xF0) == 0xE0) {
      need = 2;
    } else if ((c & 0xF8) == 0xF0) {
      need = 3;
      if (c > 0xF4) {
        return false;
      }
    } else {
      return false;
    }
    if (i + need >= length) {
      return false;
    }
    for (uint8_t j = 1; j <= need; j++) {
      if ((bytes[i + j] & 0xC0) != 0x80) {
        return false;
      }
    }
    i += static_cast<size_t>(need) + 1;
  }
  return true;
}

}  // namespace

void CounterStore::loadDefaults() {
  count_ = DEFAULT_COUNT;
  max_ = DEFAULT_MAX;
  nameLen_ = 0;
  memset(name_, 0, sizeof(name_));
  dirty_ = false;
}

void CounterStore::begin() {
  InternalFS.begin();
  if (!loadFromFlash()) {
    loadDefaults();
  }
}

bool CounterStore::loadFromFlash() {
  File file(InternalFS);
  if (!file.open(STORAGE_PATH, FILE_O_READ)) {
    return false;
  }

  uint8_t buf[kRecordSize];
  const int n = file.read(buf, kRecordSize);
  file.close();
  if (n != static_cast<int>(kRecordSize)) {
    return false;
  }

  if (memcmp(buf, STORAGE_MAGIC, 4) != 0) {
    return false;
  }

  const uint16_t storedCrc = static_cast<uint16_t>(buf[41] | (buf[42] << 8));
  if (storedCrc != crc16CcittFalse(buf, 41)) {
    return false;
  }

  const uint16_t count = static_cast<uint16_t>(buf[4] | (buf[5] << 8));
  const uint16_t maximum = static_cast<uint16_t>(buf[6] | (buf[7] << 8));
  const uint8_t nameLen = buf[8];

  if (maximum < 1 || maximum > COUNT_HARD_CAP) {
    return false;
  }
  if (count > maximum) {
    return false;
  }
  if (nameLen > NAME_MAX_BYTES) {
    return false;
  }
  if (!isValidUtf8(&buf[9], nameLen)) {
    return false;
  }

  count_ = count;
  max_ = maximum;
  nameLen_ = nameLen;
  memset(name_, 0, sizeof(name_));
  memcpy(name_, &buf[9], nameLen_);
  dirty_ = false;
  return true;
}

bool CounterStore::writeToFlash() const {
  uint8_t buf[kRecordSize];
  memset(buf, 0, sizeof(buf));
  memcpy(buf, STORAGE_MAGIC, 4);
  buf[4] = static_cast<uint8_t>(count_ & 0xFF);
  buf[5] = static_cast<uint8_t>((count_ >> 8) & 0xFF);
  buf[6] = static_cast<uint8_t>(max_ & 0xFF);
  buf[7] = static_cast<uint8_t>((max_ >> 8) & 0xFF);
  buf[8] = nameLen_;
  memcpy(&buf[9], name_, nameLen_);
  const uint16_t crc = crc16CcittFalse(buf, 41);
  buf[41] = static_cast<uint8_t>(crc & 0xFF);
  buf[42] = static_cast<uint8_t>((crc >> 8) & 0xFF);

  // Overwrite: remove then create. SoftDevice-safe via InternalFS.
  if (InternalFS.exists(STORAGE_PATH)) {
    InternalFS.remove(STORAGE_PATH);
  }

  File file(InternalFS);
  if (!file.open(STORAGE_PATH, FILE_O_WRITE)) {
    return false;
  }
  const size_t written = file.write(buf, kRecordSize);
  file.close();
  return written == kRecordSize;
}

bool CounterStore::addOne() {
  if (count_ >= max_) {
    return false;
  }
  count_++;
  dirty_ = true;
  return true;
}

bool CounterStore::subtractOne() {
  if (count_ == 0) {
    return false;
  }
  count_--;
  dirty_ = true;
  return true;
}

bool CounterStore::setMax(uint16_t maximum) {
  if (maximum < 1 || maximum > COUNT_HARD_CAP) {
    return false;
  }
  if (maximum == max_) {
    return true;
  }
  max_ = maximum;
  if (count_ > max_) {
    count_ = max_;
  }
  dirty_ = true;
  return true;
}

bool CounterStore::setName(const uint8_t* bytes, uint8_t length) {
  if (length > NAME_MAX_BYTES) {
    return false;
  }
  if (length > 0 && bytes == nullptr) {
    return false;
  }
  if (!isValidUtf8(bytes, length)) {
    return false;
  }
  if (length == nameLen_ && (length == 0 || memcmp(name_, bytes, length) == 0)) {
    return true;
  }
  nameLen_ = length;
  memset(name_, 0, sizeof(name_));
  if (length > 0) {
    memcpy(name_, bytes, length);
  }
  dirty_ = true;
  return true;
}

void CounterStore::commit() {
  if (!dirty_) {
    return;
  }
  if (writeToFlash()) {
    dirty_ = false;
  }
}
