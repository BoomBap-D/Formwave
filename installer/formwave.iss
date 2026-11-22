; Inno Setup script: builds Formwave-Setup.exe from the Windows x64 build output in ..\dist
; Install Inno Setup 6, open this file, press Compile.
[Setup]
AppName=Formwave
AppVersion=0.4.0
AppPublisher=Formwave
DefaultDirName={commonpf}\Formwave
DefaultGroupName=Formwave
OutputBaseFilename=Formwave-Setup
Compression=lzma2
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
WizardStyle=modern
DisableProgramGroupPage=yes
UninstallDisplayName=Formwave

[Types]
Name: "full"; Description: "Standalone app and VST3 plug-in"
Name: "vst"; Description: "VST3 plug-in only"
Name: "custom"; Description: "Custom"; Flags: iscustom

[Components]
Name: "standalone"; Description: "Standalone application"; Types: full custom
Name: "vst3"; Description: "VST3 plug-in"; Types: full vst custom

[Files]
Source: "..\dist\Standalone\Formwave.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
Source: "..\dist\VST3\Formwave.vst3\*"; DestDir: "{commoncf64}\VST3\Formwave.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Formwave"; Filename: "{app}\Formwave.exe"; Components: standalone

[Run]
Filename: "{app}\Formwave.exe"; Description: "Start Formwave"; Flags: nowait postinstall skipifsilent; Components: standalone
