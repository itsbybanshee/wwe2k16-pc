// wwe2k16 - scripted controller for automated tests (WWE2K16_TEST_INPUT=1, commands in logs\test_input.txt, see
// test_input.cpp). Added to the SDK's input system as an extra driver for controller 1; the system merges it with
// the real controller and keyboard, so it works without the window having focus.
#pragma once

#include <cstdlib>

#include <rex/input/input_driver.h>

#include "test_input.h"

namespace wwe2k16 {

using rex::X_RESULT;
using rex::X_STATUS;
using namespace rex::input;

class ScriptInputDriver : public rex::input::InputDriver {
 public:
  explicit ScriptInputDriver(rex::ui::Window* window) : InputDriver(window, 0) {}

  static bool Enabled() {
    const char* v = std::getenv("WWE2K16_TEST_INPUT");
    return v && *v == '1';
  }

  X_STATUS Setup() override { return X_STATUS_SUCCESS; }

  X_RESULT GetCapabilities(uint32_t user_index, uint32_t, X_INPUT_CAPABILITIES* out_caps) override {
    if (user_index != 0) return X_ERROR_DEVICE_NOT_CONNECTED;
    *out_caps = {};
    out_caps->type = 0x01;     // gamepad
    out_caps->sub_type = 0x01;  // standard controller
    return X_ERROR_SUCCESS;
  }

  X_RESULT GetState(uint32_t user_index, X_INPUT_STATE* out_state) override {
    if (user_index != 0) return X_ERROR_DEVICE_NOT_CONNECTED;
    const TestInput in = PollTestInput();
    *out_state = {};
    out_state->packet_number = ++packet_;
    out_state->gamepad.buttons = in.buttons;
    return X_ERROR_SUCCESS;
  }

  X_RESULT SetState(uint32_t user_index, X_INPUT_VIBRATION*) override {
    return user_index == 0 ? X_ERROR_SUCCESS : X_ERROR_DEVICE_NOT_CONNECTED;
  }

  X_RESULT GetKeystroke(uint32_t, uint32_t, X_INPUT_KEYSTROKE*) override { return X_ERROR_EMPTY; }

 private:
  uint32_t packet_ = 0;
};

}  // namespace wwe2k16
