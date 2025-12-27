# SIT — System Insight Toolkit

**SIT** is a lightweight, cross-platform **System Information Toolkit** written in C++.
It provides essential system details and real-time usage metrics with a clean and minimal design.

The goal of SIT is to offer **clear visibility into system state** without unnecessary complexity,
making it useful for learning, diagnostics, and future system monitoring extensions.

---

## Features

![System Start Menu](assest/sit1.2.png)

### System Information
- Operating System name
- CPU model
- Total RAM size
- Total disk capacity
- System uptime
- Current user information

### System Usage
- CPU usage percentage
- RAM usage percentage
- DISK usage percentage

### System Health Score
- CPU health score based on its usage
- RAM health score based on its usage
- DISK health score based on its usage
- Overall health score

### CLI 
- Active cli commands
- Help & Version commands added
- System usage commands
- System info commands
- System health commands
- System all for listing all system info , usage & health score 

All usage values are calculated using **native OS APIs**, not estimates.

---

## Usage

SIT supports both command-line and interactive modes.

### Command-Line Mode

```bash
./sysinfo [command]
```

#### Commands
- `info`     - Show basic system information
- `usage`    - Show system resource usage
- `health`   - Show system health status
- `all`      - Show all information

#### Options
- `--help`, `-h`    - Show help message
- `--version`, `-v` - Show version information

#### Examples
```bash
./sysinfo --help
./sysinfo info
./sysinfo usage
./sysinfo health
./sysinfo all
```

### Interactive Mode

If no arguments are provided, SIT starts in interactive shell mode.

```bash
./sysinfo
```

You will see a prompt `sysinfo> ` where you can type commands like `info`, `usage`, `health`, `all`, `help`, `version`, or `exit` to quit.

Example session:
```
sysinfo> help
System Information Tool Commands

Commands:
  info     Show basic system information
  usage    Show system resource usage
  health   Show system health status
  all      Show all information
  help     Show this help message
  version  Show version information
  exit     Exit the tool

sysinfo> info
---------- System Basic Info ----------
...
sysinfo> exit
Exiting System Information Tool.
```

---

## Planned Features

The following features are planned and currently under development:

- [ ] System problems scanner (errors / weaknesses indicators)
- [ ] File integrity checker

These additions are designed to keep SIT **lightweight, transparent, and safe**.

---

## Supported Platforms

- **Windows** (via WinAPI)
- **Linux** (via `/proc` and standard system files)

The codebase uses conditional compilation to keep platform-specific logic isolated.

---

## Design Philosophy

- Minimal dependencies
- Native OS APIs only
- Clear and readable C++ code
- No background services or invasive system access
- Easy to extend without refactoring the core

SIT is intentionally built as a **toolkit**.

---

## Build Instructions

### Requirements
- C++17 compatible compiler
- Windows: MinGW / MSVC  
- Linux: GCC or Clang  

### Build (example)
```bash
g++ -o sysinfo main.cpp cli.cpp systemInfo.cpp health.cpp -std=c++17

OR

.\sysinfo (in vs code terminal)
