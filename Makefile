# Compiler and flags
CXX = g++
CC = gcc
CXXFLAGS = -std=c++11 -Wall -I.
CFLAGS = -Wall -I.
LDFLAGS = -L.
LDLIBS = -ljson

# Project files
C_SRCS = json_parser.c
C_OBJS = $(C_SRCS:.c=.o)
TEST_SRC = test/test_json.cpp
TEST_EXEC = test/runner
LIB = libjson.a

# Phony targets are targets that don't represent files.
.PHONY: all test clean

# Default target: 'make' will run this.
all: $(LIB)

# Rule to create the static library.
$(LIB): $(C_OBJS)
	@echo "Creating static library $(LIB)..."
	@ar rcs $@ $^

# Rule to compile C source files into object files.
%.o: %.c
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

# Target to build and run tests.
test: $(LIB)
	@echo "Building test runner..."
	@$(CXX) $(CXXFLAGS) $(TEST_SRC) $(LDFLAGS) $(LDLIBS) -o $(TEST_EXEC)
	@echo "--- Running tests ---"
	@./$(TEST_EXEC)
	@echo "---------------------"

# Target to clean up the project directory.
clean:
	@echo "Cleaning up project..."
	@rm -f $(C_OBJS) $(LIB) $(TEST_EXEC)