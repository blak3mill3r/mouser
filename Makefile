LDFLAGS = -lXtst -lX11 -lboost_thread -lboost_system -lpthread -lboost_fiber -lboost_context

mouser: src/*.cpp
	clang++ -std=c++17 $(LDFLAGS) -o ./mouser src/*.cpp

all: mouser

clean:
	rm -f ./mouser
