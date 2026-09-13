# Builds the demo shell for both STM32 Nucleos. Needs arm-none-eabi-gcc only.
#   make            both targets
#   make h753       one target
# Artifacts land in build/ as demo-shell-<board type>.bin.

CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size

CFLAGS_COMMON = -mthumb -mfloat-abi=soft -Os -g -std=c99 -Wall -Wextra \
                -ffreestanding -ffunction-sections -fdata-sections
LDFLAGS_COMMON = -nostdlib -Wl,--gc-sections

SRC = ../../src/shell.c port.c startup.c

CFLAGS_h753 = -mcpu=cortex-m7 -DTARGET_H753
CFLAGS_f446 = -mcpu=cortex-m4 -DTARGET_F446
BOARD_h753  = stm32-h753
BOARD_f446  = stm32-f446

all: h753 f446

define TARGET_RULE
$(1): build/demo-shell-$(BOARD_$(1)).bin
build/demo-shell-$(BOARD_$(1)).elf: $(SRC) $(1)/link.ld ../../src/shell.h
	@mkdir -p build
	$(CC) $(CFLAGS_COMMON) $(CFLAGS_$(1)) $(LDFLAGS_COMMON) -T $(1)/link.ld \
	      -o $$@ $(SRC) -lgcc
	$(SIZE) $$@
build/demo-shell-$(BOARD_$(1)).bin: build/demo-shell-$(BOARD_$(1)).elf
	$(OBJCOPY) -O binary $$< $$@
endef

$(foreach t,h753 f446,$(eval $(call TARGET_RULE,$(t))))

clean:
	rm -rf build

.PHONY: all clean h753 f446
