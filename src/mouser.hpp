#ifndef MOUSER_H
#define MOUSER_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>
#include <cstring>
#include <iostream>
#include <boost/fiber/all.hpp>
#include <boost/variant.hpp>
#include <boost/chrono.hpp>

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

typedef boost::variant< CmdMovement, CmdButton > Command;

// Channel types
typedef boost::fibers::unbounded_channel< XEvent > xevent_channel_t;
typedef boost::fibers::bounded_channel< KeyboardAction > keyboard_action_channel_t;
typedef boost::fibers::bounded_channel< Command > xtest_action_channel_t;

class Mouser
{
public:
  Mouser();
  virtual ~Mouser();
  void run();
  //xevent_channel_t channel();
  void stop();
  void xevent_handler();
  void key_handler();
  void physics_handler();
  void xtest_handler();

  xevent_channel_t _xevent_channel;
  keyboard_action_channel_t _keyboard_action_channel;
  xtest_action_channel_t _xtest_action_channel;

  void relative_move(int x, int y);

  void button(int n, bool pressed);

  void do_command(Command c);

private:
  Display * _d;
  int _x_fd;
  bool _running;

  float fx = 0.0f;
  float fy = 0.0f;
  float vx = 0.0f;
  float vy = 0.0f;
  float deltax = 0.0f;
  float deltay = 0.0f;

  const float FORCE = 0.8f;
  const float FRICTION = 0.98f;
  const float FRICTION_BRAKES = 0.80f;
  bool brakes = false;

  class command_visitor : public boost::static_visitor<void> {
  protected:
    Mouser *m;
  public:
    explicit command_visitor(Mouser *mover) : m(mover) { }

    void operator()(CmdMovement c) const
    {
      m->relative_move(c.first,c.second);
    }
    
    void operator()(CmdButton c) const
    {
      m->button(c.button,c.pressed);
    }
  };
};

#endif /* MOUSER_H */
