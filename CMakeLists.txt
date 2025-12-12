#[[
Copyright (C) 2014-2025 V-Nova International Limited

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

option(VN_BUILD_BINARIES
       "Build lcevc_stream_analyzer. Not required for unit tests." ON)
option(VN_FFMPEG_DOWNLOAD
       "Download prebuilt ffmpeg/libav binaries if not found." ON)
option(VN_BUILD_UNIT_TESTS
       "Build unit tests. Requires gtest library from Conan or pkg-config." OFF)

include(cmake/project_version.cmake)

message(
  STATUS
    "Using clang-tidy with settings (disabled if blank): ${CMAKE_CXX_CLANG_TIDY}"
)

include(CMakeSources.cmake)

project(
  lcevc_stream_analyzer
  VERSION "${PROJECT_VERSION}"
  LANGUAGES CXX)
add_compile_definitions(VERSION_STRING="${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)

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

# If Conan has generated CMake paths, include them so find_package works.
if(EXISTS "${CMAKE_BINARY_DIR}/../conan_paths.cmake")
  include("${CMAKE_BINARY_DIR}/../conan_paths.cmake")
  set(VN_FFMPEG_DOWNLOAD OFF)
endif()

message("VN_FFMPEG_DOWNLOAD: ${VN_FFMPEG_DOWNLOAD}")

if(VN_BUILD_BINARIES)
  # Define the executable
  add_executable(lcevc_stream_analyzer ${SOURCES})

  # Include directories
  target_include_directories(lcevc_stream_analyzer
                             PUBLIC "${CMAKE_CURRENT_LIST_DIR}/src")
  target_include_directories(lcevc_stream_analyzer
                             PUBLIC "${CMAKE_CURRENT_LIST_DIR}/third_party")

  # Optional FFmpeg download fallback aligned with guide
  find_package(ffmpeg)
  if(${ffmpeg_FOUND})
    message(
      STATUS "Did not need to download FFmpeg binaries, as find_package worked")
    target_link_libraries(
      lcevc_stream_analyzer
      PUBLIC ffmpeg::avcodec ffmpeg::avformat ffmpeg::avutil ffmpeg::swscale
             ffmpeg::swresample)
    if(WIN32 OR MSYS)
      install(
        DIRECTORY "${ffmpeg_avcodec_LIB_DIRS}/../bin"
        DESTINATION "${CMAKE_INSTALL_PREFIX}"
        FILES_MATCHING
        PATTERN "*.dll")
    else()
      file(
        GLOB FFMPEG_LIB_GLOB
        LIST_DIRECTORIES true
        "${ffmpeg_avcodec_LIB_DIRS}/*.so*")
      install(FILES ${FFMPEG_LIB_GLOB} TYPE LIB)
    endif()

  elseif(VN_FFMPEG_DOWNLOAD AND NOT APPLE)
    message(STATUS "Downloading FFMPEG binaries")
    include(cmake/download_ffmpeg.cmake)
    set(FFMPEG_DIR "${CMAKE_BINARY_DIR}/ffmpeg")
    download_ffmpeg("${FFMPEG_DIR}" 7.1)
    target_include_directories(lcevc_stream_analyzer
                               PUBLIC "${FFMPEG_DIR}/include")

    target_link_directories(lcevc_stream_analyzer PUBLIC "${FFMPEG_DIR}/lib")

    target_link_libraries(lcevc_stream_analyzer PUBLIC avcodec avformat avutil
                                                       swscale swresample)

    if(WIN32 OR MSYS)
      install(
        DIRECTORY "${FFMPEG_DIR}/bin"
        DESTINATION "${CMAKE_INSTALL_PREFIX}"
        FILES_MATCHING
        PATTERN "*.dll")
    else()
      file(
        GLOB FFMPEG_LIB_GLOB
        LIST_DIRECTORIES true
        "${FFMPEG_DIR}/lib/*.so*")
      install(FILES ${FFMPEG_LIB_GLOB} TYPE LIB)
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
    target_link_libraries(lcevc_stream_analyzer PUBLIC PkgConfig::LIBAV)

  endif()

  if(WIN32 OR MSYS)
    target_link_libraries(lcevc_stream_analyzer PUBLIC ws2_32)
  else()
    target_link_libraries(lcevc_stream_analyzer PUBLIC dl)
  endif()

  install(TARGETS lcevc_stream_analyzer)
endif()

if(VN_BUILD_UNIT_TESTS)
  message(STATUS "Unit tests enabled")
  message(STATUS "ctest args: ${CMAKE_CTEST_ARGUMENTS}")
  include(CTest)
  enable_testing()

  if(WIN32)
    include(cmake/download_gtest.cmake)
    download_gtest()
  else()
    find_package(GTest REQUIRED)
  endif()

  add_subdirectory(tests)

else()
  message(STATUS "Unit tests skipped")
endif()
