#include "zip_util.hpp"
#include <miniz.h>
#include <miniz_zip.h>

/* Various ZIP archive enums. To completely avoid cross platform compiler alignment and platform endian issues, miniz.c doesn't use structs for any of this stuff. */
enum {
    /* ZIP archive identifiers and record sizes */
    MZ_ZIP_LOCAL_DIR_HEADER_SIG = 0x04034b50,
    MZ_ZIP_LOCAL_DIR_HEADER_SIZE = 30,

    /* Local directory header offsets */
    MZ_ZIP_LDH_SIG_OFS = 0,
    MZ_ZIP_LDH_VERSION_NEEDED_OFS = 4,
    MZ_ZIP_LDH_BIT_FLAG_OFS = 6,
    MZ_ZIP_LDH_METHOD_OFS = 8,
    MZ_ZIP_LDH_FILE_TIME_OFS = 10,
    MZ_ZIP_LDH_FILE_DATE_OFS = 12,
    MZ_ZIP_LDH_CRC32_OFS = 14,
    MZ_ZIP_LDH_COMPRESSED_SIZE_OFS = 18,
    MZ_ZIP_LDH_DECOMPRESSED_SIZE_OFS = 22,
    MZ_ZIP_LDH_FILENAME_LEN_OFS = 26,
    MZ_ZIP_LDH_EXTRA_LEN_OFS = 28,
    MZ_ZIP_LDH_BIT_FLAG_HAS_LOCATOR = 1 << 3,
};

mz_uint64 zip_get_entry_data_offset(mz_zip_archive *pZip, mz_zip_archive_file_stat *file_stat, mz_uint64 &out) {
    mz_uint32 local_header_u32[(MZ_ZIP_LOCAL_DIR_HEADER_SIZE + sizeof(mz_uint32) - 1) / sizeof(mz_uint32)];
    mz_uint8 *pLocal_header = (mz_uint8 *) local_header_u32;

    if ((!pZip) || (!pZip->m_pState) || (!pZip->m_pRead))
        return false;

    /* A directory or zero length file */
    if ((file_stat->m_is_directory) || (!file_stat->m_comp_size))
        return false;

    mz_uint64 cur_file_ofs = file_stat->m_local_header_ofs;

    /* Read and parse the local directory entry. */
    if (pZip->m_pRead(pZip->m_pIO_opaque, cur_file_ofs, pLocal_header, MZ_ZIP_LOCAL_DIR_HEADER_SIZE) != MZ_ZIP_LOCAL_DIR_HEADER_SIZE)
        return false;

    if (MZ_READ_LE32(pLocal_header) != MZ_ZIP_LOCAL_DIR_HEADER_SIG)
        return false;

    cur_file_ofs += (mz_uint64) (MZ_ZIP_LOCAL_DIR_HEADER_SIZE) +
                    MZ_READ_LE16(pLocal_header + MZ_ZIP_LDH_FILENAME_LEN_OFS) +
                    MZ_READ_LE16(pLocal_header + MZ_ZIP_LDH_EXTRA_LEN_OFS);

    out = cur_file_ofs;
    return true;
}
