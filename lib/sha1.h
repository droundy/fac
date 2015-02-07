#ifndef SHA1_H
#define SHA1_H

#include <stdint.h>
#include <string.h>

/* header */

#define HASH_LENGTH 20
#define BLOCK_LENGTH 64

typedef struct sha1nfo {
	uint32_t buffer[BLOCK_LENGTH/4];
	uint32_t state[HASH_LENGTH/4];
	uint32_t byteCount;
	uint8_t bufferOffset;
	uint8_t keyBuffer[BLOCK_LENGTH];
	uint8_t innerHash[HASH_LENGTH];
} sha1nfo;

struct struct_hash {
  uint64_t a, b;
  uint32_t c;
};

typedef union sha1hash {
  struct struct_hash abc;
  uint32_t hash32[5];
  uint8_t hash8[HASH_LENGTH];
  unsigned char u8[HASH_LENGTH];
} sha1hash;

void sha1_init(sha1nfo *s);
void sha1_writebyte(sha1nfo *s, uint8_t data);
void sha1_write(sha1nfo *s, const char *data, size_t len);
uint8_t* sha1_result(sha1nfo *s);
sha1hash sha1_out(sha1nfo *s);

#endif
