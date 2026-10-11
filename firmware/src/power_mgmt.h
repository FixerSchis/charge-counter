#pragma once

// Sleep between button and radio events. The power policy is in docs/power.md.
// idle() currently just yields; it does not sleep the radio.
class PowerMgmt {
 public:
  void begin();

  // Wait for the next button or SoftDevice event, then return.
  // Must not switch the chip to System OFF while advertising is required.
  void idle();
};
