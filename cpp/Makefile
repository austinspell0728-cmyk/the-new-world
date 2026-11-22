# Makefile for the C++ / raylib prototype.
#
# raylib is NOT included in this repo. Point RAYLIB_PATH at the folder that
# contains your raylib.h and libraylib.a, either by editing the default below
# or on the command line:
#
#     make RAYLIB_PATH=/path/to/raylib/src
#
# Targets:
#     make         build the game          -> build/new-world(.exe)
#     make run     build and start the game
#     make test    build and run the rule tests (no raylib needed)
#     make clean   delete build output

# --- Settings ------------------------------------------------------------------

RAYLIB_PATH ?= C:/raylib/raylib/src

CXX      := g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
BUILD    := build

# --- Platform-specific link flags ----------------------------------------------

ifeq ($(OS),Windows_NT)
    EXE        := .exe
    RAYLIB_LIBS := -lraylib -lopengl32 -lgdi32 -lwinmm
else
    UNAME := $(shell uname -s)
    ifeq ($(UNAME),Darwin)
        RAYLIB_LIBS := -lraylib -framework OpenGL -framework Cocoa -framework IOKit \
                       -framework CoreVideo -framework CoreAudio
    else
        RAYLIB_LIBS := -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
    endif
endif

# --- Sources -------------------------------------------------------------------

# Game rules and data: plain C++, no raylib.
GAME_SRC     := $(wildcard src/game/*.cpp)
# Platform helpers written without raylib, so the tests can use them too.
# They are compiled WITHOUT the raylib include path, so a raylib include
# slipping into one of them breaks the build straight away.
PURE_PLATFORM_SRC := src/platform/BuildingData.cpp src/platform/BuildingLayout.cpp \
                     src/platform/TreeData.cpp
# Window, input and drawing: uses raylib.
PLATFORM_SRC := $(filter-out $(PURE_PLATFORM_SRC),$(wildcard src/platform/*.cpp)) src/main.cpp
TEST_SRC     := tests/GameTests.cpp

GAME_OBJ          := $(GAME_SRC:%.cpp=$(BUILD)/%.o)
PURE_PLATFORM_OBJ := $(PURE_PLATFORM_SRC:%.cpp=$(BUILD)/%.o)
PLATFORM_OBJ      := $(PLATFORM_SRC:%.cpp=$(BUILD)/%.o)
TEST_OBJ          := $(TEST_SRC:%.cpp=$(BUILD)/%.o)

GAME_BIN := $(BUILD)/new-world$(EXE)
TEST_BIN := $(BUILD)/game-tests$(EXE)

# -MMD -MP writes a .d file next to each object listing the headers it
# includes, so editing a header rebuilds everything that uses it.
DEPFLAGS := -MMD -MP

# --- Rules ---------------------------------------------------------------------

.PHONY: all run test clean

all: $(GAME_BIN)

$(GAME_BIN): $(GAME_OBJ) $(PURE_PLATFORM_OBJ) $(PLATFORM_OBJ)
	$(CXX) $^ -o $@ -L$(RAYLIB_PATH) $(RAYLIB_LIBS)

# The tests link only the game objects and the raylib-free platform
# helpers. If anything in them ever includes raylib, this target stops
# building, which keeps the layers apart.
$(TEST_BIN): $(GAME_OBJ) $(PURE_PLATFORM_OBJ) $(TEST_OBJ)
	$(CXX) $^ -o $@

# Game and test objects are compiled WITHOUT the raylib include path.
# (GNU make picks the most specific pattern, so these win over the
# general $(BUILD)/%.o rule below.)
$(BUILD)/src/game/%.o: src/game/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD)/tests/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

# (A static pattern rule: it applies only to the listed objects.)
$(PURE_PLATFORM_OBJ): $(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

# Platform objects and main.cpp get the raylib headers.
$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -I$(RAYLIB_PATH) -c $< -o $@

run: $(GAME_BIN)
	./$(GAME_BIN)

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -rf $(BUILD)

-include $(GAME_OBJ:.o=.d) $(PURE_PLATFORM_OBJ:.o=.d) $(PLATFORM_OBJ:.o=.d) $(TEST_OBJ:.o=.d)
