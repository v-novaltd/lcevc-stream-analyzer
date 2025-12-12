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
#include "io/file_io.h"

#include <cerrno>
#include <cstring>
#include <utility>

namespace vnova::utility::io {
FileIO::FileIO(const char* filename, const char* mode, bool quiet)
{
#if defined(_MSC_VER)
    FILE* rawFile = nullptr;
    const errno_t openResult = fopen_s(&rawFile, filename, mode);
    if (openResult != 0) {
        rawFile = nullptr;
        if (!quiet) {
            errorNumber = openResult;
        }
    }
    m_fileStream.reset(rawFile); // NOLINT(cppcoreguidelines-owning-memory)
#else
    // fopen returns an owning pointer; transferring directly into unique_ptr is intended.
    m_fileStream.reset(std::fopen(filename, mode)); // NOLINT(cppcoreguidelines-owning-memory)
#endif

    if ((m_fileStream == nullptr) && !quiet && errorNumber == 0) {
        errorNumber = errno;
    }
}

FileIO::FileIO(FileIO&& rhs) noexcept
    : m_fileStream(rhs.m_fileStream.get())
    , errorNumber(rhs.errorNumber)
{
    rhs.m_fileStream = nullptr;
    rhs.errorNumber = 0;
}

FileIO& FileIO::operator=(FileIO&& rhs) noexcept
{
    if (this != &rhs) {
        m_fileStream.reset();
        m_fileStream = std::move(rhs.m_fileStream);
        errorNumber = rhs.errorNumber;
        rhs.m_fileStream = nullptr;
        rhs.errorNumber = 0;
    }
    return *this;
}

uint64_t FileIO::size()
{
    if (m_fileStream != nullptr) {
        return file::getFileSize(m_fileStream.get());
    }
    return 0;
}

void FileIO::seek(long long offset, uint32_t origin)
{
    if (m_fileStream != nullptr) {
        file::seekToPosition(m_fileStream.get(), offset, static_cast<int32_t>(origin));
    }
}

uint64_t FileIO::tell()
{
    return (m_fileStream != nullptr) ? file::tellPosition(m_fileStream.get()) : 0;
}

uint64_t FileIOReadRaw::read(std::byte* buffer, uint64_t size)
{
    if (m_fileStream != nullptr) {
        // Whilst this may look weird and we could return the result of fread
        // I have seen anomalies on some platforms/devices where the result or
        // fread/fwrite doesn't match the final position of the file. This may have
        // been fixed by now BUT for now I am playing safe
        uint64_t pos = tell();
        if (auto res = fread(static_cast<void*>(buffer), sizeof(uint8_t), size, m_fileStream.get());
            res != size || ferror(m_fileStream.get())) {
            errorNumber = errno;
        }
        return tell() - pos;
    }
    return 0;
}

bool FileIOReadRaw::readToBuffer(utility::DataBuffer& buffer, const std::string& filename)
{
    FileIORead file = FileIORead(filename);
    if (!file.isValid()) {
        return false;
    }

    uint64_t size = file.size();

    if (buffer.size() != size) {
        buffer.resize(size);
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    if (file.read(reinterpret_cast<std::byte*>(buffer.data()), size) == size) {
        return true;
    }

    return false;
}

void FileIOReadRaw::reset() { rewind(m_fileStream.get()); }

uint64_t FileIOWriteRaw::write(const std::byte* buffer, uint64_t size)
{
    if (m_fileStream != nullptr) {
        // Whilst this may look weird and we could return the result of fread
        // I have seen anomalies on some platforms/devices where the result or
        // fread/fwrite doesn't match the final position of the file. This may have
        // been fixed by now BUT for now I am playing safe
        uint64_t pos = tell();
        if (auto res = fwrite(static_cast<const void*>(buffer), sizeof(char), size, m_fileStream.get());
            res != size) {
            errorNumber = errno;
        }
        return tell() - pos;
    }
    return 0;
}

uint64_t FileIOWriteRaw::write(const std::string& data)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return write(reinterpret_cast<const std::byte*>(data.data()), data.length());
}

int FileIOWriteRaw::flush()
{
    if (m_fileStream != nullptr) {
        return fflush(m_fileStream.get());
    }
    return 0;
}

bool FileIOWriteRaw::writeFromBuffer(const std::byte* buffer, uint64_t size, const std::string& filename)
{
    FileIOWrite file = FileIOWrite(filename);

    if (!file.isValid()) {
        return false;
    }

    return (file.write(buffer, size) == size);
}

} // namespace vnova::utility::io
