; Windows installer for Artilody Plugweaver (Inno Setup 6).
; Build: ISCC /DAppVersion=0.3.0 /DSourceExe=<path to exe> /DOutputDir=<dir> Plugweaver.iss
#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef SourceExe
  #define SourceExe "..\..\build-windows\Plugweaver_artefacts\Release\Artilody Plugweaver.exe"
#endif
#ifndef OutputDir
  #define OutputDir "."
#endif

[Setup]
AppId={{8C1F3B52-6E0A-4C55-9D7B-5A2E81F4C3A7}
AppName=Artilody Plugweaver
AppVersion={#AppVersion}
AppVerName=Artilody Plugweaver {#AppVersion}
AppPublisher=Artilody
AppPublisherURL=https://github.com/shenphenchoedron-hue/Artilody-Plugweaver
DefaultDirName={autopf}\Artilody Plugweaver
DefaultGroupName=Artilody Plugweaver
DisableProgramGroupPage=yes
; Installs for the current user without administrator rights by default;
; the user can choose an all-users installation instead.
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir={#OutputDir}
OutputBaseFilename=Artilody-Plugweaver-{#AppVersion}-Windows-Setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\Artilody Plugweaver.exe
UninstallDisplayName=Artilody Plugweaver
LicenseFile=..\..\LICENSE

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "danish"; MessagesFile: "compiler:Languages\Danish.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#SourceExe}"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\LICENSE"; DestDir: "{app}"; DestName: "LICENSE.txt"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\Artilody Plugweaver"; Filename: "{app}\Artilody Plugweaver.exe"
Name: "{autodesktop}\Artilody Plugweaver"; Filename: "{app}\Artilody Plugweaver.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\Artilody Plugweaver.exe"; Description: "{cm:LaunchProgram,Artilody Plugweaver}"; Flags: nowait postinstall skipifsilent
