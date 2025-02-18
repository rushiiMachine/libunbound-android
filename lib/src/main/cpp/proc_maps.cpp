#include <cstdint>
#include "proc_maps.hpp"
#include "logging.hpp"

bool proc_map_parse_line(char *line, proc_map_t &map) {
    char perms[4];
    int filename_start = 0, filename_end = 0;

    sscanf(line, "%lx-%lx %4c %lx %hx:%hx %lu %n%*[^\n]%n",
           (long unsigned *) &map.address_start,
           (long unsigned *) &map.address_end,
           perms,
           (long unsigned *) &map.offset,
           &map.dev_major, &map.dev_minor,
           &map.inode,
           &filename_start, &filename_end);

    map.flags = (perms[0] == 'r' ? PROC_MAP_READ : 0)
                | (perms[1] == 'w' ? PROC_MAP_WRITE : 0)
                | (perms[2] == 'x' ? PROC_MAP_EXEC : 0)
                | (perms[3] == 's' ? PROC_MAP_SHARED : 0)
                | (perms[3] == 'p' ? PROC_MAP_PRIVATE : 0);


    if (filename_end && filename_start != filename_end) {
        map.file_name = std::string_view{line + filename_start, (size_t) (filename_end - filename_start)};
    }

    return true;
}

bool proc_map_parse(std::vector<proc_map_t> &maps) {
    FILE *fp = fopen("/proc/self/maps", "r");
    if (!fp) return false;

    char *line = nullptr;
    size_t line_len = 0;

    while (getline(&line, &line_len, fp) > 0) {
        LOGD("parsing maps line: {}", std::string_view{line, line_len});

        if (!proc_map_parse_line(line, maps.emplace_back())) {
            free(line);
            fclose(fp);
            return false;
        }
    }

    free(line);
    fclose(fp);
    return true;
}
