@echo off
::Update this variable to reflect your project filename
::for example: Qwiic_BMP384_v10.ino -> set PTITLE=Qwiic_BMP384_v10
::Omit anything before the fileextension
set PTITLE=RTK_mosaic-X5_WiFi_STN_BT_Prov
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
::set ESPTOOL="C:\Users\public\Documents\GitHub\qcvault-live\UTILITIES\Espressif\esptool.exe"
::set ESPTOOL="C:\Program Files (x86)\Arduino\hardware\espressif\esp32\tools\esptool\esptool.exe"
set ESPTOOL="C:\Users\%USERNAME%\AppData\Local\Arduino15\packages\esp32\tools\esptool_py\4.5.1\esptool.exe"

call %ESPTOOL% --chip esp32 -p %COMPORT% -b 460800 --before=default_reset --after=hard_reset write_flash --flash_mode dio --flash_freq 40m --flash_size 4MB 0x1000 %BOOTLOADER% 0x10000 %FIRMWARE% 0x8000 %PARTITIONS%