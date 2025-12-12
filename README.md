<!--
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
-->
# LCEVC Stream Analyzer

**LCEVC Stream Analyzer** is a cross-platform command-line tool for analyzing
LCEVC‑enhanced video streams.

## Features

- Parses and analyzes LCEVC-enhanced video streams
- Supports multiple input formats: TS, SEI, NAL, WebM, MP4, BIN
- Compatible with base codecs: H264, HEVC, AV1, VVC, VP9
- Writes LCEVC data in bin, raw, or length_delimited formats
- Supports JSON and verbose text logging for debugging and analysis
- Cross-platform support: Windows, Linux, and macOS

## CLI Usage

Run the following command to see available options:

```bash
lcevc_stream_analyzer --help
```

## Example Usage

Analyze an MP4 input with verbose logging:
`lcevc_stream_analyzer -i input.mp4 -t mp4 -v 1`

Extract LCEVC from a TS stream with manual PID selection:
`lcevc_stream_analyzer -i input.ts -t ts -p 256`

Analyze a raw H.264 SEI stream with specified base codec:
`lcevc_stream_analyzer -i stream_sei.h264 -t sei -b h264`

Output LCEVC data to a .bin file:
`lcevc_stream_analyzer -i input.webm -t webm -o output.bin -u bin`

Generate JSON logs of LCEVC information:
`lcevc_stream_analyzer -i input_raw_nal.h265 -t nal -b hevc -l logs/out.json -f json`

## Known Issues

### VVC streams can exceed ffmpeg `VVC_MAX_DPB_SIZE` buffer depth.

#### Diagnosis

The issue presents with terminal messages of the following form.

```
[vvc @ 0x6347db064940] Error allocating frame, DPB full.
[vvc @ 0x6347db064940] Error parsing NAL unit #1.
[vist#0:0/vvc @ 0x6347dae24a80] [dec:vvc @ 0x6347db0641c0] Error submitting packet to decoder: Cannot allocate memory
```

#### Cause

By default, ffmpeg is compiled with `VVC_MAX_DPB_SIZE` to 16. As a result, some streams fail to decode due to LCEVC VVC streams sometimes requiring a deeper buffer.

#### Resolution

If you are compiling ffmpeg from source, increasing `VVC_MAX_DPB_SIZE` to 32 may allow the stream to be decoded successfully.

## Building

### Windows - tested Windows 11 x86_64

#### Install MSVC Build Tools

1. Download
   **[Build Tools](https://visualstudio.microsoft.com/visual-cpp-build-tools/)**
   and run `vs_BuildTools.exe`.
2. In the installer, select **Desktop development with C++**.
3. Ensure these components are selected:
   - MSVC v14.x
   - Windows 10/11 SDK
   - CMake tools
   - Include ATL and MFC for latest v14.x tools (optional).

#### Alternative: MSYS2-based Build Environment

1. **Install MSYS2** : Download and install MSYS2 from:
   [MSYS2](https://www.msys2.org/)
2. **Install Required Packages** : Choose one of the following environments to
   install the required packages.

   ***UCRT64 Environment (Recommended)*** Open the **MSYS2 UCRT64** terminal and
   run:

   ```bash
   pacman -S git mingw-w64-ucrt-x86_64-clang \
                 mingw-w64-ucrt-x86_64-cmake 
   ```

   ***MINGW64 Environment*** Open the **MSYS2 MinGW64** terminal and run:

   ```bash
   pacman -S git mingw-w64-x86_64-clang \
                 mingw-w64-x86_64-cmake 
   ```


### Debian/Ubuntu based Linux - tested Ubuntu 22.04 x86_64

```bash
sudo apt update
sudo apt install git
sudo apt install cmake
sudo apt install build-essential
sudo apt install libgtest-dev # optional, for unit tests only
```

### macOS - tested macOS 15 arm64

```bash
# Install Homebrew (if not already installed) - https://https://brew.sh/
 
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

> If you're using Windows, make sure you call the following from a developer
> Powershell prompt.

```bash
git clone https://github.com/v-novaltd/lcevc-stream-analyzer.git -b main
cd lcevc-stream-analyzer

cmake -S . -B build -DCMAKE_INSTALL_PREFIX=build/install
cmake --build build --config Release
cmake --install build --config Release

# Linux only to find dynamic libraries at runtime
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

| Value | Definition    |
|-||
| 0     | LCEVC Payload |

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

Copyright © V-Nova Limited 2014-2025

Additional Information and Restrictions
* The LCEVC Stream Analyzer software is licensed under the BSD-3-Clause-Clear
  License.
* The LCEVC Stream Analyzer software is a stand-alone project and is NOT A
  CONTRIBUTION to any other project.
* If the software is incorporated into another project, THE TERMS OF THE
  BSD-3-CLAUSE-CLEAR LICENSE and the additional licensing information contained
  in this folder MUST BE MAINTAINED, AND THE SOFTWARE DOES NOT AND MUST NOT
  ADOPT THE LICENSE OF THE INCORPORATING PROJECT.
* ANY ONWARD DISTRIBUTION, WHETHER STAND-ALONE OR AS PART OF ANY OTHER PROJECT,
  REMAINS SUBJECT TO THE EXCLUSION OF PATENT LICENSES PROVISION OF THIS
  BSD-3-CLAUSE-CLEAR LICENSE. For enquiries about patent licenses, please
  contact legal@v-nova.com.
