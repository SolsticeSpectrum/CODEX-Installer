# CODEX Installer + build tools

This repository contains the code for the CODEX installer and the tools required to build it

## Directory Structure

- **tooling/**: Contains necessary tools and dependencies.
  - `InnoSetupEE/`: Enhanced edition of Inno Setup with additional features.

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

## Contributing

Feel free to contribute by submitting pull requests or reporting issues in the repository.

## Credits

All credits goes to the amazing guys from (fileforums.com)
