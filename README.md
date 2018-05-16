# ZedboardDigitalClock

This is a software for the Zedboard SoC. It is able to generate a digital clock using the onboard Zynq  processor. 
It can display the clock on a monitor through a VGA cable .


# Running Instructions: 

This program REQUIRES vivado 2017.4 to run.

Use the hardware files from the following git : https://github.com/delhatch/VGA_mem_mapped

1) Open SDK of the project.
2) Flash the FPGA with the predefined hardware
3) Run SDK application ( C code)
4) In the console, it will ask for seconds, minutes , and hours.
5) We used Putty to run the console since the Vivado console was generating errors.
6) Set time based on console prompt.
7) The clock should be running. 


Video Link:
https://www.youtube.com/watch?v=MjSMgS9EPO0
