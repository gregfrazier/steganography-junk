#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifndef _MSC_VER
#define fopen_s(pFile,filename,mode) ((*(pFile))=fopen((filename),(mode)))==NULL
#endif

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

	fread(&bitmapFileHeader, sizeof(unsigned char) * 14, 1, TARGET);
	fread(&bitmapInfoHeader, sizeof(unsigned char) * 40, 1, TARGET);

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
	h = (long) rawsize - 54;

	printf("%d\n", rawsize);

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
	long fileSize, stenoSize;
	unsigned int w, h;
	FILE *writeFile;
	unsigned char xorChar = 0x3d;
	unsigned char *filecontents = 0;
	unsigned char *stenoContents = 0;

	filecontents = readBMP(argv[1], &fileSize);
	if(filecontents == 0)
	{
		printf("main(): bad bmp file\n");
		return -1;
	}

	stenoContents = readBMP(argv[2], &stenoSize);
	if(stenoContents == 0)
	{
		printf("main(): bad steno bmp file\n");
		return -1;
	}

	fopen_s(&writeFile, argv[3], "wb");
	if(!writeFile)
	{
		printf("main(): bad output file\n");
		return -1;
	}

	double divisor = 0.90;

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
}