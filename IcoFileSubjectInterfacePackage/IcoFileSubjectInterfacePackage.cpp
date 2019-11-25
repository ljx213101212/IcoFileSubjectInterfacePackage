#include "pch.h"
#include "IconFileSubjectInterfacePackage.h"
#include "pngsip.h"
#include <assert.h>
#include <stdio.h>
#include "IcoDigest.h"

extern "C" {
STDAPI DllRegisterServer()
{
	SIP_ADD_NEWPROVIDER provider = { 0 };
	GUID subjectGuid = GUID_PNG_SIP;
	provider.cbStruct = sizeof(SIP_ADD_NEWPROVIDER);
	provider.pgSubject = &subjectGuid;

#ifdef _WIN64
	provider.pwszDLLFileName = (WCHAR*)L"C:\\Windows\\System32\\icosip.dll";
#else
	provider.pwszDLLFileName = (WCHAR*)L"C:\\Windows\\SYSWOW64\\icosip.dll";
#endif
	provider.pwszGetFuncName = (WCHAR*)L"IcoCryptSIPGetSignedDataMsg";
	provider.pwszPutFuncName = (WCHAR*)L"IcoCryptSIPPutSignedDataMsg";
	provider.pwszCreateFuncName = (WCHAR*)L"IcoCryptSIPCreateIndirectData";
	provider.pwszVerifyFuncName = (WCHAR*)L"IcoCryptSIPVerifyIndirectData";
	provider.pwszRemoveFuncName = (WCHAR*)L"IcoCryptSIPRemoveSignedDataMsg";
	provider.pwszIsFunctionNameFmt2 = (WCHAR*)L"IcoIsFileSupportedName";

	BOOL result = CryptSIPAddProvider(&provider);
	if (result)
	{
		return S_OK;
	}
	else
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}
}

STDAPI DllUnregisterServer()
{
	GUID subjectGuid = GUID_PNG_SIP;
	BOOL result = CryptSIPRemoveProvider(&subjectGuid);
	if (result)
	{
		return S_OK;
	}
	else
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}
}



	BOOL WINAPI IcoIsFileSupportedName(WCHAR* pwszFileName, GUID* pgSubject)
	{
		const WCHAR* ext = L".png";
		size_t len = wcslen(pwszFileName);
		if (len < wcslen(ext))
		{
			return FALSE;
		}
		size_t offset = len - wcslen(ext);
		assert(offset >= 0);
		const WCHAR* substring = &pwszFileName[offset];
		int result = _wcsicmp(substring, ext);
		if (result == 0)
		{
			*pgSubject = GUID_PNG_SIP;
			return TRUE;
		}
		return FALSE;
	}

	BOOL WINAPI IcoCryptSIPGetSignedDataMsg(SIP_SUBJECTINFO* pSubjectInfo, DWORD* pdwEncodingType, DWORD dwIndex,
		DWORD* pcbSignedDataMsg, BYTE* pbSignedDataMsg)
	{
		PNGSIP_ERROR_BEGIN;
		if (dwIndex != 0 ||
			pSubjectInfo == NULL ||
			pdwEncodingType == NULL)
		{
			PNGSIP_ERROR_FAIL(ERROR_INVALID_PARAMETER);
		}
		*pdwEncodingType = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;
		DWORD error;
		if (IcoGetDigest(pSubjectInfo->hFile, pcbSignedDataMsg, pbSignedDataMsg, &error))
		{
			PNGSIP_ERROR_SUCCESS();
		}
		else
		{
			PNGSIP_ERROR_FAIL(error);
		}
		PNGSIP_ERROR_FINISH_BEGIN_CLEANUP;
		PNGSIP_ERROR_FINISH_END_CLEANUP;
	}

	BOOL WINAPI IcoCryptSIPPutSignedDataMsg(SIP_SUBJECTINFO* pSubjectInfo, DWORD dwEncodingType, DWORD* pdwIndex,
		DWORD cbSignedDataMsg, BYTE* pbSignedDataMsg)
	{
		PNGSIP_ERROR_BEGIN;
		if (*pdwIndex != 0 || pSubjectInfo == NULL
			|| pbSignedDataMsg == NULL)
		{
			PNGSIP_ERROR_FAIL(ERROR_INVALID_PARAMETER);
		}
		DWORD error;
		if (IcoPutDigest(pSubjectInfo->hFile, cbSignedDataMsg, pbSignedDataMsg, &error))
		{
			PNGSIP_ERROR_SUCCESS();
		}
		else
		{
			PNGSIP_ERROR_FAIL(error);
		}
		PNGSIP_ERROR_FINISH_BEGIN_CLEANUP;
		PNGSIP_ERROR_FINISH_END_CLEANUP;
	}

	BOOL WINAPI IcoCryptSIPCreateIndirectData(SIP_SUBJECTINFO* pSubjectInfo, DWORD* pcbIndirectData,
		SIP_INDIRECT_DATA* pIndirectData)
	{
		PNGSIP_ERROR_BEGIN;
		BOOL allocdAlgorithm = FALSE, allocdHashHandle = FALSE;
		BCRYPT_ALG_HANDLE hAlgorithm = { 0 };
		allocdAlgorithm = TRUE;
		BCRYPT_HASH_HANDLE hHashHandle = { 0 };
		allocdHashHandle = TRUE;
		DWORD dwHashSize = 0, cbHashSize = sizeof(DWORD);
		PCCRYPT_OID_INFO info = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY, pSubjectInfo->DigestAlgorithm.pszObjId, CRYPT_HASH_ALG_OID_GROUP_ID);
		size_t oidLen = strlen(pSubjectInfo->DigestAlgorithm.pszObjId) + sizeof(CHAR);
		INTERNAL_SIP_INDIRECT_DATA* pInternalIndirectData = (INTERNAL_SIP_INDIRECT_DATA*)pIndirectData;
		if (pSubjectInfo == NULL || pcbIndirectData == NULL)
		{
			PNGSIP_ERROR_FAIL(ERROR_INVALID_PARAMETER);
		}
		// Win32 is asking how much it needs to allocate for pIndirectData
		if (pIndirectData == NULL)
		{
			*pcbIndirectData = sizeof(INTERNAL_SIP_INDIRECT_DATA);
			PNGSIP_ERROR_SUCCESS();
		}
		if (*pcbIndirectData < sizeof(INTERNAL_SIP_INDIRECT_DATA))
		{
			PNGSIP_ERROR_FAIL(ERROR_INSUFFICIENT_BUFFER);
			
		}
		if (info == NULL)
		{
			PNGSIP_ERROR_FAIL(ERROR_NOT_SUPPORTED);
		}

		if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlgorithm, info->pwszCNGAlgid, NULL, 0)))
		{
			PNGSIP_ERROR_FAIL(ERROR_NOT_SUPPORTED);
		}

		if (!BCRYPT_SUCCESS(BCryptCreateHash(hAlgorithm, &hHashHandle, NULL, 0, NULL, 0, 0)))
		{
			PNGSIP_ERROR_FAIL(ERROR_NOT_SUPPORTED);
		}

		if (!BCRYPT_SUCCESS(BCryptGetProperty(hHashHandle, BCRYPT_HASH_LENGTH, (PUCHAR)&dwHashSize, sizeof(DWORD), &cbHashSize, 0)))
		{
			PNGSIP_ERROR_FAIL(ERROR_NOT_SUPPORTED);
		}

		if (dwHashSize > MAX_HASH_SIZE || oidLen > MAX_OID_SIZE)
		{
			PNGSIP_ERROR_FAIL(ERROR_INSUFFICIENT_BUFFER);
		}
		//We checked the size earlier above.

		memset(pInternalIndirectData, 0, sizeof(INTERNAL_SIP_INDIRECT_DATA));

		DWORD error;

		//
		IcoFileInfo icoInfo;
		ICO_FILE_INFO icoFileInfo;
		icoInfo.GetIcoFileInfo(pSubjectInfo->hFile, pSubjectInfo->pwsFileName ,&icoFileInfo);
		if (!IcoDigestChunks(pSubjectInfo->hFile, hHashHandle, dwHashSize, 
			icoFileInfo.beforePNGStartPosition,
			icoFileInfo.PNGStartPosition,
			icoFileInfo.afterPNGStartPosition,
			icoFileInfo.fileEndPosition,
			&pInternalIndirectData->digest[0], &error))
		{
			PNGSIP_ERROR_FAIL(error);
		}

		if (0 != strcpy_s(&pInternalIndirectData->oid[0], oidLen, pSubjectInfo->DigestAlgorithm.pszObjId))
		{
			PNGSIP_ERROR_FAIL(ERROR_INVALID_PARAMETER);
		}
		pInternalIndirectData->indirectData.Digest.cbData = dwHashSize;
		pInternalIndirectData->indirectData.Digest.pbData = &pInternalIndirectData->digest[0];
		pInternalIndirectData->indirectData.DigestAlgorithm.pszObjId = pInternalIndirectData->oid;
		pInternalIndirectData->indirectData.DigestAlgorithm.Parameters.cbData = 0;
		pInternalIndirectData->indirectData.DigestAlgorithm.Parameters.pbData = NULL;
		pInternalIndirectData->indirectData.Data.pszObjId = NULL;
		pInternalIndirectData->indirectData.Data.Value.cbData = 0;
		pInternalIndirectData->indirectData.Data.Value.pbData = NULL;
		PNGSIP_ERROR_SUCCESS();

		PNGSIP_ERROR_FINISH_BEGIN_CLEANUP;
		if (allocdAlgorithm) BCryptCloseAlgorithmProvider(hAlgorithm, 0);
		if (allocdHashHandle) BCryptDestroyHash(hHashHandle);
		PNGSIP_ERROR_FINISH_END_CLEANUP;
		
	}

	BOOL WINAPI IcoCryptSIPVerifyIndirectData(SIP_SUBJECTINFO* pSubjectInfo, SIP_INDIRECT_DATA* pIndirectData)
	{
		PNGSIP_ERROR_BEGIN;
		BOOL allocdAlgorithm = FALSE, allocdHashHandle = FALSE;
		BCRYPT_ALG_HANDLE hAlgorithm = { 0 };
		allocdAlgorithm = TRUE;
		PCCRYPT_OID_INFO info = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY, pIndirectData->DigestAlgorithm.pszObjId, CRYPT_HASH_ALG_OID_GROUP_ID);
		BCRYPT_HASH_HANDLE hHashHandle = { 0 };
		allocdHashHandle = TRUE;
		DWORD dwHashSize = 0, cbHashSize = sizeof(DWORD);
		BYTE digestBuffer[MAX_HASH_SIZE];
		DWORD error;
		if (pSubjectInfo == NULL || pIndirectData == NULL)
		{
			PNGSIP_ERROR_FAIL(ERROR_INVALID_PARAMETER);
		}
	
		if (info == NULL)
		{
			PNGSIP_ERROR_FAIL(ERROR_INVALID_PARAMETER);
		}

		if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlgorithm, info->pwszCNGAlgid, NULL, 0)))
		{
			PNGSIP_ERROR_FAIL(ERROR_NOT_SUPPORTED);
		}
		if (!BCRYPT_SUCCESS(BCryptCreateHash(hAlgorithm, &hHashHandle, NULL, 0, NULL, 0, 0)))
		{
			PNGSIP_ERROR_FAIL(ERROR_NOT_SUPPORTED);
		}
	
		if (!BCRYPT_SUCCESS(BCryptGetProperty(hHashHandle, BCRYPT_HASH_LENGTH, (PUCHAR)&dwHashSize, sizeof(DWORD), &cbHashSize, 0)))
		{
			PNGSIP_ERROR_FAIL(ERROR_NOT_SUPPORTED);
		}
		if (dwHashSize > MAX_HASH_SIZE || dwHashSize != pIndirectData->Digest.cbData)
		{
			PNGSIP_ERROR_FAIL(ERROR_INVALID_PARAMETER);
		}
		//
		IcoFileInfo icoInfo;
		ICO_FILE_INFO icoFileInfo;
		icoInfo.GetIcoFileInfo(pSubjectInfo->hFile, pSubjectInfo->pwsFileName, &icoFileInfo);

		if (!IcoDigestChunks(pSubjectInfo->hFile, hHashHandle, dwHashSize, 
			icoFileInfo.beforePNGStartPosition,
			icoFileInfo.PNGStartPosition,
			icoFileInfo.afterPNGStartPosition,
			icoFileInfo.fileEndPosition,
			&digestBuffer[0], &error))
		{
			PNGSIP_ERROR_FAIL(error);
		}
		if (0 == memcmp(&digestBuffer, pIndirectData->Digest.pbData, dwHashSize))
		{
			PNGSIP_ERROR_SUCCESS();
		}
		else
		{
			PNGSIP_ERROR_FAIL(TRUST_E_SUBJECT_NOT_TRUSTED);
		}

		PNGSIP_ERROR_FINISH_BEGIN_CLEANUP;
		if (allocdAlgorithm) BCryptCloseAlgorithmProvider(hAlgorithm, 0);
		if (allocdHashHandle) BCryptDestroyHash(hHashHandle);
		PNGSIP_ERROR_FINISH_END_CLEANUP;
	}

	BOOL WINAPI IcoCryptSIPRemoveSignedDataMsg(SIP_SUBJECTINFO* pSubjectInfo, DWORD dwIndex)
	{
		return FALSE;
	}
}