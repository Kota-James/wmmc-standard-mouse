# コマンドライン / CI 用ビルド（CubeIDE の GUI ビルドと併存する。出力は build/ のみ）
# 使い方: make          … ファームウェア (build/NucleoMouse2023.elf/.bin) を生成
#         make clean    … build/ を削除
# 必要なもの: arm-none-eabi-gcc（Ubuntu: sudo apt install gcc-arm-none-eabi libnewlib-arm-none-eabi）

TARGET  = NucleoMouse2023
BUILD   = build

CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size

# ソース（ワイルドカードで拾うため、ファイル追加時に Makefile の編集は不要）
C_SOURCES  = $(wildcard Core/Src/*.c) \
             $(wildcard Drivers/STM32F3xx_HAL_Driver/Src/*.c)
ASM_SOURCES = $(wildcard Core/Startup/*.s)

INCLUDES = -ICore/Inc \
           -IDrivers/STM32F3xx_HAL_Driver/Inc \
           -IDrivers/STM32F3xx_HAL_Driver/Inc/Legacy \
           -IDrivers/CMSIS/Device/ST/STM32F3xx/Include \
           -IDrivers/CMSIS/Include

DEFS = -DUSE_HAL_DRIVER -DSTM32F303x8

# CubeIDE の Debug 構成と同じ設定（Cortex-M4F / hard float）
MCU     = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS  = $(MCU) $(DEFS) $(INCLUDES) -std=gnu11 -O0 -g3 -Wall \
          -ffunction-sections -fdata-sections --specs=nano.specs -MMD -MP
ASFLAGS = $(MCU) -x assembler-with-cpp
LDFLAGS = $(MCU) -T STM32F303K8TX_FLASH.ld --specs=nano.specs --specs=nosys.specs \
          -Wl,-Map=$(BUILD)/$(TARGET).map -Wl,--gc-sections -static \
          -Wl,--start-group -lc -lm -Wl,--end-group

OBJS = $(addprefix $(BUILD)/,$(C_SOURCES:.c=.o) $(ASM_SOURCES:.s=.o))

all: $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).bin
	$(SIZE) $(BUILD)/$(TARGET).elf

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) -c $(ASFLAGS) $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -rf $(BUILD)

-include $(OBJS:.o=.d)

.PHONY: all clean
