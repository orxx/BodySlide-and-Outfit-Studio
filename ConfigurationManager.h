#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "tinyxml.h"
#include "Portability.h"

using namespace std;

class ConfigurationItem {
	vector<ConfigurationItem*> children;
	vector<ConfigurationItem*> properties;

public:
	ConfigurationItem() : parent(NULL), level(0), isProp(false), isComment(false), isDefault(false) { }
	ConfigurationItem(TiXmlElement* srcElement, ConfigurationItem* inParent = NULL, int inLevel = 0) {
		parent = inParent;
		level = inLevel;
		isProp = false;
		isComment = false;
		isDefault = false;
		SettingFromXML(srcElement);
	}
	ConfigurationItem(const string& text, ConfigurationItem* inParent = NULL, int inLevel = 0) {
		parent = inParent;
		level = inLevel;
		isProp = false;
		isComment = true;
		isDefault = false;
		value = text;
	}
	~ConfigurationItem();

	ConfigurationItem* parent;
	int level;
	bool isProp;
	bool isComment;
	bool isDefault;								// Default values are not saved with config

	string name;								// Name associated with this node 
	string value;								// Value associated with this node 
	string path;

	int SettingFromXML(TiXmlElement* xml);

	void ToXML(TiXmlElement* parent);
	int EnumerateProperties(vector<ConfigurationItem*>& outList);
	int EnumerateProperties(string& outList);
	int EnumerateChildren(vector<ConfigurationItem*>& outList, bool withProperties = true, bool traverse = true);
	int EnumerateChildren(const string& inName, vector<string>& outValueList);
	int EnumerateChildrenProperty(const string& inName, const string& propertyName, vector<string>& outValueList);
	ConfigurationItem* FindChild(const string& inName, bool recurse = true);
	ConfigurationItem* AddChild(const string& inName, const string& value, bool isElement = true);

	ConfigurationItem* FindProperty(const string& inName);

	bool Match(const string& otherName) {
		return (!_stricmp(otherName.c_str(), name.c_str()));
	}
};

class ConfigurationManager
{
	vector<ConfigurationItem*> ciList;
	string file;
	ConfigurationItem* FindCI(const string& inName);

public:
	ConfigurationManager();
	~ConfigurationManager();

	void Clear();
	int LoadConfig(const string& pathToFile = "Config.xml", const string& rootElement = "BodySlideConfig");

	int SaveConfig(const string& pathToFile, const string& rootElementName = "BodySlideConfig");

	int EnumerateCIs (vector<ConfigurationItem*>& outList, bool withProperties = true, bool traverse = true);

	int EnumerateChildCIs(vector<ConfigurationItem*>& outList, const string& parentCI, bool withProperties = true, bool traverse = true);

	bool Exists(const string& inName);

	const char* GetCString(const string& inName, const string& def = NULL);

	string GetString(const string& inName);

	int GetIntValue(const string& inName, int def = 0);
	float GetFloatValue(const string& inName, float def = 0.0f);

	void SetValue(const string& inName, const string& newValue, bool flagDefault = false);
	void SetValue(const string& inName, int newValue, bool flagDefault = false);
	void SetValue(const string& inName, float newValue, bool flagDefault = false);

	void SetDefaultValue(const string& inName, const string& newValue);

	bool MatchValue(const string& inName, const string& val, bool useCase = false);

	void GetFullKey(ConfigurationItem* from, string& outstr);

	int GetValueArray(const string& containerName, const string& arrayName, vector<string>& outValues);
	int GetValueAttributeArray(const string& containerName, const string& arrayName, const string& attributeName, vector<string>& outValues);

	string operator [] (const string& inName) {
		return GetString(inName);
	}
};

extern ConfigurationManager Config;
