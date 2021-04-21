#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Copyright (C) 2019  F1RMB, Daniel Caujolle-Bert <f1rmb.daniel@gmail.com>
                    VK3KYY / G4KYF, Roger Clark. 

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


You also need python3-serial

On windows install pyserial, pillow and cimage (using pip install ...)

"""
from datetime import datetime
import time
import os.path
import ntpath
import getopt, sys
import serial
import platform
from PIL import Image
from PIL import ImageDraw, ImageColor


imgBuffer = [0x0] * 1024
MAX_TRANSFER_SIZE = 32

# Default values
DEFAULT_SCALE = 2
DEFAULT_FOREGROUND = "#000000"
DEFAULT_BACKGROUND = "#99d9ea"
##
PROGRAM_VERSION = '0.0.2'

###
# Send the command to the GD-77 and read buffer back
###
def sendAndReceiveCommand(ser):
    #
    DataModeReadScreenGrab = 6
    sendbuffer = [0x0] * 8
    readbuffer = [0x0] * 64
    currentDataAddressInTheRadio = 0
    currentDataAddressInLocalBuffer = 0
    size = 1024
    progress = 0
    
    ser.flush()
    
    print(" - downloading from the GD-77...")
    while (size > 0):
        if (size > MAX_TRANSFER_SIZE):
            size = MAX_TRANSFER_SIZE;
	
        sendbuffer[0] = ord('R')
        sendbuffer[1] = DataModeReadScreenGrab
        sendbuffer[2] = ((currentDataAddressInTheRadio >> 24) & 0xFF)
        sendbuffer[3] = ((currentDataAddressInTheRadio >> 16) & 0xFF)
        sendbuffer[4] = ((currentDataAddressInTheRadio >> 8) & 0xFF)
        sendbuffer[5] = ((currentDataAddressInTheRadio >> 0) & 0xFF)
        sendbuffer[6] = ((size >> 8) & 0xFF);
        sendbuffer[7] = ((size >> 0) & 0xFF);
    
        ret = ser.write(sendbuffer)
        if (ret != 8):
            print("ERROR: write() wrote " + ret + " bytes")
            return False

        while (ser.in_waiting == 0):
            time.sleep(0.2)
        
        readbuffer = ser.read(ser.in_waiting)
    
        header = ord('R')
            
        if (readbuffer[0] == header):

            l = (readbuffer[1] << 8) + (readbuffer[2] << 0)

            for i in range(0, l):
                imgBuffer[currentDataAddressInLocalBuffer] = readbuffer[i + 3]

                currentDataAddressInLocalBuffer += 1
    
            progress = currentDataAddressInTheRadio * 100 // 1024
            print("\r - reading: " + str(progress) + "%", end='')
            sys.stdout.flush()
            
            currentDataAddressInTheRadio += l
        else:
            print("read stopped (error at " + str(currentDataAddressInTheRadio) + ")")
            return False
	    
        size = 1024 - currentDataAddressInTheRadio
    print("")
    return True


###
# Scale and save the downloaded image to a PNG file
###
def saveImage(filename, scale, foreground, background):
    # Image (scale 1:1)
    img = Image.new('RGB', (128, 64), background)
    # Drawing context
    d = ImageDraw.Draw(img)

    for stripe in range(0, 8):
        for column in range(0, 128):
            for line in range(0, 8):
                if (((imgBuffer[(stripe * 128) + column] >> line) & 0x01) != 0):
                    d.point((column, stripe * 8 + line), foreground)


    print(" - saving " + filename + ".png")    
    if (scale == 1):
        img.save(filename + '.png')
    else:
        rimg = img.resize((128 * scale, 64 * scale), resample=Image.NEAREST)
        rimg.save(filename + '.png')


###
# Display command line options
###
def usage():
    print("GD-77 Screen Grabber v" + PROGRAM_VERSION)
    print("Usage:  " + ntpath.basename(sys.argv[0]) + " [OPTION]")
    print("")
    print("    -h, --help                 : Display this help text,")
    print("    -d, --device=<device>      : Use the specified device as serial port,")
    print("    -s, --scale=v              : Apply scale factor (1..x) [default: " + str(DEFAULT_SCALE) + "],")
    print("    -o, --output=<filename>    : Save the image in <filename>.png (without file extension),")
    print("    -f, --foreground=#RRGGBB   : Use specified color as foreground color [default: " + DEFAULT_FOREGROUND + "],")
    print("    -b, --background=#RRGGBB   : Use specified color as background color [default: " + DEFAULT_BACKGROUND + "].")
    print("")


###
# main function
###
def main():
    # Default tty
    if (platform.system() == 'Windows'):
        serialDev = "COM13"
    else:
        serialDev = "/dev/ttyACM0"

    scale = DEFAULT_SCALE
    foreground = DEFAULT_FOREGROUND
    background = DEFAULT_BACKGROUND
    dateTimeObj = datetime.now()
    timestampStr = dateTimeObj.strftime("%Y-%m-%d_%H_%M_%S")
    filename = "GD-77_screengrab-" + timestampStr
    
    # Command line argument parsing
    try:                                
        opts, args = getopt.getopt(sys.argv[1:], "hd:s:o:f:b:", ["help", "device=", "scale=", "output=", "foreground=", "background="])
    except getopt.GetoptError as err:
        print(str(err))
        usage()
        sys.exit(2)
    
    for opt, arg in opts:
        if opt in ("-h", "--help"):
            usage()
            sys.exit(2)
        elif opt in ("-d", "--device"):
            serialDev = arg
        elif opt in ("-s", "--scale"):
            scale = int(arg)
            if (scale < 1):
                scale = 1
        elif opt in ("-o", "--output"):
            filename = arg
        elif opt in ("-f", "--foreground"):
            # Check color validity
            try:
                rgb = ImageColor.getrgb(arg)
            except ValueError as err:
                print("Color '" + arg +"' is invalid")
                sys.exit(-3)
                
            foreground = arg
        elif opt in ("-b", "--background"):
            # Check color validity
            try:
                rgb = ImageColor.getrgb(arg)
            except ValueError as err:
                print("Color '" + arg +"' is invalid")
                sys.exit(-3)

            background = arg
        else:
            assert False, "Unhandled option"

    # Initialize Serial Port
    ser = serial.Serial()
    ser.port = serialDev
    ser.baudrate = 115200
    ser.bytesize = serial.EIGHTBITS
    ser.parity = serial.PARITY_NONE
    ser.stopbits = serial.STOPBITS_ONE
    ser.timeout = 1000.0
    #ser.xonxoff = 0
    #ser.rtscts = 0
    ser.write_timeout = 1000.0
    
    if (os.path.isfile(filename + ".png") == True):
        # Add timestamp to avoid file override
        print("WARNING: the file '" + filename + ".png' already exists. Image will be saved in '" + filename + "-" + timestampStr + ".png'.")
        filename += "-" + timestampStr
    
    try:
        ser.open()
    except serial.SerialException as err:
        print(str(err))
        sys.exit(1)
    
    print("Save screen image:")
    if (sendAndReceiveCommand(ser) == True):
        saveImage(filename, scale, foreground, background)
        print("Done.")
    else:
        print("Failure")
        
    if (ser.is_open):
        ser.close()


###
# Calling main function
###
main()
sys.exit(0)
