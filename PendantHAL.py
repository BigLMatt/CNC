import serial

SERIAL_PORT = 'COM3'
BAUDRATE = 115200

enableX = False
enableY = False
enableZ = False
x = 0
y = 0
z = 0
factor = 1
feedSpeed = 0
FWD = False
REV = False

def decodeFeed(feedByte):
    if((feedByte & 0b01111111) == 0): return False, False, 0
    
    REV = (feedByte & 0b01000000) != 0
    FWD = not(REV)
    feedSpeed = feedByte & 0b00111111
    return FWD, REV, feedSpeed

def decodeJog(jogByte, enableX, enableY, enableZ, x, y, z):
    sign = (jogByte & 0b00100000) != 0
    magnitude = jogByte & 0b00011111

    delta = magnitude * factor
    if sign: delta = -delta

    if enableX: x += delta
    elif enableY: y += delta
    elif enableZ: z += delta
    
    return x,y,z
    
def decodeAxis(axisByte):
    scaleBits = (axisByte & 0b00001100) >> 2
    axisBits = axisByte & 0b00000011

    enableX = False
    enableY = False
    enableZ = False

    match(scaleBits):
        case 0b01:
            factor = 1
        case 0b10:
            factor = 10
        case 0b11:
            factor = 100
        case _:
            factor = 0
    
    match(axisBits):
        case 0b01:
            enableX = True
        case 0b10:
            enableY = True
        case 0b11:
            enableZ = True
    

    return enableX, enableY, enableZ, factor

try:
    ser = serial.Serial(
        port = SERIAL_PORT,
        baudrate = BAUDRATE,
        timeout = None,
        bytesize = serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE)
    print(f'[INFO] Serial port opened at {SERIAL_PORT} at baudrate: {BAUDRATE} baud.')

except serial.SerialException as e:
    print(f'[ERROR] Failed to open serial port at {SERIAL_PORT}: {e}')
    exit(1)

while True:
    encodedByte = ser.read(1)

    if((encodedByte & 0b10000000) == 0b10000000):
        FWD,REV,feedSpeed = decodeFeed(encodedByte)
    elif((encodedByte & 0b11000000) == 0b01000000):
        x,y,z = decodeJog(encodedByte, enableX, enableY, enableZ, factor, x, y, z)
    elif((encodedByte & 0b11000000) == 0b00000000):
        enableX,enableY,enableZ,factor = decodeAxis(encodedByte)

    print(f'X: {x}, Y: {y} Z: {z}, FWD: {FWD}, REV: {REV}, Feed: {feedSpeed}')