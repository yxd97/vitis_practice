CXX = g++

libbeyond.so: beyond.cpp
	$(CXX) -fPIC -c -o beyond.o beyond.cpp
	$(CXX) -shared -fPIC -o libbeyond.so beyond.o

ampersand.o: ampersand.cpp
	$(CXX) -c -o ampersand.o ampersand.cpp

hello_world.o: hello_world.cpp
	$(CXX) -c -o hello_world.o hello_world.cpp

hello_world: hello_world.o ampersand.o
	$(CXX) -o hello_world hello_world.o ampersand.o -ldl

.PHONY: run clean
run: hello_world libbeyond.so
	./hello_world

clean:
	rm -f *.o *.so hello_world
