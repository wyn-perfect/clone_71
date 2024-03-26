CW2VEC_DIR := $(call select_from_ports,cw2vec)/src/app/cw2vec



TARGET = word

LIBS += libc libm stdcxx  posix cxx 
LIBS += libpng  zlib

SRC_CC   += word2vec/src/main.cpp  

CC_OPT  += -DHAVE_CONFIG_H -DGENODE_BUILD -mavx2 -mfma -ffast-math -DUSE_INT8_INT4_PRODUCT -fpermissive -DQM_x86

CC_WARN = -Wall -w -Wno-unused -Wno-maybe-uninitialized -Wno-format-truncation \
          -Wno-stringop-truncation -Wno-stringop-overflow -Wno-unknown-pragmas



INC_DIR += $(PRG_DIR)
INC_DIR += $(CW2VEC_DIR)

INC_DIR += $(CW2VEC_DIR)/word2vec/src/include

vpath %.cpp      $(CW2VEC_DIR)



