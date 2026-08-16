// quark's userspace executables link with -nostdlib and provide their own
// entry point (start.asm), so GCC's normal crtbegin.o/crtend.o are never
// linked in. Those objects are what would otherwise define __dso_handle,
// which the compiler references for every translation unit with a global
// C++ constructor/destructor (passed to __cxa_atexit/__cxa_finalize to
// identify which "module" a destructor belongs to), and which mlibc's own
// static-build teardown code (options/lsb/generic/dso_exit.cpp) also reads
// directly. A unique, non-null pointer value is all that's required, so we
// point it at itself.
extern "C" void *__dso_handle = &__dso_handle;
