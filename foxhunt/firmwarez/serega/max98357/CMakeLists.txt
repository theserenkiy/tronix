# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# idf.py storage-flash  <- to flash files

project(max98357)

# set(PARTITION_TABLE "partitions.csv")
# littlefs_create_partition_image(storage filesystem)  # FLASH_IN_PROJECT)
