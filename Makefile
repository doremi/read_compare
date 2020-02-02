TARGET = read_compare
CFLAGS = -Wall

all:$(TARGET)

read_compare:read_compare.c
	gcc $(CFLAGS) -o $@ $<

clean:
	rm -rf $(TARGET)
