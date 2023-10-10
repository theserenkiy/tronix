esptool.py --port COM7 erase_flash
esptool.py --chip esp32 --port COM7 write_flash -z 0x1000 ESP32_GENERIC-20230426-v1.20.0.bin