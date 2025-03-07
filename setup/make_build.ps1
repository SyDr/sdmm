$MSBuild = &"${Env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe
$InnoSetup = "C:\Program Files (x86)\Inno Setup 6\iscc.exe"
$StaticDirBuild = ".\Portable\SD Mod Manager"

&"$MSBuild" /noconsolelogger /fileLogger /flp:logfile=log-release.txt /property:Configuration=release "..\\SD MM.sln"
&"$MSBuild" /noconsolelogger /fileLogger /flp:logfile=log-release-static.txt /property:Configuration=release-static "..\\SD MM.sln"
&"$InnoSetup" setup.iss

$MMVersion = (Get-Item -Path '..\Release\Mod Manager.exe').VersionInfo.ProductVersion
Remove-Item -Path "Output\sdmm_setup_v$MMVersion.exe" -ErrorAction Ignore
Rename-Item -Path "Output\mm_setup.exe" -NewName "sdmm_setup_v$MMVersion.exe"
Remove-Item -Path "$StaticDirBuild" -Recurse -ErrorAction Ignore
Remove-Item -Path "Output\sdmm_v$MMVersion.zip" -ErrorAction Ignore

New-Item -ItemType Directory -Path "$StaticDirBuild" | Out-Null
New-Item -ItemType File -Path "$StaticDirBuild\base_dir.txt" -Value "../../" | Out-Null
Copy-Item -Path "..\release-static\Mod Manager.exe" -Destination "$StaticDirBuild" -Recurse
Copy-Item -Path "..\vcpkg_installed\x86-windows\x86-windows\bin\WebView2Loader.dll" -Destination "$StaticDirBuild" -Recurse
Copy-Item -Path "..\LICENSE.txt" -Destination "$StaticDirBuild\" -Recurse -Container
Copy-Item -Path "..\THIRD_PARTY.txt" -Destination "$StaticDirBuild\" -Recurse -Container
Copy-Item -Path "..\lng\" -Destination "$StaticDirBuild\" -Recurse -Container
Copy-Item -Path "..\icons\" -Filter "*.svg" -Destination "$StaticDirBuild\" -Recurse -Container
Copy-Item -Path "..\data\" -Destination "$StaticDirBuild\" -Recurse -Container
Compress-Archive -Path $StaticDirBuild -DestinationPath "Output\sdmm_v$MMVersion.zip"
