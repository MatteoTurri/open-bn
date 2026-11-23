/*
 * The interface of the scripts of the top menu (top, top_list, top_camera).
 */

/* Detection modes (the mediadetect of a MENUGROUP, enable_media_detect of the page): the discs the page reacts
 * to. */
const DETECT_GAME = 1;
const DETECT_MUSIC = 2;
const DETECT_MOVIE = 4;
const DETECT_PHOTO = 8;
const DETECT_ALL = 15;

/* Bits of init_status of top_list, which top reads when the list ends. */
const INIT_ISP_FAILED = 1; /* the ISP list could not be read */
const INIT_NO_ITEM = 255;  /* the menu has no item */

/* Views of the icon menu (the "DIST" message to the camera). */
const VIEW_CLOSE = 0; /* in front of one icon */
const VIEW_WIDE = 1;
