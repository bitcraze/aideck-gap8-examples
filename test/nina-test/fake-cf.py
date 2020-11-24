# This scipt can be used to run the tests against the NINA using a serial cable from the PC.

import serial
import struct

ser = serial.Serial('/dev/ttyUSB5', 115200)  # open serial port

# Used to read out "hello\0"

#for i in range(6):
#  test = ser.read()          # read one byte
#  print(test)

# Read out if the button has been high
ser.write(struct.pack('<B', 0x01))
test = ser.read()
print("Button has been high: " + str(test))

# Read out if the button has been low
ser.write(struct.pack('<B', 0x02))
test = ser.read()
print("Button has been low: " + str(test))

# Reset from crazyflie
ser.write(struct.pack('<B', 0x03))
test = ser.read()
print("Button is reset " + str(test))
ser.write(struct.pack('<B', 0x02))
test = ser.read()
print("Button has been low again: " + str(test))

# Read out the GPIO mask
ser.write(struct.pack('<B', 0x04))
test = ser.read()          # read one byte
print("Mask: " + str(test))

# Read out the GPIO mask
ser.write(struct.pack('<B', 0x05))
test = ser.read()          # read one byte
print("Gap8 has been reset from the NINAm: " + str(test))


ser.close()             # close port

# Reset 