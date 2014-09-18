#undef strlcat
#define strlcat compat_strlcat

size_t compat_strlcat(char *, const char *, size_t);
