/*
 * What follows a push in the information menu of the pages (infomenu: handle_push, apply_result); pop_info_menu
 * reopens the menu below INFOMENU_CLOSE. Also the side and the mode of the menu.
 */

const INFOMENU_STAY = 0;
const INFOMENU_REOPEN = 1;
const INFOMENU_RETITLE = 2; /* reopen titled as the menu the item returns to */
const INFOMENU_SUBMENU = 3; /* open the subgroup of the item */
const INFOMENU_BACK = 4;    /* open the menu of retgroup and retid of the item */
const INFOMENU_CLOSE = 5;
const INFOMENU_RENAMED = 6; /* closed after the title was changed */
const INFOMENU_ERROR = 7;
const INFOMENU_USB_REMOVED = 9; /* the USB device was removed */

/* The side of the screen the menu opens on (the side of pop_info_menu); 2 is the left side too. */
const INFOMENU_SIDE_LEFT = 0;
const INFOMENU_SIDE_RIGHT = 1;

/* The mode of info_menu_core and handle_push: the menu lists the TEXT elements of its group. */
const INFOMENU_MODE_GROUP = 0;
