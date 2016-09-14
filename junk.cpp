#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define fopen_s(pFile,filename,mode) ((*(pFile))=fopen((filename),(mode)))==NULL

double calcLuma(unsigned char red, unsigned char green, unsigned char blue)
{
	double nits;
	nits = (.27 * (unsigned char)red) + (.67 * (unsigned char)green) + (.06 * (unsigned char)blue);
	return nits;
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
	h = (long) rawsize - 54;

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
	unsigned int w, h, m = 0;
	FILE *writeFile;
	unsigned char *filecontents = 0;
	unsigned char *stenoContents = 0;
	unsigned char xorChar = 0x3d;

	filecontents = loadFile(argv[1], &fileSize);
	if(!filecontents)
	{
		printf("main(): bad file\n");
		return -1;
	}

	stenoContents = readBMP(argv[2], &stenoSize);
	if(!stenoContents)
	{
		printf("main(): bad steno file\n");
		return -1;
	}

	fopen_s(&writeFile, argv[3], "wb");
	if(!writeFile)
	{
		printf("main(): bad output file\n");
		return -1;
	}

	unsigned char bmpfileheader[14] = { 'B', 'M', 0,0,0,0, 0,0, 0,0, 54,0,0,0 };
	unsigned char bmpinfoheader[40] = { 40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0,   24,0,      0,0,0,0, 0,0,0,0,  0x13,0x0B,0,0, 0x13,0x0B,0,0, 0,0,0,0, 0,0,0,0 };
	//                                 // size      width    height   plane  bit depth           rawsize   DPI horiz      DPI vert       palette  imp colors

	unsigned int sqrtAct = (unsigned int)ceil(sqrt(fileSize + m));
	w = h = (sqrtAct + (4 - (sqrtAct % 4)));

	//unsigned int sqrtNormal = (unsigned int)ceil(sqrt((sqrtAct * sqrtAct) / 3));
	//w = h = (sqrtNormal + (4 - (sqrtNormal % 4))) * 3;

	printf("%d\n", w);

	unsigned int rawsize = (w * 3) * h;
	unsigned int bmpsize = 54 + rawsize;

	bmpfileheader[2] = (unsigned char)(bmpsize);
	bmpfileheader[3] = (unsigned char)(bmpsize>>8);
	bmpfileheader[4] = (unsigned char)(bmpsize>>16);
	bmpfileheader[5] = (unsigned char)(bmpsize>>24);

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
		// green/yellow dominates high intensity whitish colors, trying to combat that.
		int luma = (int)(calcLuma(stenoContents[(x*3)+2], stenoContents[(x*3)+1], stenoContents[x*3]) + 0.5);
		double divisor = 1;

		if(luma >= 180 || stenoContents[(x*3)+1] >= 180)
			divisor = 0.90;
		
		unsigned char xc1 = ((filecontents[x] & 0x0F) + ((unsigned char)(stenoContents[x*3] * divisor))) % 255;
		unsigned char xc2 = ((unsigned char)(stenoContents[(x*3)+1] * divisor));
		unsigned char xc3 = (((filecontents[x] >> 4) & 0x0F) + ((unsigned char)(stenoContents[(x*3)+2] * divisor))) % 255;

		fwrite(&xc1, 1, 1, writeFile);
		fwrite(&xc2, 1, 1, writeFile);
		fwrite(&xc3, 1, 1, writeFile);
	}

	for(unsigned int x = 0; x < rawsize - (fileSize * 3); x++)
	{
		unsigned char xc = stenoContents[(fileSize * 3) + x];
		fwrite(&xc, 1, 1, writeFile);
	}

	fclose(writeFile);
}