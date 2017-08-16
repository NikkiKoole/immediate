
#include <stdio.h>
#include <stdint.h>  // int8_t et al.
#include <stdbool.h> // bool

#include "SDL.h"
#include <GL/glew.h>
#include <SDL_opengl.h>

#include "my_math.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wold-style-definition"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb_image.h"
#pragma GCC diagnostic pop




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
#define CHECK()                                                         \
    {                                                                   \
        int error = glGetError();                                       \
        if (error != 0) {                                               \
            printf("%d, function %s, file: %s, line:%d. \n", error, __FUNCTION__, __FILE__, __LINE__); \
            exit(0);                                                    \
        }                                                               \
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
global_value float vertices[1024];
global_value int vertex_count = 0;
global_value Matrix4 mvp;

typedef struct {
    s32 width;
    s32 height;
    s32 bpp;
    u8* data;
} Image;

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
    /* error = (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048)); */
    /* if (error < 0) { */
    /*     printf("%d %s\n", error, Mix_GetError()); */
    /* } */

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
    "layout (location = 1) in vec3 color;\n"
    "uniform mat4 MVP;\n"
    "out vec3 out_color;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = MVP * vec4(aPos.x, aPos.y, 1.0, 1.0);\n"
    "   out_color = color;"
    "}\0";
const char *fragmentShaderSource = "#version 330 core\n"
    "in vec3 out_color;\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(out_color.r, out_color.g, out_color.b, 1.0f);\n"
    "}\n\0";

const char *vertexShaderSourceUV = "#version 330 core\n"
    "layout (location = 0) in vec2 xy;\n"
    "layout (location = 1) in vec2 uv;\n"
    "uniform mat4 MVP;\n"
    "out vec2 out_uv;\n"
    "void main()\n"
    "{\n"
    "	gl_Position =  MVP * vec4(xy, 1.0f, 1.0f);\n"
    "    out_uv = vec2(uv.x, uv.y);\n"
    "}\n\0";

const char *fragmentShaderSourceUV = "#version 330 core\n"
    "in vec2 out_uv;\n"
    "out vec4 color;\n"
    "uniform sampler2D sprite_atlas;\n"
    "void main()\n"
    "{\n"
    "	color = texture(sprite_atlas, out_uv);\n"
    "    if (color.a == 0.0) {\n"
    "        discard;\n"
    "    }\n"
    "}\n";

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

internal void addVertexXYRGB(float x, float y, float r, float g, float b) {
    int i = vertex_count;
    vertices[i    ] = x;
    vertices[i + 1] = SCREEN_HEIGHT - y;
    vertices[i + 2] = r;
    vertices[i + 3] = g;
    vertices[i + 4] = b;
    vertex_count += 5;
}

internal void addVertexXYUV(float x, float y, float u, float v) {
    int i = vertex_count;
    vertices[i    ] = x;
    vertices[i + 1] = SCREEN_HEIGHT - y;
    vertices[i + 2] = u;
    vertices[i + 3] = v;
    vertex_count += 4;
}


typedef struct {
    u32 VAO;
    u32 VBO;
} GLBuffers;

internal void begin_draw(GLBuffers * buf) {
    glGenVertexArrays(1, &buf->VAO);
    glGenBuffers(1, &buf->VBO);
    glBindVertexArray(buf->VAO);
}

internal void end_draw(GLBuffers * buf) {
    glBindBuffer(GL_ARRAY_BUFFER, buf->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    //position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2* sizeof(float)));
    glEnableVertexAttribArray(1);
    ///////
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}


internal void draw_triangle(float x1, float y1, float x2, float y2, float x3, float y3, float r, float g, float b) {
    addVertexXYRGB(x1, y1, r,g,b);
    addVertexXYRGB(x2, y2, r,g,b);
    addVertexXYRGB(x3, y3, r,g,b);
}

internal void draw_rectangle(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, float r, float g, float b) {
    addVertexXYRGB(x1, y1, r,g,b);
    addVertexXYRGB(x2, y2, r,g,b);
    addVertexXYRGB(x3, y3, r,g,b);

    addVertexXYRGB(x3, y3, r,g,b);
    addVertexXYRGB(x4, y4, r,g,b);
    addVertexXYRGB(x1, y1, r,g,b);
}

internal void draw_image(Image *img, float x, float y, float width, float height) {
    UNUSED(img);UNUSED(x);UNUSED(y);UNUSED(width);UNUSED(height);

    addVertexXYUV(1024, 0,   1.0f, 1.0f);
    addVertexXYUV(1024, 768, 1.0f, 1.0f);
    addVertexXYUV(0,    768, 1.0f, 1.0f);
    addVertexXYUV(0,    0,   1.0f, 1.0f);
}

internal void initMVP(void) {
    int screen_offset_x = 0;
    int screen_offset_y = 0;
    float identity[16] = { 1.0f, 0.0f, 0.0f, 0.0f,
                           0.0f, 1.0f, 0.0f, 0.0f,
                           0.0f, 0.0f, 1.0f, 0.0f,
                           0.0f, 0.0f, 0.0f, 1.0f};

    Matrix4 model = Matrix4MakeWithArray(identity);
    Matrix4 projection = Matrix4MakeOrtho(0.0f, 1.0f * (float)SCREEN_WIDTH,
                                          0.0f, 1.0f * (float)SCREEN_HEIGHT,
                                          -1.0f  * (float)SCREEN_WIDTH, 1.0f  * (float)SCREEN_HEIGHT);
    Matrix4 view = Matrix4MakeLookAt(0.0f, 0.0f, 200.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    mvp = Matrix4Multiply(model, Matrix4Multiply(projection, view));
    mvp = Matrix4Translate(mvp,  screen_offset_x,  screen_offset_y, 0 );
}



internal Image create_image(const char * path) {
    Image result;
    result.data = stbi_load(path, &result.width, &result.height, &result.bpp, 3 );
    return result;
}



int main(void) {
    init();
    initMVP();

    int shader     = make_shader(vertexShaderSource, fragmentShaderSource);
    int shaderUV   = make_shader(vertexShaderSourceUV, fragmentShaderSourceUV);
    bool quit      = false;
    const u8 *keys = SDL_GetKeyboardState(NULL);
    Image img      = create_image("resources/sprite.png");

    GLBuffers buf;
    begin_draw(&buf);


    draw_image(&img, 0, 0, 100, 100);

          glBindBuffer(GL_ARRAY_BUFFER, buf.VBO);
          glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
          //position
          glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
          glEnableVertexAttribArray(0);
          // color attribute
          glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2* sizeof(float)));
          glEnableVertexAttribArray(1);
          ///////
          glBindBuffer(GL_ARRAY_BUFFER, 0);
          glBindVertexArray(0);
    /* end_draw(&buf); */




    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || keys[SDL_SCANCODE_ESCAPE]) {
                quit = true;
            }
        }
        glUseProgram(shaderUV);
        GLint MatrixID = glGetUniformLocation(shaderUV, "MVP");
        ASSERT(MatrixID >= 0);
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &mvp.m[0]);
        CHECK();
        u32 texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img.width, img.height, 0, GL_RGB, GL_UNSIGNED_BYTE, img.data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        CHECK();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(glGetUniformLocation(shaderUV, "sprite_atlas"), 0);
        CHECK();

        glClearColor(1.0, 0.5, 1.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        CHECK();


        //glUseProgram(shaderUV);
        glBindVertexArray(buf.VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertex_count/4);
        CHECK();


        SDL_GL_SwapWindow(window);
        }

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
