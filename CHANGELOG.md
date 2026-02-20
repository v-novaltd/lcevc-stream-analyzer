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

# Changelog

## 2.0.1

### Fixes

- Correct release lint error.

## 2.0.0

### Features

- Subcommand-based CLI to separate core ANALYZE/EXTRACT features from other new features
- Improved JSON output hierarchy which closely matches LCEVC spec
- Clearer human-readable terminal output formatting
- Warn user if inconsistent PTS value increment is detected
- Warn user if the tool failed to parse all frames
- Add network buffer simulation feature which can report plottable buffer fill over time data

### Fixes

- Fix frame byte accounting and ordering logic
- Correct reported base frame sizes and LCEVC frame association
- Correct summary bitrate calculations
- Fix VVC NAL header decoding in MP4 files

### Dependencies / Compatibility

- Bump FFmpeg to 8.0 to enable VVC frame type parsing

### Docs

- Add "Quick start" section
- Add subcommands explainer and update docs

## 1.0.0

Initial release.
