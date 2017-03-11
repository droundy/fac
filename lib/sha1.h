#ifndef SHA1_H
#define SHA1_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

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
void sha1_write(sha1nfo *s, const char *data, size_t len);
uint8_t* sha1_result(sha1nfo *s);

sha1hash sha1_out(sha1nfo *s);
void fprint_sha1(FILE *f, sha1hash h);
sha1hash read_sha1(const char *str);
static inline bool sha1_is_zero(sha1hash h) {
  return h.abc.a == 0 && h.abc.b == 0 && h.abc.c == 0;
}
static inline bool sha1_same(sha1hash h1, sha1hash h2) {
  return h1.abc.a == h2.abc.a && h1.abc.b == h2.abc.b && h1.abc.c == h2.abc.c;
}

#endif
