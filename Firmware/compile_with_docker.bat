::Uncomment "docker builder prune -f" below to clear the build cache
::docker builder prune -f
docker build -t rtk_mosaic-x5_firmware --progress=plain --no-cache-filter deployment .
docker create --name=rtk_mosaic-x5 rtk_mosaic-x5_firmware:latest
docker cp rtk_mosaic-x5:/RTK_mosaic-X5_Firmware.bin ./RTK_mosaic-X5_Firmware/build/RTK_mosaic-X5_Firmware.bin
docker cp rtk_mosaic-x5:/bootloader.bin ./RTK_mosaic-X5_Firmware/build/bootloader/bootloader.bin
docker cp rtk_mosaic-x5:/partition-table.bin ./RTK_mosaic-X5_Firmware/build/partition_table/partition-table.bin
docker container rm rtk_mosaic-x5
