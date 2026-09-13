# ESP32-S3 build of the demo shell. Needs a sourced ESP-IDF.
#   idf.py set-target esp32s3 && idf.py build && ./merge.sh
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(demo-shell-esp32-s3)
