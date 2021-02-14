all: desktopview.c
	gcc -Wall desktopview.c -lpthread -lxcb -lxcb-shm -lxcb-errors -o desktopview 
