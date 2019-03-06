#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "CRC.hpp"
#include "Deflate.hpp"

using std::vector;
using std::string;

//Chunks for PNG.
class Chunk {
	
	private:
		
		string type;
		uint32_t length;
		uint8_t* data;
		uint32_t crc;
		bool fromFile;
				
	public:
		
		Chunk(char* start);
		Chunk(string typ, uint32_t len, uint8_t* dat, uint32_t CRC);
		Chunk(const Chunk&) = delete;
		Chunk(Chunk&&);
		Chunk& operator=(const Chunk&) = delete;
		Chunk& operator=(const Chunk&&) = delete;
		~Chunk();
		
		string getType() const;
		uint32_t getLength() const;
		uint8_t* getData() const;
		uint32_t getCRC() const;
		void toBytes(string& buf) const;
		
};

class PNG_RW {
	
	private:
		
		string fileName;
		uint32_t width, height;
		uint8_t depth, color, comp, filt, ilace;
		
		Deflate deflate;
		vector<Chunk> chunks;
		uint8_t* image;		
		
		char* acquire(string fullPath);
		void output(vector<uint8_t>& zlib, string& fullPath) const;
		void parse(char*& buffer, int fileSize, vector<uint8_t>& zlib);
		
		void filter(vector<uint8_t>& data) const;
		void reconstruct(vector<uint8_t>& data);

		void clearImage();
		void constructImage();
	
	public:
		
		PNG_RW();
		PNG_RW(const PNG_RW&) = delete;
		~PNG_RW();
		
		void read(string fullPath);
		void write(string fullPath);
		
		string getName() const;
		uint8_t* getImage() const;
		uint32_t getWidth() const;
		uint32_t getHeight() const;
		uint8_t getColor() const;		
	
};