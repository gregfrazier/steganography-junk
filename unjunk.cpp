#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifndef _MSC_VER
#define fopen_s(pFile,filename,mode) ((*(pFile))=fopen((filename),(mode)))==NULL
#endif

typedef struct s_params
{
	char *inputFilename;
	char *outputFilename;
	char *blendFilename;
	long fileSize;
	int canExecute; // equals 7 when true
} params;

params commandParams(char* arguments[], int argCount)
{
	char y, *command;
	params p;
	p.canExecute = 0;
	p.fileSize = 0;
	
	for(int CycleArguments = 1; CycleArguments < argCount; CycleArguments++)
	{
		y = (arguments[CycleArguments])[0];

		if(y == '-')
		{
			command = arguments[CycleArguments];
			++command;
			
			if(strcmp(command, "i") == 0)
			{
				// Get the port, it will be the next "command"
				if(CycleArguments + 1 < argCount){
					command = arguments[++CycleArguments];
					p.inputFilename = (char*)malloc(sizeof(char) * (strlen(command) + 1));
					memcpy(p.inputFilename, command, strlen(command)+1);
					p.canExecute |= 0x01;
				}
			}
			else if(strcmp(command, "o") == 0)
			{
				if(CycleArguments + 1 < argCount){
					command = arguments[++CycleArguments];
					p.outputFilename = (char*)malloc(sizeof(char) * (strlen(command) + 1));
					memcpy(p.outputFilename, command, strlen(command)+1);
					p.canExecute |= 0x02;
				}
			}
			else if(strcmp(command, "b") == 0)
			{
				if(CycleArguments + 1 < argCount){
					command = arguments[++CycleArguments];
					p.blendFilename = (char*)malloc(sizeof(char) * (strlen(command) + 1));
					memcpy(p.blendFilename, command, strlen(command)+1);
					p.canExecute |= 0x04;
				}
			}
			else if(strcmp(command, "s") == 0)
			{
				if(CycleArguments + 1 < argCount){
					command = arguments[++CycleArguments];
					p.fileSize = atoi(command);
				}
			}
		}
	}
	return p;
}

unsigned char *readBMP(char *pszFilename, long *lgSize, long *offset)
{
	FILE *TARGET;
	unsigned char *mem;
	long h = 0;
	unsigned char bitmapFileHeader[14];
	unsigned char bitmapInfoHeader[40];
	unsigned int rawsize;

	fopen_s(&TARGET, pszFilename, "rb");
	if(!TARGET)
	{
		printf("readBMP(): Unable to open file %s\n", pszFilename);
		*lgSize = 0;
		return 0;
	}

	fread(&bitmapFileHeader, sizeof(unsigned char) * 14, 1, TARGET);
	fread(&bitmapInfoHeader, sizeof(unsigned char) * 40, 1, TARGET);

	if(bitmapInfoHeader[14] != 24)
	{
		printf("readBMP(): Not a 24-bit BMP File\n");
		*lgSize = 0;
		return 0;
	}

	if(offset != 0)
		*offset = bitmapFileHeader[6]         | 
		          (bitmapFileHeader[7] << 8)  | 
		          (bitmapFileHeader[8] << 16) | 
		          (bitmapFileHeader[9] << 24);
		
	fseek(TARGET, 54, SEEK_SET);
	rawsize = bitmapInfoHeader[20]         | 
	          (bitmapInfoHeader[21] << 8)  | 
	          (bitmapInfoHeader[22] << 16) | 
	          (bitmapInfoHeader[23] << 24);
	h = (long) rawsize - 54;

	//printf("%d\n", rawsize);

	mem = (unsigned char*)malloc(h+1);
	if(mem == NULL)
	{
		printf("readBMP(): Memory not allocated\n");
		*lgSize = 0;
		return 0;
	}

	fread(mem, sizeof(unsigned char), h, TARGET);
	*lgSize = h;
	fclose(TARGET);
	return mem;
}

int main(int argc, char **argv)
{
	long fileSize, stenoSize, endOffset;
	unsigned int w, h;
	FILE *writeFile;
	unsigned char xorChar = 0x3d;
	unsigned char *filecontents = 0;
	unsigned char *stenoContents = 0;
	params p;

	p = commandParams(argv, argc);

	if(p.canExecute != 7)
	{
		printf("Incorrect parameters:\n");
		printf("-i inputfile -b blendfile -o outputfile [-s size_in_bytes]\n");
		return -1;
	}

	filecontents = readBMP(p.inputFilename, &fileSize, &endOffset);
	if(filecontents == 0)
	{
		printf("main(): bad input file\n");
		return -1;
	}

	endOffset *= 3;

	//printf("Offset: %d\n", endOffset);

	stenoContents = readBMP(p.blendFilename, &stenoSize, 0);
	if(stenoContents == 0)
	{
		printf("main(): bad blend file\n");
		return -1;
	}

	fopen_s(&writeFile, p.outputFilename, "wb");
	if(!writeFile)
	{
		printf("main(): bad output file\n");
		return -1;
	}

	double divisor = 0.90;

	if(endOffset == 0)
		printf("Embedded filesize not found. Please specify one as an argument. Decoding using full size of input file.\n",);

	if(p.fileSize <= 0){
		fileSize = fileSize >= endOffset ? endOffset : fileSize;
	} else {
		fileSize = fileSize >= p.fileSize ? p.fileSize : fileSize;
	}

	printf("Output Filesize: %u\n", fileSize);

	for(unsigned int x = 0; x < fileSize; x += 3)
	{
		unsigned char xc1 = filecontents[x]   - (unsigned char)(stenoContents[x]   * divisor);
		unsigned char xc2 = filecontents[x+1] - (unsigned char)(stenoContents[x+1] * divisor);
		unsigned char xc3 = filecontents[x+2] - (unsigned char)(stenoContents[x+2] * divisor);
		unsigned char xc = ((xc3 << 5) & 0xE0) | ((xc2 << 3) & 0x18) | (xc1 & 0x07);
		//((xc2 << 4) & 0xF0) | (xc1 & 0x0F);
		fwrite(&xc, 1, 1, writeFile);
	}

	fclose(writeFile);
	free(filecontents);
	free(stenoContents);
}