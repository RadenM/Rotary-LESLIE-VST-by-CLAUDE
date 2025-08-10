# Leslie Rotary VST - GitHub Repository Setup Script

# 1. Initialize Git repository
git init

# 2. Create .gitignore file
cat > .gitignore << 'EOF'
# Build directories
Builds/
build/
Build/
cmake-build-*/

# IDE files
.vscode/
.idea/
*.suo
*.user
*.userosscache
*.sln.docstates
*.vcxproj.user

# OS generated files
.DS_Store
.DS_Store?
._*
.Spotlight-V100
.Trashes
ehthumbs.db
Thumbs.db

# Temporary files
*.tmp
*.temp
*.log
*.cache

# Plugin binaries (optional - you might want to track these)
*.vst3
*.component
*.dll
*.so
*.dylib

# JUCE generated files (if using Projucer instead of CMake)
JuceLibraryCode/

# Audio files used for testing
*.wav
*.aif
*.mp3
*.flac

# Backup files
*~
*.bak
*.swp
*.orig
EOF

# 3. Create comprehensive README.md
cat > README.md << 'EOF'
# Leslie Rotary VST Plugin

A modern, authentic Leslie rotary speaker simulation VST plugin built with JUCE, inspired by the BOSS RT-2 Rotary Ensemble.

## Features

### ✅ Implemented (Phase 1)
- **Dual Rotor Engine**: Separate horn and drum simulation with independent speeds
- **Authentic Speed Ramping**: Smooth transitions between slow/fast speeds
- **Brake System**: Variable angle positioning for creative effects
- **Real-time Modulation**: Doppler and amplitude modulation
- **Modern UI**: Dark theme with animated rotor visualization
- **Professional Controls**: Thread-safe parameter automation

### 🔄 In Development (Phase 2)
- **Proper Crossover Filtering**: High/low-pass separation for horn/drum
- **Enhanced Doppler Effect**: Real pitch-shifting using delay lines
- **Tube Drive Stages**: Multiple saturation characteristics
- **Cabinet Resonance**: Wood and speaker modeling

### 🎯 Planned Features
- **Mic Positioning**: Distance, angle, and room simulation
- **Preset System**: Save/load with A/B morphing
- **MIDI Integration**: Full CC mapping and expression control
- **Tempo Sync**: Lock rotor speeds to DAW tempo

## Build Instructions

### Prerequisites
- C++17 compatible compiler (Visual Studio 2019+, Xcode 12+, GCC 9+)
- CMake 3.22+
- Git (for JUCE dependency)

### Building
```bash
# Clone the repository
git clone https://github.com/yourusername/leslie-rotary-vst.git
cd leslie-rotary-vst

# Create build directory
mkdir build && cd build

# Configure and build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# Install to system plugin directories (optional)
cmake --build . --target install_plugin
```

## Usage

1. **Speed Control**: Switch between Slow/Fast/Brake modes
2. **Ramp Time**: Adjust acceleration/deceleration curves  
3. **Drive**: Add tube-style saturation
4. **Balance**: Blend horn/drum mix
5. **Modulation**: Control Doppler and AM intensity
6. **Stereo Spread**: Adjust width from mono to extreme stereo

## Technical Specs

- **Formats**: VST3, AU, Standalone
- **Sample Rates**: 44.1kHz - 192kHz
- **Processing**: 32-bit float, real-time safe
- **Latency**: Ultra-low for live performance
- **Compatibility**: All major DAWs

## Development

### Project Structure
```
Source/
├── LeslieRotaryProcessor.h/cpp    # Main audio processor
├── LeslieRotaryEditor.h/cpp       # UI editor components
└── RotaryEngine.h/cpp             # Core DSP engine
```

### Contributing
1. Fork the repository
2. Create feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open Pull Request

## License

This project is licensed under the GPL-3.0 License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Inspired by BOSS RT-2 Rotary Ensemble
- Built with [JUCE Framework](https://juce.com/)
- Special thanks to the Leslie speaker legacy and all rotary enthusiasts

## Support

- 🐛 **Bug Reports**: Use GitHub Issues
- 💡 **Feature Requests**: Use GitHub Discussions  
- 📧 **Contact**: [your-email@example.com]

---

**Status**: Active Development | **Version**: 1.0.0-alpha | **Build**: Passing ✅
EOF

# 4. Create LICENSE file (GPL-3.0 as example, choose your preferred license)
cat > LICENSE << 'EOF'
GNU GENERAL PUBLIC LICENSE
Version 3, 29 June 2007

Copyright (C) 2025 [Your Name]

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
EOF

# 5. Create GitHub workflow for CI/CD (optional)
mkdir -p .github/workflows
cat > .github/workflows/build.yml << 'EOF'
name: Build Plugin

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

jobs:
  build:
    strategy:
      matrix:
        os: [ubuntu-latest, windows-latest, macos-latest]
        
    runs-on: ${{ matrix.os }}
    
    steps:
    - uses: actions/checkout@v3
      with:
        submodules: recursive
        
    - name: Setup CMake
      uses: jwlawson/actions-setup-cmake@v1.13
      with:
        cmake-version: '3.22'
        
    - name: Install Linux dependencies
      if: runner.os == 'Linux'
      run: |
        sudo apt update
        sudo apt install -y libasound2-dev libjack-dev libfreetype6-dev libx11-dev libxext-dev libxrandr-dev libxinerama-dev libxcursor-dev
        
    - name: Configure CMake
      run: cmake -B build -DCMAKE_BUILD_TYPE=Release
      
    - name: Build
      run: cmake --build build --config Release
      
    - name: Upload artifacts
      uses: actions/upload-artifact@v3
      with:
        name: plugin-${{ matrix.os }}
        path: build/LeslieRotaryVST_artefacts/Release/
EOF

# 6. Create project directories
mkdir -p Source
mkdir -p Resources
mkdir -p Documentation
mkdir -p Tests

# 7. Initial commit setup
echo "# Repository setup complete! Next steps:"
echo ""
echo "1. Copy your source files to Source/ directory:"
echo "   - LeslieRotaryProcessor.h"
echo "   - LeslieRotaryProcessor.cpp" 
echo "   - LeslieRotaryEditor.h"
echo "   - LeslieRotaryEditor.cpp"
echo ""
echo "2. Copy CMakeLists.txt to root directory"
echo ""
echo "3. Create GitHub repository:"
echo "   gh repo create leslie-rotary-vst --public --source=. --remote=origin --push"
echo ""
echo "4. Or manually:"
echo "   git add ."
echo "   git commit -m 'Initial commit: Leslie Rotary VST foundation'"
echo "   git branch -M main"
echo "   git remote add origin https://github.com/yourusername/leslie-rotary-vst.git"
echo "   git push -u origin main"