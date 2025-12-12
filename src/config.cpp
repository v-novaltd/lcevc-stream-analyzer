/*
 * Copyright (C) 2014-2025 V-Nova International Limited
 *
 *     * All rights reserved.
 *     * This software is licensed under the BSD-3-Clause-Clear License.
 *     * No patent licenses are granted under this license. For enquiries about patent
 *       licenses, please contact legal@v-nova.com.
 *     * The software is a stand-alone project and is NOT A CONTRIBUTION to any other
 *       project.
 *     * If the software is incorporated into another project, THE TERMS OF THE
 *       BSD-3-CLAUSE-CLEAR LICENSE AND THE ADDITIONAL LICENSING INFORMATION CONTAINED
 *       IN THIS FILE MUST BE MAINTAINED, AND THE SOFTWARE DOES NOT AND MUST NOT ADOPT
 *       THE LICENSE OF THE INCORPORATING PROJECT. However, the software may be
 *       incorporated into a project under a compatible license provided the
 *       requirements of the BSD-3-Clause-Clear license are respected, and V-Nova
 *       International Limited remains licensor of the software ONLY UNDER the
 *       BSD-3-Clause-Clear license (not the compatible license).
 *
 * ANY ONWARD DISTRIBUTION, WHETHER STAND-ALONE OR AS PART OF ANY OTHER PROJECT,
 * REMAINS SUBJECT TO THE EXCLUSION OF PATENT LICENSES PROVISION OF THE
 * BSD-3-CLAUSE-CLEAR LICENSE.
 */
#include "config.h"

#include "cli11.hpp"
#include "utility/base_type.h"
#include "utility/enum_map.h"
#include "utility/input_detection.h"
#include "utility/log_util.h"

#include <optional>

using namespace vnova::utility;

namespace vnova::analyzer {

namespace {
    // clang-format off
        const auto kInputTypeMap = EnumMap<InputType>
            (InputType::TS,  "ts")
            (InputType::ELEMENTARY, "es")
            (InputType::WEBM, "webm")
            (InputType::MP4, "mp4")
            (InputType::BIN, "bin")
            (InputType::LCEVC, "lcevc");

        const auto kOutputTypeMap = EnumMap<OutputType>
            (OutputType::Bin, "bin")
            (OutputType::Raw, "raw")
            (OutputType::LengthDelimited, "length_delimited");

        const auto kBaseTypeMap = EnumMap<utility::BaseType::Enum>
            ( utility::BaseType::Enum::H264, "h264" )
            ( utility::BaseType::Enum::HEVC, "hevc" )
            ( utility::BaseType::Enum::VVC, "vvc" )
            ( utility::BaseType::Enum::VP8, "vp8" )
            ( utility::BaseType::Enum::VP9, "vp9" )
            ( utility::BaseType::Enum::AV1, "av1" )
            ( utility::BaseType::Enum::VC6, "vc6" );

        const auto kLogFormatMap = EnumMap<LogFormat>
            ( LogFormat::Text, "log" )
            ( LogFormat::JSON, "json" );
    // clang-format on

    const std::map<std::string, std::pair<InputType, utility::BaseType::Enum>, std::less<>> kSupportedFileTypes = {
        /** TRANSPORT STREAM */
        {"ts", {InputType::TS, utility::BaseType::Enum::Invalid}},

        /** ELEMENTARY STREAM */
        {"es", {InputType::ELEMENTARY, utility::BaseType::Enum::Invalid}},
        {"264", {InputType::ELEMENTARY, utility::BaseType::Enum::H264}},
        {"h264", {InputType::ELEMENTARY, utility::BaseType::Enum::H264}},
        {"avc", {InputType::ELEMENTARY, utility::BaseType::Enum::H264}},
        {"265", {InputType::ELEMENTARY, utility::BaseType::Enum::HEVC}},
        {"h265", {InputType::ELEMENTARY, utility::BaseType::Enum::HEVC}},
        {"hevc", {InputType::ELEMENTARY, utility::BaseType::Enum::HEVC}},
        {"266", {InputType::ELEMENTARY, utility::BaseType::Enum::VVC}},
        {"h266", {InputType::ELEMENTARY, utility::BaseType::Enum::VVC}},
        {"vvc", {InputType::ELEMENTARY, utility::BaseType::Enum::VVC}},

        /** WEBM */
        {"webm", {InputType::WEBM, utility::BaseType::Enum::Unknown}},

        /**  MP4  */
        {"mp4", {InputType::MP4, utility::BaseType::Enum::Unknown}},

        /** BIN  */
        {"bin", {InputType::BIN, utility::BaseType::Enum::Unknown}},

        /** RAW LCEVC ANNEX B  */
        {"lvc", {InputType::LCEVC, utility::BaseType::Enum::Unknown}},
        {"lcevc", {InputType::LCEVC, utility::BaseType::Enum::Unknown}}};

    bool inputTypeRequiresBaseType(InputType type)
    {
        switch (type) {
            case InputType::TS:
            case InputType::BIN:
            case InputType::LCEVC:
            case InputType::WEBM: return false;
            case InputType::ELEMENTARY:
            case InputType::MP4:
            case InputType::Unknown: return true;
        }
        return true;
    }

    bool baseTypeIsValid(utility::BaseType::Enum type)
    {
        return (type != utility::BaseType::Enum::Unknown && type != utility::BaseType::Enum::Invalid);
    }
} // namespace

std::optional<const Config> parseArgs(CLI::App& app, int argc, char* argv[])
{
    Config config;

    app.add_option("-i,--input", config.inputPath, "Input source file")->required()->check(CLI::ExistingFile);
    app.add_option("-t,--type", config.inputType, "Override input file type")
        ->transform(CLI::CheckedTransformer(kInputTypeMap.nameToValueMap(InputType::Unknown),
                                            CLI::ignore_case));
    app.add_option("-p,--ts_pid", config.tsPID,
                   " The transport-stream PID that the LCEVC data resides. "
                   "Default -1 will automatically find the first PID stream containing LCEVC data");

    auto* outputPath =
        app.add_option("-o,--output", config.outputPath, "Path to write output data (not log)");
    auto* outputType = app.add_option("-u,--output_type", config.outputType, "The output file type")
                           ->transform(CLI::CheckedTransformer(
                               kOutputTypeMap.nameToValueMap(OutputType::Unknown), CLI::ignore_case));
    outputType->needs(outputPath);

    const auto* baseTypeOpt =
        app.add_option("-b,--base_type", config.baseType, "The base video type")
            ->transform(CLI::CheckedTransformer(
                kBaseTypeMap.nameToValueMap(utility::BaseType::Enum::Unknown), CLI::ignore_case));

    app.add_option("-v,--verbose", config.verboseOutput, "Enable verbose output during LCEVC parsing");

    app.add_option("-l,--log", config.logPath, "Output log file path");
    const auto* formatOpt =
        app.add_option("-f,--format", config.logFormat, "Output log format. Default log")
            ->transform(CLI::CheckedTransformer(kLogFormatMap.nameToValueMap(LogFormat::Text),
                                                CLI::ignore_case));

    try {
        app.parse(argc, argv);
    } catch (const CLI::CallForHelp&) {
        fprintf(stderr, "%s\n", app.help().c_str());
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        exit(static_cast<int>(ReturnCode::Success));
    } catch (const CLI::CallForVersion&) {
        fprintf(stderr, "%s v%s\n", app.get_name().c_str(), VERSION_STRING);
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        exit(static_cast<int>(ReturnCode::Success));
    } catch (const CLI::ParseError& e) {
        VNLog::Error("CLI parsing error: %s - %s\n", e.get_name().c_str(), e.what());
        return std::nullopt;
    }

    using FileTypePair = std::pair<InputType, utility::BaseType::Enum>;
    FileTypePair typesFromFileSuffix = {InputType::Unknown, BaseType::Enum::Invalid};

    const bool userSpecifiedBaseType = !baseTypeOpt->empty();
    const bool userSpecifiedOutputType = !outputType->empty();
    const bool userSpecifiedLogFormat = !formatOpt->empty();

    // Extract the format of the input file
    const std::string inputTypeName = utility::string::pathExtension(config.inputPath);

    // Try to deduce file types from input path extension
    if (config.inputType == InputType::Unknown) {
        if (const auto foundType = kSupportedFileTypes.find(inputTypeName);
            foundType != kSupportedFileTypes.end()) {
            typesFromFileSuffix = foundType->second;
            config.inputType = typesFromFileSuffix.first;
            VNLog::Info("Inferred inputType to %d\n", config.inputType);
        } else {
            VNLog::Error("Could not infer file type for \"%s\".\n", inputTypeName.c_str());
            VNLog::Error(
                "Only the following types can be inferred, set inputType manually otherwise: ");
            for (const auto& [file, detail] : kSupportedFileTypes) {
                VNLog::Error("%s, ", file.c_str());
            }
            VNLog::Error("\n");
        }
    }

    DetectedInputFormat detectedFormat;
    const bool detectedFromContent = detectInputFormatFromFile(config.inputPath, detectedFormat);
    if (detectedFromContent && detectedFormat.inputType != InputType::Unknown &&
        config.inputType != InputType::Unknown && config.inputType != detectedFormat.inputType) {
        VNLog::Info("Warning: extension suggests %d but byte analysis found %d\n", config.inputType,
                    detectedFormat.inputType);
    }

    if (config.inputType == InputType::Unknown && detectedFromContent) {
        config.inputType = detectedFormat.inputType;
    }

    if (config.inputType == InputType::Unknown) {
        VNLog::Error("Unrecognized input type [%s]\n", inputTypeName.c_str());
        return std::nullopt;
    }

    if (!userSpecifiedBaseType) {
        if (detectedFromContent && detectedFormat.baseType != BaseType::Enum::Invalid) {
            config.baseType = detectedFormat.baseType;
        } else if (typesFromFileSuffix.second != BaseType::Enum::Invalid) {
            config.baseType = typesFromFileSuffix.second;
        }
    } else if (detectedFromContent && detectedFormat.baseType != BaseType::Enum::Invalid &&
               config.baseType != detectedFormat.baseType) {
        VNLog::Info("Warning: user-specified base type %d disagrees with detected %d\n",
                    config.baseType, detectedFormat.baseType);
    }

    if (inputTypeRequiresBaseType(config.inputType) && (baseTypeIsValid(config.baseType) == false)) {
        VNLog::Error("Base type must be specified for the input type selected [%s]\n",
                     inputTypeName.c_str());
        return std::nullopt;
    }

    if (userSpecifiedOutputType && config.outputPath.empty()) {
        VNLog::Error("Output type specified but no output path provided. Please supply -o.\n");
        return std::nullopt;
    }

    // Infer output type when path provided but type omitted
    if (!config.outputPath.empty() && !userSpecifiedOutputType && config.outputType == OutputType::Unknown) {
        const std::string outputExt = utility::string::pathExtension(config.outputPath);
        if (outputExt == "bin") {
            config.outputType = OutputType::Bin;
        } else if (outputExt == "lvc" || outputExt == "lcevc") {
            config.outputType = OutputType::Raw;
        } else {
            VNLog::Error("Failed to infer output type from extension .%s. Please specify -u.\n",
                         outputExt.c_str());
            return std::nullopt;
        }
    }

    // Infer log format if user didn't specify one but provided a log path
    if (!config.logPath.empty() && !userSpecifiedLogFormat) {
        const std::string logExt = utility::string::pathExtension(config.logPath);
        if (logExt == "json") {
            config.logFormat = LogFormat::JSON;
        } else if (logExt == "log" || logExt == "txt") {
            config.logFormat = LogFormat::Text;
        } else {
            VNLog::Error("Failed to infer log format from log file extension .%s. Please specify "
                         "-f [json|log].\n",
                         logExt.c_str());
            return std::nullopt;
        }
    }

    return config;
}

} // namespace vnova::analyzer
