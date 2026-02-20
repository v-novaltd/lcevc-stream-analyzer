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

set(EXE_SOURCES_CPP "src/app/main.cpp")
set(SOURCES_CPP
    "src/app/analyze.cpp"
    "src/app/config.cpp"
    "src/app/simulate_buffer.cpp"
    "src/extractor/extractor.cpp"
    "src/extractor/extractor_bin.cpp"
    "src/extractor/extractor_demuxer.cpp"
    "src/extractor/extractor_lcevc.cpp"
    "src/extractor/extractor_mp4.cpp"
    "src/extractor/extractor_sei.cpp"
    "src/helper/entropy_decoder.cpp"
    "src/helper/frame_timestamp.cpp"
    "src/helper/input_detection.cpp"
    "src/helper/lcevc_bin.cpp"
    "src/helper/nal_unit.cpp"
    "src/helper/reorder.cpp"
    "src/helper/stream_reader.cpp"
    "src/parser/parsed_frame.cpp"
    "src/parser/parser.cpp"
    "src/simulate_buffer/simulate_buffer_algorithm.cpp"
    "src/sink/sink.cpp"
    "src/sink/sink_bin.cpp"
    "src/sink/sink_raw.cpp"
    "src/utility/file_io.cpp"
    "src/utility/json_util.cpp"
    "src/utility/output_util.cpp"
    "src/utility/platform.cpp"
    #
)

set(SOURCES_C)

set(SOURCES_H
    "src/app/analyze.h"
    "src/app/config.h"
    "src/app/config_types.h"
    "src/app/simulate_buffer.h"
    "src/extractor/extractor.h"
    "src/extractor/extractor_bin.h"
    "src/extractor/extractor_demuxer.h"
    "src/extractor/extractor_lcevc.h"
    "src/extractor/extractor_mp4.h"
    "src/extractor/extractor_sei.h"
    "src/helper/base_type.h"
    "src/helper/entropy_decoder.h"
    "src/helper/extracted_frame.h"
    "src/helper/frame_timestamp.h"
    "src/helper/input_detection.h"
    "src/helper/lcevc_bin.h"
    "src/helper/nal_unit.h"
    "src/helper/reorder.h"
    "src/helper/stream_reader.h"
    "src/parser/parsed_frame.h"
    "src/parser/parsed_types.h"
    "src/parser/parser.h"
    "src/simulate_buffer/simulate_buffer_algorithm.h"
    "src/simulate_buffer/simulate_buffer_algorithm_internal.h"
    "src/sink/sink.h"
    "src/sink/sink_bin.h"
    "src/sink/sink_null.h"
    "src/sink/sink_raw.h"
    "src/utility/bit_field.h"
    "src/utility/byte_order.h"
    "src/utility/enum_map.h"
    "src/utility/file_io.h"
    "src/utility/format_attribute.h"
    "src/utility/json_util.h"
    "src/utility/log_util.h"
    "src/utility/math_util.h"
    "src/utility/multi_byte.h"
    "src/utility/output_util.h"
    "src/utility/platform.h"
    "src/utility/span.h"
    "src/utility/types_util.h"
    "src/utility/unsigned_exp_golomb.h"
    "src/utility/value_string_map.h"
    #
)

set(SOURCES_ALL_FILES ${SOURCES_CPP} ${SOURCES_C} ${SOURCES_H})

# -------------------------------------------------------------------------------
# Group sources for IDE
# -------------------------------------------------------------------------------

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES_ALL_FILES})

# -------------------------------------------------------------------------------
# Export
# -------------------------------------------------------------------------------

set(SOURCES "CMakeLists.txt" "CMakeSources.cmake" ${SOURCES_ALL_FILES})

set(INTERFACE_INCLUDE_DIRS)
