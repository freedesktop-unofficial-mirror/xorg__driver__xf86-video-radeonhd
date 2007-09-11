AM_CFLAGS = @XORG_CFLAGS@
radeonhd_drv_la_LTLIBRARIES = radeonhd_drv.la
radeonhd_drv_la_LDFLAGS = -module -avoid-version
radeonhd_drv_ladir = @moduledir@/drivers

radeonhd_drv_la_SOURCES = \
	rhd_atombios.c \
	rhd_i2c.c \
	rhd_crtc.c \
	rhd_connector.c \
	rhd_cursor.c \
	rhd_dac.c \
	rhd_driver.c \
	rhd_helper.c \
	rhd_id.c \
	rhd_lut.c \
	rhd_modes.c \
	rhd_output.c \
	rhd_pll.c \
	rhd_tmds.c \
	rhd_vga.c \
	rhd_atombios.h \
	rhd.h \
	rhd_i2c.h \
	rhd_crtc.h \
	rhd_connector.h \
	rhd_cursor.h \
	rhd_lut.h \
	rhd_modes.h \
	rhd_output.h \
	rhd_pll.h \
	rhd_regs.h \
	rhd_vga.h

if ATOM_BIOS
radeonhd_drv_la_SOURCES += \
	rhd_atomwrapper.h
endif

if ATOM_BIOS

noinst_LTLIBRARIES = libatom.la
libatom_la_CFLAGS = $(AM_CFLAGS) -DDRIVER_PARSER -DDISABLE_EASF -DENABLE_ALL_SERVICE_FUNCTIONS
# libatom_la_LDFLAGS = -module -avoid-version

libatom_la_SOURCES = \
	rhd_atomwrapper.c \
	rhd_atomwrapper.h \
	AtomBios/CD_Operations.c \
	AtomBios/Decoder.c \
	AtomBios/hwserv_drv.c \
	AtomBios/include/atombios.h \
	AtomBios/include/CD_binding.h \
	AtomBios/include/CD_Common_Types.h \
	AtomBios/include/CD_Definitions.h \
	AtomBios/include/CD_hw_services.h \
	AtomBios/include/CD_Opcodes.h \
	AtomBios/include/CD_Structs.h \
	AtomBios/include/Decoder.h \
	AtomBios/include/regsdef.h

INCLUDES = -I./AtomBios/includes

radeonhd_drv_la_LIBADD = libatom.la

endif