#include "XmlFinder.h"


using std::string;

#ifdef _WIN32

XmlFinder::XmlFinder(const std::string& path) {
	basePath = path + "\\";
	string filefilter = path + "\\*.xml";
	hfind = FindFirstFileA(filefilter.c_str(), &wfd);
	if (hfind == INVALID_HANDLE_VALUE) {
		return;
	}
}

XmlFinder::~XmlFinder() {
	if (hfind != INVALID_HANDLE_VALUE) {
		close();
	}
}

std::string XmlFinder::next() {
	std::string filename = basePath + wfd.cFileName;

	if (!FindNextFileA(hfind, &wfd)) {
		DWORD searchStatus = GetLastError();
		if (searchStatus != ERROR_NO_MORE_FILES) {
			error = true;
		}
		close();
	}

	return filename;
}

void XmlFinder::close() {
	FindClose(hfind);
	hfind = INVALID_HANDLE_VALUE;
}

#else // !_WIN32

#include <dirent.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <cstring>

XmlFinder::XmlFinder(const std::string& path) {
	dirPath = path + "/";
	dir = opendir(path.c_str());
	if (dir == nullptr) {
		error = errno;
		return;
	}

	// Allocate the dirent as recommended in the readdir_r manpage
	auto name_max = pathconf(path.c_str(), _PC_NAME_MAX);
	if (name_max == -1) {
		name_max = 255; // guess;
	}
	size_t len = offsetof(struct dirent, d_name) + name_max + 1;
	ent = static_cast<struct dirent*>(malloc(len));
	if (ent == nullptr) {
		closedir(dir);
		throw std::bad_alloc();
	}

	// Read the first entry
	advanceUntilXml();
}

XmlFinder::~XmlFinder() {
	if (dir != nullptr) {
		close();
	}
}

void XmlFinder::advanceUntilXml() {
	while (dir != nullptr) {
		struct dirent* ret;
		int rc = readdir_r(dir, ent, &ret);
		if (rc != 0) {
			error = errno;
			close();
			return;
		}
		if (ret == nullptr) {
			close();
			return;
		}

#ifdef _DIRENT_HAVE_D_TYPE
		if (ent->d_type != DT_REG) {
			continue;
		}
#else
		struct stat s;
		int rc = stat((dirPath + ent->d_name).c_str(), &s);
		if (rc != 0) {
			continue;
		}
		if (!S_ISREG(s.st_mode)) {
			continue;
		}
#endif

		auto namelen = strlen(ent->d_name);
		if (namelen >= 4 &&
		    strcasecmp(ent->d_name + namelen - 4, ".xml") == 0) {
			break;
		}
	}
}

void XmlFinder::close() {
	closedir(dir);
	dir = nullptr;
	free(ent);
	ent = nullptr;
}

#endif // !_WIN32
