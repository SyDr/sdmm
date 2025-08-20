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

TODO (In no particular order)
-----------------------------
- Allow sent disabled mods into archive on loading preset.
- Use wxWidgets from trunk to test dark theme support.
- Check why WebView2Loader.dll from VCPKG location not copied automatically.
- Write memory dump on crash (and include debug symbols?).
- Use UPX or something else to reduce release binary size.
- Update icons.
- Use FromDIP instead of fixed sizes.
- Use cmark-gfm instead of cmark (tables).
- Implement mod installtion from archives.
- Add settings dialog and move/copy all settings here.
- Implement auto update.
- Implement mod editor (or not?).
- WebPreview check width.
- Navigate in mod list via keys?
- On first launch, ask some important settings.
- Would be nice to have at least the possibiliy to see what plugins are tied to a mod - for users helpful when its about bugfixing/testing
