SDL_CFLAGS := $(shell sdl2-config --cflags)
SDL_LFLAGS := $(shell sdl2-config --libs)

STRICT = -Werror -Wall -Wextra  -Wformat=2 -Wno-import -Wimplicit -Wmain -Wchar-subscripts -Wsequence-point -Wmissing-braces -Wparentheses -Winit-self -Wswitch-enum -Wstrict-aliasing=2 -Wpointer-arith -Wcast-align -Wwrite-strings  -Wold-style-definition -Wmissing-prototypes -Wmissing-declarations  -Wnested-externs -Winline -Wdisabled-optimization -Wno-unused
PROGRAM_NAME := immediate.out

DEBUG:= -g3 -fsanitize=address -fno-omit-frame-pointer
OPTIMIZE:= #-O3
STD:= -std=gnu99
CC:= gcc

BACKEND_FILES:=src/main.c

linux:
	${CC} -I/usr/local/include/  $(SDL_CFLAGS) $(SDL_LFLAGS) $(STRICT) -DLINUX ${STD} -lSDL2_mixer   ${DEBUG} -lGL  -lGLEW  ${BACKEND_FILES} -lm -o $(PROGRAM_NAME)

osx:
	${CC} -I/usr/local/include/  $(SDL_CFLAGS) $(SDL_LFLAGS) $(STRICT) -DLINUX ${STD}  ${DEBUG}  -lglew -framework OpenGL  ${BACKEND_FILES} -lm -o $(PROGRAM_NAME)
