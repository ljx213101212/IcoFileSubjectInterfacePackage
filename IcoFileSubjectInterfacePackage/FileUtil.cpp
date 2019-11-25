#include "pch.h"
#include "FileUtil.h"

#define BUFFER_SIZE 0x10000
#define CHUNK_SIZE 0x400
namespace MyUtility {
	DWORD  GetFilePointer(HANDLE hFile) {
		return SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
	}

	void ExpandFile(HANDLE hFile, DWORD expandSize) {
		if (SetFilePointer(hFile, 0, NULL, FILE_END) == 0xFFFFFFFF)
		{
			return;
		}
		//WriteFile(fHandle, str, totalLength, &bytesWrittenCurr, NULL);
		BYTE dummyData[BUFFER_SIZE] = {0x00};
		DWORD bytesWrite;
		::WriteFile(hFile, dummyData, expandSize, &bytesWrite, NULL);
	}

	BOOL MoveBytesToFileEnd(HANDLE hFile, DWORD start, DWORD end , DWORD expandSize) {

		LONG writeTotal = 0;
		while ((DWORD)writeTotal < expandSize) {
			if (SetFilePointer(hFile, -(CHUNK_SIZE + (LONG)expandSize + writeTotal), NULL, FILE_END) == 0xFFFFFFFF)
			{
				return NULL;
			}
			BYTE buffer[CHUNK_SIZE];
			DWORD bytesRead = 0;
			if (!::ReadFile(hFile, &buffer, CHUNK_SIZE, &bytesRead, NULL)) {
				return NULL;
			}
			if (SetFilePointer(hFile, -(CHUNK_SIZE + writeTotal), NULL, FILE_END) == 0xFFFFFFFF)
			{
				return NULL;
			}
			DWORD bytesWrite = 0;
			if (!::WriteFile(hFile, &buffer, CHUNK_SIZE, &bytesWrite, NULL)) {
				return NULL;
			}
			writeTotal += CHUNK_SIZE;
		}
		return true;
	}
};