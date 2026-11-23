/*
 * The interface of the scripts of the file browser (browser, browser_thumb, browser_fileinfo, browser_infomenu).
 */

/* The device of the album: the window variable BRDEVMODE, and the mode of svdatGetFreeSize. */
const DEVMODE_HDD = 0;
const DEVMODE_MC = 1;
const DEVMODE_VMC = 2;

/* The fields of a save in the list of datOpen, for datGetData and datUpdateData. */
const SAVE_STATE = 0; /* kept by the browser: STATE_* */
const SAVE_NAME = 1;
const SAVE_COMMENT = 2;
const SAVE_TYPE = 4;
const SAVE_ATTR = 7;

/* The bit of SAVE_ATTR of a save that must not be copied (sceMcFileAttrDupProhibit). */
const ATTR_NO_COPY = 0x8;

/* The values of SAVE_TYPE that are saves. */
const TYPE_PS2_SAVE = 3;
const TYPE_PS1_SAVE = 4;
const TYPE_PDA_SAVE = 5; /* of a PocketStation */

/* The most folders the browser takes in pfs:/__common, and the most saves a hard disk folder may hold. */
const MAX_FOLDERS = 256;
const MAX_FOLDER_SAVES = 1022;

/* The errors of svdatChkFolderName. */
const NAME_TOO_LONG = -1; /* or not UTF-8 */
const NAME_BAD_CHAR = -2;
const NAME_DOTS = -3;     /* "." or ".." */

/* The kinds of a save of svdatGetInfoData: from the memory card, or (3 to 7) from the type in its icon.sys. */
const KIND_PS2_SAVE = 0;
const KIND_PS1_SAVE = 1;
const KIND_PDA_SAVE = 2;     /* PocketStation data */
const KIND_PS2_SOFTWARE = 3;
const KIND_UNKNOWN = 4;
const KIND_PDA = 5;          /* shown as PocketStation data too */
const KIND_SETTING = 6;
const KIND_SYSTEM_DRIVER = 7;

/* The values of SAVE_STATE, also passed in the window variable BRDAMAGED. */
const STATE_UNCHECKED = 0; /* the icon of the save has not been loaded yet */
const STATE_SOUND = 1;
const STATE_DAMAGED = 2; /* damaged, or of a kind without an icon */

/*
 * The status a save job ends with (svdatSync; fileSync for a rename or a delete, which run the same file operation):
 * 0 when it succeeds (and while it runs), a negative errno when it fails, or one of these. A job that cannot read the
 * free space of the hard disk ends with 100, which no script tests.
 */
const SVDAT_EXISTS = 1; /* copy, move, rename: the destination exists (and overwrite was not set) */
const SVDAT_NO_SPACE = 2; /* copy, move, new folder: not enough free space */
const SVDAT_SKIPPED = 3; /* folder copy: done, but saves that are copy protected or damaged were left out */
const DISK_ERROR = 101; /* an I/O error on the hard disk (-EIO); iconSync gives it too */
const SVDAT_EIO = -5; /* svdatInformation: the hard disk could not be read (-EIO) */

/* The "TYPE" of an icon object once it is loaded, 0 before. */
const ICON_TYPE_PS1 = 1;
const ICON_TYPE_PS2 = 2;

/* The status of iconSync: 0 loaded, DISK_ERROR, or: */
const ICON_CARD_ERROR = 1; /* the memory card failed: taken out, changed or not formatted */
