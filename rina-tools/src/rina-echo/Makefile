SRCFILES=$(wildcard src/*.cpp)
OBJSRCFILES=$(SRCFILES:.cpp=.o)
OBJFILES=$(subst src,build,$(OBJSRCFILES))

all: mkdirs rinaechotime
mkdirs:
	mkdir -p build;
build/%.o: src/%.cpp
	gcc -c -O2 -I/usr/local/irati/include -std=c++11  src/$*.cpp -o build/$*.o
rinaechotime: $(OBJFILES)
	gcc -L/usr/local/irati/lib -lstdc++ -lrina $(OBJFILES) -o rinaechotime
