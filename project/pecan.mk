# top level project rules for the LG P350 (PECAN) project
#
LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := pecan

MODULES += app/aboot

#DEFINES += WITH_DEBUG_DCC=1
#DEFINES += WITH_DEBUG_UART=1
DEFINES += WITH_DEBUG_FBCON=1
