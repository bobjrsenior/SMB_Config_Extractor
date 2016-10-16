#include <stdio.h>
#include <string.h>

#define READINT(x) (getc(x) << 24) + (getc(x) << 16) + (getc(x) << 8) + (getc(x))

typedef struct {
	int number;
	int offset;
}ConfigObject;

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

int main(int argc, char* argv[]) {
	if (argc <= 1) {
		printf("Add level paths as command line params");
	}

	for (int i = 1; i < argc; ++i) {
		FILE* lz = fopen(argv[i], "r");
		if (lz == NULL) {
			printf("ERROR: %s not found\n", argv[i]);
			continue;
		}
		ConfigObject startPositions;
		ConfigObject falloutY;
		ConfigObject goals;
		ConfigObject bumpers;
		ConfigObject jamabars;
		ConfigObject bananas;

		fseek(lz, 16, SEEK_SET);
		startPositions.offset = READINT(lz);

		falloutY.number = 1;
		falloutY.offset = READINT(lz);

		startPositions.number = (falloutY.offset - startPositions.offset) / 0x14;

		goals.number = READINT(lz);
		goals.offset = READINT(lz);

		fseek(lz, 8, SEEK_CUR);

		bumpers.number = READINT(lz);
		bumpers.offset = READINT(lz);

		jamabars.number = READINT(lz);
		jamabars.offset = READINT(lz);

		bananas.number = READINT(lz);
		bananas.offset = READINT(lz);


		char outfileName[512];
		sscanf(argv[i], "%507s", outfileName);
		int length = strlen(outfileName);
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

			char type = getc(lz);
			getc(lz);

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

		fclose(lz);
		fclose(outfile);
	}



	return 0;
}