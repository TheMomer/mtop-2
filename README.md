# mtop 2

> Modern Top 2 - A fork of beautiful system monitor for Linux (creator: @AnmiTaliDev)

[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![License](https://img.shields.io/badge/license-Apache%202.0-green.svg)](LICENSE)
[![Meson](https://img.shields.io/badge/build-meson-orange.svg)](https://mesonbuild.com/)

## Features

- 🎨 **Beautiful Unicode interface** with colors
- ⚡ **Fast and lightweight** - minimal system overhead
- 🔧 **Highly configurable** - config files + CLI options
- 📊 **Essential metrics** - CPU, Memory, Load, Processes
- 🎯 **Smart filtering** - hide/show processes by name or user
- 📋 **Multiple sorting** - by memory, CPU, PID, or name

## Quick Start

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install build-essential meson
# Install dependencies (Arch/Arch-based)
sudo pacman -S --needed base-devel git meson

# Clone and build
git clone https://github.com/AnmiTaliDev/mtop.git
cd mtop
meson setup build
meson compile -C build

# Run
./build/mtop
```

## Usage

```bash
# Basic usage
./mtop

# Custom options
./mtop --delay 5 --max-processes 15 --sort-cpu

# Help
./mtop --help
```

## Configuration

Create `~/.config/mtop/config`:

```ini
[display]
update_interval = 2
max_processes = 20
header = true

[processes]
sort_by = memory
hide_processes = kthreadd,ksoftirqd
show_kernel_threads = false
```

## Requirements

- Linux with /proc filesystem
- C++17 compiler (GCC 7+)
- Meson build system


## Contributing

Pull requests welcome! Please maintain the minimalistic design philosophy.

---

*Made with ❤️ for system administrators and developers*