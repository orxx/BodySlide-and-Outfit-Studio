#pragma once

#include <boost/filesystem.hpp>
#include "Portability.h"

class XmlFinder {
public:
	explicit XmlFinder(const std::string& path) {
		boost::filesystem::path fspath(path);
		iter = directory_iterator(path, error);
		if (error) {
			iter = directory_iterator();
			return;
		}
		advanceUntilXml();
	}

	bool atEnd() const {
		return iter == directory_iterator();
	}

	std::string next() {
		std::string path = iter->path().native();
		++iter;
		advanceUntilXml();
		return path;
	}

	boost::system::error_code lastError() const {
		return error;
	}

private:
	typedef boost::filesystem::directory_iterator directory_iterator;
	void advanceUntilXml() {
		while (iter != directory_iterator()) {
			if (iter->status().type() ==
			    boost::filesystem::regular_file &&
			    iter->path().extension() == ".xml") {
				break;
			}
			iter.increment(error);
			if (error) {
				iter = directory_iterator();
				break;
			}
		}
	}

	boost::system::error_code error;
	directory_iterator iter;
};
