#include "pch.h"
#include "IcoFileInfo.h"
#include "FileUtil.h"

#define BUFFER_SIZE 0x10000

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
	MyUtility::ResetFilePointer(hFile);
	info->beforePNGStartPosition = GetFilePointer(hFile);
	// Allocate memory for the resource structure
	if ((lpIR = (LPICONRESOURCE)malloc(sizeof(ICONRESOURCE))) == NULL)
	{
		//MessageBox(hWndMain, TEXT("Error Allocating Memory"), szFileName, MB_OK);
		//CloseHandle(hFile);
		return NULL;
	}
	// Read in the header
	if ((lpIR->nNumImages = ReadICOHeader(hFile)) == (UINT)-1)
	{
		//MessageBox(hWndMain, TEXT("Error Reading File Header"), szFileName, MB_OK);
		//CloseHandle(hFile);
		free(lpIR);
		return NULL;
	}
	// Adjust the size of the struct to account for the images
	if ((lpNew = (LPICONRESOURCE)realloc(lpIR, sizeof(ICONRESOURCE) + ((lpIR->nNumImages - 1) * sizeof(ICONIMAGE)))) == NULL)
	{
		//MessageBox(hWndMain, TEXT("Error Allocating Memory"), szFileName, MB_OK);
		//CloseHandle(hFile);
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
		//CloseHandle(hFile);
		free(lpIR);
		return NULL;
	}
	// Read in the icon directory entries
	if (!ReadFile(hFile, lpIDE, lpIR->nNumImages * sizeof(ICONDIRENTRY), &dwBytesRead, NULL))
	{
		//MessageBox(hWndMain, TEXT("Error Reading File"), szFileName, MB_OK);
		//CloseHandle(hFile);
		free(lpIR);
		return NULL;
	}
	if (dwBytesRead != lpIR->nNumImages * sizeof(ICONDIRENTRY))
	{
		//MessageBox(hWndMain, TEXT("Error Reading File"), szFileName, MB_OK);
		//CloseHandle(hFile);
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
			//CloseHandle(hFile);
			free(lpIR);
			free(lpIDE);
			return NULL;
		}

		lpIR->IconImages[i].dwNumBytes = lpIDE[i].dwBytesInRes;
		// Seek to beginning of this image
		if (SetFilePointer(hFile, lpIDE[i].dwImageOffset, NULL, FILE_BEGIN) == 0xFFFFFFFF)
		{
			//MessageBox(hWndMain, TEXT("Error Seeking in File"), szFileName, MB_OK);
			//CloseHandle(hFile);
			free(lpIR);
			free(lpIDE);
			return NULL;
		}

		// Read it in
		if (!ReadFile(hFile, lpIR->IconImages[i].lpBits, lpIDE[i].dwBytesInRes, &dwBytesRead, NULL))
		{
			//MessageBox(hWndMain, TEXT("Error Reading File"), szFileName, MB_OK);
			//CloseHandle(hFile);
			free(lpIR);
			free(lpIDE);
			return NULL;
		}

		//check PNG SIG
		if (CheckPNGSignature(lpIR->IconImages[i].lpBits)) {
			info->afterPNGStartPosition = GetFilePointer(hFile);
			info->PNGStartPosition = info->afterPNGStartPosition - dwBytesRead;
			info->nthImageIsPng = i + 1;
			info->numOfIco = lpIR->nNumImages;
			info->pngChunkOffset = lpIDE[i].dwImageOffset;
			info->pngChunkSize = lpIDE[i].dwBytesInRes;
			UpdateSignaturePosition(lpIR->IconImages[i].lpBits, info->pngChunkSize, info->pngChunkOffset, PNG_SIG_CHUNK_TYPE, info);
			UpdateITxtTupleList(i, lpIR->IconImages[i].lpBits, info->pngChunkOffset, info->pngChunkSize, PNG_SIG_CHUNK_TYPE, info);
		}
		if (dwBytesRead != lpIDE[i].dwBytesInRes)
		{
			//MessageBox(hWndMain, TEXT("Error Reading File"), szFileName, MB_OK);
			//CloseHandle(hFile);
			free(lpIDE);
			free(lpIR);
			return NULL;
		}
		// Set the internal pointers appropriately
		if (!AdjustIconImagePointers(&(lpIR->IconImages[i])))
		{
			//MessageBox(hWndMain, TEXT("Error Converting to Internal Format"), szFileName, MB_OK);
			//CloseHandle(hFile);
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
	//CloseHandle(hFile);
	MyUtility::ResetFilePointer(hFile);
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

BOOL IcoFileInfo::EraseITxtChunk(HANDLE hFile, DWORD pngIndex, DWORD numOfIco, DWORD pngChunkOffset, DWORD pngChunkSize, DWORD iTXtOffset, DWORD iTXtSize, ICO_FILE_INFO* info)
{
	DWORD start = iTXtOffset + iTXtSize;
	DWORD end = info->fileEndPosition;
	MyUtility::MoveBytesFromFileEndToFileLeft(hFile, start, end, iTXtSize);
	MyUtility::ShrinkFile(hFile, iTXtSize);
	//update fileEndPosition
	info->fileEndPosition = info->fileEndPosition - iTXtSize;
	////update header
	UpdateIcoHeaderByPngIndex(hFile, pngIndex, numOfIco , iTXtSize, false);
	return true;
}

BOOL IcoFileInfo::EraseAllITxtChunk(HANDLE hFile, ICO_FILE_INFO* info)
{
	std::vector<std::tuple<DWORD, DWORD, DWORD, DWORD, DWORD>>& iTXtList = info->iTXtList;
	for (std::tuple<DWORD, DWORD, DWORD, DWORD, DWORD> iTXt : iTXtList) {
		DWORD pngIndex = std::get<0>(iTXt);
		DWORD pngOffset = std::get<1>(iTXt);
		DWORD pngSize = std::get<2>(iTXt);
		DWORD iTXtOffset = std::get<3>(iTXt);
		DWORD iTXtSize = std::get<4>(iTXt);

		EraseITxtChunk(hFile,pngIndex, info->numOfIco, pngOffset, pngSize, iTXtOffset, iTXtSize, info);
	}
	return true;
}

BOOL IcoFileInfo::UpdateITxtTupleList(DWORD pngIndex, LPBYTE pngChunk,  DWORD pngChunkOffset, DWORD pngChunkSize, const char* signature, ICO_FILE_INFO* info)
{
	DWORD bytesRead = 0;
	BYTE buffer[PNG_CHUNK_HEADER_SIZE];
	DWORD pngChunkPtr = 0;
	DWORD sigChunkOffset = 0;

	pngChunkPtr += PNG_CHUNK_HEADER_SIZE;
	while (pngChunkPtr < pngChunkSize) {
		memcpy_s(&buffer, PNG_CHUNK_HEADER_SIZE, pngChunk + pngChunkPtr, PNG_CHUNK_HEADER_SIZE);
		const unsigned int size = buffer[3] | buffer[2] << 8 | buffer[1] << 16 | buffer[0] << 24;
		const unsigned char* tag = ((const unsigned char*)&buffer[4]);
		if (0 == memcmp(tag, signature, PNG_TAG_SIZE))
		{
			info->sigChunkOffset = pngChunkOffset + pngChunkPtr;
			info->sigChunkSize = PNG_CHUNK_HEADER_SIZE + size + PNG_CRC_SIZE;
			info->iTXtList.push_back(std::make_tuple(pngIndex, pngChunkOffset, pngChunkSize, info->sigChunkOffset, info->sigChunkSize));
		}
		pngChunkPtr += PNG_CHUNK_HEADER_SIZE + size + PNG_CRC_SIZE;
	}
	return NULL;
}

BOOL IcoFileInfo::UpdateSignaturePosition(LPBYTE pngChunk, DWORD pngChunkSize, DWORD pngChunkOffset, const char* signature, ICO_FILE_INFO* info)
{
	DWORD bytesRead = 0;
	BYTE buffer[PNG_CHUNK_HEADER_SIZE];
	DWORD pngChunkPtr = 0;
	DWORD sigChunkOffset = 0;

	pngChunkPtr += PNG_CHUNK_HEADER_SIZE;
	while (pngChunkPtr < pngChunkSize) {
		memcpy_s(&buffer, PNG_CHUNK_HEADER_SIZE, pngChunk + pngChunkPtr, PNG_CHUNK_HEADER_SIZE);
		const unsigned int size = buffer[3] | buffer[2] << 8 | buffer[1] << 16 | buffer[0] << 24;
		const unsigned char* tag = ((const unsigned char*)&buffer[4]);
		if (0 == memcmp(tag, signature, PNG_TAG_SIZE))
		{
			info->sigChunkOffset = pngChunkOffset + pngChunkPtr;
			info->sigChunkSize = PNG_CHUNK_HEADER_SIZE + size + PNG_CRC_SIZE;
			return true;
		}
		pngChunkPtr += PNG_CHUNK_HEADER_SIZE + size + PNG_CRC_SIZE;
	}
	return NULL;
}

VOID GetSignatureSize(LPBYTE pngChunk, DWORD pngChunkSize, const char* signature, DWORD* signatureSize) 
{
	DWORD bytesRead = 0;
	BYTE buffer[PNG_CHUNK_HEADER_SIZE];
	DWORD pngChunkPtr = 0;

	pngChunkPtr += PNG_CHUNK_HEADER_SIZE;
	while (pngChunkPtr < pngChunkSize) {
		memcpy_s(&buffer, PNG_CHUNK_HEADER_SIZE, pngChunk + pngChunkPtr, PNG_CHUNK_HEADER_SIZE);
		const unsigned int size = buffer[3] | buffer[2] << 8 | buffer[1] << 16 | buffer[0] << 24;
		const unsigned char* tag = ((const unsigned char*)&buffer[4]);
		if (0 == memcmp(tag, signature, PNG_TAG_SIZE))
		{
			*signatureSize = PNG_CHUNK_HEADER_SIZE + size + PNG_CRC_SIZE;
			break;
		}
		pngChunkPtr += PNG_CHUNK_HEADER_SIZE + size + PNG_CRC_SIZE;
	}
}

BOOL IcoFileInfo::UpdateIcoHeaderByPngIndex(HANDLE hFile, DWORD pngIndex, DWORD numOfIco, DWORD updateSize, BOOL IsIncrease)
{
	LONG switchFatcor = IsIncrease ? 1 : -1;
	MyUtility::ResetFilePointer(hFile);
	//update png header size (little endian by default)
	BYTE buffer[ICO_SIZE_OF_DATA_SIZE];
	DWORD bytesRead = 0;
	DWORD bytesWrite = 0;
	DWORD pngHeaderSizeOffset = ICO_HEADER_SIZE + pngIndex * ICO_DIRECTORY_CHUNK_SIZE + ICO_OFFSET_OF_DATA_SIZE_IN_HEADER;
	if (!SetFilePointer(hFile, pngHeaderSizeOffset, NULL, FILE_BEGIN)) {
		return false;
	}
	if (!::ReadFile(hFile, &buffer, ICO_SIZE_OF_DATA_SIZE, &bytesRead, NULL)) {
		return false;
	}
	LONG size = buffer[0] | buffer[1] << 8 | buffer[2] << 16 | buffer[3] << 24;
	size += ((LONG)updateSize * switchFatcor);
	//little endian
	buffer[0] = size & 0xFF;
	buffer[1] = (size >> 8) & 0xFF;
	buffer[2] = (size >> 16) & 0xFF;
	buffer[3] = (size >> 24) & 0xFF;
	//Update file pointer 
	if (!SetFilePointer(hFile, -ICO_SIZE_OF_DATA_SIZE, NULL, FILE_CURRENT)) {
		return false;
	}
	//Update current png chunk size
	if (!::WriteFile(hFile, &buffer, ICO_SIZE_OF_DATA_SIZE, &bytesWrite, NULL)) {
		return false;
	}
	if (!SetFilePointer(hFile, 4, NULL, FILE_CURRENT)) {
		return false;
	}
	//update rest of BMP file offset.
	INT32 restNumOfBMPFile = numOfIco - (pngIndex + 1);
	for (int i = 0; i < restNumOfBMPFile; i++) {
		if (!SetFilePointer(hFile, ICO_PNG_LEFT_OVER_BMP_OFFSET, NULL, FILE_CURRENT)) {
			return false;
		}
		BYTE offsetBuffer[ICO_SIZE_OF_DATA_OFFSET];
		if (!::ReadFile(hFile, &offsetBuffer, ICO_SIZE_OF_DATA_OFFSET, &bytesRead, NULL)) {
			return false;
		}
		LONG offset = offsetBuffer[0] | offsetBuffer[1] << 8 | offsetBuffer[2] << 16 | offsetBuffer[3] << 24;
		offset += ((LONG)updateSize * switchFatcor);
		//little endian
		buffer[0] = offset & 0xFF;
		buffer[1] = (offset >> 8) & 0xFF;
		buffer[2] = (offset >> 16) & 0xFF;
		buffer[3] = (offset >> 24) & 0xFF;
		//Update file pointer 
		if (!SetFilePointer(hFile, -ICO_SIZE_OF_DATA_OFFSET, NULL, FILE_CURRENT)) {
			return false;
		}
		bytesWrite = 0;
		if (!::WriteFile(hFile, &buffer, ICO_SIZE_OF_DATA_OFFSET, &bytesWrite, NULL)) {
			return false;
		}
	}
	return true;
}
BOOL IcoFileInfo::UpdateIcoHeader(HANDLE hFile, DWORD signatureSize, BOOL IsOriginToUpdate)
{
	ICO_FILE_INFO info;
	IcoFileInfo icoFileInfo;
	LONG switchFatcor = IsOriginToUpdate ? 1 : -1;
	DWORD bytesWrite = 0;
	icoFileInfo.GetIcoFileInfo(hFile, TEXT(""), &info);
	MyUtility::ResetFilePointer(hFile);
	DWORD pngHeaderSizeOffset = ICO_HEADER_SIZE + (info.nthImageIsPng - 1) * ICO_DIRECTORY_CHUNK_SIZE + ICO_OFFSET_OF_DATA_SIZE_IN_HEADER;
	if (!SetFilePointer(hFile, pngHeaderSizeOffset, NULL, FILE_BEGIN)) {
		return false;
	}
	//update png header size (little endian by default)
	BYTE buffer[ICO_SIZE_OF_DATA_SIZE];
	DWORD bytesRead = 0;
	if (!::ReadFile(hFile, &buffer, ICO_SIZE_OF_DATA_SIZE, &bytesRead, NULL)) {
		return false;
	}
	LONG size = buffer[0] | buffer[1] << 8 | buffer[2] << 16 | buffer[3] << 24;
	size += ((LONG)signatureSize * switchFatcor);
	//little endian
	buffer[0] = size & 0xFF;
	buffer[1] = (size >> 8) & 0xFF;
	buffer[2] = (size >> 16) & 0xFF;
	buffer[3] = (size >> 24) & 0xFF;

	//Update file pointer 
	if (!SetFilePointer(hFile, -ICO_SIZE_OF_DATA_SIZE, NULL, FILE_CURRENT)) {
		return false;
	}

	if (!::WriteFile(hFile, &buffer, ICO_SIZE_OF_DATA_SIZE, &bytesWrite, NULL)) {
		return false;
	}

	if (!SetFilePointer(hFile, 4, NULL, FILE_CURRENT)) {
		return false;
	}
	//update rest of BMP file offset.
	INT32 restNumOfBMPFile = info.numOfIco - info.nthImageIsPng;
	for (int i = 0; i < restNumOfBMPFile; i++) {
		if (!SetFilePointer(hFile, ICO_PNG_LEFT_OVER_BMP_OFFSET, NULL, FILE_CURRENT)) {
			return false;
		}
		BYTE offsetBuffer[ICO_SIZE_OF_DATA_OFFSET];
		if (!::ReadFile(hFile, &offsetBuffer, ICO_SIZE_OF_DATA_OFFSET, &bytesRead, NULL)) {
			return false;
		}
		LONG offset = offsetBuffer[0] | offsetBuffer[1] << 8 | offsetBuffer[2] << 16 | offsetBuffer[3] << 24;
		offset += ((LONG)signatureSize * switchFatcor);
		//little endian
		buffer[0] = offset & 0xFF;
		buffer[1] = (offset >> 8) & 0xFF;
		buffer[2] = (offset >> 16) & 0xFF;
		buffer[3] = (offset >> 24) & 0xFF;
		//Update file pointer 
		if (!SetFilePointer(hFile, -ICO_SIZE_OF_DATA_OFFSET, NULL, FILE_CURRENT)) {
			return false;
		}
		bytesWrite = 0;
		if (!::WriteFile(hFile, &buffer, ICO_SIZE_OF_DATA_OFFSET, &bytesWrite, NULL)) {
			return false;
		}
	}
	return true;
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



BOOL GetPngChunk(HANDLE hFile, DWORD pngChunkSize, DWORD startPNG, DWORD endPNG, LPBYTE pngChunk) 
{
	//check
	DWORD t_pngChunkSize = endPNG - startPNG;
	if (t_pngChunkSize != pngChunkSize) { return NULL; }

	MyUtility::ResetFilePointer(hFile);
	SetFilePointer(hFile, startPNG, NULL, FILE_BEGIN);
	DWORD dwBytesRead = 0;
	if (!ReadFile(hFile, pngChunk, pngChunkSize, &dwBytesRead, NULL)) {
		return NULL;
	}
	return true;
}