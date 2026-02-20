/*
 * Copyright (C) 2014-2026 V-Nova International Limited
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
#include "config_types.h"
#include "helper/base_type.h"
#include "helper/input_detection.h"
#include "utility/enum_map.h"
#include "utility/log_util.h"
#include "utility/platform.h"

#include <optional>

using namespace vnova::utility;
using namespace vnova::helper;

namespace vnova::analyzer {

namespace {
    // clang-format off
        const auto kInputTypeMap = EnumMap<InputType>
            (InputType::TS,  "TS")
            (InputType::ES, "ES")
            (InputType::WEBM, "WEBM")
            (InputType::MP4, "MP4")
            (InputType::BIN, "BIN")
            (InputType::LCEVC, "LCEVC");

        const auto kOutputTypeMap = EnumMap<ExtractOutputType>
            (ExtractOutputType::Bin, "BIN")
            (ExtractOutputType::Raw, "RAW")
            (ExtractOutputType::LengthDelimited, "DELIMITED");

        const auto kBaseTypeMap = EnumMap<helper::BaseType>
            ( helper::BaseType::H264, "H264" )
            ( helper::BaseType::HEVC, "HEVC" )
            ( helper::BaseType::VVC, "VVC" )
            ( helper::BaseType::VP8, "VP8" )
            ( helper::BaseType::VP9, "VP9" )
            ( helper::BaseType::AV1, "AV1" )
            ( helper::BaseType::VC6, "VC6" );

        const auto kLogFormatMap = EnumMap<LogFormat>
            ( LogFormat::TEXT, "LOG" )
            ( LogFormat::JSON, "JSON" );
    // clang-format on

    const std::map<std::string, std::pair<InputType, helper::BaseType>, std::less<>> kSupportedFileTypes = {
        /** TRANSPORT STREAM */
        {".ts", {InputType::TS, helper::BaseType::INVALID}},

        /** ELEMENTARY STREAM */
        {".es", {InputType::ES, helper::BaseType::INVALID}},
        {".264", {InputType::ES, helper::BaseType::H264}},
        {".h264", {InputType::ES, helper::BaseType::H264}},
        {".avc", {InputType::ES, helper::BaseType::H264}},
        {".265", {InputType::ES, helper::BaseType::HEVC}},
        {".h265", {InputType::ES, helper::BaseType::HEVC}},
        {".hevc", {InputType::ES, helper::BaseType::HEVC}},
        {".266", {InputType::ES, helper::BaseType::VVC}},
        {".h266", {InputType::ES, helper::BaseType::VVC}},
        {".vvc", {InputType::ES, helper::BaseType::VVC}},

        /** WEBM */
        {".webm", {InputType::WEBM, helper::BaseType::UNKNOWN}},

        /**  MP4  */
        {".mp4", {InputType::MP4, helper::BaseType::UNKNOWN}},

        /** BIN  */
        {".bin", {InputType::BIN, helper::BaseType::UNKNOWN}},

        /** RAW LCEVC ANNEX B  */
        {".lvc", {InputType::LCEVC, helper::BaseType::UNKNOWN}},
        {".lcevc", {InputType::LCEVC, helper::BaseType::UNKNOWN}}};

    bool inputTypeRequiresBaseType(InputType type)
    {
        switch (type) {
            case InputType::TS:
            case InputType::BIN:
            case InputType::LCEVC:
            case InputType::WEBM: return false;
            case InputType::ES:
            case InputType::MP4:
            case InputType::Unknown: return true;
        }
        return true;
    }

    bool baseTypeIsValid(helper::BaseType type)
    {
        return (type != helper::BaseType::UNKNOWN && type != helper::BaseType::INVALID);
    }

    void makeParentDirectory(const std::filesystem::path& path)
    {
        if (path.empty() == true) {
            return;
        }

        const std::filesystem::path dir = std::filesystem::absolute(path).parent_path();

        if (std::filesystem::is_directory(dir)) {
            // Already exists as dir, that's OK.
            return;
        }

        if (std::filesystem::exists(dir)) {
            // Already exists but is NOT dir.
            VNLOG_ERROR("Cannot overwrite existing entity at %s", dir.c_str());
            throw utility::file::FileError("Failed to create directory");
        }

        // Otherwise, make the dir and any parent dirs.
        std::filesystem::create_directories(path.parent_path());
    }

    constexpr bool isDigit(char val) { return val >= '0' && val <= '9'; }

    constexpr std::array<std::uint8_t, 3> parseVersion3(const char* str)
    {
        std::array<std::uint8_t, 3> out{};

        std::size_t idx = 0;

        for (int part = 0; part < 3; ++part) {
            // Must start with a digit
            if (!isDigit(str[idx])) {
                throw std::invalid_argument("version: expected digit");
            }

            unsigned value = 0;

            // Parse digits
            while (isDigit(str[idx])) {
                value = (value * 10u) + unsigned(str[idx] - '0');
                if (value > 255u) {
                    throw std::out_of_range("version: component > 255");
                }
                ++idx;
            }

            out[part] = static_cast<std::uint8_t>(value);

            if (part < 2) {
                // Expect dot after first two components
                if (str[idx] != '.') {
                    throw std::invalid_argument("version: expected '.'");
                }
                ++idx;
            } else {
                // Must end after 3rd component
                if (str[idx] != '\0') {
                    throw std::invalid_argument("version: trailing characters");
                }
            }
        }

        return out;
    }

    CLI::App* addSubcommand(CLI::App& parent, const std::string& name, const std::string& description)
    {
        std::string help{"\nlcevc_stream_analyzer::"};
        help += name;
        help += "\n -> ";
        help += description;
        auto* cmd = parent.add_subcommand(name, help);

        std::string usage{"lcevc_stream_analyzer --input <FILE> [global options] "};
        usage += name;
        usage += " [";
        usage += name;
        usage += " options]";
        cmd->usage(usage);

        cmd->fallthrough(true);
        cmd->ignore_case();

        return cmd;
    }

} // namespace

std::optional<const Config> parseArgs(int argc, char* argv[])
{
    Config config;
    config.version = parseVersion3(VERSION_STRING);

    CLI::App app{"\nlcevc_stream_analyzer\n"
                 "=====================\n"
                 "\n"
                 "A suite of tools to analyze and inspect LCEVC-enhanced video streams.",
                 "lcevc_stream_analyzer"};
    argv = app.ensure_utf8(argv);

    app.usage(" -> usage: \n"
              "         lcevc_stream_analyzer --input <FILE> [global options] \\\n"
              "                               SUBCOMMAND [subcommand options] \\\n"
              "                               [SUBCOMMAND] [subcommand options]    # (second "
              "subcommand optional)\n"
              "    e.g.\n"
              "         lcevc_stream_analyzer --input input.mp4 --base-type hevc ANALYZE --output "
              "output.json\n"
              "\n"
              " -> to view the options specific to each subcommand:\n"
              "         lcevc_stream_analyzer SUBCOMMAND --help\n"
              "    e.g.\n"
              "         lcevc_stream_analyzer ANALYZE --help");

    app.set_version_flag("--version", "Display program version information and exit");
    app.set_help_flag("-h,--help", "Show help and exit");

    app.require_subcommand(1, 0);

    // Global options (apply to all subcommands)
    app.add_option("-i,--input", config.inputPath, "Source video file")->required()->check(CLI::ExistingFile);

    app.add_option("-t,--type", config.inputType,
                   "Source video file type - default: detected from file extension")
        ->transform(CLI::CheckedTransformer(kInputTypeMap.NameToValueMap(InputType::Unknown),
                                            CLI::ignore_case));

    app.add_option("-p,--ts_pid", config.tsPID, "Transport stream PID with LCEVC data [-1: auto detect]")
        ->capture_default_str();
    app.add_option("--framerate", config.inputFramerate,
                   "Source video framerate - default: read from container");
    const auto* baseTypeOpt =
        app.add_option("-b,--base_type", config.baseType,
                       "Source video base codec - default: detected by parsing stream")
            ->transform(CLI::CheckedTransformer(kBaseTypeMap.NameToValueMap(helper::BaseType::UNKNOWN),
                                                CLI::ignore_case));
    app.add_option("--frames", config.frameLimit,
                   "Stop processing after N presentation order frames [0: process all frames]")
        ->capture_default_str();

    // analyze options
    auto* analyzeCmd = addSubcommand(app, "ANALYZE", "Output per-frame and summary stream info");
    analyzeCmd->add_option("-v,--verbose", config.analyzeVerboseOutput,
                           "Also output internal LCEVC parameters for each frame");
    analyzeCmd
        ->add_option("-o,--output", config.analyzeLogPath, "File to write parsed information to")
        ->check(!CLI::ExistingDirectory);
    const auto* formatOpt =
        analyzeCmd
            ->add_option("-f,--format", config.analyzeLogFormat,
                         "JSON or human readable output - default: match output file ext")
            ->default_str("LOG")
            ->transform(CLI::CheckedTransformer(kLogFormatMap.NameToValueMap(LogFormat::TEXT),
                                                CLI::ignore_case));

    // extract options
    auto* extractCmd = addSubcommand(app, "EXTRACT", "Extract LCEVC enhancement layer to file");
    extractCmd->add_option("-o,--output", config.extractOutputPath, "File to write LCEVC data to")
        ->required()
        ->check(!CLI::ExistingDirectory);
    auto* extractOutputTypeOpt =
        extractCmd
            ->add_option("-f,--format",
                         config.extractOutputType, "Type of file - BIN = LCEVC bin format, RAW = sequence of raw NALs, DELIMITED = sequence of  4-byte length delimited NALs")
            ->default_str("BIN")
            ->transform(CLI::CheckedTransformer(kOutputTypeMap.NameToValueMap(ExtractOutputType::Bin),
                                                CLI::ignore_case));

    // DUMP_AU options
    auto* dumpAuCmd =
        addSubcommand(app, "DUMP_AU", "Dump access unit packets to files, print NAL breakdown");
    dumpAuCmd
        ->add_option("-o,--output", config.dumpAuOutputPath, "Directory to write per-packet AU dump binary files")
        ->required()
        ->check(!CLI::ExistingFile);

    // SIMULATE_BUFFER options
    auto* simulateBufferCmd =
        addSubcommand(app, "SIMULATE_BUFFER", "Simulate receive network buffer over time");
    simulateBufferCmd
        ->add_option("--model", config.simulateBufferModelPath, "JSON file describing the receive buffer model")
        ->required()
        ->check(CLI::ExistingFile);
    simulateBufferCmd
        ->add_option("--output", config.simulateBufferOutputPath,
                     "File to store receive buffer simulation output")
        ->check(!CLI::ExistingDirectory);
    simulateBufferCmd
        ->add_option("--sample-interval", config.simulateBufferSampleInterval,
                     "Simulation sample interval in seconds")
        ->default_val(config.simulateBufferSampleInterval);

    try {
        app.parse(argc, argv);
    } catch (const CLI::CallForHelp& e) {
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        exit(app.exit(e));
    } catch (const CLI::CallForVersion&) {
        fprintf(stderr, "%s v%s\n", app.get_name().c_str(), VERSION_STRING);
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        exit(static_cast<int>(ReturnCode::Success));
    } catch (const CLI::ParseError& e) {
        if (argc <= 2) {
            fprintf(stderr, "%s\n", app.help().c_str());
            // NOLINTNEXTLINE(concurrency-mt-unsafe)
            exit(0);
        }
        VNLOG_ERROR("CLI parsing error: %s - %s", e.get_name().c_str(), e.what());
        return std::nullopt;
    }

    using FileTypePair = std::pair<InputType, helper::BaseType>;
    FileTypePair typesFromFileSuffix = {InputType::Unknown, helper::BaseType::INVALID};

    config.subcommand[Subcommand::ANALYZE] = analyzeCmd->parsed();
    config.subcommand[Subcommand::EXTRACT] = extractCmd->parsed();
    config.subcommand[Subcommand::SIMULATE_BUFFER] = simulateBufferCmd->parsed();
    config.subcommand[Subcommand::DUMP_AU] = dumpAuCmd->parsed();

    if (config.subcommand[Subcommand::DUMP_AU] == true) {
        try {
            std::filesystem::create_directories(config.dumpAuOutputPath);
        } catch (const std::exception& e) {
            VNLOG_ERROR("Failed to create DUMP_AU output directory: %s", e.what());
            return std::nullopt;
        }
    }

    const bool userSpecifiedBaseType = !baseTypeOpt->empty();
    const bool userSpecifiedOutputType = !extractOutputTypeOpt->empty();
    const bool userSpecifiedLogFormat = !formatOpt->empty();

    // Extract the format of the input file
    const std::string inputTypeName = config.inputPath.extension().string();

    // Try to deduce file types from input path extension
    if (config.inputType == InputType::Unknown) {
        if (const auto foundType = kSupportedFileTypes.find(inputTypeName);
            foundType != kSupportedFileTypes.end()) {
            typesFromFileSuffix = foundType->second;
            config.inputType = typesFromFileSuffix.first;
            VNLOG_DEBUG("Inferred file input type from filename");
        } else {
            VNLOG_ERROR("Could not infer file type for \"%s\"", inputTypeName.c_str());
            VNLOG_ERROR(
                "Only the following types can be inferred, set inputType manually otherwise: ");
            for (const auto& [file, detail] : kSupportedFileTypes) {
                VNLOG_ERROR("%s, ", file.c_str());
            }
            VNLOG_ERROR("");
        }
    }
    VNLOG_INFO("Using file input type to %s (%d)", toString(config.inputType), config.inputType);

    DetectedInputFormat detectedFormat;
    const bool detectedFromContent = detectInputFormatFromFile(config.inputPath, detectedFormat);
    if (detectedFromContent && detectedFormat.inputType != InputType::Unknown &&
        config.inputType != InputType::Unknown && config.inputType != detectedFormat.inputType) {
        VNLOG_WARN(
            "Argument defined input type as %s (%d) but byte analysis proposed %s (%d) - check "
            "file extension and -t/--input-type arg if used.",
            toString(config.inputType), config.inputType, toString(detectedFormat.inputType),
            detectedFormat.inputType);
    }

    if (config.inputType == InputType::Unknown && detectedFromContent) {
        config.inputType = detectedFormat.inputType;
    }

    if (config.inputType == InputType::Unknown) {
        VNLOG_ERROR("Unrecognized input type [%s]", inputTypeName.c_str());
        return std::nullopt;
    }

    if (!userSpecifiedBaseType) {
        if (detectedFromContent && detectedFormat.baseType != helper::BaseType::INVALID) {
            config.baseType = detectedFormat.baseType;
        } else if (typesFromFileSuffix.second != helper::BaseType::INVALID) {
            config.baseType = typesFromFileSuffix.second;
        }
    } else if (detectedFromContent && detectedFormat.baseType != helper::BaseType::INVALID &&
               (baseTypeIsValid(config.baseType) == false) && config.baseType != detectedFormat.baseType) {
        VNLOG_WARN("User-specified base type %d disagrees with detected %d", config.baseType,
                   detectedFormat.baseType);
    }

    if (inputTypeRequiresBaseType(config.inputType) && (baseTypeIsValid(config.baseType) == false)) {
        VNLOG_ERROR("Base type must be specified for the input type selected [%s]", inputTypeName.c_str());
        return std::nullopt;
    }

    if (baseTypeIsValid(config.baseType)) {
        VNLOG_INFO("Using base video type %s (%d)", toString(config.baseType), config.baseType);
    }

    if (extractCmd->parsed() && userSpecifiedOutputType && config.extractOutputPath.empty()) {
        VNLOG_ERROR("Output type specified but no output path provided. Please supply -o");
        return std::nullopt;
    }

    // Infer output type when path provided but type omitted (extract only)
    if (extractCmd->parsed() && !config.extractOutputPath.empty() && !userSpecifiedOutputType &&
        config.extractOutputType == ExtractOutputType::Unknown) {
        const std::string outputExt = config.extractOutputPath.extension().string();
        if (outputExt == ".bin") {
            config.extractOutputType = ExtractOutputType::Bin;
        } else if (outputExt == ".lvc" || outputExt == ".lcevc") {
            config.extractOutputType = ExtractOutputType::Raw;
        } else {
            VNLOG_ERROR("Failed to infer output type from extension .%s. Please specify -u",
                        outputExt.c_str());
            return std::nullopt;
        }
    }

    // Infer log format if user didn't specify one but provided a log path
    if (!config.analyzeLogPath.empty() && !userSpecifiedLogFormat) {
        const std::string logExt = config.analyzeLogPath.extension().string();
        if (logExt == ".json") {
            config.analyzeLogFormat = LogFormat::JSON;
        } else if (logExt == ".log" || logExt == ".txt") {
            config.analyzeLogFormat = LogFormat::TEXT;
        } else {
            VNLOG_ERROR("Failed to infer log format from log file extension %s. Please specify "
                        "-f [json|log]",
                        logExt.c_str());
            return std::nullopt;
        }
    }

    // Create output directories.
    makeParentDirectory(config.analyzeLogPath);
    makeParentDirectory(config.extractOutputPath);
    makeParentDirectory(config.simulateBufferOutputPath);

    return config;
}

} // namespace vnova::analyzer
