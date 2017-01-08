// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>

#include "core/core_timing.h"
#include "core/hw/gpu.h"

#include "input_core/devices/keyboard.h"
#include "input_core/devices/sdl_gamepad.h"
#include "input_core/input_core.h"

int InputCore::tick_event;
Service::HID::PadState InputCore::pad_state;
std::tuple<s16, s16> InputCore::circle_pad;
std::shared_ptr<Keyboard> InputCore::main_keyboard; ///< Keyboard is always active for Citra
std::vector<std::shared_ptr<IDevice>>
    InputCore::devices; ///< Devices that are handling input for the game
std::map<Settings::InputDeviceMapping, std::vector<Service::HID::PadState>> InputCore::key_mappings;
std::map<Service::HID::PadState, bool>
    InputCore::keys_pressed; ///< keys that were pressed on previous frame.
std::mutex InputCore::pad_state_mutex;
std::mutex InputCore::touch_mutex;
u16 InputCore::touch_x;        ///< Touchpad X-position in native 3DS pixel coordinates (0-320)
u16 InputCore::touch_y;        ///< Touchpad Y-position in native 3DS pixel coordinates (0-240)
bool InputCore::touch_pressed; ///< True if touchpad area is currently pressed, otherwise false
const float default_deadzone =
    0.5; ///< Applies to analog controls being used for digital 3ds inputs.

void InputCore::Init() {
    ParseSettings();
    tick_event = CoreTiming::RegisterEvent("InputCore::tick_event", InputTickCallback);
    CoreTiming::ScheduleEvent(GPU::frame_ticks, tick_event);
}

void InputCore::Shutdown() {
    CoreTiming::UnscheduleEvent(tick_event, 0);
    devices.clear();
}
void InputCore::InputTickCallback(u64, int cycles_late) {
    std::vector<std::map<Settings::InputDeviceMapping, float>> inputs;
    for (auto& device : devices) {
        inputs.push_back(device->ProcessInput());
    }
    UpdateEmulatorInputs(inputs);

    Service::HID::Update();

    // Reschedule recurrent event
    CoreTiming::ScheduleEvent(GPU::frame_ticks - cycles_late, tick_event);
}
void InputCore::UpdateEmulatorInputs(
    std::vector<std::map<Settings::InputDeviceMapping, float>> inputs) {
    std::lock_guard<std::mutex> lock(pad_state_mutex);

    // Apply deadzone
    float leftx = 0, lefty = 0;
    float circle_pad_modifier = 1.0;
    auto circle_pad_modifier_mapping = Settings::values.pad_circle_modifier;
    for (auto& input_device : inputs) {
        for (auto& button_states : input_device) { // Loop through all buttons from input device
            float strength = button_states.second;
            auto emulator_inputs = key_mappings[button_states.first];
            for (auto& emulator_input : emulator_inputs) {
                if (emulator_input == Service::HID::PAD_CIRCLE_UP && abs(strength) > 0) {
                    lefty = -strength;
                } else if (emulator_input == Service::HID::PAD_CIRCLE_DOWN && abs(strength) > 0) {
                    lefty = strength;
                } else if (emulator_input == Service::HID::PAD_CIRCLE_LEFT && abs(strength) > 0) {
                    leftx = -strength;
                } else if (emulator_input == Service::HID::PAD_CIRCLE_RIGHT && abs(strength) > 0) {
                    leftx = strength;
                }
            }
            if (button_states.first == circle_pad_modifier_mapping)
                circle_pad_modifier = (button_states.second > default_deadzone)
                                          ? Settings::values.pad_circle_modifier_scale
                                          : 1.0;
        }
    }
    float deadzone = Settings::values.pad_circle_deadzone;
    std::tuple<float, float> left_stick = ApplyDeadzone(leftx, lefty, deadzone);

    for (auto& input_device : inputs) {
        for (auto& button_states : input_device) { // Loop through all buttons from input device
            float strength = button_states.second;
            auto emulator_inputs = key_mappings[button_states.first];
            for (auto& emulator_input : emulator_inputs) {
                if (std::find(std::begin(KeyMap::analog_inputs), std::end(KeyMap::analog_inputs),
                              emulator_input) ==
                    std::end(KeyMap::analog_inputs)) { // digital button
                    if (abs(strength) < default_deadzone && keys_pressed[emulator_input] == true) {
                        pad_state.hex &= ~emulator_input.hex;
                        keys_pressed[emulator_input] = false;
                    } else if (abs(strength) >= default_deadzone &&
                               keys_pressed[emulator_input] == false) {
                        pad_state.hex |= emulator_input.hex;
                        keys_pressed[emulator_input] = true;
                    }
                } else { // analog stick
                    if (emulator_input == Service::HID::PAD_CIRCLE_UP ||
                        emulator_input == Service::HID::PAD_CIRCLE_DOWN) {
                        std::get<1>(circle_pad) = std::get<1>(left_stick) *
                                                  KeyMap::MAX_CIRCLEPAD_POS * -1 *
                                                  circle_pad_modifier;
                    } else if (emulator_input == Service::HID::PAD_CIRCLE_LEFT ||
                               emulator_input == Service::HID::PAD_CIRCLE_RIGHT) {
                        std::get<0>(circle_pad) = std::get<0>(left_stick) *
                                                  KeyMap::MAX_CIRCLEPAD_POS * circle_pad_modifier;
                    }
                }
            }
        }
    }
}

Service::HID::PadState InputCore::GetPadState() {
    std::lock_guard<std::mutex> lock(pad_state_mutex);
    return pad_state;
}

void InputCore::SetPadState(const Service::HID::PadState& state) {
    std::lock_guard<std::mutex> lock(pad_state_mutex);
    pad_state.hex = state.hex;
}

std::tuple<s16, s16> InputCore::GetCirclePad() {
    return circle_pad;
}

void InputCore::SetCirclePad(std::tuple<s16, s16> pad) {
    circle_pad = pad;
}

std::shared_ptr<Keyboard> InputCore::GetKeyboard() {
    if (main_keyboard == nullptr)
        main_keyboard = std::make_shared<Keyboard>();
    return main_keyboard;
}

std::tuple<u16, u16, bool> InputCore::GetTouchState() {
    std::lock_guard<std::mutex> lock(touch_mutex);
    return std::make_tuple(touch_x, touch_y, touch_pressed);
}

void InputCore::SetTouchState(std::tuple<u16, u16, bool> value) {
    std::lock_guard<std::mutex> lock(touch_mutex);
    std::tie(touch_x, touch_y, touch_pressed) = value;
}

/// Helper method to check if device was already initialized
bool InputCore::CheckIfMappingExists(const std::vector<Settings::InputDeviceMapping>& uniqueMapping,
                                     Settings::InputDeviceMapping mappingToCheck) {
    return std::any_of(uniqueMapping.begin(), uniqueMapping.end(),
                       [mappingToCheck](const auto& mapping) { return mapping == mappingToCheck; });
}

/// Get Unique input mappings from settings
std::vector<Settings::InputDeviceMapping> InputCore::GatherUniqueMappings() {
    std::vector<Settings::InputDeviceMapping> uniqueMappings;

    for (const auto& mapping : Settings::values.input_mappings) {
        if (!CheckIfMappingExists(uniqueMappings, mapping)) {
            uniqueMappings.push_back(mapping);
        }
    }
    if (!CheckIfMappingExists(uniqueMappings, Settings::values.pad_circle_modifier)) {
        uniqueMappings.push_back(Settings::values.pad_circle_modifier);
    }
    return uniqueMappings;
}

/// Builds map of input keys to 3ds buttons for unique device
void InputCore::BuildKeyMapping() {
    key_mappings.clear();
    for (size_t i = 0; i < Settings::values.input_mappings.size(); i++) {
        auto val = KeyMap::mapping_targets[i];
        auto key = Settings::values.input_mappings[i];
        key_mappings.emplace(key, std::vector<Service::HID::PadState>());
        key_mappings[key].push_back(val);
    }
}

/// Generate a device for each unique mapping
void InputCore::GenerateUniqueDevices() {
    auto uniqueMappings = GatherUniqueMappings();
    devices.clear();
    std::shared_ptr<IDevice> input;
    for (const auto& mapping : uniqueMappings) {
        switch (mapping.framework) {
        case Settings::DeviceFramework::SDL: {
            if (mapping.device == Settings::Device::Keyboard) {
                main_keyboard = std::make_shared<Keyboard>();
                input = main_keyboard;
                break;
            } else if (mapping.device == Settings::Device::Gamepad) {
                input = std::make_shared<SDLGamepad>();
                break;
            }
        }
        }
        devices.push_back(input);
        input->InitDevice(mapping.number, mapping);
    }
    if (main_keyboard == nullptr) {
        main_keyboard = std::make_shared<Keyboard>();
        main_keyboard->InitDevice(0, Settings::InputDeviceMapping("SDL/0/Keyboard/-1"));
        devices.push_back(main_keyboard);
    }
}

/// Read settings to initialize devices
void InputCore::ParseSettings() {
    GenerateUniqueDevices();
    BuildKeyMapping();
}

std::tuple<float, float> InputCore::ApplyDeadzone(float x, float y, float dead_zone) {
    float magnitude = std::sqrt((x * x) + (y * y));
    if (magnitude < dead_zone) {
        x = 0;
        y = 0;
    } else {
        float normalized_x = x / magnitude;
        float normalized_y = y / magnitude;
        x = normalized_x * ((magnitude - dead_zone) / (1 - dead_zone));
        y = normalized_y * ((magnitude - dead_zone) / (1 - dead_zone));
    }
    return std::tuple<float, float>(x, y);
}

void InputCore::ReloadSettings() {
    if (devices.empty())
        return;
    std::lock_guard<std::mutex> lock(pad_state_mutex);
    devices.clear();
    ParseSettings();
}

/// Returns all available input devices. Used for key binding in GUI
std::vector<std::shared_ptr<IDevice>> InputCore::GetAllDevices() {
    auto all_devices = SDLGamepad::GetAllDevices();
    auto keyboard = InputCore::GetKeyboard();
    keyboard->InitDevice(0, Settings::InputDeviceMapping("SDL/0/Keyboard/-1"));
    all_devices.push_back(keyboard);

    return all_devices;
}

Settings::InputDeviceMapping InputCore::DetectInput(int max_time,
                                                    std::function<void(void)> update_GUI) {
    auto devices = GetAllDevices();
    for (auto& device : devices) {
        device->Clear();
    }
    Settings::InputDeviceMapping input_device;
    auto start = std::chrono::high_resolution_clock::now();
    while (input_device.key == -1) {
        update_GUI();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::high_resolution_clock::now() - start)
                            .count();
        if (duration >= max_time) {
            break;
        }
        for (auto& device : devices) {
            input_device = device->GetInput();
            if (input_device.key != -1)
                break;
        }
    };
    return input_device;
}
