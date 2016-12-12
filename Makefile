ifeq ($(DEBUG), 1)
    BUILD_DIR := debug
    CXXFLAGS  += -O0 -g3 -Wno-misleading-indentation -Wno-unknown-warning-option
else
    BUILD_DIR := release
    CXXFLAGS  += -O2
endif

PATH_BIN  := dvbjet

SRC_DIR   := src
INCLUDES  := -Isyscpp/include
LDLIBS    := -lpthread

include syscpp/posix.mk
