# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/lo/Desktop/hackaton/esp-idf/components/bootloader/subproject"
  "/home/lo/Desktop/hackaton/esp-idf/myCode/my-first-project/build/bootloader"
  "/home/lo/Desktop/hackaton/esp-idf/myCode/my-first-project/build/bootloader-prefix"
  "/home/lo/Desktop/hackaton/esp-idf/myCode/my-first-project/build/bootloader-prefix/tmp"
  "/home/lo/Desktop/hackaton/esp-idf/myCode/my-first-project/build/bootloader-prefix/src/bootloader-stamp"
  "/home/lo/Desktop/hackaton/esp-idf/myCode/my-first-project/build/bootloader-prefix/src"
  "/home/lo/Desktop/hackaton/esp-idf/myCode/my-first-project/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/lo/Desktop/hackaton/esp-idf/myCode/my-first-project/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/lo/Desktop/hackaton/esp-idf/myCode/my-first-project/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
