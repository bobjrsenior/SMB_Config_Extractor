#include "configExtractor.h"

#include <stdint.h>

#include "FunctionsAndDefines.h"
#include "xmlbuddy.h"

typedef struct CollisionGroup {
	int x;
}CollisionGroup;

typedef struct {
	uint32_t number;
	uint32_t offset;
}ConfigObject;

typedef struct {
	uint32_t triangleListOffset;
	uint32_t gridTriangleListOffet;
	float gridStartX;
	float gridStartZ;
	float gridStepX;
	float gridStepZ;
	uint32_t gridStepXCount;
	uint32_t gridStepZCount;
}CollisionGroupHeader;

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


// Constants (Some may move into XMLBuddy enum itself)
enum Seesaw_Type {
	LOOPING_ANIMATION = 0x0000,
	PLAY_ONCE_ANIMATION = 0x0001,
	SEESAW = 0x0002
};

enum EASING {
	CONSTANT = 0x00000000,
	LINEAR = 0x00000001,
	EASED = 0x00000002
};

enum GOAL_TYPE {
	BLUE = 0x0001,
	GREEN = 0x0101,
	RED = 0x0201
};

enum BANANA_TYPE {
	SINGLE = 0x00000000,
	BUNCH = 0x00000001,
};

// File Reading Functions (Endianness handling)
static uint32_t(*readInt)(FILE*);
static uint32_t(*readIntRev)(FILE*);
static uint16_t(*readShort)(FILE*);
static uint16_t(*readShortRev)(FILE*);

// Config Helper Functions
static ConfigObject readItem(FILE *input);
static VectorF32 readVectorF32(FILE *input);
static VectorI16 readVectorI16(FILE *input, int eatPadding);
static VectorF32 convertRot16ToF32(VectorI16 rotOriginal);
static CollisionGroupHeader readCollisionGroupHeader(FILE *input);

// XML Buddy Helper Functions
static void writeVectorF32(XMLBuddy *xmlBuddy, enum Tag_Type tagType, VectorF32 vectorF32);
static void writeVectorI16(XMLBuddy *xmlBuddy, enum Tag_Type tagType, VectorI16 vectorI16);
static void writeAnimSeesawType(XMLBuddy *xmlBuddy, uint16_t seesawType);
static void writeAsciiName(FILE *input, XMLBuddy *xmlBuddy, uint32_t nameOffset);

// Config Parser Functions
static void copyCollisionFields(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item);
static void copyFieldAnimationType(FILE *input, XMLBuddy *xmlBuddy, enum Tag_Type tagType, ConfigObject animData);
static void copyFieldAnimation(FILE *input, XMLBuddy *xmlBuddy, uint32_t animHeaderOffset);
static void copyCollisionGroup(FILE *input, XMLBuddy *xmlBuddy, CollisionGroupHeader item);
static void copyGoals(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item);
static void copyBumpers(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item);
static void copyJamabars(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item);
static void copyBananas(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item);
static void copyCones(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item);
static void copySpheres(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item);
static void copyCylinders(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item);
static void copyFalloutVolumes(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item);
static void copyReflectiveModels(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item);
static void copyLevelModelInstances(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item);
static void copyLevelModelBs(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item);

void extractConfig(char *filename, int gameVersion) {
	if (gameVersion != SMB2 && gameVersion != SMBX) {
		return;
	}
	FILE *input = fopen(filename, "rb");

	// Make the output file name
	char outfileName[512];
	sscanf(filename, "%507s", outfileName);
	{
		int nameLength = (int)strlen(outfileName);
		outfileName[nameLength++] = '.';
		outfileName[nameLength++] = 'x';
		outfileName[nameLength++] = 'm';
		outfileName[nameLength++] = 'l';
		outfileName[nameLength++] = '\0';
	}

	XMLBuddy xmlBuddyObj;
	XMLBuddy *xmlBuddy = initXMLBuddy(&outfileName[0], &xmlBuddyObj, 0);

	// Init read functions (SMB2 is big endian, SMBX is little endian)
	if (gameVersion == SMB2) {
		readInt = &readBigInt;
		readIntRev = &readLittleInt;
		readShort = &readBigShort;
		readShortRev = &readLittleShort;
	}
	else if (gameVersion == SMBX) {
		readInt = &readLittleInt;
		readIntRev = &readBigInt;
		readShort = &readLittleShort;
		readShortRev = &readBigShort;
	}

	ConfigObject collisionFields;
	ConfigObject startPositions;

	// Start the initial XML header
	startTagType(xmlBuddy, TAG_TITLE);
	addAttrTypeStr(xmlBuddy, ATTR_VERSION, "1.0.0");
	
	// Seek to collision headers
	fseek(input, 0x8, SEEK_SET);
	//                                                                 Offset   Size   Description
	collisionFields = readItem(input);                              // 0x8,    0x8    Collision Header
	startPositions.offset = readInt(input);                         // 0x10    0x4    Offset to start position
	uint32_t falloutPlaneOffset = readInt(input);                   // 0x14    0x4    Offset to fallout plane

	// The number of start positions is the fallout Y offset - startPosition offset / sizeof(startPosition)
	startPositions.number = (falloutPlaneOffset - startPositions.offset) / 0x14;

	// Skip most other stuff here for now
	// A lot of it isn't needed (since it is required in collision fields anyways
	// Backgrounds (and a bit more) will need to be covered though
	copyCollisionFields(input, xmlBuddy, collisionFields);


	endTag(xmlBuddy);
	fclose(input);
	closeXMlBuddy(xmlBuddy);
}


static void copyCollisionFields(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item) {
	if (item.number == 0 || item.offset == 0) return;
	long savePos = ftell(input);
	fseek(input, item.offset, SEEK_SET);

	for (uint32_t i = 0; i < item.number; i++) {
		startTagType(xmlBuddy, TAG_ITEM_GROUP);

		CollisionGroupHeader colGroupHeader;
		//                                                                 Offset   Size   Description
		VectorF32 centerOfRotation = readVectorF32(input);              // 0x0      0xC    Center of Rotation (X, Y, Z)
		writeVectorF32(xmlBuddy, TAG_ROTATION_CENTER, centerOfRotation);
		VectorI16 initialRotation = readVectorI16(input, 0);            // 0xC      0x6    Initial Rotation (X, Y, Z)
		writeVectorI16(xmlBuddy, TAG_INITIAL_ROTATION, initialRotation);
		uint16_t animSeesawType = readShort(input);                     // 0x12     0x2    Animation Seesaw Type
		writeAnimSeesawType(xmlBuddy, animSeesawType);
		uint32_t animHeaderOffset = readInt(input);                     // 0x14     0x4    Animation Header Offset
		copyFieldAnimation(input, xmlBuddy, animHeaderOffset);
		VectorF32 conveyorSpeed = readVectorF32(input);                 // 0x18     0xC    Conveyor Speed (X, Y, Z)
		writeVectorF32(xmlBuddy, TAG_CONVEYOR_SPEED, conveyorSpeed);
		colGroupHeader = readCollisionGroupHeader(input);               // 0x24     0x20   Collision Group Data
		copyCollisionGroup(input, xmlBuddy, colGroupHeader);
		ConfigObject goals = readItem(input);                           // 0x44     0x8    Goal number/offset
		copyGoals(input, xmlBuddy, goals);
		ConfigObject bumpers = readItem(input);                         // 0x4C     0x8    Bumper number/offset
		copyBumpers(input, xmlBuddy, bumpers);
		ConfigObject jamabars = readItem(input);                        // 0x54     0x8    Jamabar number/offset
		copyJamabars(input, xmlBuddy, jamabars);
		ConfigObject bananas = readItem(input);                         // 0x5C     0x8    Bananas number/offset
		copyBananas(input, xmlBuddy, bananas);
		ConfigObject cones = readItem(input);                           // 0x64     0x8    cones number/offset
		copyCones(input, xmlBuddy, cones);
		ConfigObject spheres = readItem(input);                         // 0x6C     0x8    Spheres number/offset
		copySpheres(input, xmlBuddy, spheres);
		ConfigObject cylinders = readItem(input);                       // 0x74     0x8    Cylinders number/offset
		copyCylinders(input, xmlBuddy, cylinders);

		// Amount to skip: Header size - curPosition in header
		// TODO UPDATE THIS WHEN UPDATING ABOVE
		fseek(input, 0x49C - 0x4C, SEEK_CUR);

		endTag(xmlBuddy);
	}
	fseek(input, savePos, SEEK_SET);
}

static void copyFieldAnimation(FILE *input, XMLBuddy *xmlBuddy, uint32_t animHeaderOffset) {
	if (animHeaderOffset == 0) return;
	long savePos = ftell(input);
	fseek(input, animHeaderOffset, SEEK_SET);

	//                                                                 Offset   Size   Description
	ConfigObject rotX = readItem(input);                            // 0x0      0x8    Rotation X Anim Data (Number, offset)
	ConfigObject rotY = readItem(input);                            // 0x8      0x8    Rotation Y Anim Data (Number, offset)
	ConfigObject rotZ = readItem(input);                            // 0x10     0x8    Rotation Z Anim Data (Number, offset)
	ConfigObject posX = readItem(input);                            // 0x18     0x8    Translation X Anim Data (Number, offset)
	ConfigObject posY = readItem(input);                            // 0x20     0x8    Translation Y Anim Data (Number, offset)
	ConfigObject posZ = readItem(input);                            // 0x28     0x8    Translation Z Anim Data (Number, offset)

	copyFieldAnimationType(input, xmlBuddy, TAG_ROT_X, rotX);
	copyFieldAnimationType(input, xmlBuddy, TAG_ROT_Y, rotY);
	copyFieldAnimationType(input, xmlBuddy, TAG_ROT_Z, rotZ);
	copyFieldAnimationType(input, xmlBuddy, TAG_POS_X, posX);
	copyFieldAnimationType(input, xmlBuddy, TAG_POS_Y, posY);
	copyFieldAnimationType(input, xmlBuddy, TAG_POS_Z, posZ);

	fseek(input, savePos, SEEK_SET);
}

static void copyFieldAnimationType(FILE *input, XMLBuddy *xmlBuddy, enum Tag_Type tagType, ConfigObject animData) {
	if (animData.number == 0) return;
	long savePos = ftell(input);
	fseek(input, animData.offset, SEEK_SET);
	startTagType(xmlBuddy, tagType);

	for (uint32_t i = 0; i < animData.number; i++) {
		//                                                                 Offset   Size   Description
		uint32_t easing = readInt(input);                               // 0x0      0x4    Easing
		float time = readFloat(input);                                  // 0x4      0x4    Time (Seconds)
		float value = readFloat(input);                                 // 0x8      0x4    Value (Amount: pos, rot, R/G/B, ect)

		startTagType(xmlBuddy, TAG_KEYFRAME);
		addAttrTypeDouble(xmlBuddy, ATTR_TIME, time);
		addAttrTypeDouble(xmlBuddy, ATTR_VALUE, value);
		switch(easing) {
		case CONSTANT:
			addValStr(xmlBuddy, "CONSTANT");
			break;
		case LINEAR:
			addValStr(xmlBuddy, "LINEAR");
			break;
		case EASED:
			addValStr(xmlBuddy, "EASED");
			break;
		}

		endTag(xmlBuddy);
	}

	endTag(xmlBuddy);
	fseek(input, savePos, SEEK_SET);
}

static void copyCollisionGroup(FILE *input, XMLBuddy *xmlBuddy, CollisionGroupHeader item) {
	// TODO Possibly look at collision data
	// and move this into separate write function)
	startTagType(xmlBuddy, TAG_COLLISION_GRID);

	startTagType(xmlBuddy, TAG_START);
	addAttrTypeDouble(xmlBuddy, ATTR_X, item.gridStartX);
	addAttrTypeDouble(xmlBuddy, ATTR_Z, item.gridStartZ);
	endTag(xmlBuddy);

	startTagType(xmlBuddy, TAG_STEP);
	addAttrTypeDouble(xmlBuddy, ATTR_X, item.gridStepX);
	addAttrTypeDouble(xmlBuddy, ATTR_Z, item.gridStepZ);
	endTag(xmlBuddy);

	startTagType(xmlBuddy, TAG_COUNT);
	addAttrTypeDouble(xmlBuddy, ATTR_X, item.gridStepXCount);
	addAttrTypeDouble(xmlBuddy, ATTR_Z, item.gridStepZCount);
	endTag(xmlBuddy);

	endTag(xmlBuddy);
}

static void copyGoals(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item) {
	if (item.number == 0 || item.offset == 0) return;
	long savePos = ftell(input);
	fseek(input, item.offset, SEEK_SET);

	for (uint32_t i = 0; i < item.number; i++) {
		startTagType(xmlBuddy, TAG_GOAL);
		//                                                                 Offset   Size   Description
		VectorF32 position = readVectorF32(input);                      // 0x0      0xC    Position (X, Y, Z)
		writeVectorF32(xmlBuddy, TAG_POSITION, position);
		VectorI16 rotOriginal = readVectorI16(input, 0);                // 0xC      0x6    Rotation (X, Y, Z)
		VectorF32 rotation = convertRot16ToF32(rotOriginal);
		writeVectorF32(xmlBuddy, TAG_ROTATION, rotation);
		uint16_t goalType = readShort(input);                          // 0x12     0x2     Goal Type
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
		endTag(xmlBuddy); // Goal Type

		endTag(xmlBuddy); // Goal
	}
	fseek(input, savePos, SEEK_SET);
}

static void copyBumpers(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item) {
	if (item.number == 0 || item.offset == 0) return;
	long savePos = ftell(input);
	fseek(input, item.offset, SEEK_SET);

	for (uint32_t i = 0; i < item.number; i++) {
		startTagType(xmlBuddy, TAG_BUMPER);
		//                                                                 Offset   Size   Description
		VectorF32 position = readVectorF32(input);                      // 0x0      0xC    Position (X, Y, Z)
		writeVectorF32(xmlBuddy, TAG_POSITION, position);
		VectorI16 rotOriginal = readVectorI16(input, 1);                // 0xC      0x8    Rotation (X, Y, Z, Pad)
		VectorF32 rotation = convertRot16ToF32(rotOriginal);
		writeVectorF32(xmlBuddy, TAG_ROTATION, rotation);
		VectorF32 scale = readVectorF32(input);                         // 0x14    0xC     Scale (X, Y, Z)
		writeVectorF32(xmlBuddy, TAG_SCALE, scale);

		endTag(xmlBuddy);
	}
	fseek(input, savePos, SEEK_SET);
}

static void copyJamabars(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item) {
	if (item.number == 0 || item.offset == 0) return;
	long savePos = ftell(input);
	fseek(input, item.offset, SEEK_SET);

	for (uint32_t i = 0; i < item.number; i++) {
		startTagType(xmlBuddy, TAG_JAMABAR);
		//                                                                 Offset   Size   Description
		VectorF32 position = readVectorF32(input);                      // 0x0      0xC    Position (X, Y, Z)
		writeVectorF32(xmlBuddy, TAG_POSITION, position);
		VectorI16 rotOriginal = readVectorI16(input, 1);                // 0xC      0x8    Rotation (X, Y, Z, Pad)
		VectorF32 rotation = convertRot16ToF32(rotOriginal);
		writeVectorF32(xmlBuddy, TAG_ROTATION, rotation);
		VectorF32 scale = readVectorF32(input);                         // 0x14    0xC     Scale (X, Y, Z)
		writeVectorF32(xmlBuddy, TAG_SCALE, scale);

		endTag(xmlBuddy);
	}
	fseek(input, savePos, SEEK_SET);
}

static void copyBananas(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item) {
	if (item.number == 0 || item.offset == 0) return;
	long savePos = ftell(input);
	fseek(input, item.offset, SEEK_SET);

	for (uint32_t i = 0; i < item.number; i++) {
		startTagType(xmlBuddy, TAG_BANANA);
		//                                                                 Offset   Size   Description
		VectorF32 position = readVectorF32(input);                      // 0x0      0xC    Position (X, Y, Z)
		writeVectorF32(xmlBuddy, TAG_POSITION, position);
		uint32_t bananaType = readInt(input);                           // 0xC      0x4    Banana Type
		startTagType(xmlBuddy, TAG_TYPE);
		switch (bananaType) {
		case SINGLE:
			addValStr(xmlBuddy, "SINGLE");
			break;
		case BUNCH:
			addValStr(xmlBuddy, "BUNCH");
			break;
		}
		endTag(xmlBuddy); // Banana Type

		endTag(xmlBuddy); // Banana
	}
	fseek(input, savePos, SEEK_SET);
}

static void copyCones(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item) {
	if (item.number == 0 || item.offset == 0) return;
	long savePos = ftell(input);
	fseek(input, item.offset, SEEK_SET);

	for (uint32_t i = 0; i < item.number; i++) {
		startTagType(xmlBuddy, TAG_CONE);
		//                                                                 Offset   Size   Description
		VectorF32 position = readVectorF32(input);                      // 0x0      0xC    Position (X, Y, Z)
		writeVectorF32(xmlBuddy, TAG_POSITION, position);
		VectorI16 rotOriginal = readVectorI16(input, 1);                // 0xC      0x8    Rotation (X, Y, Z, Pad)
		VectorF32 rotation = convertRot16ToF32(rotOriginal);
		writeVectorF32(xmlBuddy, TAG_ROTATION, rotation);
		// TODO Cone Radius/Height/Radius (How represent in xml)
		// TO For implementing sphere and cylinder as well
		fseek(input, 0x12, SEEK_CUR);
		endTag(xmlBuddy);
	}
	fseek(input, savePos, SEEK_SET);
}

static void copySpheres(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item) {
	if (item.number == 0 || item.offset == 0) return;
	long savePos = ftell(input);
	fseek(input, item.offset, SEEK_SET);

	for (uint32_t i = 0; i < item.number; i++) {
		startTagType(xmlBuddy, TAG_SPHERE);
		//                                                                 Offset   Size   Description
		VectorF32 position = readVectorF32(input);                      // 0x0      0xC    Position (X, Y, Z)
		writeVectorF32(xmlBuddy, TAG_POSITION, position);
		// TODO Cone Radius/Height/Radius (How represent in xml)
		// TO For implementing sphere and cylinder as well
		fseek(input, 0x8, SEEK_CUR);
		endTag(xmlBuddy);
	}
	fseek(input, savePos, SEEK_SET);
}

static void copyCylinders(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item) {
	if (item.number == 0 || item.offset == 0) return;
	long savePos = ftell(input);
	fseek(input, item.offset, SEEK_SET);

	for (uint32_t i = 0; i < item.number; i++) {
		startTagType(xmlBuddy, TAG_CYLINDER);
		//                                                                 Offset   Size   Description
		VectorF32 position = readVectorF32(input);                      // 0x0      0xC    Position (X, Y, Z)
		writeVectorF32(xmlBuddy, TAG_POSITION, position);
		// TODO Cone Radius/Height/Radius (How represent in xml)
		// TO For implementing sphere and cylinder as well
		fseek(input, 8, SEEK_CUR);
		VectorI16 rotOriginal = readVectorI16(input, 1);                // 0xC      0x8    Rotation (X, Y, Z, Pad)
		VectorF32 rotation = convertRot16ToF32(rotOriginal);
		writeVectorF32(xmlBuddy, TAG_ROTATION, rotation);
		endTag(xmlBuddy);
	}
	fseek(input, savePos, SEEK_SET);
}

static void copyFalloutVolumes(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item) {
	if (item.number == 0 || item.offset == 0) return;
	long savePos = ftell(input);
	fseek(input, item.offset, SEEK_SET);

	for (uint32_t i = 0; i < item.number; i++) {
		startTagType(xmlBuddy, TAG_FALLOUT_VOLUME);
		//                                                                 Offset   Size   Description
		VectorF32 position = readVectorF32(input);                      // 0x0      0xC    Position (X, Y, Z)
		writeVectorF32(xmlBuddy, TAG_POSITION, position);
		VectorF32 scale = readVectorF32(input);                         // 0xC      0xC    Scale (X, Y, Z)
		writeVectorF32(xmlBuddy, TAG_SCALE, scale);
		VectorI16 rotOriginal = readVectorI16(input, 1);                // 0x18     0x8    Rotation (X, Y, Z, Pad)
		VectorF32 rotation = convertRot16ToF32(rotOriginal);
		writeVectorF32(xmlBuddy, TAG_ROTATION, rotation);

		endTag(xmlBuddy);
	}
	fseek(input, savePos, SEEK_SET);
}

static void copyReflectiveModels(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item) {
	if (item.number == 0 || item.offset == 0) return;
	long savePos = ftell(input);
	fseek(input, item.offset, SEEK_SET);

	for (uint32_t i = 0; i < item.number; i++) {
		startTagType(xmlBuddy, TAG_REFLECTIVE_MODEL);
		//                                                                 Offset   Size   Description
		uint32_t nameOffset = readInt(input);                           // 0x0      0x4    Name offset
		writeAsciiName(input, xmlBuddy, nameOffset);
		fseek(input, 0x4, SEEK_CUR);                                    // 0x4      0x4    Null
		endTag(xmlBuddy);
	}
	fseek(input, savePos, SEEK_SET);
}

static ConfigObject readItem(FILE *input) {
	ConfigObject configObject;
	configObject.number = readInt(input);
	configObject.offset = readInt(input);
	return configObject;
}

static VectorF32 readVectorF32(FILE *input) {
	VectorF32 vector32;
	vector32.x = readFloat(input);
	vector32.y = readFloat(input);
	vector32.y = readFloat(input);
	return vector32;
}

static VectorI16 readVectorI16(FILE *input, int eatPadding) {
	VectorI16 vector16;
	vector16.x = readShort(input);
	vector16.y = readShort(input);
	vector16.y = readShort(input);
	if (eatPadding) readShort(input);
	return vector16;
}

static VectorF32 convertRot16ToF32(VectorI16 rotOriginal) {
	const double conversionFactor = 65536.0 / 360.0;
	VectorF32 rotation;
	rotation.x = (float)(conversionFactor * rotOriginal.x);
	rotation.y = (float)(conversionFactor * rotOriginal.y);
	rotation.z = (float)(conversionFactor * rotOriginal.z);
	return rotation;
}

static CollisionGroupHeader readCollisionGroupHeader(FILE *input) {
	CollisionGroupHeader colGroupHeader;
	//                                                                 Offset   Size   Description
	colGroupHeader.triangleListOffset = readInt(input);             // 0x0      0x4    Offset to the Collision Triangle List
	colGroupHeader.gridTriangleListOffet = readInt(input);          // 0x4      0x4    Offset to the Grid Triangle List Pointers
	colGroupHeader.gridStartX = readFloat(input);                   // 0x8      0x4    Grid Start X
	colGroupHeader.gridStartZ = readFloat(input);                   // 0xC      0x4    Grid Start Z
	colGroupHeader.gridStepX = readFloat(input);                    // 0x10     0x4    Grid Step X
	colGroupHeader.gridStepZ = readFloat(input);                    // 0x14     0x4    Grid Step Z
	colGroupHeader.gridStepXCount = readInt(input);                 // 0x18     0x4    Grid X Step Count
	colGroupHeader.gridStepZCount = readInt(input);                 // 0x1C     0x4    Grid Z Steo Count
	return colGroupHeader;
}

static void writeVectorF32(XMLBuddy *xmlBuddy, enum Tag_Type tagType, VectorF32 vectorF32) {
	startTagType(xmlBuddy, tagType);
	addAttrTypeDouble(xmlBuddy, ATTR_X, vectorF32.x);
	addAttrTypeDouble(xmlBuddy, ATTR_Y, vectorF32.y);
	addAttrTypeDouble(xmlBuddy, ATTR_Z, vectorF32.z);
	endTag(xmlBuddy);
}

static void writeVectorI16(XMLBuddy *xmlBuddy, enum Tag_Type tagType, VectorI16 vectorI16) {
	startTagType(xmlBuddy, tagType);
	addAttrTypeDouble(xmlBuddy, ATTR_X, vectorI16.x);
	addAttrTypeDouble(xmlBuddy, ATTR_Y, vectorI16.y);
	addAttrTypeDouble(xmlBuddy, ATTR_Z, vectorI16.z);
	endTag(xmlBuddy);
}

static void writeAnimSeesawType(XMLBuddy *xmlBuddy, uint16_t seesawType) {
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

static void writeAsciiName(FILE *input, XMLBuddy *xmlBuddy, uint32_t nameOffset) {
	if (nameOffset == 0) return;
	long savePos = ftell(input);
	fseek(input, nameOffset, SEEK_SET);
	char nameBuff[256] = { 0 };

	int index = 0;
	while (1) {
		uint8_t c = fgetc(input);
		if (feof(input) || c == 0 || index >= 255) break;
		nameBuff[index++] = c;
	}
	nameBuff[index] = '\0';

	addValStr(xmlBuddy, nameBuff);
	fseek(input, savePos, SEEK_SET);
}