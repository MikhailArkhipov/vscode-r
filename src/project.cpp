/* ****************************************************************************
*
* Copyright (c) Microsoft Corporation. All rights reserved.
*
*
* This file is part of Microsoft R Host.
*
* Microsoft R Host is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* (at your option) any later version.
*
* Microsoft R Host is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Microsoft R Host.  If not, see <http://www.gnu.org/licenses/>.
*
* ***************************************************************************/


#include "project.h"
#include "util.h"
#include <zip.h>

namespace fs = std::experimental::filesystem;

namespace rhost {
    namespace rproj {
        namespace {
            void extract_project_internal(fs::path zip_file, fs::path dest_dir, fs::path temp_dir) {
                // Open ZIP archive file
                int zip_err = ZIP_ER_OK;
                zip_t* archive = zip_open(zip_file.make_preferred().string().c_str(), ZIP_RDONLY, &zip_err);
                SCOPE_WARDEN(zip_close, {
                    if (archive) {
                        zip_close(archive);
                    }
                });

                if (zip_err != ZIP_ER_OK) {
                    throw std::exception("Error while opening compressed project file.");
                }

                // Get the number of entries in the archive
                zip_int64_t nfiles = zip_get_num_entries(archive, ZIP_FL_UNCHANGED);

                // 10 MB buffer to extract files
                size_t buff_size = 10 * 1024 * 1024;
                std::unique_ptr<byte[]> buffer(new byte[buff_size]);

                for (zip_int64_t i = 0; i < nfiles; ++i) {
                    // Follow the ZIP specification and expect CP - 437 encoded names in the ZIP archive(except if they are explicitly marked as UTF - 8).
                    // Convert it to UTF - 8.
                    fs::path file_archive_name = fs::u8path(zip_get_name(archive, i, ZIP_FL_ENC_STRICT));

                    zip_file_t* zf = nullptr;
                    SCOPE_WARDEN(zf_close, {
                        if (zf) {
                            zip_fclose(zf);
                        }
                    });
                    
                    // Open a file in the archive
                    zf = zip_fopen_index(archive, i, ZIP_FL_UNCHANGED);
                    if (!zf) {
                        throw std::exception("Error while reading compressed project file.");
                    }

                    // Create a temp file path to extract above file to.
                    fs::path temp_file_name = temp_dir / file_archive_name;
                    fs::path parent = temp_file_name.parent_path();
                    if (!fs::exists(parent)) {
                        fs::create_directories(parent);
                    }

                    FILE* f = std::fopen(temp_file_name.make_preferred().string().c_str(), "wb");
                    SCOPE_WARDEN(f_close, {
                        if (f) {
                            std::fclose(f);
                        }
                    });

                    // Decompress the file
                    zip_int64_t bytes_read = zip_fread(zf, buffer.get(), buff_size);
                    while (bytes_read == buff_size) {
                        std::fwrite(buffer.get(), sizeof(byte), static_cast<size_t>(bytes_read), f);
                        bytes_read = zip_fread(zf, buffer.get(), buff_size);
                        std::fflush(f);
                    }

                    if (bytes_read < 0) {
                        throw std::exception("Error while reading compressed project file.");
                    }

                    if (bytes_read > 0) {
                        fwrite(buffer.get(), sizeof(byte), static_cast<size_t>(bytes_read), f);
                        fflush(f);
                    }
                }

                if (!fs::exists(dest_dir)) {
                    fs::create_directories(dest_dir);
                }

                // Copy all decompressed files to their destination
                fs::copy(temp_dir, dest_dir, fs::copy_options::recursive | fs::copy_options::overwrite_existing);

                // Delete all temp files created during decompression phase
                fs::remove_all(temp_dir);
            }
        }

        void extract_project(fs::path zip_file, fs::path dest_dir, fs::path temp_dir) {
            return extract_project_internal(zip_file, dest_dir, temp_dir);
        }
    }
}