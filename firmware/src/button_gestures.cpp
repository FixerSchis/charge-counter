#include "button_gestures.h"

#include "board_gpio.h"
#include "board_pins.h"
#include "config.h"

void ButtonGestures::begin() {
  board_pin_input_pullup(PIN_BUTTON);
  state_ = State::Idle;
  busy_ = false;
  atMax_ = false;
  stablePressed_ = false;
  candidatePressed_ = false;
  candidateSinceMs_ = 0;
  pairTimerArmed_ = false;
  openedPairingThisHold_ = false;
  needArmPairTimer_ = false;
}

void ButtonGestures::setAtMax(bool atMax) {
  if (atMax && !atMax_) {
    // Count just reached max during a hold; arm on the next poll.
    needArmPairTimer_ = true;
  }
  atMax_ = atMax;
}

void ButtonGestures::endPairing() {
  if (state_ == State::Pairing || state_ == State::PairingHeld) {
    state_ = State::Idle;
    busy_ = false;
    pairTimerArmed_ = false;
    openedPairingThisHold_ = false;
    needArmPairTimer_ = false;
  }
}

bool ButtonGestures::updateDebounced(uint32_t nowMs) {
  const bool rawPressed = !board_pin_read(PIN_BUTTON);
  if (rawPressed != candidatePressed_) {
    candidatePressed_ = rawPressed;
    candidateSinceMs_ = nowMs;
    return false;
  }
  if ((uint32_t)(nowMs - candidateSinceMs_) < DEBOUNCE_MS) {
    return false;
  }
  if (stablePressed_ == candidatePressed_) {
    return false;
  }
  stablePressed_ = candidatePressed_;
  return true;
}

GestureEvent ButtonGestures::onPressed(uint32_t nowMs) {
  pressAcceptedMs_ = nowMs;

  switch (state_) {
    case State::Idle:
      state_ = State::Held;
      busy_ = true;
      openedPairingThisHold_ = false;
      needArmPairTimer_ = false;
      if (atMax_) {
        // Pairing timer starts at the accepted press when already at max.
        pairTimerArmed_ = true;
        pairDeadlineMs_ = nowMs + PAIR_AT_MAX_MS;
      } else {
        pairTimerArmed_ = false;
      }
      return GestureEvent::None;

    case State::Pending:
      state_ = State::Subtracting;
      busy_ = true;
      return GestureEvent::SubtractOne;

    case State::Subtracting:
      busy_ = true;
      return GestureEvent::SubtractOne;

    case State::Pairing:
      state_ = State::PairingHeld;
      busy_ = true;
      pairDeadlineMs_ = nowMs + PAIR_FORGET_MS;
      openedPairingThisHold_ = false;
      return GestureEvent::None;

    default:
      return GestureEvent::None;
  }
}

GestureEvent ButtonGestures::onReleased(uint32_t nowMs) {
  releaseAcceptedMs_ = nowMs;

  switch (state_) {
    case State::Held:
      state_ = State::Pending;
      busy_ = true;
      pairTimerArmed_ = false;
      needArmPairTimer_ = false;
      return GestureEvent::None;

    case State::Adding:
      state_ = State::Idle;
      busy_ = false;
      pairTimerArmed_ = false;
      needArmPairTimer_ = false;
      return GestureEvent::None;

    case State::PairingHeld:
      if (openedPairingThisHold_) {
        // Hold that opened pairing, or that finished forget: keep the window.
        openedPairingThisHold_ = false;
        state_ = State::Pairing;
        busy_ = true;
        return GestureEvent::None;
      }
      // Short press in the pairing window cancels.
      state_ = State::Idle;
      busy_ = false;
      return GestureEvent::CancelPairing;

    case State::Subtracting:
      busy_ = true;
      return GestureEvent::None;

    default:
      return GestureEvent::None;
  }
}

GestureEvent ButtonGestures::pollWhileHeld(uint32_t nowMs) {
  switch (state_) {
    case State::Held:
      if ((uint32_t)(nowMs - pressAcceptedMs_) >= LONG_PRESS_MS) {
        state_ = State::Adding;
        nextAddMs_ = nowMs + ADD_REPEAT_MS;
        if (atMax_) {
          // Timer already armed at press; do not add.
          return GestureEvent::None;
        }
        return GestureEvent::AddOne;
      }
      return GestureEvent::None;

    case State::Adding:
      if (atMax_) {
        if (needArmPairTimer_) {
          pairTimerArmed_ = true;
          needArmPairTimer_ = false;
          pairDeadlineMs_ = nowMs + PAIR_AT_MAX_MS;
        }
        if (pairTimerArmed_ && (int32_t)(nowMs - pairDeadlineMs_) >= 0) {
          state_ = State::PairingHeld;
          openedPairingThisHold_ = true;
          pairTimerArmed_ = false;
          busy_ = true;
          return GestureEvent::OpenPairing;
        }
        return GestureEvent::None;
      }
      if ((int32_t)(nowMs - nextAddMs_) >= 0) {
        nextAddMs_ = nowMs + ADD_REPEAT_MS;
        return GestureEvent::AddOne;
      }
      return GestureEvent::None;

    case State::PairingHeld:
      if (!openedPairingThisHold_ && (int32_t)(nowMs - pairDeadlineMs_) >= 0) {
        openedPairingThisHold_ = true;  // release must not cancel
        busy_ = true;
        return GestureEvent::ForgetBonds;
      }
      return GestureEvent::None;

    case State::Subtracting:
      // A hold that continues a subtract press does not add.
      return GestureEvent::None;

    default:
      return GestureEvent::None;
  }
}

GestureEvent ButtonGestures::pollWhileUp(uint32_t nowMs) {
  switch (state_) {
    case State::Pending:
      if ((uint32_t)(nowMs - releaseAcceptedMs_) >= PRESS_GAP_MS) {
        state_ = State::Idle;
        busy_ = false;
        return GestureEvent::View;
      }
      busy_ = true;
      return GestureEvent::None;

    case State::Subtracting:
      if ((uint32_t)(nowMs - releaseAcceptedMs_) >= PRESS_GAP_MS) {
        state_ = State::Idle;
        busy_ = false;
      } else {
        busy_ = true;
      }
      return GestureEvent::None;

    case State::Pairing:
      busy_ = true;
      return GestureEvent::None;

    default:
      return GestureEvent::None;
  }
}

GestureEvent ButtonGestures::poll(uint32_t nowMs) {
  const bool edge = updateDebounced(nowMs);

  if (edge) {
    if (stablePressed_) {
      return onPressed(nowMs);
    }
    return onReleased(nowMs);
  }

  if (stablePressed_) {
    return pollWhileHeld(nowMs);
  }
  return pollWhileUp(nowMs);
}
