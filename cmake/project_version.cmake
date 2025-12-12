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

# Ensure VERSION file exists and is valid, and track it for reconfigure
set(PROJECT_VERSION_FILE "${CMAKE_SOURCE_DIR}/VERSION")
if(NOT EXISTS "${PROJECT_VERSION_FILE}")
  message(FATAL_ERROR "VERSION file not found at ${PROJECT_VERSION_FILE}")
endif()

file(STRINGS "${PROJECT_VERSION_FILE}" PROJECT_VERSION LIMIT_COUNT 1)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)
if(PROJECT_VERSION STREQUAL "")
  message(FATAL_ERROR "VERSION file is empty; please specify a version string")
endif()

# Copy VERSION into build dir so changes trigger configure
configure_file("${PROJECT_VERSION_FILE}" "${CMAKE_BINARY_DIR}/VERSION" COPYONLY)
