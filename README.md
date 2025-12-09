# Propaganda Spread Model

A simulation of propaganda spreading across the US map, implemented in **C++20** using the **Qt6** framework.

## Prerequisites

To build this project, you need:
* **CMake** (version 3.20 or newer)
* **C++ Compiler** supporting C++20 standard (GCC, Clang, or MSVC)
* **Qt6** (Required components: `Widgets`, `Charts`, `Svg`)

---

## 🐧 Linux Instructions (Ubuntu/Debian/WSL)

### 1. Install Dependencies
Open your terminal and install the required build tools and Qt6 libraries:

```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev libqt6charts6-dev libqt6svg6-dev libxkbcommon-dev
```

### 2. Build the Project
Navigate to the project root directory and run:

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### 3. Run
```bash
./PropagandaSpreadModel
```

## 🪟 Windows Instructions

The easiest way to build and run on Windows is using Visual Studio Code or Visual Studio.
Visual Studio Code (Recommended)

### 1. Install Qt6 via the official Online Installer.

### 2. Install the following extensions in VS Code:
- C/C++ (Microsoft)
- CMake Tools (Microsoft)
### 3. Open the project folder in VS Code.
### 4. When prompted to select a Kit, choose the one matching your installed Qt version (e.g., Visual Studio Community 2022 Release - amd64).
### 5. Click Build on the status bar (bottom of the window).
### 6. Click Run (Play icon).

Note: The CMakeLists.txt includes a post-build step that runs windeployqt automatically to copy necessary DLLs.
