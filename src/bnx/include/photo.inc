/*
 * The natives of the photo library (/opt0/bn/modules/photo.so), in the order of its FuncTable: a list of photos and
 * text conversions.
 */

#pragma library "photo"
native photoListClear();
native photoListAdd(const path[]);
native photoListNum();
native photoListGet(index, const buf[], maxlength);
/*
 * Rewrites a date read from the database (in UTC) in local time, in the date notation (0: Y/M/D, 1: M/D/Y, 2: D/M/Y)
 * and the time notation (0: 24 hours, 1: 12 hours) of the system settings.
 */
native photoDateGetFromDB(const date_str[], maxlength, date_notation, time_notation);
native photoSJIStoUTF8(const dest[], const src[]);
native photoUTF8toSJIS(const dest[], const src[]);
#pragma library
