/*
* scramble : scrambler/unscrambler for Pioneer DVR firmwares
* version 3.7
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

#define	FIRMWARE_SIZE			0x100000	// GENERAL firmware size
#define	KEY_SIZE				0x000100	// GENERAL key size
#define	KEY_ADDRESS				0x008000	// GENERAL key position
#define SCRAMBLED_ID_POS        0x70
#define NB_TYPES				5			// Number of drive models currently known
#define NB_TYPES_0              3
#define NB_TYPES_1              2

// The new scrambler makes use of these new parameters
typedef struct {
	u32     scrambled_start;
	u32		scrambled_size;
	u32		key_address;
} Scrambler1;

// Defaults parameters
static  char scramblerType[NB_TYPES]        = {0,0,0,1,1};
static  Scrambler1 type1[2][NB_TYPES_1]     = { {{0x00B000, 0x005000, 0x009000},	// KERNEL
												 {0x010000, 0x0C3000, 0x0E0000}},   // GENERAL
												{{0x00B000, 0x005000, 0x009000},	// KERNEL
												 {0x010000, 0x0C3000, 0x0E0000}} }; // GENERAL
static  char def_keyname[NB_TYPES_0][16]	= {"key103.bin", "key104.bin", "key105.bin"};
static  char scrambled_ID[NB_TYPES][8]		= {"DVR-103", "DVR-104", "DVR-105", "DVR-106", "DVR-107"};
static  u32  unscrambled_ID_pos[NB_TYPES]	= {0x003800, 0x010100, 0x010000, 0x010000, 0x010000};
static  char unscrambled_ID[NB_TYPES][17]	= {"PIONEER DVR-S301", "PIONEER DVD-R104", "PIONEER  DVR-105", 
											   "PIONEER  DVR-106", "PIONEER  DVR-107"};
static  u32  header_length[NB_TYPES]		= {0x140, 0x160, 0x160, 0x160, 0x160};
static  u32  checksum_pos[NB_TYPES]			= {0x003900, 0x010200, 0x010010, 0x0D2000, 0x0D2000};
static  u32  checksum_pos_kernel			= 0xB000;
static  u32  checksum_blocks[NB_TYPES][6]	= { {0x003902, 0x008000, 0x010000, 0x040000, 0x080000, 0x0F8000},
                                                {0x010202, 0x012000, 0x020000, 0x050000, 0x080000, 0x0F8000},
												{0x010012, 0x08C000, 0x090000, 0x091000, 0x000000, 0x000000},
												{0x010000, 0x0CBFFC, 0x0D0000, 0x0D1000, 0x000000, 0x000000},
												{0x010000, 0x0CBFFC, 0x0D0000, 0x0D1000, 0x000000, 0x000000} };
static	u32  checksum_blocks_kernel[6]		= {0x00C000, 0x010000, 0x000000, 0x000000, 0x000000, 0x000000};
int opt_verbose = 0;


// The handy ones
u32 readlong(u8* buffer, u32 addr)
{
	return ((((u32)buffer[addr+3])<<24) + (((u32)buffer[addr+2])<<16) +
		(((u32)buffer[addr+1])<<8) + ((u32)buffer[addr]));
}

void writelong(u8* buffer, u32 addr, u32 value)
{
	buffer[addr] = (u8)value;
	buffer[addr+1] = (u8)(value>>8);
	buffer[addr+2] = (u8)(value>>16);
	buffer[addr+3] = (u8)(value>>24);
}

// 103, 104, 105 checksum (general)
int 
checksum (u8* buffer, int type)
{
	u16 fchk = 0;
	u16 chk = 0;
	u32 i, j;

	fchk = ((((u16)buffer[checksum_pos[type]+1])<<8) + ((u16)buffer[checksum_pos[type]]));
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
	return 0;
}

// 106, 107 checksum (general & kernel)
int 
checksum2 (u8* buffer, int type)
{
	u32 fchk = 0;
	u32 chk = 0;
	u32 i, j;

	fchk = readlong(buffer,checksum_pos[type]);

	for (j=0;j<6;j+=2)
	{
		for (i=checksum_blocks[type][j]; i<checksum_blocks[type][j+1]; i+=4)
			chk += readlong(buffer,i);
	}
	chk = 0-chk;

	printf("  Computed checksum = 0x%04X\n",chk);
	printf("  Firmware checksum = 0x%04X\n",fchk);
	if (fchk != chk)
	{
		printf("  Computed and firmware checksum differ: updating firmware\n");
		writelong(buffer, checksum_pos[type], chk);
	}

	return 0;
}


// 106, 107 extra checksum (general)
int 
checksum3 (u8* buffer, int type)
{
	u32 sum = 0;
	u32 pbuf = 0xE0000;
	u32 ptr;

	printf("  Applying general extra checksum\n");


	for (ptr = 0x10000; ptr < 0xD3000; ptr+=4) {
		sum += readlong (buffer, ptr);
		if ((ptr&0x7F) == 0x7C) {
            writelong(buffer, pbuf, sum);
			pbuf += 4;
		}
	}
	
	printf ("  Computed second checksum: 0x%04X\n", sum);
	printf ("  Frimware second checksum: 0x%04X\n", readlong(buffer, pbuf));

	writelong(buffer, pbuf, sum);
	pbuf += 4;

	for (ptr=0xE0000;pbuf<0xEC300;ptr+=4,pbuf+=4)
		writelong (buffer, pbuf, sum-readlong(buffer,ptr));

	return 0;
}

// Rotate left or right in little endian mode
int
rotate(u8* buffer, char rotation, int size)
{
	u8	temp;
	int i,j;
	char rol, ror;

	if (rotation > 0)
	{	// Right rotation
		rol = rotation%8;
		ror = 8-rol;
		for (j=0; j<(rotation/8); j++)
		{
			temp = buffer[0];
			for (i=0; i<size-1; i++)
				buffer[i] = buffer[i+1];
			buffer[size-1] = temp;
		}
		if (rol)
		{
			temp = buffer[0]; 
			for (i=0; i<size-1; i++)
				buffer[i] = (buffer[i]>>rol) | (buffer[i+1]<<ror);
			buffer[size-1] = (buffer[size-1]>>rol) | (temp<<ror);
		}
	}
	if (rotation < 0)
	{	// Left Rotation
		rol = (0-rotation)%8;
		ror = 8-rol;
		for (j=0; j<((0-rotation)/8); j++)
		{
			temp = buffer[size-1];
			for (i=size-1; i>0; i--)
				buffer[i] = buffer[i-1];
			buffer[0] = temp;
		}
		if (rol)
		{
			temp = buffer[size-1]; 
			for (i=size-1; i>0; i--)
				buffer[i] = (buffer[i]<<rol) | (buffer[i-1]>>ror);
			buffer[0] = (buffer[0]<<rol) | (temp>>ror);
		}
	}
	return(rotation);
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
	u32 *saved_header	= NULL;
	char keyname[16];
	char checkname[16];
	char sourcename[16];
	char destname[16];
	u8  *buffer, *key;
	int is_general		= 1;
	int firmware_type 	= -1;	// Undefined by default
	int opt_error 		= 0;
	int opt_unscramble 	= -1;	// Unscramble by default
	int opt_force       = 0;
	int opt_keyname 	= 0;
	int opt_check		= 0;
	u32 i;
	FILE *fd = NULL;

	setbuf (stdin, NULL);

	while ((i = getopt (argc, argv, "usvc:k:t:")) != -1)
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
		case 'c':       // Check file to use
			strncpy (checkname, optarg, 15);
			checkname[15] = 0;
			opt_check = -1;
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
		puts ("scramble v3.7 - Pioneer DVR firwmare scrambler/unscrambler");
		puts ("usage: scramble [-v] [-u] [-s] [-k key] [-c check] [-t type] source [dest]");
		puts ("Most features are autodetected, but if you want to force options:");
		puts ("                -u : force unscrambling");
		puts ("                -s : force scrambling");
		puts ("                -k : force use of unscrambling key from file 'key'");
		puts ("                -c : force the use of checksum values from 'check'");
		puts ("                -t : force drive type (0=DVR-103 -> 4=DVR-107)");
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

	// Fills buffer with 0xFF in case the firmware file is smaller than buffer
	memset (buffer, 0xFF, FIRMWARE_SIZE);

	// Read firmware
	if (opt_verbose)
		printf("Reading firmware...\n");
	// We don't care about scrambled kernel size for fread
	fread (buffer, 1, FIRMWARE_SIZE, fd);
	fclose (fd);

	// Check whether we have a Kernel or General firmware
	if (buffer[0x110] == 'K')
		is_general = 0;
	printf("This is a %s firmware.\n",(is_general==0)?"KERNEL":"GENERAL");

	// Get firmware type, if not forced
	if (firmware_type == -1)
	{
		printf("Getting firmware type...\n");

		if (opt_unscramble || (!is_general))
		{   // Get the type from a scrambled or Kernel firmware 
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
		else // Get the type from an unscrambled GENERAL firmware
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

	if (opt_check)
	{	// Get checksum addresses from file
		if (opt_verbose)
			printf("Reading checksum addresses from file %s\n", checkname);
		if ((fd = fopen (checkname, "rb")) == NULL)
		{
			if (opt_verbose)
				perror ("fopen()");
			fprintf (stderr, "Can't open check file %s\n", checkname);
			free (buffer); 
			free (key);
			exit (1);
		}
		if (fscanf (fd, "%x", &checksum_pos[firmware_type]) != 1)
		{
			fprintf (stderr, "Could not read checksum address from %s\n", checkname);
			free (buffer); 
			free (key);
			exit (1);
		}
		for (i=0;i<6;i++)
			if (fscanf (fd, "%x", &checksum_blocks[firmware_type][i]) !=1 )
			{
				fprintf (stderr, "Could not read all block boundaries from %s\n", checkname);
				free (buffer); 
				free (key);
				exit (1);
			}
		fclose (fd);
	}

	switch (scramblerType[firmware_type])
	{

	case 1:	// Scrambler type 1 (106, 107)
		if (opt_unscramble)
		{	// Unscramble
		Scrambler1* t1 = &(type1[is_general][firmware_type - NB_TYPES_0]);
			for (i = 0; i<t1->scrambled_size; i+=4)
			{
				if ((i != 0x8000) && (i != 0x70000))
				// Pioneer exception (thx to TheKeyMaker for this one)
					*(u32*)(buffer+t1->scrambled_start+i) ^= *(u32*)(buffer+t1->key_address+i%(t1->scrambled_size>>4));
				rotate(buffer+t1->scrambled_start+i, buffer[t1->key_address+i%(t1->scrambled_size>>4)]&0x1F, 4);
			}
		}
		else
		{	// Scramble
		Scrambler1* t1 = &(type1[is_general][firmware_type - NB_TYPES_0]);

			if (!is_general)
			{	// This is a bit ugly but it'll do for now. We just hijack the general 
				// values to replace them with the kernel ones
				checksum_pos[firmware_type] = checksum_pos_kernel;
				for (i=0; i<6; i++)
					checksum_blocks[firmware_type][i] = checksum_blocks_kernel[i];
			}

			// Compute Checksum
			printf("Computing checksum...\n");
			checksum2(buffer, firmware_type);

			// Extra checksum for the 106, 107
			if (is_general)
				checksum3(buffer, firmware_type);
		
			for (i = 0; i<t1->scrambled_size; i+=4)
			{
				rotate(buffer+t1->scrambled_start+i, 0-buffer[t1->key_address+i%(t1->scrambled_size>>4)]&0x1F, 4);
				if ((i != 0x8000) && (i != 0x70000))
					*(u32*)(buffer+t1->scrambled_start+i) ^= *(u32*)(buffer+t1->key_address + i%(t1->scrambled_size>>4));
			}


		}
		printf ("%scrambling firmware to file '%s'...\n",(opt_unscramble)?"Uns":"S", destname);
		break;

	case 0: // Scrambler type 0 (103, 104, 105)
		if (is_general)
		{	// No need to do anything on kernel files for scrambler type 0
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
				checksum(buffer, firmware_type);
			}

			// Save the unmodified header
    		if (saved_header)
				memcpy(saved_header, buffer, header_length[firmware_type]);

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
		}
		else
		{
			printf("Kernel file is not encrypted. Aborting.\n");
			exit(0);
		}
		break;

	default:
		printf("Unknown Scrambler type...\n");
		break;
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
	fwrite (buffer, 1, ((scramblerType[firmware_type]) && (!is_general))?
		0x10000:FIRMWARE_SIZE, fd);
	fclose (fd);
	free(key);
	free(buffer);
	if (opt_verbose)
		printf("All done.");
	return 0;
}
