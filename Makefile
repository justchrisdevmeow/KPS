CC = gcc
CFLAGS = -Wall -O2 -Iinclude
LIBS = -lgdi32 -lcomctl32 -ladvapi32 -luser32 -lshell32 -lshlwapi
RES = src/resource.o

OBJS = src/main.o src/utils.o src/system.o src/file.o $(RES)

all: KittyPrivSystem.exe

KittyPrivSystem.exe: $(OBJS) kittypriv.manifest
	$(CC) -o $@ $(OBJS) $(LIBS)
	mt.exe -manifest kittypriv.manifest -outputresource:$@;1

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

src/resource.o: src/resource.rc
	windres src/resource.rc -o src/resource.o

clean:
	rm -f src/*.o KittyPrivSystem.exe
