#ifndef MOUSER_H
#define MOUSER_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XTest.h>
#include <chrono>
#include <cstring>
#include <iostream>
#include <optional>
#include <unordered_map>
#include <variant>

#include <boost/fiber/all.hpp>

#include "config.hpp"

// High-level actions that can be bound to keys
enum class Action {
    Left,
    Right,
    Up,
    Down,
    Brakes,
    Click1,
    Click2,
    Click3,
    Quit
};

enum ControlKey {k_up, k_down, k_left, k_right, k_button_1, k_button_2, k_button_3, k_brakes};

struct KeyboardAction {
  ControlKey control_key;
  bool on_off;
  KeyboardAction(ControlKey d, bool o) : control_key(d), on_off(o) {}
  KeyboardAction() {}
};

typedef std::pair<int,int> CmdMovement;
struct CmdButton {
  int button;
  bool pressed;
  CmdButton(int b, bool p) : button(b), pressed(p) {}
};

using Command = std::variant<CmdMovement, CmdButton>;

// Channel types
typedef boost::fibers::unbuffered_channel< XEvent > xevent_channel_t;
typedef boost::fibers::buffered_channel< KeyboardAction > keyboard_action_channel_t;
typedef boost::fibers::buffered_channel< Command > xtest_action_channel_t;

class Mouser
{
public:
  explicit Mouser(const Config& cfg = Config{});
  virtual ~Mouser();
  void run();
  //xevent_channel_t channel();
  void stop();
  void xevent_handler();
  void key_handler();
  void physics_handler();
  void xtest_handler();

  xevent_channel_t _xevent_channel;
  keyboard_action_channel_t _keyboard_action_channel{4};
  xtest_action_channel_t _xtest_action_channel{8};

  void relative_move(int x, int y);

  void button(int n, bool pressed);

  void do_command(Command c);

private:
  Display * _d;
  int _x_fd;
  bool _running{true};

  float vx = 0.0f;
  float vy = 0.0f;
  float deltax = 0.0f;
  float deltay = 0.0f;

  // New smoothed target velocity model state
  float max_speed       = 25.0f;
  float response        = 0.22f;
  float brake_max_speed = 5.0f;
  float brake_response  = 0.55f;

  // Current pressed state for directional keys + brakes modifier
  bool dir_left  = false;
  bool dir_right = false;
  bool dir_up    = false;
  bool dir_down  = false;
  bool brakes    = false;

  // keysym string (e.g. "h") -> Action
  std::unordered_map<std::string, Action> key_to_action;

  // Modern visitor using std::visit (no longer inherits from boost::static_visitor)
  struct command_visitor {
    Mouser *m;
    explicit command_visitor(Mouser *mover) : m(mover) { }

    void operator()(CmdMovement c) const {
      m->relative_move(c.first, c.second);
    }

    void operator()(CmdButton c) const {
      m->button(c.button, c.pressed);
    }
  };
};

#endif /* MOUSER_H */
