/*
* scramble : scrambler/unscrambler for Pioneer DVR firmwares
* version 3.2
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "getopt.h"

#ifndef u8
#define u8 unsigned char
#endif

#ifndef u16
#define u16 unsigned short
#endif

#ifndef u32
#define u32 unsigned long
#endif

#define	FIRMWARE_SIZE	0x100000	// firmware size
#define	KEY_SIZE		0x000100	// key size
#define	KEY_ADDRESS		0x008000	// key position
#define SCRAMBLED_ID_POS        0x70
#define NB_TYPES		2		// Number of drive models currently known

// Defaults for the DVR-103 and DVR-104
static  char def_keyname[NB_TYPES][16]		= {"key103.bin", "key104.bin"};
static  char scrambled_ID[NB_TYPES][8]		= {"DVR-103", "DVR-104"};
static  u32  unscrambled_ID_pos[NB_TYPES]	= {0x003800, 0x010100};
static  char unscrambled_ID[NB_TYPES][17]	= {"PIONEER DVR-S301", "PIONEER DVD-R104"};
static  u32  header_length[NB_TYPES]		= {0x140, 0x160};
static  u32  checksum_pos[NB_TYPES]			= {0x003900, 0x010200};
static  u32  checksum_blocks[NB_TYPES][6]	= { {0x003902, 0x008000, 0x010000, 0x040000, 0x080000, 0x0F8000},
                                                {0x010202, 0x012000, 0x020000, 0x050000, 0x080000, 0x0F8000} };
int opt_verbose = 0;

int 
checksum (u8* buffer, u8 is_big_endian, int type)
{
	u16 fchk = 0;
	u16 chk = 0;
	u32 i, j;

	if (is_big_endian)
	{
		fchk += ((((u16)buffer[checksum_pos[type]+1])<<8) + ((u16)buffer[checksum_pos[type]]));
		for (j=0;j<6;j+=2)
		{
			for (i=checksum_blocks[type][j]; i<checksum_blocks[type][j+1]; i+=2)
				chk += ((((u16)buffer[i+1])<<8) + ((u16)buffer[i]));
		}
		chk = 0-chk;

		printf("  Computed checksum = 0x%04X\n",chk);
		printf("  Firmware checksum = 0x%04X\n",fchk);
		if (fchk != chk)
		{
			printf("  Computed and firmware checksum differ: updating firmware\n");
			buffer[checksum_pos[type]] = (u8)chk;
			buffer[checksum_pos[type]+1] = (u8)(chk>>8);
		}
	}
	else
	{
		fchk = *((u16*)(buffer+checksum_pos[type]));
		for (j=0;j<6;j+=2)
		{	// Computes the 3 blocks checksum
			for (i=checksum_blocks[type][j]; i<checksum_blocks[type][j+1]; i+=2)
				chk += *((u16*)(buffer+i));
		}
		chk = 0-chk;

		printf("  Computed checksum = 0x%04X\n",chk);
		printf("  Firmware checksum = 0x%04X\n",fchk);
		if (fchk != chk)
		{ 
			printf("  Computed and firmware checksum differ: updating firmware\n");
			*((u16*)(buffer+checksum_pos[type])) = chk;
		}
	}
	return 0;
}


int
scramble (u8* buffer, u8* key)
{
	u32 pos;
	u8  b1, b2, k1, k2;

	for (pos = 0; pos < (KEY_SIZE/2); pos++) 
	{
		b1 = buffer[pos];
		b2 = buffer[KEY_SIZE-1-pos];
		k1 = ~(key[pos]);
		k2 = ~(key[KEY_SIZE-1-pos]);

		b1 ^= k2;
		b2 ^= k1;
		k1 &= 0x07;
		k2 &= 0x07;
		for(;k2!=0;k2--)
			b1=(b1&1)?(b1>>1)|0x80:(b1>>1);
		for(;k1!=0;k1--)
			b2=(b2&1)?(b2>>1)|0x80:(b2>>1);
		buffer[pos] = b2;
		buffer[KEY_SIZE-1-pos] = b1;
	}

	return 0;
}

int
unscramble (u8* buffer, u8* key)
{
	u32 pos;
	u8  b1, b2, k1, k2, t1, t2;

	for (pos = 0; pos < (KEY_SIZE/2); pos++)
	{
		b1 = buffer[pos];
		b2 = buffer[KEY_SIZE-1-pos];
		k1 = ~(key[pos]);
		k2 = ~(key[KEY_SIZE-1-pos]);

		t1 = k1 & 0x07;
		t2 = k2 & 0x07;

		for(;t1!=0;t1--)
			b1=(b1&0x80)?(b1<<1)|1:(b1<<1);
		for(;t2!=0;t2--)
			b2=(b2&0x80)?(b2<<1)|1:(b2<<1);

		b1 ^= k1;
		b2 ^= k2;
		buffer[pos] = b2;
		buffer[KEY_SIZE-1-pos] = b1;
	}

	return 0;
}


int
main (int argc, char *argv[])
{
	unsigned char *saved_header = NULL;
	char keyname[16];
	char sourcename[16];
	char destname[16];
	u8  *buffer, *key;
	u8  is_big_endian  	= 0;
	int firmware_type 	= -1;	// Undefined by default
	int opt_error 		= 0;
	int opt_unscramble 	= -1;	// Unscramble by default
	int opt_force       = 0;
	int opt_keyname 	= 0;
	int i;
	FILE *fd = NULL;

	setbuf (stdin, NULL);

	while ((i = getopt (argc, argv, "usvk:t:")) != -1)
		switch (i)
	{
		case 'v':	// Print verbose messages
			opt_verbose = -1;
			break;
		case 'u':       // Force unscrambling
			opt_unscramble = -1;
			opt_force = -1;
			break;
		case 's':       // Force scrambling
			opt_unscramble = 0;
			opt_force = -1;
			break;
		case 'k':       // Key file to use
			strncpy (keyname, optarg, 15);
			keyname[15] = 0;
			opt_keyname = -1;
			break;
		case 't':	// force firmware type
			firmware_type = atoi(optarg);
			if (firmware_type < 0 || firmware_type >= NB_TYPES)
			{
				printf("Invalid firmware type. Please use a number between 0 and %d with option -t\n",(NB_TYPES-1));
				exit(1);
			}
			break;
		case '?':	// Unknown option
			opt_error++;
			break;
		default:
			printf ("?? getopt returned character code 0%o ??\n", i);
	}

	if ((optind == argc) || opt_error)
	{
		puts ("");
		puts ("scramble - Pioneer DVR firwmare scrambler/unscrambler");
		puts ("usage: scramble [-v] [-u] [-s] [-k keyfile] [-t type] source [dest]");
		puts ("Most features are autodetected, but if you want to force options:");
		puts ("                -u : force unscrambling");
		puts ("                -s : force scrambling");
		puts ("                -k : force use of unscrambling key from file 'keyfile'");
		puts ("                -t : force drive type (0 = DVR-103, 1 = DVR-104)");
		puts ("                -v : verbose");
		puts ("");
		exit (1);
	}

        // Copy firmware name
	strncpy (sourcename, argv[optind], 15);
	sourcename[15] = 0;
	
	// By default, we'll use sourcename as destination
	// and toggle first letter between R/U for scrambled/unscrambled
	strcpy (destname, sourcename);
	switch (sourcename[0])
	{
		case 'u':
			opt_unscramble = (opt_force)?opt_unscramble:0;
			destname[0] = 'r';
			break;
		case 'U':
			opt_unscramble = (opt_force)?opt_unscramble:0;
			destname[0] = 'R';
			break;
		case 'r':
			opt_unscramble = (opt_force)?opt_unscramble:-1;
			destname[0] = 'u';
			break;
		case 'R':
			opt_unscramble = (opt_force)?opt_unscramble:-1;
			destname[0] = 'U';
			break;
		default:
			printf ("Firmware name does not begin by letter 'R' or 'U'\n");
			printf ("You have to force scrambling or unscrambling as option cannot be guessed.\n");
			break;
	} 

	if ( (optind+1) < argc)
	{	// There is a destination name. Let's use it
		strncpy (destname, argv[optind+1], 15);
		destname[15] = 0;
	}

	if ((fd = fopen (sourcename, "rb")) == NULL)
	{
		if (opt_verbose)
			perror ("fopen()");
		fprintf (stderr, "Can't open source file %s\n", sourcename);
		exit (1);
	}

  
	if (opt_verbose)
		printf("Allocating buffers...\n");
	if ( (buffer = (u8*) malloc(FIRMWARE_SIZE)) == NULL)
	{
		printf ("Could not allocate firmware buffer\n");
		fclose (fd);
		exit (1);
	}

	if ( (key = (u8*) malloc(KEY_SIZE)) == NULL)
	{
		printf
			("Could not allocate key buffer\n");
		fclose (fd);
		exit (1);
	}


	// Check endianness
	buffer[0] = 0x00;
	buffer[1] = 0xFF;
	is_big_endian = (u8)(0x00FF&((u16*)buffer)[0]);
	printf("System is %s endian\n", (is_big_endian)?"big":"little");

	// Fills buffer with 0xFF in case the firmware file is smaller than buffer
	memset (buffer, 0xFF, FIRMWARE_SIZE);

	// Read firmware
	if (opt_verbose)
		printf("Reading firmware...\n");
	fread (buffer, 1, FIRMWARE_SIZE, fd);
	fclose (fd);

	// Get firmware type, if not forced
	if (firmware_type == -1)
	{
		printf("Getting firmware type...\n");
		if (opt_unscramble)
		{   // Get the type form a scrambled firmware 
			firmware_type = 0;
			do
			{
				if (memcmp(buffer+SCRAMBLED_ID_POS,scrambled_ID[firmware_type],7) == 0)
				{
					printf("Firmware type is %s (-t%d)\n", scrambled_ID[firmware_type], firmware_type);
					break;
				}
				firmware_type++;
			}
			while (firmware_type < NB_TYPES);
			if (firmware_type == NB_TYPES)
			{
				printf("Could not identify type of firmware...\n");
				exit(1);
			}
		}
		else // Get the type from an unscrambled firmware
		{
			firmware_type = 0;
			do
			{
				if (memcmp(buffer+unscrambled_ID_pos[firmware_type],unscrambled_ID[firmware_type],16) == 0)
				{
					printf("Firmware type is %s (-t%d)\n", scrambled_ID[firmware_type], firmware_type);
					break;
				}
				firmware_type++;
			}
			while (firmware_type < NB_TYPES);
			if (firmware_type == NB_TYPES)
			{
				printf("Could not identify type of firmware...\n");
				exit(1);
			}
		}
	}

        if (header_length[firmware_type])
	{
		if ( (saved_header = malloc(header_length[firmware_type])) == NULL)
		{
			printf("Could not allocate header buffer\n");
			fclose (fd);
			exit (1);
		}
	}

	if (!opt_keyname)
		// If the keyname has not been defined yet, define it from the firmware type
		strcpy (keyname, def_keyname[firmware_type]);

	if (opt_verbose)
		printf("Using key from file %s\n", keyname);
	if ((fd = fopen (keyname, "rb")) == NULL)
	{
		if (opt_verbose)
			perror ("fopen()");
			fprintf (stderr, "Can't open key file %s\n", keyname);
			free (buffer); 
			free (key);
			exit (1);
	}
	if (fread (key, 1, KEY_SIZE, fd) != KEY_SIZE)
	{
		fprintf (stderr, "%s is too small to be a key file!", keyname);
		free (buffer);
		free (key);
		fclose (fd);
		exit (1);
	}
	fclose (fd);

	if (!opt_unscramble)
	{
		printf("Computing checksum...\n");
		checksum(buffer, is_big_endian, firmware_type);
	}

	// Save the unmodified header
    	if (saved_header)
	{
		memcpy(saved_header, buffer, header_length[firmware_type]);
	}

	printf ("%scrambling firmware to file '%s'...\n",(opt_unscramble)?"Uns":"S", destname);
	for (i = 0; i < (FIRMWARE_SIZE / KEY_SIZE); i++)
		if (opt_unscramble)
			unscramble (buffer+i*KEY_SIZE, key);
		else
			scramble (buffer+i*KEY_SIZE, key);
	
	// Copy back the unmodified header
	if (saved_header)
	{
		memcpy(buffer, saved_header, header_length[firmware_type]);
		free(saved_header);
	}

	if (opt_verbose)
		printf ("Writing firmware...\n");

	if ((fd = fopen (destname, "wb")) == NULL)
	{
		if (opt_verbose)
			perror ("fopen()");
		fprintf (stderr, "Can't open output file\n");
		free(key);
		free(buffer);
		exit (1);
	}  
	fwrite (buffer, 1, FIRMWARE_SIZE, fd);
	fclose (fd);
	free(key);
	free(buffer);
	if (opt_verbose)
		printf("All done.");
	return 0;
}
