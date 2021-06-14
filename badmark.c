/*
 * badmark.c
 *
 *  Created on: 31 авг. 2014 г.
 *      Author: andrey
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <getopt.h>

#include <asm/types.h>
#include "mtd/mtd-user.h"


struct mtd_info_user meminfo;
struct mtd_ecc_stats oldstats, newstats;
int fd;
int markbad=0;
int seed;

// 0x0b640000 = 191102976; 191102976/(2048 * 64) = 1458 ; это завод.
// всего блоков на /dev/mtd4:
// /dev/mtd4 max addr f860000 --> blocks - f860000 = 260440064/(2048 * 64) = 1987

int main(int argc, char **argv) {
	puts("!!!Hello World!!!"); /* prints !!!Hello World!!! */

	//static const char short_options[] = "ab:C::d";

	uint32_t offset = 0;
	uint32_t length = -1;
	int rez;
	int all_flag = 1;
	int good_flag = 0, test_flag = 0;
	int bad_num, offset_blk, num_blks, good_num;
	bad_num = 0; offset_blk = 0; num_blks = 0, good_num =0;

	// -a             ->>  scan all device
	// -o 100 -s 10   ->>  scan form 100's block to 110's block
	// -n 100 		  ->>  just test 100's block if erase error, then mark it bad.

	while ( (rez = getopt(argc,argv,"ato:s:b:g:")) != -1){
		printf("rez = %c \n", rez);
		switch (rez){
		case 'b':
			all_flag = 0;
			bad_num = atol(optarg);
			break;
		case 'g':
			good_flag = 1;
			good_num = atol(optarg);
			break;
		case 't':
			test_flag = 1;
			break;
		}
	};

	printf("device name: %s \n", argv[optind] );

	struct mtd_info_user meminfo;
	struct mtd_ecc_stats oldstats, newstats;
	struct mtd_ecc_stats stat1, stat2;
	int fd;
	fd = open(argv[optind], O_RDWR);
	if (fd < 0) {
		perror("open");
		exit(1);
	}

	if (ioctl(fd, MEMGETINFO, &meminfo)) {
		perror("MEMGETINFO");
		close(fd);
		exit(1);
	}

	struct erase_info_user er;
	struct erase_info_user64 ei64;
	int ebsize = 131072;
	er.length = ebsize;
	ei64.length = ebsize;
	loff_t some_bb;

	//printf("meminfo.type: %d \n", meminfo.type);
	printf("Before test \n");

	if (!ioctl(fd, ECCGETSTATS, &stat1)) {
		fprintf(stderr, "ECC failed: %d\n", stat1.failed);
		fprintf(stderr, "ECC corrected: %d\n", stat1.corrected);
		fprintf(stderr, "Number of bad blocks: %d\n", stat1.badblocks);
		fprintf(stderr, "Number of bbt blocks: %d\n", stat1.bbtblocks);
	} else {
		perror("No ECC status information available");
	}

	if(test_flag == 0){

	if(all_flag == 0){
		// try to erease bad block bad_num;
		ei64.start = (__u64)bad_num * ebsize;
		// test only specified block 1458;
		if(good_flag == 0){
			printf("bad_num: %d \n", bad_num);
			some_bb = (loff_t)bad_num * ebsize;
		}else{
			printf("bad_num: %d; reall (int)ei64.starty marked: %d \n", bad_num, good_num);
			some_bb = (loff_t)good_num * ebsize;
		}

		if (ioctl(fd, MEMERASE64, &ei64)) {
			printf("MEMERASE64 ERROR %08lx\n", (int)ei64.start);
			printf("Mark block bad at %08lx\n", (int)some_bb);
			if(ioctl(fd, MEMSETBADBLOCK, &some_bb)){
				printf("MEMSETBADBLOCK OK: %08lx \n", (int)some_bb);
			}else{
				printf("MEMSETBADBLOCK FALSE %08lx \n", (int)some_bb);
			}
		}else{
			printf("MEMERASE OK %08lx\n",  (int)ei64.start);
		}

	}else{
		//printf("from 0 block to 1987 block \n");
		int i;
		for(i=0;i<1987;i++){
			ei64.start = (__u64)i * ebsize;
			//printf("block num: %d \n", i);
			if (ioctl(fd, MEMERASE64, &ei64)) {
				//printf("MEMERASE64 ERROR \n");
				//printf("Mark block bad at %08lx\n", (int)ei64.start);
				ioctl(fd, MEMSETBADBLOCK, &some_bb);
			}else{
				//printf("MEMERASE OK \n");
			}
		}

	}

	printf("After test \n");


	if (!ioctl(fd, ECCGETSTATS, &stat1)) {
		fprintf(stderr, "ECC failed: %d\n", stat1.failed);
		fprintf(stderr, "ECC corrected: %d\n", stat1.corrected);
		fprintf(stderr, "Number of bad blocks: %d\n", stat1.badblocks);
		fprintf(stderr, "Number of bbt blocks: %d\n", stat1.bbtblocks);
	} else {
		perror("No ECC status information available");
	}


	}else{
		loff_t test_ofs;
		int i;
		for(i=0;i<127232;i++){
			test_ofs = i*2048;
			if (ioctl(fd, MEMGETBADBLOCK, &test_ofs)) {
				printf("\rBad block at 0x%08x\n", (unsigned)test_ofs);
			} else {
				printf("\rGood block at 0x%08x\n", (unsigned)test_ofs);
			}
		}
	}



	return 0;
}
