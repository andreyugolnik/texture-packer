/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#pragma once

#include <cstdint>
#include <string>
#include <vector>

class cFileList final
{
public:
    void addFile(uint32_t trimCount, const std::string& path);
    void addPath(uint32_t trimCount, const std::string& root, bool recurse);

    void removeDupes();

    uint32_t getCount() const;

    struct FileInfo
    {
        uint32_t trimCount = 0u;
        std::string path;
    };

    using List = std::vector<FileInfo>;

    const List& getList() const;

private:
    List m_files;
};
