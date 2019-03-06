//Header for utility functions.

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <CL/cl.hpp>

#define RAND -1

using std::ifstream;
using std::string;
using std::vector;

//Converts 4 bytes into a single word.
inline uint32_t bytesTo32(uint8_t* bytes) {
	
	uint32_t val = 0;
	uint32_t mul = 256 * 256 * 256;
	while (mul != 0) {
		
		val += *bytes * mul;
		bytes++;
		mul /= 256;
		
	}
	
	return val;
	
}

//Splits a 32-bit word into 4 bytes.
inline void _32ToBytes(uint32_t val, uint8_t* bytes) {
	
	for (int i = 0; i < 4; i++)	{
		
		*(bytes	+ 3 - i) = val & (uint8_t) 0xFF;
		val >>= 8;
		
	}	
	
}

//Splits a 32-bit word into 4 bytes, little endian.
inline void L32ToBytes(uint32_t val, uint8_t* bytes) {

	for (int i = 0; i < 4; i++) {

		*(bytes + i) = val & (uint8_t)0xFF;
		val >>= 8;

	}

}

//Checks whether the ZLIB buffer is valid.
inline void zlibParse(vector<uint8_t>& zlib) {
	
	uint8_t windowSizeLog = zlib[0];
	uint8_t fdict = zlib[1];
	
	if ((windowSizeLog * 256 + fdict) % 31 != 0) throw string("Invalid PNG file: Invalid ZLIB header.");	
	if ((windowSizeLog & 0x0F) != 0x08) throw string("Invalid PNG file: Invalid ZLIB compression method.");
	
	windowSizeLog >>= 4;
	if (windowSizeLog > 7) throw string("Invalid PNG file: LZ77 window size too big.");
	
	fdict >>= 5;	
	if (fdict % 2 == 1) throw string("Invalid PNG file: Preset DEFLATE dictionaries shall not be present.");	
	
	
}

//Reads desired number of bits from a byte buffer.
inline uint8_t readBits(vector<uint8_t>& buf, uint8_t num, unsigned int& pos) {
	
	if (num > 8) throw string("Error: Trying to read too many bits.");
	
	uint8_t val = 0;
	for (uint8_t i = 0; i < num; i++) {
		
		uint8_t mask = 0x01 << (pos % 8);
		val ^= ((buf[pos >> 3] & mask) >> (pos % 8)) << i;
		pos++;
		
	}
	
	return val;
	
}

//PaethPredictor for PNG filtering
inline int paeth(int a, int b, int c) {

	int p = a + b - c;
	int pa = p > a ? p - a : a - p;
	int pb = p > b ? p - b : b - p;
	int pc = p > c ? p - c : c - p;
	int Pr;
	if (pa <= pb && pa <= pc) Pr = a;
	else if (pb <= pc) Pr = b;
	else Pr = c;
	return Pr;

}

//Calculates the Adler-32 checksum of a ZLIB buffer.
inline unsigned long adler32(vector<uint8_t>& zlib, int len) {

	unsigned long adler = 1L;

	unsigned long s1 = adler & 0xffff;
	unsigned long s2 = (adler >> 16) & 0xffff;
	
	for (int n = 2; n < len; n++) {

		s1 = (s1 + zlib[n]) % 65521;
		s2 = (s2 + s1) % 65521;

	}

	return (s2 << 16) + s1;

}

//Finds number of spaces in string.
inline unsigned int find_spaces(string s) {

	unsigned int val = 0;
	for (char c : s) if (c == ' ') val++;
	return val;

}

inline string pngToRaw(string s) {

	s[s.size() - 3] = 'r';
	s[s.size() - 2] = 'a';
	s[s.size() - 1] = 'w';
	return s;

}

typedef struct {

	unsigned int pos;
	unsigned int width;
	unsigned int height;
	unsigned char format;
	unsigned int size() const { return width * height * format; }

} texDataInfo;

inline texDataInfo findTextureInBin(string texName, string binPath) {

	if (binPath.length() != 0) {

		ifstream bin(binPath, ifstream::binary);
		bin.seekg(2);
		unsigned int numTextures = bin.get();
		numTextures += bin.get() * 256;
		while (numTextures > 0) {

			bin.seekg(4 * numTextures--);
			unsigned int pos = bin.get();
			pos += bin.get() * 256;
			pos += bin.get() * 256 * 256;
			pos += bin.get() * 256 * 256 * 256;

			bin.seekg(pos + 0x28);
			char name[0x40];
			bin.read(name, 0x40);

			if (string(name) == texName) {

				unsigned int w = bin.get();
				w += bin.get() * 256;
				unsigned int h = bin.get();
				h += bin.get() * 256;
				unsigned char fmt = bin.get();
				return { pos + 0x80, w, h, fmt };

			}

		}

	}
	return { 0, 0, 0, 0 };

}

inline void formatTextureBuffer(uint8_t* origBuf, unsigned char* newBuf, const texDataInfo& textureBinData) {

	static int tileOrder[] = { 0, 1, 8, 9, 2, 3, 10, 11, 16, 17, 24, 25, 18, 19, 26, 27, 4, 5, 12, 13, 6, 7, 14, 15, 20, 21, 28, 29, 22, 23, 30, 31, 32, 33, 40, 41, 34, 35, 42, 43, 48, 49, 56, 57, 50, 51, 58, 59, 36, 37, 44, 45, 38, 39, 46, 47, 52, 53, 60, 61, 54, 55, 62, 63 };
	unsigned int outputOffset = 0;
	for (int tY = 0; tY < textureBinData.height / 8; tY++)
	{
		for (int tX = 0; tX < textureBinData.width / 8; tX++)
		{
			for (int pixel = 0; pixel < 64; pixel++)
			{
				int x = tileOrder[pixel] % 8;
				int y = (tileOrder[pixel] - x) / 8;
				long dataOffset = ((tY * 8 + y) * textureBinData.width + tX * 8 + x) * 4;
				
				if (textureBinData.format != 2)	{

					if (textureBinData.format == 4) newBuf[outputOffset++] = origBuf[dataOffset + 3];
					newBuf[outputOffset++] = origBuf[dataOffset + 2];
					newBuf[outputOffset++] = origBuf[dataOffset + 1];
					newBuf[outputOffset++] = origBuf[dataOffset];

				}

				else {

					unsigned char r = origBuf[dataOffset], g = origBuf[dataOffset + 1], b = origBuf[dataOffset + 2];
					newBuf[outputOffset++] = ((g & 0x1c) << 3) | ((b & 0xf8) >> 3);					
					newBuf[outputOffset++] = (r & 0xf8) | ((g & 0xe0) >> 5);

				}
				
			}
		}
	}

}

inline size_t find_nth_of(const string& str, char c, size_t n) {

	size_t pos = 0, len = str.size(), num = 0;
	for (pos; pos < len; pos++) {

		if (str[pos] == c) num++;
		if (num == n) return pos;

	}

	return string::npos;

}

inline bool isNumeric(char c) {

	return '0' <= c && c <= '9';

}