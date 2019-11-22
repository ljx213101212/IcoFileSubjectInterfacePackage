#pragma once

#include <initguid.h>

#define PNGSIP_CALL _stdcall

// {06b186f5-3e6c-41b7-a865-c43c21f6b540}
DEFINE_GUID(GUID_PNG_SIP,
	0x06b186f5, 0x3e6c, 0x41b7, 0xa8, 0x65, 0xc4, 0x3c, 0x21, 0xf6, 0xb5, 0x40);

#define PNGSIP_ERROR_BEGIN { \
	DWORD _pngSipError; \
	BOOL  _pngSipSuccess = FALSE

#define PNGSIP_ERROR_FAIL(ERR) \
	_pngSipError = ERR; \
	_pngSipSuccess = FALSE; \
	goto PNGSIP_RET

#define PNGSIP_ERROR_FAIL_LAST_ERROR() PNGSIP_ERROR_FAIL(GetLastError())

#define PNGSIP_ERROR_SUCCESS() \
	_pngSipError = ERROR_SUCCESS; \
	_pngSipSuccess = TRUE; \
	goto PNGSIP_RET

#define PNGSIP_ERROR_FINISH_BEGIN_CLEANUP PNGSIP_RET: \
	SetLastError(_pngSipError)

#define PNGSIP_ERROR_FINISH_BEGIN_CLEANUP_TRANSFER(TO) PNGSIP_RET: \
	TO = _pngSipError

#define PNGSIP_ERROR_FINISH_END_CLEANUP return _pngSipSuccess == TRUE; }