all: desktopview.c
	gcc -Wall desktopview.c -lxcb -lxcb-shm -lxcb-errors -o desktopview 
