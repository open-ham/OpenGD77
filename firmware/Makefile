##
## export PATH=$ARM_TOOLCHAIN_PATH:$PATH
## mkdir build && cd build && make -f ../Makefile -j8
##
## if needed, you can specify some options like this:
## make -f ../Makefile -j8 RADIO=GD77s VERBOSE=1
##

ifndef RADIO
	RADIO := GD77
endif

##
## Selection of MCU platform and baseband
##
ifeq ("$(RADIO)","GD77")
	DEFINES  := -DPLATFORM_GD77
	BIN2SGL_OPT  :=
	SGL_FILE  := OpenGD77.sgl
else ifeq ("$(RADIO)","GD77s")
	DEFINES  := -DPLATFORM_GD77S
	BIN2SGL_OPT  := -m GD-77S
	SGL_FILE  := OpenGD77S.sgl
else ifeq ("$(RADIO)","DM1801")
	DEFINES  := -DPLATFORM_DM1801
	BIN2SGL_OPT  := -m DM-1801
	SGL_FILE  := OpenDM1801.sgl
else ifeq ("$(RADIO)","RD5R")
	DEFINES  := -DPLATFORM_RD5R
	BIN2SGL_OPT  := -m RD-5R
	SGL_FILE  := OpenDM5R.sgl
endif

##
## List here your source files (both .s, .c and .cpp)
##
SRC := \
$(wildcard ../source/*.c)                   \
$(wildcard ../source/dmr_codec/*.c)         \
../source/dmr_codec/codec_bin.S             \
$(wildcard ../source/functions/*.c)         \
$(wildcard ../source/hotspot/*.c)           \
$(wildcard ../source/hardware/*.c)          \
$(wildcard ../source/interfaces/*.c)        \
$(wildcard ../source/io/*.c)                \
$(wildcard ../source/SeggerRTT/RTT/*.c)     \
$(wildcard ../source/SeggerRTT/Syscalls/*.c)\
$(wildcard ../source/usb/*.c)               \
$(wildcard ../source/user_interface/*.c )

OZERO_SOURCES := \
../drivers/fsl_dspi.c                                   \
../source/SeggerRTT/RTT/SEGGER_RTT.c                    \
../source/SeggerRTT/RTT/SEGGER_RTT_printf.c             \
../source/SeggerRTT/Syscalls/SEGGER_RTT_Syscalls_GCC.c  \
../source/dmr_codec/codec.c                             \
../source/firmware.c                                    \
../source/functions/sound.c                             \
../source/hardware/EPL003.c                             \
../source/hardware/SPI_Flash.c                          \
../source/hardware/UC1701_transfer.c                    \
../source/interfaces/i2s.c                              \
../source/main.c                                        \
../source/semihost_hardfault.c

##
## Drivers' source files and include directories
##
DRIVERS_INC := \
-I../                             \
-I../drivers                      \
-I../usb/device/class             \
-I../usb/device/source            \
-I../usb/device/include           \
-I../usb/device/source/ehci       \
-I../usb/device/source/khci       \
-I../usb/device/source/lpcip3511  \
-I../usb/include                  \
-I../usb/phy 

DRIVERS_SRC := \
$(wildcard ../drivers/*.c)                     \
$(wildcard ../usb/device/class/*.c)            \
$(wildcard ../usb/device/source/*.c)           \
$(wildcard ../usb/device/source/ehci/*.c)      \
$(wildcard ../usb/device/source/khci/*.c)      \
$(wildcard ../usb/device/source/lpcip3511/*.c) \
$(wildcard ../usb/phy/*.c)                     \
../osa/usb_osa_freertos.c 

##
## List here additional static libraries with relative path
##
LIBS :=  

##
## List here additional include directories (in the form -Iinclude_dir)
##
INCLUDE_DIRS := \
-I../source             \
-I../include            \
-I../include/hardware   \
-I../include/codec      \
-I../include/functions  \
-I../include/interfaces \
-I../include/io         \
-I../include/usb        \
-I../osa \

##
## List here additional defines
##
DEFINES += \
-DGITVERSION=\"`git rev-parse --short HEAD`\" \
-DSDK_DEBUGCONSOLE=0     \
-DUSE_SEGGER_RTT         \
-DCR_INTEGER_PRINTF      \
-DPRINTF_FLOAT_ENABLE=0  \
-DFSL_RTOS_FREE_RTOS     \
-DSDK_OS_FREE_RTOS       \
-DCPU_MK22FN512VLL12_cm4 \
-D__MCUXPRESSO           \
-D__USE_CMSIS            \
-DDEBUG                  \


##
## Debug defines
##

DEBUG := 1
ifeq ($(DEBUG), 1)
    DEFINES += -DDEBUG -g3
else
    DEFINES += -DNDEBUG
endif


##
## Define used to select target processor
##
TARGET := -DCPU_MK22FN512VLL12


##
## Optimization level
##
OPTLEVEL := -Os
#OPTLEVEL:= -O2
#OPTLEVEL:= -O3
#OPTLEVEL:= -Os

##
## Device-specific source files and include directories, e.g. startup code
##
DEVICE_INC := -I../CMSIS -I../board -I../device
DEVICE_SRC :=  \
$(wildcard ../board/*.c)      \
../device/system_MK22F51212.c \
$(wildcard ../startup/*.c) 

##
## Operating system's source files and include directories
##
OS_INC := \
-I../amazon-freertos/include           \
-I../amazon-freertos/FreeRTOS/portable 

OS_SRC := \
$(wildcard ../amazon-freertos/FreeRTOS/*.c)                 \
../amazon-freertos/FreeRTOS/portable/fsl_tickless_systick.c \
../amazon-freertos/FreeRTOS/portable/heap_4.c               \
../amazon-freertos/FreeRTOS/portable/port.c

ALL_INC := $(OS_INC) $(DEVICE_INC) $(DRIVERS_INC) $(INCLUDE_DIRS)
ALL_SRC := $(SRC) $(OS_SRC) $(DEVICE_SRC) $(DRIVERS_SRC)
CONFIGS := $(TARGET)  \
			-fno-common -g3 -Wall -ffunction-sections   \
			-fdata-sections -ffreestanding -fno-builtin \
			-fmerge-constants

REDLIB := 0
ifeq ($(REDLIB), 1)
    DEFINES += -D__REDLIB__
    LINKERSCRIPT = ../linkerscripts/firmware.ld
    CONFIGS += -specs=redlib.specs
else
    DEFINES += -D__NEWLIB_H__
    LINKERSCRIPT = ../linkerscripts/firmware_newlib.ld
endif


ifeq ("$(VERBOSE)","1")
Q := 
ECHO := @true
else
Q := @
ECHO := @echo
endif


BUILD_DIR ?= ./bin

## Replaces both "foo.cpp"-->"foo.o" and "foo.c"-->"foo.o"
OBJ := $(addprefix $(BUILD_DIR)/,  $(addsuffix .o, $(basename $(ALL_SRC))))
OZERO_OBJECTS := $(addprefix $(BUILD_DIR)/,  $(addsuffix .o, $(basename $(OZERO_SOURCES))))

CFLAGS   := $(ALL_INC) -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb $(CONFIGS) \
            $(DEFINES) $(EXCEPT) -c

AFLAGS   := -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb
LFLAGS   := -nostdlib -Xlinker -Map="firmware.map" -Xlinker --gc-sections -Xlinker \
			-print-memory-usage -Xlinker --sort-section=alignment -mcpu=cortex-m4  \
			-mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -T $(LINKERSCRIPT)

DFLAGS   := -MMD -MP

LINK_LIBS := $(LIBS)

CC  := arm-none-eabi-gcc
AS  := arm-none-eabi-as
CP  := arm-none-eabi-objcopy
SZ  := arm-none-eabi-size

all: $(SGL_FILE)

$(SGL_FILE): firmware.axf
	$(ECHO) 'Performing post-build steps'
	$(Q)$(SZ) $(BUILD_DIR)/firmware.axf
	$(Q)$(CP) -v -O binary $(BUILD_DIR)/firmware.axf $(BUILD_DIR)/firmware.bin
	$(Q) ../tools/bin2sgl $(BIN2SGL_OPT) $(BUILD_DIR)/firmware.bin
	$(Q) mv $(BUILD_DIR)/firmware.sgl $(BUILD_DIR)/$(SGL_FILE)
#../tools/GD77_FirmwareLoader "firmware.bin" GUI
# checksum -p MK22FN512xxx12 -d "firmware.bin"

firmware.axf: $(OBJ) #all-recursive
	$(ECHO) "[LD ] firmware.axf"
	$(Q)$(CC) $(LFLAGS) -o $(BUILD_DIR)/firmware.axf $(OBJ) $(LINK_LIBS)

$(OZERO_OBJECTS):
	$(Q)$(MKDIR_P) $(dir $@)
	$(ECHO) "[CC0] $(@:bin/%.o=%.c)"
	$(Q)$(CC)  $(DFLAGS) $(CFLAGS) -O0 $(@:bin/%.o=%.c) -o $@

$(BUILD_DIR)/%.o: %.s
	$(Q)$(MKDIR_P) $(dir $@)
	$(ECHO) "[AS ] $<"
	$(Q)$(AS)  $(AFLAGS) $< -o $@

$(BUILD_DIR)/%.o : %.c
	$(Q)$(MKDIR_P) $(dir $@)
	$(ECHO) "[CC ] $<"
	$(Q)$(CC)  $(DFLAGS) $(CFLAGS) $(OPTLEVEL) $< -o $@

.PHONY: clean

clean:
	$(Q)$(RM) -rf $(BUILD_DIR) firmware.map && find . -type f -name "*.o" -exec rm {} + -o -name "*.d" -exec rm {} +

-include $(DEPS)

MKDIR_P ?= mkdir -p
