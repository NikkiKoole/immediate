#include <stdio.h>
#include <stdint.h>  // int8_t et al.
#include <stdbool.h> // bool

#include "SDL.h"
#include "SDL_mixer.h"
#include <GL/glew.h>
#include <SDL_opengl.h>

#define internal static
#define global_value
#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define CLAMP(x, lo, hi) MIN((hi), MAX((lo), (x)))
#define ARRAY_COUNT(array) (sizeof(array) / sizeof(array[0]))
#define UNUSED(x) (void)(x)
#define ABS(x)(((x) < 0) ? -(x) : (x))
#define ASSERT(expression)                                              \
if (!(expression)) {                                                    \
    printf("%s, function %s, file: %s, line:%d. \n", #expression, __FUNCTION__, __FILE__, __LINE__); \
    exit(0);                                                            \
 }

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float r32;
typedef double r64;
typedef bool b32;

global_value SDL_Window *window;
global_value SDL_GLContext context;

internal const char *gl_error_string(GLenum error) {
    switch (error) {
    case GL_INVALID_OPERATION:
        return "INVALID_OPERATION";
        break;
    case GL_INVALID_ENUM:
        return "INVALID_ENUM";
        break;
    case GL_INVALID_VALUE:
        return "INVALID_VALUE";
        break;
    case GL_OUT_OF_MEMORY:
        return "OUT_OF_MEMORY";
        break;
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return "INVALID_FRAMEBUFFER_OPERATION";
        break;
    case GL_NO_ERROR:
        return "NO_ERROR";
        break;
    }
    return "UNKNOWN_ERROR";
}


internal void init(void) {
    int error = (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0);
    if (error < 0) {
        printf("%d %s\n", error, SDL_GetError());
    }
    error = (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048));
    if (error < 0) {
        printf("%d %s\n", error, Mix_GetError());
    }
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetSwapInterval(1);

    window = SDL_CreateWindow("Hello!!!",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              SCREEN_WIDTH, SCREEN_HEIGHT,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    context = SDL_GL_CreateContext(window);
    ASSERT(context != NULL);

    SDL_DisplayMode displayMode;
    SDL_GetDesktopDisplayMode(0, &displayMode);

    glewExperimental = GL_TRUE;
    ASSERT(glewInit() == GLEW_OK);
    glGetError(); // pop the first error glewInit generates.
    error = glGetError();

    if (error != GL_NO_ERROR) {
       printf("Error initializing OpenGL (%s)\n", gl_error_string(error));
       exit(0);
    }
}

const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, 1.0, 1.0);\n"
    "}\0";
const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(0.4f, 0.5f, 0.2f, 1.0f);\n"
    "}\n\0";


internal int make_shader(const char * vertex, const char *fragment) {
    int success;
    char infoLog[512];
    int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertex, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        printf("Error creating vertex shader: %s \n", infoLog);
    }

    int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragment, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        printf("Error creating fragment shader: %s \n", infoLog);
    }

    int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        printf("Error linking shader program: %s \n", infoLog);

    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

int main(void) {
    init();

    int shader = make_shader(vertexShaderSource, fragmentShaderSource);
    
    bool quit = false;
    const u8 *keys = SDL_GetKeyboardState(NULL);

    ////
    float vertices[] = {
        -0.5f, -0.5f, // left  
         0.5f, -0.5f, // right 
         0.0f,  0.5f  // top   
    }; 

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0); 
    ///////
    
    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || keys[SDL_SCANCODE_ESCAPE]) {
                quit = true;
            }
        }
        glClearColor(1.0, 0.5, 1.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        ////////
        glUseProgram(shader);
        glBindVertexArray(VAO); 
        glDrawArrays(GL_TRIANGLES, 0, 3);
        ////////
        
        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}


