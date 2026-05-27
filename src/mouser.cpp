#include "mouser.hpp"

#include <iostream>

using namespace std;
using namespace boost;

using namespace boost::fibers;
using namespace boost::this_fiber;

Mouser::Mouser(const Config& cfg)
  : _d(XOpenDisplay(NULL)),
    _x_fd(ConnectionNumber(_d)),
    force(cfg.force),
    friction(cfg.friction),
    friction_brakes(cfg.friction_brakes)
{
  if (_d == NULL) {
    fprintf(stderr, "Cannot open display\n");
    exit(1);
  }

  // Build the runtime keymap from config (keysym string -> Action)
  for (const auto& [action_name, keysym] : cfg.keys) {
    Action act;
    if (action_name == "left")       act = Action::Left;
    else if (action_name == "right") act = Action::Right;
    else if (action_name == "up")    act = Action::Up;
    else if (action_name == "down")  act = Action::Down;
    else if (action_name == "brakes") act = Action::Brakes;
    else if (action_name == "button1") act = Action::Click1;
    else if (action_name == "button2") act = Action::Click2;
    else if (action_name == "button3") act = Action::Click3;
    else if (action_name == "quit")  act = Action::Quit;
    else continue; // should have been validated by load_config

    key_to_action[keysym] = act;
  }

  // If user provided no custom bindings at all, fall back to classic defaults
  if (key_to_action.empty()) {
    key_to_action["h"] = Action::Left;
    key_to_action["l"] = Action::Right;
    key_to_action["k"] = Action::Up;
    key_to_action["j"] = Action::Down;
    key_to_action["a"] = Action::Brakes;
    key_to_action["f"] = Action::Click1;
    key_to_action["d"] = Action::Click2;
    key_to_action["s"] = Action::Click3;
    key_to_action["q"] = Action::Quit;
  }

  // TODO(Blake): test whether AutoRepeat is on in the first place; restore its value afterwards
  XAutoRepeatOff(_d);
  XGrabKeyboard(_d, DefaultRootWindow(_d), False, GrabModeAsync, GrabModeAsync, CurrentTime);
}

Mouser::~Mouser() {
  XUngrabKeyboard(_d, CurrentTime);
  // TODO(Blake): test whether AutoRepeat is on in the first place; restore its value afterwards
  XAutoRepeatOn(_d);
  XCloseDisplay(_d);
}

// With thanks to Aaron Digulla
// https://stackoverflow.com/questions/8592292/how-to-quit-the-blocking-of-xlibs-xnextevent
void Mouser::run() {
  struct timeval tv;
  XEvent ev;
  fd_set in_fds;
  while( _running ) {
    // Create a File Description Set containing x_fd
    FD_ZERO(&in_fds);
    FD_SET(_x_fd, &in_fds);

    // eight milliseconds
    tv.tv_usec = 1000 * 8;
    tv.tv_sec = 0;

    // Wait for X Event or the timeout
    int num_ready_fds = select(_x_fd + 1, &in_fds, NULL, NULL, &tv);
    if (num_ready_fds == 0)
      boost::this_fiber::yield();
    else if (num_ready_fds > 0)
      // Put XEvents onto the channel
      while(XPending(_d)) {
        XNextEvent(_d, &ev);
        _xevent_channel.push(ev);
      }
    else {
      printf("Fatal: select() returned %d\n", num_ready_fds);
      exit(1);
    }
  }
}

void Mouser::xevent_handler() {
  XEvent ev;
  while (_running && channel_op_status::success == _xevent_channel.pop(ev) ) {
    switch(ev.type) {
    case KeyPress:
    case KeyRelease: {
      bool is_down = (ev.type == KeyPress);
      unsigned int kc = ((XKeyPressedEvent*)&ev)->keycode;

      // Modern non-deprecated lookup (XKB)
      KeySym ks = XkbKeycodeToKeysym(_d, kc, 0, 0);
      const char* s = XKeysymToString(ks);
      if (!s) break;

      std::string keysym = s;

      // Look up in our loaded keymap first
      auto it = key_to_action.find(keysym);
      if (it != key_to_action.end()) {
        Action act = it->second;

        switch (act) {
        case Action::Quit:
          if (is_down) stop();
          break;
        case Action::Click1:
          _xtest_action_channel.push(CmdButton(1, is_down));
          break;
        case Action::Click2:
          _xtest_action_channel.push(CmdButton(2, is_down));
          break;
        case Action::Click3:
          _xtest_action_channel.push(CmdButton(3, is_down));
          break;
        case Action::Left:
          _keyboard_action_channel.push(KeyboardAction(k_left, is_down));
          break;
        case Action::Right:
          _keyboard_action_channel.push(KeyboardAction(k_right, is_down));
          break;
        case Action::Up:
          _keyboard_action_channel.push(KeyboardAction(k_up, is_down));
          break;
        case Action::Down:
          _keyboard_action_channel.push(KeyboardAction(k_down, is_down));
          break;
        case Action::Brakes:
          _keyboard_action_channel.push(KeyboardAction(k_brakes, is_down));
          break;
        }
        break; // handled via config
      }

      // No custom binding for this keysym — ignore (old hardcoded behavior is now fully in config)
      break;
    }
    }
  }
}

void Mouser::key_handler() {
  KeyboardAction action;
  while (_running && channel_op_status::success == _keyboard_action_channel.pop(action)) {
    switch(action.control_key){
    case k_left:  fx += (action.on_off ? -force : +force); break;
    case k_right: fx += (action.on_off ? +force : -force); break;
    case k_up:    fy += (action.on_off ? -force : +force); break;
    case k_down:  fy += (action.on_off ? +force : -force); break;
    case k_brakes: brakes = action.on_off; break;
    default: break;
    }
  }
}

void Mouser::stop() {
  _running = false;
  _keyboard_action_channel.close();
  _xtest_action_channel.close();
  _xevent_channel.close();
}

void Mouser::physics_handler() {
  while (_running) {
    vx += fx;
    vy += fy;
    vx *= (brakes ? friction_brakes : friction);
    vy *= (brakes ? friction_brakes : friction);
    deltax += vx;
    deltay += vy;
    int mx=0, my=0;
    if(abs(deltax) >= 1.0) mx = int(deltax);
    if(abs(deltay) >= 1.0) my = int(deltay);
    if(mx != 0) deltax -= mx;
    if(my != 0) deltay -= my;
    if(mx != 0 || my != 0) _xtest_action_channel.push(make_pair(mx,my));

    boost::this_fiber::sleep_for(std::chrono::milliseconds(8));
  }
}

void Mouser::xtest_handler() {
  Command cmd;
  while (_running && channel_op_status::success == _xtest_action_channel.pop(cmd))
    do_command(cmd);
}

void Mouser::relative_move(int x, int y) {
  XTestFakeRelativeMotionEvent(_d, x, y, CurrentTime);
  XFlush(_d);
}

void Mouser::button(int n, bool pressed) {
  XTestFakeButtonEvent(_d, n, pressed, 0);
}

void Mouser::do_command(Command c) {
  std::visit( Mouser::command_visitor(this), c );
}
