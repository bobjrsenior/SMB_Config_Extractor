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

enum Seesaw_Type {
	LOOPING_ANIMATION = 0x0000,
	PLAY_ONCE_ANIMATION = 0x0001,
	SEESAW = 0x0002
};

enum SWITCH_TYPE {
	PLAY = 0x0000,
	PAUSE = 0x0001,
	PLAY_BACWARD = 0x0002,
	FAST_FORWARD = 0x0003,
	REWIND = 0x0004
};

enum GOAL_TYPE {
	BLUE = 0x0001,
	GREEN = 0x0101,
	RED = 0x0201
};

enum BANANA_TYPE {
	SINGLE = 0x00000000,
	BUNCH = 0x00000001
};

enum EASING {
	CONSTANT = 0x00000000,
	LINEAR = 0x00000001,
	EASED = 0x00000002
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

typedef struct {
	float x;
	float y;
	float z;
}VectorF32;

typedef struct {
	uint16_t x;
	uint16_t y;
	uint16_t z;
}VectorI16;

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

void writeGoalType(XMLBuddy *xmlBuddy, uint16_t goalType);
void writeBananaType(XMLBuddy *xmlBuddy, uint32_t bananaType);
void writeAnimType(XMLBuddy *xmlBuddy, enum TAG_TYPE tagType, uint16_t switchType);
void writeAnimEasingVal(XMLBuddy *xmlBuddy, uint32_t easingVal);
void writeTagWithUInt32Value(XMLBuddy *xmlBuddy, enum TAG_TYPE tagType, uint32_t value);
void writeTagWithInt32Value(XMLBuddy *xmlBuddy, enum TAG_TYPE tagType, int value);
void writeTagWithFloatValue(XMLBuddy *xmlBuddy, enum TAG_TYPE tagType, float value);
void writeVectorF32(XMLBuddy *xmlBuddy, enum TAG_TYPE tagType, VectorF32 vectorF32);
void writeVectorI16(XMLBuddy *xmlBuddy, enum TAG_TYPE tagType, VectorI16 vectorI16);
void writeAnimSeesawType(XMLBuddy *xmlBuddy, uint16_t seesawType);
