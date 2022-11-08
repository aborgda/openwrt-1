/*
 * Copyright (C) 2017 Weijie Gao <hackpascal@gmail.com>
 *
 * Based on:
 *   Tool to convert ELF image to be the AP downloadable binary.
 *   Copyright (C) 2009 David Hsu <davidhsu@realtek.com.tw>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#include <zlib.h>
#include <time.h>

#include <sys/stat.h>
#include <arpa/inet.h>

#include "intelbras_footer.h"

#define ARRAY_SIZE(a) (sizeof (a) / sizeof (a[0]))

#define SIGNATURE_LEN		4

#define FAKE_ROOTFS_SUPER_SIZE	640
#define FAKE_ROOTFS_CHKSUM_SIZE	2
#define FAKE_ROOTFS_SIZE	(FAKE_ROOTFS_SUPER_SIZE + FAKE_ROOTFS_CHKSUM_SIZE)

#define FAKE_ROOTFS_MAGIC	"hsqs"
#define FAKE_ROOTFS_IDENT	"FAKE"

#define FAKE_ROOTFS_ALIGNMENT	4096

#define FAKE_ROOTFS_MAGIC_SIZE	4
#define FAKE_ROOTFS_IDENT_SIZE	4
#define FAKE_ROOTFS_SIZE_OFS	16

// UImage
#define UIMAGE_MAGIC    			0x27051956			// UImage Magic Number
#define UIMAGE_EMPTY_SIZE    		16					// UImage Empty Length
#define UIMAGE_LOAD_ADDRESS			0x80000000
#define UIMAGE_ENTRY_ADDRESS		0x80457170			// UImage entry point address for AC1200RF
#define UIMAGE_OPERATING_SYSTEM		0x05
#define UIMAGE_CPU_ARCHITECTURE		0x05
#define UIMAGE_IMAGE_TYPE			0x02
#define UIMAGE_COMPRESSION_TYPE		0x03
#define UIMAGE_UNKNOWN_1			0x02000000
#define UIMAGE_BURN_ADDRESS			0x00030027

// Intelbras header
#define INTBR_FIRM_TEXT 					"itbs"
#define INTBR_FIRM_TEXT_SIZE 				4
#define INTBR_FIRMWARE_TYPE					0x0001
#define INTBR_FIRMWARE_VERSION				"1.21.9"		// Might not be needed
#define INTBR_FIRMWARE_VERSION_SIZE			32
#define INTBR_GF1200_MODEL					"GF1200"
#define INTBR_W51200F_MODEL					"W5-1200F"
#define INTBR_FIRMWARE_MODEL_SIZE			20

// Intelbras new header
#define NEW_INTBR_MAGIC_NUMBER 				0x56190527
#define NEW_INTBR_UKNOWN_SIZE_1				28
#define NEW_INTBR_FIRMWARE_TYPE				0x00000002
#define NEW_INTBR_RG1200AC_PROD_ID			0x31000300
#define NEW_INTBR_PADDING_SIZE 				14
#define NEW_INTBR_FIRM_TEXT 				"NITBF"



// Models: AC1200RF
struct modified_uimage_header {
	uint32_t    magic_number;					/* Image Header Magic Number    */
	uint32_t    header_crc;						/* Image Header CRC Checksum    */
	uint32_t    timestamp;						/* Image Creation Timestamp     */
	uint32_t    data_size;						/* Image Data Size              */
	uint32_t    load_addr;						/* Data     Load  Address       */
	uint32_t    entry_addr;						/* Entry Point Address          */
	uint32_t    data_crc;						/* Image Data CRC Checksum      */
	uint8_t     os;								/* Operating System             */
	uint8_t     arch;							/* CPU architecture             */
	uint8_t     image_type;						/* Image Type                   */
	uint8_t     compression_type;				/* Compression Type             */
	uint32_t    unknown1;						/* Unknown 1                    */
	uint32_t    burn_addres;					/* Firmware burn address        */
	uint32_t    firmware_size;					/* Payload + Header Size        */
	uint8_t 	empty[UIMAGE_EMPTY_SIZE];		/* Zeroed portion of the header */
	uint32_t    fs_offset;         				/* Offset of the file system
													partition from the start of
													the header                  */
};


// Models: GF 1200, W5 1200F V1
struct intbr_img_header {

	// itbs
	unsigned char magic_text[INTBR_FIRM_TEXT_SIZE];

	// Firmware type = 0x01 0x00
	uint16_t firmware_type;

	// Firmware version
	unsigned char firmware_version[INTBR_FIRMWARE_VERSION_SIZE];

	// Model
	unsigned char firmware_model[INTBR_FIRMWARE_MODEL_SIZE];

	// Checksum
	uint16_t firmware_checksum;

};


// Models: RG 1200 AC
struct new_intbr_img_header {

	// 0x27 0x05 0x19 0x56
	uint32_t magic_number;

	// Unkown data
	unsigned char unknown_data_1[NEW_INTBR_UKNOWN_SIZE_1];

	// Firmware type
	uint32_t firmware_type;

	// Product ID
	uint32_t product_id;

	// Unkown data 2
	uint32_t unknown_data_2;

	// 0x00 * 14 times
	unsigned char padding[NEW_INTBR_PADDING_SIZE];

	// New firmware identifier text
	// Includes /n or /0
	char firmware_id_text[sizeof(NEW_INTBR_FIRM_TEXT)];

};

struct img_header
{
    unsigned char signature[SIGNATURE_LEN];
    unsigned int start_addr;
    unsigned int burn_addr;
    unsigned int len;
};

enum data_type
{
	DATA_BOOT = 0,
	DATA_KERNEL,
	DATA_ROOTFS,
	DATA_FIRMWARE
};

enum cpu_type
{
	CPU_ANY = 0,
	CPU_RTL8196B,
	CPU_NEW,
	CPU_OTHERS
};

struct signature
{
	enum data_type	type;
	enum cpu_type	cpu;
	const char	*sig;
};

const static struct signature sig_known[] =
{
	{
		.type	= DATA_BOOT,
		.cpu	= CPU_ANY,
		.sig	= "boot"
	},
	{
		.type	= DATA_KERNEL,
		.cpu	= CPU_RTL8196B,
		.sig	= "cs6b"
	},
	{
		.type	= DATA_KERNEL,
		.cpu	= CPU_NEW,
		.sig	= "cs6c"
	},
	{
		.type	= DATA_KERNEL,
		.cpu	= CPU_OTHERS,
		.sig	= "csys"
	},
	{
		.type	= DATA_ROOTFS,
		.cpu	= CPU_RTL8196B,
		.sig	= "r6br"
	},
	{
		.type	= DATA_ROOTFS,
		.cpu	= CPU_NEW,
		.sig	= "r6cr"
	},
	{
		.type	= DATA_ROOTFS,
		.cpu	= CPU_OTHERS,
		.sig	= "root"
	},
	{
		.type	= DATA_FIRMWARE,
		.cpu	= CPU_RTL8196B,
		.sig	= "cr6b"
	},
	{
		.type	= DATA_FIRMWARE,
		.cpu	= CPU_NEW,
		.sig	= "cr6c"
	},
	{
		.type	= DATA_FIRMWARE,
		.cpu	= CPU_OTHERS,
		.sig	= "csro"
	},
};

static const char jffs2_marker[] = {0xde, 0xad, 0xc0, 0xde};

static char *progname;
static char *ifname;
static char *ofname;
static char *model;
static const char *sig = NULL;
static uint32_t start_addr = 0;
static uint32_t burn_addr = 0;
static uint32_t align_size = 0;
static uint32_t append_fake_rootfs = 0;
static uint32_t append_jffs2_marker = 0;
static uint32_t insert_chksum_after_format = 0;
static uint32_t insert_intelbras_header = 0;
static uint32_t insert_new_intelbras_header = 0;
static uint32_t insert_uimage_header = 0;
static uint32_t insert_intelbras_footer = 0;
static uint32_t insert_chksum_sysupgrade_format = 0;
static enum data_type sig_type = (enum data_type) -1;
static enum cpu_type sig_cpu = (enum cpu_type) -1;

static inline uint32_t size_aligned(uint32_t size, uint32_t align)
{
	uint32_t remainder;

	if (align > 1)
	{
		remainder = size % align;
		if (remainder)
			size += align - remainder;
	}

	return size;
}

static void usage(int status)
{
	fprintf(stderr, "Usage: %s [OPTIONS...]\n", progname);
	fprintf(stderr,
		"\n"
		"Options:\n"
		"  -i <file>       use <file> as the input payload data\n"
		"  -o <file>       write output to the file <file>\n"
		"  -e <ep>         use <ep> as the start/load address\n"
		"  -b <ba>         use <ba> as the flash address where the payload data to be burned\n"
		"  -s <sig>        use customized signature <sig> (4 characters, overrides -t and -c)\n"
		"  -t <type>       set the type of the payload data (boot/kernel/rootfs/fw are allowed)\n"
		"  -c <cpu>        set the CPU type of pre-defined signatures (any/rtl8196b/new/other are allowed)\n"
		"  -a <size>       set the size of output file to be aligned\n"
		"  -T              Insert a checksum considering a total image length 16 bytes shorter (Tenda bootloader)\n"
		"  -f              append fake rootfs only, image header will not be created, requires -a specified for block size\n"
		"  -j              append jffs2 end marker image only, conflicts with -f, requires -b to be specified for fw burn address and -a for block size\n"
		"  -g              Add the Intelbras header for W5 1200F V1 or GF 1200"
		"  -I              Add the new Intelbras header\n"
		"  -h              show this screen\n"
	);

	exit(status);
}

static enum cpu_type resolve_cpu_type(const char *str)
{
	if (!strcasecmp(str, "any"))
		return CPU_ANY;

	if (!strcasecmp(str, "rtl8196b"))
		return CPU_RTL8196B;

	if (!strcasecmp(str, "new"))
		return CPU_NEW;

	if (!strcasecmp(str, "other"))
		return CPU_OTHERS;

	fprintf(stderr, "Error: invalid CPU type '%s'\n", str);
	exit(EINVAL);

	return (enum cpu_type) -1;
}

static enum data_type resolve_payload_type(const char *str)
{
	if (!strcasecmp(str, "boot"))
		return DATA_BOOT;

	if (!strcasecmp(str, "kernel"))
		return DATA_KERNEL;

	if (!strcasecmp(str, "rootfs"))
		return DATA_ROOTFS;

	if (!strcasecmp(str, "fw"))
		return DATA_FIRMWARE;

	fprintf(stderr, "Error: invalid payload data type '%s'\n", str);
	exit(EINVAL);

	return (enum data_type) -1;
}

static const char *find_signature(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sig_known); i++)
	{
		if ((sig_known[i].cpu == CPU_ANY || sig_known[i].cpu == sig_cpu) &&
		    (sig_known[i].type == sig_type))
			return sig_known[i].sig;
	}

	return NULL;
}

static int file_exists(const char *fname)
{
	struct stat st;
	int res;

	res = stat(fname, &st);
	if (res)
	{
		fprintf(stderr, "Error: stat failed on %s\n", fname);
		return 0;
	}

	return 1;
}

static int check_options(void)
{
	if (!ifname)
	{
		fprintf(stderr, "Error: please specify a valid payload file\n");
		return -EINVAL;
	}

	if (!file_exists(ifname))
		return -EINVAL;

	if (!ofname)
	{
		fprintf(stderr, "Error: please specify a valid output file name\n");
		return -EINVAL;
	}

	if (align_size % 2)
	{
		fprintf(stderr, "Error: size of alignment must be multiple of 2\n");
		return -EINVAL;
	}

	if (append_fake_rootfs && append_jffs2_marker)
	{
		fprintf(stderr, "Error: -j conflicts with -f\n");
		return -EINVAL;
	}

	if (append_fake_rootfs)
		return 0;

	if (!sig)
		sig = find_signature();

	if (!sig)
	{
		fprintf(stderr, "Error: unable to find proper signature, please use -s or -t and -c to specify\n");
		return -EINVAL;
	}

	if (strlen(sig) != 4)
	{
		fprintf(stderr, "Error: size of signature must be 4\n");
		return -EINVAL;
	}

	return 0;
}

static uint16_t calculate_checksum(const uint8_t *buf, int len)
{
	int i;
	uint16_t sum = 0, tmp;

	for (i = 0; i < len; i += 2)
	{
		tmp = (((uint16_t) buf[i]) << 8) | buf[i + 1];
		sum += tmp;
	}

	if ( len % 2 )
	{
		tmp = ((uint16_t) buf[len - 1]) << 8;
		sum += tmp;
	}

	return htons(~sum + 1);
}

/*
 * Write the uImage header
 */
static int image_append_uimage_header()
{
	struct modified_uimage_header header;
	FILE *file;
	char *aux_buffer;
	int file_size, buffer_size, crc;
	short checksum = 0;

	// Open input file
	file = fopen(ifname, "rb");
	if (!file)
	{
		fprintf(stderr, "Error: unable to open payload file %s\n", ifname);
		return -1;
	}


	// Get file size
	fseek(file, 0, SEEK_END);
	file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	if (!file_size)
	{
		fclose(file);
		fprintf(stderr, "Error: payload file %s is empty\n", ifname);
		return -1;
	}


	// Set the header
	memset(&header, 0x00, sizeof(header));
	header.magic_number 	= htonl(UIMAGE_MAGIC);
	header.timestamp 		= htonl(time(NULL));
	header.data_size 		= htonl(file_size);
	header.load_addr 		= htonl(UIMAGE_LOAD_ADDRESS);
	header.entry_addr 		= htonl(UIMAGE_ENTRY_ADDRESS);
	header.os 				= UIMAGE_OPERATING_SYSTEM;
	header.arch 			= UIMAGE_CPU_ARCHITECTURE;
	header.image_type 		= UIMAGE_IMAGE_TYPE;
	header.compression_type = UIMAGE_COMPRESSION_TYPE;
	header.unknown1			= htonl(UIMAGE_UNKNOWN_1);
	header.burn_addres		= htonl(UIMAGE_BURN_ADDRESS);
	header.firmware_size	= htonl(sizeof(header) + file_size);


	// Allocate the buffer
	buffer_size = sizeof(header) + file_size;
	
	aux_buffer = (char *) calloc(buffer_size, 1);
	if (!aux_buffer)
	{
		fclose(file);
		fprintf(stderr, "Error: unable to allocate memory\n");
		return -1;
	}
	
	// Write the payload
	if (fread(aux_buffer + sizeof(header), 1, file_size, file) != file_size)
	{
		fclose(file);
		fprintf(stderr, "Error: failed to read payload file\n");
		return -1;
	}

	fclose(file);


	// Calculate Payload checksum
	crc = crc32(0L, Z_NULL, 0);
	crc = crc32(crc, &aux_buffer[sizeof(header)], file_size);
	header.data_crc = htonl(crc);

	// Calculate header checksum
	crc = crc32(0L, Z_NULL, 0);
	crc = crc32(crc, (void *) &header, sizeof(header));
	header.header_crc = htonl(crc);

	// Write the header
	memcpy(aux_buffer, (const unsigned char *) &header, sizeof(header));

	
	// Open output file
	file = fopen(ofname, "wb");
	if (!file)
	{
		fprintf(stderr, "Error: unable to open output file %s\n", ofname);
		return -1;
	}

	// Write to output
	if (fwrite(aux_buffer, 1, buffer_size, file) != buffer_size)
	{
		fclose(file);
		fprintf(stderr, "Error: failed to write output file\n");
		return -1;
	}

	fclose(file);

	printf(
		"UImage header appended:\n"
		"    Filename:\t\t\t%s\n"
		"    Image size (oct.):\t\t%d bytes\n",
		ofname,
		buffer_size
	);
	
	return 0;
}

// Function extracted from GF 1200 1.10.6 headerless firmware
short calculate_intelbras_checksum(char* firmware, unsigned int firmware_size, short checksum_start) {
	unsigned short firmware_checksum;
	int firmware_offset;
	short checksum = checksum_start;

	for (firmware_offset = 0; firmware_offset < (int)firmware_size / 2 << 1; firmware_offset = firmware_offset + 2) {
		checksum = (*(unsigned short *)(firmware + firmware_offset) >> 8 |
					*(unsigned short *)(firmware + firmware_offset) << 8) + checksum;
	}

	if ((firmware_size & 1) != 0) {
		firmware_checksum = (*(char *)(firmware + (firmware_size - 1)));
		checksum = (firmware_checksum >> 8 | firmware_checksum << 8) + checksum;
	}

	return checksum;
} 

static int image_append_intelbras_header(void)
{
	struct intbr_img_header header;
	FILE *file;
	char *aux_buffer;
	int file_size, buffer_size;
	short checksum = 0;

	// Open input file
	file = fopen(ifname, "rb");
	if (!file)
	{
		fprintf(stderr, "Error: unable to open payload file %s\n", ifname);
		return -1;
	}


	// Get file size
	fseek(file, 0, SEEK_END);
	file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	if (!file_size)
	{
		fclose(file);
		fprintf(stderr, "Error: payload file %s is empty\n", ifname);
		return -1;
	}


	// Set the header
	memset(&header, 0x00, sizeof(header));
	memcpy(&header.magic_text, INTBR_FIRM_TEXT, INTBR_FIRM_TEXT_SIZE);
	header.firmware_type = INTBR_FIRMWARE_TYPE;
	memcpy(&header.firmware_version, INTBR_FIRMWARE_VERSION, sizeof(INTBR_FIRMWARE_VERSION));


	// GF 1200
	if (!memcmp(model, INTBR_GF1200_MODEL, sizeof(INTBR_GF1200_MODEL)))
		memcpy(&header.firmware_model, INTBR_GF1200_MODEL, sizeof(INTBR_GF1200_MODEL));

	// W5 1200F V1
	else if (!memcmp(model, INTBR_W51200F_MODEL, sizeof(INTBR_W51200F_MODEL)))
		memcpy(&header.firmware_model, INTBR_W51200F_MODEL, sizeof(INTBR_W51200F_MODEL));

	// Invalid model name
	else {
		fclose(file);
		fprintf(stderr, "Error: unknown model: %s\n", model);
		return -1;
	}

	// Allocate the buffer
	buffer_size = sizeof(header) + file_size;
	
	aux_buffer = (char *) calloc(buffer_size, 1);
	if (!aux_buffer)
	{
		fclose(file);
		fprintf(stderr, "Error: unable to allocate memory\n");
		return -1;
	}
	
	// Write the header in the buffer + the payload
	memcpy(aux_buffer, &header, sizeof(header));
	if (fread(aux_buffer + sizeof(header), 1, file_size, file) != file_size)
	{
		fclose(file);
		fprintf(stderr, "Error: failed to read payload file\n");
		return -1;
	}

	fclose(file);

	// Get the checksum of the payload
	checksum = calculate_intelbras_checksum(&aux_buffer[sizeof(header)], file_size, 0);
	// Assign the checksum to the header in the buffer
	aux_buffer[59] = (checksum >> 8) & 0xff;
	aux_buffer[58] = checksum & 0xff;
	
	// Open output file
	file = fopen(ofname, "wb");
	if (!file)
	{
		fprintf(stderr, "Error: unable to open output file %s\n", ofname);
		return -1;
	}

	// Write to output
	if (fwrite(aux_buffer, 1, buffer_size, file) != buffer_size)
	{
		fclose(file);
		fprintf(stderr, "Error: failed to write output file\n");
		return -1;
	}

	fclose(file);

	printf(
		"Intelbras header appended:\n"
		"    Filename:\t\t\t%s\n"
		"    Image size (oct.):\t\t%d bytes\n",
		ofname,
		buffer_size
	);
	
	return 0;
}

static int image_append_new_intelbras_header(void)
{
	struct new_intbr_img_header header;
	FILE *file;
	char *aux_buffer;
	int file_size, buffer_size;

	// Set the new header
	header.magic_number = NEW_INTBR_MAGIC_NUMBER;
	memset(&header.unknown_data_1, 0x00, NEW_INTBR_UKNOWN_SIZE_1);
	header.firmware_type = NEW_INTBR_FIRMWARE_TYPE;
	header.product_id = NEW_INTBR_RG1200AC_PROD_ID;
	header.unknown_data_2 = 0x00000000;
	memset(&header.padding, 0x00, NEW_INTBR_PADDING_SIZE);
	memcpy(&header.firmware_id_text, NEW_INTBR_FIRM_TEXT, sizeof(NEW_INTBR_FIRM_TEXT));
	
	// Open input file
	file = fopen(ifname, "rb");
	if (!file)
	{
		fprintf(stderr, "Error: unable to open payload file %s\n", ifname);
		return -1;
	}

	fseek(file, 0, SEEK_END);
	file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	if (!file_size)
	{
		fclose(file);
		fprintf(stderr, "Error: payload file %s is empty\n", ifname);
		return -1;
	}


	// Allocate the buffer
	buffer_size = sizeof(header) + file_size;

	aux_buffer = (char *) calloc(buffer_size, 1);
	if (!aux_buffer)
	{
		fclose(file);
		fprintf(stderr, "Error: unable to allocate memory\n");
		return -1;
	}
	
	// Write the new header in the buffer + the payload
	memcpy(aux_buffer, &header, sizeof(header));

	if (fread(aux_buffer + sizeof(header), 1, file_size, file) != file_size)
	{
		fclose(file);
		fprintf(stderr, "Error: failed to read payload file\n");
		return -1;
	}

	fclose(file);
	
	// Open output file
	file = fopen(ofname, "wb");
	if (!file)
	{
		fprintf(stderr, "Error: unable to open output file %s\n", ofname);
		return -1;
	}

	// Write to output
	if (fwrite(aux_buffer, 1, buffer_size, file) != buffer_size)
	{
		fclose(file);
		fprintf(stderr, "Error: failed to write output file\n");
		return -1;
	}

	fclose(file);

	printf(
		"New Intelbras header appended:\n"
		"    Filename:\t\t\t%s\n"
		"    Image size (oct.):\t\t%d bytes\n",
		ofname,
		buffer_size
	);


	return 0;
}


static int image_append_intelbras_footer(void)
{
	FILE *file;
	char *aux_buffer;
	int file_size, buffer_size;


	// Open input file
	file = fopen(ifname, "rb");
	if (!file)
	{
		fprintf(stderr, "Error: unable to open payload file %s\n", ifname);
		return -1;
	}

	fseek(file, 0, SEEK_END);
	file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	if (!file_size)
	{
		fclose(file);
		fprintf(stderr, "Error: payload file %s is empty\n", ifname);
		return -1;
	}
	

	// Allocate the buffer
	buffer_size = sizeof(intelbras_footer) + file_size + (16 - (file_size % 16));

	aux_buffer = (char *) calloc(buffer_size, 1);
	if (!aux_buffer)
	{
		fclose(file);
		fprintf(stderr, "Error: unable to allocate memory\n");
		return -1;
	}


	// Write the image file
	if (fread(aux_buffer, 1, file_size, file) != file_size)
	{
		fclose(file);
		fprintf(stderr, "Error: failed to read payload file\n");
		return -1;
	}

	fclose(file);

	// Write the footer, make the sure the footer is aligned
	memcpy(aux_buffer + file_size + (16 - (file_size % 16)), &intelbras_footer, sizeof(intelbras_footer));
	
	// Open output file
	file = fopen(ofname, "wb");
	if (!file)
	{
		fprintf(stderr, "Error: unable to open output file %s\n", ofname);
		return -1;
	}

	// Write to output
	if (fwrite(aux_buffer, 1, buffer_size, file) != buffer_size)
	{
		fclose(file);
		fprintf(stderr, "Error: failed to write output file\n");
		return -1;
	}

	fclose(file);

	printf(
		"Intelbras footer appended:\n"
		"    Filename:\t\t\t%s\n"
		"    Image size (oct.):\t\t%d bytes\n",
		ofname,
		buffer_size
	);
	return 0;
}


static int image_append_fake_rootfs(void)
{
	FILE *f;
	char *buf, *fake_rootfs;
	int fsize, fsize_aligned, bufsize;
	int fake_rootfs_start, fake_rootfs_size, fake_rootfs_size_be;
	uint16_t chksum;

	fake_rootfs_size = size_aligned(FAKE_ROOTFS_SIZE, sizeof (uint32_t));
	fake_rootfs_size_be = htonl(fake_rootfs_size);
	fake_rootfs = (char *) calloc(fake_rootfs_size, 1);
	if (!fake_rootfs)
	{
		fprintf(stderr, "Error: unable to allocate memory for fake rootfs\n");
		return -1;
	}

	memcpy(fake_rootfs, FAKE_ROOTFS_MAGIC, sizeof (FAKE_ROOTFS_MAGIC));
	memcpy(fake_rootfs + FAKE_ROOTFS_MAGIC_SIZE, FAKE_ROOTFS_IDENT, sizeof (FAKE_ROOTFS_IDENT));
	memcpy(fake_rootfs + FAKE_ROOTFS_SIZE_OFS, &fake_rootfs_size_be, sizeof (fake_rootfs_size_be));

	chksum = calculate_checksum((uint8_t *) fake_rootfs, FAKE_ROOTFS_SIZE - FAKE_ROOTFS_CHKSUM_SIZE);
	memcpy(fake_rootfs + FAKE_ROOTFS_SIZE - FAKE_ROOTFS_CHKSUM_SIZE, &chksum, sizeof (chksum));

	f = fopen(ifname, "rb");
	if (!f)
	{
		fprintf(stderr, "Error: unable to open payload file %s\n", ifname);
		return -1;
	}

	fseek(f, 0, SEEK_END);
	fsize = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (!fsize)
	{
		fclose(f);
		fprintf(stderr, "Error: payload file %s is empty\n", ifname);
		return -1;
	}

	if (align_size < 16)
		align_size = FAKE_ROOTFS_ALIGNMENT;

	fsize_aligned = size_aligned(fsize, align_size) - sizeof (struct img_header);

	bufsize = fsize_aligned + fake_rootfs_size;

	buf = (char *) calloc(bufsize, 1);
	if (!buf)
	{
		fclose(f);
		fprintf(stderr, "Error: unable to allocate memory\n");
		return -1;
	}

	if (fread(buf, 1, fsize, f) != fsize)
	{
		fclose(f);
		fprintf(stderr, "Error: failed to read payload file\n");
		return -1;
	}

	fclose(f);

	memcpy(buf + fsize_aligned, fake_rootfs, fake_rootfs_size);

	f = fopen(ofname, "wb");
	if (!f)
	{
		fprintf(stderr, "Error: unable to open output file %s\n", ofname);
		return -1;
	}

	if (fwrite(buf, 1, bufsize, f) != bufsize)
	{
		fclose(f);
		fprintf(stderr, "Error: failed to write output file\n");
		return -1;
	}

	fclose(f);

	printf(
		"Fake rootfs appended:\n"
		"    Filename:\t\t\t%s\n"
		"    Fake rootfs offset:\t\t0x%08x\n"
		"    Fake rootfs size (oct.):\t%d bytes\n"
		"    Image size (oct.):\t\t%d bytes\n"
		"    Fake rootfs Checksum:\t%04x\n",
		ofname,
		fsize_aligned,
		fake_rootfs_size,
		bufsize,
		chksum
	);

	return 0;
}

static int image_append_jffs2_marker(void)
{
	FILE *f;
	char *buf;
	char jffs2_buf[sizeof (struct img_header) + sizeof (jffs2_marker) + sizeof (uint16_t)];
	int fsize, bufsize;
	struct img_header hdr;
	uint32_t jffs2_addr;
	uint16_t chksum;

	f = fopen(ifname, "rb");
	if (!f)
	{
		fprintf(stderr, "Error: unable to open payload file %s\n", ifname);
		return -1;
	}

	fseek(f, 0, SEEK_END);
	fsize = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (!fsize)
	{
		fclose(f);
		fprintf(stderr, "Error: payload file %s is empty\n", ifname);
		return -1;
	}

	bufsize = size_aligned(fsize + sizeof (jffs2_buf), sizeof (uint32_t));

	buf = (char *) calloc(bufsize, 1);
	if (!buf)
	{
		fclose(f);
		fprintf(stderr, "Error: unable to allocate memory\n");
		return -1;
	}

	if (fread(buf, 1, fsize, f) != fsize)
	{
		fclose(f);
		fprintf(stderr, "Error: failed to read payload file\n");
		return -1;
	}

	fclose(f);

	jffs2_addr = size_aligned(burn_addr + fsize, align_size);

	memcpy(hdr.signature, sig, 4);
	hdr.start_addr = htonl(jffs2_addr);
	hdr.burn_addr = htonl(jffs2_addr);
	hdr.len = htonl(sizeof (jffs2_marker) + sizeof (chksum));

	memcpy(jffs2_buf, &hdr, sizeof (struct img_header));
	memcpy(jffs2_buf + sizeof (struct img_header), jffs2_marker, sizeof (jffs2_marker));

	chksum = calculate_checksum((uint8_t *) jffs2_buf + sizeof (struct img_header), sizeof (jffs2_marker));

	memcpy(jffs2_buf + sizeof (jffs2_buf) - sizeof (chksum), &chksum, sizeof (chksum));

	memcpy(buf + fsize, jffs2_buf, sizeof (jffs2_buf));

	f = fopen(ofname, "wb");
	if (!f)
	{
		fprintf(stderr, "Error: unable to open output file %s\n", ofname);
		return -1;
	}

	if (fwrite(buf, 1, bufsize, f) != bufsize)
	{
		fclose(f);
		fprintf(stderr, "Error: failed to write output file\n");
		return -1;
	}

	fclose(f);

	printf(
		"JFFS2 end marker appended:\n"
		"    Filename:\t\t%s\n"
		"    Signature:\t\t%s\n"
		"    Burn address:\t0x%08x\n"
		"    Data size (oct.):\t%lu bytes\n"
		"    Checksum:\t\t%04x\n",
		ofname,
		sig,
		jffs2_addr,
		sizeof (jffs2_buf),
		chksum
	);

	return 0;
}

static int build_image(void)
{
	FILE *f;
	char *buf, *aux_buffer;
	int fsize, fsize_aligned, bufsize;
	uint16_t chksum, chksum_after_format;
	struct img_header hdr;
	struct img_header hdr_after_format;
	struct new_intbr_img_header intbr_header;

	f = fopen(ifname, "rb");
	if (!f)
	{
		fprintf(stderr, "Error: unable to open payload file %s\n", ifname);
		return -1;
	}

	fseek(f, 0, SEEK_END);
	fsize = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (!fsize)
	{
		fclose(f);
		fprintf(stderr, "Error: payload file %s is empty\n", ifname);
		return -1;
	}

	fsize_aligned = size_aligned(fsize, sizeof (chksum));

	memcpy(hdr.signature, sig, 4);
	hdr.start_addr = htonl(start_addr);
	hdr.burn_addr = htonl(burn_addr);
	hdr.len = htonl(fsize_aligned + sizeof (chksum));

	if (insert_chksum_after_format == 1 || insert_chksum_sysupgrade_format == 1)
	{
		// This header will only be used to generate the checksum
		memcpy(hdr_after_format.signature, sig, 4);
		hdr_after_format.start_addr = htonl(start_addr);
		hdr_after_format.burn_addr = htonl(burn_addr);
		hdr_after_format.len = htonl(fsize_aligned + sizeof (chksum) - sizeof (struct img_header));
	}

	bufsize = size_aligned(fsize_aligned + sizeof (struct img_header) + sizeof (chksum), align_size);

	// The new Intelbras header
	if (insert_new_intelbras_header) {
		// Set the new header
		intbr_header.magic_number = NEW_INTBR_MAGIC_NUMBER;
		memset(&intbr_header.unknown_data_1, 0x00, NEW_INTBR_UKNOWN_SIZE_1);
		intbr_header.firmware_type = NEW_INTBR_FIRMWARE_TYPE;
		intbr_header.product_id = NEW_INTBR_RG1200AC_PROD_ID;
		intbr_header.unknown_data_2 = 0x00000000;
		memset(&intbr_header.padding, 0x00, NEW_INTBR_PADDING_SIZE);
		memcpy(&intbr_header.firmware_id_text, NEW_INTBR_FIRM_TEXT, sizeof(NEW_INTBR_FIRM_TEXT));

		bufsize += sizeof(struct new_intbr_img_header);
	}

	buf = (char *) calloc(bufsize, 1);
	if (!buf)
	{
		fclose(f);
		fprintf(stderr, "Error: unable to allocate memory\n");
		return -1;
	}

	// Append the new Intelbras header
	if (insert_new_intelbras_header) {
		memcpy(buf, &intbr_header, sizeof(struct  new_intbr_img_header));
		// Move the pointer
		aux_buffer = buf;
		buf += sizeof(struct new_intbr_img_header);
	}

	if (fread(buf + sizeof (struct img_header), 1, fsize, f) != fsize)
	{
		fclose(f);
		fprintf(stderr, "Error: failed to read payload file\n");
		return -1;
	}

	fclose(f);

	if (insert_chksum_after_format == 1 || insert_chksum_sysupgrade_format == 1)
	{
		// This header will only be used to generate the checksum
		memcpy(buf, &hdr_after_format, sizeof (struct img_header));
		// Copy length without last 16 bytes (img_header)
		memcpy(buf + fsize_aligned - sizeof (hdr_after_format.len), &hdr_after_format.len, sizeof (hdr_after_format.len));
		chksum_after_format = calculate_checksum((uint8_t *) buf + sizeof (struct img_header), fsize_aligned - sizeof (struct img_header));
		// Copy new checksum after format
		memcpy(buf + fsize_aligned, &chksum_after_format, sizeof (chksum_after_format));
	}

	if (insert_chksum_sysupgrade_format)
	{
		memcpy(buf, &hdr_after_format, sizeof (struct img_header));
	}
	else
	{
		memcpy(buf, &hdr, sizeof (struct img_header));
		chksum = calculate_checksum((uint8_t *) buf + sizeof (struct img_header), fsize_aligned);
		memcpy(buf + fsize_aligned + sizeof (struct img_header), &chksum, sizeof (chksum));
	}

	f = fopen(ofname, "wb");
	if (!f)
	{
		fprintf(stderr, "Error: unable to open output file %s\n", ofname);
		return -1;
	}

	// Rewind the pointer
	if (insert_new_intelbras_header)
		buf = aux_buffer;

	if (fwrite(buf, 1, bufsize, f) != bufsize)
	{
		fclose(f);
		fprintf(stderr, "Error: failed to write output file\n");
		return -1;
	}

	fclose(f);

	printf(
		"Image created:\n"
		"    Filename:\t\t%s\n"
		"    Signature:\t\t%s\n"
		"    Start address:\t0x%08x\n"
		"    Burn address:\t0x%08x\n"
		"    Data size (oct.):\t%lu bytes\n"
		"    Checksum:\t\t%04x\n",
		ofname,
		sig,
		start_addr,
		burn_addr,
		fsize_aligned + sizeof (chksum),
		chksum
	);

	return 0;
}

int main(int argc, char *argv[])
{
	int ret = EXIT_FAILURE;
	char *unit;

	progname = basename(argv[0]);

	if (argc < 2)
		usage(EXIT_SUCCESS);

	while ( 1 )
	{
		int c;

		c = getopt(argc, argv, "a:b:c:e:i:o:s:t:g:TSfhjIuz");
		if (c == -1)
			break;

		switch (c)
		{
		case 'a':
			align_size = strtoul(optarg, &unit, 0);
			if (unit) {
				switch (tolower(*unit)) {
				case 'b':
					break;
				case 'k':
					align_size <<= 10;
					break;
				case 'm':
					align_size <<= 20;
					break;
				default:
					fprintf(stderr, "Error: invalid size unit '%c'\n", *unit);
					return -EINVAL;
				}
			}
			break;
		case 'b':
			burn_addr = strtoul(optarg, NULL, 0);
			break;
		case 'c':
			sig_cpu = resolve_cpu_type(optarg);
			break;
		case 'e':
			start_addr = strtoul(optarg, NULL, 0);
			break;
		case 'i':
			ifname = optarg;
			break;
		case 'o':
			ofname = optarg;
			break;
		case 's':
			sig = optarg;
			break;
		case 't':
			sig_type = resolve_payload_type(optarg);
			break;
		case 'T':
			insert_chksum_after_format = 1;
			break;
		case 'S':
			insert_chksum_sysupgrade_format = 1;
			break;
		case 'f':
			append_fake_rootfs = 1;
			break;
		case 'j':
			append_jffs2_marker = 1;
			break;
		case 'h':
			usage(EXIT_SUCCESS);
			break;
		case 'g':
			model = optarg;
			insert_intelbras_header = 1;
			break;
		case 'I':
			insert_new_intelbras_header = 1;
			break;
		case 'u':
			insert_uimage_header = 1;
			break;
		case 'z':
			insert_intelbras_footer = 1;
			break;
		default:
			usage(EXIT_FAILURE);
			break;
		}
	}

	ret = check_options();
	if (ret)
		return ret;

	if (append_fake_rootfs)
		return image_append_fake_rootfs();
	else if (append_jffs2_marker)
		return image_append_jffs2_marker();
	else if (insert_uimage_header)
		return image_append_uimage_header();
	else if (insert_intelbras_header) 
		return image_append_intelbras_header();
	else if (insert_new_intelbras_header && !insert_chksum_sysupgrade_format)
		return image_append_new_intelbras_header();
	else if (insert_intelbras_footer)
		return image_append_intelbras_footer();
	else
		return build_image();
}
