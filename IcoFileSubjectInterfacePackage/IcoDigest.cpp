#include "pch.h"


#include <intrin.h>
#include "IcoDigest.h"
#include "crc.h"
#include "pngsip.h"
#include "IcoFileInfo.h"
#include "FileUtil.h"

#define BUFFER_SIZE 0x10000
#define CHUNK_SIZE 0x400

//namespace myUtility 
//{
//	DWORD  GetFilePointer(HANDLE hFile) {
//		return SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
//	}
//
//	void ExpandFile(HANDLE hFile, DWORD expandSize) {
//		if (SetFilePointer(hFile, 0, NULL, FILE_END) == 0xFFFFFFFF)
//		{
//			return;
//		}
//		//WriteFile(fHandle, str, totalLength, &bytesWrittenCurr, NULL);
//		BYTE dummyData[BUFFER_SIZE] = { 0x00 };
//		DWORD bytesWrite;
//		::WriteFile(hFile, dummyData, expandSize, &bytesWrite, NULL);
//	}
//
//	BOOL MoveBytesToFileEnd(HANDLE hFile, DWORD start, DWORD end, DWORD expandSize) {
//
//		LONG writeTotal = 0;
//		while ((DWORD)writeTotal < expandSize) {
//			if (SetFilePointer(hFile, -(CHUNK_SIZE + (LONG)expandSize + writeTotal), NULL, FILE_END) == 0xFFFFFFFF)
//			{
//				return NULL;
//			}
//			BYTE buffer[CHUNK_SIZE];
//			DWORD bytesRead = 0;
//			if (!::ReadFile(hFile, &buffer, CHUNK_SIZE, &bytesRead, NULL)) {
//				return NULL;
//			}
//			if (SetFilePointer(hFile, -(CHUNK_SIZE + writeTotal), NULL, FILE_END) == 0xFFFFFFFF)
//			{
//				return NULL;
//			}
//			DWORD bytesWrite = 0;
//			if (!::WriteFile(hFile, &buffer, CHUNK_SIZE, &bytesWrite, NULL)) {
//				return NULL;
//			}
//			writeTotal += CHUNK_SIZE;
//		}
//		return true;
//	}
//};

DWORD GetBeforePNGSize(LONG PNGStartPosition, LONG beforePNGStartPosition) {
	return (DWORD)(PNGStartPosition - beforePNGStartPosition);
}

DWORD GetPNGSize(LONG PNGStartPosition, LONG afterPNGStartPosition) {
	return (DWORD)(afterPNGStartPosition - PNGStartPosition);
}

DWORD GetAfterPNGSize(LONG afterPNGStartPosition, LONG fileEndPosition) {
	return (DWORD)(fileEndPosition - afterPNGStartPosition);
}

BOOL PNGSIP_CALL IcoDigestChunks(HANDLE hFile, BCRYPT_HASH_HANDLE hHashHandle,
	DWORD digestSize, LONG beforePNGStartPosition,  
	LONG PNGStartPosition,  LONG afterPNGStartPosition, LONG fileEndPosition,
	SignToolProcess process,
	PBYTE pBuffer, DWORD* error)
{
	PNGSIP_ERROR_BEGIN;
	if (hFile == INVALID_HANDLE_VALUE)
	{
		PNGSIP_ERROR_FAIL(ERROR_INVALID_PARAMETER);
	}
	if (SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		PNGSIP_ERROR_FAIL(ERROR_BAD_FORMAT);
	}

	DWORD result;
	//Hash Before PNG Chunk.
	if (process == SignToolProcess::verify) {
		if (!HashIcoHeaderChunk(hFile, hHashHandle, false, &result)) {
			PNGSIP_ERROR_FAIL(result);
		}
	}
	else {
		if (!HashCustomChunk(hFile, hHashHandle, GetBeforePNGSize(PNGStartPosition, beforePNGStartPosition), &result)) {
			PNGSIP_ERROR_FAIL(result);
		}
	}
	
	//Hash PNG Chunk.
	if (!HashPNGChunk(hFile, hHashHandle, PNGStartPosition, GetPNGSize(PNGStartPosition, afterPNGStartPosition), &result)) {
		PNGSIP_ERROR_FAIL(result);
	}
	////Hash After PNG Chunk.
	if (!HashCustomChunk(hFile, hHashHandle, GetAfterPNGSize(afterPNGStartPosition, fileEndPosition), &result)) {
		PNGSIP_ERROR_FAIL(result);
	}

	if (!BCRYPT_SUCCESS(BCryptFinishHash(hHashHandle, pBuffer, digestSize, 0)))
	{
		PNGSIP_ERROR_FAIL(ERROR_INVALID_OPERATION);
	}
	PNGSIP_ERROR_SUCCESS();

	PNGSIP_ERROR_FINISH_BEGIN_CLEANUP_TRANSFER(*error);
	PNGSIP_ERROR_FINISH_END_CLEANUP;
}

BOOL PNGSIP_CALL HashIcoHeaderChunk(HANDLE hFile, BCRYPT_HASH_HANDLE hHash, BOOL isOriginToUpdate, DWORD* error) {

	PNGSIP_ERROR_BEGIN;
	BYTE headerBuffer[BUFFER_SIZE];
	BYTE buffer[BUFFER_SIZE];
	DWORD bufferPtr = 0;
	DWORD bytesRead = 0;
	ICO_FILE_INFO info;
	LONG switchFatcor = isOriginToUpdate ? 1 : -1;
	IcoFileInfo icoFileInfo;
	icoFileInfo.GetIcoFileInfo(hFile, TEXT(""), &info);
	DWORD pngHeaderSizeOffset = ICO_HEADER_SIZE + (info.nthImageIsPng - 1) * ICO_DIRECTORY_CHUNK_SIZE + ICO_OFFSET_OF_DATA_SIZE_IN_HEADER;
	DWORD icoHeaderOffset = ICO_HEADER_SIZE + info.numOfIco * ICO_DIRECTORY_CHUNK_SIZE;
	DWORD signatureSize = info.sigChunkSize;
	/*DWORD signatureSize = icoFileInfo.GetSignatureSize();*/
	MyUtility::ResetFilePointer(hFile);
	if (!::ReadFile(hFile, headerBuffer, icoHeaderOffset, &bytesRead, NULL)) {
		return false;
	}
	bufferPtr += pngHeaderSizeOffset;
	memcpy_s(buffer, ICO_SIZE_OF_DATA_SIZE, headerBuffer + bufferPtr, ICO_SIZE_OF_DATA_SIZE);
	LONG size = buffer[0] | buffer[1] << 8 | buffer[2] << 16 | buffer[3] << 24;
	size += ((LONG)signatureSize * switchFatcor);
	////little endian
	buffer[0] = size & 0xFF;
	buffer[1] = (size >> 8) & 0xFF;
	buffer[2] = (size >> 16) & 0xFF;
	buffer[3] = (size >> 24) & 0xFF;
	memcpy_s(headerBuffer + bufferPtr, ICO_SIZE_OF_DATA_SIZE, buffer, ICO_SIZE_OF_DATA_SIZE);

	bufferPtr += (ICO_SIZE_OF_DATA_SIZE + ICO_SIZE_OF_DATA_OFFSET);
	INT32 restNumOfBMPFile = info.numOfIco - info.nthImageIsPng;
	for (int i = 0; i < restNumOfBMPFile; i++) {

		bufferPtr += ICO_PNG_LEFT_OVER_BMP_OFFSET;
		BYTE offsetBuffer[ICO_SIZE_OF_DATA_OFFSET];
		memcpy_s(offsetBuffer, ICO_SIZE_OF_DATA_OFFSET, headerBuffer + bufferPtr, ICO_SIZE_OF_DATA_OFFSET);

		LONG offset = offsetBuffer[0] | offsetBuffer[1] << 8 | offsetBuffer[2] << 16 | offsetBuffer[3] << 24;
		offset += ((LONG)signatureSize * switchFatcor);
		//little endian
		buffer[0] = offset & 0xFF;
		buffer[1] = (offset >> 8) & 0xFF;
		buffer[2] = (offset >> 16) & 0xFF;
		buffer[3] = (offset >> 24) & 0xFF;

		memcpy_s(headerBuffer + bufferPtr, ICO_SIZE_OF_DATA_OFFSET, buffer, ICO_SIZE_OF_DATA_OFFSET);
		bufferPtr += ICO_SIZE_OF_DATA_OFFSET;
	}

	if (!BCRYPT_SUCCESS(BCryptHashData(hHash, headerBuffer, bytesRead, 0)))
	{
		PNGSIP_ERROR_FAIL(ERROR_INVALID_OPERATION);
	}
	PNGSIP_ERROR_SUCCESS();
	PNGSIP_ERROR_FINISH_BEGIN_CLEANUP_TRANSFER(*error);
	PNGSIP_ERROR_FINISH_END_CLEANUP;
}

BOOL PNGSIP_CALL HashCustomChunk(HANDLE hFile, BCRYPT_HASH_HANDLE hHash, DWORD chunkSize, DWORD* error) {
	PNGSIP_ERROR_BEGIN;
	DWORD bytesRead = 0;
	BYTE buffer[BUFFER_SIZE];
	DWORD remainder = (chunkSize % BUFFER_SIZE);
	for (DWORD i = 0; i < chunkSize / BUFFER_SIZE; i++)
	{
		if (!ReadFile(hFile, &buffer, BUFFER_SIZE, &bytesRead, NULL))
		{
			PNGSIP_ERROR_FAIL_LAST_ERROR();
		}
		if (!BCRYPT_SUCCESS(BCryptHashData(hHash, &buffer[0], bytesRead, 0)))
		{
			PNGSIP_ERROR_FAIL(ERROR_INVALID_OPERATION);
		}
	}
	if (!ReadFile(hFile, &buffer, remainder, &bytesRead, NULL))
	{
		PNGSIP_ERROR_FAIL_LAST_ERROR();
	}
	if (!BCRYPT_SUCCESS(BCryptHashData(hHash, &buffer[0], bytesRead, 0)))
	{
		PNGSIP_ERROR_FAIL(ERROR_INVALID_OPERATION);
	}
	PNGSIP_ERROR_SUCCESS();
	PNGSIP_ERROR_FINISH_BEGIN_CLEANUP_TRANSFER(*error);
	PNGSIP_ERROR_FINISH_END_CLEANUP;
}

BOOL PNGSIP_CALL HashHeader(HANDLE hFile, BCRYPT_HASH_HANDLE hHash, DWORD& hashedSize, DWORD* error)
{
	PNGSIP_ERROR_BEGIN;
	DWORD bytesRead = 0;
	BYTE buffer[BUFFER_SIZE];
	if (!ReadFile(hFile, &buffer, PNG_HEADER_SIZE, &bytesRead, NULL))
	{
		PNGSIP_ERROR_FAIL_LAST_ERROR();
	}
	if (bytesRead != PNG_HEADER_SIZE)
	{
		PNGSIP_ERROR_FAIL(STATUS_INVALID_PARAMETER);
	}

	if (!BCRYPT_SUCCESS(BCryptHashData(hHash, &buffer[0], PNG_HEADER_SIZE, 0)))
	{
		PNGSIP_ERROR_FAIL(ERROR_INVALID_OPERATION);
	}
	hashedSize = PNG_HEADER_SIZE;
	PNGSIP_ERROR_SUCCESS();

	PNGSIP_ERROR_FINISH_BEGIN_CLEANUP_TRANSFER(*error);
	PNGSIP_ERROR_FINISH_END_CLEANUP;
}


BOOL PNGSIP_CALL HashPNGChunk(HANDLE hFile, BCRYPT_HASH_HANDLE hHash, LONG startPosition, DWORD chunkSize, DWORD* error) {
	PNGSIP_ERROR_BEGIN;
	DWORD bytesRead = 0;
	DWORD bytesReadTotal = 0;

	DWORD result;
	if (!HashHeader(hFile, hHash, bytesRead, &result)) {
		PNGSIP_ERROR_FAIL(result);
	}
	bytesReadTotal += bytesRead;
	bytesRead = 0;

	while (bytesReadTotal < chunkSize) {
		if (!HashChunk(hFile, hHash, bytesRead, &result)) {
			PNGSIP_ERROR_FAIL(result);
		}
		bytesReadTotal += bytesRead;
		bytesRead = 0;
	}

	PNGSIP_ERROR_SUCCESS();
	PNGSIP_ERROR_FINISH_BEGIN_CLEANUP_TRANSFER(*error);
	PNGSIP_ERROR_FINISH_END_CLEANUP;
}

BOOL PNGSIP_CALL HashChunk(HANDLE hFile, BCRYPT_HASH_HANDLE hHash, DWORD &hashedSize, DWORD* error)
{
	PNGSIP_ERROR_BEGIN;
	DWORD bytesRead = 0;
	BYTE buffer[BUFFER_SIZE];

	if (!ReadFile(hFile, &buffer, PNG_CHUNK_HEADER_SIZE, &bytesRead, NULL))
	{
		PNGSIP_ERROR_FAIL_LAST_ERROR();
	}
	if (bytesRead == 0)
	{
		// We "fail" here even though everything was successful.
		PNGSIP_ERROR_FAIL(ERROR_SUCCESS);
	}
	if (bytesRead != PNG_CHUNK_HEADER_SIZE)
	{
		PNGSIP_ERROR_FAIL(ERROR_BAD_FORMAT);
	}
	{
		const unsigned int size = buffer[3] | buffer[2] << 8 | buffer[1] << 16 | buffer[0] << 24;
	
		const unsigned char* tag = ((const unsigned char*)&buffer[4]);

		if (0 == memcmp(tag, PNG_SIG_CHUNK_TYPE, PNG_TAG_SIZE))
		{
			if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, size + PNG_CRC_SIZE, NULL, FILE_CURRENT))
			{
				PNGSIP_ERROR_FAIL(ERROR_INVALID_OPERATION);
			}
			PNGSIP_ERROR_SUCCESS();
		}
		if (!BCRYPT_SUCCESS(BCryptHashData(hHash, &buffer[0], PNG_CHUNK_HEADER_SIZE, 0)))
		{
			PNGSIP_ERROR_FAIL(ERROR_INVALID_OPERATION);
		}
		hashedSize += PNG_CHUNK_HEADER_SIZE;
		for (DWORD i = 0; i < size / BUFFER_SIZE; i++)
		{
			if (!ReadFile(hFile, &buffer, BUFFER_SIZE, &bytesRead, NULL))
			{
				PNGSIP_ERROR_FAIL_LAST_ERROR();
			}
			if (!BCRYPT_SUCCESS(BCryptHashData(hHash, &buffer[0], bytesRead, 0)))
			{
				PNGSIP_ERROR_FAIL(ERROR_INVALID_OPERATION);
			}
			hashedSize += bytesRead;
		}

		DWORD remainder = (size % BUFFER_SIZE) + PNG_CRC_SIZE;
		if (!ReadFile(hFile, &buffer, remainder, &bytesRead, NULL))
		{
			PNGSIP_ERROR_FAIL_LAST_ERROR();
		}
		if (!BCRYPT_SUCCESS(BCryptHashData(hHash, &buffer[0], bytesRead, 0)))
		{
			PNGSIP_ERROR_FAIL(ERROR_INVALID_OPERATION);
		}
		hashedSize += bytesRead;
		//HashCustomChunk(hFile, hHash, size, error);
	}
	PNGSIP_ERROR_SUCCESS();

	PNGSIP_ERROR_FINISH_BEGIN_CLEANUP_TRANSFER(*error);
	PNGSIP_ERROR_FINISH_END_CLEANUP;
}



BOOL PNGSIP_CALL IcoPutDigest(HANDLE hFile, LPCWSTR pwsFileName, DWORD dwSignatureSize, PBYTE pSignature, DWORD* error)
{
	PNGSIP_ERROR_BEGIN;
	IcoFileInfo info;
	ICO_FILE_INFO icoFileInfo;
	LONG pngStartOffset, pngEndOffset, fileEndOffset;
	DWORD pngSize = 0;
	if (!info.GetIcoFileInfo(hFile, pwsFileName, &icoFileInfo)) {
		PNGSIP_ERROR_FAIL_LAST_ERROR();
	}
	pngStartOffset = icoFileInfo.PNGStartPosition;
	pngEndOffset = icoFileInfo.afterPNGStartPosition;
	fileEndOffset = icoFileInfo.fileEndPosition;
	pngSize = (DWORD)(pngEndOffset - pngStartOffset);

	MyUtility::ExpandFile(hFile, dwSignatureSize + PNG_CHUNK_HEADER_SIZE + PNG_CRC_SIZE);
	MyUtility::MoveBytesToFileEnd(hFile, pngEndOffset, fileEndOffset,dwSignatureSize + PNG_CHUNK_HEADER_SIZE + PNG_CRC_SIZE);

	if (SetFilePointer(hFile, pngEndOffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		PNGSIP_ERROR_FAIL(ERROR_BAD_FORMAT);
	}
	if (SetFilePointer(hFile, -12, NULL, FILE_CURRENT) == INVALID_SET_FILE_POINTER)
	{
		PNGSIP_ERROR_FAIL(ERROR_BAD_FORMAT);
	}
	{
		DWORD dwBytesWritten;
		DWORD dwSignatureSizeBigEndian = _byteswap_ulong(dwSignatureSize);

		if (!WriteFile(hFile, &dwSignatureSizeBigEndian, sizeof(DWORD), &dwBytesWritten, NULL))
		{
			PNGSIP_ERROR_FAIL_LAST_ERROR();
		}
		if (!WriteFile(hFile, PNG_SIG_CHUNK_TYPE, 4, &dwBytesWritten, NULL))
		{
			PNGSIP_ERROR_FAIL_LAST_ERROR();
		}
		if (!WriteFile(hFile, pSignature, dwSignatureSize, &dwBytesWritten, NULL))
		{
			PNGSIP_ERROR_FAIL_LAST_ERROR();
		}
		unsigned long checksum = crc_init();
		checksum = update_crc(checksum, (unsigned char*)PNG_SIG_CHUNK_TYPE, PNG_TAG_SIZE);
		checksum = update_crc(checksum, pSignature, dwSignatureSize);
		checksum = crc_finish(checksum);
		checksum = _byteswap_ulong(checksum);
		if (!WriteFile(hFile, &checksum, sizeof(DWORD), &dwBytesWritten, NULL))
		{
			PNGSIP_ERROR_FAIL_LAST_ERROR();
		}

		const BYTE iendChunk[12] = { 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82 };
		if (!WriteFile(hFile, &iendChunk, 12, &dwBytesWritten, NULL))
		{
			PNGSIP_ERROR_FAIL_LAST_ERROR();
		}
	}
	PNGSIP_ERROR_SUCCESS();

	PNGSIP_ERROR_FINISH_BEGIN_CLEANUP_TRANSFER(*error);
	PNGSIP_ERROR_FINISH_END_CLEANUP;
}

BOOL PNGSIP_CALL IcoGetDigest(HANDLE hFile, LPCWSTR pwsFileName, DWORD* pcbSignatureSize, PBYTE pSignature, DWORD* error)
{
	PNGSIP_ERROR_BEGIN;

	IcoFileInfo info;
	ICO_FILE_INFO icoFileInfo;
	LONG pngStartOffset, pngEndOffset, fileEndOffset;
	DWORD pngSize = 0;
	if (!info.GetIcoFileInfo(hFile, pwsFileName, &icoFileInfo)) {
		PNGSIP_ERROR_FAIL_LAST_ERROR();
	}
	pngStartOffset = icoFileInfo.PNGStartPosition;
	pngEndOffset = icoFileInfo.afterPNGStartPosition;
	fileEndOffset = icoFileInfo.fileEndPosition;
	pngSize = pngEndOffset - pngStartOffset;
	//expand the file
	//utility::ExpandFile(hFile, *pcbSignatureSize);
	//utility::MoveBytesToFileEnd(hFile, pngEndOffset, fileEndOffset, *pcbSignatureSize);

	if (SetFilePointer(hFile, pngStartOffset + PNG_HEADER_SIZE, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		PNGSIP_ERROR_FAIL_LAST_ERROR();
	}
	{
		//Locate the PNG header position.
		DWORD bytesRead = 0, totalRead = 0;
		BYTE buffer[BUFFER_SIZE];
		for (;totalRead < pngSize;)
		{
			if (!ReadFile(hFile, &buffer[0], PNG_CHUNK_HEADER_SIZE, &bytesRead, NULL))
			{
				PNGSIP_ERROR_FAIL_LAST_ERROR();
			}
			if (bytesRead < PNG_CHUNK_HEADER_SIZE)
			{
				PNGSIP_ERROR_FAIL(ERROR_BAD_FORMAT);
			}
			const unsigned int size = buffer[3] | buffer[2] << 8 | buffer[1] << 16 | buffer[0] << 24;
			const unsigned char* tag = ((const unsigned char*)&buffer[4]);
			if (memcmp(tag, PNG_SIG_CHUNK_TYPE, 4) != 0)
			{
				if (SetFilePointer(hFile, size + 4, NULL, FILE_CURRENT) == INVALID_SET_FILE_POINTER)
				{
					PNGSIP_ERROR_FAIL(ERROR_BAD_FORMAT);
				}
				continue;
			}
			// Win32 is asking how big of a buffer it needs. Set the size and exit.
			if (pSignature == NULL)
			{
				*pcbSignatureSize = size;
				PNGSIP_ERROR_SUCCESS();
			}
			// It supplied a buffer, but it wasn't big enough.
			else if (*pcbSignatureSize < size)
			{
				PNGSIP_ERROR_FAIL(ERROR_INSUFFICIENT_BUFFER);
			}
			for (DWORD i = 0; i < size / BUFFER_SIZE; i++)
			{
				if (!ReadFile(hFile, &buffer, BUFFER_SIZE, &bytesRead, NULL))
				{
					PNGSIP_ERROR_FAIL_LAST_ERROR();
				}
				memcpy(pSignature + totalRead, &buffer[0], bytesRead);
				totalRead += bytesRead;
			}
			DWORD remainder = size % BUFFER_SIZE;
			if (remainder > 0)
			{
				if (!ReadFile(hFile, &buffer, remainder, &bytesRead, NULL))
				{
					PNGSIP_ERROR_FAIL_LAST_ERROR();
				}
				memcpy(pSignature + totalRead, &buffer[0], bytesRead);
				totalRead += bytesRead;
			}
			*pcbSignatureSize = totalRead;
			PNGSIP_ERROR_SUCCESS();
		}
	}
	//We should not get out of the loop without going to success
	PNGSIP_ERROR_FAIL(TRUST_E_NOSIGNATURE);

	PNGSIP_ERROR_FINISH_BEGIN_CLEANUP_TRANSFER(*error);
	PNGSIP_ERROR_FINISH_END_CLEANUP;
}
