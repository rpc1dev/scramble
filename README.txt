scramble v3.x - Pioneer DVR-10x firmware scrambling/unscrambling utility 

To unscramble firmware (supposing the firmware is "firmware.bin" and the key "key.bin")
   ./scramble -u -k key.bin firmware.bin unscrambled.bin

To scramble a firmware (supposing the firmware is "firmware.bin" and the key is "key.bin")
   ./scramble -k key.bin firmware.bin scrambled.bin

Options:
  -v 		: verbose mode
  -u		: force unscrambling
  -s		: force scrambling
  -k <key> 	: force use of unscrambling key from file 'key'
  -c        : force the use of checksum values from 'check'
  -t[0-3]	: force type of firmware (in case autodetection fails)
                  0 = DVR-103
                  1 = DVR-104
                  2 = DVR-105
                  3 = DVR-106

The keys for the known Pioneer DVR-10x firmwares are provided with the utility which will
autodetect the one to use.

Please keep quiet about this utility, as the Pioneer guys wouldn't apreciate to know its 
existence ;-)

>NIL:
