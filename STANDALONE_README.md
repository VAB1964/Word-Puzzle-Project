# Creating a Standalone Version of Word Puzzle Game

## Quick Start

Run the PowerShell script to automatically create a standalone version:

```powershell
.\create_standalone.ps1
```

This will create a `Standalone` folder with all necessary files.

## Manual Method

If you prefer to create it manually, follow these steps:

### 1. Build the Project

**Important:** Build in **Release x64** configuration for a self-contained executable (no DLLs needed).

1. Open `SFML_TestProject.sln` in Visual Studio
2. Set configuration to **Release** and platform to **x64**
3. Build the solution (Build → Build Solution)

### 2. Create Standalone Folder Structure

Create a folder (e.g., `Standalone`) and copy these files:

```
Standalone/
├── SFML_TestProject.exe          (from x64/Release/)
├── fonts/
│   └── arialbd.ttf               (required)
├── assets/
│   ├── *.png                     (all image files)
│   ├── sounds/
│   │   ├── select_letter.wav
│   │   ├── place_letter.wav
│   │   ├── puzzle_solved.wav
│   │   ├── button_click.wav
│   │   └── hint_used.mp3
│   └── music/
│       ├── track1.mp3
│       ├── track2.mp3
│       ├── track3.mp3
│       ├── track4.mp3
│       └── track5.mp3
└── words_processed.csv            (required)
```

### 3. SFML DLLs (Only if using Debug build)

If you built in **Debug** configuration, you'll need SFML DLLs. Copy these from your SFML installation's `bin` folder:

- `sfml-graphics-3.dll`
- `sfml-audio-3.dll`
- `sfml-window-3.dll`
- `sfml-system-3.dll`
- `sfml-network-3.dll` (if used)

**Note:** Release builds with static linking (`sfml-*-s.lib`) don't need DLLs.

## Troubleshooting

### Game quits immediately

1. **Check console output**: Run from command prompt to see error messages
2. **Verify file paths**: Make sure `fonts/`, `assets/`, and `words_processed.csv` are in the same folder as the `.exe`
3. **Check SFML DLLs**: If using Debug build, ensure all SFML DLLs are present
4. **Missing assets**: Check that all required asset files exist

### Common Issues

- **"FATAL Error loading font"**: Missing `fonts/arialbd.ttf`
- **"Could not load main background texture"**: Missing `assets/BackgroundandFrame.png`
- **"Failed to load word list"**: Missing `words_processed.csv`
- **"The program can't start because sfml-*.dll is missing"**: Need SFML DLLs (Debug build) or rebuild in Release with static linking

## File Locations

- **Executable**: `x64/Release/SFML_TestProject.exe` (or `x64/Debug/` for debug)
- **Fonts**: `fonts/` folder in project root
- **Assets**: `assets/` folder in project root
- **Data**: `words_processed.csv` in project root

## Testing the Standalone Version

1. Navigate to the `Standalone` folder
2. Double-click `SFML_TestProject.exe`
3. If it doesn't work, open Command Prompt in that folder and run:
   ```
   SFML_TestProject.exe
   ```
   This will show any error messages.
