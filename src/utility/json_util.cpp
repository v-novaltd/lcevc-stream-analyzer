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

#include "json_util.h"

#include "log_util.h"

#include <json.hpp>

#include <fstream>

namespace vnova::utility::json {

std::optional<nlohmann::json> read(const std::filesystem::path& path)
{
    try {
        std::ifstream file(path);
        nlohmann::json data;
        file >> data;
        return data;
    } catch (const nlohmann::json::parse_error& e) {
        VNLOG_ERROR("JSON error: %s", e.what());
    } catch (const std::filesystem::filesystem_error& e) {
        VNLOG_ERROR("Filesystem error: %s", e.what());
    }
    return std::nullopt;
}

bool write(const std::filesystem::path& path, const nlohmann::ordered_json& data)
{
    try {
        std::ofstream file(path);
        file << data.dump(4);
        return true;
    } catch (const nlohmann::json::exception& e) {
        VNLOG_ERROR("JSON error: %n", e.what());
    } catch (const std::filesystem::filesystem_error& e) {
        VNLOG_ERROR("Filesystem error: %s", e.what());
    }

    return false;
}

bool dump(FILE* file, const nlohmann::ordered_json& data)
{
    if (file == nullptr) {
        return false;
    }

    try {
        fprintf(file, "%s\n", data.dump(4).c_str());
        return true;
    } catch (const nlohmann::json::exception& e) {
        VNLOG_ERROR("Failed to dump json to stdout: %s", e.what());
    }

    return false;
}

} // namespace vnova::utility::json
