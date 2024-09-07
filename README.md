# luchok_libretro

> 64x32 is enough for everybody.

![Playing invaders.luchok](invaders.png)
![Playing snake.luchok](snake.png)

Luchok is Fantasy Console inspired by [Chip-8](https://en.wikipedia.org/wiki/CHIP-8).  It is a [libretro](https://libretro.com) core.

# Features

* Screen: 64x32

* Scripting: Lua

* Sound: 440Hz square wave

* Controls: 16 buttons, labeled from 0 to F


# Building
To build Luchok, you must have [cmake](https://cmake.org) and a build system of your choice.
* Download the repository
* Create `build` folder there.
* Move into `build` folder
* `cmake .. -G "Ninja"`. Cmake supports many different build systems, [Ninja](https://ninja-build.org) is only one of them.
* When finished, move back to the repository root.
* `cmake --build build`
* When finished, there will be `luchok_libretro.dll` or `luchok_libretro.so` in the `bin` folder.

# Running
*retroarch executable*  -L  *luchok library afer building*  **game file**

For example, if on a GNU/Linux system, and you are locared in the repository root, you can run `retroarch -L bin/luchok_libretro.so bin/invaders.luchok`.

In Retroarch, press ScrollLock to switch between retroarch key bindings and Luchok keypad.

# Installing

On Windows, you copy the `.dll` file into the `cores` folder of Retroarch, and copy the `.info` file into the `info` folder.

# Usage
The games for Luchok are written in Lua, with file extension .luchok .

Every frame (60 per second) the function `vblank()` is called.

Each sprite is an array. Consecutive bytes are drawn in verical order. Each bit in a byte represents one picel, MSB to LSB is drawn left to right.

The API for the fantasy console is:

* `cls()` - clears the screen.
* `draw(sprite, x, y)` - draw the array `sprite` at `x` (0-63) and `y` (0-31).
* `draw(sprite, x, y, start, len)` - draw the array `sprite` (`len` (0-31) bytes starting from index `start`) at `x` (0-63) and `y` (0-31).
* `rnd(limit)` - generate a random integer from 0 to `limit` (0-255).
* `key_pressed(num)` - check if the specified key (0-15) is pressed at this frame.
* `key_released(num)` - check if the specified key (0-15) has been released.
* `bcd(num)` - splits the specified number (0-255), into array of its digits
* `get_sprite(num)` - gets the built-in sprite for the specified number (0-15)
* `delay_timer` (0-255) - decrements every frame until it hits 0
* `sound_timer` (0-255) - decrements every frame until it hits 0, each frame it's not 0 a buzzer sounds

# Keys

Luchok has 16 keys. They are located like this:

> 1 2 3 C
> 
> 4 5 6 D
> 
> 7 8 9 E
> 
> A 0 B F

and mapped to:

> 1 2 3 4
> 
> Q W E R
> 
> A S D F
> 
> Z X C V

# License
Licensed under [GNU General Public License 3](https://www.gnu.org/licenses/gpl-3.0.html).


# Author
Roman "shinkarom" Shynkarenko (https://shinkarom.github.io).
