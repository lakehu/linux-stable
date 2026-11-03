/*
 *  NOTE!!! memcpy(dest,src,n) assumes ds=es=normal data segment. This
 *  goes for all kernel functions (ds=es=kernel space, fs=local data,
 *  gs=null), as well as for all well-behaving user programs (ds=es=
 *  user data space). This is NOT a bug, as any user program that changes
 *  es deserves to die if it isn't careful.
 */
#define memcpy(dest,src,n) ({ \
void * _res = dest; \
int d0, d1, d2; \
__asm__ __volatile__ ("cld;rep;movsb" \
	:"=&D" (d0),"=&S" (d1),"=&c" (d2) \
	:"0" ((long)(_res)),"1" ((long)(src)),"2" ((long) (n)) \
	:"memory"); \
_res; \
})
