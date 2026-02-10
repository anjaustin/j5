/**
 * stdlib_support.c - Minimal stdlib implementations for FatFs
 */

#include <stdint.h>
#include <stddef.h>

// Get current time for FAT timestamps (simplified - returns fixed time)
uint32_t get_fattime(void) {
    // Return: bit31:25 Year from 1980 (0-127)
    //         bit24:21 Month (1-12)
    //         bit20:16 Day (1-31)
    //         bit15:11 Hour (0-23)
    //         bit10:5  Minute (0-59)
    //         bit4:0   Second/2 (0-29)
    // Setting to: 2026/02/10 12:00:00
    return ((2026 - 1980) << 25) | (2 << 21) | (10 << 16) | (12 << 11) | (0 << 5) | (0 >> 1);
}

void *memset(void *s, int c, size_t n) {
    uint8_t *p = s;
    while (n--) *p++ = (uint8_t)c;
    return s;
}

void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t *d = dest;
    const uint8_t *s = src;
    while (n--) *d++ = *s++;
    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = s1;
    const uint8_t *p2 = s2;
    while (n--) {
        if (*p1 != *p2) return *p1 - *p2;
        p1++;
        p2++;
    }
    return 0;
}

char *strchr(const char *s, int c) {
    while (*s) {
        if (*s == (char)c) return (char *)s;
        s++;
    }
    return NULL;
}
