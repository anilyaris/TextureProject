#pragma once

#include <cstdint>
#include <vector>
#include <array>

using std::vector;
using std::array;

//Node content of the Huffmann tree.
struct Entry {
	
	bool isFull;
	uint16_t val;
	uint8_t extra;
			
};

//Hufmann tree stored as a binary heap.
struct HuffmanTree {
	
	vector<Entry> heap;
	unsigned int offset;
	
};


class Deflate {
	
	private:
	
		HuffmanTree litlen, dist, temp;
		array<Entry, 288> std_litlen;
		array<Entry, 32> std_dist;
		array<Entry, 19> std_dynamic;
		uint8_t dyn_order[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
		uint8_t mode;
		bool std_constructed;
		
		void huffman(Entry* begin, unsigned int length, HuffmanTree& tree) const;
		unsigned int calculatePos(int code, int len, int offset) const;
		Entry& findLeaf (HuffmanTree& tree, vector<uint8_t>& buf, unsigned int& pos) const;
		void writeCode(uint16_t code, vector<uint8_t>& zlib, uint8_t& residue, uint8_t& residueSize) const;
		
	public:
		
		Deflate();
		Deflate(const Deflate&) = delete;	
		Deflate& operator=(const Deflate&) = delete;
		
		void decompress(vector<uint8_t>& zlib, vector<uint8_t>& decompressed);
		void compress(vector<uint8_t>& zlib, vector<uint8_t>& filtered) const;
		
};