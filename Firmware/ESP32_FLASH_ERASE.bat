@echo off

if [%1]==[] goto findPort

set COMPORT=%1
goto erase

:findPort

for /f "delims=" %%A in ('powershell -Command "Get-CimInstance Win32_PnPEntity | Where-Object { $_.Name -match '\(COM[0-9]+\)' } | Where-Object { $_.Name -match 'CH340' } | ForEach-Object { if ($_ -match '\(COM[0-9]+\)') { $matches[0].Trim('()') } }"') do (
    set COMPORT=%%A
)

:erase

set ESPTOOL="esptool.exe"

call %ESPTOOL% --chip esp32 -p %COMPORT% -b 460800 erase_flash