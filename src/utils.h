#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>


#define length(x) (sizeof(x)/sizeof(*x))

#ifdef DEBUG
    #include <assert.h>
    #define NOT_IMPLEMENTED 0
#endif

#ifdef DEBUG
    #define unreachable()                                                           \
        fprintf(stderr, "%s:%d: Unreachable line reached.\n", __FILE__, __LINE__);  \
        exit(666);
#else
#ifdef _WIN32
    #define unreachable()   assume(0);
#else
    #define unreachable()   __builtin_unreachable();
#endif
#endif

#define safe_read(buffer, size, count, stream)                                  \
    if (fread(buffer, size, count, stream) != count) {                          \
        fprintf(stderr, "%s:%d: fread failure! %s. exiting...\n",               \
                __FILE__, __LINE__, feof(stream) ? "EOF reached" : "Error");    \
        exit(666);                                                              \
    }

#define safe_write(buffer, size, count, stream)                                         \
    if (fwrite(buffer, size, count, stream) != count) {                                 \
        fprintf(stderr, "%s:%d: fwrite failure! exiting...\n", __FILE__, __LINE__);     \
        exit(666);                                                                      \
    }

#define malloc_check(ptr)                                                               \
    if (!(ptr)) {                                                                       \
        fprintf(stderr, "%s:%d: malloc failure! exiting...\n", __FILE__, __LINE__);     \
        exit(666);                                                                      \
    }

#define file_check(file, path)                                                                  \
    if (!(file)) {                                                                              \
        fprintf(stderr, "%s:%d: fopen failure! path: \"%s\".\n", __FILE__, __LINE__, path);     \
        exit(666);                                                                              \
    }

#define eof_check(file)                                                                 \
    if (fgetc(file) != EOF) {                                                           \
        fprintf(stderr, "%s:%d: expected EOF! exiting...\n", __FILE__, __LINE__);       \
        exit(666);                                                                      \
    }


char *readFile(const char *name);


#endif
