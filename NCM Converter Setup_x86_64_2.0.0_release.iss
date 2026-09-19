; 脚本由 Inno Setup 脚本向导生成。
; 有关创建 Inno Setup 脚本文件的详细信息，请参阅帮助文档！
; 仅供非商业使用

#define MyAppName "NCM Converter"
#define MyAppVersion "2.0.0 - Release"
#define MyAppPublisher "ZHB3306"
#define MyAppURL "https://zhb1323306.qzz.io"
#define MyAppExeName "NCM Converter.exe"

[Setup]
; 注意：AppId 的值唯一标识此应用程序。不要在其他应用程序的安装程序中使用相同的 AppId 值。
; (若要生成新的 GUID，请在 IDE 中单击 "工具|生成 GUID"。)
AppId={{06DDE27C-499B-4549-8F06-41D012C6F61D}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
UninstallDisplayIcon={app}\{#MyAppExeName}
; "ArchitecturesAllowed=x64compatible" 指定
; 安装程序只能在 x64 和 Windows 11 on Arm 上运行。

ArchitecturesAllowed=x64compatible
; "ArchitecturesInstallIn64BitMode=x64compatible" 要求
; 在 X64 或 Windows 11 on Arm 上以 "64-位模式" 进行安装，
; 这意味着它应该使用本地 64 位 Program Files 目录
; 和注册表的 64 位视图。
ArchitecturesInstallIn64BitMode=x64compatible
ChangesAssociations=yes
DisableProgramGroupPage=yes
LicenseFile=C:\Users\zhb1323306\Desktop\others\ncmdump-GUI(without go)DEV2\LICENSE
;SetupIconFile=C:\Users\zhb1323306\Desktop\others\ncmdump-GUI(without go)DEV2\icons\NCMC_setup.ico
; 取消注释以下行以在非管理安装模式下运行 (仅为当前用户安装)。
;PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=commandline
OutputDir=C:\Users\zhb1323306\Desktop
OutputBaseFilename=NCM_Converter_2.0.0_release_x86_64
SolidCompression=yes
WizardStyle=modern dynamic windows11
DisableWelcomePage=no

[Languages]
Name: "chinesesimp"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}";
Name: "associate"; Description: "关联 .ncm 文件"; GroupDescription: "其他任务:"; Flags: checkedonce

[Files]
Source: "C:\Users\zhb1323306\Desktop\NCM Converter 2.0.0\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
; 注意：不要在任何共享系统文件上使用 "Flags: ignoreversion"

[Registry]
; .ncm 文件关联（可选任务）
Root: HKA; Subkey: "Software\Classes\.ncm"; ValueType: string; ValueName: ""; ValueData: "NCMConverter.ncm"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\NCMConverter.ncm"; ValueType: string; ValueName: ""; ValueData: "NCM 加密音乐文件"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\NCMConverter.ncm\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\icons\NCM_File.ico"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\NCMConverter.ncm\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
