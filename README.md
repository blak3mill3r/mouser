# Mouser
> Use your keyboard as a virtual (XWindows) mouse

## Why?
> So you don't have to reach for your mouse

## Alternatives
> I used `keynav` before this. It lets you repeatedly bisect the screen to narrow in on the point you want.
> I know I've seen other "virtual mice" out there too

## Physics

What's different about `mouser` is that it lets you move the mouse with a little physics simulation, like a video game.

When you run `mouser`, it grabs the keyboard, until you press `q`.

## Controls

Default bindings (customizable — see Configuration below):

* `hjkl` Move the mouse (smoothed target velocity model)
* `a` Precision / brakes modifier (lower speed + snappier response)
* `f` Click button 1
* `d` Click button 2
* `s` Click button 3
* `q` Ungrab the keyboard

You can also double-click by pressing the button key twice quickly.

## Configuration

Create a file at `~/.config/mouser/config` (or `$XDG_CONFIG_HOME/mouser/config`).

Example:

```
left   = h
right  = l
up     = k
down   = j
brakes = a
button1 = f
button2 = d
button3 = s
quit   = q

# Optional physics tuning (new smoothed target velocity model)
max_speed       = 25.0
response        = 0.22
brake_max_speed = 5.0
brake_response  = 0.55
```

See `config.example` in the repo for a documented template.

You can also point at a specific file on the command line:

```
mouser -c ~/my-special-bindings
```

Run `mouser --help` for the full list of options.

A config file lets you completely change the feel and the keys without recompiling.

## Design

I wrote a blog post about my experience designing and building `mouser`:

http://blake-miller.net/post/mouser-debut/

## Building and Running

```
make
./mouser
```

Then (recommended) configure your window manager to launch the `mouser` binary on some key sequence.

`make clean` is also available.

## Dependencies

* C++17 compiler (clang++ or g++)
* libboost_fiber + libboost_context (the fiber library is still used for the lightweight concurrent handlers)
* libboost_thread + libboost_system
* libX11 + libXtst (XKBlib is used for modern key symbol lookup)

No other runtime dependencies. The old `-Wno-deprecated-declarations` hack is gone.

![Mouser](https://vignette.wikia.nocookie.net/fantendo/images/7/77/Mouser_-_Toadette_Strikes.png/revision/latest?cb=20150715175143)
