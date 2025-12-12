# Copyright (C) 2014-2025 V-Nova International Limited
#
#     * All rights reserved.
#     * This software is licensed under the BSD-3-Clause-Clear License.
#     * No patent licenses are granted under this license. For enquiries about patent licenses,
#       please contact legal@v-nova.com.
#     * The software is a stand-alone project and is NOT A CONTRIBUTION to any other project.
#     * If the software is incorporated into another project, THE TERMS OF THE BSD-3-CLAUSE-CLEAR
#       LICENSE AND THE ADDITIONAL LICENSING INFORMATION CONTAINED IN THIS FILE MUST BE MAINTAINED,
#       AND THE SOFTWARE DOES NOT AND MUST NOT ADOPT THE LICENSE OF THE INCORPORATING PROJECT.
#       However, the software may be incorporated into a project under a compatible license
# provided the requirements of the BSD-3-Clause-Clear license are respected, and V-Nova
# International Limited remains licensor of the software ONLY UNDER the BSD-3-Clause-Clear license
# (not the compatible license).
#
# ANY ONWARD DISTRIBUTION, WHETHER STAND-ALONE OR AS PART OF ANY OTHER PROJECT, REMAINS SUBJECT TO
# THE EXCLUSION OF PATENT LICENSES PROVISION OF THE BSD-3-CLAUSE-CLEAR LICENSE.

set(SOURCES_CPP
        "src/config.cpp"
        "src/utility/base_type.cpp"
        "src/utility/input_detection.cpp"
        "src/utility/nal_header.cpp"
        "src/utility/nal_unit.cpp"
        "src/utility/platform.cpp"
        "src/utility/string_util.cpp"
        "src/bin/lcevc_bin.cpp"
        "src/io/file_io.cpp"
        "src/extractor/extractor.cpp"
        "src/extractor/extractor_demuxer.cpp"
        "src/extractor/extractor_sei.cpp"
        "src/extractor/extractor_mp4.cpp"
        "src/extractor/extractor_bin.cpp"
        "src/extractor/extractor_lcevc.cpp"
        "src/helper/entropy_decoder.cpp"
        "src/helper/reorder.cpp"
        "src/helper/stream_reader.cpp"
        "src/main.cpp"
        "src/parser/parser.cpp"
        "src/sink/sink.cpp"
        "src/sink/sink_bin.cpp"
        "src/sink/sink_raw.cpp"
)

set(SOURCES_C

)

set(SOURCES_H
        "src/config.h"
        "src/utility/base_type.h"
        "src/utility/bit_field.h"
        "src/utility/byte_order.h"
        "src/utility/enum_map.h"
        "src/utility/format_attribute.h"
        "src/utility/input_detection.h"
        "src/utility/log_util.h"
        "src/utility/math_util.h"
        "src/utility/multi_byte.h"
        "src/utility/nal_header.h"
        "src/utility/nal_unit.h"
        "src/utility/platform.h"
        "src/utility/string_util.h"
        "src/utility/types_util.h"
        "src/utility/unsigned_exp_golomb.h"
        "src/utility/value_string_map.h"
        "src/bin/lcevc_bin.h"
        "src/io/file_io.h"
        "src/extractor/extractor.h"
        "src/extractor/extractor_demuxer.h"
        "src/extractor/extractor_sei.h"
        "src/extractor/extractor_mp4.h"
        "src/extractor/extractor_bin.h"
        "src/extractor/extractor_lcevc.h"
        "src/helper/entropy_decoder.h"
        "src/helper/reorder.h"
        "src/helper/stream_reader.h"
        "src/helper/frame_queue.h"
        "src/parser/parser.h"
        "src/sink/sink.h"
        "src/sink/sink_bin.h"
        "src/sink/sink_null.h"
        "src/sink/sink_raw.h"
)

set(SOURCES_ALL_FILES
        ${SOURCES_CPP}
        ${SOURCES_C}
        ${SOURCES_H}
)

#-------------------------------------------------------------------------------
# Group sources for IDE
#-------------------------------------------------------------------------------

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES_ALL_FILES})

#-------------------------------------------------------------------------------
# Export
#-------------------------------------------------------------------------------

set(SOURCES
        "CMakeLists.txt"
        "CMakeSources.cmake"
        ${SOURCES_ALL_FILES}
)

set(INTERFACE_INCLUDE_DIRS

)
