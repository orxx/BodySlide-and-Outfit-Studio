/*
Caliente's Body Slide
by Caliente

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#pragma once
#include "stdafx.h"
#include "GLSurface.h"
#include "NifFile.h"

#include <wx/frame.h>
#include <wx/glcanvas.h>

#define SMALL_PREVIEW 0
#define BIG_PREVIEW 1

#ifdef _WIN32
LRESULT CALLBACK GLPreviewWindowWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK GLWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
#endif

class BodySlideApp;
class PreviewCanvas;

class PreviewWindow
{
	BodySlideApp* app{nullptr};

#ifdef _WIN32
	static bool has_registered;
	HWND mHwnd;
	HWND mGLWindow;
#else
	wxFrame* frame{nullptr};
	PreviewCanvas* canvas{nullptr};
	wxGLContext* context{nullptr};
#endif
	GLSurface gls;
#ifdef _WIN32
	void registerClass();
#endif
	std::unordered_map<std::string, GLMaterial*> shapeTextures;
	string baseDataPath;
public:
	bool isSmall;
#ifdef _WIN32
	HWND hOwner;
#endif
	PreviewWindow(void);
#ifdef _WIN32
	PreviewWindow(HWND owner, char PreviewType = SMALL_PREVIEW, char* shapeName = NULL);
	PreviewWindow(HWND owner, vector<vector3>* verts, vector<triangle>* tris, float scale = 1.0f);
#else
	PreviewWindow(BodySlideApp* app, char PreviewType = SMALL_PREVIEW, char* shapeName = NULL);
	void OnShown();
	void OnClose();
#endif

	void SetBaseDataPath(const string& path) {
		baseDataPath = path;
	}

	void Create(const string& title);

	void Update(int shapeIndex, vector<vector3>* verts, vector<vector2>* uvs = NULL) {
		gls.Update(shapeIndex, verts, uvs);
		string n = gls.GetMeshName(shapeIndex);
		gls.GetMesh(n)->SmoothNormals();
		Refresh();
	}

	void Refresh() {
#if _WIN32
		InvalidateRect(mGLWindow, NULL, FALSE);
#else
		Render();
#endif
	}

	void AddMeshDirect(mesh* m);
	void AddMeshFromNif(NifFile* nif, char* shapeName=NULL);
	void RefreshMeshFromNif(NifFile* nif, char* shapeName=NULL);
	void AddNifShapeTexture(NifFile* fromNif, const string& shapeName);

	void Update(string& shapeName, vector<vector3>* verts, vector<vector2>* uvs = NULL) {
		gls.Update(gls.GetMeshID(shapeName), verts, uvs);
		mesh* m = gls.GetMesh(shapeName);
		if(m) { // the mesh could be missing if a zap slider removes it
			gls.GetMesh(shapeName)->SmoothNormals();
		}
		Refresh();
	}

	void SetShapeTexture(const string& shapeName, const string& texturefile, int shaderType = 0) {
		mesh* m = gls.GetMesh(shapeName);
		if (!m) return;
		GLMaterial* mat;
		if (shaderType == 0 )
			mat = gls.AddMaterial(texturefile, "res\\defvshader.vs", "res\\defshader.fs");
		else 	
			mat = gls.AddMaterial(texturefile, "res\\defvshader.vs", "res\\skinshader.fs");

		m->material = mat;
		shapeTextures[shapeName] = mat;
	}

	void Render() {
		gls.RenderOneFrame();
	}

	void SetSize(unsigned int w, unsigned int h) {
#if _WIN32
		SetWindowPos(mGLWindow, 0, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER);
#else
                // FIXME
#endif
#if 0
		gls.SetSize(w, h);
#endif
	}

	void ToggleSmoothSeams() {
		mesh* m;
		for (auto s : shapeTextures) {
			m = gls.GetMesh(s.first);
			if (m) {
				if(m->smoothSeamNormals) {
					m->smoothSeamNormals = false;
				} else {
					m->smoothSeamNormals = true;
				}
				m->SmoothNormals();
			}
		}
		Refresh();
	}
	void ToggleSmoothSeams(mesh* m) {
		if (m) {
			if(m->smoothSeamNormals) {
				m->smoothSeamNormals = false;
			} else {
				m->smoothSeamNormals = true;
			}
			m->SmoothNormals();
		}
		Refresh();
	}

	void ToggleTextures() {
		gls.ToggleTextures();
		Refresh();
	}

	void ToggleWireframe() {
		gls.ToggleWireframe();
		Refresh();
	}

	void ToggleLighting(){
		gls.ToggleLighting();
		Refresh();
	}

	void ToggleEditMode() {
		if(gls.bEditMode == false )
			gls.BeginEditMode();
		else 
			gls.EndEditMode();
		
	}

	void Close();

	bool HasFocus() {
#if _WIN32
		return(mHwnd == GetFocus());
#else
		return frame->HasFocus();
#endif
	}

	void RightDrag(int dX, int dY);
	void LeftDrag(int dX, int dY);
	void MouseWheel(int dW);
	void TrackMouse(int X, int Y);

	void Pick(int X, int Y);
	~PreviewWindow(void);
};

#ifndef _WIN32
class PreviewFrame : public wxFrame {
	PreviewWindow* previewWindow{nullptr};

public:
	PreviewFrame(PreviewWindow* pw, const string& title);

	void OnClose(wxCloseEvent& event);

	DECLARE_EVENT_TABLE();
};

class PreviewCanvas : public wxGLCanvas {
	PreviewWindow* previewWindow{nullptr};
	bool firstPaint{true};
	wxPoint lastMousePosition;

public:
	PreviewCanvas(PreviewWindow* window,
		      wxWindow* wxwin,
		      const int* attribs);

	void OnPaint(wxPaintEvent& event);
	void OnKeyUp(wxKeyEvent& event);
	void OnMotion(wxMouseEvent& event);
	void OnMouseWheel(wxMouseEvent& event);

	DECLARE_EVENT_TABLE();
};
#endif // !_WIN32
