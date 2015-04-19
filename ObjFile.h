#pragma once

//#define _HAS_ITERATOR_DEBUGGING 0
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include "Object3d.h"
#include <fstream>
#include "AsyncMonitor.h"
#include "Portability.h"

using namespace std;

struct VertUV {
	int v;
	int uv;
	VertUV (int inV, int inUV):v(inV), uv(inUV) {}
};

struct ObjData {
	string name;
	vector<vec3> verts;
	vector<tri> tris;
	vector<vec2> uvs;
};

class ObjFile {
	vector<string> objGroups;
	map<string, ObjData*> data;

public:
	float uvDupThreshold;
	vec3 scale;
	vec3 offset;

	/* async load helper values */
	string _asyncFN;
	string _asyncGN;
	AsyncMonitor* monitor;

	ObjFile(void);
	~ObjFile(void);

	void SetScale(const vec3& inScale) { scale = inScale; }
	void SetOffset(const vec3& inOffset) { offset = inOffset; }

	int LoadForNif(const string& inFn,const string& groupName = "");
	int LoadForNif(fstream& base,const string& groupName = "");

	int Load(const string& inFn,const string& groupName = "");
	int Load(fstream& base,const string& groupName = "");

	bool CopyDataForGroup(const string& name, vector<vec3>* v, vector<tri>* t, vector<vec2>* uv);
	bool CopyDataForIndex(int index, vector<vec3>* v, vector<tri>* t, vector<vec2>* uv);

	void GetGroupList(vector<string>& outNames);

	static unsigned int WINAPI _startAsyncLoad (void* param) {
		ObjFile* obj = (ObjFile*)param;

		return obj->Load(obj->_asyncFN, obj->_asyncGN);
	}

	void LoadAsync(AsyncMonitor* threadMonitor, const string& inFn, const string& groupName = "") {
		monitor = threadMonitor;
		_asyncFN = inFn;
		_asyncGN = groupName;

		monitor->threadHandle = std::thread(_startAsyncLoad, (void*)this);		
		// Currently AsyncMonitor never joins, so detach
		monitor->threadHandle.detach();
	}
};
