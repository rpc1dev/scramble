scramble v2.0 - Pioneer DVR-103 and DVR-104 firmware scrambling/unscrambling utility 

To unscramble firmware (supposing the firmware is "firmware.bin" and the key "key.bin")
   ./scramble -u -k key.bin firmware.bin unscrambled.bin

To scramble a firmware (supposing the firmware is "firmware.bin" and the key is "key.bin")
   ./scramble -k key.bin firmware.bin scrambled.bin

Options:
  -v 		: verbose mode
  -u		: unscramble
  -k<filename> 	: key file to use
  -f		: use key from firmware file
  -s		: save key from a complete firmware file (kernel + general)
  -t[0-1]	: force type of firmware (in case autodetection fails)
                  0 = DVR-103
                  1 = DVR-104
    
The keys for the Pioneer DVR-103 and DVR-104 firmwares are provided with the utility 
which will autodetect the one to use.

Please keep quiet about this utility, as the Pioneer guys wouldn't apreciate to know its 
existence ;-)

>NIL:
