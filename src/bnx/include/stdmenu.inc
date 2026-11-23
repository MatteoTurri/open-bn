/*
 * The results of a push in the standard option menu of the pages (std_menu_core, pop_std_menu and the
 * handle_menu_push and apply_result of each page), the flags of pop_std_menu and the mode of std_menu_core.
 */

const MENU_REDRAW = 1;  /* show the same menu again */
const MENU_REOPEN = 2;  /* reopen it with the title, group and id it sends */
const MENU_RETURN = 3;  /* reopen it on the same group and id */
const MENU_SUBMENU = 4; /* open the subgroup of the item */
const MENU_BACK = 5;    /* open the menu of retgroup and retitem */
const MENU_CANCEL = 6;  /* closed by cross; from this result on, pop_std_menu ends */
const MENU_CLOSED = 7;
const MENU_ITEM = 8;    /* MENU_ITEM + index: item index was chosen */

/* The side of the screen the menu opens on (flags & 0x3 of pop_std_menu); 2 is the left side too. */
const MENU_SIDE_LEFT = 0;
const MENU_SIDE_RIGHT = 1;

/* In the flags of pop_std_menu: any bit of 0x300 puts a dark backdrop behind the menu, this one with a fading edge. */
const MENU_BACKDROP_FADED = 0x200;

/* The mode of std_menu_core: the menu lists the items of its MENUGROUP; a page may use other modes for the menus it
 * fills itself (count_extra_items, fill_menu_item). */
const MENU_MODE_GROUP = 0;
