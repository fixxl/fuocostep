################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
LSS += \
Fuocostep.lss \

FLASH_IMAGE += \
Fuocostep.hex \

EEPROM_IMAGE += \
Fuocostep.eep \

SIZEDUMMY += \
sizedummy \


# All Target
all: Fuocostep.elf secondary-outputs

# Tool invocations
Fuocostep.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: AVR C Linker'
	avr-gcc -Wl,-Map,Fuocostep.map -mmcu=atmega328p -o "Fuocostep.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

Fuocostep.lss: Fuocostep.elf
	@echo 'Invoking: AVR Create Extended Listing'
	-avr-objdump -h -S Fuocostep.elf  >"Fuocostep.lss"
	@echo 'Finished building: $@'
	@echo ' '

Fuocostep.hex: Fuocostep.elf
	@echo 'Create Flash image (ihex format)'
	-avr-objcopy -R .eeprom -R .fuse -R .lock -R .signature -O ihex Fuocostep.elf  "Fuocostep.hex"
	@echo 'Finished building: $@'
	@echo ' '

Fuocostep.eep: Fuocostep.elf
	@echo 'Create eeprom image (ihex format)'
	-avr-objcopy -j .eeprom --no-change-warnings --change-section-lma .eeprom=0 -O ihex Fuocostep.elf  "Fuocostep.eep"
	@echo 'Finished building: $@'
	@echo ' '

sizedummy: Fuocostep.elf
	@echo 'Invoking: Print Size'
	-avr-size --format=avr --mcu=atmega328p Fuocostep.elf
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(FLASH_IMAGE)$(ELFS)$(OBJS)$(ASM_DEPS)$(EEPROM_IMAGE)$(S_DEPS)$(SIZEDUMMY)$(S_UPPER_DEPS)$(LSS)$(C_DEPS) Fuocostep.elf
	-@echo ' '

secondary-outputs: $(LSS) $(FLASH_IMAGE) $(EEPROM_IMAGE) $(SIZEDUMMY)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
