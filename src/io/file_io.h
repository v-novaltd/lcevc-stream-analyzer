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
#ifndef VN_IO_FILE_IO_H_
#define VN_IO_FILE_IO_H_

#include "utility/platform.h"

#include <cstddef>
#include <cstdio>
#include <optional>
#include <string>

namespace vnova::utility::io {

class FileDeleter
{
public:
    void operator()(FILE* fp) const
    {
        // NOLINTNEXTLINE
        std::fclose(fp);
    }
};

class IO
{
public:
    IO() = default;
    virtual ~IO() = default;
    IO(const IO&) = delete;
    IO& operator=(const IO&) = delete;
    IO(IO&& rhs) = delete;
    IO& operator=(IO&& rhs) = delete;

    virtual bool isValid() { return false; }
    virtual uint64_t read(std::byte* /*buffer*/, uint64_t /*size*/) { return 0; }
    virtual uint64_t write(const std::byte* /*buffer*/, uint64_t /*size*/) { return 0; }
    virtual uint64_t size() { return 0; }
    virtual void seek(long long /*offset*/, uint32_t /*origin*/)
    {
        // Default no-op: override in derived classes that support seeking.
    }
    virtual uint64_t tell() { return 0; }
    virtual int flush() { return 0; }
};

class FileIO : public IO
{
public:
    FileIO(const char* filename, const char* mode, bool quiet = false);
    ~FileIO() override = default;
    FileIO(const FileIO&) = delete;
    FileIO& operator=(const FileIO&) = delete;
    FileIO(FileIO&& rhs) noexcept;
    FileIO& operator=(FileIO&& rhs) noexcept;

    bool isValid() override { return m_fileStream != nullptr; }
    bool isEndOfFile() { return m_fileStream != nullptr ? feof(m_fileStream.get()) != 0 : true; }
    bool isErrored() { return m_fileStream != nullptr ? ferror(m_fileStream.get()) != 0 : true; }
    int32_t getErrorNumber() const noexcept { return errorNumber; }

    uint64_t size() override;
    void seek(long long offset, uint32_t origin) override;
    uint64_t tell() override;

protected:
    std::unique_ptr<FILE, FileDeleter> m_fileStream = nullptr;
    int32_t errorNumber = 0;
};

class FileIOReadRaw : public FileIO
{
public:
    FileIOReadRaw(const char* filename, const char* mode, bool quiet)
        : FileIO(filename, mode, quiet)
    {}
    uint64_t read(std::byte* buffer, uint64_t size) override;
    static bool readToBuffer(utility::DataBuffer& buffer, const std::string& filename);
    void reset();
};
class FileIORead : public FileIOReadRaw
{
public:
    FileIORead(const std::string& filename, bool quiet = false)
        : FileIOReadRaw(filename.c_str(), "rb", quiet)
    {}
};

class FileIOWriteRaw : public FileIO
{
public:
    FileIOWriteRaw(const char* filename, const char* mode, bool quiet)
        : FileIO(filename, mode, quiet)
    {}
    uint64_t write(const std::byte* buffer, uint64_t size) override;
    uint64_t write(const std::string& data);
    int flush() override;
    static bool writeFromBuffer(const std::byte* buffer, uint64_t size, const std::string& filename);
};

class FileIOWrite : public FileIOWriteRaw
{
public:
    FileIOWrite(const std::string& filename, bool quiet = false, bool append = false)
        : FileIOWriteRaw(filename.c_str(), !append ? "wb+" : "ab+", quiet)
    {}
};

}; // namespace vnova::utility::io

#endif // VN_IO_FILE_IO_H_
