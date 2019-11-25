#pragma once
#include "pch.h"

namespace MyUtility {
	DWORD GetFilePointer(HANDLE hFile);
	void ExpandFile(HANDLE hFile, DWORD expandSize);
	BOOL MoveBytesToFileEnd(HANDLE hFile, DWORD start, DWORD end, DWORD expandSize);
};