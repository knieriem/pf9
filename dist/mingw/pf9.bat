set cputype=mingw

REM A directory you want to use as a home directory
set HOME=c:\%HOMEPATH%\Eigene Dateien

REM Where the unpacked distribution can be found
set PF9=e:\pf9
set PLAN9=%PF9%



REM ---------------------------------------------
REM 
set CPUTYPE=%cputype%
set PATH=%PF9%\%cputype%\bin;%PF9%\rc\bin;%PATH%;.

REM run a command interpreter in the home directory
cd %HOME%
cmd


