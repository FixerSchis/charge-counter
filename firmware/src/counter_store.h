#pragma once

#include <stdint.h>

// In-memory count and max. Clamping rules live here.
// Flash load/store is specified in docs/behavior.md and is not implemented:
// begin() always starts from the defaults, and commit() keeps the dirty flag.
class CounterStore {
 public:
  void begin();

  uint16_t count() const { return count_; }
  uint16_t maximum() const { return max_; }
  bool dirty() const { return dirty_; }

  // Returns false when the value does not change.
  bool addOne();
  bool subtractOne();

  // Accepts 1..COUNT_HARD_CAP. Clamps count down to the new max.
  // Returns false when the request is out of range. An in-range value
  // that is already stored returns true and does not mark the record dirty.
  bool setMax(uint16_t maximum);

  // Write the record if dirty. The flash write is not implemented.
  void commit();

 private:
  uint16_t count_ = 0;
  uint16_t max_ = 0;
  bool dirty_ = false;
};
