CXXFLAGS := -g -Isrc -Wall -Wextra -Wno-unused-parameter -std=c++20 \
			$(shell pkg-config --cflags sdl2 SDL2_ttf fmt) \
			-DARDUINO=100
LDLIBS   := $(shell pkg-config --libs sdl2 SDL2_ttf fmt)
VPATH    := src:examples
outdir 	 := out
_files   := arduino_sdl.cpp arduino_string.cpp LiquidCrystal_I2C.cpp lcd.cpp
files 	 := $(patsubst %,$(outdir)/%.o,$(_files))
_images  := button pot lcd1 font
images   := $(patsubst %,$(outdir)/%.bmp,$(_images))

all: $(outdir)/program

$(outdir)/program: $(outdir) $(files) $(images)
	$(CXX) $(files) -o $@ $(LDLIBS)

$(outdir)/%.bmp: %.png
	convert $< $@

$(outdir)/%.cpp.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(outdir):
	mkdir -p $@

.PHONY: clean

clean:
	rm -r $(outdir)
