/*
 * Size of an HDD application partition, including its sub-partitions.
 */

#include <string>
#include <stdio.h>

#include <string.h>
#include "lib/scepfsd.h"

#include "fs/bbfileaccess.h"
#include "fs/bbmount.h"

static char g_hddPathBuf[64];
static char g_hddNameBuf[64];

/* Size of the partition at `p` in 2048-byte units: the main partition plus
   every sub-partition (ioctl 0x6803 returns the sub-partition count, ioctl
   0x6833 the size of one). */
static int bn_hdd_part_size(const char *p)
{
	long long total = 0;
	int fd;
	int n;
	int i;

	fd = scepfsOpen(p, 1);
	if (fd < 0)
		return -1;
	n = scepfsIoctl2(fd, 0x6803, 0, 0, 0, 0);
	for (i = 0; i < n + 1; i++)
		total += scepfsIoctl2(fd, 0x6833, &i, 4, 0, 0);
	scepfsClose(fd);
	return total / 2048;
}

/*
 * Sums the size of partition `path` ("hdd0:<name>") and of all its
 * sub-partitions. Partition names are compared on three '.'-separated
 * fields: the base name and the two segments after it.
 */
int bnHddGetPartitionSize(char *path)
{
	iox_dirent_t de;
	string s(path);
	char *pathSeg1;
	char *pathSeg2;
	char *entSeg1;
	char *entSeg2;
	char *p;
	char *name;
	int i;
	int r;
	int fd;
	int total;

	memset(g_hddPathBuf, 0, 64);
	strcpy(g_hddPathBuf, path + 5);
	pathSeg1 = entSeg1 = "";
	pathSeg2 = entSeg2 = "";
	s.erase(0, 4);
	unsigned int lead = s.find_first_not_of('/', 0);
	if (lead != 0 && lead != (unsigned int)-1)
		s.erase(0, lead);
	BBpfsDisableMounter(s);
	i = 0;
	do {
		r = BBpfsUnMount(s);
		if (r == 0 || r == 2)
			break;
		i++;
	} while (i < 1000);
	if (r != 0 && r != 2)
		return -1;

	p = g_hddPathBuf;
	for (i = 0; i < 3; i++) {
		p = strchr(p, '.');
		if (p == 0)
			break;
		if (i == 0) {
			*p = '\0';
			p++;
			pathSeg1 = p;
		} else if (i == 1) {
			*p = '\0';
			p++;
			pathSeg2 = p;
		} else {
			*p = '\0';
			break;
		}
	}
	name = g_hddNameBuf;
	sprintf(name, "hdd0:%s", path + 5);
	total = bn_hdd_part_size(name);
	if (total < 0)
		return total;
	g_hddPathBuf[1] = 'C';
	fd = scepfsDopen("hdd0:");
	if (fd < 0) {
		BBpfsClearDisableMounter();
		return -2;
	}
	while (scepfsDread(fd, &de) > 0) {
		if (de.name[0] == '_')
			continue;
		if (de.name[2] != '.')
			continue;
		if (de.stat.attr & 1)  /* bit 0 set = skip this entry */
			continue;
		memset(g_hddNameBuf, 0, 64);
		strcpy(g_hddNameBuf, de.name);
		p = g_hddNameBuf;
		for (i = 0; i < 3; i++) {
			p = strchr(p, '.');
			if (p == 0)
				break;
			if (i == 0) {
				*p = '\0';
				p++;
				entSeg1 = p;
			} else if (i == 1) {
				*p = '\0';
				p++;
				entSeg2 = p;
			} else {
				*p = '\0';
				break;
			}
		}
		if (strcmp(g_hddPathBuf, name) != 0)
			continue;
		if (strcmp(pathSeg1, entSeg1) != 0)
			continue;
		if (strcmp(pathSeg2, entSeg2) != 0)
			continue;
		sprintf(g_hddNameBuf, "hdd0:%s", de.name);
		total += bn_hdd_part_size(g_hddNameBuf);
	}
	scepfsDclose(fd);
	BBpfsClearDisableMounter();
	return total;
}
