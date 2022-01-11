#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

const char* const license =
"MIT License\n"
"\n"
"Copyright (c) 2022 Brandon McGriff\n"
"\n"
"Permission is hereby granted, free of charge, to any person obtaining a copy\n"
"of this software and associated documentation files (the \"Software\"), to deal\n"
"in the Software without restriction, including without limitation the rights\n"
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n"
"copies of the Software, and to permit persons to whom the Software is\n"
"furnished to do so, subject to the following conditions:\n"
"\n"
"The above copyright notice and this permission notice shall be included in all\n"
"copies or substantial portions of the Software.\n"
"\n"
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n"
"SOFTWARE.\n";

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

#define lengthof(array) (sizeof((array)) / sizeof(*(array)))

#define YMZ_SIZE 0x400000
#define USER1_SIZE 0x80000
#define SPRITEGEN_SIZE 0x400000
#define GFX2_SIZE 0x400000
#define GFX3_SIZE 0x400000
#define READBUFFER_SIZE (USER1_SIZE * 2u)

static uint8_t
	* user1Data = NULL,
	* spritegenData = NULL,
	* gfx2Data = NULL,
	* gfx3Data = NULL,
	* ymzData = NULL,
	* readBuffer = NULL;

static const char
	* const user1OriginalFilenames[2] = {
		"snw000j1.u6",
		"snw001j1.u4"
	},
	* const spritegenOriginalFilenames[2] = {
		"snw10000.u21",
		"snw10100.u20"
	},
	* const gfx2OriginalFilenames[2] = {
		"snw20000.u17",
		"snw20100.u9"
	},
	* const gfx3OriginalFilename = "snw21000.u3",
	* const ymzOriginalFilename = "snw30000.u1";

static FILE
	* user1OriginalFiles[2] = { 0 },
	* spritegenOriginalFiles[2] = { 0 },
	* gfx2OriginalFiles[2] = { 0 },
	* gfx3OriginalFile = NULL,
	* ymzOriginalFile = NULL;

static const char
	* const user1ConvertedFilename = "user1.rom",
	* const spritegenConvertedFilename = "spritegen.rom",
	* const gfx2ConvertedFilename = "gfx2.rom",
	* const gfx3ConvertedFilename = "gfx3.rom",
	* const ymzConvertedFilename = "ymz.rom";

static FILE
	* user1ConvertedFile = NULL,
	* spritegenConvertedFile = NULL,
	* gfx2ConvertedFile = NULL,
	* gfx3ConvertedFile = NULL,
	* ymzConvertedFile = NULL;

static int finish(int failureLevel, int exitCode) {
	switch (failureLevel) {
	case 2:
		if (user1ConvertedFile) fclose(user1ConvertedFile);
		if (spritegenConvertedFile) fclose(spritegenConvertedFile);
		if (gfx2ConvertedFile) fclose(gfx2ConvertedFile);
		if (gfx3ConvertedFile) fclose(gfx3ConvertedFile);
		if (ymzConvertedFile) fclose(ymzConvertedFile);

	case 1:
		if (user1OriginalFiles[0]) fclose(user1OriginalFiles[0]);
		if (user1OriginalFiles[1]) fclose(user1OriginalFiles[1]);
		if (spritegenOriginalFiles[0]) fclose(spritegenOriginalFiles[0]);
		if (spritegenOriginalFiles[1]) fclose(spritegenOriginalFiles[1]);
		if (gfx2OriginalFiles[0]) fclose(gfx2OriginalFiles[0]);
		if (gfx2OriginalFiles[1]) fclose(gfx2OriginalFiles[1]);
		if (gfx3OriginalFile) fclose(gfx3OriginalFile);
		if (ymzOriginalFile) fclose(ymzOriginalFile);

	case 0:
		if (user1Data) free(user1Data);
		if (spritegenData) free(spritegenData);
		if (gfx2Data) free(gfx2Data);
		if (gfx3Data) free(gfx3Data);
		if (ymzData) free(ymzData);
		if (readBuffer) free(readBuffer);

	default:
		return exitCode;
	}
}

static void showUsage() {
	fprintf(stderr, "Show this software's license:\nsenknow-romtool --license\n\n");
	fprintf(stderr, "Rename and combine original ROMs to singular binary files:\nsenknow-romtool --combine [split ROMs source directory] [combined ROMs destination directory]\n\n");
	fprintf(stderr, "Rename and split apart combined ROMs to the original format:\nsenknow-romtool --split [combined ROMs source directory] [split ROMs destination directory]\n\n");
	fprintf(stderr, "This tool cannot create directories, the directories passed in must already exist\n");
}

int main(int argc, char** argv) {
	if (argc == 2 && !strcmp(argv[1], "--license")) {
		fprintf(stderr, "%s", license);
		return EXIT_FAILURE;
	}
	if (argc < 4) {
		showUsage();
		return EXIT_FAILURE;
	}

	char* task = argv[1];
	char* source = argv[2];
	char* destination = argv[3];
	int failureLevel = -1;

	failureLevel++;
	if (
		!(user1Data = malloc(USER1_SIZE * lengthof(user1OriginalFilenames))) ||
		!(spritegenData = malloc(SPRITEGEN_SIZE * lengthof(spritegenOriginalFilenames))) ||
		!(gfx2Data = malloc(GFX2_SIZE * lengthof(gfx2OriginalFilenames))) ||
		!(gfx3Data = malloc(GFX3_SIZE)) ||
		!(ymzData = malloc(YMZ_SIZE)) ||
		!(readBuffer = malloc(READBUFFER_SIZE))
	) {
		fprintf(stderr, "Failed to allocate memory\n");
		return finish(failureLevel, EXIT_FAILURE);
	}

	if (!strcmp(task, "--combine")) {
		failureLevel++;
		if (
			!(user1OriginalFiles[0] = openFile(source, user1OriginalFilenames[0], "rb")) ||
			!(user1OriginalFiles[1] = openFile(source, user1OriginalFilenames[1], "rb")) ||
			!(spritegenOriginalFiles[0] = openFile(source, spritegenOriginalFilenames[0], "rb")) ||
			!(spritegenOriginalFiles[1] = openFile(source, spritegenOriginalFilenames[1], "rb")) ||
			!(gfx2OriginalFiles[0] = openFile(source, gfx2OriginalFilenames[0], "rb")) ||
			!(gfx2OriginalFiles[1] = openFile(source, gfx2OriginalFilenames[1], "rb")) ||
			!(gfx3OriginalFile = openFile(source, gfx3OriginalFilename, "rb")) ||
			!(ymzOriginalFile = openFile(source, ymzOriginalFilename, "rb"))
		) {
			fprintf(stderr, "Failed to open split ROM files for reading\n");
			return finish(failureLevel, EXIT_FAILURE);
		}

		failureLevel++;
		if (
			!(user1ConvertedFile = openFile(destination, "user1.rom", "wb")) ||
			!(spritegenConvertedFile = openFile(destination, "spritegen.rom", "wb")) ||
			!(gfx2ConvertedFile = openFile(destination, "gfx2.rom", "wb")) ||
			!(gfx3ConvertedFile = openFile(destination, "gfx3.rom", "wb")) ||
			!(ymzConvertedFile = openFile(destination, "ymz.rom", "wb"))
		) {
			fprintf(stderr, "Failed to open combined ROM files for writing\n");
			return finish(failureLevel, EXIT_FAILURE);
		}

		if (
			fread(readBuffer + USER1_SIZE * 0u, USER1_SIZE, 1u, user1OriginalFiles[0]) != 1u ||
			fread(readBuffer + USER1_SIZE * 1u, USER1_SIZE, 1u, user1OriginalFiles[1]) != 1u
		) {
			fprintf(stderr, "Failed to read user1 split ROM files\n");
			return finish(failureLevel, EXIT_FAILURE);
		}
		for (size_t i = 0u; i < USER1_SIZE; i++) {
			user1Data[i * 2u + 0u] = readBuffer[USER1_SIZE * 0u + i];
		}
		for (size_t i = 0u; i < USER1_SIZE; i++) {
			user1Data[i * 2u + 1u] = readBuffer[USER1_SIZE * 1u + i];
		}

		if (
			fread(spritegenData + SPRITEGEN_SIZE * 0u, SPRITEGEN_SIZE, 1u, spritegenOriginalFiles[0]) != 1u ||
			fread(spritegenData + SPRITEGEN_SIZE * 1u, SPRITEGEN_SIZE, 1u, spritegenOriginalFiles[1]) != 1u
		) {
			fprintf(stderr, "Failed to read split spritegen ROM files\n");
			return finish(failureLevel, EXIT_FAILURE);
		}

		if (
			fread(gfx2Data + GFX2_SIZE * 0u, GFX2_SIZE, 1u, gfx2OriginalFiles[0]) != 1u ||
			fread(gfx2Data + GFX2_SIZE * 1u, GFX2_SIZE, 1u, gfx2OriginalFiles[1]) != 1u
		) {
			fprintf(stderr, "Failed to read split gfx2 ROM files\n");
			return finish(failureLevel, EXIT_FAILURE);
		}

		if (
			fread(gfx3Data, GFX3_SIZE, 1u, gfx3OriginalFile) != 1u
		) {
			fprintf(stderr, "Failed to read original gfx3 ROM file\n");
			return finish(failureLevel, EXIT_FAILURE);
		}

		if (
			fread(ymzData, YMZ_SIZE, 1u, ymzOriginalFile) != 1u
		) {
			fprintf(stderr, "Failed to read original ymz ROM file\n");
			return finish(failureLevel, EXIT_FAILURE);
		}

		if (
			fwrite(user1Data, USER1_SIZE, lengthof(user1OriginalFilenames), user1ConvertedFile) != lengthof(user1OriginalFilenames) ||
			fwrite(spritegenData, SPRITEGEN_SIZE, lengthof(spritegenOriginalFilenames), spritegenConvertedFile) != lengthof(spritegenOriginalFilenames) ||
			fwrite(gfx2Data, GFX2_SIZE, lengthof(gfx2OriginalFilenames), gfx2ConvertedFile) != lengthof(gfx2OriginalFilenames) ||
			fwrite(gfx3Data, GFX3_SIZE, 1u, gfx3ConvertedFile) != 1u ||
			fwrite(ymzData, YMZ_SIZE, 1u, ymzConvertedFile) != 1u
		) {
			fprintf(stderr, "Failed to write converted ROM files\n");
			return finish(failureLevel, EXIT_FAILURE);
		}

		printf("Successfully wrote all combined/renamed ROMs\n");
		return finish(failureLevel, EXIT_SUCCESS);
	}
	else if (!strcmp(task, "--split")) {
		failureLevel++;
		if (
			!(user1OriginalFiles[0] = openFile(destination, user1OriginalFilenames[0], "wb")) ||
			!(user1OriginalFiles[1] = openFile(destination, user1OriginalFilenames[1], "wb")) ||
			!(spritegenOriginalFiles[0] = openFile(destination, spritegenOriginalFilenames[0], "wb")) ||
			!(spritegenOriginalFiles[1] = openFile(destination, spritegenOriginalFilenames[1], "wb")) ||
			!(gfx2OriginalFiles[0] = openFile(destination, gfx2OriginalFilenames[0], "wb")) ||
			!(gfx2OriginalFiles[1] = openFile(destination, gfx2OriginalFilenames[1], "wb")) ||
			!(gfx3OriginalFile = openFile(destination, gfx3OriginalFilename, "wb")) ||
			!(ymzOriginalFile = openFile(destination, ymzOriginalFilename, "wb"))
		) {
			fprintf(stderr, "Failed to open split ROM files for writing\n");
			return finish(failureLevel, EXIT_FAILURE);
		}

		failureLevel++;
		if (
			!(user1ConvertedFile = openFile(source, user1ConvertedFilename, "rb")) ||
			!(spritegenConvertedFile = openFile(source, spritegenConvertedFilename, "rb")) ||
			!(gfx2ConvertedFile = openFile(source, gfx2ConvertedFilename, "rb")) ||
			!(gfx3ConvertedFile = openFile(source, gfx3ConvertedFilename, "rb")) ||
			!(ymzConvertedFile = openFile(source, ymzConvertedFilename, "rb"))
		) {
			fprintf(stderr, "Failed to open combined ROM files for reading\n");
			return finish(failureLevel, EXIT_FAILURE);
		}

		if (fread(readBuffer, USER1_SIZE, lengthof(user1OriginalFilenames), user1ConvertedFile) != lengthof(user1OriginalFilenames)) {
			fprintf(stderr, "Failed to read %s\n", user1ConvertedFilename);
			return finish(failureLevel, EXIT_FAILURE);
		}
		for (size_t i = 0u; i < USER1_SIZE; i++) {
			user1Data[USER1_SIZE * 0u + i] = readBuffer[i * 2u + 0u];
		}
		for (size_t i = 0u; i < USER1_SIZE; i++) {
			user1Data[USER1_SIZE * 1u + i] = readBuffer[i * 2u + 1u];
		}

		if (fread(spritegenData, SPRITEGEN_SIZE, lengthof(spritegenOriginalFilenames), spritegenConvertedFile) != lengthof(spritegenOriginalFilenames)) {
			fprintf(stderr, "Failed to read %s\n", spritegenConvertedFilename);
			return finish(failureLevel, EXIT_FAILURE);
		}

		if (fread(gfx2Data, GFX2_SIZE, lengthof(gfx2OriginalFilenames), gfx2ConvertedFile) != lengthof(gfx2OriginalFilenames)) {
			fprintf(stderr, "Failed to read %s\n", gfx2ConvertedFilename);
			return finish(failureLevel, EXIT_FAILURE);
		}

		if (fread(gfx3Data, GFX3_SIZE, 1u, gfx3ConvertedFile) != 1u) {
			fprintf(stderr, "Failed to read %s\n", gfx3ConvertedFilename);
			return finish(failureLevel, EXIT_FAILURE);
		}

		if (fread(ymzData, YMZ_SIZE, 1u, ymzConvertedFile) != 1u) {
			fprintf(stderr, "Failed to read %s\n", ymzConvertedFilename);
			return finish(failureLevel, EXIT_FAILURE);
		}

		if (
			fwrite(user1Data + USER1_SIZE * 0u, USER1_SIZE, 1u, user1OriginalFiles[0]) != 1u ||
			fwrite(user1Data + USER1_SIZE * 1u, USER1_SIZE, 1u, user1OriginalFiles[1]) != 1u ||
			fwrite(spritegenData + SPRITEGEN_SIZE * 0u, SPRITEGEN_SIZE, 1u, spritegenOriginalFiles[0]) != 1u ||
			fwrite(spritegenData + SPRITEGEN_SIZE * 1u, SPRITEGEN_SIZE, 1u, spritegenOriginalFiles[1]) != 1u ||
			fwrite(gfx2Data + GFX2_SIZE * 0u, GFX2_SIZE, 1u, gfx2OriginalFiles[0]) != 1u ||
			fwrite(gfx2Data + GFX2_SIZE * 1u, GFX2_SIZE, 1u, gfx2OriginalFiles[1]) != 1u ||
			fwrite(gfx3Data, GFX3_SIZE, 1u, gfx3OriginalFile) != 1u ||
			fwrite(ymzData, YMZ_SIZE, 1u, ymzOriginalFile) != 1u
		) {
			fprintf(stderr, "Failed to write converted ROM files\n");
			return finish(failureLevel, EXIT_FAILURE);
		}

		printf("Successfully wrote all split/renamed ROMs\n");
		return finish(failureLevel, EXIT_SUCCESS);
	}
	else {
		showUsage();
		return finish(failureLevel, EXIT_FAILURE);
	}
}
