/*
 * The interface of the general_menu part (parts/general_menu): the values of its "MODE" message (menu type, scroll
 * mode of scroll.inc, exit mask).
 */

/* Menu types. */
const MENU_TYPE_PAGE = 0;  /* part of a page: the focus leaves it by the edges of the exit mask */
const MENU_TYPE_POPUP = 1; /* a popup menu: circle and cross end it */

/* The edges by which the focus may leave a MENU_TYPE_PAGE menu (the exit mask). */
const EXIT_UP = 1;
const EXIT_DOWN = 2;
const EXIT_LEFT = 4;
const EXIT_RIGHT = 8;
