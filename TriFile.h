#pragma once

#include "Object3d.h"
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <fstream>
#include <memory>
#include <algorithm>

using namespace std;

typedef unsigned char byte;
typedef unsigned int uint;
typedef unsigned short ushort;


struct MorphData {
	string name;
	map<int, vec3> offsets;
};

typedef shared_ptr<MorphData> MorphDataPtr;

class TriFile {
	map<string, vector<MorphDataPtr>> shapeMorphs;

public:
	int Read(string fileName);
	int Write(string fileName, bool packed = false);
	
	void AddMorph(string shapeName, MorphDataPtr data);
	void DeleteMorph(string shapeName, string morphName);
	void DeleteMorphs(string shapeName);
	void DeleteMorphFromAll(string morphName);

	MorphDataPtr GetMorph(string shapeName, string morphName);
};
