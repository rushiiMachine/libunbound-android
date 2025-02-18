#pragma once

#include <miniz.h>

/**
 * Gets the zip entry data offset in bytes from the start of the archive.
 * @return -1 on failure otherwise the offset in bytes from the start of the archive.
 */
mz_uint64 zip_get_entry_data_offset(mz_zip_archive *pZip, mz_zip_archive_file_stat *file_stat, mz_uint64 &out);
