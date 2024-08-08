$MSBuild = &"${Env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe
$InnoSetup = "C:\Program Files (x86)\Inno Setup 6\iscc.exe"
$StaticDirBuild = ".\Portable\SD Mod Manager"

&"$MSBuild" /noconsolelogger /fileLogger /flp:logfile=log-release.txt /property:Configuration=release "..\\SD MM.sln"
&"$MSBuild" /noconsolelogger /fileLogger /flp:logfile=log-release-static.txt /property:Configuration=release-static "..\\SD MM.sln"
&"$InnoSetup" setup.iss

$MMVersion = (Get-Item -Path '..\Release\main.exe').VersionInfo.ProductVersion
Remove-Item -Path "Output\sdmm_setup_v_$MMVersion.exe" -ErrorAction Ignore
Rename-Item -Path "Output\mm_setup.exe" -NewName "sdmm_setup_v_$MMVersion.exe"
Remove-Item -Path "$StaticDirBuild" -Recurse -ErrorAction Ignore
Remove-Item -Path "Output\sdmm_v_$MMVersion.zip" -ErrorAction Ignore

New-Item -ItemType Directory -Path "$StaticDirBuild" | Out-Null
New-Item -ItemType File -Path "$StaticDirBuild\settings.json" | Out-Null
Copy-Item -Path "..\release-static\main.exe" -Destination "$StaticDirBuild" -Recurse
Copy-Item -Path "..\src\vcpkg_installed\x86-windows\x86-windows\bin\WebView2Loader.dll" -Destination "$StaticDirBuild" -Recurse
Copy-Item -Path "..\LICENSE" -Destination "$StaticDirBuild\" -Recurse -Container
Copy-Item -Path "..\lng\" -Destination "$StaticDirBuild\" -Recurse -Container
Copy-Item -Path "..\icons\" -Filter "*.png" -Destination "$StaticDirBuild\" -Recurse -Container
Copy-Item -Path "..\data\" -Destination "$StaticDirBuild\" -Recurse -Container
Compress-Archive -Path $StaticDirBuild -DestinationPath "Output\sdmm_v_$MMVersion.zip"
