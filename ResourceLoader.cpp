#include "ResourceLoader.h"

#include "GLShader.h"
#include "Portability.h"

#include <boost/functional/hash.hpp>

#ifdef _WIN32
#include "SOIL.h"
#ifdef _DEBUG
#pragma comment (lib, "SOIL_d.lib")
#else
#pragma comment (lib, "SOIL.lib")
#endif
#else // !_WIN32
#include <SOIL/SOIL.h>
#endif

using std::string;

size_t ResourceLoader::MatKeyHash::operator()(const MaterialKey& key) const {
	std::hash<std::string> strHash;
	size_t ret = 0;
	boost::hash_combine(ret, strHash(std::get<0>(key)));
	boost::hash_combine(ret, strHash(std::get<1>(key)));
	boost::hash_combine(ret, strHash(std::get<2>(key)));
	return ret;
}

ResourceLoader::ResourceLoader() {
}

ResourceLoader::~ResourceLoader() {
}

GLMaterial* ResourceLoader::AddMaterial(const string& textureFile,
					const string& vShaderFile,
					const string& fShaderFile) {
	auto texPath = NativePath(textureFile);
	MaterialKey key(texPath, vShaderFile, fShaderFile);
	auto it = materials.find(key);
	if (it != materials.end()) {
		return it->second.get();
	}

	// TODO: The textureFile string usually doesn't match the
	// uppercase/lowercase spelling of the file on disk.  On windows this
	// isn't an issue since the filesystem is case insensitive.
	// On Linux we should add code here that does a case-insensitive search
	// for the desired file.

	unsigned int texid = SOIL_load_OGL_texture(texPath.c_str(), SOIL_LOAD_AUTO, 0, SOIL_FLAG_TEXTURE_REPEATS);
	if (!texid) {
		auto errstr = SOIL_last_result();
		std::cerr << "unable to load texture " << texPath <<
			": " << errstr << std::endl;
		return nullptr;
	}

	auto& entry = materials[key];
	entry.reset(new GLMaterial(texid, vShaderFile.c_str(), fShaderFile.c_str()));
	return entry.get();
}

void ResourceLoader::Cleanup() {
	materials.clear();
}
