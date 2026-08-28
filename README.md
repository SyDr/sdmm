SD Mod Manager
==============

`SD Mod Manager` is a mod manager for [ERA](http://wforum.heroes35.net/showthread.php?tid=5830) mod platform.  
TODO: write more information.

Installing
----------
Download and install from [releases](./releases) page.

Development
-----------
Install Visual Studio (2022) with C++ support.
Install Git for Windows (`winget install --id Git.Git -e --source winget`)
Install InnoSetup if you want to make builds (`winget install -e --id JRSoftware.InnoSetup')

`git clone https://github.com/microsoft/vcpkg/`  
`setx VCPKG_ROOT <vcpkg_root>`
`bootstrap-vcpkg.bat`
`vcpkg.exe integrate install`

Open SD MM.sln via Visual Studio. Press Start Debugging (F5).

Feature regression tests can be built from `tests/feature_tests.vcxproj` (`Release|Win32`). Run
`tests/bin/feature_tests.exe <repository-root>` to exercise plugin renames, SFX package generation,
archive inspection, transactional installation, rollback, and recovery against temporary fixtures.

Mod packages
------------
Use **Mod → Add new mod(s)** or drag package files onto the main window to review and install one or
more `.exe`, `.rar`, `.zip`, or `.7z` packages. Packages are inspected with the bundled 7-Zip and are
never executed. For safety, every archive entry must be inside a `Mods/<mod directory>` root. Selected
mods are installed as one transaction; existing enabled states and priorities are restored if any part
of the batch fails.

TODO (In no particular order)
-----------------------------
- Allow sent disabled mods into archive on loading preset.
- Use wxWidgets from trunk to test dark theme support.
- Check why WebView2Loader.dll from VCPKG location not copied automatically.
- Write memory dump on crash (and include debug symbols?).
- Use FromDIP instead of fixed sizes.
- Use cmark-gfm instead of cmark (tables).
- Add settings dialog and move/copy all settings here.
- Implement auto update.
- Implement mod editor (or not?).
- WebPreview check width.
- Navigate in mod list via keys?
- On first launch, ask some important settings.
- Add wiki with usage information (or video?)
