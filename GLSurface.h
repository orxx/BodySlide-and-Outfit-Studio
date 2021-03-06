#pragma once
#include <Windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <GL/wglext.h>
#include "GLShader.h"
#include "NifFile.h"
#include "Object3d.h"
#include "KDMatcher.h"
#include "Mesh.h"
#include "ResourceLoader.h"
#include "TweakBrush.h"
#include <string>
#include <vector>
#include <hash_map>
#include <wx/glcanvas.h>

using namespace std;

class GLSurface {
	wxGLCanvas* canvas{nullptr};
	wxGLContext* context{nullptr};

	float mFov;
	vec3 camPos;
	vec3 camRot;		// Turntable camera emulation.
	unsigned int vpW;
	unsigned int vpH;

	bool bUseAF;
	GLfloat largestAF;

	bool bUseVBO;
	static bool multiSampleEnabled;

	bool bWireframe;
	bool bLighting;
	bool bTextured;
	bool bMaskVisible;
	bool bWeightColors;

	float defLineWidth;
	float defPointSize;
	float cursorSize;

	ResourceLoader resLoader;
	GLMaterial* noImage{nullptr};
	GLMaterial* skinMaterial{nullptr};
	unordered_map<string, int> texMats;

	TweakUndo tweakUndo;

	unordered_map<string, int> namedMeshes;
	unordered_map<string, int> namedOverlays;
	vector<mesh*> meshes;
	vector<mesh*> overlays;
	int activeMesh;

	void initLighting();
	void initMaterial(vec3 diffusecolor);
	void InitGLExtensions();
	int InitGLSettings(bool bUseDefaultShaders);
	static int QueryMultisample(wxWindow* parent);
	static int FindBestNumSamples(HDC hDC);

	void DeleteMesh(int meshID) {
		if (meshID < meshes.size()) {
			delete meshes[meshID];
			meshes.erase(meshes.begin() + meshID);
			for (int i = meshID; i < meshes.size(); i++) { // Renumber meshes after the deleted one.
				namedMeshes[meshes[i]->shapeName] = i;
			}
		}
		if (activeMesh >= meshID)
			activeMesh--;
	}

public:
	bool bEditMode;
	GLSurface(void);
	~GLSurface(void);

	// Get the attributes to use for creating a wxGLCanvas
	static const int* GetGLAttribs(wxWindow* parent);

	void DeleteAllMeshes() {
		for (auto m : meshes) {
			delete m;
		}
		meshes.clear();
		namedMeshes.clear();
	}


	void DeleteMesh(const string& shapeName) {
		int id = GetMeshID(shapeName);
		if (id >= 0) {
			DeleteMesh(id);
			namedMeshes.erase(shapeName);
		}
	}

	void DeleteOverlays() {
		for (int i = 0; i < overlays.size(); i++) {
			delete overlays[i];
		}
		overlays.clear();
		namedOverlays.clear();
	}


	int GetMeshID(const string& shapeName) {
		auto it = namedMeshes.find(shapeName);
		if (it != namedMeshes.end())
			return it->second;

		return -1;
	}
	string GetMeshName(int id) {
		for (auto mn : namedMeshes)
			if (mn.second == id)
				return mn.first;

		return "";
	}

	void RenameMesh(const string& shapeName, const string& newShapeName) {
		if (namedMeshes.find(shapeName) != namedMeshes.end()) {
			int mid = namedMeshes[shapeName];
			namedMeshes[newShapeName] = mid;
			meshes[mid]->shapeName = newShapeName;
			namedMeshes.erase(shapeName);
		}
	}


	int GetOverlayID(const string& shapeName) {
		unordered_map<string, int>::iterator it = namedOverlays.find(shapeName);
		if (it != namedOverlays.end()) {
			return it->second;
		}

		return -1;
	}
	mesh* GetActiveMesh() {
		if (activeMesh >= 0 && activeMesh < meshes.size())
			return meshes[activeMesh];

		return NULL;
	}

	vec3 GetActiveCenter(bool useMask = true) {
		mesh* m = GetActiveMesh();
		if (!m) return vec3(0, 0, 0);
		int count = 0;
		vec3 total;
		for (int i = 0; i < m->nVerts; i++) {
			if (!useMask || m->vcolors[i].x == 0.0f) {
				total = total + m->verts[i];
				count++;
			}
		}
		total = total / count;
		return total;
	}

	bool IsValidMesh(mesh* query) {
		for (int i = 0; i < meshes.size(); i++) {
			if (meshes[i] == query) {
				return true;
			}
		}
		return false;
	}

	mesh* GetMesh(const string& shapeName) {
		int id = GetMeshID(shapeName);
		if (id >= 0)
			return meshes[id];
		return NULL;
	}

	mesh* GetOverlay(int overlayID) {
		if (overlayID >= 0)
			return overlays[overlayID];
		return NULL;
	}

	mesh* GetOverlay(const string& overlayName) {
		int id = GetOverlayID(overlayName);
		if (id >= 0)
			return overlays[id];
		return NULL;
	}

	float GetCursorSize() {
		return cursorSize;
	}

	void SetCursorSize(float newsize) {
		cursorSize = newsize;
	}

	static bool IsWGLExtensionSupported(char* szTargetExtension);
	static bool IsExtensionSupported(char* szTargetExtension);
	int Initialize(wxGLCanvas* canvas, wxGLContext* context, bool bUseDefaultShaders = true);
	void Begin();
	void Cleanup();

	void SetStartingView(vec3 camPos, unsigned int vpWidth, unsigned int vpHeight, float fov = 60.0f);
	void SetSize(unsigned int w, unsigned int h);

	void TurnTableCamera(int dScreenX);
	void PitchCamera(int dScreenY);
	void PanCamera(int dScreenX, int dScreenY);
	void DollyCamera(int dAmount);
	void UnprojectCamera(vector3& result);

	void GetPickRay(int ScreenX, int ScreenY, vec3& dirVect, vec3& outNearPos);
	int PickMesh(int ScreenX, int ScreenY);
	bool UpdateCursor(int ScreenX, int ScreenY, int* outHoverTri = NULL, float* outHoverWeight = NULL, float* outHoverMask = NULL);
	bool GetCursorVertex(int ScreenX, int ScreenY, vtx* outHoverVtx = NULL);
	void ShowCursor(bool show = true);

	// Ray/mesh collision detection. From a screen point, calculates a ray and finds the nearest collision point and surface normal on
	// the active mesh. Optionally, the ray and ray origin can be provided, which skips the internal call to GetPickRay.
	// Screen x/y are ignored if the ray is provided.
	bool CollideMesh(int ScreenX, int ScreenY, vec3& outOrigin, vec3& outNormal, int* outFacet = NULL, vec3* inRayDir = 0, vec3* inRayOrigin = 0);
	bool CollidePlane(int ScreenX, int ScreenY, vec3& outOrigin, const vec3& inPlaneNormal, float inPlaneDist);
	int CollideOverlay(int ScreenX, int ScreenY, vec3& outOrigin, vec3& outNormal, int* outFacet = NULL, vec3* inRayDir = 0, vec3* inRayOrigin = 0);

	int AddVisRay(vec3& start, vec3& direction, float length);
	int AddVisCircle(const vec3& center, const vec3& normal, float radius, const string& name = "RingMesh");
	int AddVis3dRing(const vec3& center, const vec3& normal, float holeRadius, float ringRadius, const vec3& color, const string& name = "XRotateMesh");
	int AddVis3dArrow(const vec3& origin, const vec3& direction, float stemRadius, float pointRadius, float length, const vec3& color, const string& name = "XMoveMesh");
	int AddVisPoint(const vec3& p, const string& name = "PointMesh");
	int AddVisTri(const vec3& p1, const vec3& p2, const vec3& p3, const string& name = "TriMesh");
	int AddVisFacets(vector<int>& triIDs, const string& name = "TriMesh");
	int AddVisFacetsInSphere(vec3& origin, float radius, const string& name = "SphereFIntersect");
	int AddVisPointsInSphere(vec3& origin, float radius, const string& name = "SpherePIntersect");

	void BeginEditMode();
	void EditUndo();
	void EditRedo();
	void EndEditMode();

	void AddMeshFromNif(NifFile* nif, string shapeName, vec3* color = NULL, bool smoothNormalSeams = true);
	void AddMeshExplicit(vector<vector3>* verts, vector<triangle>* tris, vector<vector2>* uvs = NULL, const string& name = "", float scale = 1.0f);
	void AddMeshDirect(mesh* m);
	void Update(const string& shapeName, vector<vector3>* vertices, vector<vector2>* uvs = NULL);
	void Update(int shapeIndex, vector<vector3>* vertices, vector<vector2>* uvs = NULL);
	void ReloadMeshFromNif(NifFile* nif, string shapeName);
	void RecalculateMeshBVH(const string& shapeName);
	void RecalculateMeshBVH(int shapeIndex);

	void SetMeshVisibility(const string& name, bool visible = true);
	void SetMeshVisibility(int shapeIndex, bool visible = true);
	void SetOverlayVisibility(const string& name, bool visible = true);
	void SetActiveMesh(int shapeIndex);
	void SetActiveMesh(const string& shapeName);

	RenderMode SetMeshRenderMode(const string& name, RenderMode mode);

	GLMaterial* AddMaterial(const string& textureFile, const string& vShaderFile, const string& fShaderFile);

	int TestRender();
	int RenderOneFrame();
	void RenderMesh(mesh* m);

	void ToggleTextures() {
		if (bTextured)
			bTextured = false;
		else
			bTextured = true;

		for (int i = 0; i < meshes.size(); i++)
			if (meshes[i]->material && (activeMesh > -1))
				meshes[i]->material->shader->ShowTexture(bTextured);
	}

	void ToggleWireframe() {
		if (bWireframe)
			bWireframe = false;
		else
			bWireframe = true;
	}

	void ToggleLighting() {
		if (bLighting)
			bLighting = false;
		else
			bLighting = true;

		for (int i = 0; i < meshes.size(); i++)
			if (meshes[i]->material && (activeMesh > -1))
				meshes[i]->material->shader->EnableVertexLighting(bLighting);
	}

	void ToggleMask() {
		if (bMaskVisible)
			bMaskVisible = false;
		else
			bMaskVisible = true;

		for (int i = 0; i < meshes.size(); i++)
			if (meshes[i]->material && (activeMesh > -1))
				meshes[i]->material->shader->ShowMask(bMaskVisible);
	}

	void SetWeightColors(bool bVisible = true) {
		bWeightColors = bVisible;

		for (int i = 0; i < meshes.size(); i++)
			if (meshes[i]->material && (activeMesh > -1))
				meshes[i]->material->shader->ShowWeight(bWeightColors);
	}

	void ToggleWeightColors() {
		if (bWeightColors)
			bWeightColors = false;
		else
			bWeightColors = true;

		if (meshes.size() > 0 && (activeMesh > -1) && meshes[activeMesh]->material)
			meshes[activeMesh]->material->shader->ShowWeight(bWeightColors);
	}
};
