# Planetside 2 Cheat - PS2Base - PlanetSide 2 DX11 Internal Framework

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Visual Studio](https://img.shields.io/badge/IDE-Visual%20Studio-blue.svg)](https://visualstudio.microsoft.com/)
[![C++](https://img.shields.io/badge/Language-C%2B%2B-blue.svg)](https://isocpp.org/)

![img](https://i.imgur.com/z9uOEpU.jpeg)

## ⚠️ Legal Disclaimer

**This project is for educational and research purposes only.** The code demonstrates:
- Advanced C++ threading and synchronization patterns
- DirectX 11 rendering techniques
- Memory management and process injection
- Game development frameworks

**Use at your own risk.** The authors are not responsible for any misuse of this code.

## 🛠️ Build Instructions

### Quick Start
1. Clone the repository:
```bash
git clone https://github.com/yourusername/PS2Base.git
cd PS2Base
```

2. Open `Internal DX11 Base.sln` in Visual Studio

3. Select **x64** platform and **Debug** or **Release** configuration

4. Build the solution (Ctrl+Shift+B)

## 📁 Project Structure

```
PS2Base/
├── Internal DX11 Base/          # Main project directory
│   ├── Core/                   # Core engine components
│   │   ├── Engine.h/cpp        # Main engine class
│   │   ├── Main.cpp            # Entry point and thread management
│   │   └── dllmain.cpp         # DLL entry point
│   ├── Game/                   # Game-specific components
│   │   ├── SDK.h               # Game structures and enums
│   │   ├── Offsets.h           # Memory offsets
│   │   ├── GameData.h          # Data structures
│   │   └── Game.h/cpp          # Game state management
│   ├── Features/               # Feature implementations
│   │   ├── ESP.h/cpp           # Visual features
│   │   ├── Aimbot.h/cpp        # Targeting system
│   │   ├── MagicBullet.h/cpp   # Bullet manipulation
│   │   └── TargetManager.h/cpp # Target selection
│   ├── Renderer/               # Rendering system
│   │   └── Renderer.h/cpp      # DirectX 11 wrapper
│   ├── Utils/                  # Utility classes
│   │   ├── Vector.h            # Vector mathematics
│   │   ├── Settings.h          # Configuration
│   │   └── Logger.h/cpp        # Logging system
│   └── Framework/             # Third-party libraries
│       └── ImGui/              # UI framework
├── docs/                       # Documentation
├── examples/                   # Usage examples
└── research/                   # Research materials
```

## 🎯 Usage

### Basic Usage
1. Compile the project in Visual Studio
2. The resulting DLL will be in `x64/Debug/` or `x64/Release/`
3. Use appropriate injection methods (not covered in this documentation)

### Configuration
- Press **INSERT** to toggle the menu
- Press **END** to exit
- All settings are configurable through the in-game interface

---

**Remember**: This is an educational project. Use responsibly and in accordance with applicable laws and terms of service.
