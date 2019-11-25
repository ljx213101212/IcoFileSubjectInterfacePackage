#include "pch.h"
#include "IcoFileInfo.h"


#define BUFFER_SIZE 0x20000
#define PNG_SIG_SIZE 8

//Declaration of methods. 
UINT ReadICOHeader(HANDLE hFile);
DWORD  GetFilePointer(HANDLE hFile);
BOOL CheckPNGSignature(LPBYTE chunkData);

DWORD  GetFilePointer(HANDLE hFile) {
	return SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
}
BOOL CheckPNGSignature(LPBYTE chunkData) {
	BYTE bufferSIG[PNG_SIG_SIZE];
	const BYTE PNGSIG[PNG_SIG_SIZE] = { 0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A };
	memcpy_s(&bufferSIG, PNG_SIG_SIZE, chunkData, PNG_SIG_SIZE);
	return (0 == memcmp(bufferSIG, PNGSIG, PNG_SIG_SIZE));
}

BOOL IcoFileInfo::GetIcoFileInfo(HANDLE hFile, LPCTSTR szFileName, ICO_FILE_INFO *info){

	LPICONRESOURCE    	lpIR = NULL, lpNew = NULL;

	LPRESOURCEPOSINFO	lpRPI = NULL;
	UINT                i;
	DWORD            	dwBytesRead;
	LPICONDIRENTRY    	lpIDE = NULL;

	if (info == nullptr) { return NULL; }
	// Open the file
	if (hFile == nullptr)
	{
		//MessageBox(hWndMain, TEXT("Error Opening File for Reading"), szFileName, MB_OK);
		return NULL;
	}
	info->beforePNGStartPosition = GetFilePointer(hFile);
	// Allocate memory for the resource structure
	if ((lpIR = (LPICONRESOURCE)malloc(sizeof(ICONRESOURCE))) == NULL)
	{
		//MessageBox(hWndMain, TEXT("Error Allocating Memory"), szFileName, MB_OK);
		CloseHandle(hFile);
		return NULL;
	}
	// Read in the header
	if ((lpIR->nNumImages = ReadICOHeader(hFile)) == (UINT)-1)
	{
		//MessageBox(hWndMain, TEXT("Error Reading File Header"), szFileName, MB_OK);
		CloseHandle(hFile);
		free(lpIR);
		return NULL;
	}
	// Adjust the size of the struct to account for the images
	if ((lpNew = (LPICONRESOURCE)realloc(lpIR, sizeof(ICONRESOURCE) + ((lpIR->nNumImages - 1) * sizeof(ICONIMAGE)))) == NULL)
	{
		//MessageBox(hWndMain, TEXT("Error Allocating Memory"), szFileName, MB_OK);
		CloseHandle(hFile);
		free(lpIR);
		return NULL;
	}
	lpIR = lpNew;
	// Store the original name
	lstrcpy(lpIR->szOriginalICOFileName, szFileName);
	lstrcpy(lpIR->szOriginalDLLFileName, TEXT(""));
	// Allocate enough memory for the icon directory entries
	if ((lpIDE = (LPICONDIRENTRY)malloc(lpIR->nNumImages * sizeof(ICONDIRENTRY))) == NULL)
	{
		//MessageBox(hWndMain, TEXT("Error Allocating Memory"), szFileName, MB_OK);
		CloseHandle(hFile);
		free(lpIR);
		return NULL;
	}
	// Read in the icon directory entries
	if (!ReadFile(hFile, lpIDE, lpIR->nNumImages * sizeof(ICONDIRENTRY), &dwBytesRead, NULL))
	{
		//MessageBox(hWndMain, TEXT("Error Reading File"), szFileName, MB_OK);
		CloseHandle(hFile);
		free(lpIR);
		return NULL;
	}
	if (dwBytesRead != lpIR->nNumImages * sizeof(ICONDIRENTRY))
	{
		//MessageBox(hWndMain, TEXT("Error Reading File"), szFileName, MB_OK);
		CloseHandle(hFile);
		free(lpIR);
		return NULL;
	}

	// Loop through and read in each image
	for (i = 0; i < lpIR->nNumImages; i++)
	{
		// Allocate memory for the resource
		if ((lpIR->IconImages[i].lpBits = (LPBYTE)malloc(lpIDE[i].dwBytesInRes)) == NULL)
		{
			//MessageBox(hWndMain, TEXT("Error Allocating Memory"), szFileName, MB_OK);
			CloseHandle(hFile);
			free(lpIR);
			free(lpIDE);
			return NULL;
		}

		lpIR->IconImages[i].dwNumBytes = lpIDE[i].dwBytesInRes;
		// Seek to beginning of this image
		if (SetFilePointer(hFile, lpIDE[i].dwImageOffset, NULL, FILE_BEGIN) == 0xFFFFFFFF)
		{
			//MessageBox(hWndMain, TEXT("Error Seeking in File"), szFileName, MB_OK);
			CloseHandle(hFile);
			free(lpIR);
			free(lpIDE);
			return NULL;
		}

		// Read it in
		if (!ReadFile(hFile, lpIR->IconImages[i].lpBits, lpIDE[i].dwBytesInRes, &dwBytesRead, NULL))
		{
			//MessageBox(hWndMain, TEXT("Error Reading File"), szFileName, MB_OK);
			CloseHandle(hFile);
			free(lpIR);
			free(lpIDE);
			return NULL;
		}

		//check PNG SIG
		if (CheckPNGSignature(lpIR->IconImages[i].lpBits)) {
			info->afterPNGStartPosition = GetFilePointer(hFile);
			info->PNGStartPosition = info->afterPNGStartPosition - dwBytesRead;
		}
		if (dwBytesRead != lpIDE[i].dwBytesInRes)
		{
			//MessageBox(hWndMain, TEXT("Error Reading File"), szFileName, MB_OK);
			CloseHandle(hFile);
			free(lpIDE);
			free(lpIR);
			return NULL;
		}
		// Set the internal pointers appropriately
		if (!AdjustIconImagePointers(&(lpIR->IconImages[i])))
		{
			//MessageBox(hWndMain, TEXT("Error Converting to Internal Format"), szFileName, MB_OK);
			CloseHandle(hFile);
			free(lpIDE);
			free(lpIR);
			return NULL;
		}
	}
	info->fileEndPosition = GetFilePointer(hFile);

	/*SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
	BYTE testData[BUFFER_SIZE];*/
	//ReadFile(hFile, &testData, info.PNGStartPosition - info.beforePNGStartPosition, &dwBytesRead, NULL);
	//ReadFile(hFile, &testData, info.afterPNGStartPosition - info.PNGStartPosition, &dwBytesRead, NULL);
	//ReadFile(hFile, &testData, info.fileEndPosition - info.afterPNGStartPosition, &dwBytesRead, NULL);
	// Clean up	
	free(lpIDE);
	free(lpRPI);
	CloseHandle(hFile);
	return true;
}
UINT ReadICOHeader(HANDLE hFile)
{
	WORD    Input;
	DWORD	dwBytesRead;

	// Read the 'reserved' WORD
	if (!ReadFile(hFile, &Input, sizeof(WORD), &dwBytesRead, NULL))
		return (UINT)-1;
	// Did we get a WORD?
	if (dwBytesRead != sizeof(WORD))
		return (UINT)-1;
	// Was it 'reserved' ?   (ie 0)
	if (Input != 0)
		return (UINT)-1;
	// Read the type WORD
	if (!ReadFile(hFile, &Input, sizeof(WORD), &dwBytesRead, NULL))
		return (UINT)-1;
	// Did we get a WORD?
	if (dwBytesRead != sizeof(WORD))
		return (UINT)-1;
	// Was it type 1?
	if (Input != 1)
		return (UINT)-1;
	// Get the count of images
	if (!ReadFile(hFile, &Input, sizeof(WORD), &dwBytesRead, NULL))
		return (UINT)-1;
	// Did we get a WORD?
	if (dwBytesRead != sizeof(WORD))
		return (UINT)-1;
	// Return the count
	return Input;
}


BOOL AdjustIconImagePointers(LPICONIMAGE lpImage)
{
	// Sanity check
	if (lpImage == NULL)
		return FALSE;
	// BITMAPINFO is at beginning of bits
	lpImage->lpbi = (LPBITMAPINFO)lpImage->lpBits;
	// Width - simple enough
	lpImage->Width = lpImage->lpbi->bmiHeader.biWidth;
	// Icons are stored in funky format where height is doubled - account for it
	lpImage->Height = (lpImage->lpbi->bmiHeader.biHeight) / 2;
	// How many colors?
	lpImage->Colors = lpImage->lpbi->bmiHeader.biPlanes * lpImage->lpbi->bmiHeader.biBitCount;
	// XOR bits follow the header and color table
	lpImage->lpXOR = (LPBYTE)FindDIBBits((LPSTR)lpImage->lpbi);
	// AND bits follow the XOR bits
	lpImage->lpAND = lpImage->lpXOR + (lpImage->Height * BytesPerLine((LPBITMAPINFOHEADER)(lpImage->lpbi)));
	return TRUE;
}


LPSTR FindDIBBits(LPSTR lpbi)
{
	return (lpbi + *(LPDWORD)lpbi + PaletteSize(lpbi));
}

WORD DIBNumColors(LPSTR lpbi)
{
	WORD wBitCount;
	DWORD dwClrUsed;

	dwClrUsed = ((LPBITMAPINFOHEADER)lpbi)->biClrUsed;

	if (dwClrUsed)
		return (WORD)dwClrUsed;

	wBitCount = ((LPBITMAPINFOHEADER)lpbi)->biBitCount;

	switch (wBitCount)
	{
	case 1: return 2;
	case 4: return 16;
	case 8:	return 256;
	default:return 0;
	}
	return 0;
}

WORD PaletteSize(LPSTR lpbi)
{
	return (DIBNumColors(lpbi) * sizeof(RGBQUAD));
}

DWORD BytesPerLine(LPBITMAPINFOHEADER lpBMIH)
{
	return WIDTHBYTES(lpBMIH->biWidth * lpBMIH->biPlanes * lpBMIH->biBitCount);
}