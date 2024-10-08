# CODEX Installer + build tools
![image](https://github.com/user-attachments/assets/0f3c3cdb-be2e-4a8c-8d15-8fafa4431cb0)
This repository contains the code for the CODEX installer and the tools required to build it

## Directory Structure

- **tooling/**: Contains necessary tools and dependencies.
  - `InnoSetupEE/`: Enhanced edition of Inno Setup with additional features.
  - `UltraARC/`: Tool used for compressing files to bin files.
  - `InnoUnp/`: Tool that lets you unpack other InnoSetup installers.

- **installer/**: Inno Setup scripts for creating custom installers.
  - `src/`: Contains the source code for the installer.

## Getting Started

1. **Using Inno Setup Enhanced Edition**:
   - Navigate to `tooling/InnoSetupEE/`.
   - Run `Compil32.exe` to open the enhanced edition of Inno Setup.
   - Go to `File -> Open` and navigate `installer/src`.
   - Open `setup.iss` and now you can modify the installer.

2. **Testing and Compiling**:
   - To test your changes you can press `F9` or click the Run button.
   - To compile the installer, you can press `CTRL+F9` or press the Compile button.
   - If you want to modify one of the styles in `src/installer/Include/Style`, you need RAD Studio.
   
3. **Adding the Game Files**:
   - In the code, change the defines like Game, GameExe, NeedSize and so on.
   - Compile the Setup, it will show up in `DISTRIBUTABLE`.
   - Add additional crack files to `DISTRIBUTABLE/CODEX` if needed.
   - Otherwise delete it or make it empty.
   - Navigate to `tooling/UltraARC/` and run `UltraARC.exe`.
   - Select the game folder as Source folder.
   - Select `DISTRIBUTABLE` as First Volume Content.
   - Choose some Output folder where the finilized bundle will go.
   - Switch to the `Options` tab and under `Methods`, check `LZMA`.
   - Switch to the `Output` tab and change Archive Prefix to `setup-` and Extension to `bin`.
   - You can play with some options here and there to achieve better compression.
   - If you select CD Image to bundle into `.iso`, you can add an icon in `Options -> CD Image`.
   - The icon can be found in the root of the project.
  
4. **IMPORTANT**:
   - `UltraARC` may split one of the bins into two or more parts aka `setup-2a` and `setup-2b` for example.
   - If this happens, you have to fiddle with the `Thershold Percent` option under `Output` tab.
   - The installer doesn't know how to deal with parted archives.
   
6. **Additional Information**:
   - You can easily switch between different group graphics.
   - In `setup.iss` change `Style` to `PLAZA/RUNE`.
   - There are more styles to choose from in `src/installer/Include/Style`.
   - Change `GroupName` to the same.
   - Don't forget to change the crack direcotry name.
   - Set `LogoGroup` to 7 and `IconGroup` to 2 for PLAZA.
   - Set logo to 8 and icon to 3 for RUNE.
   - You can also change the music to PLAZA/RUNE by changin `MusicGroup`.

## Contributing

Feel free to contribute by submitting pull requests or reporting issues in the repository.

## Credits

All credits goes to the amazing guys from [FileForums](https://fileforums.com)
- Rinaldo: [Original Code](https://fileforums.com/showthread.php?t=97259)
- Jiva: [Fixed CODEX version used in this repo](https://fileforums.com/member.php?u=229855)
- Razor12911 [Fixed the Enhanced Edition and added several features](https://fileforums.com/showpost.php?p=437536)
