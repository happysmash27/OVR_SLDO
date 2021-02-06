all: desktopview.c
	gcc -Wall desktopview.c -lxcb -o desktopview 
