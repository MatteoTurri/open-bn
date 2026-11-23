/*
 * The natives of the movie library (/opt0/bn/modules/movie.so), in the order of its FuncTable: the play list and the
 * player.
 */

#pragma library "movie"
native movieListClear();
native movieListAdd(const path[]);
native movieListNum();
native moviePlay(index);
/* Starts reading the length of a movie, for movieGetTimeSync. */
native movieGetTimeStart(const path[]);
/* Returns 0 while reading, 1 with the length, -1 on an error. */
native movieGetTimeSync(&hours, &minutes, &seconds);
#pragma library
