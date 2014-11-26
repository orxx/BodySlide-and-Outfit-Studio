#include "ObjFile.h"

ObjFile::ObjFile(void) {
	scale = vec3(1.0f, 1.0f, 1.0f);
	offset = vec3(0.0f, 0.0f, 0.0f);
	uvDupThreshold = 0.005f;

	monitor = NULL_MONITOR;
}

ObjFile::~ObjFile(void) {
	map<string,ObjData*>::iterator dataitems;
	for (dataitems = data.begin(); dataitems != data.end(); ++dataitems) {
		delete dataitems->second;
	}
}

int ObjFile::LoadForNif(const string &inFn, const string& groupName) {
	fstream base(inFn.c_str(), ios_base::in | ios_base::binary);
	if (base.fail()) 
		return 1;
	LoadForNif(base, groupName);
	base.close();
	return 0;
}

int ObjFile::LoadForNif(fstream &base, const string& groupName) {
	ObjData* di = new ObjData();	

	vec3 v;
	vec2 uv;
	vec2 uv2;
	tri t;

	string dump;
	string curgrp;
	string facept1;
	string facept2;
	string facept3;
	string facept4;
	int f[4];
	int ft[4];
	int nPoints = 0;
	int v_idx[4];

	vector<vector3> verts;
	vector<vector2> uvs;
	vector<triangle> tris;
	size_t pos;
	map<int,vector<VertUV>> vertMap;
	map<int,vector<VertUV>>::iterator savedVert;

	bool gotface = false;
	bool readgroup = true;

	while (!base.eof()) {
		if (!gotface)
			base >> dump;
		else gotface = false;

		if (dump.compare("v") == 0) {
			base >> v.x >> v.y >> v.z;
			verts.push_back(v);
		} else if (dump.compare("g") == 0 || dump.compare("o") == 0) {

			base >> curgrp;

			if (di->name != "") {
				data[di->name] = di;
				di = new ObjData;
			}
			di->name = curgrp;
			objGroups.push_back(curgrp);
			
			if (groupName.length() > 0) {
				if (curgrp.compare(groupName) == 0) {
					readgroup = true;
				} else {
					readgroup = false;
				}
			}
		} else if (dump.compare("vt") == 0) {		
			base >> uv.u >> uv.v;
			uvs.push_back(uv);
		} else if (dump.compare("f") == 0) {
			base >> facept1 >> facept2 >> facept3;
			pos = facept1.find('/');
			f[0] = atoi(facept1.c_str()) - 1;
			ft[0] = atoi(facept1.substr(pos + 1).c_str()) - 1;
			pos = facept2.find('/');
			f[1] = atoi(facept2.c_str()) - 1;
			ft[1] = atoi(facept2.substr(pos + 1).c_str()) - 1;
			pos = facept3.find('/');
			f[2] = atoi(facept3.c_str()) - 1;
			ft[2] = atoi(facept3.substr(pos + 1).c_str()) - 1;
			base >> facept4;

			if (facept4 == "f") {
				gotface = true;
				dump = "f";
				nPoints = 3;
			} else if (facept4 == "g") {			
				gotface = true;
				dump = "g";
				nPoints = 3;
			} else if (facept4 == "s") {		
				gotface = true;
				dump = "s";
				nPoints = 3;				
			} else if (facept4.length() > 0) {
				pos = facept4.find('/');
				if (pos == string::npos) {
					gotface = true;
					dump = "f";
					nPoints = 3;
				} else {
					f[3] = atoi(facept4.c_str()) - 1;
					ft[3] = atoi(facept4.substr(pos + 1).c_str()) - 1;
					nPoints = 4;
				}
			}

			if (f[0] == -1 || f[1] == -1 || f[2] == -1)
				continue;
			
			if (!readgroup) 
				continue;

			for (int i = 0; i < nPoints; i++) {
				v_idx[i] = di->verts.size();
				if ((savedVert = vertMap.find(f[i])) != vertMap.end()) {
					for (int j = 0; j < savedVert->second.size(); j++) {
						if (savedVert->second[j].uv == ft[i])
							v_idx[i]=savedVert->second[j].v;
						else if (uvs.size() > 0) {
							uv = uvs[ft[i]];
							uv2 = uvs[savedVert->second[j].uv];
							if (fabs(uv.u - uv2.u) > uvDupThreshold) {
								v_idx[i] = v_idx[i];
								continue;
							} else if (fabs(uv.v - uv2.v) > uvDupThreshold) {
								v_idx[i] = v_idx[i];
								continue;
							}
							v_idx[i] = savedVert->second[j].v;
						}
					}
				}

				if (v_idx[i] == di->verts.size()) {
					vertMap[f[i]].push_back(VertUV(v_idx[i], ft[i]));
					//if(di->name != "PointCheck") {
					//	verts[f[i]].x += offset.x;
					//	verts[f[i]].y += offset.y;
					//	verts[f[i]].z += offset.z;
					//}
					di->verts.push_back(verts[f[i]]);
					if (uvs.size() > 0) {
						di->uvs.push_back(uvs[ft[i]]);
					}
				}
			}
			t.p1 = v_idx[0];
			t.p2 = v_idx[1];
			t.p3 = v_idx[2];
			di->tris.push_back(t);
			if (nPoints == 4) {
				t.p1 = v_idx[0];
				t.p2 = v_idx[2];
				t.p3 = v_idx[3];
				di->tris.push_back(t);
			}
		}
	}
	data[di->name] = di;
	//base.close();
	return 0;
}

int ObjFile::Load(const string &inFn, const string& groupName) {
	int error = 0;
	monitor->Begin("Loading " + inFn);
	fstream base(inFn.c_str(), ios_base::in | ios_base::binary);
	if (base.fail()) {
		error = 1;
		monitor->Error("Failed to load obj file: " + inFn, 1);
	} else {
		Load(base, groupName);
		base.close();
	}
	monitor->End("Obj file load complete.", error);
	return error;
}

int ObjFile::Load(fstream &base, const string& groupName) {
	ObjData* di = new ObjData();	

	vec3 v;
	vec2 uv;
	vec2 uv2;
	tri t;

	string dump;
	string curgrp;
	string facept1;
	string facept2;
	string facept3;
	string facept4;
	int f[4];
	int ft[4];
	int nPoints = 0;
	int v_idx[4];

	vector3 * verts;
	vector2 * uvs;
	di->verts.resize(65536);
	di->uvs.resize(65536);

	int vcursor = 0;

	verts = (vector3*)malloc(sizeof(vector3) * 65536);
	uvs = (vector2*)malloc(sizeof(vector2) * 65536);

	//vector<vector3> verts(65536);
	//vector<vector2> uvs(65536);
	//vector<triangle> tris;
	size_t pos;
	unordered_map<int,vector<VertUV>> vertMap;
	unordered_map<int,vector<VertUV>>::iterator savedVert;

	int vc = 0;
	//int tc = 0;
	int uvc = 0;
	//verts.reserve(65536);
	//tris.reserve(65536);
	//uvs.reserve(65536);

	int stage = 0;
	bool gotface = false;
	bool readgroup = true;

	while (!base.eof()) {
		if (!gotface)
			base >> dump;
		else gotface = false;

		if (dump.compare("v") == 0) {
			if (stage == 0) {
				monitor->Update("Loading vertices...");
				stage++;
			}

			base >> verts[vc].x >> verts[vc].y >> verts[vc].z;
			//base >> di->verts[vc].x >> di->verts[vc].y >> di->verts[vc].z;
			vc++;
			//v.x = (v.x*scale.x) + offset.x;
			//v.y = (v.y*scale.y) + offset.y;
			//v.z = (v.z*scale.z) + offset.z;
			//verts.push_back(v);
			//verts[vc++] = v;
		} else if (dump.compare("g") == 0 || dump.compare("o") == 0) {
			base >> curgrp;

			if (di->name != "") {
				data[di->name] = di;
				di = new ObjData;
			}
			di->name = curgrp;
			objGroups.push_back(curgrp);
			
			if (groupName.length() > 0) {
				if (curgrp.compare(groupName) == 0) {
					readgroup = true;
				} else {
					readgroup = false;
				}
			}
		} else if (dump.compare("vt") == 0) {				
			if (stage == 1) {
				monitor->Update("Loading Uvs...");
				stage++;
			}	
			base >> uvs[uvc].u >> uvs[uvc].v;
			uvc++;
			//uvs.push_back(uv);
			//uvs[uvc++] = uv;
		} else if (dump.compare("f") == 0) {			
			if (stage == 2) {
				monitor->Update("Loading faces...");
				stage++;
			}	
			base >> facept1 >> facept2 >> facept3;
			pos = facept1.find('/');
			f[0] = atoi(facept1.c_str()) - 1;
			ft[0] = atoi(facept1.substr(pos + 1).c_str()) - 1;
			pos = facept2.find('/');
			f[1] = atoi(facept2.c_str()) - 1;
			ft[1] = atoi(facept2.substr(pos + 1).c_str()) - 1;
			pos = facept3.find('/');
			f[2] = atoi(facept3.c_str()) - 1;
			ft[2] = atoi(facept3.substr(pos + 1).c_str()) - 1;
			base >> facept4;

			if (facept4 =="f") {
				gotface = true;
				dump = "f";
				nPoints = 3;
			} else if (facept4 == "g") {			
				gotface = true;
				dump = "g";
				nPoints = 3;
			} else if(facept4 == "s" ){
				gotface = true;
				dump = "s";
				nPoints = 3;
			} else if (facept4.length() > 0) {
				pos = facept4.find('/');
				if (pos == string::npos) {
					gotface = true;
					dump = "f";
					nPoints = 3;
				} else {
					f[3] = atoi(facept4.c_str()) - 1;
					ft[3] = atoi(facept4.substr(pos + 1).c_str()) - 1;
					nPoints = 4;
				}
			}

			if (f[0] == -1 || f[1] == -1 || f[2] == -1)
				continue;
			
			if (!readgroup) 
				continue;
			
			for (int i = 0; i < nPoints; i++) {
				v_idx[i] = vcursor; //di->verts.size(); 
				if ((savedVert = vertMap.find(f[i])) != vertMap.end()) {
					for (int j = 0; j < savedVert->second.size(); j++) {
						if (savedVert->second[j].uv == ft[i])
							v_idx[i]=savedVert->second[j].v;
						else {
							uv = uvs[ft[i]];
							uv2 = uvs[savedVert->second[j].uv];
							if (fabs(uv.u - uv2.u) > uvDupThreshold) {
								v_idx[i] = v_idx[i];
								continue;
							} else if (fabs(uv.v - uv2.v) > uvDupThreshold) {
								v_idx[i] = v_idx[i];
								continue;
							}
							v_idx[i] = savedVert->second[j].v;
						}
					}
				}
				/**/
				if (v_idx[i] == vcursor) {
					vertMap[f[i]].push_back(VertUV(v_idx[i], ft[i]));
					//if(di->name != "PointCheck") {
					//	verts[f[i]].x += offset.x;
					//	verts[f[i]].y += offset.y;
					//	verts[f[i]].z += offset.z;
					//}
					di->verts[vcursor] = verts[f[i]];
					di->uvs[vcursor] = uvs[f[i]];
					//di->verts.push_back(verts[f[i]]);
					vcursor ++;
				}
			}
			//di->verts.push_back(verts[f[i]]);
			t.p1 = v_idx[0];
			t.p2 = v_idx[1];
			t.p3 = v_idx[2];
			di->tris.push_back(t);
			if (nPoints == 4) {
				t.p1 = v_idx[0];
				t.p2 = v_idx[2];
				t.p3 = v_idx[3];
				di->tris.push_back(t);
			}
		}
	}
	data[di->name] = di;
	//base.close();
	free(verts);
	free(uvs);

	return 0;
}

bool ObjFile::CopyDataForGroup(const string &name, vector<vec3> *v, vector<tri> *t, vector<vec2> *uv) {
	int i;
	if (data.find(name) == data.end()) 
		return false;
	ObjData* od = data[name];
	if (v) {
		v->clear();
		v->resize(od->verts.size());
		for (i = 0; i < od->verts.size(); i++) {
			(*v)[i].x = (od->verts[i].x + offset.x) * scale.x;
			(*v)[i].y = (od->verts[i].y + offset.y) * scale.y;
			(*v)[i].z = (od->verts[i].z + offset.z) * scale.z;
		}
	}
	if (t) {
		t->clear();
		t->resize(od->tris.size());
		for (i = 0; i < od->tris.size(); i++) {
			(*t)[i] = od->tris[i];
		}
	}
	if (uv) {
		uv->clear();
		uv->resize(od->uvs.size());
		for (i = 0; i < od->uvs.size(); i++) {
			(*uv)[i] = od->uvs[i];
		}
	}
	return true;
}

bool ObjFile::CopyDataForIndex(int index, vector<vec3> *v, vector<tri> *t, vector<vec2> *uv) {
	if (objGroups.size() > index)
		return CopyDataForGroup(objGroups[index], v, t, uv);
	else return false;
}

void ObjFile::GetGroupList(std::vector<string> &shapeNames) {
	shapeNames.clear();
	for (int i = 0; i < objGroups.size(); i++) {
		shapeNames.push_back(objGroups[i]);
	}
}