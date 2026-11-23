/*
 * The natives of the ispconf library (/opt0/bn/modules/ispconf.so), in the order of its FuncTable: the installation of
 * the ISP settings.
 */

/* Results of ispConfSync. */
const ISPCONF_ABORTED = -1011;       /* the download was aborted: SCE_HTTP_ERR_ABORTED of scehttp.h */
const ISPCONF_DNAS_ABORTED = -20601; /* the DNAS authentication was aborted; a DNAS error is its code - 20000 */

#pragma library "ispconf"
/*
 * Returns 1 while the installation runs; progress gets its percentage, then its result: 0, a negative error, or the
 * HTTP status of the server when it is not 200.
 */
native ispConfSync(&progress);
/*
 * Starts the installation of the ISP settings: reads the file path, authenticates with DNAS, downloads the
 * settings from the server the file names and deletes path. Returns 0, or -1 while another one runs.
 */
native ispConfInstall(const path[]);
/* Asks the installation that runs to stop; returns 0. */
native ispConfCancel();
#pragma library
