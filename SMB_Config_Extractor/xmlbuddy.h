#pragma once
typedef struct XMLBuddy XMLBuddy;

enum Tag_Types {
	TAG_X,
	TAG_Y,
	TAG_Z,
	TAG_TIME,
	TAG_VALUE,
	TAG_EASING,
};

enum Attribute_Types {
	ATTR_X,
	ATTR_Y,
	ATTR_Z,
	ATTR_TIME,
	ATTR_VALUE,
	ATTR_EASING,
};

enum ERROR_CODES {
	ERROR_BAD_STATE,
	NO_ERROR
};


XMLBuddy initXMLBuddy(char *filename, int prettyPrint);
void close(XMLBuddy xmlBuddy);

int startTag(XMLBuddy xmlBuddy, char *tagName);
int endTag(XMLBuddy xmlBuddy, char *tagName);

int startTagType(XMLBuddy xmlBuddy, enum tagType tag);
int endTagType(XMLBuddy xmlBuddy, enum tagType tag);

int addAttrStr(XMLBuddy xmlBuddy, char *attrName, char *attrValue);
int addAttrInt(XMLBuddy xmlBuddy, char *attrName, int attrValue);
int addAttrDouble(XMLBuddy xmlBuddy, char *attrName, double attrValue);


int addAttrTypeStr(XMLBuddy xmlBuddy, enum Attribute_Type, char *attrValue);
int addAttrTypeInt(XMLBuddy xmlBuddy, enum Attribute_Type, int attrValue);
int addAttrTypeDouble(XMLBuddy xmlBuddy, enum Attribute_Type, double attrValue);
