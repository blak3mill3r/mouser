LDFLAGS = -lXtst -lX11 -lboost_thread -lboost_system -lpthread -lboost_fiber -lboost_context

mouser: src/*.cpp
	g++ $(LDFLAGS) -o ./mouser -Wno-deprecated-declarations src/*.cpp

all: mouser
