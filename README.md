# CODEX Installer + build tools

This repository contains the code for the CODEX installer and the tools required to build it

## Directory Structure

- **tooling/**: Contains necessary tools and dependencies.
  - `InnoSetupEE/`: Enhanced edition of Inno Setup with additional features.
  - `UltraARC/`: Tool used for compressing files to bin files.

- **installer/**: Inno Setup scripts for creating custom installers.
  - `src/`: Contains the source code for the installer.

## Getting Started

1. **Using Inno Setup Enhanced Edition**:
   - Navigate to `tooling/InnoSetupEE/`.
   - Run `Compil32.exe` to open the enhanced edition of Inno Setup.
   - Go to `File -> Open` and navigate `installer/src`.
   - Open `codex.iss` and now you can modify the installer.

2. **Testing and Compiling**:
   - Open the Inno Setup Compiler from the Start menu or desktop shortcut.
   - Load and compile `.iss` scripts from `installer_scripts/`.

3. **Additional Tools and Dependencies**:
   - To test your changes you can press `F9` or click the Run button.
   - To compile the installer, you can press `CTRL+F9` or press the Compile button.
   
4. **Adding the Game Files**:
   - In the code, change the defines like Game, GameExe, NeedSize and so on.
   - Compile the Setup, it will show up in `DISTRIBUTABLE`.
   - Navigate to `tooling/UltraARC/` and run `UltraARC.exe`.
   - Select the game folder as Source folder.
   - Select `DISTRIBUTABLE` as First Volume Content.
   - Choose some Output folder where the finilized bundle will go.
   - Switch to the `Output` tab and change Archive Prefix to `setup-` and Extension to `bin`.
   - You can play with some options here and there to achieve better compression.

## Contributing

Feel free to contribute by submitting pull requests or reporting issues in the repository.

## Credits

All credits goes to the amazing guys from [FileForums](https://fileforums.com)
- Rinaldo: [Original Code](https://fileforums.com/showthread.php?t=97259)
- Jiva: [Fixed CODEX version used in this repo](https://fileforums.com/member.php?u=229855)
- Razor12911 [Fixed the Enhanced Edition and added several features](https://fileforums.com/showpost.php?p=437536)
