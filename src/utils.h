#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <string.h>


#define length(x) (sizeof(x)/sizeof(*x))

#ifdef _DEBUG
    #include <assert.h>
    #define NOT_IMPLEMENTED 0
#endif

#ifdef _DEBUG
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
    if (fread(buffer, size, count, stream) != (size_t)count) {                  \
        fprintf(stderr, "%s:%d: fread failure! %s. exiting...\n",               \
                __FILE__, __LINE__, feof(stream) ? "EOF reached" : "Error");    \
        exit(666);                                                              \
    }

#define safe_write(buffer, size, count, stream)                                         \
    if (fwrite(buffer, size, count, stream) != (size_t)count) {                         \
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

/* arguments must be lvalues, except for `item` */
#define safe_push(buffer, size, capacity, item)                                 \
    do {                                                                        \
        if ((size) == (capacity)) {                                             \
            capacity = (capacity) ? (capacity) * 2 : 1;                         \
            void *new_ptr = realloc(buffer, sizeof(*(buffer)) * (capacity));    \
            if (!new_ptr) {                                                     \
                new_ptr = malloc(sizeof(*(buffer)) * (capacity));               \
                malloc_check(new_ptr);                                          \
                memcpy(new_ptr, buffer, sizeof(*(buffer)) * (size));            \
            }                                                                   \
            buffer = new_ptr;                                                   \
        }                                                                       \
        buffer[size] = item;                                                    \
        ++(size);                                                               \
    } while (0)

/* arguments must be lvalues, except for `amount` */
#define safe_expand(buffer, size, capacity, amount)                             \
    do {                                                                        \
        capacity = (capacity) + (amount);                                       \
        void *new_ptr = realloc(buffer, sizeof(*(buffer)) * (capacity));        \
        if (!new_ptr) {                                                         \
            new_ptr = malloc(sizeof(*(buffer)) * (capacity));                   \
            malloc_check(new_ptr);                                              \
            memcpy(new_ptr, buffer, sizeof(*(buffer)) * (size));                \
        }                                                                       \
        buffer = new_ptr;                                                       \
    } while (0)


char *readFile(const char *name);

#endif
