#CPPFLAGS=-I"C:/dx7sdk/include" -I"/mingw/include" 
CPPFLAGS=$(shell sdl-config --cflags)

#CFLAGS=-O3
#CFLAGS=-g
CXXFLAGS=$(CFLAGS) -g

#LDFLAGS=-L"C:/dx7sdk/lib" -L"/mingw/lib"
#LOADLIBES=-mwindows
#LDLIBS=-ldxguid -ldinput -lddraw -lwinmm -lSDL -lSDL_mixer
#LDFLAGS=-L"C:/dx7sdk/lib"

LDLIBS=-ldxguid -ldinput -lddraw -lwinmm $(shell sdl-config --libs) -lSDL_mixer

COMMON_OBJS=bgm.o ddutil.o dinkvar.o fastfile.o string_util.o sfx.o	\
	gfx.o gfx_tiles.o gfx_utils.o freedink.res
APPS=freedink freedinkedit
BINARIES=$(APPS:=.exe)

# TODO, add a resource file for both make and vc++.
# Dev-C++ can show how to use them (Dink_private.res).

all: $(APPS)

freedink: $(COMMON_OBJS) freedink.o update_frame.o init.o
freedinkedit: $(COMMON_OBJS) freedinkedit.o

# .h deps
freedink.o:  		freedink.h dinkvar.h update_frame.h bgm.h sfx.h gfx.h gfx_tiles.h gfx_utils.h resource.h
freedinkedit.o:		dinkvar.h bgm.h sfx.h gfx.h gfx_tiles.h resource.h
dinkvar.o:		dinkvar.h bgm.h sfx.h ddutil.h fastfile.h freedink.h gfx.h gfx_tiles.h bgm.h
update_frame.o:		update_frame.h dinkvar.h freedink.h gfx_tiles.h gfx.h bgm.h
ddutil.o:		ddutil.h
bgm.o:			bgm.h dinkvar.h
sfx.o:			sfx.h dinkvar.h
gfx.o:			gfx.h
gfx_tiles.o:		gfx_tiles.h gfx.h dinkvar.h
gfx_utils.o:		gfx_utils.h
init.o:			init.h gfx.h dinkvar.h

# TODO: Dev-C++'s Dink_private.h add meta info about a binary. Reproduce that.

# Add the icon resource
%.res: %.rc
	windres.exe -i $< --input-format=rc -o $@ -O coff 

clean:
	-rm -f $(COMMON_OBJS) freedink.o freedinkedit.o $(BINARIES)
	-rm -f *~

# Those targets are not files
.PHONY: all clean
