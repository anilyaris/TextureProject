//Header for utility functions.

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <CL/cl.hpp>

#define RAND -1

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
