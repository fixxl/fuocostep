################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../adc.c \
../eeprom.c \
../leds.c \
../shiftregister.c \
../stepper.c \
../terminal.c \
../timeparser.c \
../timer.c \
../uart.c 

OBJS += \
./adc.o \
./eeprom.o \
./leds.o \
./shiftregister.o \
./stepper.o \
./terminal.o \
./timeparser.o \
./timer.o \
./uart.o 

C_DEPS += \
./adc.d \
./eeprom.d \
./leds.d \
./shiftregister.d \
./stepper.d \
./terminal.d \
./timeparser.d \
./timer.d \
./uart.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	avr-gcc -Wall -Os -fpack-struct -fshort-enums -ffunction-sections -fdata-sections -std=gnu99 -funsigned-char -funsigned-bitfields -mmcu=atmega328p -DF_CPU=6000000UL -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


