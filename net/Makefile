DIR_INC = -I./include -I../third/libuv/include -I../third/spdlog/include
DIR_SRC = ./src
DIR_OBJ = ./obj
DIR_LIB = ./lib

SRC = $(wildcard $(DIR_SRC)/*.cpp)
SRC_WITHOUT_DIR = $(notdir $(SRC))
OBJ = $(patsubst %.cpp, ${DIR_OBJ}/%.o, ${SRC_WITHOUT_DIR})
TARGET = $(DIR_LIB)/libmom.a

CC = g++
CFLAGS = -std=c++14 ${DIR_INC} -o3

all : PREPARE $(TARGET)

PREPARE : 
	echo $(SRC)
	echo $(OBJ)
	echo $(TARGET)
	mkdir -p $(DIR_OBJ) $(DIR_LIB)

$(DIR_OBJ)/%.o:$(DIR_SRC)/%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

${TARGET}:$(OBJ)
	ar rcs ${TARGET} $(OBJ)

.PHONY:clean
clean:
	rm -rf ${DIR_OBJ} $(DIR_LIB)
