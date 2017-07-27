#ifndef ALIGNMENT_H
#define ALIGNMENT_H

#if defined(_MSC_VER)
#define ALIGNED(x) __declspec(align(x))
#else
#if defined(__GNUC__)
#define ALIGNED(x) __attribute__ ((aligned(x)))
#endif
#endif

#endif // ALIGNMENT_H