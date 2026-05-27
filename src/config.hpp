#pragma once

#include <string>
#include <optional>
#include <unordered_map>
#include <vector>

struct Config {
    // action name (lowercase canonical) -> keysym name as string (e.g. "h", "k")
    std::unordered_map<std::string, std::string> keys;

    float force = 2.8f;
    float friction = 0.995f;
    float friction_brakes = 0.60f;

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
