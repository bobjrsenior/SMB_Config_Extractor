#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <string.h>
#include <stdint.h>

inline uint32_t readInt(FILE* file) {
	uint32_t c1 = getc(file) << 24;
	uint32_t c2 = getc(file) << 16;
	uint32_t c3 = getc(file) << 8;
	uint32_t c4 = getc(file);
	return (c1 | c2 | c3 | c4);
}

inline uint16_t readShort(FILE* file) {
	uint32_t c1 = getc(file) << 8;
	uint32_t c2 = getc(file);
	return (uint16_t) (c1 | c2);
}
#define SMB1 0
#define SMB2 1

typedef struct {
	int number;
	int offset;
}ConfigObject;

typedef struct {
	float time;
	float xPos;
	float yPos;
	float zPos;
	float xRot;
	float yRot;
	float zRot;
}AnimFrame;

float readFloat(FILE* file) {
	char floatStr[5];
	fscanf(file, "%c%c%c%c", (floatStr + 3), (floatStr + 2), (floatStr + 1), (floatStr + 0));
	return *((float*)floatStr);
}

float readRot(FILE* file) {
	char rotStr[3];
	fscanf(file, "%c%c", (rotStr + 1), (rotStr + 0));
	float angle = (float)*((unsigned short*)rotStr);
	angle = angle * 360.0f / 65536.0f;
	return angle;
}

inline uint16_t readBigShort(FILE* file) {
	char rotStr[3];
	fscanf(file, "%c%c", (rotStr + 1), (rotStr + 0));
	return *((uint16_t*)rotStr);
}

inline uint32_t readBigInt(FILE* file) {
	uint32_t c1 = getc(file);
	uint32_t c2 = getc(file) << 8;
	uint32_t c3 = getc(file) << 16;
	uint32_t c4 = getc(file) << 24;
	return (c1 | c2 | c3 | c4);
}

int decompress(char* filename);

int insert(AnimFrame frameTimes[], int count, float value) {

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

void extractConfig(char* filename);

int main(int argc, char* argv[]) {
	if (argc <= 1) {
		printf("Add level paths as command line params");
	}

	for (int i = 1; i < argc; ++i) {
		char filename[512];
		int decomp = 0;
		int filelength = (int) strlen(argv[i]);
		sscanf(argv[i], "%507s", filename);
		if (filename[filelength - 1] == 'z') {
			printf("Decompressing\n");
			decomp = 1;
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
		extractConfig(filename);
	}



	return 0;
}

void extractConfig(char* filename) {
	FILE* lz = fopen(filename, "rb");
	if (lz == NULL) {
		printf("ERROR: %s not found\n", filename);
		return;
	}

	ConfigObject collisionFields;
	ConfigObject startPositions;
	ConfigObject falloutY;
	ConfigObject goals;
	ConfigObject bumpers;
	ConfigObject jamabars;
	ConfigObject bananas;
	ConfigObject backgrounds;

	int game = SMB1;

	fseek(lz, 4, SEEK_SET);

	int gameCheck = readInt(lz);
	if (gameCheck == 0x64 || gameCheck == 0x78 || gameCheck == 0x0000001A) {
		game = SMB1;
	}
	else if (gameCheck == 0x447A0000 || gameCheck == 0x44E10000 || gameCheck == 0x42C80000) {
		game = SMB2;
	}
	else {
		printf("Unsupported or Unrecognized SMB Game: %s\n\nAssuming game is SMB1\nPlease report this level to bobjrsenior\n", filename);
		game = SMB1;
	}

	if (game == SMB1) {
		collisionFields.number = readInt(lz);
		collisionFields.offset = readInt(lz);
	}
	else if (game == SMB2) {
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
	else if (game == SMB2) {
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

	float falloutYYPos = readFloat(lz);
	fprintf(outfile, "fallout [ 0 ] . pos . y = %f\n", falloutYYPos);

	fprintf(outfile, "\n");

	fseek(lz, startPositions.offset, SEEK_SET);

	for (int j = 0; j < startPositions.number; ++j) {
		float xPos = readFloat(lz);
		float yPos = readFloat(lz);
		float zPos = readFloat(lz);

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
		float xPos = readFloat(lz);
		float yPos = readFloat(lz);
		float zPos = readFloat(lz);

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
		else if (game == SMB2) {
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
		float xPos = readFloat(lz);
		float yPos = readFloat(lz);
		float zPos = readFloat(lz);

		float xRot = readRot(lz);
		float yRot = readRot(lz);
		float zRot = readRot(lz);

		fseek(lz, 2, SEEK_CUR);

		float xScl = readFloat(lz);
		float yScl = readFloat(lz);
		float zScl = readFloat(lz);

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
		float xPos = readFloat(lz);
		float yPos = readFloat(lz);
		float zPos = readFloat(lz);

		float xRot = readRot(lz);
		float yRot = readRot(lz);
		float zRot = readRot(lz);

		fseek(lz, 2, SEEK_CUR);

		float xScl = readFloat(lz);
		float yScl = readFloat(lz);
		float zScl = readFloat(lz);

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
		float xPos = readFloat(lz);
		float yPos = readFloat(lz);
		float zPos = readFloat(lz);

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


			float xPosCenter = readFloat(lz);
			float yPosCenter = readFloat(lz);
			float zPosCenter = readFloat(lz);

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
				numFrames += insert(animFrames, numFrames, readFloat(lz));
				fseek(lz, 16, SEEK_CUR);
			}

			fseek(lz, offsetYRot + 4, SEEK_SET);
			for (int k = 0; k < numYRot; ++k) {
				numFrames += insert(animFrames, numFrames, readFloat(lz));
				fseek(lz, 16, SEEK_CUR);
			}

			fseek(lz, offsetZRot + 4, SEEK_SET);
			for (int k = 0; k < numZRot; ++k) {
				numFrames += insert(animFrames, numFrames, readFloat(lz));
				fseek(lz, 16, SEEK_CUR);
			}


			fseek(lz, offsetXPos + 4, SEEK_SET);
			for (int k = 0; k < numXPos; ++k) {
				numFrames += insert(animFrames, numFrames, readFloat(lz));
				fseek(lz, 16, SEEK_CUR);
			}

			fseek(lz, offsetYPos + 4, SEEK_SET);
			for (int k = 0; k < numYPos; ++k) {
				numFrames += insert(animFrames, numFrames, readFloat(lz));
				fseek(lz, 16, SEEK_CUR);
			}

			fseek(lz, offsetZPos + 4, SEEK_SET);
			for (int k = 0; k < numZPos; ++k) {
				numFrames += insert(animFrames, numFrames, readFloat(lz));
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
					float time = readFloat(lz);
					float amount = readFloat(lz);

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
					float time = readFloat(lz);
					float amount = readFloat(lz);

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
					float time = readFloat(lz);
					float amount = readFloat(lz);

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
					float time = readFloat(lz);
					float amount = readFloat(lz);

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
					float time = readFloat(lz);
					float amount = readFloat(lz);

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
					float time = readFloat(lz);
					float amount = readFloat(lz);

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

int decompress(char* filename) {
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
	uint32_t csize = readBigInt(lz) - 8;
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
	uint32_t filesize = readBigInt(normal) + 4;
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
				uint16_t reference = readShort(normal);

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