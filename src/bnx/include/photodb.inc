/*
 * The kinds of album of the photo pages, the states of the photo database and its fields and columns (the sce_photo
 * table), and the interface of the slide show and its toolbar (slideshow, toolbar_emb).
 */

/* Values of decstate (photo_grid, photo_parts, photo): the photo under the cursor. */
const DECSTATE_BROKEN = -1; /* it cannot be decoded */
const DECSTATE_PENDING = 0; /* not decoded yet */
const DECSTATE_SHOWN = 1;

/* Values of slide_mode of the slide show. */
const SLIDE_PAUSED = 0;
const SLIDE_STEP = 1; /* show the next photo, then pause */
const SLIDE_PLAY = 2;

/* The buttons of the toolbar of the slide show. */
const BUTTON_BACK = 0;
const BUTTON_NEXT = 1;
const BUTTON_START = 2;
const BUTTON_PAUSE = 3;
const BUTTON_STOP = 4;

/*
 * The kinds of album (the window variable "albummode", album_kind of get_photo_field): a list of albums (ALBUM_*) or
 * the photos of an album of that kind (PHOTOS_* = ALBUM_* + PHOTOS_LIBRARY).
 */
const ALBUM_LIBRARY = 0;
const ALBUM_FAVORITE = 1;
const ALBUM_DISC = 2;
const ALBUM_USB = 3; /* the files of a USB storage device */
const PHOTOS_LIBRARY = 4;
const PHOTOS_FAVORITE = 5;
const PHOTOS_DISC = 6;
const PHOTOS_USB = 7;

/* The most albums of the library, and the most photos of an album. */
const MAX_ITEMS = 999;

/* The fields of get_photo_field. */
const FIELD_URL = 0; /* the image, or the cover of an album */
const FIELD_ALBUM_NAME = 1;
const FIELD_TITLE = 2;
const FIELD_ALBUM_URI = 3; /* the id of the album */
const FIELD_DATE = 4;
const FIELD_OID = 5;

/* The states of the album and photo databases (g_album_db_state, g_thumb_db_state). */
const DB_READY = 0; /* opened and queried */
const DB_KEEP = 1;  /* the rows are still selected: the next load keeps their number */
const DB_OPEN = 2;
const DB_CLOSED = 3;

/*
 * The fields of datGetData for a row of sce_photo (SELECT *, oid: the column number plus one) and for a file of the
 * USB device.
 */
const COL_TITLE = 3;
const COL_ALBUM_NAME = 4;
const COL_CONTENT_URI = 5;
const COL_ALBUM_URI = 6;
const COL_CREATE_DATE = 7;
const COL_OID = 11;
const USB_COL_NAME = 1;
const USB_COL_PATH = 2;
