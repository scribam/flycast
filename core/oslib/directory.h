/*
	Copyright 2021 flyinghead

	This file is part of Flycast.

    Flycast is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Flycast is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Flycast.  If not, see <https://www.gnu.org/licenses/>.
*/
#pragma once
#include "types.h"
#include <nowide/config.hpp>
#include <nowide/stackstring.hpp>
#include <dirent.h>
#include <sys/stat.h>

#ifdef _MSC_VER
#include <io.h>
#define W_OK 2
#define R_OK 4
typedef unsigned short mode_t;
#else
#include <unistd.h>
#endif

namespace flycast {
#if !defined(NOWIDE_WINDOWS)
    using ::opendir;
    using ::readdir;
    using ::closedir;
    using ::stat;
    using ::access;
    using ::mkdir;
#else

inline DIR *opendir(char const *dirname)
{
    nowide::wstackstring wname;
    if (!wname.convert(dirname)) {
        errno = EINVAL;
        return nullptr;
    }
    return (DIR *)::_wopendir(wname.get());
}

inline dirent *readdir(DIR *dirstream)
{
	_WDIR *wdir = (_WDIR *)dirstream;
	_wdirent *wdirent = ::_wreaddir(wdir);

	if (wdirent == nullptr)
		return nullptr;

    nowide::stackstring name;
    if (!name.convert(wdirent->d_name)) {
        errno = EINVAL;
        return nullptr;
    }
    static dirent d;
	d.d_ino = wdirent->d_ino;
	d.d_off = wdirent->d_off;
	d.d_type = wdirent->d_type;
	d.d_reclen = sizeof(dirent);
	strncpy(d.d_name, name.get(), sizeof(d.d_name) - 1);
	d.d_name[sizeof(d.d_name) - 1] = '\0';
	d.d_namlen = strlen(d.d_name);

	return &d;
}

inline int closedir(DIR *dirstream)
{
	return ::_wclosedir((_WDIR *)dirstream);
}

inline static void _set_errno(int error)
{
#ifdef _MSC_VER
	::_set_errno (error);
#else
	errno = error;
#endif
}

inline int stat(const char *filename, struct stat *buf)
{
	nowide::wstackstring wname;
    if (!wname.convert(filename)) {
    	_set_errno(EINVAL);
    	return -1;
    }
#ifdef TARGET_UWP
    WIN32_FILE_ATTRIBUTE_DATA attrs;
	bool rc = GetFileAttributesExFromAppW(wname.get(),  GetFileExInfoStandard, &attrs);
	if (!rc)
	{
		_set_errno(GetLastError());
		return -1;
	}
	memset(buf, 0, sizeof(struct stat));
	if (attrs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		buf->st_mode = S_IFDIR;
	else
		buf->st_mode = S_IFREG;
	buf->st_size = attrs.nFileSizeLow;

	constexpr UINT64 WINDOWS_TICK = 10000000u;
	constexpr UINT64 SEC_TO_UNIX_EPOCH = 11644473600llu;
    buf->st_ctime = (unsigned)(((UINT64)attrs.ftCreationTime.dwLowDateTime | ((UINT64)attrs.ftCreationTime.dwHighDateTime << 32)) / WINDOWS_TICK - SEC_TO_UNIX_EPOCH);
    buf->st_mtime = (unsigned)(((UINT64)attrs.ftLastWriteTime.dwLowDateTime | ((UINT64)attrs.ftLastWriteTime.dwHighDateTime << 32)) / WINDOWS_TICK - SEC_TO_UNIX_EPOCH);
    buf->st_atime =(unsigned)(((UINT64)attrs.ftLastAccessTime.dwLowDateTime | ((UINT64)attrs.ftLastAccessTime.dwHighDateTime << 32)) / WINDOWS_TICK - SEC_TO_UNIX_EPOCH);

    return 0;
#else
    struct _stat _st;
    int rc = _wstat(wname.get(), &_st);
    buf->st_ctime = _st.st_ctime;
    buf->st_mtime = _st.st_mtime;
    buf->st_atime = _st.st_atime;
    buf->st_dev = _st.st_dev;
    buf->st_ino = _st.st_ino;
    buf->st_uid = _st.st_uid;
    buf->st_gid = _st.st_gid;
    buf->st_mode = _st.st_mode;
    buf->st_nlink = _st.st_nlink;
    buf->st_rdev = _st.st_rdev;
    buf->st_size = _st.st_size;

    return rc;
#endif
}

inline int access(const char *filename, int how)
{
	nowide::wstackstring wname;
    if (!wname.convert(filename)) {
    	_set_errno(EINVAL);
    	return -1;
    }
#ifdef TARGET_UWP
    WIN32_FILE_ATTRIBUTE_DATA attrs;
	bool rc = GetFileAttributesExFromAppW(wname.get(),  GetFileExInfoStandard, &attrs);
	if (!rc)
	{
		if (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND)
			_set_errno(ENOENT);
		else  if (GetLastError() == ERROR_ACCESS_DENIED)
			_set_errno(EACCES);
		else
			_set_errno(GetLastError());
		return -1;
	}
	if (how != R_OK && (attrs.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
	{
		_set_errno(EACCES);
		return -1;
	}
	else
		return 0;
#else
    return ::_waccess(wname.get(), how);
#endif
}

inline int mkdir(const char *path, mode_t mode) {
	nowide::wstackstring wpath;
    if (!wpath.convert(path)) {
    	errno = EINVAL;
    	return -1;
    }
    return ::_wmkdir(wpath.get());
}
#endif
}
