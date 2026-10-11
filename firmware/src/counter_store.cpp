#include "counter_store.h"

#include "config.h"

void CounterStore::begin() {
  count_ = DEFAULT_COUNT;
  max_ = DEFAULT_MAX;
  dirty_ = false;
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

void CounterStore::commit() {
  // The record format is in docs/behavior.md. Until flash is implemented,
  // leave dirty_ set so a later write still sees the change.
}
