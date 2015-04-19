#pragma once

#include <string>

#ifdef _WIN32

#include "stdafx.h"

class XmlFinder {
public:
	explicit XmlFinder(const std::string& path);
	virtual ~XmlFinder();

	bool atEnd() const {
		return hfind == INVALID_HANDLE_VALUE;
	}

	std::string next();

	bool hadError() const {
		return error;
	}

private:
	void advanceUntilXml();
	void close();

	WIN32_FIND_DATAA wfd;
	HANDLE hfind;
	std::string basePath;
	bool error{false};
};

#else

#include <dirent.h>

class XmlFinder {
public:
	explicit XmlFinder(const std::string& path);
	virtual ~XmlFinder();

	bool atEnd() const {
		return (dir == nullptr);
	}

	std::string next() {
		std::string path(dirPath + ent->d_name);
		advanceUntilXml();
		return path;
	}

	bool hadError() const {
		return error;
	}

private:
	void advanceUntilXml();
	void close();

	int error{0};
	DIR* dir{nullptr};
	struct dirent* ent{nullptr};
	std::string dirPath;
};
#endif
