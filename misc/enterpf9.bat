set cputype=cygwin

REM A directory you want to use as a home directory
set HOME=%HOMEPATH%\Eigene Dateien

REM Where the unpacked distribution can be found
set PF9=%HOME%\pf9



REM ---------------------------------------------
REM 
set CPUTYPE=%cputype%
set PATH=%PATH%;%PF9%\%cputype%\bin;%PF9%\rc\bin


REM Reset PF9 to tell cygwin-based PLan 9 programs that the system
REM root `/' is also the base of the Plan 9 system.
set PF9=/.

REM run a command interpreter in the home directory
cd %HOME%
cmd


