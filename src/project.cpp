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
            enum class dir_copy_options {
                none = 0,
                skip_existing = 1, overwrite_existing = 2, update_existing = 4,
                recursive = 8,
                copy_symlinks = 16, skip_symlinks = 32,
                directories_only = 64, create_symlinks = 128, create_hard_links = 256,
                _Unspecified_recursion_prevention_tag = 512
            };
            RHOST_BITMASK_OPS(dir_copy_options);

            bool is_recursive_copy(dir_copy_options opts)
            {
                // Checks a _Copy_options for whether copy should call itself recursively
                if (opts == dir_copy_options::none)
                    // This supports "copying a directory" as copying the directory and
                    // files therein but not subdirectories.
                    return (true);
                if ((opts & dir_copy_options::recursive) != dir_copy_options::none)
                    return (true);
                return (false);
            }

            void copy_file(const fs::path& source_path, const fs::path& dest_path, dir_copy_options options) {
                if ((options & dir_copy_options::overwrite_existing) != dir_copy_options::none
                    || (options & dir_copy_options::update_existing) != dir_copy_options::none) {
                    fs::copy_file(source_path, dest_path, fs::copy_option::overwrite_if_exists);
                } else if ((options & dir_copy_options::skip_existing) != dir_copy_options::none
                    && fs::exists(dest_path)) {
                    ; // NO OP
                } else {
                    fs::copy_file(source_path, dest_path, fs::copy_option::none);
                }
            }

            void copy(const fs::path& source_path, const fs::path& dest_path, dir_copy_options options) {
                // copy source_path to dest_path, general
                fs::file_status oldStat;
                fs::file_status newStat;

                if ((options & dir_copy_options::create_symlinks) != dir_copy_options::none
                    || (options & dir_copy_options::skip_symlinks) != dir_copy_options::none) {
                    // get symlink status
                    oldStat = fs::symlink_status(source_path);
                    newStat = fs::symlink_status(dest_path);
                } else {
                    // get file status
                    oldStat = fs::status(source_path);
                    newStat = fs::status(dest_path);
                } 
                
                if (!fs::exists(oldStat)
                    || fs::equivalent(source_path, dest_path)
                    || fs::is_other(oldStat)
                    || fs::is_other(newStat)
                    || (fs::is_directory(oldStat) && fs::is_regular_file(newStat))) {
                    throw std::runtime_error("Copy operation not permitted.");
                } else if (fs::is_symlink(oldStat)) {
                    if ((options & dir_copy_options::skip_symlinks) != dir_copy_options::none) {
                        ; // NO OP
                    } else if (!exists(newStat) && (options & dir_copy_options::copy_symlinks) != dir_copy_options::none) {
                        copy_symlink(source_path, dest_path);
                    } else {
                        throw std::runtime_error("Copy operation not terminated.");
                    }
                } else if (fs::is_regular_file(oldStat)) {
                    if ((options & dir_copy_options::directories_only) != dir_copy_options::none) {
                        ; // NO OP
                    } else if ((options & dir_copy_options::create_symlinks) != dir_copy_options::none) {
                        fs::create_symlink(source_path, dest_path);
                    } else if ((options & dir_copy_options::create_hard_links) != dir_copy_options::none) {
                        fs::create_hard_link(source_path, dest_path);
                    } else if (fs::is_directory(newStat)) {
                        copy_file(source_path, dest_path / source_path.filename(), options);
                    } else {
                        copy_file(source_path, dest_path, options);
                    }
                } else if (fs::is_directory(oldStat)  && is_recursive_copy(options)) {
                    // copy directory recursively
                    if (!fs::exists(dest_path) && !fs::create_directory(dest_path)) {
                        throw std::runtime_error("Operation not terminated.");
                    }
                    for (fs::directory_iterator next(source_path), end; next != end; ++next) {
                        copy(next->path(), dest_path / next->path().filename(),
                            options | dir_copy_options::_Unspecified_recursion_prevention_tag);
                    }
                }
            }

            

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
                    throw std::runtime_error("Error while opening compressed project file.");
                }

                // Get the number of entries in the archive
                zip_int64_t nfiles = zip_get_num_entries(archive, ZIP_FL_UNCHANGED);

                // 10 MB buffer to extract files
                size_t buff_size = 10 * 1024 * 1024;
                std::unique_ptr<char[]> buffer(new char[buff_size]);

                for (zip_int64_t i = 0; i < nfiles; ++i) {
                    // Follow the ZIP specification and expect CP-437 encoded names in the ZIP archive(except if they are explicitly marked as UTF-8).
                    // Convert it to UTF-8.
                    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> cvt;
                    fs::path file_name(cvt.from_bytes(zip_get_name(archive, i, ZIP_FL_ENC_STRICT)));

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
                        throw std::runtime_error(errmsg.c_str());
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
                            throw std::runtime_error(errmsg.c_str());
                        }

                        if (bytes_read > 0) {
                            size_t elements = std::fwrite(buffer.get(), static_cast<size_t>(bytes_read), 1, f);
                            if ((elements != 1) || (std::fflush(f) != 0)) {
                                std::string errmsg("Error while writing de-compressed project file: ");
                                errmsg.append(file_name.string());
                                throw std::runtime_error(errmsg.c_str());
                            }
                        }
                    } while (bytes_read == buff_size);
                }

                if (!fs::exists(dest_dir)) {
                    fs::create_directories(dest_dir);
                }

                // Copy all decompressed files to their destination
                copy(temp_dir, dest_dir, dir_copy_options::recursive | dir_copy_options::overwrite_existing);

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