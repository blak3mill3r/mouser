#include <iostream>
#include <boost/fiber/all.hpp>
#include "mouser.hpp"

using namespace std;
using namespace boost;
using namespace boost::fibers;
using namespace boost::this_fiber;

int main(int argc, char *argv[])
{
  Mouser xlib_loop;

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
