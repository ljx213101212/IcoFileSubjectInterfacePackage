#pragma once
#include "pch.h"
#include "pngsip.h"
#define PNG_SIG_CHUNK_TYPE "iTXt"
#define PNG_IEND_CHUNK_TYPE "IEND"


//Get Hash from the PNG
BOOL PNGSIP_CALL IcoDigestChunks(HANDLE hFile, BCRYPT_HASH_HANDLE hHashHandle, DWORD digestSize, PBYTE pBuffer, DWORD* error);
//Put Signature (Generated from Hash by certificate pfx file)
BOOL PNGSIP_CALL IcoPutDigest(HANDLE hFile, DWORD dwSignatureSize, PBYTE pSignature, DWORD* error);
//Get Signature (Generated from Hash by certificate pfx file)
BOOL PNGSIP_CALL IcoGetDigest(HANDLE hFile, DWORD* pcbSignatureSize, PBYTE pSignature, DWORD* error);

//Part 1 of Get Hash from the PNG
BOOL PNGSIP_CALL HashHeader(HANDLE hFile, BCRYPT_HASH_HANDLE hHash, DWORD* error);
//Part 2 of Get Hash from the PNG
BOOL PNGSIP_CALL HashChunk(HANDLE hFile, BCRYPT_HASH_HANDLE hHash, DWORD* error);

