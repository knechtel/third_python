CPYTHON_SRC := $(abspath ../..)
REPO_ROOT := $(abspath ../../../..)
PORT_DIR := $(abspath .)
MICROPYTHON_SRC := $(REPO_ROOT)/third_party/micropython
BUILD_DIR ?= $(REPO_ROOT)/build/cpython-maios
BUILD_TRIPLET ?= $(shell env -u CC "$(CPYTHON_SRC)/config.guess")
HOST_TRIPLET ?= i686-pc-maios
BUILD_PYTHON ?= python3.11
CROSS_COMPILE ?= i686-pc-maios-
READELF ?= $(shell command -v readelf)

ifeq ($(origin CC),default)
CC = $(CROSS_COMPILE)gcc
endif

SYSROOT ?=
CPPFLAGS ?=
CFLAGS ?= -O2 -ffreestanding -fno-pie -fno-stack-protector
LDFLAGS ?= -nostdlib -no-pie
CPPFLAGS += -D__maios__ -U__linux__ -Ulinux
CPPFLAGS += -I$(REPO_ROOT)
CPPFLAGS += -I$(CPYTHON_SRC)/Include
CPPFLAGS += -I$(BUILD_DIR)
LIBM_DIR := $(MICROPYTHON_SRC)/lib/libm_dbl
LIBM_SOURCES := $(filter-out $(LIBM_DIR)/thumb_vfp_sqrt.c,$(wildcard $(LIBM_DIR)/*.c))
LIBM_OBJECTS := $(patsubst $(LIBM_DIR)/%.c,$(BUILD_DIR)/runtime_libm_%.o,$(LIBM_SOURCES))

ifneq ($(SYSROOT),)
CPPFLAGS += --sysroot=$(SYSROOT)
LDFLAGS += --sysroot=$(SYSROOT)
endif

.PHONY: configure core adapter runtime

configure:
	@mkdir -p "$(BUILD_DIR)"
	@cd "$(BUILD_DIR)" && \
	  CONFIG_SITE="$(PORT_DIR)/config.site" \
	  CPPFLAGS="$(CPPFLAGS)" CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" \
	  CC="$(CC)" READELF="$(READELF)" \
	  "$(CPYTHON_SRC)/configure" \
	    --build="$(BUILD_TRIPLET)" \
	    --host="$(HOST_TRIPLET)" \
	    --with-build-python="$(BUILD_PYTHON)" \
	    --without-ensurepip --disable-ipv6 --with-libm=no && \
	  cp "$(PORT_DIR)/Modules.Setup.local.example" Modules/Setup.local && \
	  mkdir -p "$(BUILD_DIR)/ports/maios"

core:
	$(MAKE) -C "$(BUILD_DIR)" libpython3.11.a

adapter: $(BUILD_DIR)/cpython_port.o

$(BUILD_DIR)/cpython_port.o: $(PORT_DIR)/cpython_port.c $(PORT_DIR)/cpython_port.h
	$(CC) $(CPPFLAGS) -I"$(BUILD_DIR)" $(CFLAGS) -c "$<" -o "$@"

runtime: $(BUILD_DIR)/libmaiosrt.a

$(BUILD_DIR)/runtime_%.o: $(PORT_DIR)/runtime/%.c
	@mkdir -p "$(BUILD_DIR)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -c "$<" -o "$@"

$(BUILD_DIR)/runtime_libm_%.o: $(LIBM_DIR)/%.c
	@mkdir -p "$(BUILD_DIR)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -I"$(LIBM_DIR)" -c "$<" -o "$@"

$(BUILD_DIR)/libmaiosrt.a: $(BUILD_DIR)/runtime_time.o $(BUILD_DIR)/runtime_posix.o $(BUILD_DIR)/runtime_fs.o $(BUILD_DIR)/runtime_stdio.o $(BUILD_DIR)/runtime_cpython_compat.o $(BUILD_DIR)/runtime_locale.o $(BUILD_DIR)/runtime_stdlib.o $(BUILD_DIR)/runtime_random.o $(BUILD_DIR)/runtime_math_misc.o $(LIBM_OBJECTS)
	$(AR) rcs "$@" $^
