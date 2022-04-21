files := arduino_sdl.cpp.o catch_ball_states.cpp.o
CXXFLAGS := -g -Wall -Wextra -Wno-unused-parameter -std=c++20
LDLIBS := -lSDL2

all: program

program: $(files) images
	$(CXX) $(files) -o $@ $(LDLIBS)

%.bmp: %.png
	convert $< $@

%.cpp.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: images clean

images: led_green.bmp led_red.bmp button.bmp pot.bmp

clean:
	rm *.bmp *.o program
