CC=g++ 
CFLAGSBASE=-Wno-write-strings -std=c++1y 
CPPFLAGS=
CLIBS=-lm -lpthread -lallegro_font -lallegro_image -lallegro_main -lallegro_memfile -lallegro_primitives -lallegro -lallegro_ttf
ALLEGRO=/path/to/allegro5
LD_LIBRARY_PATH=${ALLEGRO}/lib
INC_INCLUDE_PATH=${ALLEGRO}/include

SOURCEFILES = $(wildcard *.cpp)
OBJFILES = $(SOURCEFILES:.cpp=.o)


CFLAGS = $(CFLAGSBASE) -O3
valgrind : CFLAGS = $(CFLAGSBASE) -g
profile : CFLAGS = $(CFLAGSBASE) -pg

PROGNAME=mepm

LOCALEXE=$(PROGNAME)
valgrind : LOCALEXE=valgrind_$(PROGNAME)
profile : LOCALEXE=profile_$(PROGNAME)

all : compile
valgrind : compile
profile : compile

compile: $(OBJFILES)
	$(CC) -L$(LD_LIBRARY_PATH) -I$(INC_INCLUDE_PATH) $(CFLAGS) $(OBJFILES) $(CLIBS) -o $(LOCALEXE)

%.o: %.cpp
	$(CC) -I$(INC_INCLUDE_PATH) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

clean:
	rm -f $(LOCALEXE) $(OBJFILES)
