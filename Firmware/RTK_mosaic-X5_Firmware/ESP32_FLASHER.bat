@echo off
set PTITLE=RTK_mosaic-X5_Firmware
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

if [%1]==[] goto findPort

set COMPORT=%1
goto program

:findPort

for /f "tokens=2 delims=(" %%a in ('wmic path win32_pnpentity get caption /format:list ^| find "COM" ^| find "CH340"') do (
    for /f "tokens=1 delims=)" %%b in ("%%a") do (
        set COMPORT=%%b
    )
)

:program

set BOOTLOADER="build\bootloader\bootloader.bin"
set PARTITIONS="build\partition_table\partition-table.bin"
set FIRMWARE="build\%PTITLE%.bin"
set ESPTOOL="esptool.exe"

call %ESPTOOL% --chip esp32 -p %COMPORT% -b 460800 --before=default_reset --after=hard_reset write_flash --flash_mode dio --flash_freq 40m --flash_size 4MB 0x1000 %BOOTLOADER% 0x10000 %FIRMWARE% 0x8000 %PARTITIONS%