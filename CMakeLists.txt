#[[
Copyright (C) 2014-2026 V-Nova International Limited

    * All rights reserved.
    * This software is licensed under the BSD-3-Clause-Clear License.
    * No patent licenses are granted under this license. For enquiries about patent
      licenses, please contact legal@v-nova.com.
    * The software is a stand-alone project and is NOT A CONTRIBUTION to any other
      project.
    * If the software is incorporated into another project, THE TERMS OF THE
      BSD-3-CLAUSE-CLEAR LICENSE AND THE ADDITIONAL LICENSING INFORMATION CONTAINED
      IN THIS FILE MUST BE MAINTAINED, AND THE SOFTWARE DOES NOT AND MUST NOT ADOPT
      THE LICENSE OF THE INCORPORATING PROJECT. However, the software may be
      incorporated into a project under a compatible license provided the
      requirements of the BSD-3-Clause-Clear license are respected, and V-Nova
      International Limited remains licensor of the software ONLY UNDER the
      BSD-3-Clause-Clear license (not the compatible license).

ANY ONWARD DISTRIBUTION, WHETHER STAND-ALONE OR AS PART OF ANY OTHER PROJECT,
REMAINS SUBJECT TO THE EXCLUSION OF PATENT LICENSES PROVISION OF THE
BSD-3-CLAUSE-CLEAR LICENSE.
]]

cmake_minimum_required(VERSION 3.19)

option(VN_BUILD_BINARIES "Build lcevc_stream_analyzer. Not required for unit tests." ON)
option(VN_FFMPEG_DOWNLOAD "Download ffmpeg/libav source and build." ON)
option(VN_GTEST_DOWNLOAD "Download gtest source and build." ON)
option(VN_BUILD_UNIT_TESTS "Build unit tests. Requires gtest library from download or pkg-config."
       ON)

include(cmake/project_version.cmake)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
  string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR})
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR})
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR})
endforeach(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)

message(STATUS "Using clang-tidy with settings (disabled if blank): ${CMAKE_CXX_CLANG_TIDY}")

include(CMakeSources.cmake)

project(
  lcevc_stream_analyzer
  VERSION "${PROJECT_VERSION}"
  LANGUAGES CXX)
add_compile_definitions(VERSION_STRING="${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Wno-gnu-zero-variadic-macro-arguments
                      -fmacro-prefix-map=${CMAKE_CURRENT_LIST_DIR}/src/=)

  if(NOT WIN32 AND NOT MSYS)
    # TODO: Add -Werror here to get warnings as errors.
    add_compile_options(-Werror)
  endif()

elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  # TODO: Add /WX here to get warnings as errors.
  add_compile_options(/W4)
else()
  message(FATAL_ERROR "Unrecognized compiler ${CMAKE_CXX_COMPILER_ID}")
endif()

message("VN_FFMPEG_DOWNLOAD: ${VN_FFMPEG_DOWNLOAD}")

# Define the core library
add_library(stream_analyzer ${SOURCES})

# Include directories
target_include_directories(stream_analyzer PUBLIC "src")
target_include_directories(stream_analyzer PUBLIC "third_party")

set(FFMPEG_VERSION 8.0)

# Optional FFmpeg download fallback aligned with guide
if(VN_FFMPEG_DOWNLOAD)
  if(WIN32 OR MSYS)
    message(STATUS "Downloading ffmpeg binaries")
    include(cmake/download_ffmpeg.cmake)
    set(FFMPEG_DIR "${CMAKE_BINARY_DIR}/ffmpeg")
    download_ffmpeg("${FFMPEG_DIR}" "${FFMPEG_VERSION}")

    target_include_directories(stream_analyzer PUBLIC "${FFMPEG_DIR}/include")
    target_link_directories(stream_analyzer PUBLIC "${FFMPEG_DIR}/lib")
    target_link_libraries(stream_analyzer PUBLIC avcodec avformat avutil swscale swresample)

  else()
    message(STATUS "Building ffmpeg from source")
    include(cmake/build_ffmpeg.cmake)
    add_dependencies(stream_analyzer ffmpeg_external)
    target_link_libraries(stream_analyzer PUBLIC FFmpeg::avformat FFmpeg::avcodec FFmpeg::avutil
                                                 FFmpeg::swresample FFmpeg::swscale)
  endif()

else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(
    LIBAV
    REQUIRED
    IMPORTED_TARGET
    libavformat
    libavcodec
    libswresample
    libavutil)
  target_link_libraries(stream_analyzer PUBLIC PkgConfig::LIBAV)

endif()

if(WIN32 OR MSYS)
  target_link_libraries(stream_analyzer PUBLIC ws2_32)
else()
  target_link_libraries(stream_analyzer PUBLIC dl)
endif()

if(VN_BUILD_BINARIES)
  add_executable(lcevc_stream_analyzer ${EXE_SOURCES_CPP})
  target_link_libraries(lcevc_stream_analyzer PUBLIC stream_analyzer)
  install(TARGETS lcevc_stream_analyzer)
endif()

if(VN_BUILD_UNIT_TESTS)
  message(STATUS "Unit tests enabled")
  message(STATUS "ctest args: ${CMAKE_CTEST_ARGUMENTS}")
  include(CTest)
  enable_testing()

  if(VN_GTEST_DOWNLOAD)
    message(STATUS "Downloading gtest using fetch content")
    include(cmake/download_gtest.cmake)
    download_gtest()
  else()
    find_package(GTest REQUIRED)
  endif()

  add_subdirectory(tests/unit)

else()
  message(STATUS "Unit tests skipped")
endif()

install(DIRECTORY third_party/LICENSES DESTINATION third_party)
install(FILES LICENSE DESTINATION ".")

set_target_properties(lcevc_stream_analyzer PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(CPACK_PACKAGE_FILE_NAME
    "${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}-${CMAKE_BUILD_TYPE}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}"
)

set(CPACK_PACKAGE_NAME "${CMAKE_PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${CMAKE_PROJECT_VERSION}")
if(WIN32 OR MSYS)
  set(CPACK_GENERATOR "ZIP")
else()
  set(CPACK_GENERATOR "TGZ")
endif()
include(CPack)
