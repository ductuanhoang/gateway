# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "D:/ProjectWork/2.Esp32/esp-idf/components/bootloader/subproject"
  "F:/2.upwork/57.BreakerMater/Code/gateway/build/bootloader"
  "F:/2.upwork/57.BreakerMater/Code/gateway/build/bootloader-prefix"
  "F:/2.upwork/57.BreakerMater/Code/gateway/build/bootloader-prefix/tmp"
  "F:/2.upwork/57.BreakerMater/Code/gateway/build/bootloader-prefix/src/bootloader-stamp"
  "F:/2.upwork/57.BreakerMater/Code/gateway/build/bootloader-prefix/src"
  "F:/2.upwork/57.BreakerMater/Code/gateway/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "F:/2.upwork/57.BreakerMater/Code/gateway/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
