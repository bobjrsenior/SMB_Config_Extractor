#pragma once
#include <stdio.h>
#include <stdint.h>

#define TAG_STACK_SIZE 20

enum TAG_TYPE {
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
	TAG_ANIM_SEESAW_TYPE,
	TAG_SEESAW_SENSITIVITY,
	TAG_SEESAW_STIFFNESS,
	TAG_SEESAW_BOUNDS,
	TAG_CONVEYOR_SPEED,
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
	TAG_CONE,
	TAG_SPHERE,
	TAG_CYLINDER,
	TAG_FALLOUT_VOLUME,
	TAG_LEVEL_MODEL,
	TAG_REFLECTIVE_MODEL,
	TAG_WORMHOLE,
	TAG_SWITCH,
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
	TAG_ANIM_GROUP_ID,
	TAG_ANIM_INITIAL_STATE,
	TAG_INVALID_TAG
};

enum ATTRIBUTE_TYPE {
	ATTR_VERSION,
	ATTR_TYPE,
	ATTR_X,
	ATTR_Y,
	ATTR_Z,
	ATTR_TIME,
	ATTR_VALUE,
	ATTR_EASING,
};

enum ERROR_CODE {
	ERROR_BAD_STATE,
	ERROR_EMPTY_STACK,
	NO_ERROR
};

enum STATE {
	STATE_NEW,
	STATE_GENERAL,
	STATE_OPENING_TAG,
	STATE_ERROR,
	STATE_CLOSED
};

typedef struct XMLBuddy {
	FILE *output;
	enum STATE state;
	int prettyPrint;
	int indentation;
	int endTagOnNewLine;
	int tagStack[TAG_STACK_SIZE];
}XMLBuddy;


XMLBuddy *initXMLBuddy(char *filename, XMLBuddy *xmlBuddy, int prettyPrint);
XMLBuddy *initXMLBuddyFile(FILE *file, XMLBuddy *xmlBuddy, int prettyPrint);
void closeXMlBuddy(XMLBuddy *xmlBuddy);
void flushXMLBuddy(XMLBuddy *xmlBuddy);
//int startTag(XMLBuddy *xmlBuddy, char *tagName);
//int endTag(XMLBuddy *xmlBuddy, char *tagName);

int startTagType(XMLBuddy *xmlBuddy, enum TAG_TYPE tagType);
//int endTagType(XMLBuddy *xmlBuddy, enum TAG_TYPE tagType);

int endTag(XMLBuddy *xmlBuddy);

//int addAttrStr(XMLBuddy *xmlBuddy, char *attrName, char *attrValue);
//int addAttrInt(XMLBuddy *xmlBuddy, char *attrName, int attrValue);
//int addAttrDouble(XMLBuddy *xmlBuddy, char *attrName, double attrValue);


int addAttrTypeStr(XMLBuddy *xmlBuddy, enum ATTRIBUTE_TYPE attr, char *attrValue);
int addAttrTypeInt(XMLBuddy *xmlBuddy, enum ATTRIBUTE_TYPE attr, int attrValue);
int addAttrTypeDouble(XMLBuddy *xmlBuddy, enum ATTRIBUTE_TYPE attr, double attrValue);

int addValStr(XMLBuddy *xmlBuddy, char *value);
int addValInt(XMLBuddy *xmlBuddy, int value);
int addValUInt32(XMLBuddy *xmlBuddy, uint32_t value);
int addValDouble(XMLBuddy *xmlBuddy, double value);
