#include "bag_engine.h"
#include "bag_keys.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static int running = 1;
int windowWidth, windowHeight;


void GLAPIENTRY openglCallback(
        GLenum source,
        GLenum type,
        GLuint id,
        GLenum severity,
        GLsizei length,
        const GLchar* message,
        const void* userParam
) {
    int error = type == GL_DEBUG_TYPE_ERROR;

    printf("[%s] type: %d, severity: %d\n%s\n",
            error ? "\033[1;31mERROR\033[0m" : "\033[1mINFO\033[0m",
            type, severity, message
    );
}


/* WOAH */
char *readFile(const char *name)
{
    FILE *file = NULL;
    char *contents = NULL;

    file = fopen(name, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file \"%s\"!\n", name);
        fprintf(stderr, "File: %s, Line: %d\n", __FILE__, __LINE__);
        goto error_exit;
    }

    if (fseek(file, 0, SEEK_END)) {
        fprintf(stderr, "Failed to seek in file \"%s\"!\n", name);
        fprintf(stderr, "File: %s, Line: %d\n", __FILE__, __LINE__);
        goto error_exit;
    }

    long size = ftell(file);
    if (size == -1L) {
        fprintf(stderr, "Failed to tell in file \"%s\"!\n", name);
        fprintf(stderr, "File: %s, Line: %d\n", __FILE__, __LINE__);
        goto error_exit;
    }

    rewind(file);

    contents = malloc(size + 1); /* null byte */
    if (!contents) {
        fprintf(stderr, "Failed to allocate memory!\n");
        fprintf(stderr, "File: %s, Line: %d\n", __FILE__, __LINE__);
        goto error_exit;
    }

    if (fread(contents, 1, size, file) != (size_t)size) {
        fprintf(stderr, "Failed to read file \"%s\"!\n", name);
        fprintf(stderr, "File: %s, Line: %d\n", __FILE__, __LINE__);
        goto error_exit;
    }

    if (fclose(file)) {
        fprintf(stderr, "Failed to close file \"%s\"!\n", name);
    }

    contents[size] = 0;
    return contents;

error_exit:
    if (file) {
        fclose(file);
    }
    free(contents);
    return NULL;
}


int loadShader(const char *path, GLenum type)
{
    char *source = readFile(path);
    if (!source)
        exit(1);

    int shader = glCreateShader(type);
    glShaderSource(shader, 1, (const char * const*)&source, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        fprintf(stderr, "[\033[1;31mERROR\033[0m] Failed to compile shader '%s'!\n"
                        "%s", path, infoLog);
        exit(-1);
    }

    free(source);
    return shader;
}


void printContextInfo()
{

    printf("Adaptive vsync: %d\n", bagE_isAdaptiveVsyncAvailable());

    const char *vendorString = (const char*)glGetString(GL_VENDOR);
    const char *rendererString = (const char*)glGetString(GL_RENDERER);
    const char *versionString = (const char*)glGetString(GL_VERSION);
    const char *shadingLanguageVersionString = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    printf("Vendor: %s\nRenderer: %s\nVersion: %s\nShading Language version: %s\n",
        vendorString, rendererString, versionString, shadingLanguageVersionString);
}


int bagE_main(int argc, char *argv[])
{
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(openglCallback, 0);

    printContextInfo();

    bagE_setWindowTitle("BRUHAPS");

    int vertexShader = loadShader("shaders/vertex.glsl", GL_VERTEX_SHADER);
    int fragmentShader = loadShader("shaders/fragment.glsl", GL_FRAGMENT_SHADER);
    int program = glCreateProgram();


    bagE_getWindowSize(&windowWidth, &windowHeight);

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    // FIXME check if linking went well
    glLinkProgram(program);

    glDetachShader(program, vertexShader);
    glDeleteShader(vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);

    glProgramUniform2i(program, 1, 100, 100);
    glProgramUniform2i(program, 2, 500, 500);
    glProgramUniform4f(program, 3, 1.0f, 0.0f, 0.0f, 1.0f);

    unsigned vao;
    glGenVertexArrays(1, &vao);

    glBindVertexArray(vao);
    glUseProgram(program);

    while (running) {
        bagE_pollEvents();

        if (!running)
            break;

        glClearColor(
		0.0f,
		0.0f,
		0.0f,
                1.0f
        );
        glClear(GL_COLOR_BUFFER_BIT);

        glProgramUniform2i(program, 0, windowWidth, windowHeight);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        bagE_swapBuffers();
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);

    return 0;
}


int bagE_eventHandler(bagE_Event *event)
{
    switch (event->type)
    {
        case bagE_EventWindowClose:
            running = 0;
            return 1;

        case bagE_EventWindowResize: {
            bagE_WindowResize *wr = &(event->data.windowResize);
            windowWidth = wr->width;
            windowHeight = wr->height;
            glViewport(0, 0, wr->width, wr->height);
        } break;

        default: break;
    }

    return 0;
}
