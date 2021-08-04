n := winsdk
mode := release
linker := msvc

ifeq ($(mode), debug)
	cflags += -g
else
	cflags += -Oz
endif

libpath := $(shell bin/winsdk.exe --type:lib --kit:um --arch:x64)

cflags += -nostdlib
cflags += -ffreestanding
cflags += -fno-stack-check
cflags += -fno-stack-protector
cflags += -mno-stack-arg-probe
ifeq ($(linker), llvm)
	cflags += -fuse-ld=lld-link
endif
cflags += -lkernel32
cflags += -lshell32
cflags += -ladvapi32
ifneq ($(linker), llvm)
	cflags += -Xlinker /align:16
endif
cflags += -Xlinker /entry:start
cflags += -Xlinker /nodefaultlib
cflags += -Xlinker /subsystem:console
cflags += -Xlinker /libpath:"$(libpath)"

bin/$n.exe: src/$n.c bin Makefile
	clang $< $(cflags) -o $@
ifeq ($(mode), release)
	llvm-strip $@
endif

run: bin/$n.exe
	$<

bin:
	mkdir bin

.PHONY: run
