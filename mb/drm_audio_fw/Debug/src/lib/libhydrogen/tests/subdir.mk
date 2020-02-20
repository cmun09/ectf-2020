################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/lib/libhydrogen/tests/tests.c 

OBJS += \
./src/lib/libhydrogen/tests/tests.o 

C_DEPS += \
./src/lib/libhydrogen/tests/tests.d 


# Each subdirectory must supply rules for building sources it contributes
src/lib/libhydrogen/tests/%.o: ../src/lib/libhydrogen/tests/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MicroBlaze gcc compiler'
	mb-gcc -Wall -O0 -g3 -I"/ectf/mb/drm_audio_fw_bsp/microblaze_0/include" -c -fmessage-length=0 -MT"$@" -I"/ectf/mb/drm_audio_fw_bsp/microblaze_0/include" -mlittle-endian -mcpu=v10.0 -mxl-soft-mul -Wl,--no-relax -ffunction-sections -fdata-sections -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


