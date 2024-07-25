GCC= g++
CFLAGS += -O3 -Wall
TARGET := main

SRCS := $(shell find . -name '*.cpp')
OBJS := $(patsubst %.cpp,%.out,$(SRCS))

all: $(TARGET)

$(TARGET): $(SRCS)
	@echo "Building..."
	$(GCC) -o $@.out $^

%.out: %.cpp
	$(GCC) $(CFLAGS) -c -o $@ $< 

clean:
	rm -f $(TARGET) *.o $(OBJS)

.PHONY:all clean
