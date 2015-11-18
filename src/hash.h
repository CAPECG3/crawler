#ifndef _HASH_H_
#define _HASH_H_
#define K 11 //¹þÏ£º¯Êý¸öÊý
class Hash {
public:
	static unsigned int RSHash(const char* str, unsigned int len);
	static unsigned int JSHash(const char* str, unsigned int len);
 	static unsigned int PJWHash(const char* str, unsigned int len);
	static unsigned int ELFHash(const char* str, unsigned int len);
	static unsigned int BKDRHash(const char* str, unsigned int len);
	static unsigned int SDBMHash(const char* str, unsigned int len);
	static unsigned int DJBHash(const char* str, unsigned int len);
	static unsigned int DEKHash(const char* str, unsigned int len);
	static unsigned int BPHash(const char* str, unsigned int len);
	static unsigned int FNVHash(const char* str, unsigned int len);
	static unsigned int APHash(const char* str, unsigned int len);
	unsigned int(*hashFamily[K])(const char *str, unsigned int len);
	Hash() {
		hashFamily[0] = Hash::RSHash;
		hashFamily[1] = Hash::JSHash;
		hashFamily[2] = Hash::PJWHash;
		hashFamily[3] = Hash::ELFHash;
		hashFamily[4] = Hash::BKDRHash;
		hashFamily[5] = Hash::SDBMHash;
		hashFamily[6] = Hash::DJBHash;
		hashFamily[7] = Hash::DEKHash;
		hashFamily[8] = Hash::BPHash;
		hashFamily[9] = Hash::FNVHash;
		hashFamily[10] = Hash::APHash;
	}
};
#endif