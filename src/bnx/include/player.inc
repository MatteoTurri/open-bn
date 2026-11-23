/*
 * The natives of the player library (/opt0/bn/modules/player.so, installed by the language pack): one variadic native
 * that takes a command number and its arguments. The arguments go by address, so a command can write its results into
 * them.
 */

/* The units of the player. */
const PLAYER_UNIT_CD = 0;
const PLAYER_UNIT_HDD = 1;

/* The states of a unit (PLAYER_CD_GET_STATE, PLAYER_GET_UNIT_STATE). */
const PLAYER_STATE_IDLE = 0;
const PLAYER_STATE_STOPPED = 2;

/* The commands, by the arguments they take. */
const PLAYER_RESUME = 0;             /* unit: back to normal play after PLAYER_FORWARD or PLAYER_REWIND */
const PLAYER_PLAY = 1;               /* unit, track: plays from track, counted from 1 */
const PLAYER_STOP = 2;               /* unit */
const PLAYER_PAUSE = 5;              /* unit: pauses, or goes on with a paused track */
const PLAYER_FORWARD = 6;            /* unit */
const PLAYER_REWIND = 7;             /* unit */
const PLAYER_RIP_START = 10;         /* starts ripping the CD to the hard disk */
const PLAYER_RIP_CANCEL = 11;
const PLAYER_RIP_GET_STATE = 12;     /* &state, &track, &percent */
const PLAYER_PD_CHECKOUT = 20;       /* 0, track: copies a track of the hard disk to the portable device */
const PLAYER_PD_CHECKIN = 21;        /* 0, track: checks a track of the portable device back in */
const PLAYER_CANCEL = 22;            /* ends the request that runs (PLAYER_GET_STATUS) */
const PLAYER_PD_GET_PROGRESS = 23;   /* &percent, &progress: of the check-out that runs */
const PLAYER_PD_GET_TRACKS = 24;     /* &tracks: the tracks of the portable device */
const PLAYER_SET_QUALITY = 25;       /* quality: sets and saves the recording quality */
const PLAYER_GET_QUALITY = 26;       /* &quality */
const PLAYER_PD_CHECK = 27;          /* mode: starts reading the portable device, for PLAYER_PD_GET_INFO */
const PLAYER_PD_GET_INFO = 28;       /* &tracks, &type, &write_disabled: what PLAYER_PD_CHECK read */
const PLAYER_READ_TRACK_INFO = 29;   /* 0, track: starts reading the values of PLAYER_GET_TRACK_INFO of a track */
/* &checkout_times, &bitrate, &size: what PLAYER_READ_TRACK_INFO read, -1 without */
const PLAYER_GET_TRACK_INFO = 30;
const PLAYER_PD_GET_RESULT = 31;     /* &result: 0 once a check-in or check-out is done, below 0 on an error */
const PLAYER_GET_STATUS = 32;        /* &status: below 0 while the last request runs */
const PLAYER_SET_ALBUM_PATH = 33;    /* path[]: the album of the hard disk to play, padded with spaces */
const PLAYER_HDD_DELETE = 34;        /* 0, track: deletes a track of the hard disk, or its album below 0 */
const PLAYER_PD_DELETE = 35;         /* 0, track: deletes a track of the portable device */
const PLAYER_PD_DELETE_ALL = 36;
const PLAYER_GET_LEVEL = 100;        /* x, &left, &right: the levels of the two channels */
const PLAYER_CD_GET_TOC = 101;       /* &tracks, ...: the number of tracks of the CD, -1 until it is known */
const PLAYER_CD_GET_TRACK = 102;     /* &track */
const PLAYER_CD_GET_TIME = 103;      /* &time[0], &time[1], &time[2]: the time in the track */
const PLAYER_CD_GET_STATE = 104;     /* &state, &info: PLAYER_STATE_* */
const PLAYER_CD_GET_PERCENT = 105;   /* &percent: of the current track */
const PLAYER_HDD_GET_TRACK = 107;    /* &track */
const PLAYER_HDD_GET_TIME = 108;     /* &time[0], &time[1], &time[2]: the time in the track */
const PLAYER_HDD_GET_STATE = 109;    /* &state, &info */
const PLAYER_HDD_GET_PERCENT = 110;  /* &percent: of the current track */
const PLAYER_SET_REPEAT_MODE = 111;  /* mode */
const PLAYER_SET_PLAY_MODE = 112;    /* mode */
const PLAYER_SET_FAVORITE = 113;     /* favorite: the favorite album to play, counted from 1 */
const PLAYER_SET_DIN_THRU = 115;     /* on: passes the digital input of the CD through */
const PLAYER_CD_READ_TOC = 116;      /* x: reads the TOC of the CD again, for PLAYER_CD_GET_TOC */
const PLAYER_GET_AREA_STATE = 117;   /* &state: not 0 while the hard disk has no recording area */
const PLAYER_MAKE_AREA = 118;        /* creates the recording area, for PLAYER_GET_AREA_STATE */
const PLAYER_GET_PREFS = 120;        /* &quality, &repeat, &shuffle */
const PLAYER_SET_PREFS = 121;        /* quality, repeat, shuffle: sets and saves them */
const PLAYER_RESET = 122;            /* once, at the start of the player */
const PLAYER_GET_WAVE = 126;         /* left[], right[]: 128 bytes of the wave of each channel */
const PLAYER_CD_GET_TOC_TIME = 127;  /* entry, &minutes, &seconds, &frames: of an entry of the TOC (past the last track,
                                        the end of the disc) */
const PLAYER_GET_INIT = 128;         /* &status: 0 until the player has started */
const PLAYER_SET_DISPLAY_MODE = 129; /* mode */
const PLAYER_GET_DISPLAY_MODE = 130; /* &mode */
const PLAYER_HOLD_COMPRESSION = 131; /* &hold: -1 stops compressing the ripped tracks and waits for it, 0 goes on;
                                        hold is 1 when it is done */
const PLAYER_PD_GET_BUSY = 133;      /* &busy: not 0 while the player reads the portable device */
const PLAYER_GET_UNIT_STATE = 134;   /* unit, &state: PLAYER_STATE_* */

/* Results of PLAYER_RIP_START. */
const RIP_START_NO_SPACE = 1;
const RIP_START_NO_AREA = 2; /* the recording area could not be made */

/* The numbers of tracks below 0 of PLAYER_PD_GET_INFO, besides -1 (no device). */
const PD_NO_MEDIA = -4;   /* the device has no media in it */
const PD_CONNECTING = -8; /* the connection is still being checked */

/* Results of PLAYER_PD_GET_RESULT for a check-in or a check-out that failed. */
const PD_ERR_NO_INFO = -101;          /* check-in: a track has no content information */
const PD_ERR_COUNT_LIMIT = -200;      /* to PD_ERR_COUNT_LIMIT_LAST: a track may be checked out no more times */
const PD_ERR_COUNT_LIMIT_LAST = -299;
const PD_ERR_PROHIBITED = -301;       /* a track may not be checked out */
const PD_ERR_NO_SPACE = -403;         /* the media of the device is full */
const PD_ERR_WRITE_PROTECTED = -407;

#pragma library "player"
native playerExecCommand(...);
#pragma library
