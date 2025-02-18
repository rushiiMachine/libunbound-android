#ifndef PROC_MAP_UTIL_HPP
#define PROC_MAP_UTIL_HPP

#include <cstdlib>
#include <optional>
#include <vector>

typedef uint8_t proc_map_flags_t;

enum proc_map_flags : uint8_t {
    PROC_MAP_READ = 1 << 0,
    PROC_MAP_WRITE = 1 << 1,
    PROC_MAP_EXEC = 1 << 2,
    PROC_MAP_SHARED = 1 << 3,
    PROC_MAP_PRIVATE = 1 << 4,
};

struct proc_map_t {
    void *address_start;
    void *address_end;
    proc_map_flags_t flags;
    size_t offset;
    uint16_t dev_minor;
    uint16_t dev_major;
    unsigned long int inode; /* ext4 use 2^32 inode */
    std::string file_name;
};

bool proc_map_parse(std::vector<proc_map_t> &maps);

void proc_map_fmt(proc_map_t &map, char *out);

#endif //PROC_MAP_UTIL_HPP
