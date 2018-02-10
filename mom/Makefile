DIR_INC = -I../net/include -I../third/libuv/include -I../third/spdlog/include
DIR_SRC = .
DIR_OBJ = ./obj
DIR_BIN = ./bin
DIR_LIB = -L../net/lib -L../third/libuv/libs

SRC = $(wildcard ${DIR_SRC}/*.cpp)
SRC_WITHOUT_DIR = $(notdir ${SRC})
OBJ = $(patsubst %.cpp, ${DIR_OBJ}/%.o, $(SRC_WITHOUT_DIR))

TARGET = $(DIR_BIN)/mom

CC = g++
CFLAGS = -std=c++14 ${DIR_INC} -o3

all : PREPARE $(TARGET)

PREPARE : 
	mkdir -p $(DIR_OBJ) $(DIR_BIN)

${DIR_OBJ}/%.o:${DIR_SRC}/%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

${TARGET}:$(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET) $(DIR_LIB) -lmom -luv -lpthread

.PHONY:clean
clean:
	rm -rf ${DIR_OBJ} $(DIR_BIN)
