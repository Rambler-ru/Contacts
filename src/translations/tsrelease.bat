@echo off

if '%1'=='' (set LANG=??) ELSE (set LANG=%1)

set TS_DIR=.
set QM_DIR=.

FOR /D %%d IN (%TS_DIR%/%LANG%) DO FOR %%t IN (%TS_DIR%/%%d/*.ts) DO lrelease -compress %TS_DIR%/%%d/%%t -qm %QM_DIR%/%%d/%%~nt.qm
