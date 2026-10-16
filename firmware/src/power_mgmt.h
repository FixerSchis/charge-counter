#pragma once

// System-on sleep between button and SoftDevice events (docs/power.md).
class PowerMgmt {
 public:
  void begin();

  // Wait for the next button or SoftDevice event, then return.
  // Must not switch the chip to System OFF while advertising is required.
  void idle();
};
