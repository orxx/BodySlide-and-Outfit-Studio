#pragma once
#include "stdafx.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <unordered_set>
#include "tinyxml.h"

using namespace std;

class SliderSetGroup {
	string name;
	vector<string> members;
	vector<string> sourceFiles;
	unordered_set<string> uniqueSourceFiles;
	bool isValid;

public:
	SliderSetGroup():isValid(false) {}
	SliderSetGroup(TiXmlElement* srcGroupElement) {
		if (LoadGroup(srcGroupElement)) {
			isValid = false;
		} 
		isValid = true;
	}

	string GetName() { return name; }
	void SetName(const string& inName) { name = inName; }

	bool HasMember(const string& search);
	int GetMembers(vector<string>& outMembers);
	int AppendMembers(vector<string>& outMembers);
	int GetMembers(unordered_set<string>& outMembers);
	int AppendMembers(unordered_set<string>& outMembers);
	int AddMembers(const vector<string>& inMembers);
	
	// Combine the source groups members into this one's list. Also merges the source file list.
	void MergeMembers(const SliderSetGroup& sourceGroup);

	int LoadGroup (TiXmlElement* srcGroupElement);
	void WriteGroup(TiXmlElement* groupElement, bool append = false);
	void AddSourceFile(const string& fileName);
};


class SliderSetGroupCollection {
	map<string, SliderSetGroup> groups;

public:
	// Loads all groups in the specified folder.
	int LoadGroups(const string& basePath);

	int GetAllGroups(set<string>& outGroups);
	int GetOutfitGroups (const string& outfitName, vector<string>& outGroups);

	int GetGroupMembers (const string& groupName, vector<string>& outMembers, bool append = true);
	int GetGroupMembers (const string& groupName, unordered_set<string>& outMembers, bool append = true);
}; 


class SliderSetGroupFile {
	TiXmlDocument doc;
	TiXmlElement* root;
	map<string, TiXmlElement*> groupsInFile;
	int error;

public:
	string fileName;
	SliderSetGroupFile():error(0), root(NULL) {}
	SliderSetGroupFile(const string& srcFileName);
	~SliderSetGroupFile() {};
	
	bool fail() { return error != 0; }
	int GetError() { return error; }
	
	// Loads the XML document and identifies included group names. On a failure, sets the internal error value.
	void Open(const string& srcFileName);

	// Creates a new empty group document structure, ready to add new groups and members to.
	void New(const string& newFileName);

	// Changes the internal file name. The XML file isn't saved until the Save() function is used.
	// Note the original file name is not changed. This method allows you to save a group as a new file without altering the original.
	void Rename(const string& newFileName);

	// Returns a list of all the groups found in the file.
	int GetGroupNames(vector<string>& outGroupNames, bool append = true, bool unique = false);

	// Returns true if the group name exists in the file.
	bool HasGroup(const string& queryGroupName);
	
	// Adds all of the groups in the file to the supplied groups vector. Does not clear the vector before doing so.
	int GetAllGroups(vector<SliderSetGroup>& outAppendGroups);

	// Gets a single group from the XML document based on the name.
	int GetGroup(const string& groupName, SliderSetGroup& outSliderSetGroup);

	// Updates a slider set group in the xml document with the provided information.
	// If the group does not already exist in the file (based on name) the group is added.
	int UpdateGroup(SliderSetGroup& inGroup);

	// Writes the xml file using the internal fileName (use Rename() to change the name).
	int Save();
};
