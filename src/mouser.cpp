#include "mouser.hpp"

using namespace std;
using namespace boost;

using namespace boost::fibers;
using namespace boost::this_fiber;

Mouser::Mouser() : _d(XOpenDisplay(NULL)), _x_fd(ConnectionNumber(_d)) {
  if (_d == NULL) {
    fprintf(stderr, "Cannot open display\n");
    exit(1);
  }
  XAutoRepeatOff(_d);
  XGrabKeyboard(_d, DefaultRootWindow(_d), False, GrabModeAsync, GrabModeAsync, CurrentTime);
}

Mouser::~Mouser() {
  XUngrabKeyboard(_d, CurrentTime);
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
    else
      printf("An error occured!\n");
  }
}

void Mouser::xevent_handler() {
  XEvent ev;
  while (_running && channel_op_status::success == _xevent_channel.pop(ev) ) {
    switch(ev.type) {
    case KeyPress:
    case KeyRelease:
      bool is_down = (KeyPress == ev.type);
      char *s;
      unsigned int kc;

      kc = ((XKeyPressedEvent*)&ev)->keycode;
      s = XKeysymToString(XKeycodeToKeysym(_d, kc, 0));

      //if(s) printf("KEY: %s\n", s);
      if(!strcmp(s, "q")) { stop(); break; }

      // probably cleaner to make KeyboardActions out of the button presses too...
      if(!strcmp(s, "f"))          _xtest_action_channel.push(CmdButton(1,   is_down));
      if(!strcmp(s, "d"))          _xtest_action_channel.push(CmdButton(2,   is_down));
      if(!strcmp(s, "s"))          _xtest_action_channel.push(CmdButton(3,   is_down));
      if(!strcmp(s, "h"))          _keyboard_action_channel.push(KeyboardAction(k_left,    is_down));
      if(!strcmp(s, "l"))          _keyboard_action_channel.push(KeyboardAction(k_right,   is_down));
      if(!strcmp(s, "k"))          _keyboard_action_channel.push(KeyboardAction(k_up,      is_down));
      if(!strcmp(s, "j"))          _keyboard_action_channel.push(KeyboardAction(k_down,    is_down));
      if(!strcmp(s, "a"))          _keyboard_action_channel.push(KeyboardAction(k_brakes,  is_down));
    }
  }
}

void Mouser::key_handler() {
  KeyboardAction action;
  while (_running && channel_op_status::success == _keyboard_action_channel.pop(action)) {
    switch(action.control_key){
    case k_left:  fx += (action.on_off ? -FORCE : +FORCE); break;
    case k_right: fx += (action.on_off ? +FORCE : -FORCE); break;
    case k_up:    fy += (action.on_off ? -FORCE : +FORCE); break;
    case k_down:  fy += (action.on_off ? +FORCE : -FORCE); break;
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
    vx *= (brakes ? FRICTION_BRAKES : FRICTION);
    vy *= (brakes ? FRICTION_BRAKES : FRICTION);
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
  boost::apply_visitor( Mouser::command_visitor(this), c );
}
