all: desktopview.c
	gcc -Wall desktopview.c -lxcb -lxcb-shm -o desktopview 
