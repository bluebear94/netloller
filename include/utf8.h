#pragma once

#include <stdlib.h>

inline int isASCII(char c) {
	return c < 128;
}
inline int isContinuation(char c) {
	return c >= 128 && c < 192;
}
inline int is2ByteStarter(char c) {
	return c >= 192 && c < 224;
}
inline int is3ByteStarter(char c) {
	return c >= 224 && c < 240;
}
inline int is4ByteStarter(char c) {
	return c >= 240 && c < 248;
}
inline int utf8DecodeErrorClass(int err) {
	return err >> 12;
}

#define ERR_UNEXPECTED_CONTINUATION 0x1000
#define ERR_INVALID_UTF8_BYTE 0x2000
#define ERR_CONTINUATION_EXPECTED 0x3000
#define ERRC_UNEXPECTED_CONTINUATION 0x1
#define ERRC_INVALID_UTF8_BYTE 0x2
#define ERRC_CONTINUATION_EXPECTED 0x3

/*
	Returns the next UTF-8 character and advances the string pointer.
	This function does not consider the case when the current byte
	does not align with any codepoint.
	@param strRef a pointer to a pointer to the current character
	@param codeRef where you want the resulting codepoint to be stored
	@return
		0 if everything went well
		ERR_UNEXPECTED_CONTINUATION + 0x100 * bytes advanced + byte
			if an unexpected continuation byte was found
		ERR_INVALID_UTF8_BYTE + 0x100 * bytes advanced + byte
			if an unexpected continuation byte was found
*/
int utf8Next(char** strRef, int* codeRef);
