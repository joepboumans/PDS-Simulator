GCC= g++
CFLAGS += -O3  -Wall
TARGET= main

all: $(TARGET)

$(TARGET): $(TARGET).cpp
	$(GCC) $(CFLAGS) -o $(TARGET).out $(TARGET).cpp

clean:
	rm -f $(TARGET)
