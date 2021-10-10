
#ifndef __aes_tiny_h__
#define __aes_tiny_h__

#include <array>
#include <stdint.h>

constexpr uint8_t key_size = 16;
using AesKey = std::array<uint8_t, key_size>;

void aes_tiny_128_encrypt(uint8_t *buffer, AesKey const &key);

#ifdef ARDUINO_ARCH_ESP32
void aes_esp_128_encrypt(uint8_t *buffer, AesKey const &key);
constexpr auto aes_128_encrypt = aes_esp_128_encrypt;
#else
constexpr auto aes_128_encrypt = aes_tiny_128_encrypt;
#endif

#endif