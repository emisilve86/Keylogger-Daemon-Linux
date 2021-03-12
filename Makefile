event=

all: k3yl0gg3r

k3yl0gg3r : keylogger.o
	gcc -o k3yl0gg3r keylogger.o
	mkdir -p bin/
	mv keylogger.o bin/
	mv k3yl0gg3r bin/

keylogger.o : src/keylogger.c src/keylogger.h
	gcc -DEVENT_NUMBER=\"$(event)\" -c src/keylogger.c

.PHONY: clean
clean :
	rm -rf bin/