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

set ESPTOOL="esptool.exe"

call %ESPTOOL% --chip esp32 -p %COMPORT% -b 460800 erase_flash