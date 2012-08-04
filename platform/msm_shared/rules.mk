LOCAL_DIR := $(GET_LOCAL_DIR)

INCLUDES += \
			-I$(LOCAL_DIR)/include

DEFINES += $(TARGET_XRES)
DEFINES += $(TARGET_YRES)

OBJS += \
	$(LOCAL_DIR)/timer.o \
	$(LOCAL_DIR)/proc_comm.o \
	$(LOCAL_DIR)/debug.o \
	$(LOCAL_DIR)/smem.o \
	$(LOCAL_DIR)/smem_ptable.o \
	$(LOCAL_DIR)/hsusb.o \
	$(LOCAL_DIR)/jtag_hook.o \
	$(LOCAL_DIR)/jtag.o \
	$(LOCAL_DIR)/nand.o \
	$(LOCAL_DIR)/lcdc.o \
	$(LOCAL_DIR)/mddi.o \
	$(LOCAL_DIR)/mmc.o

ifeq ($(PLATFORM),msm8x60)
	OBJS += $(LOCAL_DIR)/mipi_dsi.o \
	        $(LOCAL_DIR)/i2c_qup.o
endif

ifeq ($(PLATFORM),msm8x60)
        OBJS += $(LOCAL_DIR)/uart_dm.o
else
        OBJS += $(LOCAL_DIR)/uart.o
endif
