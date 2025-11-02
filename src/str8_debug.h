#ifndef STR8_DEBUG_H
#define STR8_DEBUG_H

#if defined(DEBUG)
#define STATIC
#define INLINE
#else
#define STATIC static
#define INLINE inline
#endif

#endif