#pragma once

#include <string>
#include <optional>
#include <unordered_map>
#include <vector>

struct Config {
    // action name (lowercase canonical) -> keysym name as string (e.g. "h", "k")
    std::unordered_map<std::string, std::string> keys;

    // New smoothed target velocity physics model
    float max_speed       = 25.0f;  // target speed in pixels per tick with keys held
    float response        = 0.22f;  // 0-1, how quickly actual velocity approaches desired
    float brake_max_speed = 5.0f;   // lower target speed in precision/brakes mode
    float brake_response  = 0.55f;  // higher = snappier response when brakes held

    // Returns the keysym string for a canonical action, or nullopt if not bound
    std::optional<std::string> get_key(const std::string& action) const;

    // All recognized action names (for error messages)
    static const std::vector<std::string>& valid_actions();
};

/// Load configuration.
///
/// If explicit_path is provided, it must exist and be valid (error otherwise).
/// Otherwise searches in the standard order and falls back to built-in defaults.
Config load_config(const std::optional<std::string>& explicit_path = std::nullopt);
