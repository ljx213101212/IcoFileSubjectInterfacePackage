#include "pch.h"
#include "FileUtil.h"

#define BUFFER_SIZE 0x10000
#define CHUNK_SIZE 0x400

namespace MyUtility {
	DWORD  GetFilePointer(HANDLE hFile) {
		return SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
	}

	BOOL ResetFilePointer(HANDLE hFile) {
		return SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
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

	BOOL MoveBytesToFileEnd(HANDLE hFile, DWORD start, DWORD end, DWORD expandSize) {

		LONG writeTotal = 0;
		LONG moveTotal = (DWORD)(end - start);
		while ((moveTotal - writeTotal) >= CHUNK_SIZE) {
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
		if (SetFilePointer(hFile, -((moveTotal - writeTotal) + (LONG)expandSize + writeTotal), NULL, FILE_END) == 0xFFFFFFFF)
		{
			return NULL;
		}
		BYTE buffer[CHUNK_SIZE];
		DWORD bytesRead = 0;
		if (!::ReadFile(hFile, &buffer, (moveTotal - writeTotal), &bytesRead, NULL)) {
			return NULL;
		}
		if (SetFilePointer(hFile, -moveTotal, NULL, FILE_END) == 0xFFFFFFFF)
		{
			return NULL;
		}
		DWORD bytesWrite = 0;
		if (!::WriteFile(hFile, &buffer, (moveTotal - writeTotal), &bytesWrite, NULL)) {
			return NULL;
		}
		writeTotal += (moveTotal - writeTotal);
		if (writeTotal != moveTotal) { return NULL; }

		return true;
	}

	BOOL MoveBytesFromFileEndToFileLeft(HANDLE hFile, DWORD start, DWORD end, DWORD moveSize)
	{
		LONG needToMoveTotal = abs((LONG)(end - start));
		LONG movedTotal = 0;
		BYTE buffer[CHUNK_SIZE];
		DWORD bytesRead = 0;
		DWORD bytesWrite = 0;
		while ((needToMoveTotal - movedTotal) >= CHUNK_SIZE) {
			if (SetFilePointer(hFile, movedTotal + start, NULL, FILE_BEGIN) == 0xFFFFFFFF)
			{ 
				return NULL; 
			}
			if (!::ReadFile(hFile, &buffer, CHUNK_SIZE, &bytesRead, NULL)) {
				return NULL;
			}
			if (SetFilePointer(hFile, movedTotal + start - (LONG)moveSize, NULL, FILE_BEGIN) == 0xFFFFFFFF)
			{
				return NULL;
			}
			if (!::WriteFile(hFile, &buffer, CHUNK_SIZE, &bytesWrite, NULL)) {
				return NULL;
			}
			movedTotal += CHUNK_SIZE;
		}
		//Process residue
		if (needToMoveTotal - movedTotal > 0) {
			if (SetFilePointer(hFile, movedTotal + start, NULL, FILE_BEGIN) == 0xFFFFFFFF)
			{
				return NULL;
			}
			if (!::ReadFile(hFile, &buffer, (needToMoveTotal - movedTotal), &bytesRead, NULL)) {
				return NULL;
			}
			if (SetFilePointer(hFile,  movedTotal + start - (LONG)moveSize, NULL, FILE_BEGIN) == 0xFFFFFFFF)
			{
				return NULL;
			}
			if (!::WriteFile(hFile, &buffer, (needToMoveTotal - movedTotal), &bytesWrite, NULL)) {
				return NULL;
			}
		}
		movedTotal += (needToMoveTotal - movedTotal);
		if (movedTotal != needToMoveTotal) { return NULL; }
		return true;
	}

	void ShrinkFile(HANDLE hFile, DWORD reduceSize) {
		if (SetFilePointer(hFile, -(LONG)reduceSize, NULL, FILE_END) == 0xFFFFFFFF)
		{
			return;
		}
		SetEndOfFile(hFile);
	}
};