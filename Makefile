CROSS_COMPILE ?=

TARGET_BASE = libvdpau_sunxi.so
TARGET = $(TARGET_BASE).1
SRC = device.c presentation_queue.c surface_output.c surface_video.c \
	surface_bitmap.c video_mixer.c decoder.c \
	h264.c mpeg12.c mpeg4.c mp4_vld.c mp4_tables.c mp4_block.c msmpeg4.c h265.c 

USE_VP8 = 0

ifeq ($(USE_VP8),1)
SRC += vp8_decoder.c vp8.c
endif

CEDARV_TARGET_BASE = libcedar_access.so
CEDARV_TARGET = $(CEDARV_TARGET_BASE).1
CEDARV_SRC = ve.c veisp.c handles.c

DISPLAY_TARGET_BASE = libcedarDisplay.so
DISPLAY_TARGET = $(DISPLAY_TARGET_BASE).1
DISPLAY_SRC = cedar_display.c

NV_TARGET_BASE = libvdpau_nv_sunxi.so
NV_TARGET = $(NV_TARGET_BASE).1
NV_SRC = opengl_nv.c

VE_H_INCLUDE = ve.h
LIBCEDARDISPLAY_H_INCLUDE = libcedarDisplay.h

CFLAGS ?= -Wall -O0 -g 
LDFLAGS ?= -I.
LIBS = -lrt -lm -lpthread
LIBS_EGL = -lEGL
LIBS_GLES2 = -lGLESv2
LIBS_VDPAU_SUNXI = -L $(PWD) libvdpau_sunxi.so.1
LIBS_CEDARV = -L $(PWD) -lcedar_access
LIBS_DISPLAY = -L $(PWD) -lcedar_access
CC = gcc

PCFILE := $(shell mktemp -u)

USE_UMP = 1

ifeq ($(USE_UMP),1)
LIBS  += -lUMP
CFLAGS += -DUSE_UMP=1
endif

MAKEFLAGS += -rR --no-print-directory

DEP_CFLAGS = -MD -MP -MQ $@
LIB_CFLAGS = -fpic
LIB_LDFLAGS = -shared -Wl,-soname,$(TARGET)
LIB_LDFLAGS_NV = -shared -Wl,-soname,$(NV_TARGET)
LIB_LDFLAGS_CEDARV = -shared -Wl,-soname,$(CEDARV_TARGET)
LIB_LDFLAGS_DISPLAY = -shared -Wl,-soname,$(DISPLAY_TARGET)

OBJ = $(addsuffix .o,$(basename $(SRC)))
DEP = $(addsuffix .d,$(basename $(SRC)))

CEDARV_OBJ = $(addsuffix .o,$(basename $(CEDARV_SRC)))
CEDARV_DEP = $(addsuffix .d,$(basename $(CEDARV_SRC)))

NV_OBJ = $(addsuffix .o,$(basename $(NV_SRC)))
NV_DEP = $(addsuffix .d,$(basename $(NV_SRC)))

DISPLAY_OBJ = $(addsuffix .o,$(basename $(DISPLAY_SRC)))
DISPLAY_DEP = $(addsuffix .d,$(basename $(DISPLAY_SRC)))

MODULEDIR = $(shell pkg-config --variable=moduledir vdpau)

ifeq ($(MODULEDIR),)
MODULEDIR=/usr/lib/vdpau
endif
USR = /usr
USRLIB = /usr/lib

USRINCLUDE = /usr/include

.PHONY: clean all install

all: $(CEDARV_TARGET) $(TARGET) $(NV_TARGET) $(DISPLAY_TARGET)

$(TARGET): $(OBJ) $(CEDARV_TARGET)
	$(CROSS_COMPILE)$(CC) $(LIB_LDFLAGS) $(LDFLAGS) $(OBJ) $(LIBS) $(LIBS_CEDARV) -o $@

$(NV_TARGET): $(NV_OBJ) $(CEDARV_TARGET) $(TARGET)
	$(CROSS_COMPILE)$(CC) $(LIB_LDFLAGS_NV) $(LDFLAGS) $(NV_OBJ) $(LIBS) $(LIBS_EGL) $(LIBS_GLES2) $(LIBS_VDPAU_SUNXI) $(LIBS_CEDARV) -o $@

$(CEDARV_TARGET): $(CEDARV_OBJ)
	$(CROSS_COMPILE)$(CC) $(LIB_LDFLAGS_CEDARV) $(LDFLAGS) $(CEDARV_OBJ) $(LIBS) -o $@

$(DISPLAY_TARGET): $(DISPLAY_OBJ) $(CEDARV_TARGET) $(TARGET)
	$(CROSS_COMPILE)$(CC) $(LIB_LDFLAGS_DISPLAY) $(LDFLAGS) $(DISPLAY_OBJ) $(LIBS) $(LIBS_CEDARV) -o $@

clean:
	rm -f $(OBJ)
	rm -f $(DEP)
	rm -f $(TARGET)
	rm -f $(NV_OBJ)
	rm -f $(NV_DEP)
	rm -f $(NV_TARGET)
	rm -f $(CEDARV_OBJ)
	rm -f $(CEDARV_DEP)
	rm -f $(CEDARV_TARGET)
	rm -f $(DISPLAY_OBJ)
	rm -f $(DISPLAY_DEP)
	rm -f $(DISPLAY_TARGET)

install: $(TARGET) $(NV_TARGET) $(DISPLAY_TARGET) $(CEDARV_TARGET)
	install -D $(TARGET) $(DESTDIR)$(MODULEDIR)/$(TARGET)
	ln -sf $(TARGET) $(DESTDIR)$(MODULEDIR)/$(TARGET_BASE)
	install -D $(NV_TARGET) $(DESTDIR)$(MODULEDIR)/$(NV_TARGET)
	ln -sf $(NV_TARGET) $(DESTDIR)$(MODULEDIR)/$(NV_TARGET_BASE)
	install -D $(CEDARV_TARGET) $(DESTDIR)$(USRLIB)/$(CEDARV_TARGET)
	ln -sf $(CEDARV_TARGET) $(DESTDIR)$(USRLIB)/$(CEDARV_TARGET_BASE)
	install -D $(DISPLAY_TARGET) $(DESTDIR)$(USRLIB)/$(DISPLAY_TARGET)
	ln -sf $(DISPLAY_TARGET) $(DESTDIR)$(USRLIB)/$(DISPLAY_TARGET_BASE)
	install -D $(VE_H_INCLUDE) $(DESTDIR)$(USRINCLUDE)/$(VE_H_INCLUDE)
	install -D $(LIBCEDARDISPLAY_H_INCLUDE) $(DESTDIR)$(USRINCLUDE)/$(LIBCEDARDISPLAY_H_INCLUDE)

	#create pkgconfig file for libcedarDisplay
	@echo 'prefix=${DESTDIR}${USR}' > ${PCFILE}
	@echo "exec_prefix=\$${prefix}" >> ${PCFILE}
	@echo "libdir=\$${prefix}/lib" >> ${PCFILE}
	@echo "includedir=\$${prefix}/include" >> ${PCFILE}
	@echo "" >> ${PCFILE}
	@echo "Name: cedarDisplay" >> ${PCFILE}
	@echo "Description: library using the Allwinner disp device to configure layers and display video frames" >>  ${PCFILE}
	@echo "Version: 1.0.0" >> ${PCFILE}
	@echo "Cflags: -I\$${includedir}" >> ${PCFILE}
	@echo "Libs: -L\$${libdir} -lcedarDisplay" >> ${PCFILE}
	@echo "Requires: cedar_access" >> ${PCFILE}
	install -D ${PCFILE} ${DESTDIR}${USRLIB}/pkgconfig/cedarDisplay.pc
	@rm ${PCFILE}

	#create pkgconfig file for libcedar_access
	@echo 'prefix=${DESTDIR}${USR}' > ${PCFILE}
	@echo "exec_prefix=\$${prefix}" >> ${PCFILE}
	@echo "libdir=\$${prefix}/lib" >> ${PCFILE}
	@echo "includedir=\$${prefix}/include" >> ${PCFILE}
	@echo "" >> ${PCFILE}
	@echo "Name: cedar_access" >> ${PCFILE}
	@echo "Description: library providing hardware access to the Allwinner cedar hardware + supporting functions" >>  ${PCFILE}
	@echo "Version: 1.0.0" >> ${PCFILE}
	@echo "Cflags: -I\$${includedir}" >> ${PCFILE}
	@echo "Libs: -L\$${libdir} -lcedar_access" >> ${PCFILE}
	@echo "Requires: libump" >>${PCFILE}
	install -D ${PCFILE} ${DESTDIR}${USRLIB}/pkgconfig/cedar_access.pc
	@rm ${PCFILE}

	#create pkgconfig file for libvdpau_nv_sunxi
	@echo 'prefix=${DESTDIR}' > ${PCFILE}
	@echo "exec_prefix=\$${prefix}" >> ${PCFILE}
	@echo "libdir=\$${prefix}/lib/${MODULEDIR}" >> ${PCFILE}
	@echo "includedir=\$${prefix}/include" >> ${PCFILE}
	@echo "" >> ${PCFILE}
	@echo "Name: vdpau_nv_sunxi" >> ${PCFILE}
	@echo "Description: library providing an emulated OpenGLES NV interface to the Allwinner OpenGLES stack" >>  ${PCFILE}
	@echo "Version: 1.0.0" >> ${PCFILE}
	@echo "Cflags: -I\$${includedir}" >> ${PCFILE}
	@echo "Libs: -L\$${libdir} -lvdpau_nv_sunxi" >> ${PCFILE}
	@echo "Requires: vdpau_sunxi" >> ${PCFILE}
	install -D ${PCFILE} ${DESTDIR}${USRLIB}/pkgconfig/vdpau_nv_sunxi.pc
	@rm ${PCFILE}

	#create pkgconfig file for libvdpau_sunxi
	@echo 'prefix=${DESTDIR}${MODULEDIR}' > ${PCFILE}
	@echo "exec_prefix=\$${prefix}" >> ${PCFILE}
	@echo "libdir=\$${prefix}/lib" >> ${PCFILE}
	@echo "includedir=\$${prefix}/include" >> ${PCFILE}
	@echo "" >> ${PCFILE}
	@echo "Name: vdpau_nv_sunxi" >> ${PCFILE}
	@echo "Description: library providing VDPAU driver for the Allwinner video decoder hardware" >>  ${PCFILE}
	@echo "Version: 1.0.0" >> ${PCFILE}
	@echo "Cflags: -I\$${includedir}" >> ${PCFILE}
	@echo "Libs: -L\$${libdir} -lvdpau_sunxi" >> ${PCFILE}
	@echo "Requires: cedar_access" >> ${PCFILE}
	install -D ${PCFILE} ${DESTDIR}${USRLIB}/pkgconfig/vdpau_sunxi.pc
	@rm ${PCFILE}

uninstall:
	rm -f $(DESTDIR)$(MODULEDIR)/$(TARGET)
	rm -f $(DESTDIR)$(MODULEDIR)/$(NV_TARGET)
	rm -f $(DESTDIR)$(USRLIB)/$(CEDARV_TARGET)
	rm -f $(DESTDIR)$(USRLIB)/$(DISPLAY_TARGET)
	rm -f $(DESTDIR)$(USRLIB)/cedarDisplay.pc
	rm -f $(DESTDIR)$(USRLIB)/cedar_access.pc
	rm -f $(DESTDIR)$(USRLIB)/vdpau_nv_sunxi.pc

%.o: %.c
	$(CC) $(DEP_CFLAGS) $(LIB_CFLAGS) $(CFLAGS) -c $< -o $@

include $(wildcard $(DEP))
