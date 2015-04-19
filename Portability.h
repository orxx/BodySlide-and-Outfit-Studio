#pragma once

#include <algorithm>
#include <string>
#include <system_error>

inline std::string PathBaseName(std::string path) {
#ifdef _WIN32
    auto n = path.rfind('\\');
#else
    std::replace(path.begin(), path.end(), '\\', '/');
    auto n = path.rfind('/');
#endif
    if (n == std::string::npos) {
        return path;
    }
    return path.substr(n + 1);
}

#ifdef _WIN32

#include "stdafx.h"

inline std::string NativePath(std::string path) {
    return path;
}

inline void ThrowSysError(const std::string& what) {
	throw std::system_error(GetLastError(), std::system_category(), what);
}

inline std::string GetCurDir() {
	char buffer[MAX_PATH];
	auto ret = GetCurrentDirectoryA(MAX_PATH, buffer);
	if (ret == 0) {
		ThrowSysError("failed to get current directory");
	}
	return buffer;
}

inline void CreateDir(const char* path) {
	int error = SHCreateDirectoryExA(NULL, path, NULL);
	if (error != ERROR_SUCCESS) {
		ThrowSysError(std::string("failed to create directory ") +
			      path);
	}
}

inline void EnsureDir(const char* path) {
	int error = SHCreateDirectoryExA(NULL, path, NULL);
	if (error != ERROR_SUCCESS && error != ERROR_ALREADY_EXISTS &&
	    error != ERROR_FILE_EXISTS) {
		ThrowSysError(std::string("failed to create directory ") +
			      path);
	}
}

inline bool IsRegularFile(const char* path) {
	int attrs = GetFileAttributesA(path);
	if (attrs == INVALID_FILE_ATTRIBUTES) {
		return false;
	}
	if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
		return false;
	}
	return true;
}

#define snprintf(buf, len, fmt, ...) \
    _snprintf_s((buf), (len), (len), (fmt), ##__VA_ARGS__)

#else // !_WIN32

#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#define WINAPI

#define MAX_PATH PATH_MAX

inline std::string NativePath(std::string path) {
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}

inline void ThrowSysError(const std::string& what) {
	throw std::system_error(errno, std::system_category(), what);
}

inline int GetLastError() {
	// We could return errno, but really we should probably just replace
	// any code using this with something more portable.
	return 0;
}

inline std::string GetCurDir() {
	char buffer[PATH_MAX];
	if (getcwd(buffer, PATH_MAX) == nullptr) {
		ThrowSysError("failed to get current directory");
	}
	return buffer;
}

inline void CreateDir(const char* path) {
	int rc = mkdir(path, 0777);
	if (rc != 0) {
		ThrowSysError(std::string("failed to create directory ") +
			      path);
	}
}

inline void EnsureDir(const char* path) {
	int rc = mkdir(path, 0777);
	if (rc == 0 || errno == EEXIST) {
		return;
	}
	ThrowSysError(std::string("failed to create directory ") +
		      path);
}

inline bool IsRegularFile(const char* path) {
	struct stat s;
	int rc = stat(path, &s);
	if (rc != 0) {
		return false;
	}
	return S_ISREG(s.st_mode);
}

#endif // !_WIN32
