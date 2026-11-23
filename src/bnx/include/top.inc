/*
 * The natives of the top library (/opt0/bn/modules/top.so, src/modules/top/top.cpp), in the order of its FuncTable:
 * the ISP settings files of the Top Menu and their key=value pairs, and the numbers get_isp_value gives the keys.
 */

#pragma library "top"
/* Enables the navigation menu and lists the ISP settings files. */
native topInit();
native topIspGetCount();
/* Always returns 0. */
native topIspSync();
/* Reads the key=value pairs of ISP file index, for topIspGetValue. */
native topIspOpen(index);
/* Forgets the pairs read by topIspOpen. */
native topIspClose();
native topIspGetValue(const key[], value[], maxlength);
#pragma library

/* The fields of get_isp_value (topisp.inc): the keys of an ISP file, by number. */
const ISP_VER = 1;
const ISP_CATEGORY = 2;
const ISP_ID = 3;
const ISP_PRIORITY = 5;
const ISP_NAME = 6;
const ISP_APPLICATION = 7;
const ISP_TYPE = 8;
const ISP_URL = 9;
const ISP_OPTION = 10;
const ISP_CONTENTS = 12;
