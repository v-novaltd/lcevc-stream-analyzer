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

include(GoogleTest)

add_executable(
  unit_tests #
  multi_byte_decode_test.cpp
  nal_header_test.cpp
  input_detection_test.cpp
  stream_reader_test.cpp
  simulate_buffer_algorithm_test.cpp
  math_util_test.cpp
  timestamp_validation_test.cpp)

target_link_libraries(
  unit_tests
  PUBLIC stream_analyzer
  PRIVATE GTest::gtest GTest::gtest_main)

gtest_discover_tests(unit_tests)
