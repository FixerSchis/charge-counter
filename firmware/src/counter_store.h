#pragma once

#include <stdint.h>

#include "config.h"

// In-memory count, max, and name. Clamping rules live here.
// begin() loads /counter.bin; commit() writes it when dirty.
class CounterStore {
 public:
  void begin();

  uint16_t count() const { return count_; }
  uint16_t maximum() const { return max_; }
  const char* name() const { return name_; }
  uint8_t nameLength() const { return nameLen_; }
  bool dirty() const { return dirty_; }

  // Returns false when the value does not change.
  bool addOne();
  bool subtractOne();

  // Accepts 1..COUNT_HARD_CAP. Clamps count down to the new max.
  // Returns false when the request is out of range. An in-range value
  // that is already stored returns true and does not mark the record dirty.
  bool setMax(uint16_t maximum);

  // Accepts 0..NAME_MAX_BYTES of UTF-8. Returns false when the bytes are
  // not valid UTF-8 or the length is too long. An identical name returns
  // true and does not mark the record dirty.
  bool setName(const uint8_t* bytes, uint8_t length);

  // Write the record if dirty. Safe to call while the SoftDevice is running.
  void commit();

 private:
  void loadDefaults();
  bool loadFromFlash();
  bool writeToFlash() const;

  uint16_t count_ = 0;
  uint16_t max_ = 0;
  char name_[NAME_MAX_BYTES + 1] = {0};
  uint8_t nameLen_ = 0;
  bool dirty_ = false;
};
