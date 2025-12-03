main: main.c rglfw.o
	gcc -I./raylib-glfw/glfw/include -o main main.c rglfw.o -lvulkan -lm

rglfw.o:
	gcc -I./raylib-glfw/glfw/include -c raylib-glfw/rglfw.c

clean:
	rm main rglfw.o
