# CHIP-8 Emulator

A CHIP-8 emulator written in **C** using **raylib** for graphics and input.

This project implements the complete CHIP-8 virtual machine, including:

* 4 KB memory
* 16 general-purpose registers (V0–VF)
* Stack and subroutines
* Delay and sound timers
* Sprite rendering
* Keyboard input
* ROM loading
* Collision detection

The emulator executes instructions at approximately **700 instructions per second** and renders at **60 FPS**.

---

## Screenshot

Add a screenshot here:

```md
![Screenshot](images/screenshot.png)
```

---

## Features

* Complete CHIP-8 instruction set
* Cross-platform support

  * Windows
  * Linux
* ROM browser from the terminal
* 64×32 monochrome display
* Configurable display scaling
* Sprite wrapping support
* Built-in CHIP-8 font set
* Runtime validation and error checking

---

## Controls

The CHIP-8 keypad is mapped to the keyboard as follows:

| CHIP-8 | Keyboard |
| ------ | -------- |
| 1      | 1        |
| 2      | 2        |
| 3      | 3        |
| C      | 4        |
| 4      | Q        |
| 5      | W        |
| 6      | E        |
| D      | R        |
| 7      | A        |
| 8      | S        |
| 9      | D        |
| E      | F        |
| A      | Z        |
| 0      | X        |
| B      | C        |
| F      | V        |

Visual layout:

```text
CHIP-8         Keyboard

1 2 3 C        1 2 3 4
4 5 6 D   ->   Q W E R
7 8 9 E        A S D F
A 0 B F        Z X C V
```

---

## Building

### Requirements

* C compiler (GCC, Clang, or MSVC)
* raylib

### Linux

```bash
gcc main.c -o chip8 \
    -lraylib \
    -lGL \
    -lm \
    -lpthread \
    -ldl \
    -lrt \
    -lX11
```

### Windows (MinGW)

```bash
gcc main.c -o chip8.exe -lraylib -lopengl32 -lgdi32 -lwinmm
```

---

## ROM Directory

Place CHIP-8 ROMs inside a `roms` directory:

```text
project/
├── src/
├── roms/
│   ├── Pong.ch8
│   ├── Tetris.ch8
│   └── Invaders.ch8
└── README.md
```

When the emulator starts it will list all available ROMs:

```text
==== Chip 8 Emulator ====

Roms:
Pong.ch8
Tetris.ch8
Invaders.ch8
```

Enter the ROM filename to launch it.

---

## Technical Details

### Memory Layout

| Address Range | Purpose                 |
| ------------- | ----------------------- |
| 0x000–0x1FF   | Interpreter / font data |
| 0x200–0xFFF   | Program ROM and RAM     |

Programs are loaded starting at address `0x200`, following the original CHIP-8 convention.

### Display

* Resolution: **64 × 32**
* Scale factor: **16**
* Window size: **1024 × 512**

### Timers

The emulator implements:

* Delay Timer
* Sound Timer

Both decrement at **60 Hz**.

---

## Current Limitations

* No audio output yet (`Beep!` is printed to the console instead)
* No support for Super-CHIP extensions
* ROM path is currently relative to the project structure

---

## Example ROMs

Popular ROMs to test:

* Pong
* Tetris
* Space Invaders
* Breakout
* IBM Logo

---

## References

* Cowgod's CHIP-8 Technical Reference
* CHIP-8 Wikipedia Page
* Tobias V. Langhoff's CHIP-8 Guide

---

## License

MIT License

```

Feel free to use, modify, and distribute.
```
