#include <cstdint>
#include <vector>
#include <string>
#include <algorithm>
#include "Deflate.hpp"
#include "util.hpp"

using std::vector;
using std::string;
using std::min;

Deflate::Deflate() {
	
	for (int i = 0; i < 32; i++) {
		
		unsigned int ex = i / 2 - (i >= 2);
		std_dist[i] = {false, (uint16_t)i, (uint8_t)((ex << 4) ^ 5)};
		
	}	
	
	for (int i = 0; i < 288; i++) {
		
		unsigned int ex = (i < 265 || i >= 285) ? 0 : (i - 261) / 4;
		unsigned int len =  8 + (i >= 144 && i <= 255) - (i >= 256 && i <= 279);
		std_litlen[i] = {false, (uint16_t)i, (uint8_t)((ex << 4) ^ len)};
		
	}
	
	for (int i = 0; i < 19; i++) {
		
		unsigned int ex = i >= 16 ? (3 * (i - 16) * (i - 16) - (i - 16) + 4) / 2 : 0;
		std_dynamic[i] = {false, (uint16_t)i, (uint8_t)(ex << 4)};
		
	}
	
	mode = 0;
	std_constructed = false;
	
}

//Given a Huffmann code and its length, calculates its position within the Huffmann tree heap.
unsigned int Deflate::calculatePos(int code, int len, int offset) const {
	
	int pos = -1;
	int mask = 0x01 << (len - 1);
	
	while (mask) {
		
		pos = 2 * (pos + 1) + ((code & mask) != 0);
		mask >>= 1;
		
	}
	
	return pos - offset;
		
}

//Returns the Huffmann code of a given literal appended to its length.
uint16_t findCode(uint8_t byte) {

	return byte < 144 ? (8 << 12) | (48 + byte) : (9 << 12) | (256 + byte);

}

//Gets a Huffmann code appended to its length and writes it into the output buffer.
void Deflate::writeCode(uint16_t code, vector<uint8_t>& zlib, uint8_t& residue, uint8_t& residueSize) const {

	uint8_t len = code >> 12;

	while (residueSize != 8) {

		uint8_t bit = (((0x01 << --len) & code) >> len) << residueSize++;
		residue |= bit;

	}

	zlib.push_back(residue);
	residue = residueSize = 0;

	while (len > 0) {

		uint8_t bit = (((0x01 << --len) & code) >> len) << residueSize++;
		residue |= bit;

	}

	if (residueSize == 8) {

		zlib.push_back(residue);
		residue = residueSize = 0;

	}

}

//Finds the next Huffman tree entry from the buffer of Huffman codes.
Entry& Deflate::findLeaf(HuffmanTree& tree, vector<uint8_t>& buf, unsigned int& pos) const {
	
	int index = -1;
	while (index < (int)tree.offset) index = 2 * (index + 1) + readBits(buf, 1, pos);
	
	bool isLeaf = tree.heap[index - tree.offset].isFull;
	while (!isLeaf) {
		
		index = 2 * (index + 1) + readBits(buf, 1, pos);
		isLeaf = tree.heap[index - tree.offset].isFull;
		
	}
	
	return tree.heap.at(index - tree.offset);
	
}

//Builds the Huffman tree from an array of node entries.
void Deflate::huffman(Entry* begin, unsigned int length, HuffmanTree& tree) const {

	int MIN_BITS = 16;
	vector<int>	bl_count;
	for (int i = 0; i < length; i++) {
		
		int len = begin[i].extra & 0x0F;
		if (len != 0 && len < MIN_BITS) MIN_BITS = len;
		while (bl_count.size() <= len) bl_count.push_back(0);
		bl_count[len]++;		
		
	}
	
	int code = 0;
	int MAX_BITS = bl_count.size();
	int* next_code = new int[MAX_BITS];
	next_code[0] = 0;
	
    bl_count[0] = 0;    
    for (int bits = 1; bits < MAX_BITS; bits++) {
    	
        code = (code + bl_count[bits-1]) << 1;
        next_code[bits] = code;
		        
    }
    
    tree.heap.clear();
	tree.offset = calculatePos(0, MIN_BITS, 0);
	
    
    for (int n = 0;  n < length; n++) {
    	
        unsigned int len = begin[n].extra & 0x0F;
        if (len != 0) {
        	
			int pos = calculatePos(next_code[len], len, tree.offset);
			while (tree.heap.size() <= pos) tree.heap.push_back({false, 0, 0});                
            tree.heap[pos] = {true, (uint16_t)begin[n].val, (uint8_t)(begin[n].extra >> 4)};
            next_code[len]++;
            
        }
        
	}

	delete[] next_code;
	
}

//Performs compression on PNG output buffer (currently no compression is implemented).
void Deflate::compress(vector<uint8_t>& zlib, vector<uint8_t>& filtered) const {

	zlib.push_back(0x78);
	zlib.push_back(0x5E);

	uint8_t residue = 3;
	uint8_t residueSize = 3;

	for (uint8_t byte : filtered) writeCode(findCode(byte), zlib, residue, residueSize);
	writeCode((uint16_t)7 << 12, zlib, residue, residueSize);

	if (residueSize != 0) zlib.push_back(residue);

	unsigned long adler = adler32(zlib, zlib.size());

	zlib.push_back(adler >> 24);
	zlib.push_back(adler >> 16);
	zlib.push_back(adler >> 8);
	zlib.push_back(adler);

}

//Performs decompression on PNG input buffer.
void Deflate::decompress(vector<uint8_t>& zlib, vector<uint8_t>& decompressed) {
	
	if (zlib.size() == 0) throw string("Decompression error: Input stream empty.");

	bool isLastBlock = false;
	unsigned int pos = 2 * 8;
	do {
		
		isLastBlock = readBits(zlib, 1, pos);
		mode = readBits(zlib, 2, pos);
		
		if (mode == 0) {
			
			while (pos % 8 != 0) pos++;
			uint16_t LEN = readBits(zlib, 8, pos);
			LEN ^= readBits(zlib, 8, pos) << 8;
			uint16_t NLEN = readBits(zlib, 8, pos);
			NLEN ^= readBits(zlib, 8, pos) << 8;
			
			if ((unsigned int)(LEN ^ NLEN) != 0xffff) throw string("Decompression error: Invalid input format.");
			
			unsigned int index = pos >> 3;
			decompressed.insert(decompressed.end(), zlib.begin() + index, zlib.begin() + index + LEN);
			
			pos += LEN * 8;			
			
		}
		
		else if (mode >= 3) throw string("Decompression error: Invalid compression mode.");
		
		else  {
			
			if (mode == 2) {
				
				uint16_t HLIT = readBits(zlib, 5, pos) + 257;
				uint8_t HDIST = readBits(zlib, 5, pos) + 1;
				uint8_t HCLEN = readBits(zlib, 4, pos) + 4;

				for (int i = 0; i < 19; i++) std_dynamic[i].extra &= 0xF0;				
				for (int i = 0; i < HCLEN; i++) {
					
					uint8_t codeLen = readBits(zlib, 3, pos);					
					std_dynamic[dyn_order[i]].extra ^= codeLen;
					
				}
				
				huffman(&std_dynamic[0], std_dynamic.size(), temp);
				
				vector<unsigned int> lengths;
				
				vector<Entry> dyn_litlen;
				while (lengths.size() < HLIT) {
					
					Entry& entry =  findLeaf(temp, zlib, pos);
					if (entry.val < 16) lengths.push_back(entry.val);
					else if (entry.val == 16) {
						
						unsigned int times = readBits(zlib, entry.extra, pos) + 3;
						while (times-- > 0) lengths.push_back(lengths.back());
						
					}
					else if (entry.val <= 18) {
						
						unsigned int times = readBits(zlib, entry.extra, pos) + 8 * (entry.val - 17) + 3;
						while (times-- > 0) lengths.push_back(0);
						
					}
					else throw string("Decompression error: Could not parse data.");					
					
				}
				for (int i = 0; i < lengths.size(); i++) {
					
					unsigned int ex = (i < 265 || i >= 285) ? 0 : (i - 261) / 4;
					unsigned int len =  lengths[i];
					dyn_litlen.push_back({false, (uint16_t)i, (uint8_t)((ex << 4) ^ len)});
					
				}
				
				lengths.clear();
				
				vector<Entry> dyn_dist;
				while (lengths.size() < HDIST) {
					
					Entry& entry =  findLeaf(temp, zlib, pos);
					if (entry.val < 16) lengths.push_back(entry.val);
					else if (entry.val == 16) {
						
						unsigned int times = readBits(zlib, entry.extra, pos) + 3;
						while (times-- > 0) lengths.push_back(lengths.back());
						
					}
					else if (entry.val <= 18) {
						
						unsigned int times = readBits(zlib, entry.extra, pos) + 8 * (entry.val - 17) + 3;
						while (times-- > 0) lengths.push_back(0);
						
					}
					else throw string("Decompression error: Could not parse data.");					
					
				}				
				for (int i = 0; i < lengths.size(); i++) {
		
					unsigned int ex = i / 2 - (i >= 2);
					unsigned int len =  lengths[i];
					dyn_dist.push_back({false, (uint16_t)i, (uint8_t)((ex << 4) ^ len)});
					
				}
				
				huffman(&dyn_litlen[0], dyn_litlen.size(), litlen);
				huffman(&dyn_dist[0], dyn_dist.size(), dist);
				std_constructed = false;
				
			}
			
			else if (!std_constructed) {
				
				huffman(&std_litlen[0], std_litlen.size(), litlen);
				huffman(&std_dist[0], std_dist.size(), dist);
				std_constructed = true;
				
			}
			
			unsigned int value = 0;
			while (value != 256) {

				Entry& entry = findLeaf(litlen, zlib, pos);
				value = entry.val;
				
				if (value < 256) decompressed.push_back(value);
				else if (value >= 257 && value <= 285) {
					
					unsigned int length = value - 254;
					if (value >= 265) length += (value - 265);
					if (value >= 269) length += 2 * (value - 269);
					if (value >= 273) length += 4 * (value - 273);
					if (value >= 277) length += 8 * (value - 277);
					if (value >= 281) length += 16 * (value - 281);
					if (value == 285) length--;
					length += readBits(zlib, entry.extra, pos);
					
					Entry& entry2 = findLeaf(dist, zlib, pos);
					unsigned int distCode = entry2.val;
					if(distCode <= 29) {
						
						unsigned int distance = distCode + 1;
						if (distCode >= 4) distance += (distCode - 4);
						if (distCode >= 6) distance += 2 * (distCode - 6);
						if (distCode >= 8) distance += 4 * (distCode - 8);
						if (distCode >= 10) distance += 8 * (distCode - 10);
						if (distCode >= 12) distance += 16 * (distCode - 12);
						if (distCode >= 14) distance += 32 * (distCode - 14);
						if (distCode >= 16) distance += 64 * (distCode - 16);
						if (distCode >= 18) distance += 128 * (distCode - 18);
						if (distCode >= 20) distance += 256 * (distCode - 20);
						if (distCode >= 22) distance += 512 * (distCode - 22);
						if (distCode >= 24) distance += 1024 * (distCode - 24);
						if (distCode >= 26) distance += 2048 * (distCode - 26);
						if (distCode >= 28) distance += 4096 * (distCode - 28);

						int ex = entry2.extra;
						unsigned int readEx = 0;
						unsigned int shift = 0;
						while (ex > 0) {

							readEx += readBits(zlib, min((int)ex, 8), pos) << shift;
							shift += 8;
							ex -= 8;

						}
						distance += readEx;
						
						while (length-- > 0) decompressed.push_back(*(decompressed.end() - distance));						
						
					}					
					else throw string("Decompression error: Could not parse data.");
					
				}
				else if (value > 285) throw string("Decompression error: Could not parse data.");				
				
			}
			
			
		}	
		
	} while(!isLastBlock);
	
}