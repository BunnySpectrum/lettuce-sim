
# Energia pinout
## Used
P1.0 = red LED
P1.1/2 = UART
P1.3 = PUSH2 button
P1.5 SCK
P1.6/7 MISO/MOSI
(P2.0 CS)
P2.1/2 = I2C1

## Open
P1.4 
P2.3
P2.4
P2.5
P2.6
P2.7


# Parallel-to-serial shift reg
SN74HCS165

Vcc 2-6V

Keep CLK_INH low
Can also keep SER low
Unused outputs may be left floating


Two modes: shift, and load

Load mode: SH/LD_L = 0
- asynch load 

Shift mode: SH/LD_L = 1
- on rising CLK edge, shift data


To read: 
- pulse SH/LD_L low 6-7ns
- send 8 positive clock pulses 7-11ns
- read the Qh value after each pulse after setup time

## Pinout
GPO
- SH/LD_L
- CLK

GPI
- Qh


# Serial-to-parallel shift reg
SN74HCS595

Vcc 2-6V
Maybe 6mA outputs at 3.3V? 

Pulse durations
- SRCLK/RCLK high/low: 7-9ns
- SRCLR_L low: 7-10ns


Modes:
OE_L=1: outputs disabled
OE_L=0: outputs enabled
SRCLR_L=0: shift reg cleared
SRCLR_L=1 & SER=0, when SRCLK rises: shift in 0, rest shift down
SRCLR_L=1 & SER=1, when SRCLK rises: shift in 1, rest shift down
SRCLR_L=1 & RCLK rises: data stored in register
SRCLR_L=1 & both RCLK and SRCLK rises: data stored then shifted


Keep OE_L low
Keep SRCLR_L high

SER to GPO
SRCLK & RCLK to GPO

For each bit in output byte:
- Set SER to value, then pulse SRCLK
After 8 pulses (assuming single register), pulse RCLK

## Pinout
GPO
- SER
- SRCLK
- RCLK

# Serial terminal
tio -b 115200 /dev/cu.usbmodem214302
Issues w/
- CoolTerm (no cursor moves)
- minicom (no line mode)
- picocom (no line mode/clearing)
- screen (no line mode)

minicom -b 115200 -D /dev/cu.usbmodem21202

# unicode
b"".join([x.encode('utf-8') for x in ru_settings])
