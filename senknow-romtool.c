#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define lengthof(array) (sizeof((array)) / sizeof(*(array)))

static FILE* openFile(const char* const path, const char* const filename, const char* const mode) {
	int length = snprintf(NULL, 0u, "%s%s%s", path, "/", filename);
	if (length < 0) {
		return NULL;
	}

	char* const fullFilename = malloc(length + 1);
	if (!fullFilename) {
		return NULL;
	}
	if (snprintf(fullFilename, length + 1, "%s%s%s", path, "/", filename) < 0) {
		free(fullFilename);
		return NULL;
	}

	FILE* file = fopen(fullFilename, mode);
	free(fullFilename);
	return file;
}

static const char* const user1ReadFilenames[2] = {
	"snw000j1.u6",
	"snw001j1.u4"
};
#define USER1_SIZE 0x80000

static const char* const spritegenReadFilenames[2] = {
	"snw10000.u21",
	"snw10100.u20"
};
#define SPRITEGEN_SIZE 0x400000

static const char* const gfx2ReadFilenames[2] = {
	"snw20000.u17",
	"snw20100.u9"
};
#define GFX2_SIZE 0x400000

static const char* const gfx3ReadFilename = "snw21000.u3";
#define GFX3_SIZE 0x400000

static const char* const ymzReadFilename = "snw30000.u1";
#define YMZ_SIZE 0x400000

#define READBUFFER_SIZE 0x400000

uint8_t
	* user1Data = NULL,
	* spritegenData = NULL,
	* gfx2Data = NULL,
	* gfx3Data = NULL,
	* ymzData = NULL,
	* readBuffers[2] = { 0 };

FILE
	* user1ReadFiles[2] = { 0 },
	* spritegenReadFiles[2] = { 0 },
	* gfx2ReadFiles[2] = { 0 },
	* gfx3ReadFile = NULL,
	* ymzReadFile = NULL;

FILE
	* user1WriteFile = NULL,
	* spritegenWriteFile = NULL,
	* gfx2WriteFile = NULL,
	* gfx3WriteFile = NULL,
	* ymzWriteFile = NULL;

void quit(int failureLevel, int exitCode) {
	switch (failureLevel) {
	case 2:
		if (user1WriteFile) fclose(user1WriteFile);
		if (spritegenWriteFile) fclose(spritegenWriteFile);
		if (gfx2WriteFile) fclose(gfx2WriteFile);
		if (gfx3WriteFile) fclose(gfx3WriteFile);
		if (ymzWriteFile) fclose(ymzWriteFile);

	case 1:
		if (user1ReadFiles[0]) fclose(user1ReadFiles[0]);
		if (user1ReadFiles[1]) fclose(user1ReadFiles[1]);
		if (spritegenReadFiles[0]) fclose(spritegenReadFiles[0]);
		if (spritegenReadFiles[1]) fclose(spritegenReadFiles[1]);
		if (gfx2ReadFiles[0]) fclose(gfx2ReadFiles[0]);
		if (gfx2ReadFiles[1]) fclose(gfx2ReadFiles[1]);
		if (gfx3ReadFile) fclose(gfx3ReadFile);
		if (ymzReadFile) fclose(ymzReadFile);

	case 0:
		if (user1Data) free(user1Data);
		if (spritegenData) free(spritegenData);
		if (gfx2Data) free(gfx2Data);
		if (gfx3Data) free(gfx3Data);
		if (ymzData) free(ymzData);
		if (readBuffers[0]) free(readBuffers[0]);
		if (readBuffers[1]) free(readBuffers[1]);

	default:
		exit(exitCode);
	}
}

int main(int argc, char** argv) {
	if (argc < 3) {
		fprintf(stderr, "Usage: convert-senknow-roms [source directory] [destination directory]\nDo not have trailing \"/\" or \"\\\" path separators on the directories (e.g. \"roms\", not \"roms/\").\n");
		exit(EXIT_FAILURE);
	}

	char* source = argv[1];
	char* destination = argv[2];

	int failureLevel = 0;
	if (
		!(user1Data = malloc(USER1_SIZE * lengthof(user1ReadFilenames))) ||
		!(spritegenData = malloc(SPRITEGEN_SIZE * lengthof(spritegenReadFilenames))) ||
		!(gfx2Data = malloc(GFX2_SIZE * lengthof(gfx2ReadFilenames))) ||
		!(gfx3Data = malloc(GFX3_SIZE)) ||
		!(ymzData = malloc(YMZ_SIZE)) ||
		!(readBuffers[0] = malloc(READBUFFER_SIZE)) ||
		!(readBuffers[1] = malloc(READBUFFER_SIZE))
	) {
		fprintf(stderr, "Failed to allocate memory.\n");
		quit(failureLevel, EXIT_FAILURE);
	}

	failureLevel++;
	if (
		!(user1ReadFiles[0] = openFile(source, user1ReadFilenames[0], "rb")) ||
		!(user1ReadFiles[1] = openFile(source, user1ReadFilenames[1], "rb")) ||
		!(spritegenReadFiles[0] = openFile(source, spritegenReadFilenames[0], "rb")) ||
		!(spritegenReadFiles[1] = openFile(source, spritegenReadFilenames[1], "rb")) ||
		!(gfx2ReadFiles[0] = openFile(source, gfx2ReadFilenames[0], "rb")) ||
		!(gfx2ReadFiles[1] = openFile(source, gfx2ReadFilenames[1], "rb")) ||
		!(gfx3ReadFile = openFile(source, gfx3ReadFilename, "rb")) ||
		!(ymzReadFile = openFile(source, ymzReadFilename, "rb"))
	) {
		fprintf(stderr, "Failed to open ROM files for reading.\n");
		quit(failureLevel, EXIT_FAILURE);
	}

	failureLevel++;
	if (
		!(user1WriteFile = openFile(destination, "user1.rom", "wb")) ||
		!(spritegenWriteFile = openFile(destination, "spritegen.rom", "wb")) ||
		!(gfx2WriteFile = openFile(destination, "gfx2.rom", "wb")) ||
		!(gfx3WriteFile = openFile(destination, "gfx3.rom", "wb")) ||
		!(ymzWriteFile = openFile(destination, "ymz.rom", "wb"))
	) {
		fprintf(stderr, "Failed to open ROM files for writing.\n");
		quit(failureLevel, EXIT_FAILURE);
	}

	if (
		fread(readBuffers[0], USER1_SIZE, 1u, user1ReadFiles[0]) != 1u ||
		fread(readBuffers[1], USER1_SIZE, 1u, user1ReadFiles[1]) != 1u
	) {
		fprintf(stderr, "Failed to read user1 ROM files.\n");
		quit(failureLevel, EXIT_FAILURE);
	}
	for (size_t i = 0u; i < USER1_SIZE; i++) {
		user1Data[i * 2u + 0u] = readBuffers[0][i];
	}
	for (size_t i = 0u; i < USER1_SIZE; i++) {
		user1Data[i * 2u + 1u] = readBuffers[1][i];
	}

	if (
		fread(spritegenData + SPRITEGEN_SIZE * 0u, SPRITEGEN_SIZE, 1u, spritegenReadFiles[0]) != 1u ||
		fread(spritegenData + SPRITEGEN_SIZE * 1u, SPRITEGEN_SIZE, 1u, spritegenReadFiles[1]) != 1u
	) {
		fprintf(stderr, "Failed to read spritegen ROM files.\n");
		quit(failureLevel, EXIT_FAILURE);
	}

	if (
		fread(gfx2Data + GFX2_SIZE * 0u, GFX2_SIZE, 1u, gfx2ReadFiles[0]) != 1u ||
		fread(gfx2Data + GFX2_SIZE * 1u, GFX2_SIZE, 1u, gfx2ReadFiles[1]) != 1u
	) {
		fprintf(stderr, "Failed to read gfx2 ROM files.\n");
		quit(failureLevel, EXIT_FAILURE);
	}

	if (
		fread(gfx3Data, GFX3_SIZE, 1u, gfx3ReadFile) != 1u
	) {
		fprintf(stderr, "Failed to read gfx3 ROM file.\n");
		quit(failureLevel, EXIT_FAILURE);
	}

	if (
		fread(ymzData, YMZ_SIZE, 1u, ymzReadFile) != 1u
	) {
		fprintf(stderr, "Failed to read ymz ROM file.\n");
		quit(failureLevel, EXIT_FAILURE);
	}

	if (
		fwrite(user1Data, USER1_SIZE * lengthof(user1ReadFilenames), 1u, user1WriteFile) != 1u ||
		fwrite(spritegenData, SPRITEGEN_SIZE * lengthof(spritegenReadFilenames), 1u, spritegenWriteFile) != 1u ||
		fwrite(gfx2Data, GFX2_SIZE * lengthof(gfx2ReadFilenames), 1u, gfx2WriteFile) != 1u ||
		fwrite(gfx3Data, GFX3_SIZE, 1u, gfx3WriteFile) != 1u ||
		fwrite(ymzData, YMZ_SIZE, 1u, ymzWriteFile) != 1u
	) {
		fprintf(stderr, "Failed to write converted ROM files.\n");
		quit(failureLevel, EXIT_FAILURE);
	}

	quit(failureLevel, EXIT_SUCCESS);
	return EXIT_SUCCESS; // This is unnecessary, but silences a compiler warning.
}