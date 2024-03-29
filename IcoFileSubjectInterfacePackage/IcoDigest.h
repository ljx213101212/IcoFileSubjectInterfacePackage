#pragma once
#include "pch.h"
#include "pngsip.h"

enum SignToolProcess
{
	sign = 1,
	verify = 2
};

//Get Hash from the PNG
BOOL PNGSIP_CALL IcoDigestChunks(HANDLE hFile, BCRYPT_HASH_HANDLE hHashHandle, DWORD digestSize, LONG beforePNGStartPosition,
	LONG PNGStartPosition, LONG afterPNGStartPosition, LONG fileEndPosition, SignToolProcess process, PBYTE pBuffer, DWORD* error);
//Put Signature (Generated from Hash by certificate pfx file)
BOOL PNGSIP_CALL IcoPutDigest(HANDLE hFile, LPCWSTR pwsFileName, DWORD dwSignatureSize, PBYTE pSignature, DWORD* error);
//Get Signature (Generated from Hash by certificate pfx file)
BOOL PNGSIP_CALL IcoGetDigest(HANDLE hFile, LPCWSTR pwsFileName, DWORD* pcbSignatureSize, PBYTE pSignature, DWORD* error);

//Part 1 of Get Hash from the PNG
BOOL PNGSIP_CALL HashHeader(HANDLE hFile, BCRYPT_HASH_HANDLE hHash, DWORD* error);
//Part 2 of Get Hash from the PNG
//BOOL PNGSIP_CALL HashChunk(HANDLE hFile, BCRYPT_HASH_HANDLE hHash, DWORD* error);


BOOL PNGSIP_CALL HashPNGChunk(HANDLE hFile, BCRYPT_HASH_HANDLE hHash, LONG chunkStartOffset, DWORD chunkSize, DWORD* error);
BOOL PNGSIP_CALL HashPNGSubChunk(HANDLE hFile, BCRYPT_HASH_HANDLE hHash, DWORD& hashedSize, DWORD* error);
BOOL PNGSIP_CALL HashCustomChunk(HANDLE hFile, BCRYPT_HASH_HANDLE hHash, LONG chunkStartOffset, DWORD chunkSize, DWORD* error);
BOOL PNGSIP_CALL HashIcoHeaderChunk(HANDLE hFile, BCRYPT_HASH_HANDLE hHash, LONG chunkStartOffset, LONG * icoHeaderEndOffset, BOOL isOriginToUpdate, DWORD* error);



