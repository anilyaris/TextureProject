#include <cstdint>
#include <string>
#include <vector>
#include "CRC.hpp"

using std::string;
using std::vector;

uint32_t CRC::rotr8(uint32_t val) {
	
	for (int round = 0; round < 8; round++) {
		val = val % 2 == 1 ? pol ^ (val >> 1) : val >> 1;
	}
	
	return val;
	
}

CRC::CRC(uint32_t p) {
	
	pol = p;
	for (int index = 0; index < 256; index++) table[index] = rotr8(index);
	
}

//Calcuates the CRC code of a buffer ("type" argument is for PNG chunks).
uint32_t CRC::calculate(string type, vector<uint8_t>& buf) {
	
	uint32_t crc = 0xffffffff;

	for (uint8_t chr : type) {

		int lookupIndex = (crc ^ chr) & 0xff;
		crc = table[lookupIndex] ^ (crc >> 8);

	}
	
	for (uint8_t chr : buf) {
		
		int lookupIndex = (crc ^ chr) & 0xff;
		crc = table[lookupIndex] ^ (crc >> 8);
		
	}
	
	return crc ^ 0xffffffff;
	
}