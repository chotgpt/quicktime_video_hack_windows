#include "gcdict.h"
#include <assert.h>


gcdict::gcdict()
{
	this->vec_data = { 0x08,0x00,0x00,0x00,0x74,0x63,0x69,0x64 };	//"dict"
}

gcdict::~gcdict()
{
}

void gcdict::add_item_dict(const char* key, const gcdict& item)
{
	assert(key != nullptr);
	std::vector<unsigned char> tmp_vec;
	//size_t tmp_dict_size = 4 + strlen("keyv") + 4 + strlen("strk") + strlen(key) + 4 + strlen("nmbv") + 1 + sizeof(double);
	size_t str_key_length = strlen(key);


	size_t key_size = 8 + str_key_length;		//4 + strlen("strk") + strlen(key);
	size_t val_size = item.vec_data.size();		//item.vec_data.size()
	size_t tmp_vec_size = 8 + key_size + val_size;

	tmp_vec.resize(tmp_vec_size);
	unsigned char* ptr = tmp_vec.data();

	*(int*)(ptr) = (int)tmp_vec_size;
	ptr += 4;

	memcpy(ptr, "vyek", 4);	//keyv
	ptr += 4;

	*(int*)(ptr) = (int)key_size;
	ptr += 4;

	memcpy(ptr, "krts", 4);	//strk
	ptr += 4;

	memcpy(ptr, key, str_key_length);
	ptr += str_key_length;

	memcpy(ptr, item.vec_data.data(), item.vec_data.size());

	size_t old_size = this->vec_data.size();
	this->vec_data.resize(old_size + tmp_vec_size);
	memcpy(this->vec_data.data() + old_size, tmp_vec.data(), tmp_vec_size);
	*(int*)(this->vec_data.data()) = int(*(int*)(this->vec_data.data()) + tmp_vec_size);
}

void gcdict::add_item_uint(const char* key, uint64_t val)
{
	assert(key != nullptr);
	std::vector<unsigned char> tmp_vec;
	//size_t tmp_dict_size = 4 + strlen("keyv") + 4 + strlen("strk") + strlen(key) + 4 + strlen("nmbv") + 1 + sizeof(double);
	size_t str_key_length = strlen(key);


	size_t key_size = 8 + str_key_length;	//4 + strlen("strk") + strlen(key);
	size_t val_size = 17;				//4 + strlen("nmbv") + 1 + sizeof(double);
	size_t tmp_vec_size = 8 + key_size + val_size;

	tmp_vec.resize(tmp_vec_size);
	unsigned char* ptr = tmp_vec.data();

	*(int*)(ptr) = (int)tmp_vec_size;
	ptr += 4;

	memcpy(ptr, "vyek", 4);	//keyv
	ptr += 4;

	*(int*)(ptr) = (int)key_size;
	ptr += 4;

	memcpy(ptr, "krts", 4);	//strk
	ptr += 4;

	memcpy(ptr, key, str_key_length);
	ptr += str_key_length;

	*(int*)(ptr) = (int)val_size;
	ptr += 4;

	memcpy(ptr, "vbmn", 4);	//nmbv
	ptr += 4;

	*(unsigned char*)(ptr) = 0x06;
	ptr += 1;

	*(double*)(ptr) = (double)val;

	size_t old_size = this->vec_data.size();
	this->vec_data.resize(old_size + tmp_vec_size);
	memcpy(this->vec_data.data() + old_size, tmp_vec.data(), tmp_vec_size);
	*(int*)(this->vec_data.data()) = int(*(int*)(this->vec_data.data()) + tmp_vec_size);
}

void gcdict::add_item_bool(const char* key, bool val)
{
	assert(key != nullptr);
	std::vector<unsigned char> tmp_vec;
	//size_t tmp_dict_size = 4 + strlen("keyv") + 4 + strlen("strk") + strlen(key) + 4 + strlen("nmbv") + 1 + sizeof(double);
	size_t str_key_length = strlen(key);


	size_t key_size = 8 + str_key_length;	//4 + strlen("strk") + strlen(key);
	size_t val_size = 9;				//4 + strlen("bulv") + 1
	size_t tmp_vec_size = 8 + key_size + val_size;

	tmp_vec.resize(tmp_vec_size);
	unsigned char* ptr = tmp_vec.data();

	*(int*)(ptr) = (int)tmp_vec_size;
	ptr += 4;

	memcpy(ptr, "vyek", 4);	//keyv
	ptr += 4;

	*(int*)(ptr) = (int)key_size;
	ptr += 4;

	memcpy(ptr, "krts", 4);	//strk
	ptr += 4;

	memcpy(ptr, key, str_key_length);
	ptr += str_key_length;

	*(int*)(ptr) = (int)val_size;
	ptr += 4;

	memcpy(ptr, "vlub", 4);	//bulv
	ptr += 4;

	*(unsigned char*)(ptr) = val ? 0x01 : 0x00;
	ptr += 1;

	size_t old_size = this->vec_data.size();
	this->vec_data.resize(old_size + tmp_vec_size);
	memcpy(this->vec_data.data() + old_size, tmp_vec.data(), tmp_vec_size);
	*(int*)(this->vec_data.data()) = int(*(int*)(this->vec_data.data()) + tmp_vec_size);
}


