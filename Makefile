all: desktopview.c
	gcc -Wall -Wno-uninitialized desktopview.c -lxcb -o desktopview 
