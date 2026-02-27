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

macro(download_gtest)
  # Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24:
  if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
  endif()

  include(FetchContent)

  set(INSTALL_GTEST
      OFF
      CACHE BOOL "" FORCE)

  FetchContent_Declare(
    googletest # Specify the commit you depend on and update it regularly.
    URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip)
  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt
      ON
      CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endmacro()
