#include "Anim.h"
#include <sstream>
#include <wx/wx.h>

bool AnimInfo::AddShapeBone(const string& shape, AnimBone& boneDataRef) {
	for (auto bone : shapeBones[shape])
		if (!bone.compare(boneDataRef.boneName))
			return false;

	shapeBones[shape].push_back(boneDataRef.boneName);
	AnimSkeleton::getInstance().RefBone(boneDataRef.boneName);
	return true;
}

bool AnimInfo::RemoveShapeBone(const string& shape, const string& boneName) {
	int bidx = 0;
	bool found = false;
	for (auto bone : shapeBones[shape]) {
		if (bone.compare(boneName) == 0) {
			found = true;
			break;
		}
		bidx++;
	}

	if (!found)
		return false;

	shapeBones[shape].erase(shapeBones[shape].begin() + bidx);
	shapeSkinning[shape].RemoveBone(bidx);

	AnimSkeleton::getInstance().ReleaseBone(boneName);
	return true;
}

void AnimInfo::Clear() {
	if (refNif && refNif->IsValid()) {
		vector<string> shapes;
		refNif->GetShapeList(shapes);

		for (auto shapeBoneList : shapeBones)
			for (auto boneName : shapeBoneList.second)
				AnimSkeleton::getInstance().ReleaseBone(boneName);

		shapeSkinning.clear();
		for (auto s : shapes)
			shapeBones[s].clear();

		refNif = NULL;
	}
}

void AnimInfo::ClearShape(const string& shape) {
	for (auto boneName : shapeBones[shape])
		AnimSkeleton::getInstance().ReleaseBone(boneName);

	shapeBones.erase(shape);
	shapeSkinning.erase(shape);
}

bool AnimInfo::LoadFromNif(NifFile* nif) {
	vector<string> shapes;
	nif->GetShapeList(shapes);

	Clear();

	for (auto s : shapes)
		LoadFromNif(nif, s);

	refNif = nif;
	return true;
}

bool AnimInfo::LoadFromNif(NifFile* nif, const string& shape) {
	vector<string> boneNames;
	vector<int> boneIndices;
	string invalidBones = "";

	if (!nif->GetShapeBoneList(shape, boneNames))
		return false;

	int slot = 0;
	for (auto bn : boneNames) {
		if (!AnimSkeleton::getInstance().RefBone(bn)) {
			AnimBone& cstm = AnimSkeleton::getInstance().AddBone(bn, true);
			if (!cstm.isValidBone)
				invalidBones += bn + "\n";

			vector<vec3> r;
			nif->GetNodeTransform(bn, r, cstm.trans, cstm.scale);
			cstm.rot.Set(r);
			cstm.localRot = cstm.rot;
			cstm.localTrans = cstm.trans;
			AnimSkeleton::getInstance().RefBone(bn);
		}
		shapeBones[shape].push_back(bn);
		boneIndices.push_back(slot++);
	}

	shapeSkinning[shape] = AnimSkin(nif, shape, boneIndices);

	if (!invalidBones.empty())
		wxMessageBox("Bones in shape '" + shape + "' not found in reference skeleton:\n\n" + invalidBones, "Invalid Bones");

	return true;
}

void AnimInfo::GetBoneXForm(const string& boneName, skin_transform& stransform) {
	AnimBone b;
	if (AnimSkeleton::getInstance().GetBone(boneName, b)) {
		stransform.translation = b.localTrans;
		stransform.scale = b.scale;
		b.rot.GetRow(0, stransform.rotation[0]); // = b.rot[0];
		b.rot.GetRow(1, stransform.rotation[1]); // = b.rot[1];
		b.rot.GetRow(2, stransform.rotation[2]); // = b.rot[2];
	}
}

int AnimInfo::GetShapeBoneIndex(const string& shapeName, const string& boneName) {
	int b = -1;
	for (int i = 0; i < shapeBones[shapeName].size(); i++) {
		if (shapeBones[shapeName][i] == boneName) {
			b = i;
			break;
		}
	}

	return b;
}

void AnimInfo::GetWeights(const string& shape, const string& boneName, unordered_map<ushort, float>& outVertWeights) {
	int b = GetShapeBoneIndex(shape, boneName);
	if (b < 0)
		return;

	outVertWeights = shapeSkinning[shape].boneWeights[b].weights;
}

void AnimInfo::SetShapeBoneXForm(const string& shape, const string& boneName, skin_transform& stransform) {
	int b = GetShapeBoneIndex(shape, boneName);
	if (b < 0)
		return;

	shapeSkinning[shape].boneWeights[b].xform = stransform;
}

void AnimInfo::SetWeights(const string& shape, const string& boneName, unordered_map<ushort, float>& inVertWeights) {
	int bid = GetShapeBoneIndex(shape, boneName);
	if (bid == -1)
		return;

	shapeSkinning[shape].boneWeights[bid].weights.clear();
	shapeSkinning[shape].boneWeights[bid].weights = inVertWeights;

	if (refNif && refNif->IsValid()) {
		vector<vec3> verts;
		refNif->GetVertsForShape(shape, verts);

		vec3 a(FLT_MAX, FLT_MAX, FLT_MAX);
		vec3 b(-FLT_MAX, -FLT_MAX, -FLT_MAX);
		for (auto w : inVertWeights) {
			vec3 v = verts[w.first];
			a.x = min(a.x, v.x);
			a.y = min(a.y, v.y);
			a.z = min(a.z, v.z);
			b.x = max(b.x, v.x);
			b.y = max(b.y, v.y);
			b.z = max(b.z, v.z);
		}

		vec3 tot = (a + b) / 2.0f;
		float d = 0.0f;
		for (auto w : inVertWeights) {
			vec3 v = verts[w.first];
			d = max(d, tot.DistanceTo(v));
		}

		Mat4 mat(shapeSkinning[shape].boneWeights[bid].xform.ToMatrix());
		tot = mat * tot;
		shapeSkinning[shape].boneWeights[bid].bSphereOffset = tot;
		shapeSkinning[shape].boneWeights[bid].bSphereRadius = d;
	}
}

void AnimInfo::WriteToNif(NifFile* nif, bool synchBoneIDs) {
	if (synchBoneIDs) {
		for (auto bones : shapeBones) {
			vector<int> bids;
			for (auto bone : bones.second) {
				int id = nif->GetNodeID(bone);
				if (id == -1) {
					AnimBone boneRef;
					if (!AnimSkeleton::getInstance().GetBone(bone, boneRef))
						continue;
					if (boneRef.refCount == 0)
						continue;

					vector<vec3> r(3);
					boneRef.rot.GetRow(0, r[0]);
					boneRef.rot.GetRow(1, r[1]);
					boneRef.rot.GetRow(2, r[2]);
					boneRef.scale = 1.0f;				// Bone scaling is bad!
					id = nif->AddNode(boneRef.boneName, r, boneRef.trans, boneRef.scale);
				}
				bids.push_back(id);
			}
			nif->SetShapeBoneIDList(bones.first, bids);
		}
	}

	skin_transform xForm;
	for (auto shapeBoneList : shapeBones) {
		for (auto boneName : shapeBoneList.second) {
			if (!AnimSkeleton::getInstance().GetBoneTransform(boneName, xForm))
				continue;

			nif->SetNodeTransform(boneName, xForm);

			int bid = GetShapeBoneIndex(shapeBoneList.first, boneName);
			AnimWeight& bw = shapeSkinning[shapeBoneList.first].boneWeights[bid];
			if (AnimSkeleton::getInstance().GetSkinTransform(boneName, xForm)) {
				nif->SetShapeBoneTransform(shapeBoneList.first, bid, xForm, bw.bSphereOffset, bw.bSphereRadius);
				nif->SetShapeBoneWeights(shapeBoneList.first, bid, bw.weights);
			}
		}
	}
}

void AnimInfo::RenameShape(const string& shapeName, const string& newShapeName) {
	if (shapeSkinning.find(shapeName) != shapeSkinning.end()) {
		shapeSkinning[newShapeName] = move(shapeSkinning[shapeName]);
		shapeSkinning.erase(shapeName);
	}

	if (shapeBones.find(shapeName) != shapeBones.end()) {
		shapeBones[newShapeName] = move(shapeBones[shapeName]);
		shapeBones.erase(shapeName);
	}
}

AnimBone& AnimBone::LoadFromNif(NifFile* skeletonNif, int srcBlock, AnimBone* inParent)  {
	parent = inParent;
	isValidBone = false;
	NifBlockNiNode* node = dynamic_cast<NifBlockNiNode*>(skeletonNif->GetBlock(srcBlock));
	if (node)
		isValidBone = true;
	else
		return (*this);

	boneID = srcBlock;
	boneName = node->nodeName;
	order = -1;
	refCount = 0;

	vector<vec3> m33;
	m33.push_back(node->rotation[0]);
	m33.push_back(node->rotation[1]);
	m33.push_back(node->rotation[2]);
	localRot.Set(m33);
	localTrans = node->translation;
	scale = node->scale;

	if (parent) {
		trans = parent->trans + (parent->rot * localTrans);
		rot = parent->rot * localRot;
	}
	else {
		trans = localTrans;
		rot = localRot;
	}

	for (auto c : node->children) {
		string name = skeletonNif->NodeName(c);
		if (!name.empty()){
			if (name == "_unnamed_")
				name = AnimSkeleton::getInstance().GenerateBoneName();

			AnimBone& bone = AnimSkeleton::getInstance().AddBone(name).LoadFromNif(skeletonNif, c, this);
			children.push_back(&bone);
		}
	}
	return (*this);
}

int AnimSkeleton::LoadFromNif(const string& filename) {
	NifFile nif;
	nif.Load(filename);
	if (!nif.IsValid())
		return 1;

	rootBone = Config.GetCString("Anim/SkeletonRootName", "NPC");
	int nodeid = nif.GetNodeID(rootBone);
	if (nodeid == -1)
		return 2;

	if (isValid)
		allBones.clear();

	AddBone(rootBone).LoadFromNif(&nif, nodeid, NULL);
	isValid = true;
	return 0;
}

AnimBone& AnimSkeleton::AddBone(const string& boneName, bool bCustom) {
	if (!bCustom)
		return allBones[boneName];
	else if (allowCustom)
		return customBones[boneName];
	else
		return invBone;
}

string AnimSkeleton::GenerateBoneName() {
	stringstream ss;
	ss << "UnnamedBone_" << unknownCount++;

	return ss.str();
}
	
bool AnimSkeleton::RefBone(const string& boneName) {
	if (allBones.find(boneName) != allBones.end()) {
		allBones[boneName].refCount++;
		return true;
	}
	if (allowCustom && customBones.find(boneName) != customBones.end()) {
		customBones[boneName].refCount++;
		return true;
	}
	return false;
}
	
bool AnimSkeleton::ReleaseBone(const string& boneName) {
	if (allBones.find(boneName) != allBones.end()) {
		allBones[boneName].refCount--;
		return true;
	}
	if (allowCustom && customBones.find(boneName) != customBones.end()) {
		customBones[boneName].refCount--;
		return true;
	}
	return false;
}

AnimBone* AnimSkeleton::GetBonePtr(const string& boneName) {
	if (boneName.empty())
		return &allBones[rootBone];

	if (allBones.find(boneName) != allBones.end())
		return &allBones[boneName];

	if (allowCustom && customBones.find(boneName) != customBones.end())
		return &customBones[boneName];

	return NULL;
}

bool AnimSkeleton::GetBone(const string& boneName, AnimBone& outBone) {
	if (allBones.find(boneName) != allBones.end()) {
		outBone = allBones[boneName];
		return true;
	}
	if (allowCustom && customBones.find(boneName) != customBones.end()) {
		outBone = customBones[boneName];
		return true;
	}
	return false;
}

bool AnimSkeleton::GetBoneTransform(const string &boneName, skin_transform& xform) {
	if (allBones.find(boneName) == allBones.end())
		return false;

	AnimBone* cB = &allBones[boneName];
	Mat4 rot = cB->rot;
	rot.GetRow(0, xform.rotation[0]);
	rot.GetRow(1, xform.rotation[1]);
	rot.GetRow(2, xform.rotation[2]);
	xform.scale = 1.0f; //cB->scale					// Scale should be ignored?
	xform.translation = cB->trans;
	//xform.translation = cB->trans * -1.0f;
	//xform.translation = rot * xform.translation;
	return true;
}

bool AnimSkeleton::GetSkinTransform(const string &boneName, skin_transform& xform) {
	if (allBones.find(boneName) == allBones.end())
		return false;

	AnimBone* cB = &allBones[boneName];
	Mat4 rot = cB->rot.Inverse();
	rot.GetRow(0, xform.rotation[0]);
	rot.GetRow(1, xform.rotation[1]);
	rot.GetRow(2, xform.rotation[2]);
	xform.scale = 1.0f;	//cB->scale					// Scale should be ignored?
	xform.translation = cB->trans * -1.0f;
	xform.translation = rot * xform.translation;
	return true;
}

int AnimSkeleton::GetActiveBoneNames(vector<string>& outBoneNames) {
	int c = 0;
	for (auto ab : allBones) {
		if (ab.second.refCount > 0) {
			outBoneNames.push_back(ab.first);
			c++;
		}
	}
	return c;
}
