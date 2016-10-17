#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define READINT(x) (getc(x) << 24) + (getc(x) << 16) + (getc(x) << 8) + (getc(x))
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

int main(int argc, char* argv[]) {
	if (argc <= 1) {
		printf("Add level paths as command line params");
	}

	for (int i = 1; i < argc; ++i) {
		int length = (int) strlen(argv[i]);
		if (argv[i][length - 1] == 'z') {
			printf("Warning: This might be a COMPRESSED lz file\nMake sure this is a RAW lz file\nFile: %s\nPress Y to continue or N to skip\n", argv[i]);
			char c;
			scanf("%c", &c);
			if (c == 'Y' || c == 'y') {
				printf("Continuing\n");
			}
			else {
				printf("Skipping\n");
				continue;
			}
		}

		FILE* lz = fopen(argv[i], "r");
		if (lz == NULL) {
			printf("ERROR: %s not found\n", argv[i]);
			continue;
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

		int gameCheck = READINT(lz);
		if (gameCheck == 0x64) {
			game = SMB1;
		}
		else if (gameCheck == 0x447A0000) {
			game = SMB2;
		}
		else {
			printf("Unsupported or Unrecognized SMB Game: %s\n", argv[i]);
			continue;
		}

		if (game == SMB1) {
			collisionFields.number = READINT(lz);
			collisionFields.offset = READINT(lz);
		}
		else if (game == SMB2) {
			fseek(lz, 8, SEEK_SET);
		}
		startPositions.offset = READINT(lz);

		falloutY.number = 1;
		falloutY.offset = READINT(lz);

		startPositions.number = (falloutY.offset - startPositions.offset) / 0x14;

		goals.number = READINT(lz);
		goals.offset = READINT(lz);

		if (game == SMB1) {
			fseek(lz, 8, SEEK_CUR);
		}

		bumpers.number = READINT(lz);
		bumpers.offset = READINT(lz);

		jamabars.number = READINT(lz);
		jamabars.offset = READINT(lz);

		bananas.number = READINT(lz);
		bananas.offset = READINT(lz);

		if (game == SMB1) {
			fseek(lz, 104, SEEK_SET);
		}
		else if (game == SMB2) {
			fseek(lz, 88, SEEK_SET);
		}

		backgrounds.number = READINT(lz);
		backgrounds.offset = READINT(lz);

		char outfileName[512];
		sscanf(argv[i], "%507s", outfileName);
		length = (int) strlen(outfileName);
		outfileName[length++] = '.';
		outfileName[length++] = 't';
		outfileName[length++] = 'x';
		outfileName[length++] = 't';
		outfileName[length++] = '\0';

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

			uint16_t shortType = (getc(lz) << 8) + (getc(lz));
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
			else if(game == SMB2){
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

			int intType = READINT(lz);
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

				int animationFrameOffset = READINT(lz);

				if (!animationFrameOffset) {
					fseek(lz, 172, SEEK_CUR);
					continue;
				}

				++numAnims;

				int nameOffsetOffset = READINT(lz);

				int position = ftell(lz);

				fseek(lz, nameOffsetOffset, SEEK_SET);
				int nameOffset = READINT(lz);
				fseek(lz, nameOffset, SEEK_SET);
				char modelName[512];
				char animFilename[512];

				fscanf(lz, "%501s", modelName);
				fseek(lz, position, SEEK_SET);

				strcpy(animFilename, modelName);

				int length = (int) strlen(animFilename);
				animFilename[length + 0] = 'a';
				animFilename[length + 1] = 'n';
				animFilename[length + 2] = 'i';
				animFilename[length + 3] = 'm';
				animFilename[length + 4] = '.';
				animFilename[length + 5] = 't';
				animFilename[length + 6] = 'x';
				animFilename[length + 7] = 't';
				animFilename[length + 8] = '\0';

				int numFrames = 0;
				AnimFrame animFrames[120];

				memset(animFrames, 0, 120 * sizeof(AnimFrame));

				printf("ANIM OFFSET: %d\n", animationFrameOffset);

				fseek(lz, animationFrameOffset, SEEK_SET);

				int numXRot = READINT(lz);
				int offsetXRot = READINT(lz);
				int numYRot = READINT(lz);
				int offsetYRot = READINT(lz);
				int numZRot = READINT(lz);
				int offsetZRot = READINT(lz);

				int numXPos = READINT(lz);
				int offsetXPos = READINT(lz);
				int numYPos = READINT(lz);
				int offsetYPos = READINT(lz);
				int numZPos = READINT(lz);
				int offsetZPos = READINT(lz);

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

				fprintf(outfile, "animobj [ %d ] . file . x = %s\n", j, animFilename);
				fprintf(outfile, "animobj [ %d ] . name . x = %s\n", j, modelName);
				fprintf(outfile, "animobj [ %d ] . center . x = %f\n", j, xPosCenter);
				fprintf(outfile, "animobj [ %d ] . center . y = %f\n", j, xPosCenter);
				fprintf(outfile, "animobj [ %d ] . center . z = %f\n", j, xPosCenter);

				FILE* animFile = fopen(animFilename, "w");

				for (int k = 0; k < numFrames; ++k) {
					fprintf(animFile, "frame [ %d ] . time . x = %f\n", k, animFrames[k].time);

					fprintf(animFile, "frame [ %d ] . pos . x = %f\n", k, animFrames[k].xPos);
					fprintf(animFile, "frame [ %d ] . pos . y = %f\n", k, animFrames[k].yPos);
					fprintf(animFile, "frame [ %d ] . pos . z = %f\n", k, animFrames[k].zPos);

					fprintf(animFile, "frame [ %d ] . rot . x = %f\n", k, animFrames[k].xRot + xRotCenter);
					fprintf(animFile, "frame [ %d ] . rot . y = %f\n", k, animFrames[k].yRot + yRotCenter);
					fprintf(animFile, "frame [ %d ] . rot . z = %f\n", k, animFrames[k].zRot + zRotCenter);
				}
				fclose(animFile);

			}

			if (numAnims) {
				fprintf(outfile, "\n");
			}
		}

		

		fseek(lz, backgrounds.offset, SEEK_SET);

		for (int j = 0; j < backgrounds.number; ++j) {
			

			fseek(lz, 4, SEEK_CUR);
			int nameOffset = READINT(lz);

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



	return 0;
}