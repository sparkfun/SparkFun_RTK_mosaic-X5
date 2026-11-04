::@echo off
set PTITLE=RTK_mosaic-X5_Firmware
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

if [%1]==[] goto findPort

set COMPORT=%1
goto program

:findPort

for /f "delims=" %%A in ('powershell -Command "Get-CimInstance Win32_PnPEntity | Where-Object { $_.Name -match '\(COM[0-9]+\)' } | Where-Object { $_.Name -match 'CH340' } | ForEach-Object { if ($_ -match '\(COM[0-9]+\)') { $matches[0].Trim('()') } }"') do (
    set COMPORT=%%A
)

:program

set BOOTLOADER="RTK_mosaic-X5_Firmware\build\bootloader\bootloader.bin"
set PARTITIONS="RTK_mosaic-X5_Firmware\build\partition_table\partition-table.bin"
set FIRMWARE="RTK_mosaic-X5_Firmware\build\%PTITLE%.bin"
set ESPTOOL="esptool.exe"

call %ESPTOOL% --chip esp32 -p %COMPORT% -b 460800 --before=default_reset --after=hard_reset write_flash --flash_mode dio --flash_freq 40m --flash_size 4MB 0x1000 %BOOTLOADER% 0x10000 %FIRMWARE% 0x8000 %PARTITIONS%