/*
 * The items of a list laid out on a curve: how an item moves to a new place (the scroll mode of the list, "MODE",
 * and of the "SCRL" message to its items) and which messages it follows.
 */

const SCROLL_ALONG_CURVE = 0; /* driven by a timer */
const SCROLL_LINE = 1;
const SCROLL_HERMITE = 2;

/* The messages an item follows (g_update_mask); a blank item, past either end of the list, follows none. */
const UPDATE_FADE = 1;   /* "FADE" */
const UPDATE_SCROLL = 2; /* "SCRL" */
const UPDATE_FOCUS = 4;  /* "FOCS" */
