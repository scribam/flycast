/*
    Created on: Nov 23, 2018

	Copyright 2018 flyinghead

	This file is part of reicast.

    reicast is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    reicast is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with reicast.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "archive.h"
#include "7zArchive.h"
#include "ZipArchive.h"
#include "oslib/storage.h"
#include "stdclass.h"

#include <array>

Archive *OpenArchive(const std::string& path)
{
	try {
		if (hostfs::storage().getFileInfo(path).isDirectory)
			return nullptr;
	} catch (const hostfs::StorageException& e) {
	}

	std::string extension;
	if (file_exists(path)) {
		extension = get_file_extension(path);
	} else {
		for (const auto& ext : {"7z", "7Z", "zip", "ZIP"}) {
			if (file_exists(path + "." + ext)) {
				extension = ext;
				string_tolower(extension);
				break;
			}
		}
	}

	Archive* archive = nullptr;

	if (extension == "7z")
		archive = new SzArchive();
	else if (extension == "zip")
		archive = new ZipArchive();

	if (archive != nullptr) {
		if (archive->Open(path))
			return archive;
		delete archive;
	}

	return nullptr;
}
