SRC_DIR := ./
SRC := $(wildcard $(SRC_DIR)/*.cpp)
OBJ := $(SRC:$(SRC_DIR)/%.cpp=$(SRC_DIR)/%.o)

CXX      = g++
CC       = gcc
RM       = rm -f

LIBS     = -lpthread -lrt -lDLMS
INCS     = 
BIN      = gather
CFLAGS   = $(INCS) -Os -Wall


.PHONY: all all-before all-after clean clean-custom

all: all-before $(BIN) all-after

clean: clean-custom
	${RM} $(OBJ) $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(OBJ) -o $(BIN) $(LIBS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo + CXX $<
	@$(CXX) -c -o $@ $< $(CFLAGS)
