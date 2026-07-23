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
#include <dirent.h>
#include <sys/stat.h>

namespace
{
    int DirectoryFilter(const dirent* p)
    {
        // skip . and ..
#define DOT_OR_DOTDOT(base) (base[0] == '.' && (base[1] == '\0' || (base[1] == '.' && base[2] == '\0')))
        return DOT_OR_DOTDOT(p->d_name)
            ? 0
            : 1;
    }

} // namespace

void cFileList::addFile(uint32_t trimCount, const std::string& path)
{
    m_files.push_back({ trimCount, path });
}

void cFileList::addPath(uint32_t trimCount, const std::string& root, bool recurse)
{
    auto dir = ::opendir(root.c_str());
    if (dir != nullptr)
    {
        dirent** namelist;
        int n = ::scandir(root.c_str(), &namelist, DirectoryFilter, alphasort);
        if (n >= 0)
        {
            while (n--)
            {
                std::string path(root);
                if (path[path.length() - 1] != '/')
                {
                    path += "/";
                }
                path += namelist[n]->d_name;

                if (recurse)
                {
                    addPath(trimCount, path, recurse);
                }
                else
                {
                    addFile(trimCount, path);
                }

                ::free(namelist[n]);
            }
            ::free(namelist);
        }

        ::closedir(dir);
    }
    else
    {
        struct stat st;
        if (::stat(root.c_str(), &st) != 0)
        {
            cLog::Warning("Input path '{}' does not exist.", root);
        }
        else if (S_ISDIR(st.st_mode))
        {
            cLog::Warning("Cannot open directory '{}'.", root);
        }
        else
        {
            addFile(trimCount, root);
        }
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
