# Qt6 Linux Installer App - Design Spec

## Goal

Build a C++/Qt6 Linux application that pixel-perfectly replicates the CODEX-Installer's Windows UI, using theme data extracted by the VSF parser. All widgets functional with placeholder install logic for visual comparison testing.

## Architecture

Single Qt6 C++ application with frameless window, absolute widget positioning matching the Pascal `SetBounds()` calls from setup.iss, QSS theming loaded from VSF parser output, Qt6 Multimedia for OGG audio playback.

## Window Spec

- Client area: 480x580 pixels (from `WizardForm.ClientWidth/ClientHeight`)
- Frameless (`Qt::FramelessWindowHint`)
- Centered on screen (`Qt::WindowStaysOnTopHint` not needed)
- Draggable by logo/title area
- Fade-in on launch (opacity animation)

## Widget Layout

All positions in pixels, matching `InitializeWizard()` from setup.iss at 96 DPI (ScaleX/ScaleY = 1:1).

### Top Section
- **Logo** (`QLabel` with pixmap): centered at y=0, 450x65, loaded from `Logo5.bmp`

### Directories Section (`bvlDirectories`)
- Outer bevel: x=10, y=60, w=460, h=180, box border (1px raised)

#### Install Directory (`bvlDirInstall`)
- Inner bevel: x=20, y=70, w=440, h=75, sunken box
- `lblDirInstall`: "Install directory", x=30, y=75, bold Arial 9pt, color #e1e0e6
- `cbxDrive`: drive combo, x=30, y=95, w=40
- `DirEdit`: path input, x=75, y=95, w=305
- `btnDirBrowse`: "Browse...", x=385, y=94, w=80, h=23
- `chbDesktopIcon`: checkbox at x=30, y=120 + label "Create desktop shortcut"

#### Start Menu Directory (`bvlIconGroup`)
- Inner bevel: x=20, y=155, w=440, h=75, sunken box
- `lblGroupDir`: "Directory at Start Menu", x=30, y=160, bold Arial 9pt
- `GroupEdit`: text input, x=30, y=180, w=345
- `btnGroupBrowse`: "Browse...", x=380, y=179, w=80, h=23
- `chbCreateGroup`: checkbox + label "Create a Start Menu folder"

### Options Section (`bvlOptions`)
- Bevel: x=10, y=250, w=460, h=68, sunken box

#### Install Options (`bvlInstallOptions`)
- Inner bevel: x=20, y=260, w=440, h=48
- `lblDiskSizeNeeded`: "At least 4.51 GB of free space required", x=30, y=265
- `chbNoUninstaller`: checkbox + label "Do not create uninstaller..."
- `chbCopyCrack`: checkbox + label "Copy contents of CODEX directory" (hidden by default)
- Audio controls (right side of bvlInstallOptions): Play/Pause bitmap buttons (11x11), volume track bar

### Buttons Section (`bvlButtons`)
- Bevel: x=10, y=328, w=460, h=50
- `bvlLeftButton`: x=15, y=333, w=224, h=40
  - `btnLeftButton`: "Exit" / "Cancel", fills inner area with 5px padding
- `bvlRightButton`: x=245, y=333, w=224, h=40
  - `btnRightButton`: "Install" / "Finish", fills inner area with 5px padding

### Progress Section (`bvlProgressForm`)
- Bevel: x=10, y=388, w=460, h=remaining
- `bvlProgressGauge`: inner bevel with 10px margin
  - `ProgressBar`: x=30, y=408, w=420, h=15
  - `memProgressLog`: read-only text area below progress bar

### Result Label
- `lblInstallResult`: centered horizontally, below progress section, Tahoma bold 14pt, hidden until finished
- Success: green (#00dd34), Failure: red (#e71b1b)

## Three States

### SelectDir (initial)
- All directory inputs enabled
- btnLeftButton = "Exit", btnRightButton = "Install"
- Progress area visible but empty ("Waiting for Input...")
- Pause/Retry/Run buttons hidden

### Installing
- All inputs disabled (grayed out cursors)
- btnLeftButton = "Cancel", btnRightButton hidden
- btnPause visible in right button position
- Progress bar animates (fake: 0-100% over ~5 seconds)
- Log area shows simulated extraction messages
- Cancel shows confirmation dialog

### Finished
- btnLeftButton = "Retry" or "Run", btnRightButton = "Finish"
- lblInstallResult shown: "Successfully Installed" (green) or "Installation Failed" (red)
- Progress bar at 100% (success) or stopped (failure)

## Audio

- Qt6 Multimedia: `QMediaPlayer` + `QAudioOutput`
- Loads `Music1.ogg` from assets
- Play/Pause buttons: 11x11 BMP images with 3 states (normal, hover, pressed)
- Volume slider: custom widget using TrackBkg.bmp (60x3) and TrackBtn.bmp (8x9) images
- Starts playing on app launch, loops continuously
- Draggable volume knob controls volume

## Interactive Elements

- **Drive combo**: populated from `QStorageInfo::mountedVolumes()`, filtered to real filesystems
- **Dir edit**: typing updates free space label; color turns red (#e71b1b) if insufficient space
- **Browse buttons**: open `QFileDialog::getExistingDirectory()`
- **Checkboxes**: all toggle normally
- **"Create Start Menu folder"**: unchecking disables GroupEdit + GroupBrowse
- **Install button**: validates path (ASCII check, free space), transitions to Installing state
- **Pause button**: toggles progress animation pause/resume
- **Cancel during install**: "Cancel extraction?" confirmation dialog
- **Window drag**: clicking and dragging on logo area or any non-interactive area at top
- **Fade-in**: window opacity animates from 0 to 1 over 300ms on launch

## Color Overrides

The QSS from the VSF parser provides base theming. These hardcoded colors from setup.iss are applied on top:

- Label text: #e1e0e6 (from `$E6E0E1`)
- Label hover: #ea7d2d (from `$2D7DEA`)
- Error/fail: #e71b1b (from `$1B1BE7`)
- Success: #00dd34 (from `$34DD00`)
- All font: Arial Bold 9pt for labels

## Build System

CMake project:
```cmake
find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)
```

## File Layout

```
app/
  CMakeLists.txt
  main.cpp                    # entry point, load QSS, create window
  InstallerWindow.h/.cpp      # main window class with all widgets
  AudioPlayer.h/.cpp          # QMediaPlayer wrapper with play/pause/volume
  VolumeSlider.h/.cpp         # custom widget for bitmap-based volume track
  assets/                     # copied from installer/src/Include/
    Logo5.bmp
    Play1.bmp, Play2.bmp, Play3.bmp
    Pause1.bmp, Pause2.bmp, Pause3.bmp
    TrackBkg.bmp, TrackBtn1.bmp, TrackBtn2.bmp, TrackBtn3.bmp
    Music1.ogg
```

## Dependencies

- Qt6 Widgets (qt6-base, already installed)
- Qt6 Multimedia (qt6-multimedia, already installed)
- GCC 15, CMake 4.2 (already installed)
- Generated QSS from VSF parser (at `/tmp/vsf_all_themes/CODEX/style.qss`)
