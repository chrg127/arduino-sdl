files := arduino_sdl.cpp.o arduino_string.cpp.o LiquidCrystal_I2C.cpp.o
CXXFLAGS := -g -I. -Wall -Wextra -Wno-unused-parameter -std=c++20
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
