SD Mod Manager
==============

`SD Mod Manager` is a mod manager for [ERA](http://wforum.heroes35.net/showthread.php?tid=5830) mod platform.  
TODO: write here more information.

Installing
----------
Download and install from [releases](./releases) page.

Development
-----------
Instal Visual Studio (2022) with C++ support.

`git clone https://github.com/microsoft/vcpkg/`  
`setx VCPKG_ROOT=<vcpkg_root>`

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
