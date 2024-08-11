SD Mod Manager
==============

`SD Mod Manager` is a mod manager for [ERA](http://wforum.heroes35.net/showthread.php?tid=5830) mod platform.  
TODO: write here more information.

Installing
----------
Download and install from [releases](./releases) page.
- TODO: get WebView2Loader.dll from VCPKG location (and check, why it's not copied automatically).

Development
-----------
Instal Visual Studio (2022) with C++ support.

`git clone https://github.com/microsoft/vcpkg/`  
`setx VCPKG_ROOT=<vcpkg_root>`

Open SD MM.sln via Visual Studio. Press Start Debugging (F5).

TODO
----
- For portable build maybe allow to use just parent dir instead of 2 inderections (like <root>/MM instead of <root>/Tools/MM).
- Improve screenshots display (use separate window?).
- Use UPX or something else to reduce release binary size.
- Improve IconStorage to allow different icon size (currently only 16x16 is used).
- Update icons.
- Use FromDIP instead of fixed sizes.
- Use cmark-gfm instead of cmark (tables).
- Use IE7 mode for preview, if Edge is not available (or ignore?).
- Implement mod installtion from archives.
- Add settings dialog and move/copy all settings here.
- Implement update checking.
- Implement auto update.
- Implement mod editor (or not?).
