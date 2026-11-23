/*
 * The albums of the game page, the values and the columns of the game database (the sce_game table of game.db,
 * which bn fills with the games installed on the hard disk).
 */

/* Albums (the album mode of the game page), in the order of the radio buttons. */
const ALBUM_HDD = 0;  /* the games installed on the hard disk, rows of the game database */
const ALBUM_DISC = 1; /* the disc in the drive */

/* Values of the content_subtype column. */
const SUBTYPE_PS2 = 0; /* a PlayStation 2 game */
const SUBTYPE_PS1 = 1; /* a PlayStation game */

/* Values of the content_attr column. */
const ATTR_SETTINGS = 256; /* the row that keeps the sort order, in content_type */

/* The sort orders of the games, kept in the content_type of the ATTR_SETTINGS row. */
const SORT_NEWEST = 0; /* by install_date, newest first */
const SORT_OLDEST = 1;

/* The fields of datGetData for a row of sce_game (select *: the column number plus one). */
const COL_INST_SIZE = 3;         /* in megabytes */
const COL_TITLE = 4;
const COL_TITLE_ID = 5;
const COL_PUBLISHER_ID = 9;
const COL_INSTALL_DATE = 12;
const COL_URI = 15;              /* the partition of the game */
const COL_RESOURCE_PATH = 16;    /* the folder of the icon, the jackets and the manual */
const COL_IMAGE_COUNT = 19;      /* the number of jackets */
const COL_COPYRIGHT_IMGCOUNT = 22;
const COL_VIOLENCE_FLAG = 28;
const COL_CONTENT_TYPE = 29;
const COL_CONTENT_SUBTYPE = 30;  /* SUBTYPE_* */
