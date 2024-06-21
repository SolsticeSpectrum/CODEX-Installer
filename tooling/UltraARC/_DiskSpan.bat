@echo off

title DiskSpan Compressor
setlocal enabledelayedexpansion enableextensions

:start
cls
::--------------------------------------------------------------------
::The Converter Config
set conversionBy=Simorq
set GameEXE=
set CONVFOLDR=Conversion_Output
set ARCHNAMES=Data.bin
set DISKLABEL=DVD_
set SETUPPATH=Setup_Files
set SETUPNAME=Setup.exe
set SETUPICON=Setup_Files\Setup.ico
set DVD5Size=4420
set DVD9Size=7900
set BD25Size=23450

::The batch editor (Set your name if you make some changes in the script)
set batchEditingBy= Simorq
echo.
echo.                            Based On Peterf1999 Scripts.
echo.                    ===========================================
::Example of game path to be compressed
set eg= C:\Program Files (x86)\Call Of Duty WWII
::--------------------------------------------------------------------


::variables
set choice=
set retry=
set creatingIso=
set shut=
set CMethod=
echo.


:dir
set /p choice=  Type Your Game Directory (f.e.%eg%): 
if "%choice%"=="" goto start
cls


::Check path, change MetroLL.exe to any file with extension of the game to be compressed
::You can add subfolders, e.g. "%choice%\Binaries\Win32\DMC-DevilMayCry.exe"
if not exist "%choice%\%GameEXE%" goto wrongDir
echo.
goto shutdown


::Error message if the path is incorrect, change the message style if you wish
:wrongDir
echo.
echo.--------------------------------------------------------------------
echo.
echo.The Directory You Typed is Incorrect!
echo.Please Type Again.
echo.
echo.--------------------------------------------------------------------
goto dir


:shutdown
echo.
echo.--------------------------------------------------------------------
echo.Do You Want to Shutdown Your PC After Conversion? [Y/N]
echo.--------------------------------------------------------------------
choice /C NY /N

if errorlevel 2 (

	set shut=true

	echo.Your PC Will Be Shutdown After Conversion.
	echo.%time:~0,-3% - PC Will Shutdown After Conversion is Finished>>".\Conversion.log"
	TIMEOUT 1

) else (

echo.Your Computer Won't Be Shutdown After Conversion.
echo.%time:~0,-3% - PC Will NOT Shutdown After Conversion is Finished>>".\Conversion.log"
TIMEOUT 1

)


:ISO
echo.
echo.--------------------------------------------------------------------
echo.Do You Want to Create ISO(s) After Conversion? [Y/N]
echo.--------------------------------------------------------------------
choice /C NY /N

if errorlevel 2 (

	set creatingIso=true

	echo.ISO Will Be Created After Conversion.
	echo.%time:~0,-3% - ISO Images Will Be Created After Conversion is Finished>>".\Conversion.log"
	TIMEOUT 1

) else (

echo.ISO Won't Be Created After Conversion.
echo.%time:~0,-3% - ISO Images Will NOT Be Created After Conversion is Finished>>".\Conversion.log"
TIMEOUT 1

)


:MtdSkp
cls
echo Storage Types Available:
echo [1] DVD5
echo [2] DVD9
echo [3] BD25
set INPUT=
echo.
set /P INPUT=Choice: %=%
cls
if "%INPUT%"=="" goto MtdSkp
cls
If /i %INPUT%==1 goto DVD5
If /i %INPUT%==2 goto DVD9
If /i %INPUT%==3 goto BD25
:DVD5
set PSIZE=%DVD5Size%mb:4430mb
echo.%time:~0,-3% - You Have Choosed DVD5 Size>>".\Conversion.log"
goto Methods
:DVD9
set PSIZE=%DVD9Size%mb:7950mb
echo.%time:~0,-3% - You Have Choosed DVD9 Size>>".\Conversion.log"
goto Methods
:BD25
set PSIZE=%BD25Size%mb:23500mb
echo.%time:~0,-3% - You Have Choosed BD25 Size>>".\Conversion.log"
goto Methods


:Methods
cls
echo.
echo Methods Types Available:
echo.--------------------------------------------------------------------
echo [0] No Compression
echo [1] SREP + LZMA
echo [2] SREP + 4x4:LZMA
echo [3] SREP + LZMA2
echo [4] SREP + LOLZ
echo [5] SREP + ZSTD
echo [6] xZLib + SREP + LZMA
echo [7] xZLib + SREP + LOLZ
echo [8] xLZ4 + SREP + LZMA
echo [9] xLZ4 + SREP + LOLZ
echo [10] xLZO + SREP + LZMA
echo [11] xLZO + SREP + LOLZ
echo [12] xZSTD + SREP + LZMA
echo [13] xZSTD + SREP + LOLZ
echo [14] xOodle + SREP + LZMA
echo [15] xOodle + SREP + LOLZ
echo [16] xCrilayla + SREP + LZMA
echo [17] xCrilayla + SREP + LOLZ
echo [18] Custom Compression
set INPUT=
echo.
set /P INPUT=Choice: %=%
cls
if "%INPUT%"=="" goto Methods
cls
If /i %INPUT%==0 goto Method0
If /i %INPUT%==1 goto Method1
If /i %INPUT%==2 goto Method2
If /i %INPUT%==3 goto Method3
If /i %INPUT%==4 goto Method4
If /i %INPUT%==5 goto Method5
If /i %INPUT%==6 goto Method6
If /i %INPUT%==7 goto Method7
If /i %INPUT%==8 goto Method8
If /i %INPUT%==9 goto Method9
If /i %INPUT%==10 goto Method10
If /i %INPUT%==11 goto Method11
If /i %INPUT%==12 goto Method12
If /i %INPUT%==13 goto Method13
If /i %INPUT%==14 goto Method14
If /i %INPUT%==15 goto Method15
If /i %INPUT%==16 goto Method16
If /i %INPUT%==17 goto Method17
If /i %INPUT%==18 goto CustomMethod


:Method0
set PMethod=0
goto Compress
:Method1
set PMethod=C1
goto Compress
:Method2
set PMethod=C2
goto Compress
:Method3
set PMethod=C3
goto Compress
:Method4
set PMethod=C4
goto Compress
:Method5
set PMethod=C5
goto Compress
:Method6
set PMethod=C6
goto Compress
:Method7
set PMethod=C7
goto Compress
:Method8
set PMethod=C8
goto Compress
:Method9
set PMethod=C9
goto Compress
:Method10
set PMethod=C10
goto Compress
:Method11
set PMethod=C11
goto Compress
:Method12
set PMethod=C12
goto Compress
:Method13
set PMethod=C13
goto Compress
:Method14
set PMethod=C14
goto Compress
:Method15
set PMethod=C15
goto Compress
:Method16
set PMethod=C16
goto Compress
:Method17
set PMethod=C17
goto Compress
:CustomMethod
echo.     Type in Custom Method
echo.     Example: BPK+srep:m3f+lolz:d128m:mc1023
echo.              xZ1+srep:m3f+4L1
echo.              UR+srep:m3f+XZ
echo.     See Resources\FA.ini For All Possible Settings.
echo.     You Can Also Create Your Own.
echo.
set /p PMethod=  Method:
goto Compress
cls


:Compress
cls
echo.%time:~0,-3% - Starting Archive Creation>>".\Conversion.log"
echo.Creating %ARCHNAMES%, Please Wait...
echo.
echo.--------------------------------------------------------------------
Resources\Arc.exe a -cfgFA.ini -ep1 -r -ed -s; -w.\TEMP -m%PMethod%+diskspan:%PSIZE% -x@Resources\_Exclude.lst -dp"%choice%" ".\%CONVFOLDR%\%ARCHNAMES%.001"
RD /S /Q TEMP
if ERRORLEVEL 1 goto arcfail


:Sorting
echo.%time:~0,-3% - Archive Creation Complete>>".\Conversion.log"
echo Sorting Storage Directories... > nul
Resources\Arc.exe --sort  "%CONVFOLDR%" "%DISKLABEL%" "%SETUPPATH%"\"%SETUPNAME%" "%SETUPICON%" "%ARCHNAMES%" > nul
echo Soring Complete > nul


:creatingIso
cls
echo.
echo.--------------------------------------------------------------------
echo.Creating ISO(s), Please Wait...
echo.--------------------------------------------------------------------
if "!creatingIso!"=="true" (
	echo.%time:~0,-3% - Starting ISO Creation>>".\Conversion.log"
	Resources\Arc.exe --makeiso "%CONVFOLDR%"\"%DISKLABEL%"
	echo.%time:~0,-3% - Finished ISO Creation>>".\Conversion.log"
)


:shutdownend
cls
if "!shut!"=="true" (
	echo.Conversion is Finished! Shutting Down PC in 45 Seconds.
	echo.Make Sure to Save All OF Your Work! 
	echo.%time:~0,-3% - PC Will Shutdown>>".\Conversion.log"
	!windir!\System32\shutdown.exe -s -t 45
)


:end
cls
echo.
echo.Conversion is Finished!
echo.--------------------------------------------------------------------
echo.
echo.%time:~0,-3% - Conversion is Finished!>>".\Conversion.log"
set "folder=%choice%"
set "size=" & for %%z in ("%folder%") do for /f "skip=2 tokens=2,3 delims=: " %%a in ('
	robocopy "%%~fz\." "%%~fz\." /l /nocopy /s /is /njh /nfl /ndl /r:0 /w:0 /xjd /xjf /np
	^| find ":"
	') do if not defined size (
	(for /f "delims=0123456789." %%c in ("%%b") do (break)) && (
	set "size=%%a%%b"
	) || (
	set "size=%%a"
	)
)

echo.%time:~0,-3% - Original Size:   %size%>>".\Conversion.log"
echo.%time:~0,-3% - Original Size:   %size%


set "folder=.\Conversion_Output\"
set "size=" & for %%z in ("%folder%") do for /f "skip=2 tokens=2,3 delims=: " %%a in ('
	robocopy "%%~fz\." "%%~fz\." /l /nocopy /s /is /njh /nfl /ndl /r:0 /w:0 /xjd /xjf /np
	^| find ":"
	') do if not defined size (
	(for /f "delims=0123456789." %%c in ("%%b") do (break)) && (
	set "size=%%a%%b"
	) || (
	set "size=%%a"
	)
)

echo.%time:~0,-3% - Compressed Size: %size%>>".\Conversion.log"
echo.%time:~0,-3% - Compressed Size: %size%
pause
goto EOF


:arcfail
if "!shut!"=="true" (
	echo.
	echo.WARNING: FreeArc returns an error in archive %arc%! 
	echo.%time:~0,-3% - Error in %ARCHNAMES% creation!>>".\Conversion.log"
	echo.%time:~0,-3% - PC will shutdown>>".\Conversion.log"
	!windir!\System32\shutdown.exe -s -t 30
) else (
	echo.
	echo.WARNING: FreeArc returns an error in archive %arc%! 
	echo.%time:~0,-3% - Error in %ARCHNAMES% creation!>>".\Conversion.log"
	set /p retry=Do you want to try again? [Y/N]:
	if /I "%retry%"=="Y" goto dir
	if /I "%retry%"=="N" goto END
	echo.
)
pause
:EOF

