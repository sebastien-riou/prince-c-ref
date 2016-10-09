/*
Copyright 2016 Sebastien Riou
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "stdio.h"
#include "time.h"
#include <random>

#include <string.h>
#define TEST_FAIL 0
#define TEST_PASS 1

#include <stdio.h>
#include <stdint.h>

static void print_bytes_sep(const char *msg,const unsigned char *buf, unsigned int size, const char m2[], const char sep[]){
	unsigned int i;
	printf("%s",msg);
	for(i=0;i<size-1;i++) printf("%02X%s",buf[i],sep);
	if(i<size) printf("%02X",buf[i]);
	printf("%s", m2);
}
static void print_bytes(const char m[],const uint8_t *buf, unsigned int size, const char m2[]){print_bytes_sep(m,buf,size,m2," ");}
static void println_bytes(const char m[],const uint8_t *buf, unsigned int size){print_bytes(m,buf,size,"\n");}
static void print_128(const char m[], const uint8_t a[16], const char m2[]){
	print_bytes_sep( m,a   ,4,"_","");
	print_bytes_sep("",a+4 ,4,"_","");
	print_bytes_sep("",a+8 ,4,"_","");
	print_bytes_sep("",a+12,4,m2 ,"");
}
static void print_64 (const char m[], const uint8_t a[ 8], const char m2[]){
	print_bytes_sep( m,a   ,4,"_","");
	print_bytes_sep("",a+4 ,4,m2 ,"");
}
static void println_128(const char m[], const uint8_t a[16]){print_128(m,a,"\n");}
static void println_64 (const char m[], const uint8_t a[ 8]){print_64 (m,a,"\n");}


int hexdigit_value(char c){
	int nibble = -1;
	if(('0'<=c) && (c<='9')) nibble = c-'0';
	if(('a'<=c) && (c<='f')) nibble = c-'a' + 10;
	if(('A'<=c) && (c<='F')) nibble = c-'A' + 10;	
	return nibble;
}

int is_hex_digit(char c){
	return -1!=hexdigit_value(c);
}

void hexstr_to_bytes(unsigned int size_in_bytes, void *dst, const char * const hexstr){
	unsigned int char_index = 0;
	unsigned int hexdigit_cnt=0;
	uint8_t* dst_bytes = (uint8_t*)dst;
	memset(dst,0,size_in_bytes);
	while(hexdigit_cnt<size_in_bytes*2){
		char c = hexstr[char_index++];
		if(0==c) {
			printf("\nERROR: could not find %d hex digits in string '%s'.\n",size_in_bytes*2,hexstr);
			printf("char_index=%d, hexdigit_cnt=%d\n",char_index,hexdigit_cnt);
			exit(-1);
		}
		if(is_hex_digit(c)) {
			unsigned int shift = 4 - 4*(hexdigit_cnt & 1);
			uint8_t nibble = hexdigit_value(c);	
			dst_bytes[hexdigit_cnt/2] |= nibble << shift;
			hexdigit_cnt++;
		}
	}
}

void bytes_to_hexstr(char *dst,uint8_t *bytes, unsigned int nBytes){
	unsigned int i;
	for(i=0;i<nBytes;i++){
		sprintf(dst+2*i,"%02X",bytes[i]);
	}
}

void assert_equal_bytes(const uint8_t *const a, const uint8_t *const b, unsigned int size_in_bytes){
	for(unsigned int i=0;i<size_in_bytes;i++){
		if(a[i]!=b[i]){
			printf("\nERROR: assertion failed.\n");
			print_bytes_sep("a=",a,size_in_bytes,"\n"," ");
			print_bytes_sep("b=",b,size_in_bytes,"\n"," ");
			printf("a[%u]=0x%02X\nb[%u]=0x%02X\n",i,a[i],i,b[i]);
			exit(-2);
		}
	}
}

typedef struct testvector_struct {
  uint8_t plain [8];
  uint8_t k0    [8];
  uint8_t k1    [8];
  uint8_t cipher[8];
} testvector_t;

static const char * testvectors[] = {
	//test vectors from original Prince paper (http://eprint.iacr.org/2012/529.pdf)
	"     plain              k0               k1             cipher     ",
	"0000000000000000 0000000000000000 0000000000000000 818665aa0d02dfda",
	"ffffffffffffffff 0000000000000000 0000000000000000 604ae6ca03c20ada",
	"0000000000000000 ffffffffffffffff 0000000000000000 9fb51935fc3df524",
	"0000000000000000 0000000000000000 ffffffffffffffff 78a54cbe737bb7ef",
	"0123456789abcdef 0000000000000000 fedcba9876543210 ae25ad3ca8fa9ccf",
	//custom test vectors from here, cipher values generated with prince_ref.h !
	"0123456789ABCDEF 0011223344556677 8899AABBCCDDEEFF D6DCB5978DE756EE",
	"0123456789ABCDEF 0112233445566778 899AABBCCDDEEFF0 392F599F46761CD3",
	"F0123456789ABCDE 0112233445566778 899AABBCCDDEEFF0 4FB5E332B9B409BB",
	"69C4E0D86A7B0430 D8CDB78070B4C55A 818665AA0D02DFDA 43C6B256D79DE7E8"
};   

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
//#define PRINCE_PRINT(a) printf("\t%016" PRIx64 " %s\n",a,STR(a));
#define PRINCE_PRINT(a) do{\
		uint8_t raw_bytes[8];\
		uint64_to_bytes(a,raw_bytes);\
		print_64("\t",raw_bytes," " STR(a) "\n");\
	}while(0)
#include "prince_ref.h"
int basic_test(void){
	for(int i=1;i<sizeof(testvectors)/sizeof(char*);i++){
		testvector_t tv;
		hexstr_to_bytes(sizeof(tv), &tv, testvectors[i]);
		printf("test vector %d\n",i);
		print_64  ( "plain:",tv.plain ," ");
		print_64  (    "k0:",tv.k0    ," ");
		print_64  (    "k1:",tv.k1    ," ");
		println_64("cipher:",tv.cipher);
		printf("encryption:\n");
		uint8_t result[8];
		uint8_t key[16];
		memcpy(key  ,tv.k0,8);
		memcpy(key+8,tv.k1,8);
		println_128("key:",key);
		prince_encrypt(tv.plain,key,result);
		assert_equal_bytes(tv.cipher,result,8);
		printf("decryption:\n");
		prince_decrypt(tv.cipher,key,result);
		assert_equal_bytes(tv.plain,result,8);
	}
	return TEST_PASS;
}

int main(void){
	if(TEST_PASS==basic_test()){
		printf("Basic Test PASS\n");
	} else {
		printf("Basic Test FAIL\n");
	}
	return 0;
}
