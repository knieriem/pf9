REM A directory you want to use as a home directory
set HOME=/%HOMEDRIVE%%HOMEPATH%

REM set font=%CD%/font/lucsans/euro.7.font

REM replace spaces in filenames by middot characters
set P9FILEWSCHAR=00b7 2022

REM ---------------------------------------------
set cputype=mingw

REM To avoid problems in case PF9 or PLAN9 are used within
REM a sed command, %CD% should be replaced by e.g. e:/foo,
REM or another path not containing \.
set PF9=%CD%
set PLAN9=%PF9%

REM 
set CPUTYPE=%cputype%
set PATH=%PF9%\%cputype%\bin;%PF9%\rc\bin;%PF9%\bin;%PATH%;.
