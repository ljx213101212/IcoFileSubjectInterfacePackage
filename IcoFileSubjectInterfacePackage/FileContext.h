#pragma once
#include "pch.h"
class FileContext 
{
	WCHAR* extension;
	public:
		FileContext(WCHAR* ext) : extension(ext)
		{}
		WCHAR* getExtension();
		BOOL setExtension();
};