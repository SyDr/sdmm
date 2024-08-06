$msbuild = &"${Env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe
$innosetup = "C:\Program Files (x86)\Inno Setup 6\iscc.exe"
&"$msbuild" /noconsolelogger /fileLogger /flp:logfile=log-release.txt /property:Configuration=release "..\\SD MM.sln"
$mmversion = (Get-Item -Path '..\Release\main.exe').VersionInfo.ProductVersion
&"$msbuild" /noconsolelogger /fileLogger /flp:logfile=log-release-static.txt /property:Configuration=release-static "..\\SD MM.sln"
&"$innosetup" setup.iss
Rename-Item -Path "Output\mm_setup.exe" -NewName "sdmm_setup_v_$mmversion.exe"