#include "config.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

std::string trim(const std::string& s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const auto last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

// Split on the *first* '=' only
std::pair<std::string, std::string> split_key_value(const std::string& line) {
    const auto pos = line.find('=');
    if (pos == std::string::npos) {
        return {trim(line), ""};
    }
    return {trim(line.substr(0, pos)), trim(line.substr(pos + 1))};
}

std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}

std::string xdg_config_home() {
    const char* env = std::getenv("XDG_CONFIG_HOME");
    if (env && *env) return env;
    const char* home = std::getenv("HOME");
    if (home && *home) return std::string(home) + "/.config";
    return "";
}

std::vector<std::string> candidate_paths(const std::optional<std::string>& explicit_path) {
    std::vector<std::string> paths;
    if (explicit_path) {
        paths.push_back(*explicit_path);
        return paths;
    }

    const auto xdg = xdg_config_home();
    if (!xdg.empty()) {
        paths.push_back(xdg + "/mouser/config");
    }
    paths.push_back("~/.config/mouser/config"); // will be expanded if needed, but we use real HOME
    const char* home = std::getenv("HOME");
    if (home && *home) {
        paths.push_back(std::string(home) + "/.config/mouser/config");
        paths.push_back(std::string(home) + "/.mouserrc");
    }
    return paths;
}

// Expand ~ in path (simple version)
std::string expand_tilde(const std::string& p) {
    if (p.size() >= 2 && p[0] == '~' && p[1] == '/') {
        const char* home = std::getenv("HOME");
        if (home && *home) {
            return std::string(home) + p.substr(1);
        }
    }
    return p;
}

} // namespace

const std::vector<std::string>& Config::valid_actions() {
    static const std::vector<std::string> actions = {
        "left", "right", "up", "down",
        "brakes",
        "button1", "button2", "button3",
        "quit"
    };
    return actions;
}

std::optional<std::string> Config::get_key(const std::string& action) const {
    auto it = keys.find(to_lower(action));
    if (it == keys.end()) return std::nullopt;
    return it->second;
}

static bool is_physics_key(const std::string& k) {
    return k == "force" || k == "friction" || k == "friction_brakes";
}

Config load_config(const std::optional<std::string>& explicit_path) {
    Config cfg; // starts with built-in defaults

    const auto candidates = candidate_paths(explicit_path);
    std::string chosen_path;
    std::ifstream file;

    for (const auto& raw : candidates) {
        std::string p = expand_tilde(raw);
        std::ifstream f(p);
        if (f.is_open()) {
            file = std::move(f);
            chosen_path = p;
            break;
        }
    }

    if (!file.is_open()) {
        if (explicit_path) {
            throw std::runtime_error("Config file not found: " + *explicit_path);
        }
        // No config found anywhere — use defaults silently
        return cfg;
    }

    // We found a file — parse it
    std::unordered_set<std::string> seen_keys;
    std::string line;
    int lineno = 0;

    while (std::getline(file, line)) {
        ++lineno;
        std::string t = trim(line);
        if (t.empty() || t[0] == '#') continue;

        auto [kraw, vraw] = split_key_value(t);
        if (kraw.empty()) {
            throw std::runtime_error("Invalid line " + std::to_string(lineno) + ": " + line);
        }

        const std::string key = to_lower(kraw);
        const std::string value = vraw;

        if (is_physics_key(key)) {
            try {
                float val = std::stof(value);
                if (key == "force") cfg.force = val;
                else if (key == "friction") cfg.friction = val;
                else if (key == "friction_brakes") cfg.friction_brakes = val;
            } catch (...) {
                throw std::runtime_error("Bad numeric value for '" + key + "' on line " + std::to_string(lineno));
            }
            continue;
        }

        // Treat as key binding
        bool valid_action = false;
        for (const auto& a : Config::valid_actions()) {
            if (a == key) { valid_action = true; break; }
        }
        if (!valid_action) {
            std::ostringstream oss;
            oss << "Unknown action '" << key << "' on line " << lineno
                << ". Valid actions: ";
            for (size_t i = 0; i < Config::valid_actions().size(); ++i) {
                if (i > 0) oss << ", ";
                oss << Config::valid_actions()[i];
            }
            throw std::runtime_error(oss.str());
        }

        if (value.empty()) {
            throw std::runtime_error("Missing key name for action '" + key + "' on line " + std::to_string(lineno));
        }

        // Check for duplicate action
        if (cfg.keys.count(key)) {
            throw std::runtime_error("Duplicate binding for action '" + key + "' (line " + std::to_string(lineno) + ")");
        }

        cfg.keys[key] = value;
    }

    // If the user gave an explicit file, require at least one binding or physics change?
    // For now we allow empty explicit files (they just get defaults + no custom keys).

    if (explicit_path || !chosen_path.empty()) {
        // Friendly note for the user (not an error)
        // We could log this, but for a grabber tool we keep stdout clean.
        // The caller can print if desired.
    }

    return cfg;
}
