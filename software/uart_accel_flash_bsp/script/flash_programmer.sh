#!/bin/sh
#
# This file was automatically generated.
#
# It can be overwritten by nios2-flash-programmer-generate or nios2-flash-programmer-gui.
#

#
# Converting Binary File: C:\Users\clark\Documents\eie-y2\info-processing-labs\PROJECT_NIOS\software\flash_uart_accelerometer\flash_uart_accelerometer.objdump to: "..\flash/flash_uart_accelerometer_onchip_flash_data.flash"
#
bin2flash --input="flash_uart_accelerometer.objdump" --output="../flash/flash_uart_accelerometer_onchip_flash_data.flash" --location=0x0 

#
# Programming File: "..\flash/flash_uart_accelerometer_onchip_flash_data.flash" To Device: onchip_flash_data
#
nios2-flash-programmer "../flash/flash_uart_accelerometer_onchip_flash_data.flash" --base=0x80000 --sidp=0x1410F8 --id=0x0 --accept-bad-sysid --device=1 --instance=0 '--cable=USB-Blaster on localhost [USB-0]' --program 

