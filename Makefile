GCC= g++
CFLAGS += -O3 -c -Wall
TARGET= test

all: $(TARGET)

$(TARGET): $(TARGET).cpp
	$(GCC) $(CFLAGS) -o $(TARGET).out $(TARGET).cpp

clean:
	rm -f $(TARGET)
