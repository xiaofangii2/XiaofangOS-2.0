CC      = clang
CFLAGS  = -Os -ffunction-sections -fdata-sections -fvisibility=hidden -std=c11 -D_GNU_SOURCE
NDKLIB  = /data/data/com.termux/files/usr/opt/ndk-multilib/aarch64-linux-android/lib
LDFLAGS = -static -L$(NDKLIB) -Wl,--gc-sections -Wl,--strip-all -Wl,--build-id=none
TARGET  = mykernel
SRCS    = src/kernel.c src/shell.c src/vfs.c src/sha256.c src/utils.c
OBJS    = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: all
	./$(TARGET)

.PHONY: all clean run
