/*
 * The albums of the movie pages and the values and the columns of the movie database (the sce_movie table).
 */

/* Albums (the album mode of the pages), in the order of the radio buttons. */
const ALBUM_COLLECTION = 0; /* the albums of the hard disk, rows of the movie database */
const ALBUM_DISC = 1;       /* the DVD video in the drive */
const ALBUM_FAVORITE = 2;

/* Values of the content_attr column. */
const ATTR_MOVIE = 0;       /* a movie of an album */
const ATTR_FAV_MOVIE = 1;   /* a movie of a favorite album */
const ATTR_ALBUM = 128;     /* an album: a folder of movies */
const ATTR_FAV_ALBUM = 129;
const ATTR_SETTINGS = 256;  /* the row that keeps the sort order, in content_type */

/* Values of the content_type column of a movie. */
const CONTENT_REAL = 259; /* RealMedia, played by the RealPlayer */
const CONTENT_PSS = 260;  /* a .pss or .psm movie */

/* The fields of datGetData for a row of sce_movie (select *: the column number plus one). */
const COL_TITLE = 1;
const COL_ALBUM_NAME = 2;
const COL_CREATE_DATE = 3;
const COL_URI = 4;
const COL_ALBUM_ID = 5;
const COL_CONTENT_TYPE = 6;
const COL_CONTENT_SUBTYPE = 7;
const COL_CONTENT_ATTR = 8;

/* The most albums of a kind, and the most movies of an album. */
const MAX_ITEMS = 999;

/* The errors of add_album. */
const ADD_ALBUM_FAILED = -1;
const ADD_ALBUM_NO_NAME = -2; /* the name given on the software keyboard is empty */
const ADD_ALBUM_FULL = -3;    /* there are MAX_ITEMS albums already */

/* The sort orders of the albums, kept in the content_type of the ATTR_SETTINGS row; the items of optmenu_sort. */
const SORT_NEWEST = 0; /* by create_date, newest first */
const SORT_OLDEST = 1;
