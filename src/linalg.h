#ifndef MATH_H
#define MATH_H

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    float data[16];
} Matrix;


static inline Matrix matrixIdentity(void)
{
    Matrix res = {{
         1.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 1.0f, 0.0f,
         0.0f, 0.0f, 0.0f, 1.0f,
    }};
    return res;
}


static inline Matrix matrixScale(float x, float y, float z)
{
    Matrix res = {{
         x,    0.0f, 0.0f, 0.0f,
         0.0f, y,    0.0f, 0.0f,
         0.0f, 0.0f, z,    0.0f,
         0.0f, 0.0f, 0.0f, 1.0f,
    }};
    return res;
}


static inline Matrix matrixTranslation(float x, float y, float z)
{
    Matrix res = {{
         1.0f, 0.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 1.0f, 0.0f,
         x,    y,    z,    1.0f,
    }};
    return res;
}


static inline Matrix matrixRotationX(float angle)
{
    Matrix res = {{
         1.0f, 0.0f,        0.0f,        0.0f,
         0.0f, cosf(angle), sinf(angle), 0.0f,
         0.0f,-sinf(angle), cosf(angle), 0.0f,
         0.0f, 0.0f,        0.0f,        1.0f,
    }};
    return res;
}


static inline Matrix matrixRotationY(float angle)
{
    Matrix res = {{
         cosf(angle), 0.0f,-sinf(angle), 0.0f,
         0.0f,        1.0f, 0.0f,        0.0f,
         sinf(angle), 0.0f, cosf(angle), 0.0f,
         0.0f,        0.0f, 0.0f,        1.0f,
    }};
    return res;
}


static inline Matrix matrixRotationZ(float angle)
{
    Matrix res = {{
         cosf(angle), sinf(angle), 0.0f, 0.0f,
        -sinf(angle), cosf(angle), 0.0f, 0.0f,
         0.0f,        0.0f,        1.0f, 0.0f,
         0.0f,        0.0f,        0.0f, 1.0f,
    }};
    return res;
}


#if 1
static inline Matrix matrixProjection(
        float fov,
        float width,
        float height,
        float np,
        float fp
) {
    float ar = height / width;
    float xs = (float)((1.0 / tan((fov / 2.0) * M_PI / 180)) * ar);
    float ys = xs / ar;
    float flen = fp - np;

    Matrix res = {{
         xs,   0.0f, 0.0f,                   0.0f,
         0.0f, ys,   0.0f,                   0.0f,
         0.0f, 0.0f,-((fp + np) / flen),    -1.0f,
         0.0f, 0.0f,-((2 * np * fp) / flen), 0.0f, // NOTE if 1.0f removes far plane clipping ???
    }};

    return res;
}
#else
static inline Matrix matrixProjection(
        float fov,
        float width,
        float height,
        float np,
        float fp
) {
    float top = np * tanf((M_PI * fov) / 360.0f);
    float bottom = -top;
    float right = top * (width / height);
    float left = -right;

    Matrix res = {{
         (2 * np) / (right - left), 0.0f, 0.0f, 0.0f,
         0.0f, (2 * np) / (top - bottom), 0.0f, 0.0f,
         (right + left) / (right - left), (top + bottom) / (top - bottom),-((fp + np) / (fp - np)),-1.0f,
         0.0f, 0.0f, -((2 * fp * np)/(fp - np)), 0.0f,
    }};

    return res;
}
#endif

static inline Matrix matrixMultiply(const Matrix *mat1, const Matrix *mat2)
{
    const float *m1 = mat1->data;
    const float *m2 = mat2->data;

    Matrix res;

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            float value = 0;

            for (int k = 0; k < 4; k++)
                value += m2[j * 4 + k] * m1[k * 4 + i];

            res.data[j * 4 + i] = value;
        }
    }

    return res;
}



static inline void printMatrix(const Matrix *mat)
{
    printf("\n");
    for (int y = 0; y < 4; ++y) {
        printf("[ ");
        for (int x = 0; x < 4; ++x) {
            printf("%f ", mat->data[y * 4 + x]);
        }
        printf("]\n");
    }
}



#endif
