/*
 * This file is part of LSPosed.
 *
 * LSPosed is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LSPosed is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LSPosed.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2019 Swift Gan
 * Copyright (C) 2021 LSPosed Contributors
 */
#include <malloc.h>
#include <cstring>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#include <sys/stat.h>
#include <miniz.h>
#include "logging.hpp"
#include "proc_maps.hpp"
#include "elf_util.hpp"
#include "zip_util.hpp"

using namespace SandHook;

template<typename T>
inline constexpr auto offsetOf(ElfW(Ehdr) *head, ElfW(Off) off) {
    return reinterpret_cast<std::conditional_t<std::is_pointer_v<T>, T, T *>>(
            reinterpret_cast<uintptr_t>(head) + off);
}

ElfImg::ElfImg(std::string_view base_name) : elfPath(base_name) {
    if (!findModuleBase()) {
        base = nullptr;
        return;
    }

    //load elf
    int fd = open(elfPath.data(), O_RDONLY);
    if (fd < 0) {
        LOGE("failed to open {}", elfPath);
        return;
    }

    // only get size if this elf isn't mapped from apk
    if (!size && !elfFileOffset) {
        size = lseek(fd, 0, SEEK_END);
        if (size <= 0) {
            LOGE("lseek() failed for {}", elfPath);
        }
    }

    header = reinterpret_cast<decltype(header)>(mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, elfFileOffset));

    close(fd);

    section_header = offsetOf<decltype(section_header)>(header, header->e_shoff);

    auto shoff = reinterpret_cast<uintptr_t>(section_header);
    char *section_str = offsetOf<char *>(header, section_header[header->e_shstrndx].sh_offset);

    for (int i = 0; i < header->e_shnum; i++, shoff += header->e_shentsize) {
        auto *section_h = (ElfW(Shdr) *) shoff;
        char *sname = section_h->sh_name + section_str;
        auto entsize = section_h->sh_entsize;
        switch (section_h->sh_type) {
            case SHT_DYNSYM: {
                if (bias == -4396) {
                    dynsym = section_h;
                    dynsym_offset = section_h->sh_offset;
                    dynsym_start = offsetOf<decltype(dynsym_start)>(header, dynsym_offset);
                }
                break;
            }
            case SHT_SYMTAB: {
                if (strcmp(sname, ".symtab") == 0) {
                    symtab = section_h;
                    symtab_offset = section_h->sh_offset;
                    symtab_size = section_h->sh_size;
                    symtab_count = symtab_size / entsize;
                    symtab_start = offsetOf<decltype(symtab_start)>(header, symtab_offset);
                }
                break;
            }
            case SHT_STRTAB: {
                if (bias == -4396) {
                    strtab = section_h;
                    symstr_offset = section_h->sh_offset;
                    strtab_start = offsetOf<decltype(strtab_start)>(header, symstr_offset);
                }
                if (strcmp(sname, ".strtab") == 0) {
                    symstr_offset_for_symtab = section_h->sh_offset;
                }
                break;
            }
            case SHT_PROGBITS: {
                if (strtab == nullptr || dynsym == nullptr) break;
                if (bias == -4396) {
                    bias = (off_t) section_h->sh_addr - (off_t) section_h->sh_offset;
                }
                break;
            }
            case SHT_HASH: {
                auto *d_un = offsetOf<ElfW(Word)>(header, section_h->sh_offset);
                nbucket_ = d_un[0];
                bucket_ = d_un + 2;
                chain_ = bucket_ + nbucket_;
                break;
            }
            case SHT_GNU_HASH: {
                auto *d_buf = reinterpret_cast<ElfW(Word) *>(((size_t) header) +
                                                             section_h->sh_offset);
                gnu_nbucket_ = d_buf[0];
                gnu_symndx_ = d_buf[1];
                gnu_bloom_size_ = d_buf[2];
                gnu_shift2_ = d_buf[3];
                gnu_bloom_filter_ = reinterpret_cast<decltype(gnu_bloom_filter_)>(d_buf + 4);
                gnu_bucket_ = reinterpret_cast<decltype(gnu_bucket_)>(gnu_bloom_filter_ +
                                                                      gnu_bloom_size_);
                gnu_chain_ = gnu_bucket_ + gnu_nbucket_ - gnu_symndx_;
                break;
            }
        }
    }
}

ElfW(Addr) ElfImg::ElfLookup(std::string_view name, uint32_t hash) const {
    if (nbucket_ == 0) return 0;

    char *strings = (char *) strtab_start;

    for (auto n = bucket_[hash % nbucket_]; n != 0; n = chain_[n]) {
        auto *sym = dynsym_start + n;
        if (name == strings + sym->st_name) {
            return sym->st_value;
        }
    }
    return 0;
}

ElfW(Addr) ElfImg::GnuLookup(std::string_view name, uint32_t hash) const {
    static constexpr auto bloom_mask_bits = sizeof(ElfW(Addr)) * 8;

    if (gnu_nbucket_ == 0 || gnu_bloom_size_ == 0) return 0;

    auto bloom_word = gnu_bloom_filter_[(hash / bloom_mask_bits) % gnu_bloom_size_];
    uintptr_t mask = 0
                     | (uintptr_t) 1 << (hash % bloom_mask_bits)
                     | (uintptr_t) 1 << ((hash >> gnu_shift2_) % bloom_mask_bits);
    if ((mask & bloom_word) == mask) {
        auto sym_index = gnu_bucket_[hash % gnu_nbucket_];
        if (sym_index >= gnu_symndx_) {
            char *strings = (char *) strtab_start;
            do {
                auto *sym = dynsym_start + sym_index;
                if (((gnu_chain_[sym_index] ^ hash) >> 1) == 0
                    && name == strings + sym->st_name) {
                    return sym->st_value;
                }
            } while ((gnu_chain_[sym_index++] & 1) == 0);
        }
    }
    return 0;
}

void ElfImg::MayInitLinearMap() const {
    if (symtabs_.empty()) {
        if (symtab_start != nullptr && symstr_offset_for_symtab != 0) {
            for (ElfW(Off) i = 0; i < symtab_count; i++) {
                unsigned int st_type = ELF_ST_TYPE(symtab_start[i].st_info);
                const char *st_name = offsetOf<const char *>(header, symstr_offset_for_symtab +
                                                                     symtab_start[i].st_name);
                if ((st_type == STT_FUNC || st_type == STT_OBJECT) && symtab_start[i].st_size) {
                    symtabs_.emplace(st_name, &symtab_start[i]);
                }
            }
        }
    }
}

ElfW(Addr) ElfImg::LinearLookup(std::string_view name) const {
    MayInitLinearMap();
    if (auto i = symtabs_.find(name); i != symtabs_.end()) {
        return i->second->st_value;
    } else {
        return 0;
    }
}

std::vector<ElfW(Addr)> ElfImg::LinearRangeLookup(std::string_view name) const {
    MayInitLinearMap();
    std::vector<ElfW(Addr)> res;
    for (auto [i, end] = symtabs_.equal_range(name); i != end; ++i) {
        auto offset = i->second->st_value;
        res.emplace_back(offset);
        LOGD("found {} {:#x} in {} in symtab by linear range lookup", name, offset, elfPath);
    }
    return res;
}

ElfW(Addr) ElfImg::PrefixLookupFirst(std::string_view prefix) const {
    MayInitLinearMap();
    if (auto i = symtabs_.lower_bound(prefix); i != symtabs_.end() && i->first.starts_with(prefix)) {
        LOGD("found prefix {} of {} {:#x} in {} in symtab by linear lookup", prefix, i->first, i->second->st_value, elfPath);
        return i->second->st_value;
    } else {
        return 0;
    }
}


ElfImg::~ElfImg() {
    LOGD("releasing resources");
    //open elf file local
    if (buffer) {
        free(buffer);
        buffer = nullptr;
    }
    //use mmap
    if (header) {
        munmap(header, size);
    }
}

ElfW(Addr)
ElfImg::getSymbOffset(std::string_view name, uint32_t gnu_hash, uint32_t elf_hash) const {
    if (auto offset = GnuLookup(name, gnu_hash); offset > 0) {
        LOGD("found {} {:#x} in {} in dynsym by gnuhash", name, offset, elfPath);
        return offset;
    } else if (offset = ElfLookup(name, elf_hash); offset > 0) {
        LOGD("found {} {:#x} in {} in dynsym by elfhash", name, offset, elfPath);
        return offset;
    } else if (offset = LinearLookup(name); offset > 0) {
        LOGD("found {} {:#x} in {} in symtab by linear lookup", name, offset, elfPath);
        return offset;
    } else {
        return 0;
    }
}

bool ElfImg::findModuleBase() {
    std::vector<proc_map_t> maps;
    proc_map_t *foundMap = nullptr;

    if (!proc_map_parse(maps)) {
        LOGE("failed to open or parse /proc/self/maps");
        return false;
    }

    for (auto &map: maps) {
        if ((map.flags & PROC_MAP_WRITE) != 0)
            continue;
        if ((map.flags & (PROC_MAP_READ | PROC_MAP_PRIVATE)) != (PROC_MAP_READ | PROC_MAP_PRIVATE))
            continue;

        if (!map.file_name.contains(elfPath))
            continue;

        LOGD("found map for {}: {}", elfPath, map.file_name.data());
        elfPath = map.file_name;
        elfFileOffset = 0;
        foundMap = &map;
        break;
    }

    if (!foundMap) {
        // chng to LOGV
        LOGD("did not find module. may be mmap directly from an apk");

        for (auto &map: maps) {
            mz_zip_archive zip;
            memset(&zip, 0, sizeof(mz_zip_archive));

            if ((map.flags & PROC_MAP_WRITE) != 0)
                continue;
            if ((map.flags & (PROC_MAP_READ | PROC_MAP_PRIVATE)) != (PROC_MAP_READ | PROC_MAP_PRIVATE))
                continue;

            if (!map.file_name.ends_with(".apk"))
                continue;

            // open apk
            LOGD("checking in apk: {}", map.file_name);

            if (!mz_zip_reader_init_file(&zip, map.file_name.c_str(), MZ_ZIP_FLAG_DO_NOT_SORT_CENTRAL_DIRECTORY)) {
                LOGD("failed to open apk {}", mz_zip_get_error_string(mz_zip_get_last_error(&zip)));
                continue;
            }

            for (int idx = 0; idx < mz_zip_reader_get_num_files(&zip); ++idx) {
                mz_zip_archive_file_stat zipEntry;
                std::string_view zipEntryName;
                uint64_t entryDataOffset;

                if (!mz_zip_reader_file_stat(&zip, idx, &zipEntry))
                    continue;

                // not what we're looking for
                if (zipEntry.m_is_directory
                    || zipEntry.m_is_encrypted
                    || zipEntry.m_comp_size == 0
                    || zipEntry.m_comp_size != zipEntry.m_uncomp_size) {
                    continue;
                }

                zipEntryName = std::string_view{zipEntry.m_filename};

                // not our lib
                if (!zipEntryName.starts_with("lib/") || !zipEntryName.ends_with(elfPath))
                    continue;

                // get entry data offset
                if (!zip_get_entry_data_offset(&zip, &zipEntry, entryDataOffset))
                    continue;

                // not mmap'ing our lib
                if (map.offset != entryDataOffset)
                    continue;

                LOGD("found lib in apk at path: {} with entry offset {:#x}", zipEntryName, entryDataOffset);
                foundMap = &map;
                elfPath = map.file_name;
                elfFileOffset = entryDataOffset;
                size = zipEntry.m_comp_size;

                break;
            }

            mz_zip_reader_end(&zip);
            if (foundMap) break;
        }
    }

    if (!foundMap) {
        LOGE("did not find module {}", elfPath);
        return false;
    }

    LOGD("got module base {}: {:#x}", elfPath, reinterpret_cast<uint64_t>(foundMap->address_start));
    base = foundMap->address_start;

    return true;
}
