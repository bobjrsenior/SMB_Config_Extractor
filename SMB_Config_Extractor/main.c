#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "FunctionsAndDefines.h"
#include "configExtractor.h"

typedef struct {
	int number;
	int offset;
}ConfigObjectOld;

typedef struct {
	float time;
	float xPos;
	float yPos;
	float zPos;
	float xRot;
	float yRot;
	float zRot;
}AnimFrame;

static float readRot(FILE* file) {
	char rotStr[3];
	fscanf(file, "%c%c", (rotStr + 1), (rotStr + 0));
	float angle = (float)*((unsigned short*)rotStr);
	angle = angle * 360.0f / 65536.0f;
	return angle;
}

#define NUM_SMB1_MARKERS 22
#define NUM_SMB2_MARKERS 24
#define NUM_SMBX_MARKERS 35

static uint32_t SMB1Markers[NUM_SMB1_MARKERS] = { 0x00000064, 0x00000078, 0x0000000a, 0x0000000f, 0x00000005, 0x00000008, 0x0000000e, 0x00000019, 0x00000014, 0x0000001e, 0x0000003c, 0x0000001b, 0x00000002, 0x00000006, 0x00000004, 0x0000001f, 0x00000012, 0x0000001a, 0x00000028, 0x000001e0, 0x000000f0, 0x000000c8 };
static uint32_t SMB2Markers[NUM_SMB2_MARKERS] = { 0x42c80000, 0x447a0000, 0x41f00000, 0x42700000, 0x41200000, 0x45bb8000, 0x453b8000, 0x438c0000, 0x43f00000, 0x42a00000, 0x42200000, 0x44fa0000, 0x41a00000, 0x44e10000, 0x40000000, 0x40400000, 0x43200000, 0x43520000, 0x42480000, 0x43700000, 0x44760000, 0x43dc0000, 0x442f0000, 0x43480000 };
static uint32_t SMBXMarkers[NUM_SMBX_MARKERS] = { 0x0000c842, 0x00007a44, 0x0000f041, 0x00007042, 0x00002041, 0x0080bb45, 0x00803b45, 0x00008c43, 0x0000f043, 0x0000a042, 0x00002042, 0x00004843, 0x0000fa44, 0x0000a041, 0x0000e144, 0x00000040, 0x00004040, 0x00002043, 0x00005243, 0x00004842, 0x00007043, 0x00007644, 0x0000dc43, 0x00002f44, 0x0000c040, 0x0000f042, 0x0000a040, 0x00000041, 0x0000c841, 0x0000d841, 0x00008040, 0x0000f841, 0x00009041, 0x00000042, 0x00007041 };

// File Reading Functions (Endianness handling)
static uint32_t(*readInt)(FILE*);
static uint32_t(*readIntRev)(FILE*);
static uint16_t(*readShort)(FILE*);
static uint16_t(*readShortRev)(FILE*);
static float(*readFloat)(FILE*);
static float(*readFloatRev)(FILE*);

static int decompress(const char* filename);
static int determineGame(const char *filename);
static void extractConfigOld(char* filename, int game);

static int insert(AnimFrame frameTimes[], int count, float value) {

	int position = -1;
	for (int i = 0; i < count; ++i) {
		if (frameTimes[i].time == value) {
			return 0;
		}
		else if (frameTimes[i].time > value) {
			position = i;
			break;
		}
	}
	if (position == -1) {
		frameTimes[count].time = value;
		return 1;
	}
	for (int i = count; i > position; --i) {
		frameTimes[i] = frameTimes[i - 1];
	}
	frameTimes[position].time = value;
	return 1;
}

static void printHelp() {
	puts("Usage: ./SMB_LZ_Tool [(FLAG | FILE)...]");
	puts("Flags:");
	puts("    -help      Show this help");
	puts("    -h");
	puts("");
	puts("    -legacy    Use the old config extractor for smbcnv style configs");
	puts("    -l");
	puts("");
	puts("    -new       Use the new config extractor for xml style configs (default)");
	puts("    -n");
	puts("");

}

int main(int argc, char* argv[]) {
	if (argc <= 1) {
		printf("Add level paths as command line params");
	}

	int legacyExtractor = 0;

	for (int i = 1; i < argc; ++i) {
		// Check for Command Line flags
		if (strcmp(argv[i], "-legacy") == 0 || strcmp(argv[i], "-l") == 0) {
			legacyExtractor = 1;
			continue;
		}
		else if (strcmp(argv[i], "-new") == 0 || strcmp(argv[i], "-n") == 0) {
			legacyExtractor = 0;
			continue;
		}
		else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-help") == 0) {
			printHelp();
			continue;
		}

		char filename[512];
		int decomp = 0;
		int filelength = (int) strlen(argv[i]);
		if (filelength < 4) {
			printf("Filename too short: %s\n", argv[i]);
			continue;
		}
		sscanf(argv[i], "%507s", filename);
		// If lz file
		if (filename[filelength - 2] == 'l' && filename[filelength - 1] == 'z') {
			printf("Decompressing\n");
			decomp = 1;
		}// If not raw file
		else if (!(filename[filelength - 2] == 'l' && filename[filelength - 1] == 'z' && filename[filelength - 1] == 'z')) {
			// TODO If wanted
			//printf("Warning, this may not be a raw LZ file. Please make sure you didn't drop the wrong file.");
			//printf("File in Question: %s\n", argv[i]);
			//continue;
		}

		if (decomp) {
			if (decompress(filename) == 0) {
				filename[filelength++] = '.';
				filename[filelength++] = 'r';
				filename[filelength++] = 'a';
				filename[filelength++] = 'w';
				filename[filelength++] = '\0';
			}
			else {
				printf("Failed to decompress %s\nSkipping\n", filename);
			}
		}
		int game = determineGame(filename);
		if (game == -1) {
			printf("Unknown Game Marker for '%s'.\nContact Bobjrsenior", filename);
			continue;
		}
		if (game == SMB1 || legacyExtractor) {
			extractConfigOld(filename, game);
		}
		else {
			extractConfig(filename, game);
		}
	}



	return 0;
}

static int determineGame(const char *filename) {
	FILE *input = fopen(filename, "rb");
	if (input == NULL) {
		perror("Failed to check game version");
		return -1;
	}
	fseek(input, 0x4, SEEK_SET);
	uint32_t gameCheck = readBigInt(input);
	fclose(input);

	// Check SMB 1
	for (int i = 0; i < NUM_SMB1_MARKERS; i++) {
		if (gameCheck == SMB1Markers[i]) {
			return SMB1;
		}
	}

	// Check SMB 2
	for (int i = 0; i < NUM_SMB2_MARKERS; i++) {
		if (gameCheck == SMB2Markers[i]) {
			return SMB2;
		}
	}

	// Check SMB Deluxe
	for (int i = 0; i < NUM_SMBX_MARKERS; i++) {
		if (gameCheck == SMBXMarkers[i]) {
			return SMBX;
		}
	}

	return -1;
}

static void extractConfigOld(char* filename, int game) {
	FILE* lz = fopen(filename, "rb");
	if (lz == NULL) {
		printf("ERROR: %s not found\n", filename);
		return;
	}

	ConfigObjectOld collisionFields;
	ConfigObjectOld startPositions;
	ConfigObjectOld falloutY;
	ConfigObjectOld goals;
	ConfigObjectOld bumpers;
	ConfigObjectOld jamabars;
	ConfigObjectOld bananas;
	ConfigObjectOld backgrounds;

	// Init read functions (SMB1/2 is big endian, SMBX is little endian)
	if (game == SMB1 || game == SMB2) {
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


	fseek(lz, 8, SEEK_SET);

	if (game == SMB1) {
		collisionFields.number = readInt(lz);
		collisionFields.offset = readInt(lz);
	}
	else {
		fseek(lz, 8, SEEK_CUR);
	}
	startPositions.offset = readInt(lz);

	falloutY.number = 1;
	falloutY.offset = readInt(lz);

	startPositions.number = (falloutY.offset - startPositions.offset) / 0x14;

	goals.number = readInt(lz);
	goals.offset = readInt(lz);

	if (game == SMB1) {
		fseek(lz, 8, SEEK_CUR);
	}

	bumpers.number = readInt(lz);
	bumpers.offset = readInt(lz);

	jamabars.number = readInt(lz);
	jamabars.offset = readInt(lz);

	bananas.number = readInt(lz);
	bananas.offset = readInt(lz);

	if (game == SMB1) {
		fseek(lz, 104, SEEK_SET);
	}
	else {
		fseek(lz, 88, SEEK_SET);
	}

	backgrounds.number = readInt(lz);
	backgrounds.offset = readInt(lz);

	char outfileName[512];
	sscanf(filename, "%507s", outfileName);
	int fileLength = (int)strlen(outfileName);
	outfileName[fileLength++] = '.';
	outfileName[fileLength++] = 't';
	outfileName[fileLength++] = 'x';
	outfileName[fileLength++] = 't';
	outfileName[fileLength++] = '\0';

	FILE* outfile = fopen(outfileName, "w");

	fseek(lz, falloutY.offset, SEEK_SET);

	float falloutYYPos = readBigFloat(lz);
	fprintf(outfile, "fallout [ 0 ] . pos . y = %f\n", falloutYYPos);

	fprintf(outfile, "\n");

	fseek(lz, startPositions.offset, SEEK_SET);

	for (int j = 0; j < startPositions.number; ++j) {
		float xPos = readBigFloat(lz);
		float yPos = readBigFloat(lz);
		float zPos = readBigFloat(lz);

		float xRot = readRot(lz);
		float yRot = readRot(lz);
		float zRot = readRot(lz);

		fseek(lz, 2, SEEK_CUR);

		fprintf(outfile, "start [ %d ] . pos . x = %f\n", j, xPos);
		fprintf(outfile, "start [ %d ] . pos . y = %f\n", j, yPos);
		fprintf(outfile, "start [ %d ] . pos . z = %f\n", j, zPos);

		fprintf(outfile, "start [ %d ] . rot . x = %f\n", j, xRot);
		fprintf(outfile, "start [ %d ] . rot . y = %f\n", j, yRot);
		fprintf(outfile, "start [ %d ] . rot . z = %f\n", j, zRot);

		fprintf(outfile, "\n");
	}

	fseek(lz, goals.offset, SEEK_SET);

	for (int j = 0; j < goals.number; ++j) {
		float xPos = readBigFloat(lz);
		float yPos = readBigFloat(lz);
		float zPos = readBigFloat(lz);

		float xRot = readRot(lz);
		float yRot = readRot(lz);
		float zRot = readRot(lz);

		uint16_t shortType = readShort(lz);
		char type = 'B';
		if (game == SMB1) {
			if (shortType == 0x4200) {
				type = 'B';
			}
			else if (shortType == 0x4700) {
				type = 'G';
			}
			else if (shortType == 0x5200) {
				type = 'R';
			}
		}
		else {
			if (shortType == 0x0001) {
				type = 'B';
			}
			else if (shortType == 0x0101) {
				type = 'G';
			}
			else if (shortType == 0x0201) {
				type = 'R';
			}
		}

		fprintf(outfile, "goal [ %d ] . pos . x = %f\n", j, xPos);
		fprintf(outfile, "goal [ %d ] . pos . y = %f\n", j, yPos);
		fprintf(outfile, "goal [ %d ] . pos . z = %f\n", j, zPos);

		fprintf(outfile, "goal [ %d ] . rot . x = %f\n", j, xRot);
		fprintf(outfile, "goal [ %d ] . rot . y = %f\n", j, yRot);
		fprintf(outfile, "goal [ %d ] . rot . z = %f\n", j, zRot);

		fprintf(outfile, "goal [ %d ] . type . x = %c\n", j, type);

		fprintf(outfile, "\n");
	}

	fseek(lz, bumpers.offset, SEEK_SET);

	for (int j = 0; j < bumpers.number; ++j) {
		float xPos = readBigFloat(lz);
		float yPos = readBigFloat(lz);
		float zPos = readBigFloat(lz);

		float xRot = readRot(lz);
		float yRot = readRot(lz);
		float zRot = readRot(lz);

		fseek(lz, 2, SEEK_CUR);

		float xScl = readBigFloat(lz);
		float yScl = readBigFloat(lz);
		float zScl = readBigFloat(lz);

		fprintf(outfile, "bumper [ %d ] . pos . x = %f\n", j, xPos);
		fprintf(outfile, "bumper [ %d ] . pos . y = %f\n", j, yPos);
		fprintf(outfile, "bumper [ %d ] . pos . z = %f\n", j, zPos);

		fprintf(outfile, "bumper [ %d ] . rot . x = %f\n", j, xRot);
		fprintf(outfile, "bumper [ %d ] . rot . y = %f\n", j, yRot);
		fprintf(outfile, "bumper [ %d ] . rot . z = %f\n", j, zRot);

		fprintf(outfile, "bumper [ %d ] . scl . x = %f\n", j, xScl);
		fprintf(outfile, "bumper [ %d ] . scl . y = %f\n", j, yScl);
		fprintf(outfile, "bumper [ %d ] . scl . z = %f\n", j, zScl);

		fprintf(outfile, "\n");
	}

	fseek(lz, jamabars.offset, SEEK_SET);

	for (int j = 0; j < jamabars.number; ++j) {
		float xPos = readBigFloat(lz);
		float yPos = readBigFloat(lz);
		float zPos = readBigFloat(lz);

		float xRot = readRot(lz);
		float yRot = readRot(lz);
		float zRot = readRot(lz);

		fseek(lz, 2, SEEK_CUR);

		float xScl = readBigFloat(lz);
		float yScl = readBigFloat(lz);
		float zScl = readBigFloat(lz);

		fprintf(outfile, "jamabar [ %d ] . pos . x = %f\n", j, xPos);
		fprintf(outfile, "jamabar [ %d ] . pos . y = %f\n", j, yPos);
		fprintf(outfile, "jamabar [ %d ] . pos . z = %f\n", j, zPos);

		fprintf(outfile, "jamabar [ %d ] . rot . x = %f\n", j, xRot);
		fprintf(outfile, "jamabar [ %d ] . rot . y = %f\n", j, yRot);
		fprintf(outfile, "jamabar [ %d ] . rot . z = %f\n", j, zRot);


		fprintf(outfile, "jamabar [ %d ] . scl . x = %f\n", j, xScl);
		fprintf(outfile, "jamabar [ %d ] . scl . y = %f\n", j, yScl);
		fprintf(outfile, "jamabar [ %d ] . scl . z = %f\n", j, zScl);

		fprintf(outfile, "\n");
	}

	fseek(lz, bananas.offset, SEEK_SET);

	for (int j = 0; j < bananas.number; ++j) {
		float xPos = readBigFloat(lz);
		float yPos = readBigFloat(lz);
		float zPos = readBigFloat(lz);

		int intType = readInt(lz);
		char type = 'N';
		if (intType == 1) {
			type = 'B';
		}

		fprintf(outfile, "banana [ %d ] . pos . x = %f\n", j, xPos);
		fprintf(outfile, "banana [ %d ] . pos . y = %f\n", j, yPos);
		fprintf(outfile, "banana [ %d ] . pos . z = %f\n", j, zPos);

		fprintf(outfile, "banana [ %d ] . type . x = %c\n", j, type);

		fprintf(outfile, "\n");
	}

	if (game == SMB1) {
		int numAnims = 0;
		fseek(lz, collisionFields.offset, SEEK_SET);

		for (int j = 0; j < collisionFields.number; ++j) {


			float xPosCenter = readBigFloat(lz);
			float yPosCenter = readBigFloat(lz);
			float zPosCenter = readBigFloat(lz);

			float xRotCenter = readRot(lz);
			float yRotCenter = readRot(lz);
			float zRotCenter = readRot(lz);

			fseek(lz, 2, SEEK_CUR);

			int animationFrameOffset = readInt(lz);

			if (!animationFrameOffset) {
				fseek(lz, 172, SEEK_CUR);
				continue;
			}

			++numAnims;

			int nameOffsetOffset = readInt(lz);

			int position = ftell(lz);

			fseek(lz, nameOffsetOffset, SEEK_SET);
			int nameOffset = readInt(lz);
			fseek(lz, nameOffset, SEEK_SET);
			char modelName[512];
			char animFilename[512];

			fscanf(lz, "%501s", modelName);
			fseek(lz, position, SEEK_SET);

			strcpy(animFilename, modelName);

			int animObjlength = (int)strlen(animFilename);
			animFilename[animObjlength + 0] = 'a';
			animFilename[animObjlength + 1] = 'n';
			animFilename[animObjlength + 2] = 'i';
			animFilename[animObjlength + 3] = 'm';
			animFilename[animObjlength + 4] = '.';
			animFilename[animObjlength + 5] = 't';
			animFilename[animObjlength + 6] = 'x';
			animFilename[animObjlength + 7] = 't';
			animFilename[animObjlength + 8] = '\0';

			int numFrames = 0;
			AnimFrame animFrames[4096];

			memset(animFrames, 0, 120 * sizeof(AnimFrame));

			fseek(lz, animationFrameOffset, SEEK_SET);
			int numXRot = readInt(lz);
			int offsetXRot = readInt(lz);
			int numYRot = readInt(lz);
			int offsetYRot = readInt(lz);
			int numZRot = readInt(lz);
			int offsetZRot = readInt(lz);

			int numXPos = readInt(lz);
			int offsetXPos = readInt(lz);
			int numYPos = readInt(lz);
			int offsetYPos = readInt(lz);
			int numZPos = readInt(lz);
			int offsetZPos = readInt(lz);

			// First Pass: Collect Frame Times

			fseek(lz, offsetXRot + 4, SEEK_SET);
			for (int k = 0; k < numXRot; ++k) {
				numFrames += insert(animFrames, numFrames, readBigFloat(lz));
				fseek(lz, 16, SEEK_CUR);
			}

			fseek(lz, offsetYRot + 4, SEEK_SET);
			for (int k = 0; k < numYRot; ++k) {
				numFrames += insert(animFrames, numFrames, readBigFloat(lz));
				fseek(lz, 16, SEEK_CUR);
			}

			fseek(lz, offsetZRot + 4, SEEK_SET);
			for (int k = 0; k < numZRot; ++k) {
				numFrames += insert(animFrames, numFrames, readBigFloat(lz));
				fseek(lz, 16, SEEK_CUR);
			}


			fseek(lz, offsetXPos + 4, SEEK_SET);
			for (int k = 0; k < numXPos; ++k) {
				numFrames += insert(animFrames, numFrames, readBigFloat(lz));
				fseek(lz, 16, SEEK_CUR);
			}

			fseek(lz, offsetYPos + 4, SEEK_SET);
			for (int k = 0; k < numYPos; ++k) {
				numFrames += insert(animFrames, numFrames, readBigFloat(lz));
				fseek(lz, 16, SEEK_CUR);
			}

			fseek(lz, offsetZPos + 4, SEEK_SET);
			for (int k = 0; k < numZPos; ++k) {
				numFrames += insert(animFrames, numFrames, readBigFloat(lz));
				fseek(lz, 16, SEEK_CUR);
			}

			// Second Pass: Collect Frame Data

			for (int k = 0; k < numFrames; ++k) {

				float prevTime = 0;
				float prevAmount = 0;
				char found = 0;

				// Go all from data for this axis
				fseek(lz, offsetXRot + 4, SEEK_SET);
				for (int l = 0; l < numXRot; ++l) {
					// Get the time and translation/rotation
					float time = readBigFloat(lz);
					float amount = readBigFloat(lz);

					// If times match up, insert it
					if (time == animFrames[k].time) {
						animFrames[k].xRot = amount;
						found = 1;
						break;
					}// If the time is too far, extrapolate
					else if (time > animFrames[k].time) {
						float fractionTime = time / prevTime;
						float fractionAmount = (prevAmount + amount) * fractionTime;
						animFrames[k].xRot = fractionAmount;
						found = 1;
						break;
					}
					prevTime = time;
					prevAmount = amount;
					fseek(lz, 12, SEEK_CUR);
				}

				if (!found) {
					animFrames[k].xRot = 0;
				}

				found = 0;

				// Go all from data for this axis
				fseek(lz, (offsetYRot + 4), SEEK_SET);
				for (int l = 0; l < numYRot; ++l) {
					// Get the time and translation/rotation
					float time = readBigFloat(lz);
					float amount = readBigFloat(lz);

					// If times match up, insert it
					if (time == animFrames[k].time) {
						animFrames[k].yRot = amount;
						found = 1;
						break;
					}// If the time is too far, extrapolate
					else if (time > animFrames[k].time) {
						float fractionTime = time / prevTime;
						float fractionAmount = (prevAmount + amount) * fractionTime;
						animFrames[k].yRot = fractionAmount;
						found = 1;
						break;
					}
					prevTime = time;
					prevAmount = amount;
					fseek(lz, 12, SEEK_CUR);
				}


				if (!found) {
					animFrames[k].yRot = 0;
				}

				found = 0;
				// Go all from data for this axis
				fseek(lz, (offsetZRot + 4), SEEK_SET);
				for (int l = 0; l < numZRot; ++l) {
					// Get the time and translation/rotation
					float time = readBigFloat(lz);
					float amount = readBigFloat(lz);

					// If times match up, insert it
					if (time == animFrames[k].time) {
						animFrames[k].zRot = amount;
						found = 1;
						break;
					}// If the time is too far, extrapolate
					else if (time > animFrames[k].time) {
						float fractionTime = time / prevTime;
						float fractionAmount = (prevAmount + amount) * fractionTime;
						animFrames[k].zRot = fractionAmount;
						found = 1;
						break;
					}
					prevTime = time;
					prevAmount = amount;
					fseek(lz, 12, SEEK_CUR);
				}


				if (!found) {
					animFrames[k].zRot = 0;
				}

				found = 0;
				// Go all from data for this axis
				fseek(lz, (offsetXPos + 4), SEEK_SET);
				for (int l = 0; l < numXPos; ++l) {
					// Get the time and translation/rotation
					float time = readBigFloat(lz);
					float amount = readBigFloat(lz);

					// If times match up, insert it
					if (time == animFrames[k].time) {
						animFrames[k].xPos = amount;
						found = 1;
						break;
					}// If the time is too far, extrapolate
					else if (time > animFrames[k].time) {
						float fractionTime = time / prevTime;
						float fractionAmount = (prevAmount + amount) * fractionTime;
						animFrames[k].xPos = fractionAmount;
						found = 1;
						break;
					}
					prevTime = time;
					prevAmount = amount;
					fseek(lz, 12, SEEK_CUR);
				}

				if (!found) {
					animFrames[k].xPos = 0;
				}

				found = 0;
				// Go all from data for this axis
				fseek(lz, (offsetYPos + 4), SEEK_SET);
				for (int l = 0; l < numYPos; ++l) {
					// Get the time and translation/rotation
					float time = readBigFloat(lz);
					float amount = readBigFloat(lz);

					// If times match up, insert it
					if (time == animFrames[k].time) {
						animFrames[k].yPos = amount;
						found = 1;
						break;
					}// If the time is too far, extrapolate
					else if (time > animFrames[k].time) {
						float fractionTime = time / prevTime;
						float fractionAmount = (prevAmount + amount) * fractionTime;
						animFrames[k].yPos = fractionAmount;
						found = 1;
						break;
					}
					prevTime = time;
					prevAmount = amount;
					fseek(lz, 12, SEEK_CUR);
				}

				if (!found) {
					animFrames[k].yPos = 0;
				}

				found = 0;
				// Go all from data for this axis
				fseek(lz, (offsetZPos + 4), SEEK_SET);
				for (int l = 0; l < numZPos; ++l) {
					// Get the time and translation/rotation
					float time = readBigFloat(lz);
					float amount = readBigFloat(lz);

					// If times match up, insert it
					if (time == animFrames[k].time) {
						animFrames[k].zPos = amount;
						found = 1;
						break;
					}// If the time is too far, extrapolate
					else if (time > animFrames[k].time) {
						float fractionTime = time / prevTime;
						float fractionAmount = (prevAmount + amount) * fractionTime;
						animFrames[k].zPos = fractionAmount;
						found = 1;
						break;
					}
					prevTime = time;
					prevAmount = amount;
					fseek(lz, 12, SEEK_CUR);
				}

				if (!found) {
					animFrames[k].zPos = 0;
				}

			}


			fseek(lz, position, SEEK_SET);
			fseek(lz, 168, SEEK_CUR);

			// Third Pass: Write the information

			fprintf(outfile, "animobj [ %d ] . file . x = %s\n", numAnims, animFilename);
			fprintf(outfile, "animobj [ %d ] . name . x = %s\n", numAnims, modelName);
			fprintf(outfile, "animobj [ %d ] . center . x = %f\n", numAnims, xPosCenter);
			fprintf(outfile, "animobj [ %d ] . center . y = %f\n", numAnims, yPosCenter);
			fprintf(outfile, "animobj [ %d ] . center . z = %f\n", numAnims, zPosCenter);
			fprintf(outfile, "\n");

			FILE* animFile = fopen(animFilename, "w");

			for (int k = 0; k < numFrames; ++k) {
				fprintf(animFile, "frame [ %d ] . time . x = %f\n", k, animFrames[k].time);

				fprintf(animFile, "frame [ %d ] . pos . x = %f\n", k, animFrames[k].xPos);
				fprintf(animFile, "frame [ %d ] . pos . y = %f\n", k, animFrames[k].yPos);
				fprintf(animFile, "frame [ %d ] . pos . z = %f\n", k, animFrames[k].zPos);

				fprintf(animFile, "frame [ %d ] . rot . x = %f\n", k, animFrames[k].xRot + xRotCenter);
				fprintf(animFile, "frame [ %d ] . rot . y = %f\n", k, animFrames[k].yRot + yRotCenter);
				fprintf(animFile, "frame [ %d ] . rot . z = %f\n", k, animFrames[k].zRot + zRotCenter);

				fprintf(animFile, "\n");
			}
			fclose(animFile);

		}
	}



	fseek(lz, backgrounds.offset, SEEK_SET);

	for (int j = 0; j < backgrounds.number; ++j) {


		fseek(lz, 4, SEEK_CUR);
		int nameOffset = readInt(lz);

		int position = ftell(lz);

		fseek(lz, nameOffset, SEEK_SET);
		char modelName[512];

		fscanf(lz, "%511s", modelName);
		fseek(lz, position, SEEK_SET);

		fseek(lz, 48, SEEK_CUR);

		fprintf(outfile, "background [ %d ] . name . x = %s\n", j, modelName);


	}

	if (backgrounds.number == 0) {
		fprintf(outfile, "\n");
	}

	fclose(lz);
	fclose(outfile);
}

int decompress(const char* filename) {
	// Try to open it
	FILE* lz = fopen(filename, "rb");
	if (lz == NULL) {
		printf("ERROR: File not found: %s\n", filename);
		return -1;
	}
	printf("Decompressing %s\n", filename);

	// Create a temp file for the unfixed lz (SMB lz has a slightly different header than FF7 LZS)
	FILE* normal = tmpfile();

	// Unfix the header (Turn it back into normal FF7 LZSS)
	uint32_t csize = readLittleInt(lz) - 8;
	fseek(lz, 4, SEEK_CUR);
	putc(csize & 0xFF, normal);
	putc((csize >> 8) & 0xFF, normal);
	putc((csize >> 16) & 0xFF, normal);
	putc((csize >> 24) & 0xFF, normal);
	for (int j = 0; j < (int)(csize); j++) {
		char c = (char)getc(lz);
		putc(c, normal);
	}
	fclose(lz);
	fflush(normal);
	fseek(normal, 0, SEEK_SET);

	// Make the output file name
	char outfileName[512];
	sscanf(filename, "%507s", outfileName);
	{
		int nameLength = (int)strlen(outfileName);
		outfileName[nameLength++] = '.';
		outfileName[nameLength++] = 'r';
		outfileName[nameLength++] = 'a';
		outfileName[nameLength++] = 'w';
		outfileName[nameLength++] = '\0';
	}

	// Open the output file for both reading and writing separately
	// This is because you read bytes from the output buffer to write to the end and prevents constant fseeking
	FILE* outfile = fopen(outfileName, "wb");
	FILE* outfileR = fopen(outfileName, "rb");


	// The size the the lzss data + 4 bytes for the header
	uint32_t filesize = readLittleInt(normal) + 4;
	printf("FILESIZE: %d\n", filesize);
	int lastPercentDone = -1;

	// Loop until we reach the end of the data or end of the file
	while ((unsigned)ftell(normal) < filesize && !feof(normal)) {

		float percentDone = (100.0f * ftell(normal)) / filesize;
		int intPercentDone = (int)percentDone;
		if (intPercentDone % 10 == 0 && intPercentDone != lastPercentDone) {
			printf("%d%% Completed\n", intPercentDone);
			lastPercentDone = intPercentDone;
		}

		// Read the first control block
		// Read right to left, each bit specifies how the the next 8 spots of data will be
		// 0 means write the byte directly to the output
		// 1 represents there will be reference (2 byte)
		uint8_t block = (uint8_t)getc(normal);

		// Go through every bit in the control block
		for (int j = 0; j < 8 && (unsigned)ftell(normal) < filesize && !feof(normal); ++j) {
			// Literal
			if (block & 0x01) {
				putc(getc(normal), outfile);
			}// Reference
			else {
				uint16_t reference = readBigShort(normal);

				// Length is the last four bits + 3
				// Any less than a lengh of 3 i pointess since a reference takes up 3 bytes
				// Length is the last nibble (last 4 bits) of the 2 reference bytes
				int length = (reference & 0x000F) + 3;

				// Offset if is all 8 bits in the first reference byte and the first nibble (4 bits) in the second reference byte
				// The nibble from the second reference byte comes before the first reference byte
				// EX: reference bytes = 0x12 0x34
				//     offset = 0x312
				int offset = ((reference & 0xFF00) >> 8) | ((reference & 0x00F0) << 4);

				// Convert the offset to how many bytes away from the end of the buffer to start reading from
				int backSet = ((ftell(outfile) - 18 - offset) & 0xFFF);

				// Calculate the actual location in the file
				int readLocation = ftell(outfile) - backSet;

				// Handle case where the offset is past the beginning of the file
				while (readLocation < 0 && length > 0) {
					putc(0, outfile);

					--length;
					++readLocation;
				}

				// Flush the file if data needs to be read from the outfile
				if (length > 0) {
					fflush(outfile);
				}

				// Seek to the reference position
				fseek(outfileR, readLocation, SEEK_SET);

				// Get the number of bytes until the current end of file (will need to flush once that is reached)
				int buffer = ftell(outfile) - readLocation;
				int curBuffer = 0;
				// Read the reference bytes until we reach length bytes written
				while (length > 0) {

					// If at the previous end of file, flush the new data
					if (curBuffer == buffer) {
						// Flush the file so we don't read something that hasn't been properly updated
						fflush(outfile);

						curBuffer = 0;
					}
					putc(getc(outfileR), outfile);
					++curBuffer;
					--length;
				}

			}
			// Go to the next reference bit in the block
			block = block >> 1;
		}

	}

	fclose(outfileR);
	fclose(outfile);
	fclose(normal);

	printf("Finished Decompressing %s\n", filename);
	return 0;
}