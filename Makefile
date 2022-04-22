CXXFLAGS := -g -Isrc -Wall -Wextra -Wno-unused-parameter -std=c++20
LDLIBS   := -lSDL2
VPATH    := src:examples
outdir 	 := out
_files   := arduino_sdl.cpp arduino_string.cpp LiquidCrystal_I2C.cpp catchball.cpp
files 	 := $(patsubst %,$(outdir)/%.o,$(_files))

all: $(outdir)/program

$(outdir)/program: $(files) images
	$(CXX) $(files) -o $@ $(LDLIBS)

$(outdir)/%.bmp: %.png
	convert $< $@

$(outdir)/%.cpp.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: images clean

images: $(outdir)/led_green.bmp $(outdir)/led_red.bmp $(outdir)/button.bmp $(outdir)/pot.bmp

clean:
	rm *.bmp *.o program
