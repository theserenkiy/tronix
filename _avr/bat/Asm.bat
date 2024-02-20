@echo off

if %3==reset goto skip_asm

c:\Avr\tavrasm\Tavrasmw.exe -i -v -x %1%2 -e c:\Avr\data\%1.lst -o c:\Avr\data\%1.hex
if errorlevel 1 goto asm_err

:skip_asm

if %3==compile   goto end
if %3==prog      goto prog
if %3==reset     goto reset
goto end

:prog
c:\Avr\avrdude\avrdude.exe -p m48 -F -c usbtiny -e -V -B 0.1 -U flash:w:"c:\Avr\data\%1.hex":i
if errorlevel 1 goto prog_err

rem Crystal
rem c:\Avr\avrdude\avrdude.exe -p m48 -F -c usbtiny -e -U flash:w:"c:\Avr\data\%1.hex":i -U lfuse:w:0xEF:m
if errorlevel 1 goto prog_err

goto end

:reset
c:\Avr\avrdude\avrdude.exe -p m48 -c usbtiny
if errorlevel 1 goto prog_err
goto end

:prog_err
echo *******************************************************************************
echo *************************** Programming error(s)! *****************************
echo *******************************************************************************
rem pause >> nul
goto end

:asm_err
echo *******************************************************************************
echo ******************************* Assembler error(s)! ***************************
echo *******************************************************************************
rem pause >> nul

:end
