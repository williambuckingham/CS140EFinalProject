all: build 

# install

build:
	make -C timer-int
	make -C mem-checker

clean:
	rm -f *~
	make -C timer-int clean
	make -C mem-checker clean
