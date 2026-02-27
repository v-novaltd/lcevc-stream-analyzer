# Copyright (C) 2014-2025 V-Nova International Limited
#
# * All rights reserved.
# * This software is licensed under the BSD-3-Clause-Clear License.
# * No patent licenses are granted under this license. For enquiries about patent licenses, please
#   contact legal@v-nova.com.
# * The software is a stand-alone project and is NOT A CONTRIBUTION to any other project.
# * If the software is incorporated into another project, THE TERMS OF THE BSD-3-CLAUSE-CLEAR
#   LICENSE AND THE ADDITIONAL LICENSING INFORMATION CONTAINED IN THIS FILE MUST BE MAINTAINED, AND
#   THE SOFTWARE DOES NOT AND MUST NOT ADOPT THE LICENSE OF THE INCORPORATING PROJECT. However, the
#   software may be incorporated into a project under a compatible license provided the requirements
#   of the BSD-3-Clause-Clear license are respected, and V-Nova International Limited remains
#   licensor of the software ONLY UNDER the BSD-3-Clause-Clear license (not the compatible license).
#
# ANY ONWARD DISTRIBUTION, WHETHER STAND-ALONE OR AS PART OF ANY OTHER PROJECT, REMAINS SUBJECT TO
# THE EXCLUSION OF PATENT LICENSES PROVISION OF THE BSD-3-CLAUSE-CLEAR LICENSE.

macro(DOWNLOAD_FFMPEG FFMPEG_DIR FFMPEG_VERSION)
  if(NOT (WIN32 OR MSYS))
    message(FATAL_ERROR "Official pre-built libraries are not available for macOS.")
  endif()

  if(NOT EXISTS "${FFMPEG_DIR}/include")
    set(FFMPEG_URL
        https://github.com/GyanD/codexffmpeg/releases/download/${FFMPEG_VERSION}/ffmpeg-${FFMPEG_VERSION}-full_build-shared.zip
    )

    message(STATUS "Downloading FFmpeg from ${FFMPEG_URL}")

    file(DOWNLOAD "${FFMPEG_URL}" "${CMAKE_BINARY_DIR}/ffmpeg.zip")

    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf "${CMAKE_BINARY_DIR}/ffmpeg.zip"
                    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")

    file(GLOB EXTRACTED "${CMAKE_BINARY_DIR}/ffmpeg-*")
    list(GET EXTRACTED 0 EXTRACTED_DIR)
    file(RENAME "${EXTRACTED_DIR}" "${FFMPEG_DIR}")
    file(REMOVE "${CMAKE_BINARY_DIR}/ffmpeg.zip")

    install(
      DIRECTORY "${FFMPEG_DIR}/bin"
      DESTINATION "${CMAKE_INSTALL_PREFIX}"
      FILES_MATCHING
      PATTERN "*.dll")

    install(
      DIRECTORY "${FFMPEG_DIR}/bin"
      DESTINATION "${CMAKE_INSTALL_PREFIX}"
      FILES_MATCHING
      PATTERN "*.exe")

  endif()
endmacro()
