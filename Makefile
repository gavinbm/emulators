tests = shell
obj_files = chip8/obj/

all: build

build: objects
	cc -o chip8 $(obj_files) -ggdb

objects: 
	mkdir chip8/obj
	mv *.o chip8/obj

clean-build:
	rm -rf chip8/obj

clean:
	rm -rf obj tiny $(tests)