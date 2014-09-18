#undef explicit_bzero
#define explicit_bzero compat_explicit_bzero

void compat_explicit_bzero(void *, size_t);
