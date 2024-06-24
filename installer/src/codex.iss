; CODEX installer

#define Style           "CODEX"                              ; You can find more in Include\Style (PLAZA/RUNE + more).
#define GroupName       "CODEX"                              ; Name of the group (rename crack directory).
#define LogoGroup       "5"                                  ; set header logo 1 .. 8 (more in Include/GFX/Logos)
#define IconGroup       "1"                                  ; set app icon 1 .. 3 (more in Include/GFX/Icons)
#define MusicGroup      "1"                                  ; set app icon 1 .. 3 (more in Include/Music)
#define Game            "Example Game"                       ; Name of release.
#define GameExe         "example.exe"                        ; Game executable
#define Game_NeedSize   "4616"                               ; Needed space for release.
#define MyAppURL        "http://store.steampowered.com/app/480"
#define AppVersion      "1.0.0.0"
#define AppPublisher    "Valve"
#define Game_CrackDir   "{src}\" + GroupName                 ; Game crack directory (if present).
#define SetupFiles      "Include"                            ; Files needed for the installation.
#define Uninstallexe    "unins000.exe

[Setup]
AppName={#Game}
AppVerName={#Game}
AppPublisher={#AppPublisher}
AppSupportURL={#MyAppURL}
AppVersion={#AppVersion}
DefaultDirName={code:DefDirWiz}
DefaultGroupName={#Game}
ArchitecturesInstallIn64BitMode=x64
OutputDir=.\DISTRIBUTABLE
OutputBaseFilename=setup
Uninstallable=Unnin
UninstallDisplayIcon={app}\{#GameExe}
AllowNoIcons=yes
Compression=lzma
SolidCompression=true
SetupIconFile=Include\GFX\Icons\Icon{#IconGroup}.ico
WizardImageFile=Include\GFX\Logos\Logo{#LogoGroup}.bmp
IconResource=1:Include\GFX\Icons\1.ico|2:Include\GFX\Icons\2.ico|3:Include\GFX\Icons\3.ico|4:Include\GFX\Icons\4.ico|5:Include\GFX\Icons\5.ico|6:Include\GFX\Icons\6.ico|7:Include\GFX\Icons\7.ico

[Run]
;Filename: {app}\{#GameExe}; Description: {cm:LaunchProgram,{#StringChange(GameExe, '&', '&&')}}; Flags: nowait postinstall skipifsilent unchecked

[Files]
Source:{#SetupFiles}\DLL\*; DestDir: {tmp}; Flags: dontcopy
Source:{#SetupFiles}\GFX\Buttons\*; DestDir: {tmp}; Flags: dontcopy
Source:{#SetupFiles}\Language\*; DestDir: {tmp}; Flags: dontcopy
Source:{#SetupFiles}\Music\Music{#MusicGroup}.ogg; DestDir: {tmp}; Flags: dontcopy
Source:{#SetupFiles}\Style\{#Style}.vsf; DestDir: {tmp}; Flags: dontcopy
Source:Include\DLL\WinTB.dll; Flags: dontcopy;
#ifndef IS_ENHANCED
Source:CallbackCtrl.dll; Flags: dontcopy;
#endif
;Source:Include\GFX\Icons\games.ico; Flags: dontcopy;

[Registry]
Root: HKCU; Subkey: "Software\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers"; ValueName: "{app}\SETSUNA.exe"; ValueType: String; ValueData: "RUNASADMIN"; Check: "CheckError"; Flags: uninsdeletevalue uninsdeletekeyifempty

[Icons]
Name: "{userdesktop}\{#Game}"; Filename: "{app}\{#GameExe}"; Check: Icon and CheckError;
Name: "{group}\{#Game}"; Filename: "{app}\{#GameExe}"; Check: Start and CheckError;
Name: "{group}\{cm:UninstallProgram,}"; Filename: "{uninstallexe}"; Check: Unnin and Start and CheckError;
Name: "{group}\{cm:ProgramOnTheWeb,I am Setsuna}"; Filename: "{#MyAppURL}";

[Languages]
Name: "English"; MessagesFile: "compiler:Default.isl"

[CustomMessages]
English.NameAndVersion=%1 version %2
English.AdditionalIcons=Additional icons:
English.CreateDesktopIcon=Create a &desktop icon
English.CreateQuickLaunchIcon=Create a &Quick Launch icon
English.ProgramOnTheWeb=%1 on the Web
English.UninstallProgram=Uninstall %1
English.LaunchProgram=Launch %1
English.AssocFileExtension=&Associate %1 with the %2 file extension
English.AssocingFileExtension=Associating %1 with the %2 file extension...
English.AutoStartProgramGroupDescription=Startup:
English.AutoStartProgram=Automatically start %1
ErrDir=Install path contains bad characters!
ErrBro=They are bad characters at Start Menu directory!
DirInstall=Install directory
IconDest=&Create desktop shortcut
IconGroup=Directory at Start Menu
CreateIconGroup=&Create a Start Menu folder
NoUninstall=Do not create uninstaller and do not write any specific system info
ExitBtn=&Exit
msgIsAdmin=The setup process has been started without administrator rights. Do you want to procceed ?
CopyCrack=Copy contents of {#GroupName} directory to installdir
ExDir=Directory to copy is wrong or empty!
ErrCopy=Auto copying of {#GroupName} folder is blocked by a system or AV! You need to copy {#GroupName} folder yourself.
FreeSpace1=At least
FreeSpace2=of free space required
ErrSize=There is not enough of free space on selected disk!
MemoReady=Waiting for Input...
InteProc=Cancel extraction?
Success=Successfully Installed
Fail=Installation Failed

[UninstallDelete]
Type: filesandordirs; Name: {app}

[Code]
//#include "WinTB.iss"

Type
 #IfDef UNICODE
  #Define A "W"
 #Else
  #Define A "A";
 #EndIf
 #Define isFalse(any S) (S = LowerCase(Str(S))) == "no" || S == "false" || S == "off" ? "true" : "false"

 TMargins = record
    cxLeftWidth: Integer;
    cxRightWidth: Integer;
    cyTopHeight: Integer;
    cyBottomHeight: Integer;
  end;

 Type
  TTimerProc = Procedure(HandleW, Msg, idEvent, TimeSys: LongWord); 
  TCallbackUnk = Function(OveralPct, CurrentPct: Integer; CurrentFile, TimeStr1, TimeStr2, TimeStr3: PAnsiChar): LongWord;
  TCallback = Function(OveralPct, CurrentPct: Integer; CurrentFile, TimeStr1, TimeStr2, TimeStr3: PAnsiChar): LongWord;
  TCallbackProc = Function(h:HWND; Msg, wParam, lParam: LongInt): LongInt;

var
  gv00: TStartMenuFolderTreeView;  // StartMenuTreeView - FFolderTreeView - DirFolderTreeView
  FolderTreeView: TFolderTreeView; // GlobalVar[01]
  gv02: TNewEdit;
  DirEdit: TNewEdit;               // GlobalVar[03]
  gv04: TLabel;
  gv05: TLabel;
  gv06: TLabel;                    // lblDesktopIcon
  gv07: TLabel;
  gv08: TLabel;
  gv09: TLabel;
  gv10: TLabel;
  lblInstallResult: TLabel;
  gv12: TBevel;                    // bvDirectories
  gv13: TBevel;                    // bvInstallDir
  gv14: TBevel;                    // bvStartMenuDir
  gv15: TBevel;
  gv16: TBevel;
  gv17: TBevel;
  gv18: TBevel;
  gv19: TBevel;
  gv20: TBevel;
  gv21: TBevel;
  gv22: TCheckBox;                 // chbDesktopIcon
  gv23: TCheckBox;
  gv24: TCheckBox;
  gv25: TCheckBox;
  gv26: TNewComboBox;
  gv27: AnsiString;
  gv28: AnsiString;
  gv29: Cardinal;
  gv30: Cardinal;
  hMutex: DWord;
  memInstallLog: TNewMemo;
  gv33: LongInt;
  gv34: LongInt;
  gv35: LongInt;
  gv36: LongInt;
  gv37: HWND;
  gv38: HWND;
  ISDoneError : Boolean;           // 39
  gv40 : Boolean;
  gv41: Boolean;
  gv42: Boolean;
  gv43: Boolean;
  gv44: Boolean;
  gv45: Double;
  gv46: TButton;                   // btnPause
  gv47: TButton;                   // btnRetry
  gv48: TButton;                   // btnRun
  gv49: TButton;
  gv50: TButton;
  gv51: TButton;
  gv52: TButton;
  gv53: TBitmapImage;
  gv54: TBitmapImage;
  gv55: TBitmapImage;
  gv56: TBitmapImage;
  gv57: TMemoryStream;
  gv58: DWord;                     // mp3HNDL
  gv59: HWND;                      // oldWFproc
  gv60: LongInt;
  gv61: TPoint;
  gv62: Single;
  gv63: Extended;
  gv64: DWord;
  gv65: TMargins;
  gv66: Array[1..7] of TNewIcon;
  gv67: TSetupForm;                // dirBrowseForm - BrowseForm
  gv68: TSetupForm;

const
  GWL_EXSTYLE = -20;
  DRIVE_FIXED = 3; 
  IMAGE_ICON = 1;
  SPI_GETDRAGFULLWINDOWS = $0026;
  WS_EX_LAYERED = $80000;

const
  // Constants
  TBPF_NOPROGRESS         = 0;
  TBPF_INDETERMINATE      = 1;
  TBPF_NORMAL             = 2;
  TBPF_ERROR              = 4;
  TBPF_PAUSED             = 8;
  // AnimateWindow (dwFlags)
  AW_ACTIVATE = $00020000;  
  AW_BLEND = $00080000;  
  AW_CENTER = $00000010;  
  AW_HIDE = $00010000;  
  AW_HOR_POSITIVE = $00000001;  
  AW_HOR_NEGATIVE = $00000002;  
  AW_SLIDE = $00040000;  
  AW_VER_POSITIVE = $00000004;  
  AW_VER_NEGATIVE = $00000008;                      
  AW_FADE_IN = $00080000;
  AW_FADE_OUT = $00090000;
  AW_SLIDE_IN_LEFT = $00040001;
  AW_SLIDE_OUT_LEFT = $00050002;
  AW_SLIDE_IN_RIGHT = $00040002;
  AW_SLIDE_OUT_RIGHT = $00050001;
  AW_SLIDE_IN_TOP = $00040004;
  AW_SLIDE_OUT_TOP = $00050008;
  AW_SLIDE_IN_BOTTOM = $00040008;
  AW_SLIDE_OUT_BOTTOM = $00050004;
  AW_DIAG_SLIDE_IN_TOPLEFT = $00040005;
  AW_DIAG_SLIDE_OUT_TOPLEFT = $0005000A;
  AW_DIAG_SLIDE_IN_TOPRIGHT = $00040006;
  AW_DIAG_SLIDE_OUT_TOPRIGHT = $00050009;
  AW_DIAG_SLIDE_IN_BOTTOMLEFT = $00040009;
  AW_DIAG_SLIDE_OUT_BOTTOMLEFT = $00050006;
  AW_DIAG_SLIDE_IN_BOTTOMRIGHT = $0004000A;
  AW_DIAG_SLIDE_OUT_BOTTOMRIGHT = $00050005;
  AW_EXPLODE = $00040010;
  AW_IMPLODE = $00050010;

 // 01
 Procedure LoadFromStreamVCLStyle(VCLStyle: String; misc: String);
 External 'LoadFromStreamVCLStyle{#A}@files:VclStylesInno.dll stdcall';
 // 02
 Procedure UnLoadVCLStyles;
 External 'UnLoadVCLStyles@files:VclStylesInno.dll stdcall';
 // 03
 Function  IsOwner(v1: String; v2: String): Boolean;
 External 'IsOwner@files:VclStylesInno.dll stdcall';
 // 03
 Function  IsXCOPY(v1: String; v2: String; v3: String): Boolean;
 External 'IsXCOPY@files:VclStylesInno.dll stdcall';
 // 05
 Function  isAdmin: Boolean;
 External 'isAdmin@files:VclStylesInno.dll stdcall';
 // 06
 Procedure MyRetryApp(app: String);
 External 'MyRetryApp@files:VclStylesInno.dll stdcall';
 // 07
 Function  GetLogicalDrives: DWord;
 External 'GetLogicalDrives@kernel32.dll stdcall'; 
 // 08
 Function  GetDriveType(lpRootPathName: AnsiString): UInt;
 External 'GetDriveTypeA@kernel32.dll stdcall'; 
 // 09
 Function  DeleteFile(FmemName: PAnsiChar): Boolean;
 External 'DeleteFileA@kernel32.dll stdcall';
 // 10
 Function  AnimateWindow(hWnd: HWND; dwTime: DWord; dwFlags: DWord): Boolean;
 External 'AnimateWindow@user32 stdcall';
 // (ISDone.iss)
 Function  ISArcExtract(CurComponent: Cardinal; PctOfTotal: Double; InName, OutPath, ExtractedPath: AnsiString; DeleteInFile: Boolean; Password, CfgFile, WorkPath: AnsiString; ExtractPCF: Boolean ): Boolean;
 External 'ISArcExtract@files:ISDone.dll stdcall delayload';
 Function  IS7ZipExtract(CurComponent: Cardinal; PctOfTotal: Double; InName, OutPath: AnsiString; DeleteInFile: Boolean; Password: AnsiString): Boolean;
 External 'IS7zipExtract@files:ISDone.dll stdcall delayload';
 Function  ISRarExtract(CurComponent: Cardinal; PctOfTotal: Double; InName, OutPath: AnsiString; DeleteInFile: Boolean; Password: AnsiString): Boolean;
 External 'ISRarExtract@files:ISDone.dll stdcall delayload';
 Function  ISPrecompExtract(CurComponent: Cardinal; PctOfTotal: Double; InName, OutFile: AnsiString; DeleteInFile: Boolean): Boolean;
 External 'ISPrecompExtract@files:ISDone.dll stdcall delayload';
 Function  ISSRepExtract(CurComponent: Cardinal; PctOfTotal: Double; InName, OutFile: AnsiString; DeleteInFile: Boolean): Boolean;
 External 'ISSrepExtract@files:ISDone.dll stdcall delayload';
 Function  ISxDeltaExtract(CurComponent: Cardinal; PctOfTotal: Double; minRAM, maxRAM: Integer; InName, DiffFile, OutFile: AnsiString; DeleteInFile, DeleteDiffFile: Boolean): Boolean;
 External 'ISxDeltaExtract@files:ISDone.dll stdcall delayload';
 Function  ISPackZIP(CurComponent: Cardinal; PctOfTotal: Double; InName, OutFile: AnsiString; ComprLvl: Integer; DeleteInFile: Boolean): Boolean;
 External 'ISPackZIP@files:ISDone.dll stdcall delayload';
 Function  ShowChangeDiskWindow(Text, DefaultPath, SearchFile: AnsiString): Boolean;
 External 'ShowChangeDiskWindow@files:ISDone.dll stdcall delayload';
 Function  Exec2(FileName, Param: PAnsiChar; Show: Boolean): Boolean;
 External 'Exec2@files:ISDone.dll stdcall delayload';
 Function  ISFindFiles(CurComponent: Cardinal; FileMask: AnsiString; var ColFiles: Integer): Integer;
 External 'ISFindFiles@files:ISDone.dll stdcall delayload';
 Function  ISPickFilename(FindHandle: Integer; OutPath: AnsiString; var CurIndex: Integer; DeleteInFile: Boolean): Boolean;
 External 'ISPickFilename@files:ISDone.dll stdcall delayload';
 Function  ISGetName(TypeStr: Integer): PAnsiChar;
 External 'ISGetName@files:ISDone.dll stdcall delayload';
 Function  ISFindFree(FindHandle: Integer): Boolean;
 External 'ISFindFree@files:ISDone.dll stdcall delayload';
 Function  ISExec(CurComponent: Cardinal; PctOfTotal, SpecifiedProcessTime: Double; ExeName, Parameters, TargetDir, OutputStr: AnsiString; Show: Boolean): Boolean;
 External 'ISExec@files:ISDone.dll stdcall delayload';
 Function  SrepInit(TmpPath: PAnsiChar; VirtMem, MaxSave: Cardinal): Boolean;
 External 'SrepInit@files:ISDone.dll stdcall delayload';
 Function  PrecompInit(TmpPath: PAnsiChar; VirtMem: Cardinal; PrecompVers: Single): Boolean;
 External 'PrecompInit@files:ISDone.dll stdcall delayload';
 Function  FileSearchInit(RecursiveSubDir: Boolean): Boolean;
 External 'FileSearchInit@files:ISDone.dll stdcall delayload';
 function  ISDoneInit(RecordFileName: AnsiString; TimeType, Comp1, Comp2, Comp3: Cardinal; WinHandle, NeededMem: LongInt; callback: TCallback): Boolean;
 External 'ISDoneInit@files:ISDone.dll stdcall';
 Function  ISDoneStop: Boolean;
 External 'ISDoneStop@files:ISDone.dll stdcall';
 Function  ChangeLanguage(Language: AnsiString): Boolean;
 External 'ChangeLanguage@files:ISDone.dll stdcall delayload';
 Function  SuspendProc: Boolean;
 External 'SuspendProc@files:ISDone.dll stdcall';
 Function  ResumeProc: Boolean;
 External 'ResumeProc@files:ISDone.dll stdcall';
 // 33
 Function  BASS_Init(device: Integer; freq, flags, win: DWord; clsid: Integer): Boolean;
 External 'BASS_Init@files:BASS.dll stdcall';
 // 34
 Function  BASS_Free(): Boolean;
 External 'BASS_Free@files:BASS.dll stdcall';
 // 35
 Function  BASS_StreamCreateFileLib(mem: Bool; f: PAnsiChar; offset, length, flags: DWord): DWord;
 External 'BASS_StreamCreateFile@files:bp.dll stdcall';
 // 36
 function  BASS_Start: Boolean;
 External 'BASS_Start@files:BASS.dll stdcall';
 // 37
 Function  BASS_Stop: Boolean;
 External 'BASS_Stop@files:BASS.dll stdcall';
 // 38
 Function  BASS_Pause: Boolean;
 External 'BASS_Pause@files:BASS.dll stdcall';
 // 39
 Function  BASS_SetVolume(volume: Single): Boolean;
 External 'BASS_SetVolume@files:BASS.dll stdcall';
 // 40
 Function  BASS_ChannelPlay(handle: DWord; restart: Bool): Boolean;
 External 'BASS_ChannelPlay@files:BASS.dll stdcall';
 // 41
 Function  BASS_ChannelPause(handle: DWord): Boolean;
 External 'BASS_ChannelPause@files:BASS.dll stdcall';
 // 42
 Function  BASS_ChannelIsActive(handle: DWord): DWord;
 External 'BASS_ChannelIsActive@{tmp}\BASS.dll stdcall delayload';
 // 43
 Function  BASS_ChannelSetAttribute(handle: DWord; attrib: DWord; value: Single): Boolean;
 External 'BASS_ChannelSetAttribute@{tmp}\BASS.dll stdcall delayload';
 // 44
 Procedure SetTaskBarProgressValue(APP: HWND; Value: Integer);
 External 'SetTaskBarProgressValue@{tmp}\WinTB.dll stdcall delayload';
 // 45
 Procedure SetTaskBarProgressState(APP:HWND; Value: Integer);
 External 'SetTaskBarProgressState@{tmp}\WinTB.dll stdcall delayload';
 // 46
 Procedure SetTaskBarToolTip(APP: HWND; Hint: PAnsiChar);
 External 'SetTaskBarToolTip@{tmp}\WinTB.dll stdcall delayload';
 // 47
 Function  TaskBarAddButton(Icon: Cardinal; Hint: PAnsiChar; Event: Integer; Border: Boolean): Integer;
 External 'TaskBarAddButton@{tmp}\WinTB.dll stdcall delayload';
 // 48
 Procedure TaskBarUpdateButtons(APP: HWND);
 External 'TaskBarUpdateButtons@{tmp}\WinTB.dll stdcall delayload';
 // 49
 Procedure TaskBarButtonEnabled(Button: LongInt; Enabled: Boolean);
 External 'TaskBarButtonEnabled@{tmp}\WinTB.dll stdcall delayload'; // cdecl = 2, stdcall = 3  -  delayload = 1, [ ] = 0
 // 50
 Procedure TaskBarButtonToolTip(Button: LongInt; Hint: PAnsiChar);
 External 'TaskBarButtonToolTip@{tmp}\WinTB.dll stdcall delayload';
 // 51
 Procedure TaskBarButtonIcon(Button: LongInt; Icon: DWord);
 External 'TaskBarButtonIcon@{tmp}\WinTB.dll stdcall delayload';
 // 52
 Procedure TaskBarV10(mf, wf: HWND; isSkin, isAero: Boolean; Top, Frame: Integer; Const m: TMargins);
 External 'TaskBarV10@{tmp}\WinTB.dll stdcall delayload';
 // 53
 Function  WrapCallback(callback: TCallbackUnk; paramcount: Integer): LongWord;
 External 'wrapcallback@{tmp}\ISDone.dlll stdcall delayload';
 // 54
 Procedure TaskBarDestroy;
 External 'TaskBarDestroy@{tmp}\WinTB.dll stdcall delayload';
 // 55
 Function  ShowWindow(hWnd: HWND; uType: Integer): LongInt;
 External 'ShowWindow@user32.dll stdcall';
 // 56
 Function  ScreenToClient(hWnd: HWND; var lpPoint: TPoint): Bool;
 External 'ScreenToClient@user32.dll stdcall';
 // 57
 Function  GetCursorPos(var lpPoint: TPoint): Bool;
 External 'GetCursorPos@user32.dll stdcall';
 // 58
 Function  SetWindowLong(hWnd: HWND; nIndex: Integer; dwNewLong: LongInt): LongInt;
 External 'SetWindowLongA@user32.dll stdcall';
 // 59
 Function  WndProcCallBack(callback: TCallbackProc; paramcount: Integer): LongWord;
 External 'wrapcallback@files:ISDone.dll stdcall';
 // 60
 Function  CallWindowProc(lpPrevWndFunc: LongInt; hWnd: HWND; Msg: UInt; wParam, lParam: LongInt): LongInt;
 External 'CallWindowProcA@user32.dll stdcall';
 // 61
 Function  GetWindowLong(HWND: HWND; nIndex: Integer): LongInt;
 External 'GetWindowLongA@user32.dll stdcall';
 // 62
 Function  SetLayeredWindowAttributes(hwnd: HWND; crKey: TColor; bAlpha: Byte; dwFlags: DWord): Boolean;
 External 'SetLayeredWindowAttributes@user32.dll stdcall';
 // 63
 Function  SetTimer(hWnd, nIDEvent, uElapse, lpTimerFunc: LongWord): LongWord;
 External 'SetTimer@user32.dll stdcall';
 Function  KillTimer(hWnd, nIDEvent: LongWord): LongWord;
 External 'KillTimer@user32.dll stdcall';
 // 65
 Function  WrapTimerProc(callback: TTimerProc; ParamCount: Integer): LongWord;
 External 'wrapcallback@files:ISDone.dll stdcall delayload';
 // 66
 Function  SystemParametersInfo(uiAction: UInt; uiParam: UInt; var pvParam: DWord; fWinIni: UInt): Bool;
 External 'SystemParametersInfo{#A}@user32.dll stdcall';
 // 67
 Function  IsThemeActive: Bool;
 External 'IsThemeActive@UxTheme.dll stdcall delayload'; 
 // 68
 Function  CreateMutexA(lpMutexAttributes: DWord; bInitialOwner: LongInt; lpName: AnsiString): DWord;
 External 'CreateMutexA@kernel32.dll stdcall';
 // 69
 Function  CloseHandle(hObject: DWord): Boolean;
 External 'CloseHandle@kernel32.dll stdcall';
 // 70
 Function   PathIsDirectoryEmpty(pszPath: AnsiString): Boolean;
 External  'PathIsDirectoryEmptyA@shlwapi.dll stdcall';
 // 71
 Function ProgressCallback(OveralPct, CurrentPct: Integer; CurrentFile, TimeStr1, TimeStr2, TimeStr3: PAnsiChar): LongWord;
  var s: AnsiString;
 Begin
  If OveralPct <= 1000 Then
   Wizardform.ProgressGauge.Position := OveralPct;
  SetTaskBarProgressValue(0, Wizardform.ProgressGauge.Position / 10);
  s := CurrentFile;
  If memInstallLog.Lines.Strings[memInstallLog.Lines.Count - 1] <> s Then
   memInstallLog.Lines.Add(MinimizePathName(CurrentFile, memInstallLog.Font, memInstallLog.Width - ScaleX(50)));
  Result := gv33;
 End;

 // 84
 Function CheckError: Boolean;
 Begin
  Result := not ISDoneError;
 End;

 //85
 Function NoSD: String;
  var
   x, bit, i: Integer;
   tp: Cardinal;
   sd: string;
  Begin
   sd := ExpandConstant('{sd}');
   Result := ExpandConstant('{pf}\');
   x := GetLogicalDrives;
   If x <> 0 Then
    For i:= 1 To 64 Do
     Begin
      bit := x and 1;
      If bit = 1 Then
       Begin
        tp := GetDriveType(PAnsiChar(Chr(64 + i) + ':'));
        If tp = DRIVE_FIXED Then
         If Chr(64 + i) <> Copy(sd, 1, 1) Then
          Begin
           Result := Chr(64 + i) + ':\Games\';
           Break;
          End;
       End;
      x := x shr 1;
     End;
  End; 

 // 88
 Function AddDriveToList(cb: TNewComboBox): Boolean;
  var
   x, bit, i: Integer;
   tp: DWord;
  Begin
   x := GetLogicalDrives;
   If x <> 0 Then
    For i:= 1 To 64 Do
     Begin
      bit := x and 1;
      If bit = 1 Then
       Begin
        tp := GetDriveType(PAnsiChar(Chr(64 + i) + ':'));
        If tp = DRIVE_FIXED Then
         cb.Items.Add((Chr(64 + i) + ':\'));
       End;
      x := x shr 1;
     End;
   If cb.Items.IndexOf(AddBackslash(ExtractFileDrive(WizardForm.DirEdit.Text))) >= 0 Then
    cb.ItemIndex := cb.Items.IndexOf(AddBackslash(ExtractFileDrive(WizardForm.DirEdit.Text)));
   Result := True;
  End;                         

 // 96
 Function _IsWin8: Boolean;
 var
  Version: TWindowsVersion;
 Begin
  GetWindowsVersionEx(Version);
  If ((Version.Major = 6) and (Version.Minor > 1)) or (Version.Major > 6) Then
   Result := True
  Else
   Result := False
 End;
 
 // 98
 Function IsAnsi(S: String): Boolean;
  var
   S1, S2: String;
  Begin
   S1 := AnsiUppercase(S);
   S2 := Uppercase(S);
   If CompareStr(S1, S2) = 0 Then
    begin
     S1 := Lowercase(S);
     S2 := AnsiLowercase(S);
     If CompareStr(S1, S2) = 0 Then
      Result := True;
    end;
  End;

 // 104
 Function DefDirWiz(s: String): String;
  Begin
   If _IsWin8 Then
    Result := NoSD() + '{#Game}'
   Else
    Result := ExpandConstant('{pf}\') + '{#Game}';
  End;

 // 105
 Function MbOrTb(Float: Extended): String;
  Begin
   If Float < 1024 Then
    Result := FormatFloat('0', Float) + ' Mb'
   Else If (Float / 1024) < 1024 Then
    Result := Format('%.2n', [Float / 1024]) + ' GB'
   Else
    Result := Format('%.2n', [Float / (1024 * 1024)]) + ' TB';
   StringChange(Result, ',', '.');
  End;

 // 110
 Function FileSeach(Filename: String): AnsiString;
  Begin
   If Not FileExists(Filename) Then
    Begin
     TaskBarButtonEnabled(gv37, False);
     TaskBarButtonEnabled(gv38, False);
     If GetOpenFileName('File not found!', Filename, ExtractFilePath(Filename), ExtractFileName(Filename), ExtractFileExt(Filename)) Then
      Result := Filename;
     TaskBarButtonEnabled(gv37, True);
     TaskBarButtonEnabled(gv38, True);
    End
   Else
    Result := Filename;
  End;

 // 116
 Function FinishedDone: Boolean;
  var
   i: LongInt;
  Begin
   gv46.Hide;
   TaskBarButtonEnabled(gv37, True);
   TaskBarButtonEnabled(gv38, True);
   gv52.Caption := WizardForm.NextButton.Caption;
   gv52.Visible := True;
   For i := 0 To 40 Do
    begin
     WizardForm.ClientHeight := WizardForm.ClientHeight + ScaleY(1);
     i := i + 1;
    end;
   If ISDoneError Then
    begin
     gv47.Enabled := True;
     lblInstallResult.Caption := ExpandConstant('{cm:Fail}');
     lblInstallResult.Font.Color := $1B1BE7; 
     TaskBarButtonIcon(gv38, gv66[6].Handle);
     TaskBarButtonToolTip(gv38, 'Retry');
     memInstallLog.Lines.Add('Error!');
    end
   Else
    begin
     lblInstallResult.caption := ExpandConstant('{cm:Success}');
     lblInstallResult.Font.Color := $34DD00; 
     TaskBarButtonIcon(gv38, gv66[5].Handle);
     TaskBarButtonToolTip(gv38, 'Run {#Game}');
     memInstallLog.Lines.Add('Done!');
     If gv44 Then
      begin
       TaskBarButtonEnabled(gv38, True);
       gv48.Enabled := True;
      end
     Else
      begin
       gv48.Enabled := False;
       TaskBarButtonEnabled(gv38, False);
      end;
    end;
   lblInstallResult.Left := (WizardForm.ClientWidth - lblInstallResult.Width) / 2;
   lblInstallResult.Show;
   WizardForm.ProgressGauge.Style := npbstNormal;
   TaskBarButtonToolTip(gv37, 'Exit');
   TaskBarButtonIcon(gv37, gv66[7].Handle);
   Result := True;
  End;

 // 134
 Procedure BtnOnClick(Btn: Integer);
  var
   i: LongInt;
  Begin
   Case Btn of
    gv37 {hnext}  : If gv34 = 1 Then
                     WizardForm.NextButton.OnClick(WizardForm.NextButton)
                    Else If gv34 = 2 Then
                     gv46.OnClick(gv46)
                    Else If gv34 = 3 Then
                     WizardForm.NextButton.OnClick(WizardForm.NextButton);
    gv38 {hcancel}: If gv34 <= 2 Then
                     WizardForm.CancelButton.OnClick(WizardForm.CancelButton)
                    Else If ISDoneError Then
                     gv47.OnClick(gv47)
                    Else If not ISDoneError Then
                     gv48.OnClick(gv48);
    gv36         :  If Not (BASS_ChannelIsActive(gv58) = 3) Then
                     gv54.OnClick(gv54)
                    Else
                     gv53.OnClick(gv53);
   End;
  End; 

 Function LoadTaskBar: Boolean;
  var
   i: LongInt;
  Begin
   TaskBarV10(MainForm.Handle, WizardForm.Handle, TRUE, FALSE, ScaleY(40), ScaleX(18), gv65);
   SetTaskBarToolTip(0, '{#Game}');
   For i:= 1 To 7 Do
    Begin
     gv66[i] := TNewIcon.Create;
     gv66[i].LoadFromResourceName(HInstance, '_IS_' + IntToStr(i));
    End;
   gv36 := TaskBarAddButton(gv66[4].Handle, 'Music', CallbackAddr('BtnOnClick'), True);
   gv38 := TaskBarAddButton(gv66[7].Handle, 'Exit', CallbackAddr('BtnOnClick'), True);
   gv37 := TaskBarAddButton(gv66[1].Handle, 'Install', CallbackAddr('BtnOnClick'), True);
   TaskBarUpdateButtons(0);
   Result := True;
  End;

 // 146
 Function BASS_StreamCreateFile(mem: Bool; fil: AnsiString; offset, length, flags: DWORD): DWORD;
  var
   size_: DWord; Buffer: AnsiString;
  Begin
   If mem Then
    Begin
     size_ := ExtractTemporaryFileSize(fil);
     SetLength(Buffer, size_);
     ExtractTemporaryFileToBuffer(fil, Cast{#defined UNICODE ? "Ansi" : ""}StringToInteger(Buffer));
     Result := BASS_StreamCreateFileLib(mem, Buffer, offset, size_, flags);
    End
   Else
    Begin
     Result := BASS_StreamCreateFileLib(mem, fil, offset, length, flags);
    End;
  End;

 // 151
 Procedure MyOnTimer1(h, msg, idevent, dwTime: LongWord);
  Begin
   If gv35 > 100 Then
    KillTimer(WizardForm.Handle, gv60);
   gv35 := gv35 + 5;
   SetLayeredWindowAttributes(WizardForm.Handle, 0, 255 - gv35, 2)
  End;

  // 152
 Function MyProc(h:HWND; Msg, wParam, lParam: LongInt): LongInt;
  var
   ExStyle: LongInt;
  Begin
   If Msg = 534 Then
    Begin
     ExStyle := GetWindowLong(WizardForm.Handle, GWL_EXSTYLE);
     If not SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, gv64, 0) Then
      Exit;
     SetWindowLong(WizardForm.Handle, GWL_EXSTYLE, GetWindowLong(WizardForm.Handle, GWL_EXSTYLE) or WS_EX_LAYERED);
     If (gv35 = 0) and (gv64 <> 0) Then
      gv60 := SetTimer(WizardForm.Handle, 1, 10, WrapTimerProc(@MyOnTimer1, 4));
    End;
   If Msg = 533 Then
    Begin
     gv35 := 0;
     SetWindowLong(WizardForm.Handle, GWL_EXSTYLE, ExStyle);
     SetLayeredWindowAttributes(WizardForm.Handle, 0, 255, 2)
    End;
   Result := CallWindowProc(gv59, h, Msg, wParam, lParam);
  End;

 Function IsCrackTheres: Boolean;
  Begin
   if DirExists(ExpandConstant('{#Game_CrackDir}')) then begin
    if not PathIsDirectoryEmpty(ExpandConstant('{#Game_CrackDir}')) then begin
     Result := True; end
    else begin
     Result := False;
    end; end
   else begin
    Result := False;
   end;
  End;

 Function StartupAdmin(): Boolean;
  Begin
   hMutex := CreateMutexA(0, 1, 'WSjuQKBxmd_mut');
   if IsAdmin then begin
    Result := True; end
   else begin
    if MsgBox(ExpandConstant('{cm:msgIsAdmin}'), mbConfirmation, MB_YESNO) = IDYES then begin
     Result := True; end
    else begin
     Result := False;
    end;
   end;
   CloseHandle(hMutex);
  End;

 // 157
 Procedure CBDriveOnClick(Sender: TObject);
  var
   DirValue: AnsiString;
   I: Integer;
  Begin
   DirValue := WizardForm.DirEdit.Text;
   Delete(DirValue, 1, Length(gv26.Text));
   WizardForm.DirEdit.Text := gv26.Text + DirValue;
  End;

 // 162
 Procedure CancelButtonClick(CurPageID: Integer; var Cancel, Confirm: Boolean);
  Begin
   TaskBarButtonEnabled(gv37, False);
   TaskBarButtonEnabled(gv38, False);
   TaskBarButtonEnabled(gv36, False);
   Confirm := False;
   If CurPageID = wpInstalling Then
    Begin
     Cancel := False;
     SuspendProc;
     SetTaskBarProgressState(0, TBPF_PAUSED);
     If MsgBox(ExpandConstant('{cm:InteProc}'), mbConfirmation, MB_YESNO) = IDYES Then
      gv33 := 1;
     SetTaskBarProgressState(0, TBPF_NORMAL);
     TaskBarButtonEnabled(gv37, True);
     TaskBarButtonEnabled(gv38, True);
     TaskBarButtonEnabled(gv36, True);
     ResumeProc;
    End
   Else
    Begin
     AnimateWindow(WizardForm.Handle, 300, AW_FADE_OUT);
     Cancel := True;
    End;
  End;

 // 163
 Function NextButtonClick(CurPageID: Integer): Boolean;
  Begin
   Result := True;
   If CurPageID = wpSelectDir Then Begin
    TaskBarButtonEnabled(gv37, False);
    TaskBarButtonEnabled(gv38, False);
    TaskBarButtonEnabled(gv36, False);
    If not IsAnsi(WizardForm.DirEdit.Text) Then Begin
     MsgBox(ExpandConstant('{cm:ErrDir}'), mbError, MB_OK);
     Result := False;
    End;
    If not IsAnsi(WizardForm.GroupEdit.Text) and WizardForm.GroupEdit.Enabled Then Begin
     MsgBox(ExpandConstant('{cm:ErrBro}'), mbError, MB_OK);
     Result := False;
    End;
    If gv29 < {#Game_NeedSize} Then Begin
     MsgBox(ExpandConstant('{cm:ErrSize}'), mbError, MB_OK);
     Result := False;
    End;
    TaskBarButtonEnabled(gv37, True);
    TaskBarButtonEnabled(gv38, True);
    TaskBarButtonEnabled(gv36, True);
   End;
   If CurPageID = wpFinished Then Begin
    AnimateWindow(WizardForm.Handle, 500, AW_FADE_OUT);
   End;
  End;
 
 // 166
 Procedure DirEditOnChange(Sender: TObject);
  var
   Path: String;
  Begin
   Path := ExtractFileDrive(WizardForm.DirEdit.Text); 
   GetSpaceOnDisk(Path, True, gv29, gv30);
   gv08.Caption := ExpandConstant('{cm:FreeSpace1}') + ' ' + MbOrTb({#Game_NeedSize})+ ' ' + ExpandConstant('{cm:FreeSpace2}');
   If not (gv29 > {#Game_NeedSize}) Then
    gv08.Font.Color := $1B1BE7
   Else
    gv08.Font.Color := $E6E0E1;
   If gv26.Items.IndexOf(AddBackslash(ExtractFileDrive(WizardForm.DirEdit.Text))) >= 0 Then
    gv26.ItemIndex := gv26.Items.IndexOf(AddBackslash(ExtractFileDrive(WizardForm.DirEdit.Text)));
  End;

 // 168
 Procedure PauseBtnClick(Sender: TObject);
  Begin
   If Not gv43 Then
    Begin
     SuspendProc;
     WizardForm.ProgressGauge.State := npbsPaused;
     gv46.Caption := 'Resume';
     WizardForm.CancelButton.Enabled := False;
     gv51.Enabled := False;
     SetTaskBarProgressState(0, TBPF_PAUSED);
     TaskBarButtonToolTip(gv37, 'Resume');
     TaskBarButtonIcon(gv37, gv66[1].Handle);
     TaskBarButtonEnabled(gv38, False);
     gv43 := True;
     Exit;
    End
   Else
    Begin
     ResumeProc;
     WizardForm.ProgressGauge.State := npbsNormal;
     gv46.Caption := 'Pause';
     WizardForm.CancelButton.Enabled := True;
     gv51.Enabled := True;
     SetTaskBarProgressState(0, TBPF_NORMAL);
     TaskBarButtonToolTip(gv37, 'Pause');
     TaskBarButtonIcon(gv37, gv66[2].Handle);
     TaskBarButtonEnabled(gv38, True);
     gv43 := False;
     Exit;
    End;
  End;

 // 170
 Procedure AgainOnClick(Sender: TObject);
  Begin
   hMutex := CreateMutexA(0, 1, 'WSjuQKBxmd_mut');
   MyRetryApp('');
   CloseHandle(hMutex);                                                   
   WizardForm.NextButton.OnClick(WizardForm.NextButton);
  End;

  // 171
 Procedure RunOnClick(Sender: TObject);
  var
   ResultCode: Integer;
  Begin
   if IsWin64 then begin
    Exec(Expandconstant('{#GameExe}'), '', '', SW_SHOW, ewNoWait, ResultCode); end

   end;

 // 173
 Procedure Label1OnMouseEnter(Sender: TObject);
 Begin
  If gv22.Enabled Then
   gv06.Font.Color :=$2D7DEA;
 End;

 // 174
 Procedure Label1OnMouseLeave(Sender: TObject);
 Begin
  If gv22.Enabled Then
   gv06.Font.Color :=$E6E0E1;
 End;

 // 175
 Procedure Label1OnClick(Sender: TObject);
 Begin
  If gv22.Checked And gv22.Enabled Then
   gv22.Checked := False
  Else
   gv22.Checked := True;
 End;

 // 178
 Function Icon: Boolean;
 Begin
  If gv22.Checked Then
   Result := True
  Else
   Result := False;
 End;

 // 179
 Procedure NoStartLabelOnMouseEnter(Sender: TObject);
 Begin
  If gv23.Enabled Then
   gv05.Font.Color :=$2D7DEA;
 End;

 // 180
 Procedure NoStartLabelOnMouseLeave(Sender: TObject);
 Begin
  If gv23.Enabled Then
   gv05.Font.Color :=$E6E0E1;
 End;

 // 181
 Procedure NoStartLabelOnClick(Sender: TObject);
 Begin
  If gv23.Enabled Then
   gv23.Checked := not(gv23.Checked);
  If (gv23.Checked and gv23.Enabled) Then
   begin
    gv50.Enabled := True;
    Wizardform.GroupEdit.Enabled := True;
   end
  Else
   begin
    gv50.Enabled := False;
    Wizardform.GroupEdit.Enabled := False;
   end;
 End;

 // 182
 Procedure NoStartCheckListBoxClick(Sender: TObject);
 Begin
  If (gv23.Checked and gv23.Enabled) Then
   begin
    gv50.Enabled := True;
    Wizardform.GroupEdit.Enabled := True;
   end
  Else
   begin
    gv50.Enabled := False;
    Wizardform.GroupEdit.Enabled := False;
   end;
 End;

 // 183
 Function Start: Boolean;
 Begin
  If gv23.Checked Then
   Result := True
  Else
   Result := False
 End;

 // 184
 Function UnnIn: Boolean;
  var
   Unused: Integer;
 Begin
  If ((gv24.Checked) and (gv33 <> 1)) Then
   Result := False
  Else
   Result := True
 End;

 // 185
 Procedure InstallOptionsOnMouseEnter(Sender: TObject);
 Begin
  If gv24.Enabled Then
   gv07.Font.Color :=$2D7DEA;
 End;

 // 186
 Procedure InstallOptionsOnMouseLeave(Sender: TObject);
 Begin
  If gv24.Enabled Then
   gv07.Font.Color :=$E6E0E1;
 End;

  // 187
 Procedure InstallOptionsOnClick(Sender: TObject);
 Begin
  If (gv24.Checked and gv24.Enabled) Then
   begin
    gv24.Checked := False;
   end
  Else
   begin
    gv24.Checked := True;
   end;
 End;

 Procedure LabelCrackOnMouseEnter(Sender: TObject);
  begin
   if gv25.Enabled then begin
    gv09.Font.Color := $2D7DEA;
   end;
  end;

 Procedure LabelCrackOnMouseLeave(Sender: TObject);
  begin
   if gv25.Enabled then begin
    gv09.Font.Color := $E6E0E1;
   end;
  end;

 Procedure LabelCrackOnClick(Sender: TObject);
 Begin
  If (gv25.Checked and gv25.Enabled) Then
   begin
    gv25.Checked := False;
   end
  Else
   begin
    gv25.Checked := True;
   end;
 End;

 // 191
 Procedure NullOnClick(Sender: TObject);
 Begin
 End;

 // 192
 Procedure Bass_ChangePos(var1: Single);
 Begin
  gv62 := var1;
  If gv62 < 0.03 Then
   BASS_ChannelPause(gv58)
  Else
   begin
    If (BASS_ChannelIsActive(gv58) = 3) Then
     BASS_ChannelPlay(gv58, False);
    Log(FloatToStr(gv62));
    BASS_ChannelSetAttribute(gv58, 2, gv62);
   end;
 End;

 // 195
 Procedure ImgButtonOnMove(Sender: TObject; Shift: TShiftState; X, Y: Integer);
 Begin
  If not GetCursorPos(gv61) Then Exit;
  With Sender Do
   begin
    gv61 := WizardForm.ScreenToClient(gv61);
    If (gv42 and (gv55.Left < gv61.x) and ((gv55.Left + gv55.Width) > gv61.x)) Then
     TBitmapImage(Sender).Left := (gv61.x - TBitmapImage(Sender).Width / 2 );
    gv63 := gv55.Width;
    gv62 := (gv56.Left + gv56.Width / 4 + ScaleX(1) - gv55.Left) / (gv63 + gv56.Width / 4);
    If gv42 Then 
     BASS_ChangePos(gv62);
   end;
 End;

 // 199
 Procedure BkgButtonOnMouseEnter(Sender: TObject; Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
 Begin
  If not GetCursorPos(gv61) Then Exit;
  If not gv42 Then
   begin
    gv61 := WizardForm.ScreenToClient(gv61);
    If gv61.x > gv56.Left Then
     gv56.Left := (gv56.Left + 3) + gv56.Width / 2
    Else
     gv56.Left := (gv56.Left - 3) - gv56.Width / 2;
    gv63 := gv55.Width;
    gv62 := (gv56.Left + gv56.Width / 4 + ScaleX(1) - gv55.Left) / (gv63 + gv56.Width / 4);
    BASS_ChangePos(gv62);
   end;
 End;

 // 200
 Procedure ImgButtonOnMouseDown(Sender: TObject; Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
 Begin
  gv42 := True;
  With Sender Do
   Try
    gv27 := TBitmapImage(Sender).Name;
    ExtractTemporaryFileToStream(gv27 + '3.bmp', gv57);
    gv57.Position := 0;
    TBitmapImage(Sender).Bitmap.LoadFromStream(gv57);
   Finally
    gv57.Clear;
   End;
 End;

 // 207
 Procedure ImgButtonOnMouseUp(Sender: TObject; Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
 Begin
  gv42 := False;
  If gv41 Then
   With Sender Do
    Try
     gv27 := TBitmapImage(Sender).Name;
     ExtractTemporaryFileToStream(gv27 + '2.bmp', gv57);
     gv57.Position := 0;
     TBitmapImage(Sender).Bitmap.LoadFromStream(gv57);
    Finally
     gv57.Clear;
    End;
 End;

 // 208
 Procedure ImgButtonOnMouseEnter(Sender: TObject);
 Begin
  gv41 := True;
  With Sender Do
   Try
    gv27 := TBitmapImage(Sender).Name;
    If gv42 Then
     ExtractTemporaryFileToStream(gv27 + '3.bmp', gv57)
    Else
     ExtractTemporaryFileToStream(gv27 + '2.bmp', gv57);
    gv57.Position := 0;
    TBitmapImage(Sender).Bitmap.LoadFromStream(gv57);
   Finally
    gv57.Clear;
   End;
 End;

  // 209
 Procedure ImgButtonOnMouseLeave(Sender: TObject);
 Begin
  gv41 := False;
  With Sender Do
   Try
    gv27 := TBitmapImage(Sender).Name;
    ExtractTemporaryFileToStream(gv27 + '1.bmp', gv57);
    gv57.Position := 0;
    TBitmapImage(Sender).Bitmap.LoadFromStream(gv57);
   Finally
    gv57.Clear;
   End;
 End;

  // 210
 Procedure ImgButton1OnClick(Sender: TObject);
 Begin
  If not (BASS_ChannelIsActive(gv58) = 3) Then
   begin
    BASS_ChannelPause(gv58);
    TaskBarButtonIcon(gv36, gv66[3].Handle);
   end
 End;

  // 211
 Procedure ImgButton2OnClick(Sender: TObject);
 Begin
  If ((BASS_ChannelIsActive(gv58) = 3) and (gv62 > 0.03)) Then
   begin
    BASS_ChannelPlay(gv58, False);
    TaskBarButtonIcon(gv36, gv66[4].Handle);
   end
 End;

 // 212
 Procedure LoadSoundButton;
 Begin
  gv54 := TBitmapImage.Create(WizardForm);
  gv54.ReplaceColor := clBlack;
  gv54.ReplaceWithColor := WizardForm.Color;
  gv54.Parent := WizardForm;
  gv54.Stretch := True;
  Try
   gv57 := TMemoryStream.Create;
   ExtractTemporaryFileToStream('Pause1.bmp', gv57);
   gv57.Position := 0;
   gv54.Bitmap.LoadFromStream(gv57);
  Finally
   gv57.Clear;
  End;
  gv54.SetBounds(gv16.Left + gv16.Width - ScaleX(16), gv16.Top + ScaleY(7), ScaleX(11), ScaleY(11));
  gv54.Name := 'Pause';
  gv54.OnMouseEnter := @ImgButtonOnMouseEnter;
  gv54.OnMouseLeave := @ImgButtonOnMouseLeave;
  gv54.OnMouseDown := @ImgButtonOnMouseDown;
  gv54.OnMouseUp := @ImgButtonOnMouseUp;
  gv54.OnClick := @ImgButton1OnClick;
  gv53 := TBitmapImage.Create(WizardForm);
  gv53.Parent := WizardForm;
  gv53.ReplaceColor := clBlack;
  gv53.ReplaceWithColor := WizardForm.Color;
  gv53.Stretch := True;
  Try
   ExtractTemporaryFileToStream('Play1.bmp', gv57);
   gv57.Position := 0;
   gv53.Bitmap.LoadFromStream(gv57);
  Finally
   gv57.Clear;
  End;
  gv53.SetBounds(gv54.Left - ScaleX(13), gv54.Top, ScaleX(11), ScaleY(11));
  gv53.Name := 'Play';
  gv53.OnMouseEnter := @ImgButtonOnMouseEnter;
  gv53.OnMouseLeave := @ImgButtonOnMouseLeave;
  gv53.OnMouseDown := @ImgButtonOnMouseDown;
  gv53.OnMouseUp := @ImgButtonOnMouseUp;
  gv53.OnClick := @ImgButton2OnClick;
  gv55 := TBitmapImage.Create(WizardForm);
  gv55.Parent := WizardForm;
  gv55.ReplaceColor := clBlack;
  gv55.ReplaceWithColor := WizardForm.Color;
  gv55.Stretch := True;
  Try
   ExtractTemporaryFileToStream('trackBkg.bmp', gv57);
   gv57.Position := 0;
   gv55.Bitmap.LoadFromStream(gv57);
  Finally
   gv57.Clear;
  End;
  gv55.SetBounds(gv53.Left - ScaleX(65), gv53.Top + ScaleY(4), ScaleX(60), ScaleY(3));
  gv55.Name := 'trackBkg';
  gv55.OnMouseDown := @BkgButtonOnMouseEnter;
  gv56 := TBitmapImage.Create(WizardForm);
  gv56.Parent := WizardForm;
  gv56.ReplaceColor := clBlack;
  gv56.ReplaceWithColor := WizardForm.Color;
  gv56.Stretch := True;
  Try
   ExtractTemporaryFileToStream('trackbtn1.bmp', gv57);
   gv57.Position := 0;
   gv56.Bitmap.LoadFromStream(gv57);
  Finally
   gv57.Clear;
  End;
  gv56.SetBounds(gv55.Left + gv55.Width / 2, gv55.Top - ScaleY(9) / 2 + gv55.Height / 2, ScaleX(8), ScaleY(9));
  gv56.Name := 'trackbtn';
  gv56.OnMouseEnter := @ImgButtonOnMouseEnter;
  gv56.OnMouseLeave := @ImgButtonOnMouseLeave;
  gv56.OnMouseDown := @ImgButtonOnMouseDown;
  gv56.OnMouseUp := @ImgButtonOnMouseUp;
  gv56.OnMouseMove := @ImgButtonOnMove;
  BASS_Init(-1, 44100, 0, 0, 0);
  BASS_Start;
  gv58 := BASS_StreamCreateFile(True, 'Music{#MusicGroup}.ogg', 0, 0, 4);
  gv63 := gv55.Width;
  gv62 := (gv56.Left + gv56.Width / 4 + ScaleX(1) - gv55.Left) / (gv63 + gv56.Width / 4);
  BASS_ChangePos(gv62);
  BASS_ChannelPlay(gv58, True);
  If not (BASS_ChannelIsActive(gv58) = 3) Then
   TaskBarButtonIcon(gv36, gv66[4].Handle)
  Else
   TaskBarButtonIcon(gv36, gv66[3].Handle);
 End;

 // 230
 Procedure DubleOnClick(Sender: TObject);
 Begin
  Case Sender of
   gv51: WizardForm.CancelButton.OnClick(WizardForm.CancelButton);
   gv52: WizardForm.NextButton.OnClick(WizardForm.NextButton);
  End;
 End;

 // 231
 Procedure DirFolderChange(Sender: TObject);
 Begin
  DirEdit.Text := AddBackslash(FolderTreeView.Directory) + '{#Game}';
 End;

 // 233
 Procedure BackClick(Sender: TObject);
 Begin
  FolderTreeView.ChangeDirectory(AddBackslash(WizardForm.DirEdit.Text), True);
  DirEdit.Text := AddBackslash(FolderTreeView.Directory);
 End;

 // 235
 Procedure NewClick(Sender: TObject);
 Begin
  FolderTreeView.CreateNewDirectory(SetupMessage(msgNewFolderName));
  DirEdit.Text := AddBackslash(FolderTreeView.Directory);
 End;

 // 238
 Procedure BrowseClick(Sender: TObject);
  var
   wBtn01: TButton; // 01
   wBtn02: TButton; // 02
   wBtn03: TButton; // 03
   wBtn04: TButton; // 03
   wLbl05: TLabel; // 05
   wBvl06: TBevel; // 06
   wBvl07: TBevel; // 07
   wBvl08: TBevel; // 08
   wBvl09: TBevel; // 09
   wBvl10: TBevel; // 10
   wBvl11: TBevel; // 11
   wBvl12: TBevel; // 12
   wBvl13: TBevel; // 13
   wBvl14: TBevel; // 14
   wBvl15: TBevel; // 15
  Begin
   gv67 := CreateCustomForm();
   Try
    gv67.ClientWidth := ScaleX(450);
    gv67.ClientHeight := ScaleY(317);
    gv67.Position := poScreenCenter;
    gv67.Caption := SetupMessage(msgBrowseDialogTitle);
    gv67.Font.Name := WizardForm.DirEdit.Font.Name;
    gv67.Font.Size := 8;
    gv67.Font.Color := $E6E0E1;
    gv67.CenterInsideControl(WizardForm, False);
    FolderTreeView := TFolderTreeView.Create(gv67);
    FolderTreeView.SetBounds(ScaleX(5), ScaleY(51), ScaleX(355), ScaleY(261));
    FolderTreeView.OnChange := @DirFolderChange;
    FolderTreeView.Parent := gv67;
    DirEdit := TNewEdit.Create(gv67);
    DirEdit.SetBounds(ScaleX(5), ScaleY(25), ScaleX(440), ScaleY(15));
    DirEdit.Text := WizardForm.DirEdit.Text;
    DirEdit.Parent := gv67;
    wLbl05 := TLabel.Create(gv67);
    wLbl05.SetBounds(ScaleX(6), ScaleY(5), ScaleX(400), ScaleY(20));
    wLbl05.Caption := SetupMessage(msgBrowseDialogLabel);
    wLbl05.Parent := gv67;
    wLbl05.Font.Color := $E6E0E1;
    wBvl06 := TBevel.Create(gv67);
    wBvl06.SetBounds(ScaleX(365), FolderTreeView.Top - ScaleY(2), ScaleX(80), ScaleY(5));
    wBvl06.Shape := bsBottomLine;
    wBvl06.Style := bsRaised;
    wBvl06.Parent := gv67;
    wBvl07 := TBevel.Create(gv67);
    wBvl07.SetBounds(wBvl06.Left, wBvl06.Top + ScaleY(10), wBvl06.Width, wBvl06.Height);
    wBvl07.Shape := bsBottomLine;
    wBvl07.Style := bsRaised;
    wBvl07.Parent := gv67;
    If IsThemeActive Then
     wBtn01 := TButton.Create(gv67)
    Else
     wBtn01 := TNewButton.Create(gv67);
    wBtn01.SetBounds(wBvl06.Left, wBvl07.Top + ScaleY(17), wBvl06.Width, ScaleY(25));
    wBtn01.Font.Name := WizardForm.DirEdit.Font.Name;
    wBtn01.Font.Size := 8;
    wBtn01.Font.Color := clBlack;
    wBtn01.Parent := gv67;
    wBtn01.Caption := SetupMessage(msgButtonOK);
    wBtn01.ModalResult := mrOk;
    wBtn01.Default := True;
    wBvl08 := TBevel.Create(gv67);
    wBvl08.SetBounds(wBvl06.Left, wBtn01.Top + wBtn01.Height + ScaleY(10), wBvl06.Width, wBvl06.Height);
    wBvl08.Shape := bsBottomLine;
    wBvl08.Style := bsRaised;
    wBvl08.Parent := gv67;
    wBvl09 := TBevel.Create(gv67);
    wBvl09.SetBounds(wBvl08.Left, wBvl08.Top + ScaleY(10), wBvl06.Width, wBvl06.Height);
    wBvl09.Shape := bsBottomLine;
    wBvl09.Style := bsRaised;
    wBvl09.Parent := gv67;
    If IsThemeActive Then
     wBtn02 := TButton.Create(gv67)
    Else
     wBtn02 := TNewButton.Create(gv67);
    wBtn02.SetBounds(wBvl06.Left, wBvl09.Top + ScaleY(17), wBvl06.Width, ScaleY(25));
    wBtn02.Font.Name := WizardForm.DirEdit.Font.Name;
    wBtn02.Font.Size := 8;
    wBtn02.Font.Color := clBlack;
    wBtn02.Parent := gv67;
    wBtn02.Caption := SetupMessage(msgButtonCancel);
    wBtn02.ModalResult := mrCancel;
    wBtn02.Cancel := True;
    wBvl10 := TBevel.Create(gv67);
    wBvl10.SetBounds(wBvl06.Left, wBtn02.Top + wBtn02.Height + ScaleY(10), wBvl06.Width, wBvl06.Height);
    wBvl10.Shape := bsBottomLine;
    wBvl10.Style := bsRaised;
    wBvl10.Parent := gv67;
    wBvl11 := TBevel.Create(gv67);
    wBvl11.SetBounds(wBvl08.Left, wBvl10.Top + ScaleY(10), wBvl06.Width, wBvl06.Height);
    wBvl11.Shape := bsBottomLine;
    wBvl11.Style := bsRaised;
    wBvl11.Parent := gv67;
    If IsThemeActive Then
     wBtn03 := TButton.Create(gv67)
    Else
     wBtn03 := TNewButton.Create(gv67);
    wBtn03.SetBounds(wBvl06.Left, wBvl11.Top + ScaleY(17), wBvl06.Width, ScaleY(25));
    wBtn03.Font.Name := WizardForm.DirEdit.Font.Name;
    wBtn03.Font.Size := 8;
    wBtn03.Font.Color := clBlack;
    wBtn03.Parent := gv67;
    wBtn03.Caption := 'Default';
    wBtn03.OnClick := @BackClick;
    wBvl12 := TBevel.Create(gv67);
    wBvl12.SetBounds(wBvl06.Left, wBtn03.Top + wBtn03.Height + ScaleY(10), wBvl06.Width, wBvl06.Height);
    wBvl12.Shape := bsBottomLine;
    wBvl12.Style := bsRaised;
    wBvl12.Parent := gv67;
    wBvl13 := TBevel.Create(gv67);
    wBvl13.SetBounds(wBvl08.Left, wBvl12.Top + ScaleY(10), wBvl06.Width, wBvl06.Height);
    wBvl13.Shape := bsBottomLine;
    wBvl13.Style := bsRaised;
    wBvl13.Parent := gv67;
    If IsThemeActive Then
     wBtn04 := TButton.Create(gv67)
    Else
     wBtn04 := TNewButton.Create(gv67);
    wBtn04.SetBounds(wBvl06.Left, wBvl13.Top + ScaleY(17), wBvl06.Width, ScaleY(25));
    wBtn04.Font.Name := WizardForm.DirEdit.Font.Name;
    wBtn04.Font.Size := 8;
    wBtn04.Font.Color := clBlack;
    wBtn04.Parent := gv67;
    wBtn04.Caption := SetupMessage(msgNewFolderName);
    wBtn04.OnClick := @NewClick;
    wBvl14 := TBevel.Create(gv67);
    wBvl14.SetBounds(wBvl06.Left, wBtn04.Top + wBtn04.Height + ScaleY(10), wBvl06.Width, wBvl06.Height);
    wBvl14.Shape := bsBottomLine;
    wBvl14.Style := bsRaised;
    wBvl14.Parent := gv67;
    wBvl15 := TBevel.Create(gv67);
    wBvl15.SetBounds(wBvl08.Left, wBvl14.Top + ScaleY(10), wBvl06.Width, wBvl06.Height);
    wBvl15.Shape := bsBottomLine;
    wBvl15.Style := bsRaised;
    wBvl15.Parent := gv67;
    FolderTreeView.ChangeDirectory(AddBackslash(WizardForm.DirEdit.Text), True);
    DirEdit.Text := AddBackslash(FolderTreeView.Directory);
    gv67.ActiveControl := FolderTreeView;
    If gv67.ShowModal = mrOk Then
     WizardForm.DirEdit.Text := AddBackslash(DirEdit.Text);
   Finally
    gv67.Free;
   End;
  End;

 // 260
 Procedure GroupFolderChange(Sender: TObject);
  Begin
   gv02.Text := AddBackslash(gv00.Directory) + '{#Game}';
  End;

 // 261
 Procedure GroupClick(Sender: TObject);
  var
   wBtn01: TButton; // 01 {btnCancel}
   wBtn02: TButton; // 02 {btnOk}
   wLbl03: TLabel;  // 03 {lblBrowse}
  Begin
   gv68 := CreateCustomForm();
   Try
    gv68 := CreateCustomForm();
    gv68.ClientWidth := ScaleX(455);
    gv68.ClientHeight := ScaleY(260);
    gv68.Caption := SetupMessage(msgBrowseDialogTitle);
    gv68.Position := poScreenCenter;
    gv68.Font.Name := WizardForm.DirEdit.Font.Name;
    gv68.Font.Size := 8;
    gv68.Font.Color := $E6E0E1;
    gv68.CenterInsideControl(WizardForm, False);
    If IsThemeActive Then
     wBtn02 := TButton.Create(gv68)
    Else
     wBtn02 := TNewButton.Create(gv68);
    wBtn02.SetBounds(ScaleX(285), ScaleY(25), ScaleX(80), ScaleY(25));
    wBtn02.Font.Name := WizardForm.DirEdit.Font.Name;
    wBtn02.Font.Size := 8;
    wBtn02.Font.Color := clBlack;
    wBtn02.Parent := gv68;
    wBtn02.Caption := SetupMessage(msgButtonOK);
    wBtn02.ModalResult := mrOk;
    wBtn02.Default := True;
    If IsThemeActive Then
     wBtn01 := TButton.Create(gv68)
    Else
     wBtn01 := TNewButton.Create(gv68);
    wBtn01.SetBounds(ScaleX(370), ScaleY(25), ScaleX(80), ScaleY(25));
    wBtn01.Font.Name := WizardForm.DirEdit.Font.Name;
    wBtn01.Font.Size := 8;
    wBtn01.Font.Color := clBlack;
    wBtn01.Parent := gv68;
    wBtn01.Caption := SetupMessage(msgButtonCancel);
    wBtn01.ModalResult := mrCancel;
    wBtn01.Cancel := True;
    gv00 := TStartMenuFolderTreeView.Create(gv68);
    gv00.SetBounds(ScaleX(5), ScaleY(55), ScaleX(445), ScaleY(200));
    gv00.OnChange := @GroupFolderChange;
    gv00.SetPaths(ExpandConstant('{userprograms}'), ExpandConstant('{commonprograms}'), ExpandConstant('{userstartup}'), ExpandConstant('{commonstartup}'));
    gv00.Parent := gv68;
    gv02 := TNewEdit.Create(gv68);
    gv02.SetBounds(ScaleX(5), ScaleY(27), ScaleX(275), ScaleY(15));
    gv02.Text := WizardForm.GroupEdit.Text;
    gv02.Parent := gv68;
    wLbl03 := TLabel.Create(gv68);
    wLbl03.SetBounds(ScaleX(6), ScaleY(5), ScaleX(400), ScaleY(20));
    wLbl03.Caption := SetupMessage(msgBrowseDialogLabel);
    wLbl03.Parent := gv68;
    wLbl03.Font.Color := $E6E0E1;
    gv00.ChangeDirectory(AddBackslash(WizardForm.GroupEdit.Text), True);
    gv02.Text := AddBackslash(gv00.Directory);
    gv68.ActiveControl := gv00;
    If gv68.ShowModal = mrOk Then
     WizardForm.GroupEdit.Text := AddBackslash(gv02.Text);
   Finally
    gv68.Free;
   End;
  End;

 // 264
 Procedure StyleStreamCreate(InitStylefile: String);
  var
     StyleSize: Cardinal;
   StyleBuffer: AnsiString;
  Begin
   StyleSize := ExtractTemporaryFileSize(InitStylefile);
   SetLength(StyleBuffer, StyleSize);
   ExtractTemporaryFileToBuffer(InitStylefile, Cast{#defined UNICODE ? "Ansi" : ""}StringToInteger(StyleBuffer));
   LoadFromStreamVCLStyle(StyleBuffer, 'KZXRf5RrfMs4K4Li');
  End;

 // 265
 Function InitializeSetup(): Boolean;
 Begin
  hMutex := CreateMutexA(0, 1, 'WSjuQKBxmd_mut');
  gv40 := False;
  If not FileExists(ExpandConstant('{tmp}\ISDone.dll')) Then
   ExtractTemporaryFile('ISDone.dll');
  If not FileExists(ExpandConstant('{tmp}\VclStylesinno.dll')) Then
   ExtractTemporaryFile('VclStylesinno.dll');
  If not FileExists(ExpandConstant('{tmp}\BASS.dll')) Then
   ExtractTemporaryFile('BASS.dll');
  If not FileExists(ExpandConstant('{tmp}\bp.dll')) Then
   ExtractTemporaryFile('bp.dll');
  If not FileExists(ExpandConstant('{tmp}\wintb.dll')) Then
   ExtractTemporaryFile('wintb.dll');
  StyleStreamCreate('{#Style}.vsf');
  CloseHandle(hMutex);
  Result := StartUpAdmin;
 End;

Procedure InitializeWizard();
 Begin
  Application.Title := '{#Game}';
  WizardForm.Caption := '{#Game}';
  WizardForm.ReadyMemo.Hide;
  WizardForm.MainPanel.Hide;
  WizardForm.PageNameLabel.Hide;
  WizardForm.PageDescriptionLabel.Hide;
  WizardForm.DiskSpaceLabel.Hide;
  WizardForm.WelcomeLabel1.Hide;
  WizardForm.WelcomeLabel2.Hide;
  WizardForm.SelectDirBitmapImage.Hide;
  WizardForm.SelectDirBrowseLabel.Hide;
  WizardForm.SelectDirLabel.Hide;
  WizardForm.FinishedHeadingLabel.Hide;
  WizardForm.FinishedLabel.Hide;
  WizardForm.FilenameLabel.Hide;
  WizardForm.StatusLabel.Hide;
  WizardForm.SelectStartMenuFolderLabel.Hide;
  WizardForm.SelectStartMenuFolderBrowseLabel.Hide;
  WizardForm.ReadyLabel.Hide;
  WizardForm.LicenseLabel1.Hide;
  WizardForm.InfoBeforeClickLabel.Hide;
  WizardForm.InfoAfterClickLabel.Hide;
  WizardForm.ComponentsDiskSpaceLabel.Hide;
  WizardForm.SelectComponentsLabel.Hide;
  WizardForm.WizardSmallBitmapImage.Hide;
  WizardForm.SelectGroupBitmapImage.Hide;
  WizardForm.OuterNotebook.Hide;
  WizardForm.InnerNotebook.Hide;
  WizardForm.Bevel.Hide;
  WizardForm.Bevel1.Hide;
  WizardForm.DirBrowseButton.Enabled := False; WizardForm.DirBrowseButton.Visible := False;
  WizardForm.GroupBrowseButton.Enabled := False; WizardForm.GroupBrowseButton.Visible := False;
  WizardForm.ClientWidth := ScaleX(480);
  WizardForm.ClientHeight := ScaleY(580);
  WizardForm.Position := poScreenCenter;
  WizardForm.WizardBitmapImage.Parent := WizardForm;
  WizardForm.WizardBitmapImage.SetBounds(WizardForm.ClientWidth / 2 - ScaleX(225), ScaleY(0), ScaleX(450), ScaleY(65));
  gv12 := TBevel.Create(WizardForm);
  gv12.SetBounds(ScaleX(10), ScaleY(60), WizardForm.ClientWidth - ScaleX(20), ScaleY(180));
  gv12.Shape := bsBox;  
  gv12.Style := bsRaised;  
  gv12.Parent := WizardForm;
  gv13 := TBevel.Create(WizardForm);
  gv13.SetBounds(gv12.Left + ScaleX(10), gv12.Top + ScaleY(10), gv12.Width - ScaleX(20), ScaleY(75));
  gv13.Shape := bsBox;  
  gv13.Style := bsLowered;  
  gv13.Parent := WizardForm;
  gv14 := TBevel.Create(WizardForm);
  gv14.SetBounds(gv13.Left, gv13.Top + gv13.Height + ScaleY(10), gv13.Width, gv13.Height);
  gv14.Shape := bsBox;  
  gv14.Style := bsLowered;  
  gv14.Parent := WizardForm;
  gv15 := TBevel.Create(WizardForm);
  gv15.SetBounds(gv12.Left, gv12.Top + gv12.Height + ScaleY(10), gv12.Width, gv13.Height - ScaleY(7));
  gv15.Shape := bsBox;  
  gv15.Style := bsLowered;  
  gv15.Parent := WizardForm;
  gv16 := TBevel.Create(WizardForm);
  gv16.SetBounds(gv15.Left + ScaleX(10), gv15.Top + ScaleY(10), gv15.Width - ScaleX(20), gv15.Height - ScaleY(20));
  gv16.Shape := bsBox;  
  gv16.Style := bsLowered;  
  gv16.Parent := WizardForm;
  WizardForm.DirEdit.Parent := WizardForm;
  WizardForm.DirEdit.SetBounds(gv13.Left + ScaleX(55), gv13.Top + ScaleY(25), gv13.Width - ScaleX(150), ScaleY(75))
  WizardForm.DirEdit.Font.Color := $E6E0E1;
  If IsThemeActive Then
   gv49 := TButton.Create(WizardForm)
  Else
   gv49 := TNewButton.Create(WizardForm);
  gv49.SetBounds(WizardForm.DirEdit.Left + WizardForm.DirEdit.Width + ScaleX(5), WizardForm.DirEdit.Top - ScaleY(1), ScaleX(80), ScaleY(23));
  gv49.Font.Name := WizardForm.DirEdit.Font.Name;
  gv49.Font.Size := 8;
  gv49.Parent := WizardForm;
  gv49.Caption := WizardForm.DirBrowseButton.Caption;
  gv49.OnClick := @BrowseClick;
  WizardForm.GroupEdit.Parent:= WizardForm;
  WizardForm.GroupEdit.SetBounds(gv14.Left + ScaleX(10), gv14.Top + ScaleY(25), gv14.Width - ScaleX(105), ScaleY(75))
  WizardForm.GroupEdit.Font.Color := $E6E0E1;
  If IsThemeActive Then
   gv50 := TButton.Create(WizardForm)
  Else
   gv50 := TNewButton.Create(WizardForm);
  gv50.SetBounds(WizardForm.GroupEdit.Left + WizardForm.GroupEdit.Width + ScaleX(5), WizardForm.GroupEdit.Top - ScaleY(1), ScaleX(80), ScaleY(23));
  gv50.Font.Name := WizardForm.DirEdit.Font.Name;
  gv50.Font.Size := 8;
  gv50.Parent := WizardForm;
  gv50.Caption := WizardForm.DirBrowseButton.Caption;
  gv50.OnClick := @GroupClick;
  gv04 := TLabel.Create(WizardForm);
  gv04.Parent := WizardForm;
  gv04.AutoSize := False;
  gv04.SetBounds(gv13.Left + ScaleX(10), gv13.Top + ScaleY(5), gv13.Width - ScaleX(20), ScaleY(15));
  gv04.Transparent := True;
  gv04.WordWrap := True;
  gv04.Font.Color := $E6E0E1; gv04.Font.Name := 'Arial'; gv04.Font.Style := [fsBold]; gv04.Font.Size := 9;
  gv04.Caption := ExpandConstant('{cm:DirInstall}');
  gv26 := TNewComboBox.Create(WizardForm);
  gv26.Parent := WizardForm;
  gv26.SetBounds(gv04.Left, WizardForm.DirEdit.Top, ScaleX(40), WizardForm.DirEdit.Height);
  gv26.Style := csDropDownList;
  gv26.OnClick := @CBDriveOnClick;
  AddDriveToList(gv26);
  gv22 := TCheckBox.Create(WizardForm);
  gv22.SetBounds(gv04.Left, WizardForm.DirEdit.Top + WizardForm.DirEdit.Height + ScaleY(5), ScaleX(15), ScaleY(15));
  gv22.Parent := WizardForm;
  gv22.Checked := True;
  gv22.Cursor := 1;
  gv06 := TLabel.Create(WizardForm);
  gv06.Parent := WizardForm;
  gv06.SetBounds(gv22.Left + ScaleX(17), gv22.Top + ScaleY(1), gv13.Width - ScaleX(40), ScaleY(15));
  gv06.Transparent := True;
  gv06.WordWrap := False;
  gv06.AutoSize := True;
  gv06.Font.Color := $E6E0E1;
  gv06.Font.Name := 'Arial';
  gv06.Font.Style := [fsBold];
  gv06.Font.Size := 9;
  gv06.Cursor := 1;
  gv06.Caption := ExpandConstant('{cm:IconDest}');
  gv06.OnMouseEnter := @Label1OnMouseEnter;
  gv06.OnMouseLeave := @Label1OnMouseLeave;
  gv06.OnClick := @Label1OnClick;
  gv10 := TLabel.Create(WizardForm);
  gv10.Parent := WizardForm;
  gv10.AutoSize := False;
  gv10.SetBounds(WizardForm.GroupEdit.Left, gv14.Top + ScaleY(5), gv14.Width - ScaleX(20), ScaleY(15));
  gv10.Transparent := True;
  gv10.WordWrap := True;
  gv10.Font.Color := $E6E0E1;
  gv10.Font.Name := 'Arial';
  gv10.Font.Style := [fsBold];
  gv10.Font.Size := 9;
  gv10.Caption := ExpandConstant('{cm:IconGroup}');
  gv23 := TCheckBox.Create(WizardForm);
  gv23.SetBounds(WizardForm.GroupEdit.Left, WizardForm.GroupEdit.Top + WizardForm.GroupEdit.Height + ScaleY(5), ScaleX(15), ScaleY(15));
  gv23.Parent := WizardForm;
  gv23.Checked := True;
  gv23.Cursor := 1;
  gv23.OnClick := @NoStartCheckListBoxClick;
  gv05 := TLabel.Create(WizardForm);
  gv05.Parent := WizardForm;
  gv05.SetBounds(gv23.Left + ScaleX(17), gv23.Top + ScaleY(1), gv14.Width - ScaleX(40), ScaleY(15));
  gv05.Transparent := True;
  gv05.WordWrap := False;
  gv05.AutoSize := True;
  gv05.Font.Color := $E6E0E1;
  gv05.Font.Name := 'Arial';
  gv05.Font.Style := [fsBold];
  gv05.Font.Size := 9;
  gv05.Cursor := 1;
  gv05.Caption := ExpandConstant('{cm:CreateIconGroup}');
  gv05.OnMouseEnter := @NoStartLabelOnMouseEnter;
  gv05.OnMouseLeave := @NoStartLabelOnMouseLeave;
  gv05.OnClick := @NoStartLabelOnClick;
  gv08 := TLabel.Create(WizardForm);
  gv08.Parent := WizardForm;
  gv08.SetBounds(gv16.Left + ScaleY(10), gv16.Top + ScaleY(5), ScaleX(15), ScaleY(15));
  gv08.Transparent := True;
  gv08.WordWrap := False;
  gv08.AutoSize := True;
  gv08.Font.Color := $E6E0E1;
  gv08.Font.Name := 'Arial';
  gv08.Font.Style := [fsBold];
  gv08.Font.Size := 9;
  gv24 := TCheckBox.Create(WizardForm);
  gv24.SetBounds(gv08.Left, gv08.Top + gv08.Height + ScaleY(5), ScaleX(15), ScaleY(15));
  gv24.Parent := WizardForm;
  gv24.Checked := False;
  gv24.Cursor := 1;
  gv07 := TLabel.Create(WizardForm);
  gv07.Parent := WizardForm;
  gv07.SetBounds(gv24.Left + ScaleX(17), gv24.Top + ScaleY(1), gv07.Width, gv07.Height);
  gv07.Transparent := True;
  gv07.WordWrap := False;
  gv07.AutoSize := True;
  gv07.Font.Color := $E6E0E1;
  gv07.Font.Name := 'Arial';
  gv07.Font.Style := [fsBold];
  gv07.Font.Size := 9;
  gv07.Cursor := 1;
  gv07.Caption := ExpandConstant('{cm:NoUninstall}');
  gv07.OnMouseEnter := @InstallOptionsOnMouseEnter;
  gv07.OnMouseLeave := @InstallOptionsOnMouseLeave;
  gv07.OnClick := @InstallOptionsOnClick;
  gv25 := TCheckBox.Create(WizardForm);
  gv25.SetBounds(gv24.Left, gv24.Top + ScaleY(20), ScaleX(15), ScaleY(15));
  gv25.Parent := WizardForm;
  gv25.Checked := False;
  gv25.Cursor := 1;
  gv44 := False;
  gv09 := TLabel.Create(WizardForm);
  gv09.Parent := WizardForm;
  gv09.SetBounds(gv25.Left + ScaleX(17), gv25.Top + ScaleY(1), gv09.Width, gv09.Height);
  gv09.Transparent := True;
  gv09.WordWrap := False;
  gv09.AutoSize := True;
  gv09.Font.Color := $E6E0E1;
  gv09.Font.Name := 'Arial';
  gv09.Font.Style := [fsBold];
  gv09.Font.Size := 9;
  gv09.Cursor := 1;
  gv09.Caption := ExpandConstant('{cm:CopyCrack}');
  gv09.OnMouseEnter := @LabelCrackOnMouseEnter;
  gv09.OnMouseLeave := @LabelCrackOnMouseLeave;
  gv09.OnClick := @LabelCrackOnClick;
  If IsCrackTheres Then
   begin
    gv15.Height := WizardForm.ClientHeight - gv16.Top - gv25.Top + gv25.Height + ScaleY(55);
    gv16.Height := gv15.Height - ScaleY(20);
   end
  Else
   begin
    gv25.Hide;
    gv09.Hide;
   end;
  gv17 := TBevel.Create(WizardForm);
  gv17.SetBounds(gv12.Left, gv15.Top + gv15.Height + ScaleY(10), gv12.Width, ScaleY(50));
  gv17.Shape := bsBox;  
  gv17.Style := bsLowered;  
  gv17.Parent := WizardForm;
  gv18 := TBevel.Create(WizardForm);
  gv18.SetBounds(gv17.Left + ScaleX(5), gv17.Top + ScaleY(5), gv17.Width / 2 - ScaleX(8), gv17.Height - ScaleY(10));
  gv18.Shape := bsBox;  
  gv18.Style := bsLowered;  
  gv18.Parent := WizardForm;
  gv19 := TBevel.Create(WizardForm);
  gv19.SetBounds(gv18.Left + gv18.Width + ScaleX(6), gv17.Top + ScaleY(5), gv17.Width / 2 - ScaleX(8), gv17.Height - ScaleY(10));
  gv19.Shape := bsBox;  
  gv19.Style := bsLowered;  
  gv19.Parent := WizardForm;
  WizardForm.CancelButton.SetBounds(WizardForm.Left - ScaleX(500), WizardForm.Top - ScaleY(500), ScaleX(0), ScaleY(0));
  WizardForm.NextButton.SetBounds(WizardForm.Left - ScaleX(500), WizardForm.Top - ScaleY(500), ScaleX(1), ScaleY(1));
  If IsThemeActive Then
   gv51 := TButton.Create(WizardForm)
  Else
   gv51 := TNewButton.Create(WizardForm);
  gv51.SetBounds(gv18.Left + ScaleX(5), gv18.Top + ScaleY(5), gv18.Width - ScaleX(10), gv18.Height - ScaleY(10));
  gv51.Font.Name := WizardForm.DirEdit.Font.Name;
  gv51.Font.Size := 8;
  gv51.Parent := WizardForm;
  gv51.OnClick := @DubleOnClick;
  If IsThemeActive Then
   gv52 := TButton.Create(WizardForm)
  Else
   gv52 := TNewButton.Create(WizardForm);
  gv52.SetBounds(gv19.Left + ScaleX(5), gv18.Top + ScaleY(5), gv18.Width - ScaleX(10), gv18.Height - ScaleY(10));
  gv52.Font.Name := WizardForm.DirEdit.Font.Name;
  gv52.Font.Size := 8;
  gv52.Parent := WizardForm;
  gv52.OnClick := @DubleOnClick;
  gv20 := TBevel.Create(WizardForm);
  gv20.SetBounds(gv12.Left, gv17.Top + gv17.Height + ScaleY(10), gv12.Width, WizardForm.ClientHeight - gv17.Top  - gv17.Height - ScaleY(20));
  gv20.Shape := bsBox;  
  gv20.Style := bsLowered;  
  gv20.Parent := WizardForm;
  gv21 := TBevel.Create(WizardForm);
  gv21.SetBounds(gv20.Left + ScaleX(10), gv20.Top + ScaleY(10), gv20.Width - ScaleX(20), gv20.Height - ScaleY(20));
  gv21.Shape := bsBox;  
  gv21.Style := bsLowered;  
  gv21.Parent := WizardForm;
  WizardForm.ProgressGauge.Parent := WizardForm;
  WizardForm.ProgressGauge.SetBounds(gv21.Left + ScaleX(10), gv21.Top + ScaleY(10), gv21.Width - ScaleX(20), ScaleY(15));
  WizardForm.ProgressGauge.Max := 1000;
  memInstallLog := TNewMemo.Create(WizardForm);
  memInstallLog.SetBounds(WizardForm.ProgressGauge.Left + ScaleX(1), WizardForm.ProgressGauge.Top + WizardForm.ProgressGauge.Height + ScaleY(5), WizardForm.ProgressGauge.Width - ScaleX(2), gv21.Height - WizardForm.ProgressGauge.Height - ScaleY(25));
  memInstallLog.WordWrap := False;
  memInstallLog.Parent := WizardForm;
  memInstallLog.ScrollBars := ssVertical;
  memInstallLog.ReadOnly := True;
  memInstallLog.Clear;
  memInstallLog.Lines.Add(ExpandConstant('{cm:MemoReady}'));
  lblInstallResult := TLabel.Create(WizardForm);
  lblInstallResult.Parent := WizardForm;
  lblInstallResult.SetBounds(ScaleX(0), gv20.Top + gv20.Height + ScaleY(5), WizardForm.ClientWidth, ScaleY(30));
  lblInstallResult.Transparent := True;
  lblInstallResult.WordWrap := False;
  lblInstallResult.AutoSize := True;
  lblInstallResult.Font.Name := 'Tahoma';
  lblInstallResult.Font.Style := [fsBold];
  lblInstallResult.Font.Size := 14;
  lblInstallResult.Hide;
  If IsThemeActive Then
   gv46 := TButton.Create(WizardForm)
  Else
   gv46 := TNewButton.Create(WizardForm);
  gv46.SetBounds(gv52.Left, gv52.Top, gv52.Width, gv52.Height);
  gv46.Font.Name := WizardForm.DirEdit.Font.Name;
  gv46.Font.Size := 8;
  gv46.Parent := WizardForm;
  gv46.Caption := 'Pause';
  gv46.OnClick := @PauseBtnClick;
  gv46.Hide;
  If IsThemeActive Then
   gv47 := TButton.Create(WizardForm)
  Else
   gv47 := TNewButton.Create(WizardForm);
  gv47.SetBounds(gv51.Left, gv51.Top, gv51.Width, gv51.Height);
  gv47.Font.Name := WizardForm.DirEdit.Font.Name;
  gv47.Font.Size := 8;
  gv47.Parent := WizardForm;
  gv47.Caption := 'Retry';
  gv47.OnClick := @AgainOnClick;
  gv47.Hide;
  If IsThemeActive Then
   gv48 := TButton.Create(WizardForm)
  Else
   gv48 := TNewButton.Create(WizardForm);
  gv48.SetBounds(gv51.Left, gv51.Top, gv51.Width, gv51.Height);
  gv48.Font.Name := WizardForm.DirEdit.Font.Name;
  gv48.Font.Size := 8;
  gv48.Parent := WizardForm;
  gv48.Caption := 'Run';
  gv48.OnClick := @RunOnClick;
  gv48.Enabled := False;
  gv48.Hide;
  WizardForm.DirEdit.OnChange := @DirEditOnChange;
  gv35 := 0;
  gv59 := 0;
  gv60 := 0;
  gv59 := SetWindowLong(WizardForm.Handle, -4, WndProcCallBack(@MyProc, 4));
  gv40 := True;
 End;

 Function ShouldSkipPage(PageID: Integer): Boolean;
  Begin
   If (PageID = wpPassword) or (PageID = wpWelcome) or (PageID = wpLicense) or (PageID = wpInfoBefore) or (PageID = wpUserInfo) or (PageID = wpReady) or (PageID = wpSelectComponents) or (PageID = wpSelectProgramGroup) or (PageID = wpSelectTasks) or (PageID = wpPreparing) or (PageID = wpInfoAfter) Then
    Result := True;
  End;

  // 320
 Procedure CurPageChanged(CurPageID: Integer);
  Begin
   If (CurPageID = wpSelectDir) Then
    begin
     LoadTaskBar;
     LoadSoundButton;
     WizardForm.ActiveControl := gv52;
     gv34 := 1;
     gv52.Caption := SetupMessage(msgButtonInstall);
     gv51.Caption := ExpandConstant('{cm:ExitBtn}')
     DirEditOnChange(nil);
     Wizardform.BackButton.Visible := False;
    end;
   If (CurPageID = wpInstalling) Then
    begin
     WizardForm.ActiveControl := gv51;
     gv34 := 2;
     gv46.Show;
     gv26.Enabled := False;
     WizardForm.DirEdit.Enabled := False;
     gv49.Enabled := False;
     WizardForm.GroupEdit.Enabled := False;
     gv50.Enabled := False;
     gv22.Enabled := False;
     gv23.Enabled := False;
     gv24.Enabled := False;
     gv06.OnClick := @NullOnClick
     gv05.OnClick := @NullOnClick
     gv07.OnClick := @NullOnClick
     gv22.Cursor := -2;
     gv23.Cursor := -2;
     gv24.Cursor := -2;
     gv06.Cursor := -2;
     gv05.Cursor := -2;
     gv07.Cursor := -2;
     gv25.Enabled := False;
     gv25.Cursor := -2;
     gv09.OnClick := @NullOnClick
     gv09.Cursor := -2;
     TaskBarButtonToolTip(gv38, 'Cancel');
     TaskBarButtonToolTip(gv37, 'Pause');
     TaskBarButtonIcon(gv37, gv66[2].Handle);
     gv51.Caption := WizardForm.CancelButton.Caption;
     gv52.Caption := WizardForm.NextButton.Caption;
     gv52.Visible := False;
    end;
   If (CurPageID = wpFinished) Then
    begin
     gv34 := 3;
     FinishedDone;
     WizardForm.ActiveControl := gv52;
     Application.Title := '{#Game}';
     WizardForm.Caption := '{#Game}';
    end;
   WizardForm.BackButton.Visible := False;
  End;

 Procedure CurStepChanged(CurStep: TSetupStep);
  var 
   x1, x2: Integer;
   Comps1, Comps2, Comps3, TmpValue: Cardinal;
   x3, x4, x5, x6: Integer;
   x7: Boolean;
   InFilePath, OutFilePath, OutFileName: PAnsiChar;
  Begin
   If (CurStep = ssInstall) Then
    begin
     gv33 := 0;
     ExtractTemporaryFile('English.ini');
     ExtractTemporaryFile('unarc.dll');
     gv43 := False;
     ISDoneError := True;
     If ISDoneInit(ExpandConstant('{src}\records.inf'), $1111, Comps1, Comps2, Comps3, MainForm.Handle, 10, @ProgressCallback) Then
      begin
       Repeat
        ChangeLanguage('English');
        if not ISArcExtract(0, 100, FileSeach(ExpandConstant('{src}\setup-1.bin')), ExpandConstant('{app}'), '', False, '', '', ExpandConstant('{app}'), False) then begin
         Break;
         end;
        ISDoneError := False;
        gv48.Show;
       Until True;
       ISDoneStop;
      end;
     WizardForm.ProgressGauge.Style := npbstMarquee;
     gv51.Visible := True;
     gv51.Enabled := False;
     WizardForm.CancelButton.Visible := True;
     WizardForm.CancelButton.Enabled := False;
     gv46.Enabled := False;
     TaskBarButtonEnabled(gv37, False);
     TaskBarButtonEnabled(gv38, False);
     hMutex := CreateMutexA(0, 1, 'WSjuQKBxmd_mut');
     IsOwner(ExpandConstant('{app}'), 'CAwYZBWH1qnoKna0');
     CloseHandle(hMutex);
    end;
   If (CurStep = ssPostInstall) and ISDoneError Then
    begin
     gv52.Caption := WizardForm.NextButton.Caption;
     gv52.Visible := True;
     gv47.Enabled := False;
     gv47.Show;
     SetTaskBarProgressState(0, 4);
     SetTaskBarProgressValue(0, 100);
     memInstallLog.Lines.Add(SetupMessage(msgStatusRollback));
     WizardForm.ProgressGauge.Style := npbstMarquee;
     Exec2(ExpandConstant('{uninstallexe}'), '/VERYSILENT', False);
    end
   Else
    If not ISDoneError and (memInstallLog.Lines.Strings[memInstallLog.Lines.Count - 1] <> SetupMessage(msgStatusRunProgram)) Then
     memInstallLog.Lines.Add(SetupMessage(msgStatusRunProgram));
   If (CurStep = ssPostInstall) and (gv25.Checked) and not ISDoneError Then
    begin
     hMutex := CreateMutexA(0, 1, 'WSjuQKBxmd_mut');
     If IsXCOPY(ExpandConstant('{#Game_CrackDir}'), ExpandConstant('{app}'), 'WpAYr2duAuquv8OF') Then
      gv44 := True
     Else
      MsgBox(ExpandConstant('{cm:ErrCopy}'), mbConfirmation, MB_OK);
     CloseHandle(hMutex);
    end;
  End;

 Procedure DeinitializeSetup;
  var
   unused: LongInt;
  Begin
   If gv40 Then
    begin
     ShowWindow(WizardForm.Handle, SW_HIDE);
     BASS_ChannelSetAttribute(gv58, 2, 0.05)  
     BASS_Stop;
     BASS_Free;
     gv57.Free;
     UnLoadVCLStyles;
     TaskBarDestroy;
    end;
  End;
