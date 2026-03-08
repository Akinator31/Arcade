# Arcade

Arcade is a modular gaming platform developed in C++20. It serves as a launcher that allows users to play various retro games using different graphical backends. The core design principle of the project is modularity: both games and graphics engines are implemented as standalone shared libraries (`.so` files) that are loaded dynamically at runtime.

The program enables the seamless swapping of graphics libraries (e.g., switching from a terminal-based interface to a windowed one) and games without needing to restart or recompile the application.

## Architecture

The project is divided into three distinct components:

* **The Core**: The main program responsible for the initial launcher, handling user input, and managing the dynamic loading of libraries.


* **Graphics Libraries**: Plug-ins that handle window management, rendering, and event capturing (e.g., ncurses, SDL2, SFML).


* **Game Libraries**: Plug-ins containing the internal logic of specific games (e.g., Snake, Pacman).



All communication between the Core and the libraries is handled through standardized interfaces to ensure that any game can run on any graphics library.

## Prerequisites

To build and run this project, you will need:

* A C++20 compatible compiler (GCC or Clang).
* CMake (version 3.20 or higher).
* Development files for the graphics libraries you intend to use (e.g., `libncurses-dev`, `libsdl2-dev`).
* **Criterion** (optional, for running unit tests).

## Compilation

The project uses CMake for the build process.

### Building the Core

To compile the main `arcade` executable:

```bash
mkdir build && cd build
cmake ..
make

```

### Building with Unit Tests

If you have Criterion installed and wish to build the test suite:

```bash
cmake .. -DBUILD_TESTS=ON
make
./unittests

```

### Library Placement

Per the project requirements, all compiled dynamic libraries (`.so`) must be placed in a directory named `lib` at the root of the repository to be recognized by the launcher.

## Usage

The program requires an initial graphics library as a startup argument:

```bash
./arcade ./lib/arcade_ncurses.so

```

### Controls

WIP



## Documentation

A doxygen documentation is available. Check the link in the repository presentation for more details.
