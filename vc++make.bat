@echo off
Rem *** This batch file can be used to compile with the **free** version of VC++ toolkit
Rem *** See: http://msdn.microsoft.com/visualc/vctoolkit2003/

if "%1"=="" goto :default
goto :%1

:default
cl /o scramble.exe /nologo *.c*
goto :exit


:clean

del scramble.exe
del getopt.obj

:exit