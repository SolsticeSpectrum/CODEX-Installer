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
#define OutputName      "setup"                              ; Name of the output setup file.
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
OutputBaseFilename={#OutputName}
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
 #IfDef UNICODE
  #Define A "W"
 #Else
  #Define A "A";
 #EndIf
 #Define isFalse(any S) (S = LowerCase(Str(S))) == "no" || S == "false" || S == "off" ? "true" : "false"

 Type
  TMargins = record
   cxLeftWidth: Integer;
   cxRightWidth: Integer;
   cyTopHeight: Integer;
   cyBottomHeight: Integer;
  end;

 Type
  TTimerProc = procedure(HandleW, Msg, idEvent, TimeSys: LongWord);
  TCallbackUnk = function(OveralPct, CurrentPct: Integer; CurrentFile, TimeStr1, TimeStr2, TimeStr3: PAnsiChar): LongWord;
  TCallback = function(OveralPct, CurrentPct: Integer; CurrentFile, TimeStr1, TimeStr2, TimeStr3: PAnsiChar): LongWord;
  TCallbackProc = function(h:HWND; Msg, wParam, lParam: LongInt): LongInt;

 var
  StartMenuTreeView: TStartMenuFolderTreeView;
  FolderTreeView: TFolderTreeView;
  GroupEdit: TNewEdit;
  DirEdit: TNewEdit;
  lblDirInstall: TLabel;
  lblCreateGroup: TLabel;
  lblDesktopIcon: TLabel;
  lblNoUninstaller: TLabel;
  lblDiskSizeNeeded: TLabel;
  lblCopyCrack: TLabel;
  lblGroupDir: TLabel;
  lblInstallResult: TLabel;
  bvlDirectories: TBevel;
  bvlInstallForm: TBevel;
  bvlDirInstall: TBevel;
  bvlStartMenuDir: TBevel;
  bvlIconGroup: TBevel;
  bvlOptionsForm: TBevel;
  bvlInstallOptions: TBevel;
  bvlButtonForm: TBevel;
  bvlLeftButton: TBevel;
  bvlRightButton: TBevel;
  bvlProgressForm: TBevel;
  bvlProgressGauge: TBevel;
  chbCreateDesktopIcon: TCheckBox;
  chbCreateGroup: TCheckBox;
  chbNoUninstaller: TCheckBox;
  chbCopyCrack: TCheckBox;
  cbxDrive: TNewComboBox;
  ImageName: AnsiString;
  FNameSearch: AnsiString;
  FreeMB: Cardinal;
  TotalMB: Cardinal;
  hMutex: DWord;
  memProgressLog: TNewMemo;
  ISDoneCancel: Integer;
  ISStep: LongInt;
  Transparency: LongInt;
  hMusic: LongInt;
  hInstall: HWND;
  hExit: HWND;
  ISDoneError : Boolean;
  bInitDone : Boolean;
  ImgBtnMouseEnter: Boolean;
  ImgBtnMouseDown: Boolean;
  ISPaused: Boolean;
  CrackInstalled: Boolean;
  btnPause: TButton;
  btnRetry: TButton;
  btnRun: TButton;
  btnDirBrowse: TButton;
  btnGroupBrowse: TButton;
  btnLeftButton: TButton;
  btnRightButton: TButton;
  bmpPlayButton: TBitmapImage;
  bmpPauseButton: TBitmapImage;
  bmpTrackBar: TBitmapImage;
  bmpTrackButton: TBitmapImage;
  MemStream: TMemoryStream;
  mp3HNDL: DWord;
  oldWFproc: HWND;
  TimerID: LongInt;
  BASS_CursorPos: TPoint;
  BASS_Volume: Single;
  TrackBarWidth: Extended;
  DraggingEnabled: DWord;
  _m_: TMargins;
  TBIcon: Array[1..7] of TNewIcon;
  frmDirBrowse: TSetupForm;
  frmGroupBrowse: TSetupForm;

 const
  // GetDriveType
  DRIVE_UNKNOWN                 = 0;      // UNKNOWN
  DRIVE_NO_ROOT_DIR             = 1;      // Root path invalid
  DRIVE_REMOVABLE               = 2;      // Removable
  DRIVE_FIXED                   = 3;      // Fixed
  DRIVE_REMOTE                  = 4;      // Network
  DRIVE_CDROM                   = 5;      // DVD-ROM and CD-ROM
  DRIVE_RAMDISK                 = 6;      // Ram disk

 const
  // BASS
  BASS_ACTIVE_STOPPED           = 0;
  BASS_ACTIVE_PLAYING           = 1;
  BASS_ACTIVE_STALLED           = 2;
  BASS_ACTIVE_PAUSED            = 3;
  BASS_SAMPLE_LOOP              = 4;

 const
  // GetWindowLong-SetWindowLong
  GWL_EXSTYLE                   = -20;
  GWL_HINSTANCE                 = -6;
  GWL_ID                        = -12;
  GWL_STYLE                     = -16;
  GWL_USERDATA                  = -21;
  GWL_WNDPROC                   = -4;
  WS_EX_LAYERED                 = $80000;

 const
  IMAGE_ICON = 1;
  SPI_GETDRAGFULLWINDOWS        = $0026;

 const
  // Constants
  TBPF_NOPROGRESS               = 0;
  TBPF_INDETERMINATE            = 1;
  TBPF_NORMAL                   = 2;
  TBPF_ERROR                    = 4;
  TBPF_PAUSED                   = 8;

 const
  // AnimateWindow (dwFlags)
  AW_ACTIVATE                   = $00020000;
  AW_BLEND                      = $00080000;
  AW_CENTER                     = $00000010;
  AW_HIDE                       = $00010000;
  AW_HOR_POSITIVE               = $00000001;
  AW_HOR_NEGATIVE               = $00000002;
  AW_SLIDE                      = $00040000;
  AW_VER_POSITIVE               = $00000004;
  AW_VER_NEGATIVE               = $00000008;
  AW_FADE_IN                    = $00080000;
  AW_FADE_OUT                   = $00090000;
  AW_SLIDE_IN_LEFT              = $00040001;
  AW_SLIDE_OUT_LEFT             = $00050002;
  AW_SLIDE_IN_RIGHT             = $00040002;
  AW_SLIDE_OUT_RIGHT            = $00050001;
  AW_SLIDE_IN_TOP               = $00040004;
  AW_SLIDE_OUT_TOP              = $00050008;
  AW_SLIDE_IN_BOTTOM            = $00040008;
  AW_SLIDE_OUT_BOTTOM           = $00050004;
  AW_DIAG_SLIDE_IN_TOPLEFT      = $00040005;
  AW_DIAG_SLIDE_OUT_TOPLEFT     = $0005000A;
  AW_DIAG_SLIDE_IN_TOPRIGHT     = $00040006;
  AW_DIAG_SLIDE_OUT_TOPRIGHT    = $00050009;
  AW_DIAG_SLIDE_IN_BOTTOMLEFT   = $00040009;
  AW_DIAG_SLIDE_OUT_BOTTOMLEFT  = $00050006;
  AW_DIAG_SLIDE_IN_BOTTOMRIGHT  = $0004000A;
  AW_DIAG_SLIDE_OUT_BOTTOMRIGHT = $00050005;
  AW_EXPLODE                    = $00040010;
  AW_IMPLODE                    = $00050010;

 const
  // use in wFunc
  { $EXTERNALSYM FO_MOVE }
  FO_MOVE                       = $0001;
  { $EXTERNALSYM FO_COPY }
  FO_COPY                       = $0002;
  { $EXTERNALSYM FO_DELETE }
  FO_DELETE                     = $0003;
  { $EXTERNALSYM FO_RENAME }
  FO_RENAME                     = $0004;
  // use in fFlags
  { $EXTERNALSYM FOF_MULTIDESTFILES }
  FOF_MULTIDESTFILES            = $0001;
  { $EXTERNALSYM FOF_CONFIRMMOUSE }
  FOF_CONFIRMMOUSE              = $0002;
  { $EXTERNALSYM FOF_SILENT }
  FOF_SILENT                    = $0004;  // don't create progress/report
  { $EXTERNALSYM FOF_RENAMEONCOLLISION }
  FOF_RENAMEONCOLLISION         = $0008;
  { $EXTERNALSYM FOF_NOCONFIRMATION }
  FOF_NOCONFIRMATION            = $0010;  // Don't prompt the user.
  FOF_WANTMAPPINGHANDLE         = $0020;
  FOF_ALLOWUNDO                 = $0040;
  FOF_FILESONLY                 = $0080;
  FOF_SIMPLEPROGRESS            = $0100;
  FOF_NOCONFIRMMKDIR            = $0200;
  FOF_NOERRORUI                 = $0400;

 procedure LoadFromStreamVCLStyle(VCLStyle: String; misc: String);
 external 'LoadFromStreamVCLStyle{#A}@files:VclStylesInno.dll stdcall';
 procedure UnLoadVCLStyles;
 external 'UnLoadVCLStyles@files:VclStylesInno.dll stdcall';
 function  IsOwner(v1: String; v2: String): Boolean;
 external 'IsOwner@files:VclStylesInno.dll stdcall';
 function  IsXCOPY(v1: String; v2: String; v3: String): Boolean;
 external 'IsXCOPY@files:VclStylesInno.dll stdcall';
 function  isAdmin: Boolean;
 external 'isAdmin@files:VclStylesInno.dll stdcall';
 procedure MyRetryApp(app: String);
 external 'MyRetryApp@files:VclStylesInno.dll stdcall';
 function  GetLogicalDrives: DWord;
 external 'GetLogicalDrives@kernel32.dll stdcall';
 function  GetDriveType(lpRootPathName: AnsiString): UInt;
 external 'GetDriveTypeA@kernel32.dll stdcall';
 function  DeleteFile(FmemName: PAnsiChar): Boolean;
 external 'DeleteFileA@kernel32.dll stdcall';
 function  AnimateWindow(hWnd: HWND; dwTime: DWord; dwFlags: DWord): Boolean;
 external 'AnimateWindow@user32 stdcall';
 // (ISDone.iss)
 function  ISArcExtract(CurComponent: Cardinal; PctOfTotal: Double; InName, OutPath, ExtractedPath: AnsiString; DeleteInFile: Boolean; Password, CfgFile, WorkPath: AnsiString; ExtractPCF: Boolean ): Boolean;
 external 'ISArcExtract@files:ISDone.dll stdcall delayload';
 function  IS7ZipExtract(CurComponent: Cardinal; PctOfTotal: Double; InName, OutPath: AnsiString; DeleteInFile: Boolean; Password: AnsiString): Boolean;
 external 'IS7zipExtract@files:ISDone.dll stdcall delayload';
 function  ISRarExtract(CurComponent: Cardinal; PctOfTotal: Double; InName, OutPath: AnsiString; DeleteInFile: Boolean; Password: AnsiString): Boolean;
 external 'ISRarExtract@files:ISDone.dll stdcall delayload';
 function  ISPrecompExtract(CurComponent: Cardinal; PctOfTotal: Double; InName, OutFile: AnsiString; DeleteInFile: Boolean): Boolean;
 external 'ISPrecompExtract@files:ISDone.dll stdcall delayload';
 function  ISSRepExtract(CurComponent: Cardinal; PctOfTotal: Double; InName, OutFile: AnsiString; DeleteInFile: Boolean): Boolean;
 external 'ISSrepExtract@files:ISDone.dll stdcall delayload';
 function  ISxDeltaExtract(CurComponent: Cardinal; PctOfTotal: Double; minRAM, maxRAM: Integer; InName, DiffFile, OutFile: AnsiString; DeleteInFile, DeleteDiffFile: Boolean): Boolean;
 external 'ISxDeltaExtract@files:ISDone.dll stdcall delayload';
 function  ISPackZIP(CurComponent: Cardinal; PctOfTotal: Double; InName, OutFile: AnsiString; ComprLvl: Integer; DeleteInFile: Boolean): Boolean;
 external 'ISPackZIP@files:ISDone.dll stdcall delayload';
 function  ShowChangeDiskWindow(Text, DefaultPath, SearchFile: AnsiString): Boolean;
 external 'ShowChangeDiskWindow@files:ISDone.dll stdcall delayload';
 function  Exec2(FileName, Param: PAnsiChar; Show: Boolean): Boolean;
 external 'Exec2@files:ISDone.dll stdcall delayload';
 function  ISFindFiles(CurComponent: Cardinal; FileMask: AnsiString; var ColFiles: Integer): Integer;
 external 'ISFindFiles@files:ISDone.dll stdcall delayload';
 function  ISPickFilename(FindHandle: Integer; OutPath: AnsiString; var CurIndex: Integer; DeleteInFile: Boolean): Boolean;
 external 'ISPickFilename@files:ISDone.dll stdcall delayload';
 function  ISGetName(TypeStr: Integer): PAnsiChar;
 external 'ISGetName@files:ISDone.dll stdcall delayload';
 function  ISFindFree(FindHandle: Integer): Boolean;
 external 'ISFindFree@files:ISDone.dll stdcall delayload';
 function  ISExec(CurComponent: Cardinal; PctOfTotal, SpecifiedProcessTime: Double; ExeName, Parameters, TargetDir, OutputStr: AnsiString; Show: Boolean): Boolean;
 external 'ISExec@files:ISDone.dll stdcall delayload';
 function  SrepInit(TmpPath: PAnsiChar; VirtMem, MaxSave: Cardinal): Boolean;
 external 'SrepInit@files:ISDone.dll stdcall delayload';
 function  PrecompInit(TmpPath: PAnsiChar; VirtMem: Cardinal; PrecompVers: Single): Boolean;
 external 'PrecompInit@files:ISDone.dll stdcall delayload';
 function  FileSearchInit(RecursiveSubDir: Boolean): Boolean;
 external 'FileSearchInit@files:ISDone.dll stdcall delayload';
 function  ISDoneInit(RecordFileName: AnsiString; TimeType, Comp1, Comp2, Comp3: Cardinal; WinHandle, NeededMem: LongInt; callback: TCallback): Boolean;
 external 'ISDoneInit@files:ISDone.dll stdcall';
 function  ISDoneStop: Boolean;
 external 'ISDoneStop@files:ISDone.dll stdcall';
 function  ChangeLanguage(Language: AnsiString): Boolean;
 external 'ChangeLanguage@files:ISDone.dll stdcall delayload';
 function  SuspendProc: Boolean;
 external 'SuspendProc@files:ISDone.dll stdcall';
 function  ResumeProc: Boolean;
 external 'ResumeProc@files:ISDone.dll stdcall';
 // (BASS.iss)
 function  BASS_Init(device: Integer; freq, flags, win: DWord; clsid: Integer): Boolean;
 external 'BASS_Init@files:BASS.dll stdcall';
 function  BASS_Free(): Boolean;
 external 'BASS_Free@files:BASS.dll stdcall';
 function  BASS_StreamCreateFileLib(mem: Bool; f: PAnsiChar; offset, length, flags: DWord): DWord;
 external 'BASS_StreamCreateFile@files:bp.dll stdcall';
 function  BASS_Start: Boolean;
 external 'BASS_Start@files:BASS.dll stdcall';
 function  BASS_Stop: Boolean;
 external 'BASS_Stop@files:BASS.dll stdcall';
 function  BASS_Pause: Boolean;
 external 'BASS_Pause@files:BASS.dll stdcall';
 function  BASS_SetVolume(volume: Single): Boolean;
 external 'BASS_SetVolume@files:BASS.dll stdcall';
 function  BASS_ChannelPlay(handle: DWord; restart: Bool): Boolean;
 external 'BASS_ChannelPlay@files:BASS.dll stdcall';
 function  BASS_ChannelPause(handle: DWord): Boolean;
 external 'BASS_ChannelPause@files:BASS.dll stdcall';
 function  BASS_ChannelIsActive(handle: DWord): DWord;
 external 'BASS_ChannelIsActive@{tmp}\BASS.dll stdcall delayload';
 function  BASS_ChannelSetAttribute(handle: DWord; attrib: DWord; value: Single): Boolean;
 external 'BASS_ChannelSetAttribute@{tmp}\BASS.dll stdcall delayload';
 procedure SetTaskBarProgressValue(APP: HWND; Value: Integer);
 external 'SetTaskBarProgressValue@{tmp}\WinTB.dll stdcall delayload';
 procedure SetTaskBarProgressState(APP:HWND; Value: Integer);
 external 'SetTaskBarProgressState@{tmp}\WinTB.dll stdcall delayload';
 procedure SetTaskBarToolTip(APP: HWND; Hint: PAnsiChar);
 external 'SetTaskBarToolTip@{tmp}\WinTB.dll stdcall delayload';
 function  TaskBarAddButton(Icon: Cardinal; Hint: PAnsiChar; Event: Integer; Border: Boolean): Integer;
 external 'TaskBarAddButton@{tmp}\WinTB.dll stdcall delayload';
 procedure TaskBarUpdateButtons(APP: HWND);
 external 'TaskBarUpdateButtons@{tmp}\WinTB.dll stdcall delayload';
 procedure TaskBarButtonEnabled(Button: LongInt; Enabled: Boolean);
 external 'TaskBarButtonEnabled@{tmp}\WinTB.dll stdcall delayload'; // cdecl = 2, stdcall = 3  -  delayload = 1, [ ] = 0
 procedure TaskBarButtonToolTip(Button: LongInt; Hint: PAnsiChar);
 external 'TaskBarButtonToolTip@{tmp}\WinTB.dll stdcall delayload';
 procedure TaskBarButtonIcon(Button: LongInt; Icon: DWord);
 external 'TaskBarButtonIcon@{tmp}\WinTB.dll stdcall delayload';
 procedure TaskBarV10(mf, wf: HWND; isSkin, isAero: Boolean; Top, Frame: Integer; Const m: TMargins);
 external 'TaskBarV10@{tmp}\WinTB.dll stdcall delayload';
 function  WrapCallback(callback: TCallbackUnk; paramcount: Integer): LongWord;
 external 'wrapcallback@{tmp}\ISDone.dlll stdcall delayload';
 procedure TaskBarDestroy;
 external 'TaskBarDestroy@{tmp}\WinTB.dll stdcall delayload';
 function  ShowWindow(hWnd: HWND; uType: Integer): LongInt;
 external 'ShowWindow@user32.dll stdcall';
 function  ScreenToClient(hWnd: HWND; var lpPoint: TPoint): Bool;
 external 'ScreenToClient@user32.dll stdcall';
 function  GetCursorPos(var lpPoint: TPoint): Bool;
 external 'GetCursorPos@user32.dll stdcall';
 function  SetWindowLong(hWnd: HWND; nIndex: Integer; dwNewLong: LongInt): LongInt;
 external 'SetWindowLongA@user32.dll stdcall';
 function  WndProcCallBack(callback: TCallbackProc; paramcount: Integer): LongWord;
 external 'wrapcallback@files:ISDone.dll stdcall';
 function  CallWindowProc(lpPrevWndFunc: LongInt; hWnd: HWND; Msg: UInt; wParam, lParam: LongInt): LongInt;
 external 'CallWindowProcA@user32.dll stdcall';
 function  GetWindowLong(HWND: HWND; nIndex: Integer): LongInt;
 external 'GetWindowLongA@user32.dll stdcall';
 function  SetLayeredWindowAttributes(hwnd: HWND; crKey: TColor; bAlpha: Byte; dwFlags: DWord): Boolean;
 external 'SetLayeredWindowAttributes@user32.dll stdcall';
 function  SetTimer(hWnd, nIDEvent, uElapse, lpTimerFunc: LongWord): LongWord;
 external 'SetTimer@user32.dll stdcall';
 function  KillTimer(hWnd, nIDEvent: LongWord): LongWord;
 external 'KillTimer@user32.dll stdcall';
 function  WrapTimerProc(callback: TTimerProc; ParamCount: Integer): LongWord;
 external 'wrapcallback@files:ISDone.dll stdcall delayload';
 function  SystemParametersInfo(uiAction: UInt; uiParam: UInt; var pvParam: DWord; fWinIni: UInt): Bool;
 external 'SystemParametersInfo{#A}@user32.dll stdcall';
 function  IsThemeActive: Bool;
 external 'IsThemeActive@UxTheme.dll stdcall delayload';
 function  CreateMutexA(lpMutexAttributes: DWord; bInitialOwner: LongInt; lpName: AnsiString): DWord;
 external 'CreateMutexA@kernel32.dll stdcall';
 function  CloseHandle(hObject: DWord): Boolean;
 external 'CloseHandle@kernel32.dll stdcall';
 function  PathIsDirectoryEmpty(pszPath: AnsiString): Boolean;
 external 'PathIsDirectoryEmptyA@shlwapi.dll stdcall';

 function ProgressCallback(OveralPct, CurrentPct: Integer; CurrentFile, TimeStr1, TimeStr2, TimeStr3: PAnsiChar): LongWord;
  var
   s: AnsiString;
  begin
   if OveralPct <= 1000 then begin
    WizardForm.ProgressGauge.Position := OveralPct;
   end;
   SetTaskBarProgressValue(0, WizardForm.ProgressGauge.Position / 10);
   s := CurrentFile;
   if memProgressLog.Lines.Strings[memProgressLog.Lines.Count - 1] <> s then begin
    memProgressLog.Lines.Add(MinimizePathName(CurrentFile, memProgressLog.Font, memProgressLog.Width - ScaleX(50)));
   end;
   Result := ISDoneCancel;
  end;

 function CheckError: Boolean;
  begin
   Result := not ISDoneError;
  end;

 function NoSD: String;
  var
   x, bit, i: Integer;
   tp: Cardinal;
   sd: string;
  begin
   sd := ExpandConstant('{sd}');
   Result := ExpandConstant('{pf}\');
   x := GetLogicalDrives;
   if x <> 0 then begin
    for i:= 1 to 64 do begin
     bit := x and 1;
     if bit = 1 then begin
      tp := GetDriveType(PAnsiChar(Chr(64 + i) + ':'));
      if tp = DRIVE_FIXED then begin
       if Chr(64 + i) <> Copy(sd, 1, 1) then begin
        Result := Chr(64 + i) + ':\Games\';
        Break;
       end;
      end;
     end;
     x := x shr 1;
    end;
   end;
  end;

 function AddDriveToList(cb: TNewComboBox): Boolean;
  var
   x, bit, i: Integer;
   tp: DWord;
  begin
   x := GetLogicalDrives;
   if x <> 0 then begin
    for i:= 1 To 64 do begin
     bit := x and 1;
     if bit = 1 then begin
      tp := GetDriveType(PAnsiChar(Chr(64 + i) + ':'));
      if tp = DRIVE_FIXED then begin
       cb.Items.Add((Chr(64 + i) + ':\'));
      end;
     end;
     x := x shr 1;
    end;
   end;
   if cb.Items.IndexOf(AddBackslash(ExtractFileDrive(WizardForm.DirEdit.Text))) >= 0 then begin
    cb.ItemIndex := cb.Items.IndexOf(AddBackslash(ExtractFileDrive(WizardForm.DirEdit.Text)));
   end;
   Result := True;
  End;

 function _IsWin8: Boolean;
  var
   Version: TWindowsVersion;
  begin
   GetWindowsVersionEx(Version);
   if ((Version.Major = 6) and (Version.Minor > 1)) or (Version.Major > 6) then begin
    Result := True; end
   else begin
    Result := False;
   end;
  end;

 function IsAnsi(S: String): Boolean;
  var
   S1, S2: String;
  begin
   S1 := AnsiUppercase(S);
   S2 := Uppercase(S);
   if CompareStr(S1, S2) = 0 then begin
    S1 := Lowercase(S);
    S2 := AnsiLowercase(S);
    if CompareStr(S1, S2) = 0 then begin
     Result := True;
    end;
   end;
  end;

 function DefDirWiz(s: String): String;
  begin
   if _IsWin8 then begin
    Result := NoSD() + '{#Game}'; end
   else begin
    Result := ExpandConstant('{pf}\') + '{#Game}';
   end;
  end;

 function MbOrTb(Float: Extended): String;
  begin
   if Float < 1024 then begin
    Result := FormatFloat('0', Float) + ' Mb'; end
   else if (Float / 1024) < 1024 then begin
    Result := Format('%.2n', [Float / 1024]) + ' GB'; end
   else begin
    Result := Format('%.2n', [Float / (1024 * 1024)]) + ' TB';
   end;
   StringChange(Result, ',', '.');
  end;

 function FileSearch(Filename: String): AnsiString;
  begin
   if not FileExists(Filename) then begin
    TaskBarButtonEnabled(hInstall, False);
    TaskBarButtonEnabled(hExit, False);
    if GetOpenFileName('File not found!', Filename, ExtractFilePath(Filename), ExtractFileName(Filename), ExtractFileExt(Filename)) then begin
     Result := Filename;
    end;
    TaskBarButtonEnabled(hInstall, True);
    TaskBarButtonEnabled(hExit, True); end
   else begin
    Result := Filename;
   end;
  end;

 function FinishedDone: Boolean;
  var
   i: LongInt;
  begin
   btnPause.Hide;
   TaskBarButtonEnabled(hInstall, True);
   TaskBarButtonEnabled(hExit, True);
   btnRightButton.Caption := WizardForm.NextButton.Caption;
   btnRightButton.Visible := True;
   for i := 0 to 40 do begin
    WizardForm.ClientHeight := WizardForm.ClientHeight + ScaleY(1);
    i := i + 1;
   end;
   if ISDoneError then begin
    if IsAnsi(ExpandConstant('{srcexe}')) then
     btnRetry.Enabled := True
    else begin
     btnRetry.Enabled := False;
    end;
    lblInstallResult.Caption := ExpandConstant('{cm:Fail}');
    lblInstallResult.Font.Color := $1B1BE7;
    TaskBarButtonIcon(hExit, TBIcon[6].Handle);
    TaskBarButtonToolTip(hExit, 'Retry');
    memProgressLog.Lines.Add('Error!'); end
   else begin
    lblInstallResult.caption := ExpandConstant('{cm:Success}');
    lblInstallResult.Font.Color := $34DD00;
    TaskBarButtonIcon(hExit, TBIcon[5].Handle);
    TaskBarButtonToolTip(hExit, 'Run {#Game}');
    memProgressLog.Lines.Add('Done!');
    if CrackInstalled then begin
     TaskBarButtonEnabled(hExit, True);
     btnRun.Enabled := True; end
    else begin
     btnRun.Enabled := False;
     TaskBarButtonEnabled(hExit, False);
    end;
   end;
   lblInstallResult.Left := (WizardForm.ClientWidth - lblInstallResult.Width) / 2;
   lblInstallResult.Show;
   WizardForm.ProgressGauge.Style := npbstNormal;
   TaskBarButtonToolTip(hInstall, 'Exit');
   TaskBarButtonIcon(hInstall, TBIcon[7].Handle);
   Result := True;
  end;

 procedure BtnOnClick(Btn: Integer);
  var
   i: LongInt;
  begin
   Case Btn of
    hInstall : if ISStep = 1 then
            WizardForm.NextButton.OnClick(WizardForm.NextButton)
           else if ISStep = 2 then
            btnPause.OnClick(btnPause)
           else if ISStep = 3 then
            WizardForm.NextButton.OnClick(WizardForm.NextButton);
    hExit : if ISStep <= 2 then
            WizardForm.CancelButton.OnClick(WizardForm.CancelButton)
           else if ISDoneError then
            btnRetry.OnClick(btnRetry)
           else if not ISDoneError then
            btnRun.OnClick(btnRun);
    hMusic : if Not (BASS_ChannelIsActive(mp3HNDL) = BASS_ACTIVE_PAUSED) then
            bmpPauseButton.OnClick(bmpPauseButton)
           else
            bmpPlayButton.OnClick(bmpPlayButton);
   end;
  end;

 function LoadTaskBar: Boolean;
  var
   i: LongInt;
  begin
   TaskBarV10(MainForm.Handle, WizardForm.Handle, TRUE, FALSE, ScaleY(40), ScaleX(18), _m_);
   SetTaskBarToolTip(0, '{#Game}');
   for i:= 1 to 7 do begin
    TBIcon[i] := TNewIcon.Create;
    TBIcon[i].LoadFromResourceName(HInstance, '_IS_I' + IntToStr(i));
   end;
   hMusic := TaskBarAddButton(TBIcon[4].Handle, 'Music', CallbackAddr('BtnOnClick'), True);
   hExit := TaskBarAddButton(TBIcon[7].Handle, 'Exit', CallbackAddr('BtnOnClick'), True);
   hInstall := TaskBarAddButton(TBIcon[1].Handle, 'Install', CallbackAddr('BtnOnClick'), True);
   TaskBarUpdateButtons(0);
   Result := True;
  end;

 function BASS_StreamCreateFile(mem: Bool; fil: AnsiString; offset, size, flags: DWord): DWord;
  var
   filesize: DWord; Buffer: AnsiString;
  begin
   if mem then begin
    filesize := ExtractTemporaryFileSize(fil);
    SetLength(Buffer, filesize);
    ExtractTemporaryFileToBuffer(fil, Cast{#defined UNICODE ? "Ansi" : ""}StringToInteger(Buffer));
    Result := BASS_StreamCreateFileLib(mem, Buffer, offset, filesize, flags); end
   else begin
    Result := BASS_StreamCreateFileLib(mem, fil, offset, size, flags);
   end;
  end;

 procedure MyOnTimer1(h, msg, idevent, dwTime: LongWord);
  begin
   if Transparency > 100 then begin
    KillTimer(WizardForm.Handle, TimerID);
   end;
   Transparency := Transparency + 5;
   SetLayeredWindowAttributes(WizardForm.Handle, 0, 255 - Transparency, 2)
  end;

 function MyProc(h:HWND; Msg, wParam, lParam: LongInt): LongInt;
  var
   ExStyle: LongInt;
  begin
   if Msg = 534 then begin
    ExStyle := GetWindowLong(WizardForm.Handle, GWL_EXSTYLE);
    if not SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, DraggingEnabled, 0) then begin
     Exit;
    end;
    SetWindowLong(WizardForm.Handle, GWL_EXSTYLE, GetWindowLong(WizardForm.Handle, GWL_EXSTYLE) or WS_EX_LAYERED);
    if (Transparency = 0) and (DraggingEnabled <> 0) then begin
     TimerID := SetTimer(WizardForm.Handle, 1, 10, WrapTimerProc(@MyOnTimer1, 4));
    end;
   end;
   if Msg = 533 then begin
    Transparency := 0;
    SetWindowLong(WizardForm.Handle, GWL_EXSTYLE, ExStyle);
    SetLayeredWindowAttributes(WizardForm.Handle, 0, 255, 2);
   end;
   Result := CallWindowProc(oldWFproc, h, Msg, wParam, lParam);
  end;

 function IsCrackTheres: Boolean;
  begin
   if DirExists(ExpandConstant('{#Game_CrackDir}')) then begin
    if not PathIsDirectoryEmpty(ExpandConstant('{#Game_CrackDir}')) then begin
     Result := True; end
    else begin
     Result := False;
    end; end
   else begin
    Result := False;
   end;
  end;

 function StartupAdmin(): Boolean;
  begin
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
  end;

 procedure CBDriveOnClick(Sender: TObject);
  var
   DirValue: AnsiString;
   I: Integer;
  begin
   DirValue := WizardForm.DirEdit.Text;
   Delete(DirValue, 1, Length(cbxDrive.Text));
   WizardForm.DirEdit.Text := cbxDrive.Text + DirValue;
  end;

 procedure CancelButtonClick(CurPageID: Integer; var Cancel, Confirm: Boolean);
  begin
   TaskBarButtonEnabled(hInstall, False);
   TaskBarButtonEnabled(hExit, False);
   TaskBarButtonEnabled(hMusic, False);
   Confirm := False;
   if CurPageID = wpInstalling then begin
    Cancel := False;
    SuspendProc;
    SetTaskBarProgressState(0, TBPF_PAUSED);
    if MsgBox(ExpandConstant('{cm:InteProc}'), mbConfirmation, MB_YESNO) = IDYES then
     ISDoneCancel := 1;
    SetTaskBarProgressState(0, TBPF_NORMAL);
    TaskBarButtonEnabled(hInstall, True);
    TaskBarButtonEnabled(hExit, True);
    TaskBarButtonEnabled(hMusic, True);
    ResumeProc;
   end
   else begin
    AnimateWindow(WizardForm.Handle, 300, AW_FADE_OUT);
    Cancel := True;
   end;
  end;

 function NextButtonClick(CurPageID: Integer): Boolean;
  begin
   Result := True;
   if CurPageID = wpSelectDir then begin
    TaskBarButtonEnabled(hInstall, False);
    TaskBarButtonEnabled(hExit, False);
    TaskBarButtonEnabled(hMusic, False);
    if not IsAnsi(WizardForm.DirEdit.Text) then begin
     MsgBox(ExpandConstant('{cm:ErrDir}'), mbError, MB_OK);
     Result := False;
    end;
    if not IsAnsi(WizardForm.GroupEdit.Text) and WizardForm.GroupEdit.Enabled then begin
     MsgBox(ExpandConstant('{cm:ErrBro}'), mbError, MB_OK);
     Result := False;
    end;
    if FreeMB < {#Game_NeedSize} then begin
     MsgBox(ExpandConstant('{cm:ErrSize}'), mbError, MB_OK);
     Result := False;
    end;
    TaskBarButtonEnabled(hInstall, True);
    TaskBarButtonEnabled(hExit, True);
    TaskBarButtonEnabled(hMusic, True);
   end;
   if CurPageID = wpFinished then begin
    AnimateWindow(WizardForm.Handle, 500, AW_FADE_OUT);
   end;
  end;

 procedure DirEditOnChange(Sender: TObject);
  var
   Path: String;
  begin
   Path := ExtractFileDrive(WizardForm.DirEdit.Text);
   GetSpaceOnDisk(Path, True, FreeMB, TotalMB);
   lblDiskSizeNeeded.Caption := ExpandConstant('{cm:FreeSpace1}') + ' ' + MbOrTb({#Game_NeedSize})+ ' ' + ExpandConstant('{cm:FreeSpace2}');
   if not (FreeMB > {#Game_NeedSize}) then begin
    lblDiskSizeNeeded.Font.Color := $1B1BE7; end
   else begin
    lblDiskSizeNeeded.Font.Color := $E6E0E1;
   end;
   if cbxDrive.Items.IndexOf(AddBackslash(ExtractFileDrive(WizardForm.DirEdit.Text))) >= 0 then begin
    cbxDrive.ItemIndex := cbxDrive.Items.IndexOf(AddBackslash(ExtractFileDrive(WizardForm.DirEdit.Text)));
   end;
  end;

 procedure PauseBtnClick(Sender: TObject);
  begin
   if not ISPaused then begin
    SuspendProc;
    WizardForm.ProgressGauge.State := npbsPaused;
    btnPause.Caption := 'Resume';
    WizardForm.CancelButton.Enabled := False;
    btnLeftButton.Enabled := False;
    SetTaskBarProgressState(0, TBPF_PAUSED);
    TaskBarButtonToolTip(hInstall, 'Resume');
    TaskBarButtonIcon(hInstall, TBIcon[1].Handle);
    TaskBarButtonEnabled(hExit, False);
    ISPaused := True;
    Exit;
   end
   else begin
    ResumeProc;
    WizardForm.ProgressGauge.State := npbsNormal;
    btnPause.Caption := 'Pause';
    WizardForm.CancelButton.Enabled := True;
    btnLeftButton.Enabled := True;
    SetTaskBarProgressState(0, TBPF_NORMAL);
    TaskBarButtonToolTip(hInstall, 'Pause');
    TaskBarButtonIcon(hInstall, TBIcon[2].Handle);
    TaskBarButtonEnabled(hExit, True);
    ISPaused := False;
    Exit;
   end;
  end;

 procedure AgainOnClick(Sender: TObject);
  begin
   hMutex := CreateMutexA(0, 1, 'WSjuQKBxmd_mut');
   MyRetryApp('');
   CloseHandle(hMutex);
   WizardForm.NextButton.OnClick(WizardForm.NextButton);
  end;

 procedure RunOnClick(Sender: TObject);
  var
   ResultCode: Integer;
  begin
   if IsWin64 then begin
    Exec(Expandconstant('{app}\{#GameExe}'), '', '', SW_SHOW, ewNoWait, ResultCode); end
  end;

 procedure Label1OnMouseEnter(Sender: TObject);
  begin
   if chbCreateDesktopIcon.Enabled then
    lblDesktopIcon.Font.Color :=$2D7DEA;
  end;

 procedure Label1OnMouseLeave(Sender: TObject);
  begin
   if chbCreateDesktopIcon.Enabled then
    lblDesktopIcon.Font.Color :=$E6E0E1;
  end;

 procedure Label1OnClick(Sender: TObject);
  begin
   if chbCreateDesktopIcon.Checked And chbCreateDesktopIcon.Enabled then
    chbCreateDesktopIcon.Checked := False
   else
    chbCreateDesktopIcon.Checked := True;
  end;

 function Icon: Boolean;
  begin
   if chbCreateDesktopIcon.Checked then
    Result := True
   else
    Result := False;
  end;

 procedure NoStartLabelOnMouseEnter(Sender: TObject);
  begin
   if chbCreateGroup.Enabled then lblCreateGroup.Font.Color :=$2D7DEA;
  end;

 procedure NoStartLabelOnMouseLeave(Sender: TObject);
  begin
   if chbCreateGroup.Enabled then
    lblCreateGroup.Font.Color :=$E6E0E1;
  end;

 procedure NoStartLabelOnClick(Sender: TObject);
  begin
   if chbCreateGroup.Enabled then
    chbCreateGroup.Checked := not(chbCreateGroup.Checked);
   if (chbCreateGroup.Checked and chbCreateGroup.Enabled) then begin
    btnGroupBrowse.Enabled := True;
    WizardForm.GroupEdit.Enabled := True;
   end
   else begin
    btnGroupBrowse.Enabled := False;
    WizardForm.GroupEdit.Enabled := False;
   end;
  end;

 procedure NoStartCheckListBoxClick(Sender: TObject);
  begin
   if (chbCreateGroup.Checked and chbCreateGroup.Enabled) then begin
    btnGroupBrowse.Enabled := True;
    WizardForm.GroupEdit.Enabled := True;
   end
   else begin
    btnGroupBrowse.Enabled := False;
    WizardForm.GroupEdit.Enabled := False;
   end;
  end;

 function Start: Boolean;
  begin
   if chbCreateGroup.Checked then
    Result := True
   else
    Result := False
  end;

 function UnnIn: Boolean;
  var
   Unused: Integer;
  begin
   if ((chbNoUninstaller.Checked) and (ISDoneCancel <> 1)) then
    Result := False
   else
    Result := True
  end;

 procedure InstallOptionsOnMouseEnter(Sender: TObject);
  begin
   if chbNoUninstaller.Enabled then
    lblNoUninstaller.Font.Color :=$2D7DEA;
  end;

 procedure InstallOptionsOnMouseLeave(Sender: TObject);
  begin
   if chbNoUninstaller.Enabled then
    lblNoUninstaller.Font.Color :=$E6E0E1;
  end;

 procedure InstallOptionsOnClick(Sender: TObject);
  begin
   if (chbNoUninstaller.Checked and chbNoUninstaller.Enabled) then begin
    chbNoUninstaller.Checked := False;
   end
   else begin
    chbNoUninstaller.Checked := True;
   end;
  end;

 procedure LabelCrackOnMouseEnter(Sender: TObject);
  begin
   if chbCopyCrack.Enabled then
    lblCopyCrack.Font.Color :=$2D7DEA;
  end;

 procedure LabelCrackOnMouseLeave(Sender: TObject);
  begin
   if chbCopyCrack.Enabled then
    lblCopyCrack.Font.Color :=$E6E0E1;
  end;

 procedure LabelCrackOnClick(Sender: TObject);
  begin
   if (chbCopyCrack.Checked and chbCopyCrack.Enabled) then begin
    chbCopyCrack.Checked := False;
   end
   else begin
    chbCopyCrack.Checked := True;
   end;
  end;

 procedure NullOnClick(Sender: TObject);
  begin
  end;

 procedure Bass_ChangePos(var1: Single);
  begin
   BASS_Volume := var1;
   if BASS_Volume < 0.03 then
    BASS_ChannelPause(mp3HNDL)
   else begin
    if (BASS_ChannelIsActive(mp3HNDL) = BASS_ACTIVE_PAUSED) then
     BASS_ChannelPlay(mp3HNDL, False);
    Log(FloatToStr(BASS_Volume));
    BASS_ChannelSetAttribute(mp3HNDL, 2, BASS_Volume);
   end;
  end;

 procedure ImgButtonOnMove(Sender: TObject; Shift: TShiftState; X, Y: Integer);
  begin
   if not GetCursorPos(BASS_CursorPos) then Exit;
   with Sender do begin
    BASS_CursorPos := WizardForm.ScreenToClient(BASS_CursorPos);
    if (ImgBtnMouseDown and (bmpTrackBar.Left < BASS_CursorPos.x) and ((bmpTrackBar.Left + bmpTrackBar.Width) > BASS_CursorPos.x)) then
     TBitmapImage(Sender).Left := (BASS_CursorPos.x - TBitmapImage(Sender).Width / 2 );
    TrackBarWidth := bmpTrackBar.Width;
    BASS_Volume := (bmpTrackButton.Left + bmpTrackButton.Width / 4 + ScaleX(1) - bmpTrackBar.Left) / (TrackBarWidth + bmpTrackButton.Width / 4);
    if ImgBtnMouseDown then
     BASS_ChangePos(BASS_Volume);
   end;
  end;

 procedure BkgButtonOnMouseEnter(Sender: TObject; Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
  begin
   if not GetCursorPos(BASS_CursorPos) then Exit;
   if not ImgBtnMouseDown then begin
    BASS_CursorPos := WizardForm.ScreenToClient(BASS_CursorPos);
    if BASS_CursorPos.x > bmpTrackButton.Left then
     bmpTrackButton.Left := (bmpTrackButton.Left + 3) + bmpTrackButton.Width / 2
    else
     bmpTrackButton.Left := (bmpTrackButton.Left - 3) - bmpTrackButton.Width / 2;
    TrackBarWidth := bmpTrackBar.Width;
    BASS_Volume := (bmpTrackButton.Left + bmpTrackButton.Width / 4 + ScaleX(1) - bmpTrackBar.Left) / (TrackBarWidth + bmpTrackButton.Width / 4);
    BASS_ChangePos(BASS_Volume);
   end;
  end;

 procedure ImgButtonOnMouseDown(Sender: TObject; Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
  begin
   ImgBtnMouseDown := True;
   with Sender do begin
    try
     ImageName := TBitmapImage(Sender).Name;
     ExtractTemporaryFileToStream(ImageName + '3.bmp', MemStream);
     MemStream.Position := 0;
     TBitmapImage(Sender).Bitmap.LoadFromStream(MemStream);
    finally
     MemStream.Clear;
    end;
   end;
  end;

 procedure ImgButtonOnMouseUp(Sender: TObject; Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
  begin
   ImgBtnMouseDown := False;
   if ImgBtnMouseEnter then
    with Sender do
     try
      ImageName := TBitmapImage(Sender).Name;
      ExtractTemporaryFileToStream(ImageName + '2.bmp', MemStream);
      MemStream.Position := 0;
      TBitmapImage(Sender).Bitmap.LoadFromStream(MemStream);
     finally
      MemStream.Clear;
     end;
  end;

 procedure ImgButtonOnMouseEnter(Sender: TObject);
  begin
   ImgBtnMouseEnter := True;
   with Sender do
    try
     ImageName := TBitmapImage(Sender).Name;
     if ImgBtnMouseDown then
      ExtractTemporaryFileToStream(ImageName + '3.bmp', MemStream)
     else
      ExtractTemporaryFileToStream(ImageName + '2.bmp', MemStream);
     MemStream.Position := 0;
     TBitmapImage(Sender).Bitmap.LoadFromStream(MemStream);
    finally
     MemStream.Clear;
    end;
  end;

 procedure ImgButtonOnMouseLeave(Sender: TObject);
  begin
   ImgBtnMouseEnter := False;
   with Sender do
   try
    ImageName := TBitmapImage(Sender).Name;
    ExtractTemporaryFileToStream(ImageName + '1.bmp', MemStream);
    MemStream.Position := 0;
    TBitmapImage(Sender).Bitmap.LoadFromStream(MemStream);
   finally
    MemStream.Clear;
   end;
  end;

 procedure ImgButton1OnClick(Sender: TObject);
  begin
   if not (BASS_ChannelIsActive(mp3HNDL) = BASS_ACTIVE_PAUSED) then begin
    BASS_ChannelPause(mp3HNDL);
    TaskBarButtonIcon(hMusic, TBIcon[3].Handle);
   end;
  end;

 procedure ImgButton2OnClick(Sender: TObject);
  begin
   if ((BASS_ChannelIsActive(mp3HNDL) = BASS_ACTIVE_PAUSED) and (BASS_Volume > 0.03)) then begin
    BASS_ChannelPlay(mp3HNDL, False);
    TaskBarButtonIcon(hMusic, TBIcon[4].Handle);
   end;
  end;

 procedure LoadSoundButton;
  begin
   bmpPauseButton := TBitmapImage.Create(WizardForm);
   bmpPauseButton.ReplaceColor := clBlack;
   bmpPauseButton.ReplaceWithColor := WizardForm.Color;
   bmpPauseButton.Parent := WizardForm;
   bmpPauseButton.Stretch := True;
   try
    MemStream := TMemoryStream.Create;
    ExtractTemporaryFileToStream('Pause1.bmp', MemStream);
    MemStream.Position := 0;
    bmpPauseButton.Bitmap.LoadFromStream(MemStream);
   finally
    MemStream.Clear;
   end;
   bmpPauseButton.SetBounds(bvlInstallOptions.Left + bvlInstallOptions.Width - ScaleX(16), bvlInstallOptions.Top + ScaleY(7), ScaleX(11), ScaleY(11));
   bmpPauseButton.Name := 'Pause';
   bmpPauseButton.OnMouseEnter := @ImgButtonOnMouseEnter;
   bmpPauseButton.OnMouseLeave := @ImgButtonOnMouseLeave;
   bmpPauseButton.OnMouseDown := @ImgButtonOnMouseDown;
   bmpPauseButton.OnMouseUp := @ImgButtonOnMouseUp;
   bmpPauseButton.OnClick := @ImgButton1OnClick;
   bmpPlayButton := TBitmapImage.Create(WizardForm);
   bmpPlayButton.Parent := WizardForm;
   bmpPlayButton.ReplaceColor := clBlack;
   bmpPlayButton.ReplaceWithColor := WizardForm.Color;
   bmpPlayButton.Stretch := True;
   try
    ExtractTemporaryFileToStream('Play1.bmp', MemStream);
    MemStream.Position := 0;
    bmpPlayButton.Bitmap.LoadFromStream(MemStream);
   finally
    MemStream.Clear;
   end;
   bmpPlayButton.SetBounds(bmpPauseButton.Left - ScaleX(13), bmpPauseButton.Top, ScaleX(11), ScaleY(11));
   bmpPlayButton.Name := 'Play';
   bmpPlayButton.OnMouseEnter := @ImgButtonOnMouseEnter;
   bmpPlayButton.OnMouseLeave := @ImgButtonOnMouseLeave;
   bmpPlayButton.OnMouseDown := @ImgButtonOnMouseDown;
   bmpPlayButton.OnMouseUp := @ImgButtonOnMouseUp;
   bmpPlayButton.OnClick := @ImgButton2OnClick;
   bmpTrackBar := TBitmapImage.Create(WizardForm);
   bmpTrackBar.Parent := WizardForm;
   bmpTrackBar.ReplaceColor := clBlack;
   bmpTrackBar.ReplaceWithColor := WizardForm.Color;
   bmpTrackBar.Stretch := True;
   try
    ExtractTemporaryFileToStream('trackBkg.bmp', MemStream);
    MemStream.Position := 0;
    bmpTrackBar.Bitmap.LoadFromStream(MemStream);
   finally
    MemStream.Clear;
   end;
   bmpTrackBar.SetBounds(bmpPlayButton.Left - ScaleX(65), bmpPlayButton.Top + ScaleY(4), ScaleX(60), ScaleY(3));
   bmpTrackBar.Name := 'trackBkg';
   bmpTrackBar.OnMouseDown := @BkgButtonOnMouseEnter;
   bmpTrackButton := TBitmapImage.Create(WizardForm);
   bmpTrackButton.Parent := WizardForm;
   bmpTrackButton.ReplaceColor := clBlack;
   bmpTrackButton.ReplaceWithColor := WizardForm.Color;
   bmpTrackButton.Stretch := True;
   try
    ExtractTemporaryFileToStream('trackbtn1.bmp', MemStream);
    MemStream.Position := 0;
    bmpTrackButton.Bitmap.LoadFromStream(MemStream);
   finally
    MemStream.Clear;
   end;
   bmpTrackButton.SetBounds(bmpTrackBar.Left + bmpTrackBar.Width / 2, bmpTrackBar.Top - ScaleY(9) / 2 + bmpTrackBar.Height / 2, ScaleX(8), ScaleY(9));
   bmpTrackButton.Name := 'trackbtn';
   bmpTrackButton.OnMouseEnter := @ImgButtonOnMouseEnter;
   bmpTrackButton.OnMouseLeave := @ImgButtonOnMouseLeave;
   bmpTrackButton.OnMouseDown := @ImgButtonOnMouseDown;
   bmpTrackButton.OnMouseUp := @ImgButtonOnMouseUp;
   bmpTrackButton.OnMouseMove := @ImgButtonOnMove;
   BASS_Init(-1, 44100, 0, 0, 0);
   BASS_Start;
   mp3HNDL := BASS_StreamCreateFile(True, 'Music{#MusicGroup}.ogg', 0, 0, 4);
   TrackBarWidth := bmpTrackBar.Width;
   BASS_Volume := (bmpTrackButton.Left + bmpTrackButton.Width / 4 + ScaleX(1) - bmpTrackBar.Left) / (TrackBarWidth + bmpTrackButton.Width / 4);
   BASS_ChangePos(BASS_Volume);
   BASS_ChannelPlay(mp3HNDL, True);
   if not (BASS_ChannelIsActive(mp3HNDL) = BASS_ACTIVE_PAUSED) then
    TaskBarButtonIcon(hMusic, TBIcon[4].Handle)
   else
    TaskBarButtonIcon(hMusic, TBIcon[3].Handle);
  end;

 procedure DubleOnClick(Sender: TObject);
  begin
   case Sender of
    btnLeftButton: WizardForm.CancelButton.OnClick(WizardForm.CancelButton);
    btnRightButton: WizardForm.NextButton.OnClick(WizardForm.NextButton);
   end;
  end;

 procedure DirFolderChange(Sender: TObject);
  begin
   DirEdit.Text := AddBackslash(FolderTreeView.Directory) + '{#Game}';
  end;

 procedure BackClick(Sender: TObject);
  begin
   FolderTreeView.ChangeDirectory(AddBackslash(WizardForm.DirEdit.Text), True);
   DirEdit.Text := AddBackslash(FolderTreeView.Directory);
  end;

 procedure NewClick(Sender: TObject);
  begin
   FolderTreeView.CreateNewDirectory(SetupMessage(msgNewFolderName));
   DirEdit.Text := AddBackslash(FolderTreeView.Directory);
  end;

 procedure BrowseClick(Sender: TObject);
  var
   btnOk: TButton;
   btnCancel: TButton;
   btnDefault: TButton;
   btnNewDir: TButton;
   lblBrowse: TLabel;
   bvlTop: TBevel;
   bvlTopOk: TBevel;
   bvlBotOk: TBevel;
   bvlTopCancel: TBevel;
   bvlBotCancel: TBevel;
   bvlTopDefault: TBevel;
   bvlBotDefault: TBevel;
   bvlTopNewDir: TBevel;
   bvlBotNewDir: TBevel;
   bvlBottom: TBevel;
  begin
   frmDirBrowse := CreateCustomForm();
   try
    frmDirBrowse.ClientWidth := ScaleX(450);
    frmDirBrowse.ClientHeight := ScaleY(317);
    frmDirBrowse.Position := poScreenCenter;
    frmDirBrowse.Caption := SetupMessage(msgBrowseDialogTitle);
    frmDirBrowse.Font.Name := WizardForm.DirEdit.Font.Name;
    frmDirBrowse.Font.Size := 8;
    frmDirBrowse.Font.Color := $E6E0E1;
    frmDirBrowse.CenterInsideControl(WizardForm, False);
    FolderTreeView := TFolderTreeView.Create(frmDirBrowse);
    FolderTreeView.SetBounds(ScaleX(5), ScaleY(51), ScaleX(355), ScaleY(261));
    FolderTreeView.OnChange := @DirFolderChange;
    FolderTreeView.Parent := frmDirBrowse;
    DirEdit := TNewEdit.Create(frmDirBrowse);
    DirEdit.SetBounds(ScaleX(5), ScaleY(25), ScaleX(440), ScaleY(15));
    DirEdit.Text := WizardForm.DirEdit.Text;
    DirEdit.Parent := frmDirBrowse;
    lblBrowse := TLabel.Create(frmDirBrowse);
    lblBrowse.SetBounds(ScaleX(6), ScaleY(5), ScaleX(400), ScaleY(20));
    lblBrowse.Caption := SetupMessage(msgBrowseDialogLabel);
    lblBrowse.Parent := frmDirBrowse;
    lblBrowse.Font.Color := $E6E0E1;
    bvlTop := TBevel.Create(frmDirBrowse);
    bvlTop.SetBounds(ScaleX(365), FolderTreeView.Top - ScaleY(2), ScaleX(80), ScaleY(5));
    bvlTop.Shape := bsBottomLine;
    bvlTop.Style := bsRaised;
    bvlTop.Parent := frmDirBrowse;
    bvlTopOk := TBevel.Create(frmDirBrowse);
    bvlTopOk.SetBounds(bvlTop.Left, bvlTop.Top + ScaleY(10), bvlTop.Width, bvlTop.Height);
    bvlTopOk.Shape := bsBottomLine;
    bvlTopOk.Style := bsRaised;
    bvlTopOk.Parent := frmDirBrowse;
    if IsThemeActive then begin
     btnOk := TButton.Create(frmDirBrowse); end
    else begin
     btnOk := TNewButton.Create(frmDirBrowse);
    end;
    btnOk.SetBounds(bvlTop.Left, bvlTopOk.Top + ScaleY(17), bvlTop.Width, ScaleY(25));
    btnOk.Font.Name := WizardForm.DirEdit.Font.Name;
    btnOk.Font.Size := 8;
    btnOk.Font.Color := clBlack;
    btnOk.Parent := frmDirBrowse;
    btnOk.Caption := SetupMessage(msgButtonOK);
    btnOk.ModalResult := mrOk;
    btnOk.Default := True;
    bvlBotOk := TBevel.Create(frmDirBrowse);
    bvlBotOk.SetBounds(bvlTop.Left, btnOk.Top + btnOk.Height + ScaleY(10), bvlTop.Width, bvlTop.Height);
    bvlBotOk.Shape := bsBottomLine;
    bvlBotOk.Style := bsRaised;
    bvlBotOk.Parent := frmDirBrowse;
    bvlTopCancel := TBevel.Create(frmDirBrowse);
    bvlTopCancel.SetBounds(bvlBotOk.Left, bvlBotOk.Top + ScaleY(10), bvlTop.Width, bvlTop.Height);
    bvlTopCancel.Shape := bsBottomLine;
    bvlTopCancel.Style := bsRaised;
    bvlTopCancel.Parent := frmDirBrowse;
    if IsThemeActive then begin
     btnCancel := TButton.Create(frmDirBrowse); end
    else begin
     btnCancel := TNewButton.Create(frmDirBrowse);
    end;
    btnCancel.SetBounds(bvlTop.Left, bvlTopCancel.Top + ScaleY(17), bvlTop.Width, ScaleY(25));
    btnCancel.Font.Name := WizardForm.DirEdit.Font.Name;
    btnCancel.Font.Size := 8;
    btnCancel.Font.Color := clBlack;
    btnCancel.Parent := frmDirBrowse;
    btnCancel.Caption := SetupMessage(msgButtonCancel);
    btnCancel.ModalResult := mrCancel;
    btnCancel.Cancel := True;
    bvlBotCancel := TBevel.Create(frmDirBrowse);
    bvlBotCancel.SetBounds(bvlTop.Left, btnCancel.Top + btnCancel.Height + ScaleY(10), bvlTop.Width, bvlTop.Height);
    bvlBotCancel.Shape := bsBottomLine;
    bvlBotCancel.Style := bsRaised;
    bvlBotCancel.Parent := frmDirBrowse;
    bvlTopDefault := TBevel.Create(frmDirBrowse);
    bvlTopDefault.SetBounds(bvlBotOk.Left, bvlBotCancel.Top + ScaleY(10), bvlTop.Width, bvlTop.Height);
    bvlTopDefault.Shape := bsBottomLine;
    bvlTopDefault.Style := bsRaised;
    bvlTopDefault.Parent := frmDirBrowse;
    if IsThemeActive then begin
     btnDefault := TButton.Create(frmDirBrowse); end
    else begin
     btnDefault := TNewButton.Create(frmDirBrowse);
    end;
    btnDefault.SetBounds(bvlTop.Left, bvlTopDefault.Top + ScaleY(17), bvlTop.Width, ScaleY(25));
    btnDefault.Font.Name := WizardForm.DirEdit.Font.Name;
    btnDefault.Font.Size := 8;
    btnDefault.Font.Color := clBlack;
    btnDefault.Parent := frmDirBrowse;
    btnDefault.Caption := 'Default';
    btnDefault.OnClick := @BackClick;
    bvlBotDefault := TBevel.Create(frmDirBrowse);
    bvlBotDefault.SetBounds(bvlTop.Left, btnDefault.Top + btnDefault.Height + ScaleY(10), bvlTop.Width, bvlTop.Height);
    bvlBotDefault.Shape := bsBottomLine;
    bvlBotDefault.Style := bsRaised;
    bvlBotDefault.Parent := frmDirBrowse;
    bvlTopNewDir := TBevel.Create(frmDirBrowse);
    bvlTopNewDir.SetBounds(bvlBotOk.Left, bvlBotDefault.Top + ScaleY(10), bvlTop.Width, bvlTop.Height);
    bvlTopNewDir.Shape := bsBottomLine;
    bvlTopNewDir.Style := bsRaised;
    bvlTopNewDir.Parent := frmDirBrowse;
    if IsThemeActive then begin
     btnNewDir := TButton.Create(frmDirBrowse); end
    else begin
     btnNewDir := TNewButton.Create(frmDirBrowse);
    end;
    btnNewDir.SetBounds(bvlTop.Left, bvlTopNewDir.Top + ScaleY(17), bvlTop.Width, ScaleY(25));
    btnNewDir.Font.Name := WizardForm.DirEdit.Font.Name;
    btnNewDir.Font.Size := 8;
    btnNewDir.Font.Color := clBlack;
    btnNewDir.Parent := frmDirBrowse;
    btnNewDir.Caption := SetupMessage(msgNewFolderName);
    btnNewDir.OnClick := @NewClick;
    bvlBotNewDir := TBevel.Create(frmDirBrowse);
    bvlBotNewDir.SetBounds(bvlTop.Left, btnNewDir.Top + btnNewDir.Height + ScaleY(10), bvlTop.Width, bvlTop.Height);
    bvlBotNewDir.Shape := bsBottomLine;
    bvlBotNewDir.Style := bsRaised;
    bvlBotNewDir.Parent := frmDirBrowse;
    bvlBottom := TBevel.Create(frmDirBrowse);
    bvlBottom.SetBounds(bvlBotOk.Left, bvlBotNewDir.Top + ScaleY(10), bvlTop.Width, bvlTop.Height);
    bvlBottom.Shape := bsBottomLine;
    bvlBottom.Style := bsRaised;
    bvlBottom.Parent := frmDirBrowse;
    FolderTreeView.ChangeDirectory(AddBackslash(WizardForm.DirEdit.Text), True);
    DirEdit.Text := AddBackslash(FolderTreeView.Directory);
    frmDirBrowse.ActiveControl := FolderTreeView;
    if frmDirBrowse.ShowModal = mrOk then begin
     WizardForm.DirEdit.Text := AddBackslash(DirEdit.Text);
    end;
   finally
    frmDirBrowse.Free;
   end;
  end;

 procedure GroupFolderChange(Sender: TObject);
  begin
   GroupEdit.Text := AddBackslash(StartMenuTreeView.Directory) + '{#Game}';
  end;

 procedure GroupClick(Sender: TObject);
  var
   btnCancel: TButton;
   btnOk: TButton;
   lblBrowse: TLabel;
  begin
   frmGroupBrowse := CreateCustomForm();
   try
    frmGroupBrowse := CreateCustomForm();
    frmGroupBrowse.ClientWidth := ScaleX(455);
    frmGroupBrowse.ClientHeight := ScaleY(260);
    frmGroupBrowse.Caption := SetupMessage(msgBrowseDialogTitle);
    frmGroupBrowse.Position := poScreenCenter;
    frmGroupBrowse.Font.Name := WizardForm.DirEdit.Font.Name;
    frmGroupBrowse.Font.Size := 8;
    frmGroupBrowse.Font.Color := $E6E0E1;
    frmGroupBrowse.CenterInsideControl(WizardForm, False);
    if IsThemeActive then begin
     btnOk := TButton.Create(frmGroupBrowse); end
    else begin
     btnOk := TNewButton.Create(frmGroupBrowse);
    end;
    btnOk.SetBounds(ScaleX(285), ScaleY(25), ScaleX(80), ScaleY(25));
    btnOk.Font.Name := WizardForm.DirEdit.Font.Name;
    btnOk.Font.Size := 8;
    btnOk.Font.Color := clBlack;
    btnOk.Parent := frmGroupBrowse;
    btnOk.Caption := SetupMessage(msgButtonOK);
    btnOk.ModalResult := mrOk;
    btnOk.Default := True;
    if IsThemeActive then begin
     btnCancel := TButton.Create(frmGroupBrowse); end
    else begin
     btnCancel := TNewButton.Create(frmGroupBrowse);
    end;
    btnCancel.SetBounds(ScaleX(370), ScaleY(25), ScaleX(80), ScaleY(25));
    btnCancel.Font.Name := WizardForm.DirEdit.Font.Name;
    btnCancel.Font.Size := 8;
    btnCancel.Font.Color := clBlack;
    btnCancel.Parent := frmGroupBrowse;
    btnCancel.Caption := SetupMessage(msgButtonCancel);
    btnCancel.ModalResult := mrCancel;
    btnCancel.Cancel := True;
    StartMenuTreeView := TStartMenuFolderTreeView.Create(frmGroupBrowse);
    StartMenuTreeView.SetBounds(ScaleX(5), ScaleY(55), ScaleX(445), ScaleY(200));
    StartMenuTreeView.OnChange := @GroupFolderChange;
    StartMenuTreeView.SetPaths(ExpandConstant('{userprograms}'), ExpandConstant('{commonprograms}'), ExpandConstant('{userstartup}'), ExpandConstant('{commonstartup}'));
    StartMenuTreeView.Parent := frmGroupBrowse;
    GroupEdit := TNewEdit.Create(frmGroupBrowse);
    GroupEdit.SetBounds(ScaleX(5), ScaleY(27), ScaleX(275), ScaleY(15));
    GroupEdit.Text := WizardForm.GroupEdit.Text;
    GroupEdit.Parent := frmGroupBrowse;
    lblBrowse := TLabel.Create(frmGroupBrowse);
    lblBrowse.SetBounds(ScaleX(6), ScaleY(5), ScaleX(400), ScaleY(20));
    lblBrowse.Caption := SetupMessage(msgBrowseDialogLabel);
    lblBrowse.Parent := frmGroupBrowse;
    lblBrowse.Font.Color := $E6E0E1;
    StartMenuTreeView.ChangeDirectory(AddBackslash(WizardForm.GroupEdit.Text), True);
    GroupEdit.Text := AddBackslash(StartMenuTreeView.Directory);
    frmGroupBrowse.ActiveControl := StartMenuTreeView;
    if frmGroupBrowse.ShowModal = mrOk then begin
     WizardForm.GroupEdit.Text := AddBackslash(GroupEdit.Text);
    end;
   finally
    frmGroupBrowse.Free;
   end;
  end;

 procedure StyleStreamCreate(InitStylefile: String);
  var
   StyleSize: Cardinal;
   StyleBuffer: AnsiString;
  begin
   StyleSize := ExtractTemporaryFileSize(InitStylefile);
   SetLength(StyleBuffer, StyleSize);
   ExtractTemporaryFileToBuffer(InitStylefile, Cast{#defined UNICODE ? "Ansi" : ""}StringToInteger(StyleBuffer));
   LoadFromStreamVCLStyle(StyleBuffer, 'KZXRf5RrfMs4K4Li');
  end;

 function InitializeSetup(): Boolean;
  begin
   hMutex := CreateMutexA(0, 1, 'WSjuQKBxmd_mut');
   bInitDone := False;
   if not FileExists(ExpandConstant('{tmp}\ISDone.dll')) then begin
    ExtractTemporaryFile('ISDone.dll');
   end;
   if not FileExists(ExpandConstant('{tmp}\VclStylesInno.dll')) then begin
    ExtractTemporaryFile('VclStylesInno.dll');
   end;
   if not FileExists(ExpandConstant('{tmp}\BASS.dll')) then begin
    ExtractTemporaryFile('BASS.dll');
   end;
   if not FileExists(ExpandConstant('{tmp}\BP.dll')) then begin
    ExtractTemporaryFile('BP.dll');
   end;
   if not FileExists(ExpandConstant('{tmp}\WinTB.dll')) then begin
    ExtractTemporaryFile('WinTB.dll');
   end;
   StyleStreamCreate('{#Style}.vsf');
   CloseHandle(hMutex);
   Result := StartUpAdmin;
  end;

 procedure InitializeWizard();
  begin
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
   bvlInstallForm := TBevel.Create(WizardForm);
   bvlInstallForm.SetBounds(ScaleX(10), ScaleY(60), WizardForm.ClientWidth - ScaleX(20), ScaleY(180));
   bvlInstallForm.Shape := bsBox;
   bvlInstallForm.Style := bsRaised;
   bvlInstallForm.Parent := WizardForm;
   bvlDirInstall := TBevel.Create(WizardForm);
   bvlDirInstall.SetBounds(bvlInstallForm.Left + ScaleX(10), bvlInstallForm.Top + ScaleY(10), bvlInstallForm.Width - ScaleX(20), ScaleY(75));
   bvlDirInstall.Shape := bsBox;
   bvlDirInstall.Style := bsLowered;
   bvlDirInstall.Parent := WizardForm;
   bvlIconGroup := TBevel.Create(WizardForm);
   bvlIconGroup.SetBounds(bvlDirInstall.Left, bvlDirInstall.Top + bvlDirInstall.Height + ScaleY(10), bvlDirInstall.Width, bvlDirInstall.Height);
   bvlIconGroup.Shape := bsBox;
   bvlIconGroup.Style := bsLowered;
   bvlIconGroup.Parent := WizardForm;
   bvlOptionsForm := TBevel.Create(WizardForm);
   bvlOptionsForm.SetBounds(bvlInstallForm.Left, bvlInstallForm.Top + bvlInstallForm.Height + ScaleY(10), bvlInstallForm.Width, bvlDirInstall.Height - ScaleY(7));
   bvlOptionsForm.Shape := bsBox;
   bvlOptionsForm.Style := bsLowered;
   bvlOptionsForm.Parent := WizardForm;
   bvlInstallOptions := TBevel.Create(WizardForm);
   bvlInstallOptions.SetBounds(bvlOptionsForm.Left + ScaleX(10), bvlOptionsForm.Top + ScaleY(10), bvlOptionsForm.Width - ScaleX(20), bvlOptionsForm.Height - ScaleY(20));
   bvlInstallOptions.Shape := bsBox;
   bvlInstallOptions.Style := bsLowered;
   bvlInstallOptions.Parent := WizardForm;
   WizardForm.DirEdit.Parent := WizardForm;
   WizardForm.DirEdit.SetBounds(bvlDirInstall.Left + ScaleX(55), bvlDirInstall.Top + ScaleY(25), bvlDirInstall.Width - ScaleX(150), ScaleY(75))
   WizardForm.DirEdit.Font.Color := $E6E0E1;
   if IsThemeActive then begin
    btnDirBrowse := TButton.Create(WizardForm); end
   else begin
    btnDirBrowse := TNewButton.Create(WizardForm);
   end;
   btnDirBrowse.SetBounds(WizardForm.DirEdit.Left + WizardForm.DirEdit.Width + ScaleX(5), WizardForm.DirEdit.Top - ScaleY(1), ScaleX(80), ScaleY(23));
   btnDirBrowse.Font.Name := WizardForm.DirEdit.Font.Name;
   btnDirBrowse.Font.Size := 8;
   btnDirBrowse.Parent := WizardForm;
   btnDirBrowse.Caption := WizardForm.DirBrowseButton.Caption;
   btnDirBrowse.OnClick := @BrowseClick;
   WizardForm.GroupEdit.Parent:= WizardForm;
   WizardForm.GroupEdit.SetBounds(bvlIconGroup.Left + ScaleX(10), bvlIconGroup.Top + ScaleY(25), bvlIconGroup.Width - ScaleX(105), ScaleY(75))
   WizardForm.GroupEdit.Font.Color := $E6E0E1;
   if IsThemeActive then begin
    btnGroupBrowse := TButton.Create(WizardForm); end
   else begin
    btnGroupBrowse := TNewButton.Create(WizardForm);
   end;
   btnGroupBrowse.SetBounds(WizardForm.GroupEdit.Left + WizardForm.GroupEdit.Width + ScaleX(5), WizardForm.GroupEdit.Top - ScaleY(1), ScaleX(80), ScaleY(23));
   btnGroupBrowse.Font.Name := WizardForm.DirEdit.Font.Name;
   btnGroupBrowse.Font.Size := 8;
   btnGroupBrowse.Parent := WizardForm;
   btnGroupBrowse.Caption := WizardForm.DirBrowseButton.Caption;
   btnGroupBrowse.OnClick := @GroupClick;
   lblDirInstall := TLabel.Create(WizardForm);
   lblDirInstall.Parent := WizardForm;
   lblDirInstall.AutoSize := False;
   lblDirInstall.SetBounds(bvlDirInstall.Left + ScaleX(10), bvlDirInstall.Top + ScaleY(5), bvlDirInstall.Width - ScaleX(20), ScaleY(15));
   lblDirInstall.Transparent := True;
   lblDirInstall.WordWrap := True;
   lblDirInstall.Font.Color := $E6E0E1; lblDirInstall.Font.Name := 'Arial'; lblDirInstall.Font.Style := [fsBold]; lblDirInstall.Font.Size := 9;
   lblDirInstall.Caption := ExpandConstant('{cm:DirInstall}');
   cbxDrive := TNewComboBox.Create(WizardForm);
   cbxDrive.Parent := WizardForm;
   cbxDrive.SetBounds(lblDirInstall.Left, WizardForm.DirEdit.Top, ScaleX(40), WizardForm.DirEdit.Height);
   cbxDrive.Style := csDropDownList;
   cbxDrive.OnClick := @CBDriveOnClick;
   AddDriveToList(cbxDrive);
   chbCreateDesktopIcon := TCheckBox.Create(WizardForm);
   chbCreateDesktopIcon.SetBounds(lblDirInstall.Left, WizardForm.DirEdit.Top + WizardForm.DirEdit.Height + ScaleY(5), ScaleX(15), ScaleY(15));
   chbCreateDesktopIcon.Parent := WizardForm;
   chbCreateDesktopIcon.Checked := True;
   chbCreateDesktopIcon.Cursor := 1;
   lblDesktopIcon := TLabel.Create(WizardForm);
   lblDesktopIcon.Parent := WizardForm;
   lblDesktopIcon.SetBounds(chbCreateDesktopIcon.Left + ScaleX(17), chbCreateDesktopIcon.Top + ScaleY(1), bvlDirInstall.Width - ScaleX(40), ScaleY(15));
   lblDesktopIcon.Transparent := True;
   lblDesktopIcon.WordWrap := False;
   lblDesktopIcon.AutoSize := True;
   lblDesktopIcon.Font.Color := $E6E0E1;
   lblDesktopIcon.Font.Name := 'Arial';
   lblDesktopIcon.Font.Style := [fsBold];
   lblDesktopIcon.Font.Size := 9;
   lblDesktopIcon.Cursor := 1;
   lblDesktopIcon.Caption := ExpandConstant('{cm:IconDest}');
   lblDesktopIcon.OnMouseEnter := @Label1OnMouseEnter;
   lblDesktopIcon.OnMouseLeave := @Label1OnMouseLeave;
   lblDesktopIcon.OnClick := @Label1OnClick;
   lblGroupDir := TLabel.Create(WizardForm);
   lblGroupDir.Parent := WizardForm;
   lblGroupDir.AutoSize := False;
   lblGroupDir.SetBounds(WizardForm.GroupEdit.Left, bvlIconGroup.Top + ScaleY(5), bvlIconGroup.Width - ScaleX(20), ScaleY(15));
   lblGroupDir.Transparent := True;
   lblGroupDir.WordWrap := True;
   lblGroupDir.Font.Color := $E6E0E1;
   lblGroupDir.Font.Name := 'Arial';
   lblGroupDir.Font.Style := [fsBold];
   lblGroupDir.Font.Size := 9;
   lblGroupDir.Caption := ExpandConstant('{cm:IconGroup}');
   chbCreateGroup := TCheckBox.Create(WizardForm);
   chbCreateGroup.SetBounds(WizardForm.GroupEdit.Left, WizardForm.GroupEdit.Top + WizardForm.GroupEdit.Height + ScaleY(5), ScaleX(15), ScaleY(15));
   chbCreateGroup.Parent := WizardForm;
   chbCreateGroup.Checked := True;
   chbCreateGroup.Cursor := 1;
   chbCreateGroup.OnClick := @NoStartCheckListBoxClick;
   lblCreateGroup := TLabel.Create(WizardForm);
   lblCreateGroup.Parent := WizardForm;
   lblCreateGroup.SetBounds(chbCreateGroup.Left + ScaleX(17), chbCreateGroup.Top + ScaleY(1), bvlIconGroup.Width - ScaleX(40), ScaleY(15));
   lblCreateGroup.Transparent := True;
   lblCreateGroup.WordWrap := False;
   lblCreateGroup.AutoSize := True;
   lblCreateGroup.Font.Color := $E6E0E1;
   lblCreateGroup.Font.Name := 'Arial';
   lblCreateGroup.Font.Style := [fsBold];
   lblCreateGroup.Font.Size := 9;
   lblCreateGroup.Cursor := 1;
   lblCreateGroup.Caption := ExpandConstant('{cm:CreateIconGroup}');
   lblCreateGroup.OnMouseEnter := @NoStartLabelOnMouseEnter;
   lblCreateGroup.OnMouseLeave := @NoStartLabelOnMouseLeave;
   lblCreateGroup.OnClick := @NoStartLabelOnClick;
   lblDiskSizeNeeded := TLabel.Create(WizardForm);
   lblDiskSizeNeeded.Parent := WizardForm;
   lblDiskSizeNeeded.SetBounds(bvlInstallOptions.Left + ScaleY(10), bvlInstallOptions.Top + ScaleY(5), ScaleX(15), ScaleY(15));
   lblDiskSizeNeeded.Transparent := True;
   lblDiskSizeNeeded.WordWrap := False;
   lblDiskSizeNeeded.AutoSize := True;
   lblDiskSizeNeeded.Font.Color := $E6E0E1;
   lblDiskSizeNeeded.Font.Name := 'Arial';
   lblDiskSizeNeeded.Font.Style := [fsBold];
   lblDiskSizeNeeded.Font.Size := 9;
   chbNoUninstaller := TCheckBox.Create(WizardForm);
   chbNoUninstaller.SetBounds(lblDiskSizeNeeded.Left, lblDiskSizeNeeded.Top + lblDiskSizeNeeded.Height + ScaleY(5), ScaleX(15), ScaleY(15));
   chbNoUninstaller.Parent := WizardForm;
   chbNoUninstaller.Checked := False;
   chbNoUninstaller.Cursor := 1;
   lblNoUninstaller := TLabel.Create(WizardForm);
   lblNoUninstaller.Parent := WizardForm;
   lblNoUninstaller.SetBounds(chbNoUninstaller.Left + ScaleX(17), chbNoUninstaller.Top + ScaleY(1), lblNoUninstaller.Width, lblNoUninstaller.Height);
   lblNoUninstaller.Transparent := True;
   lblNoUninstaller.WordWrap := False;
   lblNoUninstaller.AutoSize := True;
   lblNoUninstaller.Font.Color := $E6E0E1;
   lblNoUninstaller.Font.Name := 'Arial';
   lblNoUninstaller.Font.Style := [fsBold];
   lblNoUninstaller.Font.Size := 9;
   lblNoUninstaller.Cursor := 1;
   lblNoUninstaller.Caption := ExpandConstant('{cm:NoUninstall}');
   lblNoUninstaller.OnMouseEnter := @InstallOptionsOnMouseEnter;
   lblNoUninstaller.OnMouseLeave := @InstallOptionsOnMouseLeave;
   lblNoUninstaller.OnClick := @InstallOptionsOnClick;
   chbCopyCrack := TCheckBox.Create(WizardForm);
   chbCopyCrack.SetBounds(chbNoUninstaller.Left, chbNoUninstaller.Top + ScaleY(20), ScaleX(15), ScaleY(15));
   chbCopyCrack.Parent := WizardForm;
   chbCopyCrack.Checked := False;
   chbCopyCrack.Cursor := 1;
   CrackInstalled := False;
   lblCopyCrack := TLabel.Create(WizardForm);
   lblCopyCrack.Parent := WizardForm;
   lblCopyCrack.SetBounds(chbCopyCrack.Left + ScaleX(17), chbCopyCrack.Top + ScaleY(1), lblCopyCrack.Width, lblCopyCrack.Height);
   lblCopyCrack.Transparent := True;
   lblCopyCrack.WordWrap := False;
   lblCopyCrack.AutoSize := True;
   lblCopyCrack.Font.Color := $E6E0E1;
   lblCopyCrack.Font.Name := 'Arial';
   lblCopyCrack.Font.Style := [fsBold];
   lblCopyCrack.Font.Size := 9;
   lblCopyCrack.Cursor := 1;
   lblCopyCrack.Caption := ExpandConstant('{cm:CopyCrack}');
   lblCopyCrack.OnMouseEnter := @LabelCrackOnMouseEnter;
   lblCopyCrack.OnMouseLeave := @LabelCrackOnMouseLeave;
   lblCopyCrack.OnClick := @LabelCrackOnClick;
   if IsCrackTheres then begin
    bvlOptionsForm.Height := WizardForm.ClientHeight - bvlInstallOptions.Top - chbCopyCrack.Top + chbCopyCrack.Height + ScaleY(55);
    bvlInstallOptions.Height := bvlOptionsForm.Height - ScaleY(20);
   end
   else begin
    chbCopyCrack.Hide;
    lblCopyCrack.Hide;
   end;
   bvlButtonForm := TBevel.Create(WizardForm);
   bvlButtonForm.SetBounds(bvlInstallForm.Left, bvlOptionsForm.Top + bvlOptionsForm.Height + ScaleY(10), bvlInstallForm.Width, ScaleY(50));
   bvlButtonForm.Shape := bsBox;
   bvlButtonForm.Style := bsLowered;
   bvlButtonForm.Parent := WizardForm;
   bvlLeftButton := TBevel.Create(WizardForm);
   bvlLeftButton.SetBounds(bvlButtonForm.Left + ScaleX(5), bvlButtonForm.Top + ScaleY(5), bvlButtonForm.Width / 2 - ScaleX(8), bvlButtonForm.Height - ScaleY(10));
   bvlLeftButton.Shape := bsBox;
   bvlLeftButton.Style := bsLowered;
   bvlLeftButton.Parent := WizardForm;
   bvlRightButton := TBevel.Create(WizardForm);
   bvlRightButton.SetBounds(bvlLeftButton.Left + bvlLeftButton.Width + ScaleX(6), bvlButtonForm.Top + ScaleY(5), bvlButtonForm.Width / 2 - ScaleX(8), bvlButtonForm.Height - ScaleY(10));
   bvlRightButton.Shape := bsBox;
   bvlRightButton.Style := bsLowered;
   bvlRightButton.Parent := WizardForm;
   WizardForm.CancelButton.SetBounds(WizardForm.Left - ScaleX(500), WizardForm.Top - ScaleY(500), ScaleX(0), ScaleY(0));
   WizardForm.NextButton.SetBounds(WizardForm.Left - ScaleX(500), WizardForm.Top - ScaleY(500), ScaleX(1), ScaleY(1));
   if IsThemeActive then begin
    btnLeftButton := TButton.Create(WizardForm); end
   else begin
    btnLeftButton := TNewButton.Create(WizardForm);
   end;
   btnLeftButton.SetBounds(bvlLeftButton.Left + ScaleX(5), bvlLeftButton.Top + ScaleY(5), bvlLeftButton.Width - ScaleX(10), bvlLeftButton.Height - ScaleY(10));
   btnLeftButton.Font.Name := WizardForm.DirEdit.Font.Name;
   btnLeftButton.Font.Size := 8;
   btnLeftButton.Parent := WizardForm;
   btnLeftButton.OnClick := @DubleOnClick;
   if IsThemeActive then begin
    btnRightButton := TButton.Create(WizardForm); end
   else begin
    btnRightButton := TNewButton.Create(WizardForm);
   end;
   btnRightButton.SetBounds(bvlRightButton.Left + ScaleX(5), bvlLeftButton.Top + ScaleY(5), bvlLeftButton.Width - ScaleX(10), bvlLeftButton.Height - ScaleY(10));
   btnRightButton.Font.Name := WizardForm.DirEdit.Font.Name;
   btnRightButton.Font.Size := 8;
   btnRightButton.Parent := WizardForm;
   btnRightButton.OnClick := @DubleOnClick;
   bvlProgressForm := TBevel.Create(WizardForm);
   bvlProgressForm.SetBounds(bvlInstallForm.Left, bvlButtonForm.Top + bvlButtonForm.Height + ScaleY(10), bvlInstallForm.Width, WizardForm.ClientHeight - bvlButtonForm.Top  - bvlButtonForm.Height - ScaleY(20));
   bvlProgressForm.Shape := bsBox;
   bvlProgressForm.Style := bsLowered;
   bvlProgressForm.Parent := WizardForm;
   bvlProgressGauge := TBevel.Create(WizardForm);
   bvlProgressGauge.SetBounds(bvlProgressForm.Left + ScaleX(10), bvlProgressForm.Top + ScaleY(10), bvlProgressForm.Width - ScaleX(20), bvlProgressForm.Height - ScaleY(20));
   bvlProgressGauge.Shape := bsBox;
   bvlProgressGauge.Style := bsLowered;
   bvlProgressGauge.Parent := WizardForm;
   WizardForm.ProgressGauge.Parent := WizardForm;
   WizardForm.ProgressGauge.SetBounds(bvlProgressGauge.Left + ScaleX(10), bvlProgressGauge.Top + ScaleY(10), bvlProgressGauge.Width - ScaleX(20), ScaleY(15));
   WizardForm.ProgressGauge.Max := 1000;
   memProgressLog := TNewMemo.Create(WizardForm);
   memProgressLog.SetBounds(WizardForm.ProgressGauge.Left + ScaleX(1), WizardForm.ProgressGauge.Top + WizardForm.ProgressGauge.Height + ScaleY(5), WizardForm.ProgressGauge.Width - ScaleX(2), bvlProgressGauge.Height - WizardForm.ProgressGauge.Height - ScaleY(25));
   memProgressLog.WordWrap := False;
   memProgressLog.Parent := WizardForm;
   memProgressLog.ScrollBars := ssVertical;
   memProgressLog.ReadOnly := True;
   memProgressLog.Clear;
   memProgressLog.Lines.Add(ExpandConstant('{cm:MemoReady}'));
   lblInstallResult := TLabel.Create(WizardForm);
   lblInstallResult.Parent := WizardForm;
   lblInstallResult.SetBounds(ScaleX(0), bvlProgressForm.Top + bvlProgressForm.Height + ScaleY(5), WizardForm.ClientWidth, ScaleY(30));
   lblInstallResult.Transparent := True;
   lblInstallResult.WordWrap := False;
   lblInstallResult.AutoSize := True;
   lblInstallResult.Font.Name := 'Tahoma';
   lblInstallResult.Font.Style := [fsBold];
   lblInstallResult.Font.Size := 14;
   lblInstallResult.Hide;
   if IsThemeActive then begin
    btnPause := TButton.Create(WizardForm); end
   else begin
    btnPause := TNewButton.Create(WizardForm);
   end;
   btnPause.SetBounds(btnRightButton.Left, btnRightButton.Top, btnRightButton.Width, btnRightButton.Height);
   btnPause.Font.Name := WizardForm.DirEdit.Font.Name;
   btnPause.Font.Size := 8;
   btnPause.Parent := WizardForm;
   btnPause.Caption := 'Pause';
   btnPause.OnClick := @PauseBtnClick;
   btnPause.Hide;
   if IsThemeActive then begin
    btnRetry := TButton.Create(WizardForm); end
   else begin
    btnRetry := TNewButton.Create(WizardForm);
   end;
   btnRetry.SetBounds(btnLeftButton.Left, btnLeftButton.Top, btnLeftButton.Width, btnLeftButton.Height);
   btnRetry.Font.Name := WizardForm.DirEdit.Font.Name;
   btnRetry.Font.Size := 8;
   btnRetry.Parent := WizardForm;
   btnRetry.Caption := 'Retry';
   btnRetry.OnClick := @AgainOnClick;
   btnRetry.Hide;
   if IsThemeActive then begin
    btnRun := TButton.Create(WizardForm); end
   else begin
    btnRun := TNewButton.Create(WizardForm);
   end;
   btnRun.SetBounds(btnLeftButton.Left, btnLeftButton.Top, btnLeftButton.Width, btnLeftButton.Height);
   btnRun.Font.Name := WizardForm.DirEdit.Font.Name;
   btnRun.Font.Size := 8;
   btnRun.Parent := WizardForm;
   btnRun.Caption := 'Run';
   btnRun.OnClick := @RunOnClick;
   btnRun.Enabled := False;
   btnRun.Hide;
   WizardForm.DirEdit.OnChange := @DirEditOnChange;
   Transparency := 0;
   oldWFproc := 0;
   TimerID := 0;
   oldWFproc := SetWindowLong(WizardForm.Handle, GWL_WNDPROC, WndProcCallBack(@MyProc, 4));
   bInitDone := True;
  end;

 function ShouldSkipPage(PageID: Integer): Boolean;
  begin
   if (PageID = wpPassword) or (PageID = wpWelcome) or (PageID = wpLicense) or (PageID = wpInfoBefore) or (PageID = wpUserInfo) or (PageID = wpReady) or (PageID = wpSelectComponents) or (PageID = wpSelectProgramGroup) or (PageID = wpSelectTasks) or (PageID = wpPreparing) or (PageID = wpInfoAfter) then begin
    Result := True;
   end;
  end;

 procedure CurPageChanged(CurPageID: Integer);
  begin
   if (CurPageID = wpSelectDir) then begin
    LoadTaskBar;
    LoadSoundButton;
    WizardForm.ActiveControl := btnRightButton;
    ISStep := 1;
    btnRightButton.Caption := SetupMessage(msgButtonInstall);
    btnLeftButton.Caption := ExpandConstant('{cm:ExitBtn}')
    DirEditOnChange(nil);
    WizardForm.BackButton.Visible := False;
   end;
   if (CurPageID = wpInstalling) then begin
    WizardForm.ActiveControl := btnLeftButton;
    ISStep := 2;
    btnPause.Show;
    cbxDrive.Enabled := False;
    WizardForm.DirEdit.Enabled := False;
    btnDirBrowse.Enabled := False;
    WizardForm.GroupEdit.Enabled := False;
    btnGroupBrowse.Enabled := False;
    chbCreateDesktopIcon.Enabled := False;
    chbCreateGroup.Enabled := False;
    chbNoUninstaller.Enabled := False;
    lblDesktopIcon.OnClick := @NullOnClick
    lblCreateGroup.OnClick := @NullOnClick
    lblNoUninstaller.OnClick := @NullOnClick
    chbCreateDesktopIcon.Cursor := -2;
    chbCreateGroup.Cursor := -2;
    chbNoUninstaller.Cursor := -2;
    lblDesktopIcon.Cursor := -2;
    lblCreateGroup.Cursor := -2;
    lblNoUninstaller.Cursor := -2;
    chbCopyCrack.Enabled := False;
    chbCopyCrack.Cursor := -2;
    lblCopyCrack.OnClick := @NullOnClick
    lblCopyCrack.Cursor := -2;
    TaskBarButtonToolTip(hExit, 'Cancel');
    TaskBarButtonToolTip(hInstall, 'Pause');
    TaskBarButtonIcon(hInstall, TBIcon[2].Handle);
    btnLeftButton.Caption := WizardForm.CancelButton.Caption;
    btnRightButton.Caption := WizardForm.NextButton.Caption;
    btnRightButton.Visible := False;
   end;
   if (CurPageID = wpFinished) then begin
    ISStep := 3;
    FinishedDone;
    WizardForm.ActiveControl := btnRightButton;
    Application.Title := '{#Game}';
    WizardForm.Caption := '{#Game}';
   end;
   WizardForm.BackButton.Visible := False;
  end;

 procedure CurStepChanged(CurStep: TSetupStep);
  var
   unused1, unused2: Integer;
   Comps1, Comps2, Comps3, TmpValue: Cardinal;
   FindHandle1, ColFiles1, CurIndex1, tmp: Integer;
   ExecError, ISFailed: Boolean;
   InFilePath, OutFilePath, OutFileName: PAnsiChar;
   ArcFileIndex, ArcFileCount: Integer;
   ArcFileName: String;
  begin
   if (CurStep = ssInstall) then begin
    ISDoneCancel := 0;
    ExtractTemporaryFile('English.ini');
    ExtractTemporaryFile('unarc.dll');
    ISPaused := False;
    ISDoneError := True;
    ISFailed := False;
    ArcFileIndex := 1;
    ArcFileCount := 0;
    while True do begin
     ArcFileName := ExpandConstant('{src}\setup-' + IntToStr(ArcFileIndex) + '.bin');
     if not FileExists(ArcFileName) then begin
      Break;
     end;
     Inc(ArcFileIndex);
     Inc(ArcFileCount);
    end;
    if ISDoneInit(ExpandConstant('{src}\records.inf'), $1111, Comps1, Comps2, Comps3, MainForm.Handle, 10, @ProgressCallback) then begin
     repeat
      ChangeLanguage('English');
      if not FileSearchInit(False) then begin
       Break;
      end;
      ArcFileIndex := 1;
      if ArcFileCount = 0 then ArcFileCount := 1;
      while ArcFileIndex <= ArcFileCount do begin
       ArcFileName := ExpandConstant('{src}\setup-' + IntToStr(ArcFileIndex) + '.bin');
       if not ISArcExtract(0, 100 / ArcFileCount, FileSearch(ArcFileName), ExpandConstant('{app}'), '', False, '', '', ExpandConstant('{app}'), False) then begin
        ISFailed := True;
        Break;
       end;
       Inc(ArcFileIndex);
      end;
      if ISFailed then begin
       Break;
      end;
      ISDoneError := False;
      btnRun.Show;
     until True;
     ISDoneStop;
    end;
    WizardForm.ProgressGauge.Style := npbstMarquee;
    btnLeftButton.Visible := True;
    btnLeftButton.Enabled := False;
    WizardForm.CancelButton.Visible := True;
    WizardForm.CancelButton.Enabled := False;
    btnPause.Enabled := False;
    TaskBarButtonEnabled(hInstall, False);
    TaskBarButtonEnabled(hExit, False);
    hMutex := CreateMutexA(0, 1, 'WSjuQKBxmd_mut');
    IsOwner(ExpandConstant('{app}'), 'CAwYZBWH1qnoKna0')
    CloseHandle(hMutex);
   end;
   if (CurStep = ssPostInstall) and ISDoneError then begin
    btnRightButton.Caption := WizardForm.NextButton.Caption;
    btnRightButton.Visible := True;
    btnRetry.Enabled := False;
    btnRetry.Show;
    SetTaskBarProgressState(0, 4);
    SetTaskBarProgressValue(0, 100);
    memProgressLog.Lines.Add(SetupMessage(msgStatusRollback));
    WizardForm.ProgressGauge.Style := npbstMarquee;
    Exec2(ExpandConstant('{uninstallexe}'), '/VERYSILENT', False); end
   else
    if not ISDoneError and (memProgressLog.Lines.Strings[memProgressLog.Lines.Count - 1] <> SetupMessage(msgStatusRunProgram)) then
     memProgressLog.Lines.Add(SetupMessage(msgStatusRunProgram));
   if (CurStep = ssPostInstall) and (chbCopyCrack.Checked) and not ISDoneError then begin
    hMutex := CreateMutexA(0, 1, 'WSjuQKBxmd_mut');
    if IsXCOPY(ExpandConstant('{#Game_CrackDir}'), ExpandConstant('{app}'), 'WpAYr2duAuquv8OF') then
     CrackInstalled := True
    else
     MsgBox(ExpandConstant('{cm:ErrCopy}'), mbConfirmation, MB_OK);
    CloseHandle(hMutex);
   end;
  end;

 procedure DeinitializeSetup;
  var
   unused: LongInt;
  begin
   if bInitDone then begin
    ShowWindow(WizardForm.Handle, SW_HIDE);
    BASS_ChannelSetAttribute(mp3HNDL, 2, 0.05)
    BASS_Stop;
    BASS_Free;
    MemStream.Free;
    UnLoadVCLStyles;
    TaskBarDestroy;
   end;
  end;
