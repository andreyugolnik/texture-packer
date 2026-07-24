/**********************************************\
*
*  Andrey A. Ugolnik
*  http://www.ugolnik.info
*  andrey@ugolnik.info
*
\**********************************************/

#include "FileList.h"
#include "Log.h"

#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

namespace
{
    void collect(const std::string& root, bool recurse, std::vector<std::string>& out)
    {
        const auto options = fs::directory_options::skip_permission_denied;

        std::error_code ec;
        if (recurse)
        {
            auto it = fs::recursive_directory_iterator(root, options, ec);
            for (const auto end = fs::recursive_directory_iterator(); it != end; it.increment(ec))
            {
                if (ec)
                {
                    break;
                }
                if (it->is_regular_file(ec))
                {
                    out.push_back(it->path().string());
                }
            }
        }
        else
        {
            auto it = fs::directory_iterator(root, options, ec);
            for (const auto end = fs::directory_iterator(); it != end; it.increment(ec))
            {
                if (ec)
                {
                    break;
                }
                if (it->is_regular_file(ec))
                {
                    out.push_back(it->path().string());
                }
            }
        }
    }

} // namespace

void cFileList::addFile(uint32_t trimCount, const std::string& path)
{
    m_files.push_back({ trimCount, path });
}

void cFileList::addPath(uint32_t trimCount, const std::string& root, bool recurse)
{
    std::error_code ec;
    const auto status = fs::status(root, ec);
    if (ec || fs::exists(status) == false)
    {
        cLog::Warning("Input path '{}' does not exist.", root);
        return;
    }

    if (fs::is_directory(status) == false)
    {
        addFile(trimCount, root);
        return;
    }

    std::vector<std::string> entries;
    collect(root, recurse, entries);

    // Sort for deterministic output regardless of filesystem iteration order.
    std::sort(entries.begin(), entries.end());
    for (const auto& path : entries)
    {
        addFile(trimCount, path);
    }
}

void cFileList::removeDupes()
{
    std::sort(m_files.begin(), m_files.end(), [](const FileInfo& a, const FileInfo& b) {
        return a.path < b.path;
    });
    auto it = std::unique(m_files.begin(), m_files.end(), [](const FileInfo& a, const FileInfo& b) {
        return a.path == b.path;
    });
    m_files.resize(std::distance(m_files.begin(), it));
}

uint32_t cFileList::getCount() const
{
    return static_cast<uint32_t>(m_files.size());
}

const cFileList::List& cFileList::getList() const
{
    return m_files;
}
