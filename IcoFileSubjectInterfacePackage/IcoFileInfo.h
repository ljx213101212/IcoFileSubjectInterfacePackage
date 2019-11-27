#pragma once
#include "pch.h"
#define WIDTHBYTES(bits)      ((((bits) + 31)>>5)<<2)





typedef struct ICO_FILE_INFO_
{
	LONG beforePNGStartPosition;
	LONG PNGStartPosition;
	LONG afterPNGStartPosition;
	LONG fileEndPosition;
	LONG nthImageIsPng;
	LONG numOfIco;
} ICO_FILE_INFO;

typedef struct
{
	DWORD	dwBytes;
	DWORD	dwOffset;
} RESOURCEPOSINFO, * LPRESOURCEPOSINFO;

typedef struct
{
	BYTE	bWidth;               // Width of the image
	BYTE	bHeight;              // Height of the image (times 2)
	BYTE	bColorCount;          // Number of colors in image (0 if >=8bpp)
	BYTE	bReserved;            // Reserved
	WORD	wPlanes;              // Color Planes
	WORD	wBitCount;            // Bits per pixel
	DWORD	dwBytesInRes;         // how many bytes in this resource?
	WORD	nID;                  // the ID
} MEMICONDIRENTRY, * LPMEMICONDIRENTRY;
typedef struct
{
	WORD			idReserved;   // Reserved
	WORD			idType;       // resource type (1 for icons)
	WORD			idCount;      // how many images?
	MEMICONDIRENTRY	idEntries[1]; // the entries for each image
} MEMICONDIR, * LPMEMICONDIR;

// These next two structs represent how the icon information is stored
// in an ICO file.
typedef struct
{
	BYTE	bWidth;               // Width of the image
	BYTE	bHeight;              // Height of the image (times 2)
	BYTE	bColorCount;          // Number of colors in image (0 if >=8bpp)
	BYTE	bReserved;            // Reserved
	WORD	wPlanes;              // Color Planes
	WORD	wBitCount;            // Bits per pixel
	DWORD	dwBytesInRes;         // how many bytes in this resource?
	DWORD	dwImageOffset;        // where in the file is this image
} ICONDIRENTRY, * LPICONDIRENTRY;
typedef struct
{
	WORD			idReserved;   // Reserved
	WORD			idType;       // resource type (1 for icons)
	WORD			idCount;      // how many images?
	ICONDIRENTRY	idEntries[1]; // the entries for each image
} ICONDIR, * LPICONDIR;

// The following two structs are for the use of this program in
// manipulating icons. They are more closely tied to the operation
// of this program than the structures listed above. One of the
// main differences is that they provide a pointer to the DIB
// information of the masks.
typedef struct
{
	UINT			Width, Height, Colors; // Width, Height and bpp
	LPBYTE			lpBits;                // ptr to DIB bits
	DWORD			dwNumBytes;            // how many bytes?
	LPBITMAPINFO	lpbi;                  // ptr to header
	LPBYTE			lpXOR;                 // ptr to XOR image bits
	LPBYTE			lpAND;                 // ptr to AND image bits
} ICONIMAGE, * LPICONIMAGE;
typedef struct
{
	BOOL		bHasChanged;                     // Has image changed?
	TCHAR		szOriginalICOFileName[MAX_PATH]; // Original name
	TCHAR		szOriginalDLLFileName[MAX_PATH]; // Original name
	UINT		nNumImages;                      // How many images?
	ICONIMAGE	IconImages[1];                   // Image entries
} ICONRESOURCE, * LPICONRESOURCE;
/****************************************************************************/

DWORD BytesPerLine(LPBITMAPINFOHEADER lpBMIH);
LPSTR FindDIBBits(LPSTR lpbi);
WORD DIBNumColors(LPSTR lpbi);
WORD PaletteSize(LPSTR lpbi);
BOOL AdjustIconImagePointers(LPICONIMAGE lpImage);

class IcoFileInfo {

public:
	BOOL GetIcoFileInfo(HANDLE hFile, LPCTSTR szFileName, ICO_FILE_INFO* info);
	BOOL UpdateIcoHeader(HANDLE hFile, DWORD signatureSize, BOOL IsOriginToUpdate);
	VOID GetSignatureSize(LPBYTE pngChunk, DWORD pngChunkSize, const char* signature, DWORD* signatureSize);
};