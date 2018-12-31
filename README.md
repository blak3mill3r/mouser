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

* `hjkl` Move the mouse (apply a force)
* `a` Brakes (increase friction)
* `f` Click button 1
* `d` Click button 2
* `s` Click button 3
* You can also double-click...
* `q` Ungrab the keyboard

## Is it any good?

Not really. I mean, I am finding it helpful, but this is a one-day hack with lots of rough edges.

The only way to customize the keybindings is to edit the source and compile it.

## Design

I wrote a blog post about my experience designing and building `mouser`:

http://blake-miller.net/post/mouser-debut/

## Building and Running

```
make
./mouser
```

* and then maybe configure your window manager to run the `mouser` binary when you press some key sequence

## Dependencies

* libboost_fiber
* libboost_context
* libboost_thread
* libboost_system
* libX11
* libXtst

![Mouser](https://vignette.wikia.nocookie.net/fantendo/images/7/77/Mouser_-_Toadette_Strikes.png/revision/latest?cb=20150715175143)
