/*
 * Copyright (C) 2015,2018 Southern Storm Software, Pty Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
Extract from https://github.com/rweather/arduinolibs/

Change to use this project type

*/

#include "aes_tiny.h"
#include <algorithm>
#include <avr/pgmspace.h>
#include <stdint.h>
namespace {

constexpr uint8_t sbox[256] PROGMEM = {
    0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, // 0x00
    0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
    0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, // 0x10
    0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
    0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, // 0x20
    0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
    0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, // 0x30
    0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
    0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, // 0x40
    0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
    0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, // 0x50
    0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
    0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, // 0x60
    0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
    0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, // 0x70
    0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
    0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, // 0x80
    0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
    0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, // 0x90
    0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
    0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, // 0xA0
    0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
    0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, // 0xB0
    0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
    0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, // 0xC0
    0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
    0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, // 0xD0
    0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
    0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, // 0xE0
    0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
    0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, // 0xF0
    0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16};

static inline uint8_t readsbox(uint8_t val) {
  return pgm_read_byte(sbox + val);
}

// Rcon(i), 2^i in the Rijndael finite field, for i = 0..10.
// http://en.wikipedia.org/wiki/Rijndael_key_schedule
constexpr uint8_t const rcon[11] PROGMEM = {0x00, 0x01, 0x02, 0x04,
                                            0x08, 0x10, 0x20, 0x40, // 0x00
                                            0x80, 0x1B, 0x36};

static inline void keyScheduleCore(uint8_t *output, const uint8_t *input,
                                   uint8_t iteration) {

  output[0] = readsbox(input[1]) ^ pgm_read_byte(rcon + iteration);
  output[1] = readsbox(input[2]);
  output[2] = readsbox(input[3]);
  output[3] = readsbox(input[0]);
}

/** @cond aes_funcs */
constexpr uint8_t idxFrom(uint8_t col, uint8_t row) { return col * 4 + row; }

static inline void subBytesAndShiftRows(uint8_t *output, const uint8_t *input) {

  output[idxFrom(0, 0)] = readsbox(input[idxFrom(0, 0)]);

  output[idxFrom(0, 1)] = readsbox(input[idxFrom(1, 1)]);

  output[idxFrom(0, 2)] = readsbox(input[idxFrom(2, 2)]);
  output[idxFrom(0, 3)] = readsbox(input[idxFrom(3, 3)]);
  output[idxFrom(1, 0)] = readsbox(input[idxFrom(1, 0)]);
  output[idxFrom(1, 1)] = readsbox(input[idxFrom(2, 1)]);
  output[idxFrom(1, 2)] = readsbox(input[idxFrom(3, 2)]);
  output[idxFrom(1, 3)] = readsbox(input[idxFrom(0, 3)]);
  output[idxFrom(2, 0)] = readsbox(input[idxFrom(2, 0)]);
  output[idxFrom(2, 1)] = readsbox(input[idxFrom(3, 1)]);
  output[idxFrom(2, 2)] = readsbox(input[idxFrom(0, 2)]);
  output[idxFrom(2, 3)] = readsbox(input[idxFrom(1, 3)]);
  output[idxFrom(3, 0)] = readsbox(input[idxFrom(3, 0)]);
  output[idxFrom(3, 1)] = readsbox(input[idxFrom(0, 1)]);
  output[idxFrom(3, 2)] = readsbox(input[idxFrom(1, 2)]);
  output[idxFrom(3, 3)] = readsbox(input[idxFrom(2, 3)]);
}

static inline void subBytesAndShiftRows(uint8_t *buffer) {

  // TODO readsbox and shuffle in next step

  buffer[idxFrom(0, 0)] = readsbox(buffer[idxFrom(0, 0)]);
  buffer[idxFrom(1, 0)] = readsbox(buffer[idxFrom(1, 0)]);
  buffer[idxFrom(2, 0)] = readsbox(buffer[idxFrom(2, 0)]);
  buffer[idxFrom(3, 0)] = readsbox(buffer[idxFrom(3, 0)]);
  

  const uint8_t temp01 = readsbox(buffer[idxFrom(0, 1)]);
  buffer[idxFrom(0, 1)] = readsbox(buffer[idxFrom(1, 1)]);
  buffer[idxFrom(1, 1)] = readsbox(buffer[idxFrom(2, 1)]);
  buffer[idxFrom(2, 1)] = readsbox(buffer[idxFrom(3, 1)]);
  buffer[idxFrom(3, 1)] = temp01;

  const uint8_t temp02 = readsbox(buffer[idxFrom(0, 2)]);
  buffer[idxFrom(0, 2)] = readsbox(buffer[idxFrom(2, 2)]);
  buffer[idxFrom(2, 2)] = temp02;

  const uint8_t temp03 = readsbox(buffer[idxFrom(0, 3)]);
  buffer[idxFrom(0, 3)] = readsbox(buffer[idxFrom(3, 3)]);
  buffer[idxFrom(3, 3)] = readsbox(buffer[idxFrom(2, 3)]);
  buffer[idxFrom(2, 3)] = readsbox(buffer[idxFrom(1, 3)]);
  buffer[idxFrom(1, 3)] = temp03;
  

  const uint8_t temp12 = readsbox(buffer[idxFrom(1, 2)]);
  buffer[idxFrom(1, 2)] = readsbox(buffer[idxFrom(3, 2)]);
  buffer[idxFrom(3, 2)] = temp12;
  
}

// Multiply x by 2 in the Galois field, to achieve the effect of the following:
//
//     if (x & 0x80)
//         return (x << 1) ^ 0x1B;
//     else
//         return (x << 1);
//
// However, we don't want to use runtime conditionals if we can help it
// to avoid leaking timing information from the implementation.
// In this case, multiplication is slightly faster than table lookup on AVR.
/* #define gmul2(x) \
  (t = ((uint16_t)(x)) << 1,                                                   \
   ((uint8_t)t) ^ (uint8_t)(0x1B * ((uint8_t)(t >> 8))))
*/
static inline uint8_t gmul2(uint8_t const x) {
  uint8_t const t = ((uint16_t)(x)) << 1;
  return ((uint8_t)t) ^ (uint8_t)(0x1B * ((uint8_t)(t >> 8)));
}
static inline void mixColumn(uint8_t *output, uint8_t const *input) {
  uint8_t const a = input[0];
  uint8_t const b = input[1];
  uint8_t const c = input[2];
  uint8_t const d = input[3];
  uint8_t const a2 = gmul2(a);
  uint8_t const b2 = gmul2(b);
  uint8_t const c2 = gmul2(c);
  uint8_t const d2 = gmul2(d);
  output[0] = a2 ^ b2 ^ b ^ c ^ d;
  output[1] = a ^ b2 ^ c2 ^ c ^ d;
  output[2] = a ^ b ^ c2 ^ d2 ^ d;
  output[3] = a2 ^ a ^ b ^ c ^ d2;
}

static inline void kcore(uint8_t n, uint8_t schedule[16]) {
  uint8_t temp[4];
  keyScheduleCore(temp, schedule + 12, n);
  schedule[0] ^= temp[0];
  schedule[1] ^= temp[1];
  schedule[2] ^= temp[2];
  schedule[3] ^= temp[3];
}

static inline void kxor(uint8_t a, uint8_t b, uint8_t schedule[16]) {
  schedule[a * 4] ^= schedule[b * 4];
  schedule[a * 4 + 1] ^= schedule[b * 4 + 1];
  schedule[a * 4 + 2] ^= schedule[b * 4 + 2];
  schedule[a * 4 + 3] ^= schedule[b * 4 + 3];
}
} // namespace

void aes_tiny_128_encrypt(uint8_t *buffer, AesKey const &key) {
  uint8_t schedule[16];
  uint8_t posn;
  uint8_t state1[16];
  uint8_t state2[16];

  // Start with the key in the schedule buffer.
  std::copy(key.data, key.data + key.key_size, schedule);

  // Copy the input into the state and XOR with the key schedule.
  for (posn = 0; posn < 16; ++posn)
    state1[posn] = buffer[posn] ^ schedule[posn];

  // Perform the first 9 rounds of the cipher.
  for (uint8_t round = 1; round <= 9; ++round) {
    // Expand the next 16 bytes of the key schedule.
    kcore(round, schedule);
    kxor(1, 0, schedule);
    kxor(2, 1, schedule);
    kxor(3, 2, schedule);

    // Encrypt using the key schedule.
    subBytesAndShiftRows(state2, state1);
    mixColumn(state1, state2);
    mixColumn(state1 + 4, state2 + 4);
    mixColumn(state1 + 8, state2 + 8);
    mixColumn(state1 + 12, state2 + 12);
    for (posn = 0; posn < 16; ++posn)
      state1[posn] ^= schedule[posn];
  }

  // Expand the final 16 bytes of the key schedule.
  kcore(10, schedule);
  kxor(1, 0, schedule);
  kxor(2, 1, schedule);
  kxor(3, 2, schedule);

  // Perform the final round.
  subBytesAndShiftRows(state2, state1);
  for (posn = 0; posn < 16; ++posn)
    buffer[posn] = state2[posn] ^ schedule[posn];
}
