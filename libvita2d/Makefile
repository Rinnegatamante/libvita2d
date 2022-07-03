TARGET_LIB = libvita2d_vgl.a
OBJS       = source/vita2d.o source/int_htab.o source/vita2d_pgf.o source/vita2d_pvf.o \
	source/vita2d_font.o source/texture_atlas.o source/bin_packing_2d.o source/utils.o
INCLUDES   = include

PREFIX  ?= ${VITASDK}/arm-vita-eabi
CC      = arm-vita-eabi-gcc
AR      = arm-vita-eabi-ar
CFLAGS  = -Wl,-q -Wall -O3 -I$(INCLUDES) -I$(VITASDK)/arm-vita-eabi/include/freetype2
ASFLAGS = $(CFLAGS)

all: $(TARGET_LIB)

debug: CFLAGS += -DDEBUG_BUILD
debug: all

$(TARGET_LIB): $(OBJS)
	$(AR) -rc $@ $^

clean:
	rm -rf $(TARGET_LIB) $(OBJS)

install: $(TARGET_LIB)
	@mkdir -p $(DESTDIR)$(PREFIX)/lib/
	cp $(TARGET_LIB) $(DESTDIR)$(PREFIX)/lib/
	@mkdir -p $(DESTDIR)$(PREFIX)/include/
	cp include/vita2d_vgl.h $(DESTDIR)$(PREFIX)/include/
