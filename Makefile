all: prod

debug:
	  gcc -g -Wall -o ruby_bang ruby_bang.c
prod:
	  gcc -Wall -O3 -o ruby_bang ruby_bang.c
clean:
	  rm -f ruby_bang

install: all
	  /usr/bin/install ruby_bang /bin/ruby_bang

