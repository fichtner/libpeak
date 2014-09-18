#undef strlcpy
#define strlcpy compat_strlcpy

size_t compat_strlcpy(char *, const char *, size_t);
