n := winsdk
mode := release

ifeq ($(mode), debug)
	cflags += -g
else
	cflags += -Ofast
endif

libpath := $(shell bin/winsdk.exe --type:lib --kit:um --arch:x64)

cflags += -nostdlib
cflags += -ffreestanding
cflags += -fno-stack-check
cflags += -fno-stack-protector
cflags += -mno-stack-arg-probe
cflags += -fuse-ld=lld-link
cflags += -lkernel32
cflags += -lshell32
cflags += -ladvapi32
cflags += -Xlinker /nodefaultlib
cflags += -Xlinker /subsystem:console
cflags += -Xlinker "/libpath:$(libpath)"

bin/$n.exe: src/$n.c bin Makefile
	clang $< $(cflags) -o $@

run: bin/$n.exe
	$<

bin:
	mkdir bin

.PHONY: run
