CC = g++
FLAGS = -Wall -Wextra -pedantic
LIBS_FLAGS = -lpthread
OUTPUT = server.out

.PHONY: clean

server: server.cpp
	$(CC) $(FLAGS) -o $(OUTPUT) server.cpp $(LIBS_FLAGS)

clean:
	rm -f $(OUTPUT)
