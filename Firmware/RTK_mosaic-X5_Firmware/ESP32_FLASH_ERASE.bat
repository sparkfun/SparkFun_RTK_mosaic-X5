@echo off

if [%1]==[] goto findPort

set COMPORT=%1
goto erase

:findPort

for /f "tokens=2 delims=(" %%a in ('wmic path win32_pnpentity get caption /format:list ^| find "COM" ^| find "CH340"') do (
    for /f "tokens=1 delims=)" %%b in ("%%a") do (
        set COMPORT=%%b
    )
)

:erase

::set ESPTOOL="C:\Users\public\Documents\GitHub\qcvault-live\UTILITIES\Espressif\esptool.exe"
::set ESPTOOL="C:\Program Files (x86)\Arduino\hardware\espressif\esp32\tools\esptool\esptool.exe"
set ESPTOOL="C:\Users\%USERNAME%\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\4.5.1\esptool.exe"

call %ESPTOOL% --chip esp32 -p %COMPORT% -b 460800 erase_flash