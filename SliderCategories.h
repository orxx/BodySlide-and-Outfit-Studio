#pragma once
#include "stdafx.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <unordered_set>
#include "tinyxml.h"

using namespace std;

class SliderCategory {
	string name;
	vector<string> sliders;
	vector<string> sourceFiles;
	unordered_set<string> uniqueSourceFiles;
	bool isValid;
	bool isHidden;

public:
	SliderCategory():isValid(false), isHidden(false) { }
	SliderCategory(TiXmlElement* srcCategoryElement) {
		if (LoadCategory(srcCategoryElement)) {
			isValid = false;
		} 
		isValid = true;
		isHidden = false;
	}

	string GetName() { return name; }
	void SetName(const string& inName) { name = inName; }

	int AddSliders(const vector<string>& inSliders);
	bool HasSlider(const string& search);
	int GetSliders(vector<string>& outSliders);
	int GetSliders(unordered_set<string>& outSliders);
	int AppendSliders(vector<string>& outSliders);
	int AppendSliders(unordered_set<string>& outSliders);
	
	bool GetHidden();
	void SetHidden(bool hide);

	// Combine the source category's sliders into this one's list. Also merges the source file list.
	void MergeSliders(const SliderCategory& sourceCategory);
	
	int LoadCategory(TiXmlElement* srcCategoryElement);
	void WriteCategory(TiXmlElement* categoryElement, bool append = false);
	void AddSourceFile(const string& fileName);
};


class SliderCategoryCollection {
	map<string, SliderCategory> categories;

public:
	// Loads all categories in the specified folder.
	int LoadCategories(const string& basePath);

	int GetAllCategories(vector<string>& outCategories);
	int GetSliderCategory(const string& sliderName, string& outCategory);

	bool GetCategoryHidden(const string& categoryName);
	int SetCategoryHidden(const string& categoryName, bool hide);

	int GetCategorySliders(const string& categoryName, vector<string>& outSliders, bool append = true);
	int GetCategorySliders(const string& categoryName, unordered_set<string>& outSliders, bool append = true);
}; 


class SliderCategoryFile {
	TiXmlDocument doc;
	TiXmlElement* root;
	map<string, TiXmlElement*> categoriesInFile;
	int error;

public:
	string fileName;
	SliderCategoryFile():error(0), root(NULL) {}
	SliderCategoryFile(const string& srcFileName);
	~SliderCategoryFile() {};
	
	bool fail() { return error != 0; }
	int GetError() { return error; }
	
	// Loads the XML document and identifies included category names. On a failure, sets the internal error value.
	void Open(const string& srcFileName);

	// Creates a new empty category document structure, ready to add new categories and sliders to.
	void New(const string& newFileName);

	// Changes the internal file name. The XML file isn't saved until the Save() function is used.
	// Note the original file name is not changed. This method allows you to save a category as a new file without altering the original.
	void Rename(const string& newFileName);

	// Returns a list of all the categories found in the file.
	int GetCategoryNames(vector<string>& outCategoryNames, bool append = true, bool unique = false);

	// Returns true if the category name exists in the file.
	bool HasCategory(const string& queryCategoryName);
	
	// Adds all of the categories in the file to the supplied categories vector. Does not clear the vector before doing so.
	int GetAllCategories(vector<SliderCategory>& outAppendCategories);

	// Gets a single category from the XML document based on the name.
	int GetCategory(const string& categoryName, SliderCategory& outCategories);

	// Updates a slider category in the xml document with the provided information.
	// If the category does not already exist in the file (based on name) the category is added.
	int UpdateCategory(SliderCategory& inCategory);

	// Writes the XML file using the internal fileName (use Rename() to change the name).
	int Save();
};
