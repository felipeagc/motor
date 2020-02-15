#include <motor/base/filesystem.h>

const char *mt_path_ext(const char *path)
{
    const char *ret = "";
    do
    {
        if (*path == '.')
        {
            ret = path;
        }
    } while (*path++);
    return ret;
}
