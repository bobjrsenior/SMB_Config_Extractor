#include "xmlbuddy.h"

#include <inttypes.h>

static int printTagName(XMLBuddy *xmlBuddy, enum TAG_TYPE tagType);
static int printAttrName(XMLBuddy *xmlBuddy, enum ATTRIBUTE_TYPE attrType);

static int handleIndentation(XMLBuddy *xmlBuddy) {
	fputc('\n', xmlBuddy->output);
	for (int i = 0; i < xmlBuddy->indentation; i++) {
		fputs("    ", xmlBuddy->output);
	}
	return NO_ERROR;
}


XMLBuddy *initXMLBuddy(char *filename, XMLBuddy *xmlBuddy, int prettyPrint) {
	xmlBuddy->output = NULL;
	xmlBuddy->state = STATE_NEW;
	FILE *output = fopen(filename, "w");
	if (output == NULL) {
		xmlBuddy->state = STATE_ERROR;
		return NULL;
	}

	xmlBuddy->output = output;
	xmlBuddy->state = STATE_GENERAL;
	xmlBuddy->prettyPrint = prettyPrint;
	xmlBuddy->indentation = 0;
	xmlBuddy->endTagOnNewLine = 1;
	for (int i = 0; i < TAG_STACK_SIZE; i++) {
		xmlBuddy->tagStack[i] = TAG_INVALID_TAG;
	}
	return xmlBuddy;
}

XMLBuddy *initXMLBuddyFile(FILE *file, XMLBuddy *xmlBuddy, int prettyPrint) {
	xmlBuddy->output = NULL;
	xmlBuddy->state = STATE_NEW;
	if (file == NULL) {
		xmlBuddy->state = STATE_ERROR;
		return NULL;
	}
	xmlBuddy->output = file;
	xmlBuddy->state = STATE_GENERAL;
	xmlBuddy->prettyPrint = prettyPrint;
	xmlBuddy->indentation = 0;
	xmlBuddy->endTagOnNewLine = 1;
	for (int i = 0; i < TAG_STACK_SIZE; i++) {
		xmlBuddy->tagStack[i] = TAG_INVALID_TAG;
	}
	return xmlBuddy;
}

void closeXMlBuddy(XMLBuddy *xmlBuddy) {
	fclose(xmlBuddy->output);
	xmlBuddy->output = NULL;
	xmlBuddy->state = STATE_CLOSED;
	return;
}

void flushXMLBuddy(XMLBuddy *xmlBuddy) {
	fflush(xmlBuddy->output);
}

int startTagType(XMLBuddy *xmlBuddy, enum TAG_TYPE tagType) {
	if (xmlBuddy->state == STATE_OPENING_TAG) {
		putc('>', xmlBuddy->output);
		xmlBuddy->state = STATE_GENERAL;
		xmlBuddy->indentation++;
	}
	else if (xmlBuddy->state != STATE_GENERAL) {
		return ERROR_BAD_STATE;
	}
	handleIndentation(xmlBuddy);

	putc('<', xmlBuddy->output);
	printTagName(xmlBuddy, tagType);
	putc(' ', xmlBuddy->output);

	xmlBuddy->state = STATE_OPENING_TAG;
	xmlBuddy->tagStack[xmlBuddy->indentation] = tagType;
	return NO_ERROR;
}

int endTag(XMLBuddy *xmlBuddy) {
	if (xmlBuddy->state == STATE_OPENING_TAG) {
		fputs(" />", xmlBuddy->output);
		xmlBuddy->state = STATE_GENERAL;
		xmlBuddy->endTagOnNewLine = 1;
		return NO_ERROR;
	}
	else if (xmlBuddy->state != STATE_GENERAL) {
		return ERROR_BAD_STATE;
	}
	else if (xmlBuddy->indentation <= 0) {
		return ERROR_EMPTY_STACK;
	}

	xmlBuddy->indentation--;
	if (xmlBuddy->endTagOnNewLine) {
		handleIndentation(xmlBuddy);
	}
	else {
		xmlBuddy->endTagOnNewLine = 1;
	}

	fputs("</", xmlBuddy->output);
	printTagName(xmlBuddy, xmlBuddy->tagStack[xmlBuddy->indentation]);
	fputc('>', xmlBuddy->output);

	return NO_ERROR;
}

int addAttrTypeStr(XMLBuddy *xmlBuddy, enum ATTRIBUTE_TYPE attr, char *attrValue) {
	if (xmlBuddy->state != STATE_OPENING_TAG) {
		return ERROR_BAD_STATE;
	}
	
	printAttrName(xmlBuddy, attr);
	fputc('"', xmlBuddy->output);
	fputs(attrValue, xmlBuddy->output);
	fputs("\" ", xmlBuddy->output);

	xmlBuddy->endTagOnNewLine = 0;
	return NO_ERROR;
}

int addAttrTypeInt(XMLBuddy *xmlBuddy, enum ATTRIBUTE_TYPE attr, int attrValue) {
	if (xmlBuddy->state != STATE_OPENING_TAG) {
		return ERROR_BAD_STATE;
	}

	printAttrName(xmlBuddy, attr);
	fputc('"', xmlBuddy->output);
	fprintf(xmlBuddy->output, "%d", attrValue);
	fputs("\" ", xmlBuddy->output);

	xmlBuddy->endTagOnNewLine = 0;
	return NO_ERROR;
}

int addAttrTypeDouble(XMLBuddy *xmlBuddy, enum ATTRIBUTE_TYPE attr, double attrValue) {
	if (xmlBuddy->state != STATE_OPENING_TAG) {
		return ERROR_BAD_STATE;
	}

	printAttrName(xmlBuddy, attr);
	fputc('"', xmlBuddy->output);
	fprintf(xmlBuddy->output, "%f", attrValue);
	fputs("\" ", xmlBuddy->output);

	xmlBuddy->endTagOnNewLine = 0;
	return NO_ERROR;
}

int addValStr(XMLBuddy *xmlBuddy, char *value) {
	if (xmlBuddy->state == STATE_OPENING_TAG) {
		putc('>', xmlBuddy->output);
		xmlBuddy->state = STATE_GENERAL;
		xmlBuddy->indentation++;
	}
	else if (xmlBuddy->state != STATE_GENERAL) {
		return ERROR_BAD_STATE;
	}

	fputs(value, xmlBuddy->output);

	xmlBuddy->endTagOnNewLine = 0;
	return NO_ERROR;
}

int addValInt(XMLBuddy *xmlBuddy, int value) {
	if (xmlBuddy->state == STATE_OPENING_TAG) {
		putc('>', xmlBuddy->output);
		xmlBuddy->state = STATE_GENERAL;
		xmlBuddy->indentation++;
	}
	else if (xmlBuddy->state != STATE_GENERAL) {
		return ERROR_BAD_STATE;
	}

	fprintf(xmlBuddy->output, "%d", value);

	xmlBuddy->endTagOnNewLine = 0;
	return NO_ERROR;
}

int addValUInt32(XMLBuddy *xmlBuddy, uint32_t value) {
	if (xmlBuddy->state == STATE_OPENING_TAG) {
		putc('>', xmlBuddy->output);
		xmlBuddy->state = STATE_GENERAL;
		xmlBuddy->indentation++;
	}
	else if (xmlBuddy->state != STATE_GENERAL) {
		return ERROR_BAD_STATE;
	}

	fprintf(xmlBuddy->output, "%" PRIu32, value);
	
	xmlBuddy->endTagOnNewLine = 0;
	return NO_ERROR;
}

int addValDouble(XMLBuddy *xmlBuddy, double value) {
	if (xmlBuddy->state == STATE_OPENING_TAG) {
		putc('>', xmlBuddy->output);
		xmlBuddy->state = STATE_GENERAL;
		xmlBuddy->indentation++;
	}
	else if (xmlBuddy->state != STATE_GENERAL) {
		return ERROR_BAD_STATE;
	}

	fprintf(xmlBuddy->output, "%f", value);

	xmlBuddy->endTagOnNewLine = 0;
	return NO_ERROR;
}

void writeAnimType(XMLBuddy *xmlBuddy, enum TAG_TYPE tagType, uint16_t switchType) {
	startTagType(xmlBuddy, tagType);
	switch (switchType) {
	case PLAY:
		addValStr(xmlBuddy, "PLAY");
		break;
	case PAUSE:
		addValStr(xmlBuddy, "PAUSE");
		break;
	case PLAY_BACWARD:
		addValStr(xmlBuddy, "PLAY_BACKWARDS");
		break;
	case FAST_FORWARD:
		addValStr(xmlBuddy, "FAST_FORWARD");
		break;
	}
	endTag(xmlBuddy);
}

void writeTagWithUInt32Value(XMLBuddy *xmlBuddy, enum TAG_TYPE tagType, uint32_t value) {
	startTagType(xmlBuddy, tagType);
	addValUInt32(xmlBuddy, value);
	endTag(xmlBuddy);
}

void writeTagWithInt32Value(XMLBuddy *xmlBuddy, enum TAG_TYPE tagType, int value) {
	startTagType(xmlBuddy, tagType);
	addValInt(xmlBuddy, value);
	endTag(xmlBuddy);
}

void writeTagWithFloatValue(XMLBuddy *xmlBuddy, enum TAG_TYPE tagType, float value) {
	startTagType(xmlBuddy, tagType);
	addValDouble(xmlBuddy, value);
	endTag(xmlBuddy);
}

void writeVectorF32(XMLBuddy *xmlBuddy, enum TAG_TYPE tagType, VectorF32 vectorF32) {
	startTagType(xmlBuddy, tagType);
	addAttrTypeDouble(xmlBuddy, ATTR_X, vectorF32.x);
	addAttrTypeDouble(xmlBuddy, ATTR_Y, vectorF32.y);
	addAttrTypeDouble(xmlBuddy, ATTR_Z, vectorF32.z);
	endTag(xmlBuddy);
}

void writeVectorI16(XMLBuddy *xmlBuddy, enum TAG_TYPE tagType, VectorI16 vectorI16) {
	startTagType(xmlBuddy, tagType);
	addAttrTypeDouble(xmlBuddy, ATTR_X, vectorI16.x);
	addAttrTypeDouble(xmlBuddy, ATTR_Y, vectorI16.y);
	addAttrTypeDouble(xmlBuddy, ATTR_Z, vectorI16.z);
	endTag(xmlBuddy);
}

void writeAnimSeesawType(XMLBuddy *xmlBuddy, uint16_t seesawType) {
	startTagType(xmlBuddy, TAG_ANIM_SEESAW_TYPE);
	switch (seesawType) {
	case LOOPING_ANIMATION:
		addValStr(xmlBuddy, "LOOPING_ANIMATION");
		break;
	case PLAY_ONCE_ANIMATION:
		addValStr(xmlBuddy, "PLAY_ONCE_ANIMATION");
		break;
	case SEESAW:
		addValStr(xmlBuddy, "SEESAW");
		break;
	}
	endTag(xmlBuddy);
}

void writeAnimEasingVal(XMLBuddy *xmlBuddy, uint32_t easingVal) {
	
	switch (easingVal) {
	case CONSTANT:
		addAttrTypeStr(xmlBuddy, ATTR_EASING, "CONSTANT");
		break;
	case LINEAR:
		addAttrTypeStr(xmlBuddy, ATTR_EASING, "LINEAR");
		break;
	case EASED:
		addAttrTypeStr(xmlBuddy, ATTR_EASING, "EASED");
		break;
	}
}

void writeGoalType(XMLBuddy *xmlBuddy, uint16_t goalType) {
	startTagType(xmlBuddy, TAG_TYPE);
	switch (goalType) {
	case BLUE:
		addValStr(xmlBuddy, "blue");
		break;
	case GREEN:
		addValStr(xmlBuddy, "green");
		break;
	case RED:
		addValStr(xmlBuddy, "red");
		break;
	}
	endTag(xmlBuddy);
}

void writeBananaType(XMLBuddy *xmlBuddy, uint32_t bananaType) {
	startTagType(xmlBuddy, TAG_TYPE);
	switch (bananaType) {
	case SINGLE:
		addValStr(xmlBuddy, "SINGLE");
		break;
	case BUNCH:
		addValStr(xmlBuddy, "BUNCH");
		break;
	}
	endTag(xmlBuddy);
}

void writeFogType(XMLBuddy *xmlBuddy, uint8_t fogType) {
	startTagType(xmlBuddy, TAG_TYPE);
	switch (fogType) {
	case GX_FOG_NONE:
		addValStr(xmlBuddy, "GX_FOG_NONE");
		break;
	case GX_FOG_LIN:
		addValStr(xmlBuddy, "GX_FOG_LIN");
		break;
	case GX_FOG_EXP:
		addValStr(xmlBuddy, "GX_FOG_EXP");
		break;
	case GX_FOG_EXP2:
		addValStr(xmlBuddy, "GX_FOG_EXP2");
		break;
	case GX_FOG_REVEXP:
		addValStr(xmlBuddy, "GX_FOG_REVEXP");
		break;
	case GX_FOG_REVEXP2:
		addValStr(xmlBuddy, "GX_FOG_REVEXP2");
		break;
	}
	endTag(xmlBuddy);
}

static int printTagName(XMLBuddy *xmlBuddy, enum TAG_TYPE tagType) {
	switch (tagType) {
	case TAG_TITLE:
		fputs("superMonkeyBallStage", xmlBuddy->output);
		break;
	case TAG_MODEL_IMPORT:
		fputs("modelImport", xmlBuddy->output);
		break;
	case TAG_START:
		fputs("start", xmlBuddy->output);
		break;
	case TAG_END:
		fputs("end", xmlBuddy->output);
		break;
	case TAG_NAME:
		fputs("name", xmlBuddy->output);
		break;
	case TAG_POSITION:
		fputs("position", xmlBuddy->output);
		break;
	case TAG_ROTATION:
		fputs("rotation", xmlBuddy->output);
		break;
	case TAG_SCALE:
		fputs("scale", xmlBuddy->output);
		break;
	case TAG_BACKGROUND_MODEL:
		fputs("backgroundModel", xmlBuddy->output);
		break;
	case TAG_FOG:
		fputs("fog", xmlBuddy->output);
		break;
	case TAG_RED:
		fputs("red", xmlBuddy->output);
		break;
	case TAG_GREEN:
		fputs("green", xmlBuddy->output);
		break;
	case TAG_BLUE:
		fputs("blue", xmlBuddy->output);
		break;
	case TAG_FALLOUT_PLANE:
		fputs("falloutPlane", xmlBuddy->output);
		break;
	case TAG_ITEM_GROUP:
		fputs("itemGroup", xmlBuddy->output);
		break;
	case TAG_ROTATION_CENTER:
		fputs("rotationCenter", xmlBuddy->output);
		break;
	case TAG_INITIAL_ROTATION:
		fputs("initialRotation", xmlBuddy->output);
		break;
	case TAG_ANIM_SEESAW_TYPE:
		fputs("animSeesawType", xmlBuddy->output);
		break;
	case TAG_SEESAW_SENSITIVITY:
		fputs("seesawSensitivity", xmlBuddy->output);
		break;
	case TAG_SEESAW_STIFFNESS:
		fputs("seesawResetStiffness", xmlBuddy->output);
		break;
	case TAG_SEESAW_BOUNDS:
		fputs("seesawRotationBoundss", xmlBuddy->output);
		break;
	case TAG_CONVEYOR_SPEED:
		fputs("conveyorSpeed", xmlBuddy->output);
		break;
	case TAG_COLLISION_GRID:
		fputs("collisionGrid", xmlBuddy->output);
		break;
	case TAG_STEP:
		fputs("step", xmlBuddy->output);
		break;
	case TAG_COUNT:
		fputs("count", xmlBuddy->output);
		break;
	case TAG_COLLISION:
		fputs("collision", xmlBuddy->output);
		break;
	case TAG_OBJECT:
		fputs("object", xmlBuddy->output);
		break;
	case TAG_GOAL:
		fputs("goal", xmlBuddy->output);
		break;
	case TAG_TYPE:
		fputs("type", xmlBuddy->output);
		break;
	case TAG_BUMPER:
		fputs("bumper", xmlBuddy->output);
		break;
	case TAG_JAMABAR:
		fputs("jamabar", xmlBuddy->output);
		break;
	case TAG_BANANA:
		fputs("banana", xmlBuddy->output);
		break;
	case TAG_CONE:
		fputs("cone", xmlBuddy->output);
		break;
	case TAG_SPHERE:
		fputs("sphere", xmlBuddy->output);
		break;
	case TAG_CYLINDER:
		fputs("cylinder", xmlBuddy->output);
		break;
	case TAG_FALLOUT_VOLUME:
		fputs("falloutVolume", xmlBuddy->output);
	case TAG_LEVEL_MODEL:
		fputs("levelModel", xmlBuddy->output);
		break;
	case TAG_REFLECTIVE_MODEL:
		fputs("reflectiveModel", xmlBuddy->output);
		break;
	case TAG_WORMHOLE:
		fputs("wormhole", xmlBuddy->output);
		break;
	case TAG_SWITCH:
		fputs("switch", xmlBuddy->output);
		break;
	case TAG_DESTINATION_NAME:
		fputs("destinationName", xmlBuddy->output);
		break;
	case TAG_ANIM_LOOP_TIME:
		fputs("animLoopTime", xmlBuddy->output);
		break;
	case TAG_ANIM_KEYFRAMES:
		fputs("animKeyframes", xmlBuddy->output);
		break;
	case TAG_POS_X:
		fputs("posX", xmlBuddy->output);
		break;
	case TAG_POS_Y:
		fputs("posY", xmlBuddy->output);
		break;
	case TAG_POS_Z:
		fputs("posZ", xmlBuddy->output);
		break;
	case TAG_ROT_X:
		fputs("rotX", xmlBuddy->output);
		break;
	case TAG_ROT_Y:
		fputs("rotY", xmlBuddy->output);
		break;
	case TAG_ROT_Z:
		fputs("rotZ", xmlBuddy->output);
		break;
	case TAG_KEYFRAME:
		fputs("keyframe", xmlBuddy->output);
		break;
	case TAG_ANIM_GROUP_ID:
		fputs("animGroupId", xmlBuddy->output);
		break;
	case TAG_ANIM_INITIAL_STATE:
		fputs("animInitialState", xmlBuddy->output);
		break;
	default:
		fputs("Invalid", xmlBuddy->output);
	}
	return NO_ERROR;
}

static int printAttrName(XMLBuddy *xmlBuddy, enum ATTRIBUTE_TYPE attrType) {
	switch (attrType) {
	case ATTR_VERSION:
		fputs("version=", xmlBuddy->output);
		break;
	case ATTR_TYPE:
		fputs("type=", xmlBuddy->output);
		break;
	case ATTR_X:
		fputs("x=", xmlBuddy->output);
		break;
	case ATTR_Y:
		fputs("y=", xmlBuddy->output);
		break;
	case ATTR_Z:
		fputs("z=", xmlBuddy->output);
		break;
	case ATTR_TIME:
		fputs("time=", xmlBuddy->output);
		break;
	case ATTR_VALUE:
		fputs("value=", xmlBuddy->output);
		break;
	case ATTR_EASING:
		fputs("easing=", xmlBuddy->output);
		break;
	default:
		fputs("Invalid=", xmlBuddy->output);
	}
	return NO_ERROR;
}