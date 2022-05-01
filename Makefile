# this variable controls which sketch to test.
sketch := caffe.cpp

outdir 	:= out
_files  := arduino_sdl.cpp $(sketch)
_images := button pot lcd1 font
files 	:= $(patsubst %,$(outdir)/%.o,$(_files))
images  := $(patsubst %,$(outdir)/%.bmp,$(_images))

CXXFLAGS := -g -Isrc -Wall -Wextra -Wno-unused-parameter -std=c++20 \
			$(shell pkg-config --cflags sdl2 fmt)
LDLIBS	:= $(shell  pkg-config --libs   sdl2 fmt)
VPATH   := src:examples
flags_deps = -MMD -MP -MF $(@:.o=.d)

all: $(outdir)/program

-include $(outdir)/*.d

$(outdir)/program: $(outdir) $(files) $(images)
	$(CXX) $(files) -o $@ $(LDLIBS)

$(outdir)/%.bmp: %.png
	convert $< $@

$(outdir)/%.cpp.o: %.cpp
	$(CXX) $(CXXFLAGS) $(flags_deps) -c $< -o $@ 

$(outdir):
	mkdir -p $@

.PHONY: clean

clean:
	rm -r $(outdir)
