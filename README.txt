scramble v3.x - Pioneer DVR-10x firmware scrambling/unscrambling utility 

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
  -t[0-2]	: force type of firmware (in case autodetection fails)
                  0 = DVR-103
                  1 = DVR-104
                  2 = DVR-105

The keys for the Pioneer DVR-103, DVR-104 and DVR-105 firmwares are provided with the
utility which will autodetect the one to use.

Please keep quiet about this utility, as the Pioneer guys wouldn't apreciate to know its 
existence ;-)

>NIL:
