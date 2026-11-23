/*
 * The albums of the music pages, the values of the aval file that the music scripts pass to each other
 * (MusicAlbumMode, MusicAlbumNo, MusicChannelMode, ...) and the fields of the queries of the music database.
 */

/* Albums (MusicAlbumMode), in the order of the TEXT elements of the group album_menu. */
const MUSIC_ALBUM_FAVORITE = 0;
const MUSIC_ALBUM_COLLECTION = 1; /* the albums of the hard disk, rows of the music database */
const MUSIC_ALBUM_CD = 2;         /* the audio CD in the drive */
const MUSIC_ALBUM_PD = 3;         /* the portable device on USB */

/* The most albums of the hard disk, and the most tracks of a favorite album. */
const MAX_ITEMS = 999;

/* MusicAlbumNo of the albums that are no row of the database. */
const ALBUM_NO_CD = -1;
const ALBUM_NO_PD = -2;

/* MusicChannelMode: whether the information page shows an album or a track. */
const CHANNEL_ALBUM = 0;
const CHANNEL_TRACK = 1;

/*
 * The fields of datGetData for an album of the music database (select albumname,url,trackno,artistname,length,
 * datasize/1024 from scei_jukebox where format = -1, or -2 for the favorite albums).
 */
const ALBUM_COL_NAME = 1;
const ALBUM_COL_URL = 2;    /* the folder of its tracks */
const ALBUM_COL_TRACKS = 3; /* trackno: the number of tracks */
const ALBUM_COL_ARTIST = 4;
const ALBUM_COL_LENGTH = 5;
const ALBUM_COL_SIZE = 6;   /* in kilobytes */

/*
 * The fields of datGetData for a track (select songname,artistname,trackno,length,datasize/1024,format,url, and for
 * the tracks of a favorite album albumname,entrydate).
 */
const TRACK_COL_NAME = 1;
const TRACK_COL_ARTIST = 2;
const TRACK_COL_LENGTH = 4;
const TRACK_COL_SIZE = 5; /* in kilobytes */
const TRACK_COL_FORMAT = 6;
const TRACK_COL_URL = 7;
const TRACK_COL_ALBUM = 8;
const TRACK_COL_ENTRY_DATE = 9;

/* MusicPrefQuality: the recording quality of the player (PLAYER_GET_PREFS), in the order of its radio items. */
const QUALITY_HIGH = 0;
const QUALITY_NORMAL = 1;
const QUALITY_LOW = 2;

/* MusicRepeatMode (PLAYER_GET_PREFS). */
const REPEAT_OFF = 0;
const REPEAT_ALL = 1;
const REPEAT_ONE = 2; /* the track being played */

/* Bitrate: the bitrate of the compressed copy of a track, by recording quality. */
const BITRATE_132K = 5; /* QUALITY_HIGH */
const BITRATE_105K = 4; /* QUALITY_NORMAL */
const BITRATE_66K = 2;  /* QUALITY_LOW */
