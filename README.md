<!--
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
-->

# LCEVC Stream Analyzer

**LCEVC Stream Analyzer** is a cross-platform command-line tool for analyzing
LCEVC‑enhanced video streams.

## Features

- Parses and analyzes LCEVC-enhanced video streams
- Supports multiple input formats: TS, SEI, NAL, WebM, MP4, BIN
- Compatible with base codecs: H264, HEVC, VVC, and partial support for AV1, VP9
- Can export LCEVC data from muxed stream file in bin, raw, or length-delimited formats
- Supports JSON and verbose text logging for debugging and analysis
- Standalone, cross-platform tool: works on Linux, macOS and Windows

## Quick Start

### Linux

1. Download the latest [`lcevc_stream_analyzer` release](https://github.com/v-novaltd/lcevc-stream-analyzer/releases/latest) archive for Linux.

1. Unarchive, and install required ffmpeg libraries:

    ```bash
    # Create a working directory
    mkdir "lcevc_stream_analyzer"
    cd "lcevc_stream_analyzer"

    # Extract the lcevc_stream_analyzer archive
    tar -xvf ../lcevc_stream_analyzer*-linux-x86_64.tar.gz

    # Download and extract pre-built ffmpeg archive
    FFMPEG_ARCHIVE="ffmpeg-n8.0-latest-linux64-gpl-shared-8.0"
    wget "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/$FFMPEG_ARCHIVE.tar.xz"
    tar -xvf "$FFMPEG_ARCHIVE.tar.xz" && rm "$FFMPEG_ARCHIVE.tar.xz"

    # Copy the libav libraries to the binary directory
    cp -r "$FFMPEG_ARCHIVE/lib/"* "build/install/bin/"

    # Clean up
    rm -rf "$FFMPEG_ARCHIVE/"

    # Run the program
    cd "build/install/bin/"
    ./lcevc_stream_analyzer --help
    ```

### Windows

1. Download the latest [`lcevc_stream_analyzer` release](https://github.com/v-novaltd/lcevc-stream-analyzer/releases/latest) archive for Windows.

1. Unzip the `.zip` file to a directory named `lcevc_stream_analyzer` or similar.

1. Download pre-built ffmpeg v8.0 archive: [`ffmpeg-n8.0-latest-win64-gpl-8.0.zip`](https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-n8.0-latest-win64-gpl-8.0.zip).

1. Open the `ffmpeg-n8.0-latest-win64-gpl-8.0.zip` file in Windows Explorer.

1. Copy all files (including `.dll`s) from `ffmpeg-n8.0-latest-win64-gpl-8.0\bin` to `lcevc_stream_analyzer\bin`.

1. In `lcevc_stream_analyzer\bin`, the following files should be listed (the numbers in the `av` `dll`s do matter; ensure the correct version of FFmpeg was downloaded):

    ```txt
    avfilter-10.dll
    avformat-61.dll
    avutil-59.dll
    ffmpeg.exe
    ffplay.exe
    ffprobe.exe
    swresample-5.dll
    swscale-8.dll
    avcodec-61.dll
    avdevice-61.dll
    lcevc_stream_analyzer.exe
    ```

1. Run the program from the `lcevc_stream_analyzer\bin` directory

    ```cmd
    lcevc_stream_analyzer --help
    ```

### macOS

1. Download the latest [`lcevc_stream_analyzer` release](https://github.com/v-novaltd/lcevc-stream-analyzer/releases/latest) archive for macOS.

1. Unarchive, and install required ffmpeg libraries:

    ```bash
    # Install dependency
    brew install ffmpeg

    # Create a working directory
    mkdir "lcevc_stream_analyzer"
    cd "lcevc_stream_analyzer"

    # Extract the lcevc_stream_analyzer archive
    tar -xvf ../lcevc_stream_analyzer*-macos-aarch64.tar.gz

    # Run the tool
    ./build/install/bin/lcevc_stream_analyzer --help
    ```

## CLI Interface

The CLI uses a subcommand model, quite similar to how tools such as `git` work. The user must provide a subcommand to use the various features of the program.

Arguments either apply to the whole tool (global args) or to a subcommand (subcommand args).
Providing no arguments or using `--help` flag prints out a list of global arguments and the available subcommands:

```text
lcevc_stream_analyzer --help

...

OPTIONS
          --version           Display program version information and exit
  -h,     --help
  -i,     --input TEXT:FILE REQUIRED
                              Source video file
...

SUBCOMMANDS:
  analyze
                              lcevc_stream_analyzer::analyze
                              -> Output per-frame and summary stream info
  extract
                              lcevc_stream_analyzer::extract
                              -> Extract LCEVC enhancement layer to file

...
```

The only argument which is required in all cases is `--input <FILE>`, where `<FILE>` is the path to the stream you wish to analyze.

You can ask for help about any subcommand using `SUBCOMMAND --help`, e.g.:

```text
lcevc_stream_analyzer ANALYZE --help

...

OPTIONS:
  -h,     --help
  -v,     --verbose BOOLEAN   Also output internal LCEVC parameters for each frame
```

Here, you can acquire the list of subcommand arguments that are available.

The structure of a command is thus:

```txt
lcevc_stream_analyzer --input <FILE> [global options] SUBCOMMAND [subcommand options]
```

In this application, subcommand matching is case-insensitive, i.e. `analyze` works just the same as `ANALYZE`.

### Multiple Subcommands

Unlike the interface for `git`, it is possible to provide more than one subcommand to the tool. In this case, every time you provide a subcommand, the argument parsing logic switches context to that subcommand, and all arguments until the next subcommand or the end of the argument list are subcommand arguments for that command. The syntax is as follows:

```txt
lcevc_stream_analyzer --input <FILE> [global options] SUBCOMMAND [subcommand options] SUBCOMMAND [subcommand options]
```

Or as a concrete example:

```txt
lcevc_stream_analyzer --input a.mp4 --base_type h264 \   # global args
    ANALYZE --output stream.json \                       # ANALYZE args
    EXTRACT --output lcevc.bin                           # EXTRACT args
```

In the above example, the first `--output` is directed to the ANALYZE command, so we get a JSON output at the path provided. The second `--output` is directed to the EXTRACT command, so even though it is the same name, it has a different use in the program.

## Example Usage

Analyze an MP4 input with verbose logging (input and base type auto-detected):

```
lcevc_stream_analyzer --input input.mp4     ANALYZE --verbose 1
```

Extract LCEVC from a ES stream (must provide base type):

```
lcevc_stream_analyzer --input input.es --base-type vvc     analyze
```

Extract LCEVC from a TS stream with manual PID selection:

```
lcevc_stream_analyzer --input input.ts --ts-pid 256     analyze
```

Analyze a raw H.264 SEI stream with specified base codec:

```
lcevc_stream_analyzer --input stream_sei.ext --input-type sei --base-type h264     analyze
```

Generate JSON logs of LCEVC information:

```
lcevc_stream_analyzer --input stream.mp4     ANALYZE --output logs/out.json
```

Output LCEVC data to a .bin file:

```
lcevc_stream_analyzer --input input.webm     EXTRACT --output output.bin
```

Generate receive network buffer analysis output JSON (to pass in to plot tool):

```
lcevc_stream_analyzer --input input.mp4      SIMULATE_BUFFER --model buffer_model.json --output buffer_out.json
```

Extract and analyze a stream (note two subcommands!):

```
lcevc_stream_analyzer --input input.mp4     ANALYZE --output stream.json     EXTRACT --output lcevc.bin
```

## Changelog

See CHANGELOG.md

## Known Issues

### VVC streams with non-SEI encapsulated interleaved base/LCEVC NALs cannot be parsed

#### Diagnosis

The issue presents with terminal messages of the following form.

```
[vvc @ 0x5885ad1e5120] Decoding of multilayer bitstreams is not implemented. Update your FFmpeg version to the newest one from Git. If the problem still occurs, it means that your file has a feature which has not been implemented.
[vvc @ 0x5885ad1e5120] Error parsing NAL unit #4.
```

#### Cause

As of version 8.0, which is used in this project, FFmpeg does not support parsing streams with interleaved base/LCEVC NAL streams that do not use SEI encapsulation.

#### Resolution

There is no known resolution for streams of this nature. Interleaved streams encoded with SEI encapsulation are parseable.

## Building

### Windows - tested on Windows 11 x86_64

#### Install MSVC Build Tools

1. Download
   [Build Tools](https://visualstudio.microsoft.com/visual-cpp-build-tools/)
   and run `vs_BuildTools.exe`.
2. In the installer, select `Desktop development with C++`.
3. Ensure these components are selected:
    - MSVC v14.x
    - Windows 10/11 SDK
    - CMake tools
    - Include ATL and MFC for latest v14.x tools (optional).

#### Alternative: MSYS2-based Build Environment

1. Install [MSYS2](https://www.msys2.org/)

2. Install Required Packages - choose either of the following environments:

    a. _UCRT64 Environment (Recommended)_: Open the MSYS2 UCRT64 terminal and
    run:

    ```bash
    pacman -S git mingw-w64-ucrt-x86_64-clang mingw-w64-ucrt-x86_64-cmake
    ```

    b. _MINGW64 Environment_: Open the MSYS2 MinGW64 terminal and run:

    ```bash
    pacman -S git mingw-w64-x86_64-clang mingw-w64-x86_64-cmake
    ```

### Debian/Ubuntu based Linux - tested on Ubuntu 22.04 x86_64

```bash
sudo apt update
sudo apt install git
sudo apt install cmake
sudo apt install build-essential
sudo apt install libgtest-dev # optional, for unit tests only
```

### macOS - tested on macOS 15 arm64

```bash
# Install Homebrew (if not already installed) - https://brew.sh/

brew install cmake
brew install git
brew install pkgconf
brew install ffmpeg
brew install aom
brew install googletest # optional, for unit tests only
```

### Build Options

- `VN_BUILD_BINARIES`: build the `StreamAnalyzer` executable (default: ON)
- `VN_FFMPEG_DOWNLOAD`: download prebuilt FFmpeg/libav as a fallback (default:
  ON)
- `VN_BUILD_UNIT_TESTS`: build unit test executables (default: OFF)

### Build Instructions

> On Windows, make sure the following is called from a developer Powershell prompt.

```bash
git clone https://github.com/v-novaltd/lcevc-stream-analyzer.git -b main
cd lcevc-stream-analyzer

cmake -S . -B build -DCMAKE_INSTALL_PREFIX=build/install
cmake --build build --config Release
cmake --install build --config Release

# Linux only - find dynamic libraries at runtime
export LD_LIBRARY_PATH=$(pwd)/build/install/lib:$LD_LIBRARY_PATH
```

### Running Unit Tests

```bash
cmake -S . -B build -DVN_BUILD_UNIT_TESTS=ON -DVN_BUILD_BINARIES=OFF
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

## Elementary Formats

### SEI

- LCEVC data is embedded as SEI messages, which are metadata packets within the
  base video stream.
- The enhancement data is encapsulated within T35 registered user data SEI
  messages.

### NAL

- LCEVC data is interleaved with the base codec’s NAL units.
- These NAL units contain the enhancement data, identified by a specific LCEVC
  NAL unit header format.

## BIN Format

The `.bin` format is a **block-based** binary structure that stores raw LCEVC  
data. It starts with a fixed header and then contains a sequence of data blocks,
each aligned to a frame.  
All values are written in **big-endian** byte order.

### Format Overview

```
<BEGIN_FILE>

u8[8]   : magic_marker         // "lcevcbin" = 0x6C 0x63 0x65 0x76 0x63 0x62 0x69 0x6E
u32     : version              // Format version number

while more_data():
    u16 : block_type           // Type of block
    u32 : block_size           // Size in bytes of block_specific_data
    bytes: block_specific_data // Block content

<END_FILE>
```

### Field Semantics

#### magic_marker

- Fixed 8-byte header: `0x6C 0x63 0x65 0x76 0x63 0x62 0x69 0x6E`
- ASCII string `"lcevcbin"` used to identify the file format as an LCEVC binary
  stream.

#### version

- Indicates the syntax version of the file. Must be incremented if the syntax
  changes.

#### more_data()

- Indicates that there is more data available in the file. The file format is
  block-based and blocks are expected to be consecutive until the end of the
  file.

#### block_type

Defines what type of block is present:

| Value | Definition |
|-||
| 0 | LCEVC Payload |

#### block_size

- The size in bytes of the `block_specific_data` within the block.

#### block_specific_data

- Contains the actual byte content of a block.

### LCEVC Payload Block (block_type = 0)

#### Syntax

```
i64   : decode_index
i64   : presentation_index
bytes : lcevc_data
```

#### Semantics

- **decode_index**: Decode timestamp of the LCEVC data.
- **presentation_index**: Presentation timestamp of the LCEVC data.
- **lcevc_data**: The raw LCEVC byte data.

## LCEVC Output Formats

### Raw

- The Raw format contains only the raw LCEVC enhancement data.

#### Format Overview

```
<BEGIN_FILE>

while more_data():
     bytes : lcevc_data

<END_FILE>
```

#### Field Semantics

- **more_data()**: Indicates that more data blocks remain to be written. Data
  blocks are written sequentially until all enhancement data has been output.
- **lcevc_data**: Raw LCEVC enhancement data.

### Length Delimited

- The LengthDelimited format stores raw LCEVC enhancement data in a sequence of
  length-prefixed blocks. Each block is preceded by a 4-byte unsigned integer
  indicating the size of the following LCEVC data block.

#### Format Overview

```
<BEGIN_FILE>

while more_data():
     u32   : block_size           // Size in bytes of the following LCEVC data block
     bytes : lcevc_data

<END_FILE>
```

#### Field Semantics

- **more_data()**: Indicates that more data blocks remain to be written. Data
  blocks are written sequentially until all enhancement data has been output.
- **block_size**: Size in bytes of the `lcevc_data` block.
- **lcevc_data**: Raw LCEVC enhancement data.

## Third-party Dependencies

### FFmpeg

- **Source**: Prebuilt binaries are downloaded from the official FFmpeg website:  
  [https://ffmpeg.org/download.html](https://ffmpeg.org/download.html)
- **License**: FFmpeg is licensed under the **GNU Lesser General Public License
  (LGPL) version 2.1 or later**.  
  Some optional components and optimizations are covered under the **GNU General
  Public License (GPL) version 2 or later**.  
  Refer to the [FFmpeg legal page](https://ffmpeg.org/legal.html) for complete
  licensing details.

### nlohmann json

- **Source**: The `third_party/json.hpp` header file is included in the
  repository, sourced from [nlohmann/json](https://github.com/nlohmann/json).
- **License**: See the
  [license file](https://github.com/nlohmann/json/blob/develop/LICENSE.MIT) for
  details.

### CLI11

- **Source**: The `third_party/cli11.hpp` header file is included in the
  repository, sourced from [CLIUtils/CLI11](https://github.com/CLIUtils/CLI11).
- **License**: See the
  [license file](https://github.com/CLIUtils/CLI11/blob/main/LICENSE) for
  details.

### googletest

- **Source**: The `gtest` library is used for unit testing , sourced from  
  [google/googletest](https://github.com/google/googletest).
- **License**: See the
  [license file](https://github.com/google/googletest/blob/main/LICENSE) for
  details.

## Notice

Copyright © V-Nova Limited 2014-2026

Additional Information and Restrictions

- The LCEVC Stream Analyzer software is licensed under the BSD-3-Clause-Clear
  License.
- The LCEVC Stream Analyzer software is a stand-alone project and is NOT A
  CONTRIBUTION to any other project.
- If the software is incorporated into another project, THE TERMS OF THE
  BSD-3-CLAUSE-CLEAR LICENSE and the additional licensing information contained
  in this folder MUST BE MAINTAINED, AND THE SOFTWARE DOES NOT AND MUST NOT
  ADOPT THE LICENSE OF THE INCORPORATING PROJECT.
- ANY ONWARD DISTRIBUTION, WHETHER STAND-ALONE OR AS PART OF ANY OTHER PROJECT,
  REMAINS SUBJECT TO THE EXCLUSION OF PATENT LICENSES PROVISION OF THIS
  BSD-3-CLAUSE-CLEAR LICENSE. For enquiries about patent licenses, please
  contact <legal@v-nova.com>.
