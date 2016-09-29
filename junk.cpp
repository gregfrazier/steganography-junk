#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef _MSC_VER
#define fopen_s(pFile,filename,mode) ((*(pFile))=fopen((filename),(mode)))==NULL
#endif

typedef struct s_params
{
	bool dryRun;
	char *inputFilename;
	char *outputFilename;
	char *blendFilename;
	int canExecute; // equals 7 when true
} params;

params commandParams(char* arguments[], int argCount)
{
	char y, *command;
	params p;
	p.canExecute = 0;
	
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
			else if(strcmp(command, "dry") == 0)
			{
				p.dryRun = true;
			}
		}
	}
	return p;
}

unsigned char *loadFile(char *pszFilename, long *lgSize)
{
	FILE *TARGET;
	unsigned char *mem, *ptr2;
	long h = 0;

	fopen_s(&TARGET, pszFilename, "rb");
	if(!TARGET)
	{
		printf("loadFile(): Unable to open file %s\n", pszFilename);
		*lgSize = 0;
		return 0;
	}

	fseek(TARGET, 0L, SEEK_END);
	h = ftell(TARGET);
	*lgSize = h;
	fseek(TARGET, 0L, SEEK_SET);
	mem = (unsigned char*)malloc(h+1);
	if(!mem)
	{
		printf("loadFile(): Memory not allocated\n");
		*lgSize = 0;
		return 0;
	}
	fread(mem, sizeof(char), h, TARGET);
	fclose(TARGET);
	return mem;
}

unsigned char *readBMP(char *pszFilename, long *lgSize)
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

	fread(&bitmapFileHeader, sizeof(char) * 14, 1, TARGET);
	fread(&bitmapInfoHeader, sizeof(char) * 40, 1, TARGET);

	if(bitmapInfoHeader[14] != 24)
	{
		printf("readBMP(): Not a 24-bit BMP File\n");
		*lgSize = 0;
		return 0;
	}

	fseek(TARGET, 54, SEEK_SET);
	rawsize = bitmapInfoHeader[20]         | 
	          (bitmapInfoHeader[21] << 8)  | 
	          (bitmapInfoHeader[22] << 16) | 
	          (bitmapInfoHeader[23] << 24);
	h = (long) rawsize;

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
	long fileSize, blendSize, leftOvers;
	unsigned int w, h, m = 0;
	FILE *writeFile;
	unsigned char *filecontents = 0;
	unsigned char *blendContents = 0;
	unsigned char xorChar = 0x3d;
	double divisor = 0.90;
	params p;

	p = commandParams(argv, argc);

	if(p.canExecute != 7 && !(p.canExecute == 1 && p.dryRun))
	{
		printf("Incorrect parameters:\n");
		printf("-i inputfile -b blendfile -o outputfile [-dry]\n");
		return -1;
	}

	filecontents = loadFile(p.inputFilename, &fileSize);
	if(!filecontents)
	{
		printf("main(): bad file\n");
		return -1;
	}

	unsigned int sqrtAct = (unsigned int)ceil(sqrt(fileSize + m));
	w = h = (sqrtAct + (4 - (sqrtAct % 4)));
	
	if(p.canExecute == 1 && p.dryRun){
		printf("Blend bitmap must be exactly %d pixels wide and at least that in height.\n", w);
		return 1;
	}

	blendContents = readBMP(p.blendFilename, &blendSize);
	if(!blendContents)
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

	unsigned char bmpfileheader[14] = { 'B', 'M', 0,0,0,0, 0,0, 0,0, 54,0,0,0 };
	                                   // 0x06-0x09 are reserved and not used by the definition, store the offset of where the file ends
	unsigned char bmpinfoheader[40] = { 40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0,   24,0,      0,0,0,0, 0,0,0,0,  0x13,0x0B,0,0, 0x13,0x0B,0,0, 0,0,0,0, 0,0,0,0 };
	                                   // size      width    height   plane  bit depth           rawsize   DPI horiz      DPI vert       palette  imp colors

	unsigned int rawsize = (w * 3) * h;
	unsigned int bmpsize = 54 + rawsize;

	bmpfileheader[2] = (unsigned char)(bmpsize);
	bmpfileheader[3] = (unsigned char)(bmpsize>>8);
	bmpfileheader[4] = (unsigned char)(bmpsize>>16);
	bmpfileheader[5] = (unsigned char)(bmpsize>>24);

	// Not part of the definition, this tells the "decoder" where the file stops within the image
	bmpfileheader[6] = (unsigned char)(fileSize);
	bmpfileheader[7] = (unsigned char)(fileSize>>8);
	bmpfileheader[8] = (unsigned char)(fileSize>>16);
	bmpfileheader[9] = (unsigned char)(fileSize>>24);

	bmpinfoheader[4] = (unsigned char)(w);
	bmpinfoheader[5] = (unsigned char)(w>>8);
	bmpinfoheader[6] = (unsigned char)(w>>16);
	bmpinfoheader[7] = (unsigned char)(w>>24);

	bmpinfoheader[8] = (unsigned char)(h);
	bmpinfoheader[9] = (unsigned char)(h>>8);
	bmpinfoheader[10] = (unsigned char)(h>>16);
	bmpinfoheader[11] = (unsigned char)(h>>24);

	bmpinfoheader[20] = (unsigned char)(rawsize);
	bmpinfoheader[21] = (unsigned char)(rawsize>>8);
	bmpinfoheader[22] = (unsigned char)(rawsize>>16);
	bmpinfoheader[23] = (unsigned char)(rawsize>>24);

	fwrite(bmpfileheader, 1, 14, writeFile);
	fwrite(bmpinfoheader, 1, 40, writeFile);

	for(unsigned int x = 0; x < fileSize; x++)
	{
		unsigned char xc1 =  ((filecontents[x]       & 0x07) + ((unsigned char)(blendContents[x*3]     * divisor))) % 255;
		unsigned char xc2 = (((filecontents[x] >> 3) & 0x03) + ((unsigned char)(blendContents[(x*3)+1] * divisor))) % 255;
		unsigned char xc3 = (((filecontents[x] >> 5) & 0x07) + ((unsigned char)(blendContents[(x*3)+2] * divisor))) % 255;

		fwrite(&xc1, 1, 1, writeFile);
		fwrite(&xc2, 1, 1, writeFile);
		fwrite(&xc3, 1, 1, writeFile);
	}

	leftOvers = fileSize * 3;

	for(unsigned int x = 0; x < (rawsize - leftOvers) - 1; x++)
	//for(unsigned int x = 0; x < (blendSize - rawsize); x++)
	{
		if(leftOvers + x > blendSize)
		{ 
			printf("\n Overflow at: %d\n", leftOvers + x);
			break;
		}
		unsigned char xc = blendContents[leftOvers + x];
		fwrite(&xc, 1, 1, writeFile);
	}

	fclose(writeFile);
	free(blendContents);
	free(filecontents);

	return 0;
}