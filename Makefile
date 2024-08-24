keyboard_backlight:
	gcc -o keyboard_backlight keyboard_backlight.c -levdev

install: keyboard_backlight
	cp keyboard_backlight /usr/local/bin
