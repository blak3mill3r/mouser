#include <iostream>
#include <optional>
#include <string>

#include <boost/fiber/all.hpp>
#include "mouser.hpp"
#include "config.hpp"

using namespace std;
using namespace boost;
using namespace boost::fibers;
using namespace boost::this_fiber;

static void print_usage(const char* prog) {
    cerr << "Usage: " << prog << " [-c|--config FILE] [--help] [--version]\n"
         << "\n"
         << "  -c, --config FILE   Use explicit config file (keys + physics)\n"
         << "  --help              Show this help\n"
         << "  --version           Show version\n";
}

int main(int argc, char *argv[])
{
    std::optional<std::string> config_path;
    bool show_help = false;
    bool show_version = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-c" || arg == "--config") {
            if (i + 1 >= argc) {
                cerr << "Error: --config requires a filename\n";
                return 1;
            }
            config_path = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            show_help = true;
        } else if (arg == "--version" || arg == "-V") {
            show_version = true;
        } else {
            cerr << "Unknown argument: " << arg << "\n";
            print_usage(argv[0]);
            return 1;
        }
    }

    if (show_help) {
        print_usage(argv[0]);
        return 0;
    }
    if (show_version) {
        cout << "mouser 0.2 (modernized)\n";
        return 0;
    }

    Config cfg;
    try {
        cfg = load_config(config_path);
    } catch (const std::exception& e) {
        cerr << "Config error: " << e.what() << "\n";
        return 1;
    }

    Mouser xlib_loop(cfg);

    fiber xevent_fiber(  [&](){ xlib_loop.xevent_handler();  });
    fiber keyboard_fiber([&](){ xlib_loop.key_handler();     });
    fiber physics_fiber( [&](){ xlib_loop.physics_handler(); });
    fiber xtest_fiber(   [&](){ xlib_loop.xtest_handler();   });

    xlib_loop.run();
    xevent_fiber.join();
    keyboard_fiber.join();
    xtest_fiber.join();
    physics_fiber.join();

    return 0;
}
