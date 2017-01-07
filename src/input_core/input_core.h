// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <tuple>

#include "core/hle/service/hid/hid.h"
#include "core/settings.h"
#include "input_core/devices/device.h"

class Keyboard;

class InputCore {
public:
    static void Init();
    static void Shutdown();

    /**
     * Threadsafe getter to the current PadState
     * @return Service::HID::PadState instance
     */
    static Service::HID::PadState GetPadState();

    /**
     * Threadsafe setter for the current PadState
     * @param state New PadState to overwrite current PadState.
     */
    static void SetPadState(const Service::HID::PadState& state);

    /**
     * Getter for current CirclePad
     * @return std::tuple<s16, s16> CirclePad state
     */
    static std::tuple<s16, s16> GetCirclePad();

    /**
     * Setter for current CirclePad
     * @param circle New CirclePad state
     */
    static void SetCirclePad(std::tuple<s16, s16> circle);

    /**
     * Getter for Citra's main keyboard input handler
     * @return std::shared_ptr<Keyboard> Device Keyboard instance
     */
    static std::shared_ptr<Keyboard> GetKeyboard();

    /**
     * Gets the current touch screen state (touch X/Y coordinates and whether or not it is pressed).
     * Threadsafe.
     * @note This should be called by the core emu thread to get a state set by the window thread.
     * @return std::tuple of (x, y, pressed) where `x` and `y` are the touch coordinates and
     *         `pressed` is true if the touch screen is currently being pressed
     */
    static std::tuple<u16, u16, bool> GetTouchState();

    /**
     * Threadsafe setter for the current touch screen state.
     * @param value New Touch State
     */
    static void SetTouchState(std::tuple<u16, u16, bool> value);

    /**
     * Reload input key mapping settings during game-play
     */
    static void ReloadSettings();

    static std::vector<std::shared_ptr<IDevice>> GetAllDevices();

    /**
     * Loops through all devices and detects the first device that produces an input
     * @param max_time: maximum amount of time to wait until input detected, in milliseconds.
     * @param update_GUI: function to run in while loop to process any gui events.
     * @return Settings::InputDeviceMapping of input device
     */
    static Settings::InputDeviceMapping DetectInput(int max_time,
                                                    std::function<void(void)> update_GUI);

private:
    static int tick_event;
    static Service::HID::PadState pad_state;
    static std::tuple<s16, s16> circle_pad;
    static std::shared_ptr<Keyboard> main_keyboard; ///< Keyboard is always active for Citra
    static std::vector<std::shared_ptr<IDevice>>
        devices; ///< Devices that are handling input for the game
    static std::map<Settings::InputDeviceMapping, std::vector<Service::HID::PadState>> key_mappings;
    static std::map<Service::HID::PadState, bool>
        keys_pressed; ///< keys that were pressed on previous frame.
    static std::mutex pad_state_mutex;
    static std::mutex touch_mutex;
    static u16 touch_x;        ///< Touchpad X-position in native 3DS pixel coordinates (0-320)
    static u16 touch_y;        ///< Touchpad Y-position in native 3DS pixel coordinates (0-240)
    static bool touch_pressed; ///< True if touchpad area is currently pressed, otherwise false

    static void UpdateEmulatorInputs(
        std::vector<std::map<Settings::InputDeviceMapping, float>> inputs);
    static void InputTickCallback(u64, int cycles_late);

    static bool CheckIfMappingExists(const std::vector<Settings::InputDeviceMapping>& uniqueMapping,
                                     Settings::InputDeviceMapping mappingToCheck);

    static std::vector<Settings::InputDeviceMapping> GatherUniqueMappings();

    static void BuildKeyMapping();

    static void GenerateUniqueDevices();

    static void ParseSettings();
    static std::tuple<float, float> ApplyDeadzone(float x, float y, float dead_zone);
};
