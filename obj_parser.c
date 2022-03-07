#include "utils.h"

#include <stdio.h>

typedef struct
{
    int vertexCount;
    int textureCount;
    int normalCount;
    int faceOffset;
} ObjInfo;


static ObjInfo loadInfo(const char *data)
{
    
}


int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "No file provided!\n");
        exit(1);
    }

    char *data = readFile(argv[1]);

    ObjInfo info = loadInfo(data);

    free(data);
    return 0;
}
