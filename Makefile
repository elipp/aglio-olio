# aglio olio linux makefile

CC=g++ -g -march=corei7 # for -msse, -msse2 etc.
CFLAGS=-c -Wall
LIBS=-lGL -lGLU -lGLEW

SOURCES=common.cpp lin_alg.cpp lodepng.cpp main.cpp model.cpp objloader.cpp shader.cpp texture.cpp vertex.cpp
SRCDIR=src
sources=$(addprefix $(SRCDIR)/, $(SOURCES))

INCLUDE=-Iinclude
OBJS=common.o lin_alg.o lodepng.o model.o objloader.o shader.o texture.o vertex.o
OBJDIR=objs
objects = $(addprefix $(OBJDIR)/, $(OBJS))

EXECUTABLE=jep

all: dir $(EXECUTABLE)

dir:
	mkdir -p $(OBJDIR)

$(EXECUTABLE): $(objects)
	$(CC) $(INCLUDE) $(objects) src/main.cpp -o $(EXECUTABLE)  $(LIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -o $@ $< $(LIBS)

clean:
	rm -rf $(EXECUTABLE) $(OBJDIR)/*.o
