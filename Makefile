GCC= g++
CFLAGS += -O3 -Wall
TARGET := main

SRCS := $(shell find . -name '*.cpp' -not -path "*test*")
DEP := $(shell find . -name '*.cpp' -not -path "*test*" -not -name "*main*")
OBJS := $(patsubst %.cpp,%.out,$(SRCS))


TEST := $(shell find . -name '*.cpp' -path "*test*")
TEST_TARGETS := $(patsubst %.cpp,%,$(TEST)) 

all: $(TARGET)
test: $(TEST_TARGETS)

$(TARGET): $(SRCS)
	@echo "Building..."
	$(GCC) -o $@.out $^


$(TEST_TARGETS): $(TEST)
	@echo "Building tests..."
	$(GCC) -o $@.out $@.cpp 

%.out: %.cpp
	$(GCC) $(CFLAGS) -c -o $@ $< 

clean:
	rm -f $(TARGET) *.o $(OBJS) $(TEST_TARGETS)

run: $(TARGET) 
	./$(TARGET).out

run_test: $(TEST_TARGETS)
	for f in $^; do echo $$f; ./$$f.out; done

.PHONY:all clean test run run_test
