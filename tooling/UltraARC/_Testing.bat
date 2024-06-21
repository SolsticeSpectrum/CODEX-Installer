@echo off
echo.
cd /d "%~dp0"
If NOT exist "%1" goto ERR1

Resources\Arc t -cfgFA.ini -i2 -w.\Temp -dp%~d1 "%~1" 
RD /S /Q TEMP
goto OK

:OK
pause
exit

:ERR1
echo Error: Drag and Drop folder on bat file
pause
exit