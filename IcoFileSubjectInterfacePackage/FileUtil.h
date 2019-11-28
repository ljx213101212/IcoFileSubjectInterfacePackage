#pragma once
#include "pch.h"

namespace MyUtility {
	BOOL ResetFilePointer(HANDLE hFile);
	DWORD GetFilePointer(HANDLE hFile);
	void ExpandFile(HANDLE hFile, DWORD expandSize);
	BOOL MoveBytesToFileEnd(HANDLE hFile, DWORD start, DWORD end, DWORD expandSize);
	void ShrinkFile(HANDLE hFile, DWORD reduceSize);
	BOOL MoveBytesFromFileEndToFileLeft(HANDLE hFile, DWORD start, DWORD end, DWORD moveSize);
};