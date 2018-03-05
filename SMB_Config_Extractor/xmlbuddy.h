#pragma once
typedef struct XMLBuddy XMLBuddy;

enum Tag_Types {
	TAG_TITLE,
	TAG_MODEL_IMPORT,
	TAG_START,
	TAG_NAME,
	TAG_POSITION,
	TAG_ROTATION,
	TAG_SCALE,
	TAG_BACKGROUND_MODEL,
	TAG_FALLOUT_PLANE,
	TAG_ITEM_GROUP,
	TAG_ROTATION_CENTER,
	TAG_INITIAL_ROTATION,
	TAG_COLLISION_GRID,
	TAG_STEP,
	TAG_COUNT,
	TAG_COLLISION,
	TAG_OBJECT,
	TAG_GOAL,
	TAG_TYPE,
	TAG_BUMPER,
	TAG_JAMABAR,
	TAG_BANANA,
	TAG_FALLOUT_VOLUME,
	TAG_LEVEL_MODEL,
	TAG_WORMHOLE,
	TAG_DESTINATION_NAME,
	TAG_ANIM_LOOP_TIME,
	TAG_ANIM_KEYFRAMES,
	TAG_POS_X,
	TAG_POS_Y,
	TAG_POS_Z,
	TAG_ROT_X,
	TAG_ROT_Y,
	TAG_ROT_Z,
	TAG_KEYFRAME,
	TAG_INVALID_TAG
};

enum Attribute_Types {
	ATTR_VERSION,
	ATTR_TYPE,
	ATTR_X,
	ATTR_Y,
	ATTR_Z,
	ATTR_TIME,
	ATTR_VALUE,
	ATTR_EASING,
};

enum ERROR_CODES {
	ERROR_BAD_STATE,
	ERROR_EMPTY_STACK,
	NO_ERROR
};


XMLBuddy initXMLBuddy(char *filename, int prettyPrint);
void close(XMLBuddy xmlBuddy);

int startTag(XMLBuddy xmlBuddy, char *tagName);
int endTag(XMLBuddy xmlBuddy, char *tagName);

int startTagType(XMLBuddy xmlBuddy, enum tagType tagType);
int endTagType(XMLBuddy xmlBuddy, enum tagType tagType);

int endTag(XMLBuddy xmlBuddy);

int addAttrStr(XMLBuddy xmlBuddy, char *attrName, char *attrValue);
int addAttrInt(XMLBuddy xmlBuddy, char *attrName, int attrValue);
int addAttrDouble(XMLBuddy xmlBuddy, char *attrName, double attrValue);


int addAttrTypeStr(XMLBuddy xmlBuddy, enum Attribute_Type attr, char *attrValue);
int addAttrTypeInt(XMLBuddy xmlBuddy, enum Attribute_Type attr, int attrValue);
int addAttrTypeDouble(XMLBuddy xmlBuddy, enum Attribute_Type attr, double attrValue);

int addValStr(XMLBuddy xmlBuddy, char *value);
int addValInt(XMLBuddy xmlBuddy, int value);
int addValDouble(XMLBuddy xmlBuddy, double value);
