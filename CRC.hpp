#pragma once

#include <cstdint>
#include <string>
#include <vector>

using std::string;
using std::vector;

class CRC {
	
	private:
		
		uint32_t pol;
		uint32_t table[256];
		uint32_t rotr8(uint32_t val);		
	
	public:
		
		CRC(uint32_t p = 0xedb88320);
		uint32_t calculate(string type, vector<uint8_t>& buf);		
	
};