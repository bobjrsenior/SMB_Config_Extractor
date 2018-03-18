#include "configExtractor.h"

#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "FunctionsAndDefines.h"
#include "xmlbuddy.h"

#define MAX_NUM_WORMHOLES 256

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
	struct {
		float x1;
		float y1;
		float z1;
	};
	struct {
		float x2;
		float y2;
		float z2;
	};
	struct {
		float x3;
		float y3;
		float z3;
	};
}Mat3;

// File Reading Functions (Endianness handling)
static uint32_t(*readInt)(FILE*);
static uint32_t(*readIntRev)(FILE*);
static uint16_t(*readShort)(FILE*);
static uint16_t(*readShortRev)(FILE*);
static float(*readFloat)(FILE*);
static float(*readFloatRev)(FILE*);

// Config Helper Functions
static ConfigObject readItem(FILE *input);
static VectorF32 readVectorF32(FILE *input);
static VectorI16 readVectorI16(FILE *input, int eatPadding);
static VectorF32 convertRot16ToF32(VectorI16 rotOriginal);
static CollisionGroupHeader readCollisionGroupHeader(FILE *input);
static int getWormholeIndex(uint32_t offset);

// Obj Helper Functions
static uint16_t calculateNumTriangles(FILE *input, CollisionGroupHeader colGroupHeader);
static void initObjoutput();
static void writeObjOutput(FILE *input, CollisionGroupHeader colGroupHeader, uint16_t numTris);
static void writeTriVertex(VectorF32 vertex);
static void writeTriNormal(VectorF32 normal);
static void writeLastTriFace();

// XML Buddy Helper Functions
static void writeAsciiName(FILE *input, XMLBuddy *xmlBuddy, uint32_t nameOffset);

// Config Parser Functions
static void copyStartPositions(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item);
static void copyFalloutPlane(FILE *input, XMLBuddy *xmlBuddy, uint32_t offset);
static void copyBackgroundModels(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item);
static void copyBackgroundAnimationOne(FILE *input, XMLBuddy *xmlBuddy, uint32_t animOffset);
static void copyFog(FILE *input, XMLBuddy *xmlBuddy, uint32_t fogOffset, uint32_t fogAnimOffset);
static void copyFogAnimation(FILE *input, XMLBuddy *xmlBuddy, uint32_t fogAnimOffset);
static void copyCollisionFields(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item);
static void copyFieldAnimationType(FILE *input, XMLBuddy *xmlBuddy, enum TAG_TYPE tagType, ConfigObject animData);
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
static void copySwitches(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item);
static void copyWormholes(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item);

static uint32_t wormHoleOffsets[MAX_NUM_WORMHOLES] = { 0 };
static int wormholeCount = 0;
static uint32_t curVertexCount = 1;
static uint32_t curNormalCount = 1;

// Matrix functions
#define PI_F 3.141592653589793238462f

static float degreeToRadian(float degrees);

static Mat3 makeRotationMatrixX(float degrees);
static Mat3 makeRotationMatrixY(float degrees);
static Mat3 makeRotationMatrixZ(float degrees);

static VectorF32 multiplyMat3ByVec3(Mat3 matrix, VectorF32 vector);

static char objFilename[255] = { 0 };
static FILE *objOutput = NULL;

void extractConfig(char *filename, int game) {
	if (game != SMB2 && game != SMBX) {
		return;
	}
	FILE *input = fopen(filename, "rb");
	if (input == NULL) {
		perror("COuldn't Open File");
		return;
	}

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
	wormholeCount = 0;
	curVertexCount = 1;
	curNormalCount = 1;

	// Init read functions (SMB2 is big endian, SMBX is little endian)
	if (game == SMB2) {
		readInt = &readBigInt;
		readIntRev = &readLittleInt;
		readShort = &readBigShort;
		readShortRev = &readLittleShort;
		readFloat = &readBigFloat;
		readFloatRev = &readLittleFloat;
	}
	else if (game == SMBX) {
		readInt = &readLittleInt;
		readIntRev = &readBigInt;
		readShort = &readLittleShort;
		readShortRev = &readBigShort;
		readFloat = &readLittleFloat;
		readFloat = &readBigFloat;
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
	fseek(input, 0x58, SEEK_SET);                                   // 0x0     0x58   Seek to background models (From beginning to avoid seeking errors)
	ConfigObject backgroundModels = readItem(input);                // 0x58    0x8    Bakground Models number/offset
	fseek(input, 0xB0, SEEK_SET);                                   // 0x0     0xB0   Seek to fog animation Header (From beginning to avoid seeking errors)
	uint32_t fogAnimationOffset = readInt(input);                   // 0xB0    0x4    Fog Animation Header Offset
	fseek(input, 0xBC, SEEK_SET);                                   // 0x0     0xBC   Seek to fog offset (From beginning to avoid seeking errors)
	uint32_t fogOffset = readInt(input);                            // 0xBC    0x4    Fog offset

	// The number of start positions is the fallout Y offset - startPosition offset / sizeof(startPosition)
	startPositions.number = (falloutPlaneOffset - startPositions.offset) / 0x14;
	copyStartPositions(input, xmlBuddy, startPositions);
	copyFalloutPlane(input, xmlBuddy, falloutPlaneOffset);
	copyBackgroundModels(input, xmlBuddy, backgroundModels);
	copyFog(input, xmlBuddy, fogOffset, fogAnimationOffset);

	// Skip most other stuff here for now
	// A lot of it isn't needed (since it is required in collision fields anyways
	// Backgrounds (and a bit more) will need to be covered though
	copyCollisionFields(input, xmlBuddy, collisionFields);


	endTag(xmlBuddy);
	if (objOutput != NULL) {
		fclose(objOutput);
		objOutput = NULL;
	}
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
		ConfigObject cones = readItem(input);                           // 0x64     0x8    Cones number/offset
		copyCones(input, xmlBuddy, cones);
		ConfigObject spheres = readItem(input);                         // 0x6C     0x8    Spheres number/offset
		copySpheres(input, xmlBuddy, spheres);
		ConfigObject cylinders = readItem(input);                       // 0x74     0x8    Cylinders number/offset
		copyCylinders(input, xmlBuddy, cylinders);
		ConfigObject falloutVolumes = readItem(input);                  // 0x7C     0x8    Fallout Volumes number/offset
		copyFalloutVolumes(input, xmlBuddy, falloutVolumes);
		ConfigObject reflectiveModels = readItem(input);                // 0x84     0x8    Reflective models number/offset
		copyReflectiveModels(input, xmlBuddy, reflectiveModels);
		ConfigObject levelModelInstances = readItem(input);             // 0x8C     0x8    Level Model Instances number/offset
		// TODO copy Instances
		ConfigObject levelModelBs = readItem(input);                    // 0x94     0x8    Level Model B number/offset
		copyLevelModelBs(input, xmlBuddy, levelModelBs);
		fseek(input, 0x8, SEEK_CUR);                                    // 0x9C     0x8    Unknown/Null
		uint16_t animGroupID = readShort(input);                        // 0xA4     0x8    Animation Group ID
		writeTagWithUInt32Value(xmlBuddy, TAG_ANIM_GROUP_ID, animGroupID);
		fseek(input, 0x2, SEEK_CUR);                                    // 0xA6     0x2    Null
		ConfigObject switches = readItem(input);                        // 0xA8     0x8    Switches number/offset
		copySwitches(input, xmlBuddy, switches);
		fseek(input, 0x4, SEEK_CUR);                                    // 0xB0     0x4    Unknown/Null
		fseek(input, 0x4, SEEK_CUR);                                    // 0xB4     0x4    Offset to Mystery 5
		float seesawSensitivity = readFloat(input);                     // 0xB8     0x4    Seesaw Sensitivity
		writeTagWithFloatValue(xmlBuddy, TAG_SEESAW_SENSITIVITY, seesawSensitivity);
		float seesawStiffness = readFloat(input);                       // 0xBC     0x4    Seesaw Stiffness
		writeTagWithFloatValue(xmlBuddy, TAG_SEESAW_STIFFNESS, seesawStiffness);
		float seesawBounds = readFloat(input);                          // 0xC0     0x4    Seesaw Bounds
		writeTagWithFloatValue(xmlBuddy, TAG_SEESAW_BOUNDS, seesawBounds);
		ConfigObject wormholes = readItem(input);                       // 0xC4     0x8    Wormholes number/offset
		copyWormholes(input, xmlBuddy, wormholes);
		uint32_t initialAnimState = readInt(input);                     // 0xCC     0x4    Initial Animation State
		writeAnimType(xmlBuddy, TAG_ANIM_INITIAL_STATE, (uint16_t)initialAnimState);
		fseek(input, 0x4, SEEK_CUR);                                    // 0xD0     0x4    Unknown/Null
		float animationLoopPoint = readFloat(input);                    // 0xD4     0x4    Animation Loop Point
		writeTagWithFloatValue(xmlBuddy, TAG_ANIM_LOOP_TIME, animationLoopPoint);
		fseek(input, 0x4, SEEK_CUR);                                    // 0xD8     0x4    Offset to Mystery 11
		fseek(input, 0x3C0, SEEK_CUR);                                  // 0xDC     0x3C0  Unknown/Null
		endTag(xmlBuddy);
	}
	fseek(input, savePos, SEEK_SET);
}

static void copyStartPositions(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item) {
	if (item.number == 0 || item.offset == 0) return;
	long savePos = ftell(input);
	fseek(input, item.offset, SEEK_SET);

	for (uint32_t i = 0; i < item.number; i++) {
		startTagType(xmlBuddy, TAG_START);
		//                                                                 Offset   Size   Description
		VectorF32 position = readVectorF32(input);                      // 0x0      0xC    Position (X, Y, Z)
		writeVectorF32(xmlBuddy, TAG_POSITION, position);
		VectorI16 rotOriginal = readVectorI16(input, 1);                // 0xC      0x8    Rotation (X, Y, Z, Pad)
		VectorF32 rotation = convertRot16ToF32(rotOriginal);
		writeVectorF32(xmlBuddy, TAG_ROTATION, rotation);

		endTag(xmlBuddy);
	}
	fseek(input, savePos, SEEK_SET);
}

static void copyFalloutPlane(FILE *input, XMLBuddy *xmlBuddy, uint32_t offset) {
	if (offset == 0) return;
	long savePos = ftell(input);
	fseek(input, offset, SEEK_SET);
	//                                                                 Offset   Size   Description
	float falloutPlane = readFloat(input);                          // 0x0      0x4    Fallout Y position
	startTagType(xmlBuddy, TAG_FALLOUT_PLANE);
	addAttrTypeDouble(xmlBuddy, ATTR_Y, falloutPlane);
	endTag(xmlBuddy);

	fseek(input, savePos, SEEK_SET);
}

static void copyBackgroundModels(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item) {
	if (item.number == 0 || item.offset == 0) return;
	long savePos = ftell(input);
	fseek(input, item.offset, SEEK_SET);

	for (uint32_t i = 0; i < item.number; i++) {
		startTagType(xmlBuddy, TAG_BACKGROUND_MODEL);
		//                                                                 Offset   Size   Description
		fseek(input, 0x4, SEEK_CUR);                                    // 0x0      0x4    0x0000001F
		uint32_t asciiNameOffset = readInt(input);                      // 0x4      0x4    Offset to model name
		startTagType(xmlBuddy, TAG_NAME);
		writeAsciiName(input, xmlBuddy, asciiNameOffset);
		endTag(xmlBuddy);
		fseek(input, 0x4, SEEK_CUR);                                    // 0x8      0x4    Null
		VectorF32 position = readVectorF32(input);                      // 0xC      0xC    Position (X, Y, Z)
		writeVectorF32(xmlBuddy, TAG_POSITION, position);
		VectorI16 rotOriginal = readVectorI16(input, 1);                // 0x18      0x8    Rotation (X, Y, Z, Pad)
		VectorF32 rotation = convertRot16ToF32(rotOriginal);
		writeVectorF32(xmlBuddy, TAG_ROTATION, rotation);
		VectorF32 scale = readVectorF32(input);                         // 0x20    0xC     Scale (X, Y, Z)
		writeVectorF32(xmlBuddy, TAG_SCALE, scale);
		uint32_t animOneOffset = readInt(input);                        // 0x2C    0x4     Offset to the first background animation header
		copyBackgroundAnimationOne(input, xmlBuddy, animOneOffset);
		uint32_t animTwoOffset = readInt(input);                        // 0x30    0x4     Offset to the second background animation header
		uint32_t effectHeader = readInt(input);                         // 0x34    0x4     Offset to effect header
		endTag(xmlBuddy);
	}
	fseek(input, savePos, SEEK_SET);
}

static void copyBackgroundAnimationOne(FILE *input, XMLBuddy *xmlBuddy, uint32_t animOffset) {
	if (animOffset == 0) return;
	long savePos = ftell(input);
	fseek(input, animOffset, SEEK_SET);

	//                                                                 Offset   Size   Description
	fseek(input, 0x4, SEEK_CUR);                                    // 0x0      0x4    Unknown/Null
	float animLoopPoint = readFloat(input);                      // 0x4      0x4    Animation loop point
	writeTagWithFloatValue(xmlBuddy, TAG_ANIM_LOOP_TIME, animLoopPoint);
	fseek(input, 0x8, SEEK_CUR);                                    // 0x8      0x8    Unknown/Null

	ConfigObject rotX = readItem(input);                            // 0x10     0x8    Rotation X Anim Data (Number, offset)
	ConfigObject rotY = readItem(input);                            // 0x18     0x8    Rotation Y Anim Data (Number, offset)
	ConfigObject rotZ = readItem(input);                            // 0x20     0x8    Rotation Z Anim Data (Number, offset)
	ConfigObject posX = readItem(input);                            // 0x28     0x8    Translation X Anim Data (Number, offset)
	ConfigObject posY = readItem(input);                            // 0x30     0x8    Translation Y Anim Data (Number, offset)
	ConfigObject posZ = readItem(input);                            // 0x38     0x8    Translation Z Anim Data (Number, offset)
	fseek(input, 0x10, SEEK_CUR);                                   // 0x40     0x10   Unknown/Null

	startTagType(xmlBuddy, TAG_ANIM_KEYFRAMES);
	copyFieldAnimationType(input, xmlBuddy, TAG_ROT_X, rotX);
	copyFieldAnimationType(input, xmlBuddy, TAG_ROT_Y, rotY);
	copyFieldAnimationType(input, xmlBuddy, TAG_ROT_Z, rotZ);
	copyFieldAnimationType(input, xmlBuddy, TAG_POS_X, posX);
	copyFieldAnimationType(input, xmlBuddy, TAG_POS_Y, posY);
	copyFieldAnimationType(input, xmlBuddy, TAG_POS_Z, posZ);
	endTag(xmlBuddy);

	fseek(input, savePos, SEEK_SET);
}

static void copyFog(FILE *input, XMLBuddy *xmlBuddy, uint32_t fogOffset, uint32_t fogAnimOffset) {
	if (fogOffset == 0) return;
	long savePos = ftell(input);
	fseek(input, fogOffset, SEEK_SET);

	// Deal with main fog header first
	startTagType(xmlBuddy, TAG_FOG);
	//                                                                 Offset   Size   Description
	uint8_t fogType = (uint8_t)fgetc(input);                        // 0x0      0x1    Fog Type
	writeFogType(xmlBuddy, fogType);
	fseek(input, 0x3, SEEK_CUR);                                    // 0x1      0x3    Null
	float fogStart = readFloat(input);                              // 0x4      0x4    Fog start distance
	writeTagWithFloatValue(xmlBuddy, TAG_START, fogStart);
	float fogEnd = readFloat(input);                                // 0x8      0x4    Fog end distance
	writeTagWithFloatValue(xmlBuddy, TAG_END, fogEnd);
	float red = readFloat(input);                                   // 0xC      0x4    Amount of Red
	writeTagWithFloatValue(xmlBuddy, TAG_RED, red);
	float green = readFloat(input);                                 // 0x10     0x4    Amount of Green
	writeTagWithFloatValue(xmlBuddy, TAG_GREEN, green);
	float blue = readFloat(input);                                  // 0x14     0x4    Amount of Blue
	writeTagWithFloatValue(xmlBuddy, TAG_BLUE, blue);
	fseek(input, 0xC, SEEK_CUR);                                    // 0x18     0xC    Unknown/Null
	copyFogAnimation(input, xmlBuddy, fogAnimOffset);

	endTag(xmlBuddy);
	fseek(input, savePos, SEEK_SET);
}

static void copyFogAnimation(FILE *input, XMLBuddy *xmlBuddy, uint32_t fogAnimOffset) {
	if (fogAnimOffset == 0) return;
	long savePos = ftell(input);
	fseek(input, fogAnimOffset, SEEK_SET);

	//                                                                 Offset   Size   Description
	ConfigObject startDist = readItem(input);                       // 0x0      0x8    Start Distance Anim Data (Number, offset)
	ConfigObject endDist = readItem(input);                         // 0x8      0x8    End Distance Anim Data (Number, offset)
	ConfigObject red = readItem(input);                             // 0x10     0x8    Red Anim Data (Number, offset)
	ConfigObject green = readItem(input);                           // 0x18     0x8    Green Anim Data (Number, offset)
	ConfigObject blue = readItem(input);                            // 0x20     0x8    Blue Anim Data (Number, offset)
	fseek(input, 0x8, SEEK_CUR);                                    // 0x28     0x8    Unknown Anim Data (Number, offset)

	startTagType(xmlBuddy, TAG_ANIM_KEYFRAMES);
	copyFieldAnimationType(input, xmlBuddy, TAG_START, startDist);
	copyFieldAnimationType(input, xmlBuddy, TAG_END, endDist);
	copyFieldAnimationType(input, xmlBuddy, TAG_RED, red);
	copyFieldAnimationType(input, xmlBuddy, TAG_GREEN, green);
	copyFieldAnimationType(input, xmlBuddy, TAG_BLUE, blue);
	endTag(xmlBuddy);

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

	startTagType(xmlBuddy, TAG_ANIM_KEYFRAMES);
	copyFieldAnimationType(input, xmlBuddy, TAG_ROT_X, rotX);
	copyFieldAnimationType(input, xmlBuddy, TAG_ROT_Y, rotY);
	copyFieldAnimationType(input, xmlBuddy, TAG_ROT_Z, rotZ);
	copyFieldAnimationType(input, xmlBuddy, TAG_POS_X, posX);
	copyFieldAnimationType(input, xmlBuddy, TAG_POS_Y, posY);
	copyFieldAnimationType(input, xmlBuddy, TAG_POS_Z, posZ);
	endTag(xmlBuddy);

	fseek(input, savePos, SEEK_SET);
}

static void copyFieldAnimationType(FILE *input, XMLBuddy *xmlBuddy, enum TAG_TYPE tagType, ConfigObject animData) {
	if (animData.number == 0) return;
	long savePos = ftell(input);
	fseek(input, animData.offset, SEEK_SET);
	startTagType(xmlBuddy, tagType);

	for (uint32_t i = 0; i < animData.number; i++) {
		//                                                                 Offset   Size   Description
		uint32_t easing = readInt(input);                               // 0x0      0x4    Easing
		float time = readFloat(input);                                  // 0x4      0x4    Time (Seconds)
		float value = readFloat(input);                                 // 0x8      0x4    Value (Amount: pos, rot, R/G/B, ect)
		fseek(input, 0x8, SEEK_CUR);                                    // 0xC      0x8    Unknown/Null
		startTagType(xmlBuddy, TAG_KEYFRAME);
		addAttrTypeDouble(xmlBuddy, ATTR_TIME, time);
		addAttrTypeDouble(xmlBuddy, ATTR_VALUE, value);
		writeAnimEasingVal(xmlBuddy, easing);

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
	
	uint16_t numTris = calculateNumTriangles(input, item);
	if (numTris != 0) {
		if (objOutput == NULL) {
			initObjoutput();
		}
		if (objOutput != NULL) {
			writeObjOutput(input, item, numTris);
		}
	}
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
		writeGoalType(xmlBuddy, goalType);
		endTag(xmlBuddy);
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
		writeBananaType(xmlBuddy, bananaType);
		endTag(xmlBuddy);
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

static void copyLevelModelBs(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item) {
	if (item.number == 0 || item.offset == 0) return;
	long savePos = ftell(input);
	fseek(input, item.offset, SEEK_SET);

	for (uint32_t i = 0; i < item.number; i++) {
		startTagType(xmlBuddy, TAG_LEVEL_MODEL);
		//                                                                 Offset   Size   Description
		                                                                 // Level Model B
		uint32_t levelModelAPointerOffset = readInt(input);             // 0x0      0x4    Offset to the Level Model A Pointer
		long savePos2 = ftell(input);
		fseek(input, levelModelAPointerOffset, SEEK_SET);
		                                                                // Level Model A Pointer
		fseek(input, 0x8, SEEK_CUR);                                    // 0x0      0x8    0x0000000000000001
		uint32_t levelModelAOffset = readInt(input);                    // 0x8      0x4    Offset to Level Model A
		fseek(input, levelModelAOffset, SEEK_SET);
		                                                                 // Level Model A
		fseek(input, 0x4, SEEK_CUR);                                    // 0x0      0x4    Null
		uint32_t levelModelNameOffset = readInt(input);
		writeAsciiName(input, xmlBuddy, levelModelNameOffset);

		fseek(input, savePos2, SEEK_SET);
		endTag(xmlBuddy);
	}
	fseek(input, savePos, SEEK_SET);
}

static void copySwitches(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item) {
	if (item.number == 0 || item.offset == 0) return;
	long savePos = ftell(input);
	fseek(input, item.offset, SEEK_SET);

	for (uint32_t i = 0; i < item.number; i++) {
		startTagType(xmlBuddy, TAG_SWITCH);
		//                                                                 Offset   Size   Description
		VectorF32 position = readVectorF32(input);                      // 0x0      0xC    Position (X, Y, Z)
		writeVectorF32(xmlBuddy, TAG_POSITION, position);
		VectorI16 rotOriginal = readVectorI16(input, 0);                // 0xC      0x6    Rotation (X, Y, Z)
		VectorF32 rotation = convertRot16ToF32(rotOriginal);
		writeVectorF32(xmlBuddy, TAG_ROTATION, rotation);
		uint16_t switchType = readShort(input);                         // 0x12     0x2    Switch Type
		writeAnimType(xmlBuddy, TAG_TYPE, switchType);
		uint16_t animGroupIDAffected = readShort(input);                // 0x14     0x2    Animation Group ID affected
		startTagType(xmlBuddy, TAG_ANIM_GROUP_ID);
		addValUInt32(xmlBuddy, (uint32_t)animGroupIDAffected);
		endTag(xmlBuddy);
		fseek(input, 0x2, SEEK_CUR);                                    // 0x16     0x2    Null

		endTag(xmlBuddy);
	}
	fseek(input, savePos, SEEK_SET);
}

static void copyWormholes(FILE *input, XMLBuddy *xmlBuddy, ConfigObject item) {
	if (item.number == 0 || item.offset == 0) return;
	long savePos = ftell(input);
	fseek(input, item.offset, SEEK_SET);

	for (uint32_t i = 0; i < item.number; i++) {
		startTagType(xmlBuddy, TAG_WORMHOLE);
		uint32_t offset = (uint32_t)ftell(input);
		int wormHoleIndex = getWormholeIndex(offset);
		writeTagWithInt32Value(xmlBuddy, TAG_NAME, wormHoleIndex);
		//                                                                 Offset   Size   Description
		fseek(input, 0x4, SEEK_CUR);                                    // 0x0      0x4    0x00000001
		VectorF32 position = readVectorF32(input);                      // 0x4      0xC    Position (X, Y, Z)
		writeVectorF32(xmlBuddy, TAG_POSITION, position);
		VectorI16 rotOriginal = readVectorI16(input, 1);                // 0x10     0x8    Rotation (X, Y, Z, Padd)
		VectorF32 rotation = convertRot16ToF32(rotOriginal);
		writeVectorF32(xmlBuddy, TAG_ROTATION, rotation);
		uint32_t destOffset = readInt(input);                           // 0x18     0x4    Offset to destination wormhole
		int destWormholeIndex = getWormholeIndex(destOffset);
		writeTagWithInt32Value(xmlBuddy, TAG_DESTINATION_NAME, destWormholeIndex);
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
	vector32.z = readFloat(input);
	return vector32;
}

static VectorI16 readVectorI16(FILE *input, int eatPadding) {
	VectorI16 vector16;
	vector16.x = readShort(input);
	vector16.y = readShort(input);
	vector16.z = readShort(input);
	if (eatPadding) readShort(input);
	return vector16;
}

static VectorF32 convertRot16ToF32(VectorI16 rotOriginal) {
	const double conversionFactor = 360.0 / 65536.0;
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

static int getWormholeIndex(uint32_t offset) {
	for (int i = 0; i < wormholeCount && i < MAX_NUM_WORMHOLES; i++) {
		if (wormHoleOffsets[i] == offset) {
			return i;
		}
	}
	if (wormholeCount == MAX_NUM_WORMHOLES) {
		return -1;
	}
	wormHoleOffsets[wormholeCount] = offset;
	return wormholeCount++;
}

static uint16_t calculateNumTriangles(FILE *input, CollisionGroupHeader colGroupHeader) {
	if (colGroupHeader.gridTriangleListOffet == 0) return 0;
	long savePos = ftell(input);
	fseek(input, colGroupHeader.gridTriangleListOffet, SEEK_SET);

	uint16_t maxIndex = 0;

	// Loop through the grid of triangle lists and find the highest value
	for (uint32_t col = 0; col < colGroupHeader.gridStepXCount; col++) {
		for (uint32_t row = 0; row < colGroupHeader.gridStepZCount; row++) {
			uint32_t listOffset = readInt(input);
			if (listOffset == 0) continue;
			long savePos2 = ftell(input);
			fseek(input, listOffset, SEEK_SET);

			uint16_t triIndex = readShort(input);
			while (triIndex != 0xFFFF) {
				if (triIndex > maxIndex) {
					maxIndex = triIndex;
				}
				triIndex = readShort(input);
			}

			fseek(input, savePos2, SEEK_SET);
		}
	}
	
	fseek(input, savePos, SEEK_SET);
	return maxIndex;
}

static void writeAsciiName(FILE *input, XMLBuddy *xmlBuddy, uint32_t nameOffset) {
	if (nameOffset == 0) return;
	long savePos = ftell(input);
	fseek(input, nameOffset, SEEK_SET);
	char nameBuff[256] = { 0 };

	int index = 0;
	while (1) {
		uint8_t c = (uint8_t)fgetc(input);
		if (feof(input) || c == 0 || index >= 255) break;
		nameBuff[index++] = c;
	}
	nameBuff[index] = '\0';

	addValStr(xmlBuddy, nameBuff);
	fseek(input, savePos, SEEK_SET);
}

static void initObjoutput() {
	objOutput = fopen("test.obj", "w");
	if (objOutput == NULL) {
		perror("Failed to initialize obj output file");
	}
}

static void writeObjOutput(FILE *input, CollisionGroupHeader colGroupHeader, uint16_t numTris) {
	if (colGroupHeader.triangleListOffset == 0 || numTris == 0) return;
	long savePos = ftell(input);
	fseek(input, colGroupHeader.triangleListOffset, SEEK_SET);
	// Write out object name (base don tri count for now
	fprintf(objOutput, "o %d\n", curVertexCount);

	Mat3 *pastTris = malloc(numTris * sizeof(Mat3));
	
	for (uint16_t curTri = 0; curTri < numTris; curTri++) {
		//                                                                 Offset   Size   Description
		VectorF32 pointOne = readVectorF32(input);                      // 0x0      0xC    First point (X, Y, Z)
		VectorF32 normal = readVectorF32(input);                        // 0xC      0xC    Normal (X, Y, Z)
		VectorI16 rotOrig = readVectorI16(input, 1);                    // 0x18     0x8    Rotation from XY plane (X, Y, Z, Padd)
		VectorF32 rot = convertRot16ToF32(rotOrig);
		float dX2X1 = readFloat(input);                                 // 0x20     0x4    DX2X1
		float dY2Y1 = readFloat(input);                                 // 0x24     0x4    DY2Y1
		float dX3X1 = readFloat(input);                                 // 0x28     0x4    DX3X1
		float dY3Y1 = readFloat(input);                                 // 0x2C     0x4    DY3Y1
		fseek(input, 0x10, SEEK_CUR);                                   // 0x30     0x10   XY tangent and bitangent

		// Collect rotation into rotation matices
		Mat3 xRot = makeRotationMatrixX(rot.x);
		Mat3 yRot = makeRotationMatrixY(rot.y);
		Mat3 zRot = makeRotationMatrixZ(rot.z);

		// Collect displacement for points two and three
		VectorF32 pointTwo =   { dX2X1, dY2Y1, 0.0f };
		VectorF32 pointThree = { dX3X1, dY3Y1, 0.0f };

		// Grab 

		// Rotate them (Z, Y, X)
		pointTwo = multiplyMat3ByVec3(zRot, pointTwo);
		pointTwo = multiplyMat3ByVec3(yRot, pointTwo);
		pointTwo = multiplyMat3ByVec3(xRot, pointTwo);
		pointTwo.x += pointOne.x;
		pointTwo.y += pointOne.y;
		pointTwo.z += pointOne.z;

		pointThree = multiplyMat3ByVec3(zRot, pointThree);
		pointThree = multiplyMat3ByVec3(yRot, pointThree);
		pointThree = multiplyMat3ByVec3(xRot, pointThree);
		pointThree.x += pointOne.x;
		pointThree.y += pointOne.y;
		pointThree.z += pointOne.z;


		// Check if this tri has been written before
		int found = 0;
		for (uint16_t i = 0; i < curTri; i++) {
			if (pastTris[i].x1 == pointOne.x && pastTris[i].y1 == pointOne.y && pastTris[i].z1 == pointOne.z) {
				if (pastTris[i].x2 == pointTwo.x && pastTris[i].y2 == pointTwo.y && pastTris[i].z2 == pointTwo.z) {
					if (pastTris[i].x3 == pointThree.x && pastTris[i].y3 == pointThree.y && pastTris[i].z3 == pointThree.z) {
						found = 1;
						break;
					}
				}
			}
		}
		if (found) continue;

		// Copy new tri into past list
		pastTris[curTri].x1 = pointOne.x;
		pastTris[curTri].y1 = pointOne.y;
		pastTris[curTri].z1 = pointOne.z;

		pastTris[curTri].x2 = pointTwo.x;
		pastTris[curTri].y2 = pointTwo.y;
		pastTris[curTri].z2 = pointTwo.z;

		pastTris[curTri].x3 = pointThree.x;
		pastTris[curTri].y3 = pointThree.y;
		pastTris[curTri].z3 = pointThree.z;

		// Write out coordinates
		writeTriVertex(pointOne);
		writeTriVertex(pointTwo);
		writeTriVertex(pointThree);
		fflush(objOutput);
		// Write out normals
		writeTriNormal(normal);
		fflush(objOutput);
		// Write out face
		writeLastTriFace();
		fflush(objOutput);
	}

	fseek(input, savePos, SEEK_SET);
}

static void writeTriVertex(VectorF32 vertex) {
	fprintf(objOutput, "v %f %f %f\n", vertex.x, vertex.y, vertex.z);
}

static void writeTriNormal(VectorF32 normal) {
	fprintf(objOutput, "vn %f %f %f\n", normal.x, normal.y, normal.z);
}

static void writeLastTriFace() {
	fprintf(objOutput, "f %d//%d %d//%d %d//%d\n", curVertexCount, curNormalCount, curVertexCount + 1, curNormalCount, curVertexCount + 2, curNormalCount);
	curVertexCount += 3;
	curNormalCount++;
}

static float degreeToRadian(float degrees) {
	return (degrees * PI_F) / 180.0f;
}

static Mat3 makeRotationMatrixX(float degrees) {
	float radians = (float)degreeToRadian(degrees);
	float cosine = (float)cos(radians);
	float sine = (float)sin(radians);

	Mat3 matrix = { 1.0f,      0.0f,       0.0f,
		            0.0f,    cosine,     -sine,
		            0.0f,     sine,      cosine };
	return matrix;
}

static Mat3 makeRotationMatrixY(float degrees) {
	float radians = (float)degreeToRadian(degrees);
	float cosine = (float)cos(radians);
	float sine = (float)sin(radians);

	Mat3 matrix = { cosine,      0.0f,     sine,
		              0.0f,      1.0f,     0.0f,
		             -sine,      0.0f,    cosine };
	return matrix;
}

static Mat3 makeRotationMatrixZ(float degrees) {
	float radians = (float)degreeToRadian(degrees);
	float cosine = (float)cos(radians);
	float sine = (float)sin(radians);

	Mat3 matrix = { cosine,      -sine,      0.0f,
		             sine,       cosine,     0.0f,
		             0.0f,         0.0f,     1.0f };
	return matrix;
}

static VectorF32 multiplyMat3ByVec3(Mat3 matrix, VectorF32 vector) {
	VectorF32 result;
	result.x = (matrix.x1 * vector.x) + (matrix.y1 * vector.y) + (matrix.z1 * vector.z);
	result.y = (matrix.x2 * vector.x) + (matrix.y2 * vector.y) + (matrix.z2 * vector.z);
	result.z = (matrix.x3 * vector.x) + (matrix.y3 * vector.y) + (matrix.z3 * vector.z);
	return result;
}