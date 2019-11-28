#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define CRYPT_OID_INFO_HAS_EXTRA_FIELDS
// Windows Header Files
#include <windows.h>
#include <wincrypt.h>
#include <mssip.h>
#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include <tuple>
#include <vector>
#include <set>