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

#include "blobs.h"
#include "project.h"
#include "util.h"

namespace rhost {
    namespace rproj {
        namespace {
            void extract_project(fs::path& zip_file, fs::path& dest_dir, fs::path& temp_dir) {
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
                std::unique_ptr<char[]> buffer(new char[buff_size]);

                for (zip_int64_t i = 0; i < nfiles; ++i) {
                    // Follow the ZIP specification and expect CP-437 encoded names in the ZIP archive(except if they are explicitly marked as UTF-8).
                    // Convert it to UTF-8.
                    fs::path file_name = fs::u8path(zip_get_name(archive, i, ZIP_FL_ENC_STRICT));

                    zip_file_t* zf = nullptr;
                    SCOPE_WARDEN(zf_close, {
                        if (zf) {
                            zip_fclose(zf);
                        }
                    });

                    // Open a file in the archive
                    zf = zip_fopen_index(archive, i, ZIP_FL_UNCHANGED);
                    if (!zf) {
                        std::string errmsg("Error while reading compressed project file: ");
                        errmsg.append(file_name.string());
                        throw std::exception(errmsg.c_str());
                    }

                    // Create a temp file path to extract above file to.
                    fs::path temp_file_name = temp_dir / file_name;
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
                    zip_int64_t bytes_read = 0;
                    do {
                        bytes_read = zip_fread(zf, buffer.get(), buff_size);
                        if (bytes_read < 0) {
                            std::string errmsg("Error while reading compressed project file: ");
                            errmsg.append(file_name.string());
                            throw std::exception(errmsg.c_str());
                        }

                        if (bytes_read > 0) {
                            size_t elements = std::fwrite(buffer.get(), static_cast<size_t>(bytes_read), 1, f);
                            if ((elements != 1) || (std::fflush(f) != 0)) {
                                std::string errmsg("Error while writing de-compressed project file: ");
                                errmsg.append(file_name.string());
                                throw std::exception(errmsg.c_str());
                            }
                        }
                    } while (bytes_read == buff_size);
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

        void save_to_project_folder_worker(blobs::blob_id blob_id, fs::path& project_name, fs::path& dest_dir, fs::path& temp_dir) {
            fs::path zip_file = temp_dir / project_name;
            zip_file.replace_extension(".zip");

            fs::remove(zip_file);

            blobs::save_to_file(blob_id, zip_file);

            fs::path temp_proj_dir = temp_dir / project_name;
            fs::remove_all(temp_proj_dir);

            fs::path dest_proj_dir = dest_dir / project_name;

            extract_project(zip_file, dest_proj_dir, temp_proj_dir);
        }
    }
}