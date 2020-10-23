#PLAT     = WINDOWS
#PLAT     = LINUX
CXX      = g++
CC       = gcc
RM       = rm -f

ifndef PLAT
PLAT     = LINUX
endif
ifeq ($(PLAT), LINUX)
LIBS     = -lpthread -lrt -L"./" -lDLMS
BIN      = gather
endif
ifeq ($(PLAT), WINDOWS)
LIBS     = -lws2_32 -lWinmm -L"./" libDLMS.dll
BIN      = gather.exe
endif
INCS     = 
CFLAGS   = $(INCS) -Os -Wall

SRC_DIR := ./
SRC := $(wildcard $(SRC_DIR)/*.cpp)
OBJ := $(SRC:$(SRC_DIR)/%.cpp=$(SRC_DIR)/%.o)


.PHONY: all all-before all-after clean clean-custom

all: all-before $(BIN) all-after

clean: clean-custom
	${RM} $(OBJ) $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(OBJ) -o $(BIN) $(LIBS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo + CXX $<
	@$(CXX) -c -o $@ $< $(CFLAGS)
