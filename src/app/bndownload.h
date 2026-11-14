/*
 * Background download of a content file into the __contents partition
 * (bndownload.cpp).
 */

#ifndef BNDOWNLOAD_H
#define BNDOWNLOAD_H

int bnDownloadStart(char *user, char *password, char *uri, char *dstPath);
int bnDownloadSync(int *out);
void bnDownloadCancel();
int bnContentDelete();

#endif
