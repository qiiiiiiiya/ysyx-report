#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  const char *p = s;
  while (*p != '\0') {
    p++;
  }
  return p - s;
}

char *strcpy(char *dst, const char *src) {
  char *original_dst = dst;
  while (*src != '\0') {
    *dst++ = *src++;
  } 
  return original_dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
  size_t i;
  for (i = 0; i < n && src[i] != '\0'; i++) {
    dst[i] = src[i];
  } 
  return dst + i;
}

char *strcat(char *dst, const char *src) {
  strcpy(dst + strlen(dst), src);
  return dst;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  while( n-- && *s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return (n < 0) ? 0 : (*(unsigned char *)s1 - *(unsigned char *)s2);
}

void *memset(void *s, int c, size_t n) {
  for(int i=0; i < n; i++) {
    ((unsigned char *)s)[i] = (unsigned char)c;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  while(dst < src || dst >= src + n) {
    if (dst < src) {
      // Copy forward
      *(unsigned char *)dst = *(unsigned char *)src;
      dst++;
      src++;
    } else {
      // Copy backward
      dst--;
      src--;
      *(unsigned char *)dst = *(unsigned char *)src;
    }
  }
  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  while(n--){
    *(unsigned char *)out = *(unsigned char *)in;
    out++;
    in++;
  }
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  while(n--){
    if (*(unsigned char *)s1 != *(unsigned char *)s2) {
      return *(unsigned char *)s1 - *(unsigned char *)s2;
    }
    s1++;
    s2++;
  }
  return 0;
}

#endif
