#pragma once

#include <memory>
#include <string>
#include <unordered_map>

class GLMaterial;

class ResourceLoader {
public:
	ResourceLoader();
	virtual ~ResourceLoader();

	GLMaterial* AddMaterial(const std::string& textureFile,
				const std::string& vShaderFile,
				const std::string& fShaderFile);

	void Cleanup();

private:
	// If N3983 gets accepted into a future C++ standard then
	// we wouldn't have to explicitly define our own hash here.
	typedef std::tuple<std::string, std::string, std::string> MaterialKey;
	struct MatKeyHash {
		size_t operator()(const MaterialKey& key) const;
	};
	typedef std::unordered_map<MaterialKey, std::unique_ptr<GLMaterial>,
		                   MatKeyHash> MaterialCache;

	MaterialCache materials;
};
