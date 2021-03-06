#include "OutfitProject.h"

OutfitProject::OutfitProject(ConfigurationManager& inConfig, OutfitStudio* inOwner) : appConfig(inConfig) {
	morpherInitialized = false;
	defaultTexFile = "res\\NoImg.png";
	owner = inOwner;
	string defSkelFile = Config.GetCString("Anim/DefaultSkeletonReference", "res\\skeleton_female.nif");
	LoadSkeletonReference(defSkelFile);

	mCopyRef = true;
	mGenWeights = true;
}

OutfitProject::~OutfitProject() {
}

string OutfitProject::Save(const string& strFileName,
	const string& strOutfitName,
	const string& strDataDir,
	const string& strBaseFile,
	const string& strGamePath,
	const string& strGameFile,
	bool genWeights,
	bool copyRef) {

	owner->UpdateProgress(1, "Checking destination locations");
	string errmsg = "";
	string outfitName = strOutfitName, baseFile = strBaseFile, gameFile = strGameFile;

	char chars[] = { '\\', '/', '?', ':', '*', '>', '<', '|', '"' };
	for (unsigned int i = 0; i < sizeof(chars); ++i) {
		replace(outfitName.begin(), outfitName.end(), chars[i], '_');
		replace(baseFile.begin(), baseFile.end(), chars[i], '_');
		replace(gameFile.begin(), gameFile.end(), chars[i], '_');
	}

	SliderSet outSet;
	outSet.SetName(outfitName);
	outSet.SetDataFolder(strDataDir);
	outSet.SetInputFile(baseFile);
	outSet.SetOutputPath(strGamePath);
	outSet.SetOutputFile(gameFile);
	outSet.SetGenWeights(genWeights);

	vector<string> refShapes;
	vector<string> outfitShapes;
	RefShapes(refShapes);
	OutfitShapes(outfitShapes);

	char curdir[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH, curdir);
	char folder[MAX_PATH];
	_snprintf_s(folder, MAX_PATH, MAX_PATH, "%s\\%s\\%s", curdir, "ShapeData", strDataDir.c_str());
	SHCreateDirectoryExA(NULL, folder, NULL);

	float prog = 5.0f;
	float step = 10.0f / (outfitShapes.size() + refShapes.size());
	owner->UpdateProgress(prog, "Checking destination locations");

	if (copyRef) {
		// Add all the reference shapes to the target list. Reference shapes also get reference data folders for their bsd files.
		for (auto rs : refShapes) {
			outSet.AddShapeTarget(rs, ShapeToTarget(rs));
			outSet.AddTargetDataFolder(ShapeToTarget(rs), activeSet.ShapeToDataFolder(rs));
			owner->UpdateProgress(prog += step, "");
		}
	}
	// Add all the outfit shapes to the target list. 
	vec3 offs;
	for (auto os : outfitShapes) {
		offs = workNif.GetShapeVirtualOffset(os);
		if (offs.x != 0.0f || offs.y != 0.0f || offs.z != 0.0f)
			outSet.AddTargetVirtualOffset(ShapeToTarget(os), offs);

		outSet.AddShapeTarget(os, ShapeToTarget(os));
		owner->UpdateProgress(prog += step, "");
	}

	// Copy the reference slider info and add the outfit data to them.
	int id;
	string targ;
	string targSlider;
	string targSliderFile;
	string saveDataPath = "ShapeData\\" + strDataDir;
	string saveFileName;

	prog = 10.0f;
	step = 50.0f / activeSet.size();
	owner->UpdateProgress(prog, "Calculating slider data");
	for (int i = 0; i < activeSet.size(); i++) {
		id = outSet.CopySlider(&activeSet[i]);
		outSet[id].ClearDataFiles();
		if (copyRef) {
			for (auto rs : refShapes) {
				targ = ShapeToTarget(rs);
				targSlider = activeSet[i].TargetDataName(targ);
				targSliderFile = activeSet[i].DataFileName(targSlider);
				if ((baseDiffData.GetDiffSet(targSlider) != NULL) && (baseDiffData.GetDiffSet(targSlider)->size() > 0)) {
					if (activeSet[i].IsLocalData(targSlider)) {
						outSet[id].AddDataFile(targ, targSlider, targSliderFile, true);
						saveFileName = saveDataPath + "\\" + targSliderFile;
						baseDiffData.SaveSet(targSlider, targ, saveFileName);
					}
					else
						outSet[id].AddDataFile(targ, targSlider, targSliderFile, false);
				}
			}
		}

		for (auto os : outfitShapes) {
			targ = ShapeToTarget(os);
			targSlider = targ + outSet[i].Name;
			targSliderFile = targSlider + ".bsd";
			if (morpher.GetResultDiffSize(os, activeSet[i].Name) > 0) {
				outSet[i].AddDataFile(targ, targSlider, targSliderFile);
				saveFileName = saveDataPath + "\\" + targSliderFile;
				morpher.SaveResultDiff(os, activeSet[i].Name, saveFileName);
			}
		}
		owner->UpdateProgress(prog += step, "");
	}
	prog = 60.0f;
	owner->UpdateProgress(prog, "Creating slider set file");

	string ssFileName = strFileName;
	if (ssFileName.find("SliderSets\\") == string::npos)
		ssFileName = "SliderSets\\" + ssFileName;

	SliderSetFile ssf(ssFileName);
	if (ssf.fail()) {
		ssf.New(ssFileName);
		if (ssf.fail()) {
			errmsg = "Failed to open or create slider set file: " + ssFileName;
			return errmsg;
		}
	}

	char ssNewFolder[MAX_PATH];
	auto it = strFileName.rfind('\\');
	if (it != string::npos) {
		_snprintf_s(ssNewFolder, MAX_PATH, MAX_PATH, "%s\\%s", curdir, strFileName.substr(0, it).c_str());
		SHCreateDirectoryExA(NULL, ssNewFolder, NULL);
	}
	else {
		_snprintf_s(ssNewFolder, MAX_PATH, MAX_PATH, "%s\\%s", curdir, "SliderSets");
		SHCreateDirectoryExA(NULL, ssNewFolder, NULL);
	}

	prog = 61.0f;
	owner->UpdateProgress(prog, "");
	ssf.UpdateSet(outSet);
	if (!ssf.Save()) {
		errmsg = "Failed to write to slider set file: " + ssFileName;
		return errmsg;
	}

	prog = 70.0f;
	owner->UpdateProgress(prog, "Saving Base Nif file");

	saveFileName = saveDataPath + "\\" + baseFile;

	if (workNif.IsValid()) {
		NifFile clone(workNif);
		clone.SetNodeName(0, "Scene Root");

		if (copyRef) {
			for (auto rs : refShapes)
				clone.CopyShape(rs, baseNif, rs);
			baseAnim.WriteToNif(&clone, false);
		}

		workAnim.WriteToNif(&clone);
		clone.GetShapeList(outfitShapes);

		for (auto s : outfitShapes)
			clone.UpdateSkinPartitions(s);

		if (clone.Save(saveFileName)) {
			errmsg = "Failed to write base .nif file: " + saveFileName;
			return errmsg;
		}
	}
	else if (baseNif.IsValid()) {
		NifFile clone(baseNif);
		clone.SetNodeName(0, "Scene Root");
		baseAnim.WriteToNif(&clone);

		for (auto rs : refShapes)
			clone.UpdateSkinPartitions(rs);

		if (clone.Save(saveFileName)) {
			errmsg = "Failed to write base .nif file: " + saveFileName;
			return errmsg;
		}
	}

	prog = 100.0f;
	owner->UpdateProgress(prog, "Complete");

	mFileName = ssFileName;
	mOutfitName = outfitName;
	mDataDir = strDataDir;
	mBaseFile = baseFile;
	mGamePath = strGamePath;
	mGameFile = gameFile;
	mCopyRef = copyRef;
	mGenWeights = genWeights;

	return errmsg;
}

string OutfitProject::SliderSetName() {
	return activeSet.GetName();
}

string OutfitProject::SliderSetFileName() {
	return activeSet.GetInputFileName();
}

string OutfitProject::OutfitName() {
	return outfitName;
}

bool OutfitProject::IsDirty() {
	return (shapeDirty.size() > 0);
}

void OutfitProject::Clean(const string& specificShape) {
	shapeDirty.erase(specificShape);
}

void  OutfitProject::SetDirty(const string& specificShape) {
	shapeDirty[specificShape] = true;
}

bool OutfitProject::IsDirty(const string& specificShape) {
	return (shapeDirty.find(specificShape) != shapeDirty.end());
}

bool OutfitProject::ValidSlider(int index) {
	if (index >= 0 && index < activeSet.size())
		return true;
	return false;
}

bool OutfitProject::ValidSlider(const string& sliderName) {
	return activeSet.SliderExists(sliderName);
}

bool OutfitProject::AllSlidersZero() {
	for (int i = 0; i < activeSet.size(); i++)
		if (activeSet[i].curValue != 0.0f)
			return false;
	return true;
}

int OutfitProject::SliderCount() {
	return activeSet.size();
}

void OutfitProject::GetSliderList(vector<string>& sliderNames) {
	for (int i = 0; i < activeSet.size(); i++)
		sliderNames.push_back(activeSet[i].Name);
}

const string& OutfitProject::SliderName(int index) {
	if (!ValidSlider(index))
		return emptyname;
	return activeSet[index].Name;
}

void OutfitProject::AddEmptySlider(const string& newName) {
	string sliderAbbrev = NameAbbreviate(newName);
	int sliderID = activeSet.CreateSlider(newName);
	activeSet[sliderID].bShow = true;

	vector<string> refShapes;
	RefShapes(refShapes);
	for (auto s : refShapes) {
		string target = ShapeToTarget(s);
		string shapeAbbrev = NameAbbreviate(s);
		string shapeSlider = target + sliderAbbrev;
		activeSet[sliderID].AddDataFile(target, shapeSlider, shapeSlider + ".bsd", true);
		activeSet.AddShapeTarget(s, target);
		baseDiffData.AddEmptySet(shapeSlider, target);
	}
}

void OutfitProject::AddZapSlider(const string& newName, unordered_map<ushort, float>& verts, const string& shapeName, bool bIsOutfit) {
	string sliderAbbrev = NameAbbreviate(newName);
	unordered_map<ushort, vec3> diffData;
	vec3 moveVec(0.0f, 1.0f, 0.0f);
	for (auto v : verts)
		diffData[v.first] = moveVec;

	string target = ShapeToTarget(shapeName);
	string shapeAbbrev = NameAbbreviate(shapeName);
	string shapeSlider = target + sliderAbbrev;

	int sliderID = activeSet.CreateSlider(newName);
	activeSet[sliderID].bZap = true;
	activeSet[sliderID].defBigValue = 0.0f;
	activeSet[sliderID].defSmallValue = 0.0f;

	if (bIsOutfit) {
		morpher.SetResultDiff(shapeName, newName, diffData);
	}
	else {
		activeSet[sliderID].AddDataFile(target, shapeSlider, shapeSlider + ".bsd", true);
		activeSet.AddShapeTarget(shapeName, target);
		baseDiffData.AddEmptySet(shapeSlider, target);
		for (auto i : diffData)
			baseDiffData.SumDiff(shapeSlider, target, i.first, i.second);
	}
}

void OutfitProject::AddCombinedSlider(const string& newName) {
	string sliderAbbrev = NameAbbreviate(newName);
	vector<vec3> verts;
	unordered_map<ushort, vec3> diffData;

	vector<string> outfitShapes;
	OutfitShapes(outfitShapes);
	for (auto os : outfitShapes) {
		diffData.clear();
		GetLiveOutfitVerts(os, verts);
		workNif.CalcShapeDiff(os, &verts, diffData);
		morpher.SetResultDiff(os, newName, diffData);
	}

	int sliderID = activeSet.CreateSlider(newName);
	vector<string> refShapes;
	RefShapes(refShapes);
	for (auto rs : refShapes) {
		string target = ShapeToTarget(rs);
		string shapeSlider = target + sliderAbbrev;
		activeSet[sliderID].AddDataFile(target, shapeSlider, shapeSlider + ".bsd", true);
		baseDiffData.AddEmptySet(shapeSlider, target);
		GetLiveRefVerts(rs, verts);
		baseNif.CalcShapeDiff(rs, &verts, diffData);
		for (auto i : diffData)
			baseDiffData.SumDiff(shapeSlider, target, i.first, i.second);
	}
}

int OutfitProject::AddShapeFromObjFile(const string& fileName, const string& shapeName, const string& mergeShape) {
	ObjFile obj;
	obj.SetScale(vec3(10.0f, 10.0f, 10.0f));
	if (obj.LoadForNif(fileName))
		return 1;

	vector<vec3> v;
	vector<tri> t;
	vector<vec2> uv;
	if (!obj.CopyDataForIndex(0, &v, &t, &uv))
		return 3;

	string useShapeName = shapeName;

	if (mergeShape != "") {
		vector<vec3> shapeVerts;
		workNif.GetVertsForShape(mergeShape, shapeVerts);
		if (shapeVerts.size() == v.size()) {
			int r = wxMessageBox("The vertex count of the selected .obj file matches the currently selected outfit shape.  Do you wish to update the current shape?  (click No to create a new shape)", "Merge or New", wxYES_NO | wxICON_QUESTION);
			if (r == wxYES) {
				workNif.SetVertsForShape(mergeShape, v);
				return 101;
			}
		}
		useShapeName = wxGetTextFromUser("Please specify a name for the new shape", "New Shape Name");
		if (useShapeName == "")
			return 100;
	}

	NifFile blank;
	blank.Load("res\\SkeletonBlank.nif");
	if (!blank.IsValid())
		return 2;

	NifBlockTriShapeData* nifShapeData = new NifBlockTriShapeData();
	nifShapeData->Create(&v, &t, &uv);
	int shapeID = blank.AddBlock((NifBlock*)nifShapeData, "NiTriShapeData");

	NifBlockNiSkinData* nifSkinData = new NifBlockNiSkinData();
	int skinID = blank.AddBlock((NifBlock*)nifSkinData, "NiSkinData");

	NifBlockNiSkinPartition* nifSkinPartition = new NifBlockNiSkinPartition();
	int partID = blank.AddBlock((NifBlock*)nifSkinPartition, "NiSkinPartition");

	NifBlockBSDismemberment* nifDismemberInst = new NifBlockBSDismemberment();
	int dismemberID = blank.AddBlock((NifBlock*)nifDismemberInst, "BSDismemberSkinInstance");

	NifBlockBSLightShadeProp* nifShader = new NifBlockBSLightShadeProp();
	int shaderID = blank.AddBlock((NifBlock*)nifShader, "BSLightingShaderProperty");

	NifBlockBSShaderTextureSet* nifTexset = new NifBlockBSShaderTextureSet();
	int texsetID = blank.AddBlock((NifBlock*)nifTexset, "BSShaderTextureSet");
	nifShader->texsetRef = texsetID;

	NifBlockTriShape* nifTriShape = new NifBlockTriShape();
	int triShapeID = blank.AddBlock((NifBlock*)nifTriShape, "NiTriShape");
	nifDismemberInst->dataRef = skinID;
	nifDismemberInst->skinRef = partID;
	nifDismemberInst->skeletonRoot = 0;
	nifTriShape->propertiesRef1 = shaderID;

	int nameID = blank.AddOrFindStringId(useShapeName);
	nifTriShape->dataRef = shapeID;
	nifTriShape->skinRef = dismemberID;
	nifTriShape->nameID = nameID;
	nifTriShape->shapeName = useShapeName;

	if (!workNif.IsValid())
		LoadOutfit("res\\SkeletonBlank.nif", "New Outfit");

	workNif.CopyShape(useShapeName, blank, useShapeName);
	SetOutfitTexture(useShapeName, "_AUTO_");

	return 0;
}

string OutfitProject::SliderShapeDataName(int index, const string& shapeName) {
	if (!ValidSlider(index))
		return emptyname;
	return activeSet.ShapeToDataName(index, shapeName);
}

bool OutfitProject::SliderClamp(int index) {
	if (!ValidSlider(index))
		return false;
	return activeSet[index].bClamp;
}

bool OutfitProject::SliderZap(int index) {
	if (!ValidSlider(index))
		return false;
	return activeSet[index].bZap;
}

bool OutfitProject::SliderInvert(int index) {
	if (!ValidSlider(index))
		return false;
	return activeSet[index].bInvert;
}

bool OutfitProject::SliderHidden(int index) {
	if (!ValidSlider(index))
		return false;
	return activeSet[index].bHidden;
}

void  OutfitProject::SetSliderZap(int index, bool zap) {
	if (!ValidSlider(index))
		return;
	activeSet[index].bZap = zap;
}

void  OutfitProject::SetSliderInvert(int index, bool inv) {
	if (!ValidSlider(index))
		return;
	activeSet[index].bInvert = inv;
}

void  OutfitProject::SetSliderHidden(int index, bool hidden) {
	if (!ValidSlider(index))
		return;
	activeSet[index].bHidden = hidden;
}

void  OutfitProject::SetSliderDefault(int index, int val, bool isHi) {
	if (!ValidSlider(index))
		return;

	if (!isHi)
		activeSet[index].defSmallValue = val;
	else
		activeSet[index].defBigValue = val;
}

void OutfitProject::SetSliderName(int index, const string& newName) {
	if (!ValidSlider(index))
		return;

	string oldName = activeSet[index].Name;
	vector<string> outfitShapes;
	OutfitShapes(outfitShapes);
	for (auto s : outfitShapes)
		morpher.RenameResultDiffData(s, oldName, newName);

	activeSet[index].Name = newName;
}

float& OutfitProject::SliderValue(int index) {
	return activeSet[index].curValue;
}

float& OutfitProject::SliderValue(const string& name) {
	return activeSet[name].curValue;
}

float OutfitProject::SliderDefault(int index, bool hi) {
	if (hi)
		return activeSet[index].defBigValue;

	return activeSet[index].defSmallValue;
}

bool& OutfitProject::SliderShow(int index) {
	return activeSet[index].bShow;
}

bool& OutfitProject::SliderShow(const string& sliderName) {
	return activeSet[sliderName].bShow;
}

int OutfitProject::SliderIndexFromName(const string& sliderName) {
	for (int i = 0; i < activeSet.size(); i++)
		if (activeSet[i].Name == sliderName)
			return i;

	return -1;
}

void OutfitProject::NegateSlider(const string& sliderName, const string& shapeName, bool bIsOutfit) {
	string target = ShapeToTarget(shapeName);
	if (!bIsOutfit) {
		string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.ScaleDiff(sliderData, target, -1.0f);
	}
	else
		morpher.ScaleResultDiff(target, sliderName, -1.0f); // SaveResultDiff(target, sliderName, fileName);
}

void OutfitProject::SaveSliderBSD(const string& sliderName, const string& shapeName, const string& fileName, bool bIsOutfit) {
	string target = ShapeToTarget(shapeName);
	if (!bIsOutfit) {
		string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.SaveSet(sliderData, target, fileName);
	}
	else
		morpher.SaveResultDiff(target, sliderName, fileName);
}

void OutfitProject::SaveSliderBSDToDir(const string& sliderName, const string& shapeName, const string& dir, bool bIsOutfit) {
	string target = ShapeToTarget(shapeName);
	if (!bIsOutfit) {
		string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.SaveSet(sliderData, target, dir + "\\" + shapeName + "_" + sliderName + ".bsd");
	}
	else
		morpher.SaveResultDiff(target, sliderName, dir + "\\" + shapeName + "_" + sliderName + ".bsd");
}

void OutfitProject::SetSliderFromBSD(const string& sliderName, const string& shapeName, const string& fileName, bool bIsOutfit) {
	string target = ShapeToTarget(shapeName);
	if (!bIsOutfit) {
		string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.LoadSet(sliderData, target, fileName);
	}
	else {
		DiffDataSets tmpSet;
		tmpSet.LoadSet(sliderName, target, fileName);
		unordered_map<ushort, vector3>* diff = tmpSet.GetDiffSet(sliderName);
		morpher.SetResultDiff(target, sliderName, (*diff));
	}
}

bool OutfitProject::SetSliderFromOBJ(const string& sliderName, const string& shapeName, const string& fileName, bool bIsOutfit) {
	string target = ShapeToTarget(shapeName);

	ObjFile obj;
	obj.LoadForNif(fileName);
	vector<string> groupNames;
	obj.GetGroupList(groupNames);

	int i = 0;
	int index = 0;
	for (auto n : groupNames) {
		if (n == shapeName) {
			index = i;
			break;
		}
		i++;
	}

	vector<vec3> objVerts;
	obj.CopyDataForIndex(index, &objVerts, NULL, NULL);

	unordered_map<ushort, vector3> diff;
	if (!bIsOutfit) {
		if (baseNif.CalcShapeDiff(shapeName, &objVerts, diff, 10.0f))
			return false;

		string sliderData = activeSet[sliderName].TargetDataName(target);
		baseDiffData.LoadSet(sliderData, target, diff);
	}
	else {
		if (workNif.CalcShapeDiff(shapeName, &objVerts, diff, 10.0f))
			return false;

		morpher.SetResultDiff(target, sliderName, diff);
	}
	return true;
}

void OutfitProject::GetLiveVerts(const string& shapeName, vector<vec3>& outVerts, bool bIsOutfit) {
	if (bIsOutfit)
		GetLiveOutfitVerts(shapeName, outVerts);
	else
		GetLiveRefVerts(shapeName, outVerts);
}

void OutfitProject::GetLiveRefVerts(const string& shapeName, vector<vec3>& outVerts) {
	string target = ShapeToTarget(shapeName);
	string targetData;
	baseNif.GetVertsForShape(shapeName, outVerts);
	for (int i = 0; i < activeSet.size(); i++) {
		if (activeSet[i].bShow && activeSet[i].curValue != 0.0f) {
			targetData = activeSet.ShapeToDataName(i, shapeName);
			if (targetData == "")
				continue;

			baseDiffData.ApplyDiff(targetData, target, activeSet[i].curValue, &outVerts);
		}
	}
}

void OutfitProject::GetLiveOutfitVerts(const string& shapeName, vector<vec3>& outVerts) {
	string target = ShapeToTarget(shapeName);
	workNif.GetVertsForShape(shapeName, outVerts);
	for (int i = 0; i < activeSet.size(); i++)
		if (activeSet[i].bShow && activeSet[i].curValue != 0.0f)
			morpher.ApplyResultToVerts(activeSet[i].Name, target, &outVerts, activeSet[i].curValue);
}

const string& OutfitProject::ShapeToTarget(const string& shapeName) {
	for (auto it = activeSet.TargetShapesBegin(); it != activeSet.TargetShapesEnd(); ++it)
		if (it->second == shapeName)
			return it->first;

	return shapeName;
}

void OutfitProject::RefShapes(vector<string>& outShapeNames) {
	outShapeNames.push_back(baseShapeName);
	//baseNif.GetShapeList(outShapeNames);
}

void OutfitProject::OutfitShapes(vector<string>& outShapeNames) {
	workNif.GetShapeList(outShapeNames);
}

void OutfitProject::RefBones(vector<string>& outBoneNames) {
	AnimSkeleton::getInstance().GetActiveBoneNames(outBoneNames);
}

void OutfitProject::OutfitBones(vector<string>& outBoneNames) {
	set<string> boneSet;

	vector<string> outfitShapes;
	OutfitShapes(outfitShapes);
	for (auto s : outfitShapes) {
		vector<string> curBones;
		workNif.GetShapeBoneList(s, curBones);
		boneSet.insert(curBones.begin(), curBones.end());
	}

	outBoneNames.assign(boneSet.begin(), boneSet.end());
}

string OutfitProject::OutfitTexture(const string& shapeName) {
	if (outfitTextures.find(shapeName) != outfitTextures.end())
		return outfitTextures[shapeName];
	else
		return defaultTexFile;
}

string OutfitProject::RefTexture(const string& shapeName) {
	if (baseTextures.find(shapeName) != baseTextures.end())
		return baseTextures[shapeName];
	else
		return defaultTexFile;
}

void OutfitProject::SetOutfitTexturesDefault(const string& defaultChoice) {
	if (defaultChoice == "Grid, white background")
		SetOutfitTextures("res\\whitegrid.png");
	else if (defaultChoice == "Grid, light grey background")
		SetOutfitTextures("res\\greygrid.png");
	else if (defaultChoice == "Grid, dark grey background")
		SetOutfitTextures("res\\greygrid_inv.png");
	else
		SetOutfitTextures(defaultTexFile);
}

void  OutfitProject::SetOutfitTextures(const string& textureFile) {
	vector<string> outfitShapes;
	OutfitShapes(outfitShapes);
	for (auto s : outfitShapes)
		SetOutfitTexture(s, textureFile);
}

void OutfitProject::SetOutfitTexture(const string& shapeName, const string& textureFile) {
	if (textureFile == "_AUTO_") {
		string nifTexFile;
		workNif.GetTextureForShape((string)shapeName, nifTexFile);
		if (nifTexFile.empty())
			nifTexFile = "noimg.dds";

		string texturesDir = appConfig["GameDataPath"] + "textures\\";
		string combinedTexFile = texturesDir + nifTexFile;
		if (GetFileAttributesA(combinedTexFile.c_str()) == INVALID_FILE_ATTRIBUTES)
			outfitTextures[shapeName] = defaultTexFile;
		else
			outfitTextures[shapeName] = combinedTexFile;
	}
	else
		outfitTextures[shapeName] = textureFile;
}

void OutfitProject::SetRefTexture(const string& shapeName, const string& textureFile) {
	if (textureFile == "_AUTO_") {
		string nifTexFile;
		baseNif.GetTextureForShape((string)shapeName, nifTexFile);
		if (nifTexFile.empty())
			nifTexFile = "noimg.dds";

		string texturesDir = appConfig["GameDataPath"] + "textures\\";
		string combinedTexFile = texturesDir + nifTexFile;
		if (GetFileAttributesA(combinedTexFile.c_str()) == INVALID_FILE_ATTRIBUTES)
			baseTextures[shapeName] = defaultTexFile;
		else
			baseTextures[shapeName] = combinedTexFile;
	}
	else
		baseTextures[shapeName] = textureFile;
}

bool OutfitProject::IsValidShape(const string& shapeName) {
	vector<string> outfitShapes;
	OutfitShapes(outfitShapes);
	for (auto s : outfitShapes)
		if (s == shapeName)
			return true;

	if (baseShapeName == shapeName)
		return true;

	return false;
}

void OutfitProject::RefreshMorphOutfitShape(const string& shapeName, bool bIsOutfit) {
	if (bIsOutfit)
		morpher.UpdateMeshFromNif(workNif, shapeName);
	else
		morpher.UpdateMeshFromNif(baseNif, shapeName);
}

void OutfitProject::UpdateShapeFromMesh(const string& shapeName, const mesh* m, bool IsOutfit) {
	vector<vec3> liveVerts;
	for (int i = 0; i < m->nVerts; i++)
		liveVerts.emplace_back(move(vec3(m->verts[i].x * -10, m->verts[i].z * 10, m->verts[i].y * 10)));
	if (IsOutfit)
		workNif.SetVertsForShape(shapeName, liveVerts);
	else
		baseNif.SetVertsForShape(shapeName, liveVerts);
	Clean(shapeName);
}

void OutfitProject::UpdateMorphResult(const string& shapeName, const string& sliderName, unordered_map<ushort, vector3>& vertUpdates, bool IsOutfit) {
	// Morph results are stored in two different places depending on whether it's an outfit or the base shape.
	// The outfit morphs are stored in the automorpher, whereas the base shape diff info is stored in directly in basediffdata.
	if (IsOutfit) {
		morpher.UpdateResultDiff(shapeName, sliderName, vertUpdates);
	}
	else {
		string target = ShapeToTarget(shapeName);
		string dataName = activeSet[sliderName].TargetDataName(target);

		for (auto i : vertUpdates) {
			vec3 diffscale = vec3(i.second.x * -10, i.second.z * 10, i.second.y * 10);
			baseDiffData.SumDiff(dataName, target, i.first, diffscale);
			activeSet[sliderName].SetLocalData(dataName);
		}
	}
}

void OutfitProject::MoveVertex(const string& shapeName, vec3& pos, int& id, bool IsOutfit) {
	if (IsOutfit)
		workNif.MoveVertex(shapeName, pos, id);
	else
		baseNif.MoveVertex(shapeName, pos, id);
}

void OutfitProject::OffsetShape(const string& shapeName, vec3& xlate, bool IsOutfit, unordered_map<ushort, float>* mask) {
	if (IsOutfit)
		workNif.OffsetShape(shapeName, xlate, mask);
	else
		baseNif.OffsetShape(shapeName, xlate, mask);
}

void OutfitProject::ScaleShape(const string& shapeName, float& scale, bool IsOutfit, unordered_map<ushort, float>* mask) {
	if (IsOutfit)
		workNif.ScaleShape(shapeName, scale, mask);
	else
		baseNif.ScaleShape(shapeName, scale, mask);
}

void OutfitProject::RotateShape(const string& shapeName, vec3& angle, bool IsOutfit, unordered_map<ushort, float>* mask) {
	if (IsOutfit)
		workNif.RotateShape(shapeName, angle, mask);
	else
		baseNif.RotateShape(shapeName, angle, mask);
}

void OutfitProject::CopyBoneWeights(const string& destShape, unordered_map<ushort, float>* mask, vector<string>* inBoneList) {
	if (!baseNif.IsValid())
		return;

	vector<string> lboneList;
	vector<string>* boneList;

	owner->UpdateProgress(1, "Gathering bones");

	if (inBoneList == NULL) {
		for (auto bn : baseAnim.shapeBones[baseShapeName])
			lboneList.push_back(bn);

		boneList = &lboneList;
	}
	else
		boneList = inBoneList;

	DiffDataSets dds;
	unordered_map<ushort, float> weights;
	for (auto bone : (*boneList)) {
		weights.clear();
		dds.AddEmptySet(bone + "_WT_", "Weight");
		baseAnim.GetWeights(baseShapeName, bone, weights);
		for (auto w : weights) {
			vec3 tmp;
			tmp.y = w.second;
			dds.UpdateDiff(bone + "_WT_", "Weight", w.first, tmp);
		}
	}

	owner->UpdateProgress(10, "Initializing proximity data");

	InitConform();
	morpher.LinkRefDiffData(&dds);
	morpher.BuildProximityCache(destShape);

	float step = 50.0f / boneList->size();
	float prog = 40.0f;
	owner->UpdateProgress(prog, "Copying bone weights");

	for (auto bone : (*boneList)) {
		string wtSet = bone + "_WT_";
		morpher.GenerateResultDiff(destShape, wtSet, wtSet);

		unordered_map<ushort, vec3> diffResult;
		morpher.GetRawResultDiff(destShape, wtSet, diffResult);

		unordered_map<ushort, float> oldWeights;
		if (mask) {
			weights.clear();
			oldWeights.clear();
			workAnim.GetWeights(destShape, bone, oldWeights);
		}

		for (auto dr : diffResult) {
			if (mask) {
				if (1.0f - (*mask)[dr.first] > 0.0f)
					weights[dr.first] = dr.second.y * (1.0f - (*mask)[dr.first]);
				else
					weights[dr.first] = oldWeights[dr.first];
			}
			else
				weights[dr.first] = dr.second.y;
		}

		if (diffResult.size() > 0) {
			AnimBone boneRef;
			AnimSkeleton::getInstance().GetBone(bone, boneRef);
			if (workAnim.AddShapeBone(destShape, boneRef)) {
				skin_transform xForm;
				baseAnim.GetBoneXForm(bone, xForm);
				workAnim.SetShapeBoneXForm(destShape, bone, xForm);
			}
		}

		workAnim.SetWeights(destShape, bone, weights);
		owner->UpdateProgress(prog += step, "");
	}

	morpher.UnlinkRefDiffData();
	owner->UpdateProgress(100, "Finished");
}

void OutfitProject::TransferSelectedWeights(const string& destShape, unordered_map<ushort, float>* mask, vector<string>* inBoneList) {
	if (!baseNif.IsValid())
		return;

	owner->UpdateProgress(10, "Gathering bones");

	vector<string>* boneList;
	if (inBoneList == NULL) {
		vector<string> allBoneList;
		for (auto boneName : baseAnim.shapeBones[baseShapeName])
			allBoneList.push_back(boneName);

		boneList = &allBoneList;
	}
	else
		boneList = inBoneList;

	float step = 50.0f / boneList->size();
	float prog = 40.0f;
	owner->UpdateProgress(prog, "Transferring bone weights");

	unordered_map<ushort, float> weights;
	unordered_map<ushort, float> oldWeights;
	for (auto boneName : (*boneList)) {
		weights.clear();
		oldWeights.clear();
		baseAnim.GetWeights(baseShapeName, boneName, weights);
		workAnim.GetWeights(destShape, boneName, oldWeights);

		for (auto w : weights) {
			if (mask) {
				if (1.0f - (*mask)[w.first] > 0.0f)
					weights[w.first] = w.second * (1.0f - (*mask)[w.first]);
				else
					weights[w.first] = oldWeights[w.first];
			}
			else
				weights[w.first] = w.second;
		}

		AnimBone boneRef;
		AnimSkeleton::getInstance().GetBone(boneName, boneRef);
		if (workAnim.AddShapeBone(destShape, boneRef)) {
			skin_transform xForm;
			baseAnim.GetBoneXForm(boneName, xForm);
			workAnim.SetShapeBoneXForm(destShape, boneName, xForm);
		}

		workAnim.SetWeights(destShape, boneName, weights);
		owner->UpdateProgress(prog += step, "");
	}

	owner->UpdateProgress(100, "Finished");
}

bool OutfitProject::OutfitHasUnweighted() {
	vector<string> outfitShapes;
	OutfitShapes(outfitShapes);
	for (auto s : outfitShapes) {
		vector<string> bones;
		vector<vec3> verts;
		RefBones(bones);
		workNif.GetVertsForShape(s, verts);

		unordered_map<int, int> influences;
		for (int i = 0; i < verts.size(); i++)
			influences.emplace(i, 0);

		unordered_map<ushort, float> boneWeights;
		for (auto b : bones) {
			boneWeights.clear();
			workAnim.GetWeights(s, b, boneWeights);
			for (int i = 0; i < verts.size(); i++) {
				auto id = boneWeights.find(i);
				if (id != boneWeights.end())
					influences.at(i)++;
			}
		}

		mesh* m = owner->glView->GetMesh(s);
		bool unweighted = false;
		for (auto i : influences) {
			if (i.second == 0) {
				if (!unweighted)
					m->ColorFill(vec3(0.0f, 0.0f, 0.0f));
				m->vcolors[i.first].x = 1.0f;
				unweighted = true;
			}
		}
		if (unweighted)
			return true;
	}
	return false;
}

void OutfitProject::ApplyBoneScale(const string& bone, int sliderPos) {
	vector<string> bones;
	vector<vec3> boneRot;
	vec3 boneTranslation;
	float boneScale;
	unordered_map<ushort, float> weights;

	vector<vec3> verts;
	vector<string> shapes;
	ClearBoneScale();

	OutfitShapes(shapes);
	for (auto s : shapes) {
		workNif.GetVertsForShape(s, verts);
		workNif.GetShapeBoneList(s, bones);
		boneScaleOffsets.emplace(s, vector<vec3>(verts.size()));
		for (auto b : bones) {
			if (b == bone) {
				weights.clear();
				workNif.GetNodeTransform(b, boneRot, boneTranslation, boneScale);
				workAnim.GetWeights(s, b, weights);
				for (auto w : weights) {
					vec3 dir = verts[w.first] - boneTranslation;
					dir.Normalize();
					vec3 offset = dir * w.second * sliderPos / 5.0f;
					verts[w.first] += offset;
					boneScaleOffsets[s][w.first] += offset;
				}
			}
		}
		workNif.SetVertsForShape(s, verts);
		owner->glView->UpdateMeshVertices(s, &verts);
	}

	shapes.clear();
	RefShapes(shapes);
	for (auto s : shapes) {
		baseNif.GetVertsForShape(s, verts);
		baseNif.GetShapeBoneList(s, bones);
		boneScaleOffsets.emplace(s, vector<vec3>(verts.size()));
		for (auto b : bones) {
			if (b == bone) {
				weights.clear();
				baseNif.GetNodeTransform(b, boneRot, boneTranslation, boneScale);
				baseAnim.GetWeights(s, b, weights);
				for (auto w : weights) {
					vec3 dir = verts[w.first] - boneTranslation;
					dir.Normalize();
					vec3 offset = dir * w.second * sliderPos / 5.0f;
					verts[w.first] += offset;
					boneScaleOffsets[s][w.first] += offset;
				}
			}
		}
		baseNif.SetVertsForShape(s, verts);
		owner->glView->UpdateMeshVertices(s, &verts);
	}
}

void OutfitProject::ClearBoneScale() {
	vector<vec3> verts;
	vector<string> shapes;

	OutfitShapes(shapes);
	for (auto s : shapes) {
		if (boneScaleOffsets.find(s) != boneScaleOffsets.end()) {
			workNif.GetVertsForShape(s, verts);
			if (verts.size() == boneScaleOffsets[s].size()) {
				for (int i = 0; i < verts.size(); i++)
					verts[i] -= boneScaleOffsets[s][i];
				workNif.SetVertsForShape(s, verts);
				owner->glView->UpdateMeshVertices(s, &verts);
			}
		}
	}

	shapes.clear();
	RefShapes(shapes);
	for (auto s : shapes) {
		if (boneScaleOffsets.find(s) != boneScaleOffsets.end()) {
			baseNif.GetVertsForShape(s, verts);
			if (verts.size() == boneScaleOffsets[s].size()) {
				for (int i = 0; i < verts.size(); i++)
					verts[i] -= boneScaleOffsets[s][i];
				baseNif.SetVertsForShape(s, verts);
				owner->glView->UpdateMeshVertices(s, &verts);
			}
		}
	}

	boneScaleOffsets.clear();
}

void OutfitProject::AddBoneRef(const string& boneName, bool IsOutfit) {
	AnimBone boneRef;
	AnimSkeleton::getInstance().GetBone(boneName, boneRef);
	if (IsOutfit) {
		vector<string> shapes;
		OutfitShapes(shapes);
		for (auto s : shapes) {
			if (workAnim.AddShapeBone(s, boneRef)) {
				skin_transform xForm;
				baseAnim.GetBoneXForm(boneName, xForm);
				workAnim.SetShapeBoneXForm(s, boneName, xForm);
			}
		}
	}
}

void OutfitProject::BuildShapeSkinPartions(const string& destShape, bool IsOutfit) {
	if (IsOutfit) {
		workAnim.WriteToNif(&workNif);
		workNif.BuildSkinPartitions(destShape);
	}
	else {
		baseAnim.WriteToNif(&baseNif);
		baseNif.BuildSkinPartitions(destShape);
	}
}

void OutfitProject::ClearWorkSliders() {
	morpher.ClearResultDiff();
}

void OutfitProject::ClearReference() {
	if (baseNif.IsValid()) {
		baseAnim.Clear();
		baseNif.Clear();
	}
	if (activeSet.size() > 0)
		activeSet.Clear();

	morpher.UnlinkRefDiffData();

	baseShapeName = "";
}

void OutfitProject::ClearOutfit() {
	if (workNif.IsValid()) {
		workAnim.Clear();
		workNif.Clear();
	}
	ClearWorkSliders();
}

void OutfitProject::ClearSlider(const string& shapeName, const string& sliderName, bool isOutfit) {
	string target = ShapeToTarget(shapeName);
	if (!isOutfit) {
		string data = activeSet[sliderName].TargetDataName(target);
		baseDiffData.EmptySet(data, target);
	}
	else
		morpher.EmptyResultDiff(target, sliderName);
}

void OutfitProject::ClearUnmaskedDiff(const string& shapeName, const string& sliderName, unordered_map<ushort, float>* mask, bool isOutfit) {
	string target = ShapeToTarget(shapeName);
	if (!isOutfit) {
		string data = activeSet[sliderName].TargetDataName(target);
		baseDiffData.ZeroVertDiff(data, target, NULL, mask);
	}
	else
		morpher.ZeroVertDiff(target, sliderName, NULL, mask);
}

void OutfitProject::DeleteSlider(const string& sliderName) {
	baseDiffData.ClearSet(sliderName);
	morpher.ClearResultSet(sliderName);
	activeSet.DeleteSlider(sliderName);
}

int OutfitProject::LoadSkeletonReference(const string& skeletonFileName) {
	return AnimSkeleton::getInstance().LoadFromNif(skeletonFileName);
}

int OutfitProject::LoadReferenceTemplate(const string& templateName, bool ClearRef) {
	vector<string> templateNames;
	vector<string> templateFiles;
	vector<string> templateSets;
	Config.GetValueArray("ReferenceTemplates", "Template", templateNames);
	Config.GetValueAttributeArray("ReferenceTemplates", "Template", "sourcefile", templateFiles);
	Config.GetValueAttributeArray("ReferenceTemplates", "Template", "set", templateSets);

	string srcfile;
	string srcset;

	for (int i = 0; i < templateNames.size(); i++) {
		if (templateNames[i] == templateName) {
			srcfile = templateFiles[i];
			srcset = templateSets[i];
		}
	}
	if (srcfile.empty()) {
		wxMessageBox("Template source file entry is invalid.", "Error");
		return 1;
	}

	return LoadReference(srcfile, srcset, ClearRef);
}

int OutfitProject::LoadReferenceNif(const string& fileName, const string& shapeName, bool ClearRef) {
	if (ClearRef)
		ClearReference();

	if (baseNif.Load(fileName))
		return 2;

	baseShapeName = shapeName;

	vector<string> baseShapes;
	vector<string> workShapes;
	RefShapes(baseShapes);
	OutfitShapes(workShapes);
	for (auto s1 : workShapes) {
		for (auto s2 : baseShapes) {
			if (s1 == s2) {
				RenameShape(s1, s1 + "_ref", false);
				baseShapeName = s1 + "_ref";
			}
		}
	}

	// Clear shape and root transforms
	vec3 trans, rootTrans;
	float scale, rootScale;
	baseNif.GetShapeTranslation(baseShapeName, trans);
	baseNif.GetShapeScale(baseShapeName, scale);
	baseNif.GetRootTranslation(rootTrans);
	baseNif.GetRootScale(rootScale);
	trans -= rootTrans; // Get shape translation includes the root translation

	if (!rootTrans.IsZero()) {
		baseNif.OffsetShape(baseShapeName, rootTrans);
		baseNif.SetRootTranslation(vec3(0.0f, 0.0f, 0.0f));
	}
	if (rootScale != 1.0f) {
		baseNif.ScaleShape(baseShapeName, rootScale);
		baseNif.SetRootScale(1.0f);
	}

	if (!trans.IsZero()) {
		baseNif.OffsetShape(baseShapeName, trans);
		baseNif.SetShapeTranslation(baseShapeName, vec3(0.0f, 0.0f, 0.0f));
	}
	if (scale != 1.0f) {
		baseNif.ScaleShape(baseShapeName, scale);
		baseNif.SetShapeScale(baseShapeName, 1.0f);
	}

	baseAnim.LoadFromNif(&baseNif);
	activeSet.LoadSetDiffData(baseDiffData);
	//activeSet.LinkShapeTarget(shapeName, shapeName);

	AutoOffset(false);

	return 0;
}

int OutfitProject::LoadReference(const string& filename, const string& setName, bool ClearRef, const string& shapeName) {
	if (ClearRef)
		ClearReference();

	string oldTarget;
	SliderSetFile sset(filename);
	int oldVertCount = -1;
	int newVertCount;

	if (sset.fail()) {
		wxMessageBox("Could not load slider set XML file.", "Load Reference", 5L | wxICON_ERROR);
		return 1;
	}

	sset.GetSet(setName, activeSet);
	activeSet.SetBaseDataPath("ShapeData");
	string inMeshFile = activeSet.GetInputFileName();

	if (!ClearRef)
		oldVertCount = baseNif.GetVertCountForShape(baseShapeName);

	if (baseNif.Load(inMeshFile)) {
		ClearReference();
		wxMessageBox("Could not load reference NIF.", "Load Reference", 5L | wxICON_ERROR);
		return 2;
	}

	vector<string> shapes;
	baseNif.GetShapeList(shapes);
	if (shapes.size() == 0) {
		ClearReference();
		wxMessageBox("Reference NIF does not contain any shapes.", "Load Reference", 5L | wxICON_ERROR);
		return 3;
	}

	string shape = shapeName;
	if (shape == "") {
		shape = shapes[0];
	}

	vector<string> workShapes;
	OutfitShapes(workShapes);
	for (auto s1 : workShapes) {
		for (auto s2 : shapes) {
			if (s1 == s2) {
				RenameShape(s1, s1 + "_ref", false);
				shape = s1 + "_ref";
			}
		}
	}

	if (!ClearRef) {
		SliderSet tmpSS;
		sset.GetSet(setName, tmpSS);
		for (int i = 0; i < tmpSS.size(); i++){
			activeSet.DeleteSlider(tmpSS[i].Name);
		}
		oldTarget = ShapeToTarget(baseShapeName);
		activeSet.RenameShape(baseShapeName, shape);
		activeSet.ClearTargets(oldTarget);
	}

	newVertCount = baseNif.GetVertCountForShape(shape);
	if (newVertCount == -1) {
		ClearReference();
		wxMessageBox("Shape not found in the reference NIF.", "Load Reference", 5L | wxICON_ERROR);
		return 4;
	}

	if (oldVertCount > 0 && oldVertCount == newVertCount) {
		string newTarget = ShapeToTarget(shape);
		if (!oldTarget.empty() && newTarget != oldTarget)
			activeSet.Retarget(oldTarget, newTarget);
	}

	baseShapeName = shape;

	// Clear shape and root transforms
	vec3 trans, rootTrans;
	float scale, rootScale;
	baseNif.GetRootTranslation(rootTrans);
	baseNif.GetRootScale(rootScale);
	baseNif.GetShapeTranslation(baseShapeName, trans);
	baseNif.GetShapeScale(baseShapeName, scale);
	trans -= rootTrans; // Get shape translation includes the root translation

	if (!rootTrans.IsZero()) {
		baseNif.OffsetShape(baseShapeName, rootTrans);
		baseNif.SetRootTranslation(vec3(0.0f, 0.0f, 0.0f));
	}
	if (rootScale != 1.0f) {
		baseNif.ScaleShape(baseShapeName, rootScale);
		baseNif.SetRootScale(1.0f);
	}

	if (!trans.IsZero()) {
		baseNif.OffsetShape(baseShapeName, trans);
		baseNif.SetShapeTranslation(baseShapeName, vec3(0.0f, 0.0f, 0.0f));
	}
	if (scale != 1.0f) {
		baseNif.ScaleShape(baseShapeName, scale);
		baseNif.SetShapeScale(baseShapeName, 1.0f);
	}

	baseAnim.LoadFromNif(&baseNif);
	activeSet.LoadSetDiffData(baseDiffData);

	AutoOffset(false);
	return 0;
}

int OutfitProject::LoadOutfit(const string& filename, const string& inOutfitName) {
	vector<string> refShapes;
	vector<string> workShapes;

	ClearOutfit();

	outfitName = inOutfitName;
	size_t fnpos = filename.rfind("\\");

	if (fnpos == string::npos) {
		mGameFile = filename;
	}
	else {
		mGameFile = filename.substr(fnpos + 1);
		size_t pos;
		pos = mGameFile.rfind("_1.nif");
		if (pos == string::npos)
			pos = mGameFile.rfind("_0.nif");

		if (pos == string::npos)
			pos = mGameFile.rfind(".nif");

		if (pos != string::npos)
			mGameFile = mGameFile.substr(0, pos);

		pos = filename.find("meshes");
		if (pos != string::npos)
			mGamePath = filename.substr(pos, fnpos - pos);
		else
			mGamePath = "";
	}

	if (filename.empty())
		return 0;

	if (workNif.Load(filename))
		return 1;

	RefShapes(refShapes);
	for (auto s : refShapes)
		RenameShape(s, s + "_outfit", true);

	OutfitShapes(workShapes);

	char chars[] = { '\\', '/', '?', ':', '*', '>', '<', '|', '"' };
	unordered_set<string> uniqueNames;
	vector<string> dups;
	for (auto ws : workShapes) {
		string oldWs = ws;
		for (unsigned int i = 0; i < sizeof(chars); ++i)
			replace(ws.begin(), ws.end(), chars[i], '_');
		if (oldWs != ws)
			RenameShape(oldWs, ws, true);
		if (uniqueNames.find(ws) == uniqueNames.end())
			uniqueNames.insert(ws);
		else
			dups.push_back(ws);
	}
	if (!dups.empty()) {
		for (auto d : dups){
			workNif.RenameDuplicateShape(d);
		}
	}
	OutfitShapes(workShapes);

	////// Do some cleanup on the incoming Nif File.  ////////

	// The root and shape translations are ignored by skyrim but not nifskope. Outfit studio also doesn't use them, so here we're just trying to make everything work
	// ^ NOT TRUE, they aren't ignored (only for body shapes?)
	vec3 rootTrans;
	float rootScale;
	workNif.GetRootTranslation(rootTrans);
	workNif.GetRootScale(rootScale);

	vec3 trans;
	float scale;
	for (auto s : workShapes) {
		workNif.GetShapeTranslation(s, trans);
		workNif.GetShapeScale(s, scale);
		trans -= rootTrans; // Get shape translation includes the root translation

		if (!rootTrans.IsZero())
			workNif.OffsetShape(s, rootTrans);
		if (rootScale != 1.0f)
			workNif.ScaleShape(s, rootScale);

		if (!trans.IsZero()) {
			workNif.OffsetShape(s, trans);
			workNif.SetShapeTranslation(s, vec3(0.0f, 0.0f, 0.0f));
		}
		if (scale != 1.0f) {
			workNif.ScaleShape(s, scale);
			workNif.SetShapeScale(s, 1.0f);
		}
	}
	workNif.SetRootTranslation(vec3(0.0f, 0.0f, 0.0f));
	workNif.SetRootScale(1.0f);

	workAnim.LoadFromNif(&workNif);
	AutoOffset(true);

	// No shapes in nif file
	if (workShapes.size() == 0)
		return 2;

	return 0;
}

int OutfitProject::AddNif(const string& filename) {
	vector<string> refShapes;
	vector<string> workShapes;
	NifFile nif;

	if (filename.empty())
		return 0;

	if (nif.Load(filename))
		return 1;

	RefShapes(refShapes);
	for (auto s : refShapes)
		nif.RenameShape(s, s + "_outfit");

	nif.GetShapeList(workShapes);

	char chars[] = { '\\', '/', '?', ':', '*', '>', '<', '|', '"' };
	unordered_set<string> uniqueNames;
	vector<string> dups;
	for (auto ws : workShapes) {
		string oldWs = ws;
		for (unsigned int i = 0; i < sizeof(chars); ++i)
			replace(ws.begin(), ws.end(), chars[i], '_');
		if (oldWs != ws)
			nif.RenameShape(oldWs, ws);
		if (uniqueNames.find(ws) == uniqueNames.end())
			uniqueNames.insert(ws);
		else
			dups.push_back(ws);

		unordered_set<string> existing;
		vector<string> existingShapes;
		workNif.GetShapeList(existingShapes);
		for (auto s : existingShapes)
			existing.insert(s);
		if (existing.find(ws) != existing.end())
			dups.push_back(ws);
	}

	if (!dups.empty()) {
		for (auto d : dups) {
			nif.RenameShape(d, d + "_dup");
		}
	}
	nif.GetShapeList(workShapes);

	// The root and shape translations are ignored by skyrim but not nifskope. Outfit Studio also doesn't use them, so here we're just trying to make everything work
	// ^ NOT TRUE, they aren't ignored (only for body shapes?)
	vec3 rootTrans;
	float rootScale;
	nif.GetRootTranslation(rootTrans);
	nif.GetRootScale(rootScale);

	vec3 trans;
	float scale;
	for (auto s : workShapes) {
		nif.GetShapeTranslation(s, trans);
		nif.GetShapeScale(s, scale);
		trans -= rootTrans; // Get shape translation includes the root translation

		if (!rootTrans.IsZero())
			nif.OffsetShape(s, rootTrans);
		if (rootScale != 1.0f)
			nif.ScaleShape(s, rootScale);

		if (!trans.IsZero()) {
			nif.OffsetShape(s, trans);
			nif.SetShapeTranslation(s, vec3(0.0f, 0.0f, 0.0f));
		}
		if (scale != 1.0f) {
			nif.ScaleShape(s, scale);
			nif.SetShapeScale(s, 1.0f);
		}

		workNif.CopyShape(s, nif, s);
		workAnim.LoadFromNif(&nif, s);
	}
	nif.Clear();

	AutoOffset(true);

	// No shapes in nif file
	if (workShapes.size() == 0)
		return 3;

	return 0;
}

int OutfitProject::LoadProject(const string& filename) {
	return 0;
}

int OutfitProject::OutfitFromSliderSet(const string& filename, const string& sliderSetName) {
	owner->StartProgress("Loading slider set");
	SliderSetFile InSS(filename);
	if (InSS.fail()) {
		owner->EndProgress();
		return 1;
	}
	owner->UpdateProgress(10, "Retrieving outfit sliders");
	SliderSet tmpSet;
	if (InSS.GetSet(sliderSetName, tmpSet)) {
		owner->EndProgress();
		return 2;
	}
	owner->UpdateProgress(20, "Retrieving reference sliders");
	if (InSS.GetSet(sliderSetName, activeSet, LOADSS_REFERENCE | LOADSS_ADDEXCLUDED)) {
		owner->EndProgress();
		return 3;
	}

	tmpSet.SetBaseDataPath("ShapeData");
	activeSet.SetBaseDataPath("ShapeData");
	string inputNif = tmpSet.GetInputFileName();

	owner->UpdateProgress(30, "Loading outfit shapes");
	if (LoadOutfit(inputNif, sliderSetName)) {
		owner->EndProgress();
		return 4;
	}

	vector<string> refTargets;
	activeSet.GetReferencedTargets(refTargets);
	baseShapeName = activeSet.TargetToShape(refTargets[0]);

	if (!baseShapeName.empty())
		DeleteOutfitShape(baseShapeName);

	owner->UpdateProgress(90, "Updating outfit slider data");
	morpher.LoadResultDiffs(tmpSet);

	mFileName = filename;
	mOutfitName = sliderSetName;
	mDataDir = tmpSet.GetDefaultDataFolder();
	mBaseFile = tmpSet.GetInputFileName();
	size_t slashpos = mBaseFile.rfind("\\");
	if (slashpos != string::npos)
		mBaseFile = mBaseFile.substr(slashpos + 1);

	mGamePath = tmpSet.GetOutputPath();
	mGameFile = tmpSet.GetOutputFile();
	mCopyRef = true;
	mGenWeights = tmpSet.GenWeights();

	owner->UpdateProgress(100, "Complete");
	owner->EndProgress();
	return 0;
}

void OutfitProject::AutoOffset(bool IsOutfit) {
	vector<string> outfitShapes;
	const AnimBone* outfitBone = NULL;
	string foundShape;
	vec3 offset;

	NifFile nif;
	if (IsOutfit) {
		nif.CopyFrom(workNif);
		OutfitShapes(outfitShapes);
	}
	else {
		nif.CopyFrom(baseNif);
		RefShapes(outfitShapes);
	}

	//string bone = "";
	//vector<string> shapes;
	//vector<string> nifBones;
	//vector<string> skelBones;

	//if (!AnimSkeleton::getInstance().GetActiveBoneNames(skelBones))
	//	return;

	//if (!nif.GetShapeList(shapes))
	//	return;

	//for (auto s: shapes) {
	//	if (!nif.GetShapeBoneList(s, nifBones))
	//		return;
	//	for (auto nb: nifBones) {
	//		auto it = find_if(skelBones.begin(), skelBones.end(), [&](string & sb){ return _stricmp(sb.c_str(), nb.c_str()) == 0; }); 
	//		if (it != skelBones.end()) {
	//			bone = *it;
	//			break;
	//		}
	//	}
	//	if (bone != "")
	//		break;
	//	nifBones.clear();
	//}

	//if (bone == "") {
	//	//wxMessageBox("No matching bone comparison bone found! Outfit might need manual offset correction.", "Auto Offset");
	//	return;
	//}

	//vector<vec3> nifRot;
	//vec3 nifTrans;
	//float nifScale;
	//if (!nif.GetNodeTransform(bone, nifRot, nifTrans, nifScale))
	//	return;

	//AnimBone skelBone;
	//if (!AnimSkeleton::getInstance().GetBone(bone, skelBone))
	//	return;

	//offset = nifTrans;
	//offset -= skelBone.trans;

	//wxMessageBox("Bone: " + bone + ", " + to_string(skelBone.trans.x) + ", " + to_string(skelBone.trans.y) + ", " + to_string(skelBone.trans.z), "Auto Offset");

	/*
	//auto baseBone = baseAnim.GetShapeBone(baseShapeName, "NPC Pelvis [Pelv]");

	if(baseBone == NULL)
	return;
	for(auto s: outfitShapes) {
	outfitBone  = workAnim.GetShapeBone(s, "NPC Pelvis [Pelv]");
	if(outfitBone) {
	foundShape = s;
	break;
	}
	}
	if(outfitBone == NULL)
	return;

	offset = workAnim.shapeSkinning[foundShape].boneWeights[outfitBone->order].xform.translation;
	offset -= baseAnim.shapeSkinning[baseShapeName].boneWeights[baseBone->order].xform.translation;
	*/

	for (auto s : outfitShapes) {
		skin_transform xformOverall;
		vec3 u;
		float f;
		vector<vec3> verts;
		Mat4 t;

		if (nif.GetShapeBoneTransform(s, -1, xformOverall, u, f)) {
			t = xformOverall.ToMatrixSkin();

			nif.GetVertsForShape(s, verts);
			for (auto& v : verts) {
				v = t * v;
			}
			nif.SetVertsForShape(s, verts);

			xformOverall.translation.Zero();
			xformOverall.rotation[0] = vec3(1.0f, 0.0f, 0.0f);
			xformOverall.rotation[1] = vec3(0.0f, 1.0f, 0.0f);
			xformOverall.rotation[2] = vec3(0.0f, 0.0f, 1.0f);
			xformOverall.scale = 1.0f;
			nif.SetShapeBoneTransform(s, -1, xformOverall, u, f);
		}
		//nif.OffsetShape(s, offset * -1.0f);

		//// WIP
		//skin_transform xformBone, xformSkin;
		//vector<vec3> vertices;
		//vector<string> bones;
		//nif.GetVertsForShape(s, vertices);
		//nif.GetShapeBoneList(s, bones);

		//for (auto b: bones) {
		//	nif.GetShapeBoneTransform(s, b, xformBone, u, f);
		//	AnimSkeleton::getInstance().GetSkinTransform(b, xformSkin);
		//	//xformSkin = workAnim.shapeSkinning[s].boneWeights[workAnim.GetShapeBoneIndex(s, b)].xform;
		//	Mat4 matBone(xformBone.ToMatrix());
		//	Mat4 matSkin(xformSkin.ToMatrix());
		//	Mat4 matResult(matBone * matSkin);

		//	for (auto v: workAnim.shapeSkinning[s].boneWeights[workAnim.GetShapeBoneIndex(s, b)].weights) {
		//		vertices[v.first] += (matResult * vertices[v.first]) * v.second;
		//	}
		//}
		//nif.SetVertsForShape(s, vertices);
	}

	//vector<vec3> verts;
	//AnimInfo anim;
	//if (IsOutfit) anim = workAnim;
	//else anim = baseAnim;

	if (IsOutfit) workNif.CopyFrom(nif);
	else baseNif.CopyFrom(nif);
	nif.Clear();
}

void OutfitProject::InitConform() {
	morpher.SetRef(baseNif, baseShapeName);
	morpher.LinkRefDiffData(&baseDiffData);
	morpher.SourceShapesFromNif(workNif);
}

void OutfitProject::ConformShape(const string& shapeName) {
	if (!baseNif.IsValid() || !workNif.IsValid())
		return;

	morpher.BuildProximityCache(shapeName);

	string refTarget = ShapeToTarget(baseShapeName);
	for (int i = 0; i < activeSet.size(); i++)
		if (SliderShow(i))
			morpher.GenerateResultDiff(shapeName, activeSet[i].Name, activeSet[i].TargetDataName(refTarget));
}

void OutfitProject::DuplicateOutfitShape(const string& sourceShape, const string& destShape, const mesh* m) {
	NifFile* clone = new NifFile(workNif);
	workAnim.WriteToNif(clone);

	vector<vec3> liveVerts;
	//vector<vec3> liveNorms;
	for (int i = 0; i < m->nVerts; i++) {
		liveVerts.emplace_back(move(vec3(m->verts[i].x * -10, m->verts[i].z * 10, m->verts[i].y * 10)));
		//liveNorms.emplace_back(move(vec3(m->verts[i].nx* -1, m->verts[i].nz, m->verts[i].ny)));
	}
	clone->SetVertsForShape(m->shapeName, liveVerts);

	workNif.CopyShape(destShape, *clone, sourceShape);
	workAnim.LoadFromNif(&workNif, destShape);
	delete clone;
}

void OutfitProject::DuplicateRefShape(const string& sourceShape, const string& destShape, const mesh* m) {
	NifFile* clone = new NifFile(baseNif);
	baseAnim.WriteToNif(clone);

	vector<vec3> liveVerts;
	//vector<vec3> liveNorms;
	for (int i = 0; i < m->nVerts; i++) {
		liveVerts.emplace_back(move(vec3(m->verts[i].x * -10, m->verts[i].z * 10, m->verts[i].y * 10)));
		//liveNorms.emplace_back(move(vec3(m->verts[i].nx* -1, m->verts[i].nz, m->verts[i].ny)));
	}
	clone->SetVertsForShape(m->shapeName, liveVerts);

	workNif.CopyShape(destShape, *clone, sourceShape);
	workAnim.LoadFromNif(&workNif, destShape);
	delete clone;
}

void OutfitProject::RenameShape(const string& shapeName, const string& newShapeName, bool isOutfit) {
	if (isOutfit) {
		workNif.RenameShape(shapeName, newShapeName);
		workAnim.RenameShape(shapeName, newShapeName);
		morpher.RenameShape(shapeName, newShapeName);
	}
	else {
		baseNif.RenameShape(shapeName, newShapeName);
		baseAnim.RenameShape(shapeName, newShapeName);
		activeSet.RenameShape(shapeName, newShapeName);
	}
	if (shapeDirty.find(shapeName) != shapeDirty.end()) {
		shapeDirty.erase(shapeName);
		shapeDirty[newShapeName] = true;
	}
}

int OutfitProject::SaveOutfitNif(const string& filename) {
	return(workNif.Save(filename));
}

void OutfitProject::UpdateNifNormals(NifFile* nif, const vector<mesh*>& shapeMeshes) {
	vector<vec3> liveNorms;
	for (auto m : shapeMeshes) {
		liveNorms.clear();
		for (int i = 0; i < m->nVerts; i++)
			liveNorms.emplace_back(move(vec3(m->verts[i].nx* -1, m->verts[i].nz, m->verts[i].ny)));

		nif->SetNormalsForShape(m->shapeName, liveNorms);
		//nif->RecalculateNormals();
		nif->CalcTangentsForShape(m->shapeName);
	}
}

int OutfitProject::SaveModifiedOutfitNif(const string& filename, const vector<mesh*>& modMeshes, bool writeNormals) {
	NifFile* clone;
	if (!workNif.IsValid())
		clone = new NifFile(baseNif);
	else
		clone = new NifFile(workNif);

	clone->SetNodeName(0, "Scene Root");

	vector<vec3> liveVerts;
	vector<vec3> liveNorms;
	for (auto m : modMeshes) {
		liveVerts.clear();
		liveNorms.clear();
		for (int i = 0; i < m->nVerts; i++) {
			liveVerts.emplace_back(move(vec3(m->verts[i].x * -10, m->verts[i].z * 10, m->verts[i].y * 10)));
			liveNorms.emplace_back(move(vec3(m->verts[i].nx * -1, m->verts[i].nz, m->verts[i].ny)));
		}
		clone->SetVertsForShape(m->shapeName, liveVerts);
		auto shader = clone->GetShaderForShape(m->shapeName);
		if (writeNormals && shader && !shader->IsSkinShader()) {
			clone->SetNormalsForShape(m->shapeName, liveNorms);
			//clone->RecalculateNormals();
			clone->CalcTangentsForShape(m->shapeName);
		}
	}

	workAnim.WriteToNif(clone);
	vector<string> shapes;
	clone->GetShapeList(shapes);
	for (auto s : shapes) {
		clone->UpdateSkinPartitions(s);
		//clone.BuildSkinPartitions(s);
	}

	int ret = clone->Save(filename);
	delete clone;

	return ret;
}

int OutfitProject::ExportShape(const string& shapeName, const string& fName, bool isOutfit) {
	if (isOutfit)
		return workNif.ExportShapeObj(fName, shapeName, 0.1f);
	else
		return baseNif.ExportShapeObj(fName, shapeName, 0.1f);
}

string OutfitProject::NameAbbreviate(const string& inputName) {
	string o;
	string stripChars = "\\/?:*><|\"";
	//string stripChars = "\"'\t\n\\/";
	for (auto c : inputName) {
		if (stripChars.find(c) != string::npos)
			continue;

		o += c;
	}
	return o;
}
