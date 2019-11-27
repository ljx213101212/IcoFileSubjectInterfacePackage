// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#include "framework.h"


#define PNG_SIG_CHUNK_TYPE "iTXt"
#define PNG_IEND_CHUNK_TYPE "IEND"
#define PNG_HEADER_SIZE 8
#define PNG_CHUNK_HEADER_SIZE 8
#define PNG_CRC_SIZE 4
#define PNG_TAG_SIZE 4

#define PNG_SIG_SIZE 8
#define ICO_SIZE_OF_DATA_SIZE 4
#define ICO_SIZE_OF_DATA_OFFSET 4
#define ICO_PNG_LEFT_OVER_BMP_OFFSET 12
#define ICO_HEADER_SIZE 6
#define ICO_OFFSET_OF_DATA_SIZE_IN_HEADER 8
#define ICO_DIRECTORY_CHUNK_SIZE 16
#endif //PCH_H
