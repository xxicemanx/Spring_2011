FLAGS=`pkg-config opencv --cflags --libs` -O2

main: main.cc readsettings.h Makefile
	g++ -o main main.cc ${FLAGS}

clean:
	rm main
