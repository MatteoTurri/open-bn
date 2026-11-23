/*
 * /opt0/bn/script/music/music_dialog.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The dialogs of the music pages, each a public function attached to an object of its own: rip the audio CD into a
 * new album, add a track to a favorite album, check tracks in from the portable device or out to it, delete an album
 * or a track. Each leaves in result whether the albums changed.
 */

#include <core>
#include <bn>

#pragma dynamic 1500

#include "musicdb.inc"

#include "stdmenu.inc"
#include "genmenu.inc"
#include "scroll.inc"

/* The results of ripping_main, in the order of the messages of g_rip_results. */
enum
{
    RIP_COMPLETE,
    RIP_FAILED,
    RIP_IMPORT,
    RIP_NO_SPACE,
    RIP_MAX,
    RIP_NOT_READING,
    RIP_CANCELLED,
    RIP_AREA_FULL,  /* the recording area could not be made */
    RIP_ALBUMS_FULL /* there are MAX_ITEMS albums already */
};

/* The results of favorite_main, in the order of the messages of g_favorite_results. */
enum
{
    FAVORITE_COMPLETE,
    FAVORITE_FAILED,
    FAVORITE_IMPORT,
    FAVORITE_NO_SPACE,
    FAVORITE_MAX /* the favorite album is full */
};

/* The errors of checkin_main: offsets in g_checkin_errors, to which the type of the device is added. */
const CHECKIN_PARTIAL = 2;   /* some tracks could not be checked in */
const CHECKIN_NO_DEVICE = 3; /* the portable device is not connected properly */
const CHECKIN_WRITE_PROTECTED = 4;

/* The errors of checkout_main: offsets in g_checkout_errors, to which the type of the device is added. */
const CHECKOUT_INTERRUPTED = 2; /* cancelled, or stopped after some tracks; also an offset in g_checkout_results */
const CHECKOUT_NO_DEVICE = 3;
const CHECKOUT_COUNT_LIMIT = 4; /* a track may be checked out no more times */
const CHECKOUT_DEVICE_FULL = 6; /* the device became full after some tracks */
const CHECKOUT_NO_SPACE = 8;    /* the device has no room for the album */
const CHECKOUT_WRITE_PROTECTED = 10;
const CHECKOUT_PROHIBITED = 12; /* a track may not be checked out */

/* The errors of delete_main: offsets in g_delete_results up to DELETE_NO_TRACK. */
const DELETE_FAILED = 1;
const DELETE_NO_TRACK = 3;
const DELETE_WRITE_PROTECTED = 4;
const DELETE_NO_DEVICE = 5;

#include "player.inc"

#include "wait.inc"

#include "tag.inc"

#include <float>

/* Sleeps until a message msg arrives. */
wait_message(msg, &a, &b, &c, &d)
{
    while (!FindMessage(msg, a, b, c, d))
    {
        sleep;
    }
}

/* Returns whether str is a packed string. */
is_packed(str[])
{
    return str[0] <= 0 || str[0] > 255;
}

/* Copies the packed or unpacked string src into dest, at most maxlen cells. */
copy_string(dest[], src[], maxlen)
{
    new len = strlen(src) + 1;
    if (is_packed(src))
    {
        len = (len + 3) / 4;
    }
    len = min(len, maxlen);
    for (new i; i < len; i++)
    {
        dest[i] = src[i];
    }
}

/* Same as get_tag_attr2 of text.inc. */
get_tag_attr2(tag[], id[], attr[], value[], maxlen)
{
    if (0 == tag[0] || 0 == id[0])
    {
        return 0;
    }
    new node = domFindTag(domRoot(), tag, id, 0);
    if (node)
    {
        new len = domGetData(node, attr, value, maxlen);
        if (0 == strcmp(attr, "src") || 0 == strcmp(attr, "href"))
        {
            xmlMakeFullPath(value, maxlen);
        }
        return len;
    }
    return 0;
}

#include "misc.inc"

new g_db_name[] = "music";
new g_shadow_colors[4] = {0x5F000000, 0x5F000000, 0x5F000000, 0x5F000000}; /* the "SCL4" of the shadowed labels */

#include "string.inc"

#include "layout.inc"

#include "focus.inc"

/* Same as send_focus of dom.inc. */
send_focus(obj, target, z_offset, Float:sharp, Float:extra_w, Float:extra_h, Float:move_time, smoothing, blinking)
{
    if (!obj)
    {
        return;
    }
    SendMessage(obj, TAG("FOC0"), target, TAG(""), z_offset, sharp);
    SendMessage(obj, TAG("FOC1"), extra_w, extra_h, move_time, smoothing);
    SendMessage(obj, TAG("FOC2"), blinking, 0, 0, 0);
}

public stdmenu_curr_group; /* the MENUGROUP element of the option menu shown */

/* Makes the MENUGROUP element id the current one; returns whether it exists. */
find_menu_group(id[])
{
    stdmenu_curr_group = domFindTag(domRoot(), "MENUGROUP", id, 0);
    if (stdmenu_curr_group)
    {
        return 1;
    }
    return 0;
}

#include "validate.inc"

/* Reads the retgroup and retitem of the menu: the menu and the item to go back to; returns 0 if there are none. */
get_menu_return(ret_group[], ret_item[], group_len, item_len)
{
    if (!stdmenu_curr_group)
    {
        return 0;
    }
    if (!domGetData(stdmenu_curr_group, "retgroup", ret_group, group_len))
    {
        return 0;
    }
    if (!domGetData(stdmenu_curr_group, "retitem", ret_item, item_len))
    {
        return 0;
    }
    return 1;
}

/* Reads the label of the TITLE element of the menu; returns 0 if there is none. */
get_menu_title(label[], maxlen)
{
    if (!stdmenu_curr_group)
    {
        return 0;
    }
    new title = domFirstChild(stdmenu_curr_group, "TITLE");
    if (!title)
    {
        return 0;
    }
    if (!domGetData(title, "label", label, maxlen))
    {
        return 0;
    }
    return 1;
}

/* Returns the number of items of the current menu group, -1 without one. */
count_menu_items()
{
    new count;
    if (!stdmenu_curr_group)
    {
        return -1;
    }
    for (new node = domFirstChild(stdmenu_curr_group, "ITEM"); node; node = domNext(node, "ITEM"))
    {
        count++;
    }
    return count;
}

/* Returns the index of the item with that id in the current menu group, -1 if there is none. */
find_menu_item_index(id[])
{
    new index;
    if (!stdmenu_curr_group)
    {
        return -1;
    }
    for (new node = domFirstChild(stdmenu_curr_group, "ITEM"); node; node = domNext(node, "ITEM"))
    {
        new value[64];
        domGetData(node, "id", value, 64);
        if (!strcmp(value, id))
        {
            return index;
        }
        index++;
    }
    return -1;
}

/* Reads attribute attr of item index of the menu; returns 0 if there is no such item. */
get_item_attr(index, attr[], value[], maxlen)
{
    new i;
    if (!stdmenu_curr_group || index < 0)
    {
        return 0;
    }
    new node;
    node = domFirstChild(stdmenu_curr_group, "ITEM");
    for (; node && i < index; node = domNext(node, "ITEM"))
    {
        i++;
    }
    if (!node)
    {
        return 0;
    }
    return domGetData(node, attr, value, maxlen);
}

/*
 * Runs on the object of pop_std_menu: waits for "INIT" (manager, side, result, mode), shows the title and a
 * general_menu of the items of the MENUGROUP group (MENU_MODE_GROUP) and of those the page adds (count_extra_items),
 * and, when a push leads to another menu, sends "TITL", "GRUP", "IDTY" and "FNSH" (result, mode, selected
 * index) to the manager.
 */
public std_menu_core()
{
    new title[64];
    GetArg("title", title, 64);
    new group[64];
    GetArg("group", group, 64);
    new id[64];
    GetArg("id", id, 64);
    sleep;
    new self = Self();
    new Float:screen_w;
    new Float:screen_h;
    get_screen_size5(screen_w, screen_h);
    new manager;
    new menu_result;
    new side;
    new mode;
    {
        new init_manager;
        new init_side;
        new init_result;
        new init_mode;
        wait_message(TAG("INIT"), init_manager, init_side, init_result, init_mode);
        manager = init_manager;
        side = init_side;
        menu_result = init_result;
        mode = init_mode;
    }
    find_menu_group(group);
    if (MENU_MODE_GROUP == mode)
    {
        get_menu_title(title, 64);
    }
    sleep;
    new title_label;
    new menu;
    new scroll_bar;
    rebuild:
    new selected;
    new item_count;
    if (MENU_MODE_GROUP == mode)
    {
        item_count = item_count + count_menu_items();
        selected = find_menu_item_index(id);
        if (selected < 0)
        {
            selected = 0;
        }
    }
    sleep;
    item_count = item_count + count_extra_items(side, menu_result, mode, title, group, id, selected);
    if (item_count <= 0 || selected < 0)
    {
        goto finish;
    }
    title_label = CreateLabel(self);
    {
        new Float:x;
        new Float:y;
        get_list_title_pos(0, item_count, 1, x, y);
        SetPos(title_label, 0.0 * screen_w, screen_h * y, 0.0, 0);
        SetColor(title_label, 0.902, 0.902, 0.215, 0.0);
        AnimColor(title_label, 0.902, 0.902, 0.215, 1.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
        SetOptionI(title_label, TAG("FONT"), FontID(0));
        SetOptionS(title_label, TAG("UTF8"), title);
        SetOptionI(title_label, TAG("FSIZ"), 17);
        SetOptionI(title_label, TAG("ZSRT"), ZSORT_OVERLAY);
        SetOptionI(title_label, TAG("HALN"), ALIGN_CENTER);
        SetOptionI(title_label, TAG("VALN"), ALIGN_CENTER);
    }
    sleep;
    menu = CreateNull(self);
    new columns = 1;
    new rows = 5;
    {
        new Float:y;
        get_list_top(item_count, 1, y);
        SetPos(menu, 0.0 * screen_w, screen_h * y, 0.0, 0);
        SetOptionI(menu, TAG("ZSRT"), ZSORT_OVERLAY);
        AttachScript(menu, "general_menu", "general_menu");
        SetPubVar(menu, "manager", Self());
        SetPubVar(menu, "focus", GetPubVar(manager, "focus_glowfly"));
        SetPubVar(menu, "pad_enable", PAD_CROSS);
        rows = item_count >= 8 ? 8 : item_count;
        SendMessage(menu, TAG("INUM"), item_count, 0, 0, 0);
        SendMessage(menu, TAG("CLRW"), columns, rows, 0, 0);
        SendMessage(menu, TAG("PADL"), -1, 1, 0, 0);
        SendMessage(menu, TAG("DIST"), 0, 0.0583 * screen_h, 0, 0);
        SendMessage(menu, TAG("MGIN"), 2, 2, 0, 0);
        SendMessage(menu, TAG("MODE"), MENU_TYPE_POPUP, SCROLL_LINE, 0, 0);
        SendMessage(menu, TAG("ANIM"), 200.0, 0, ANIM_FUNC_EXPONENT, 0);
        SendMessage(menu, TAG("OFFA"), -1.0, 0, 0, 0);
        SendMessage(menu, TAG("IDLA"), 1.0, 0, 0, 0);
        SendMessage(menu, TAG("REPT"), 100.0, 100.0, 0, 0);
        SendMessage(menu, TAG("SLCT"), selected, 0, 0, 0);
        SendMessage(menu, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
    }
    sleep;
    if (item_count > rows * columns)
    {
        scroll_bar = CreateNull(self);
        new Float:bar_x;
        new Float:bar_y;
        new Float:list_x;
        new bar_side;
        if (side == MENU_SIDE_RIGHT)
        {
            bar_side = MENU_SIDE_RIGHT;
        }
        get_list_pos(bar_side, item_count, 1, list_x, bar_y);
        get_scroll_bar_pos(bar_side, item_count, 1, bar_x, bar_y);
        SetPos(scroll_bar, screen_w * (bar_x - list_x), screen_h * bar_y, 0.0, 0);
        get_list_body_height(item_count, 1, bar_y);
        SetOptionI(scroll_bar, TAG("ZSRT"), ZSORT_OVERLAY);
        AttachScript(scroll_bar, _, "scroll_bar");
        SetPubVar(scroll_bar, "manager", Self());
        SetPubVar(scroll_bar, "focus", GetPubVar(manager, "focus_glowfly"));
        SetPubVar(scroll_bar, "height", screen_h * (bar_y + 0.0583 * 2.0));
    }
    sleep;
    for (; ; )
    {
        menu_result = 0;
        new msg;
        new a;
        new b;
        new c;
        /* "ITEM" (menu, path, index): create the item of index; "PUSH" (index, button, item). */
        while (RecvMessage(msg, a, b, c))
        {
            if (msg == TAG("ITEM"))
            {
                if (a == menu)
                {
                    new index = c;
                    new label[64];
                    if (MENU_MODE_GROUP == mode)
                    {
                        get_item_attr(index, "label", label, 64);
                    }
                    fill_menu_item(side, mode, index, group, label);
                    new item = create_label_item(b, label);
                    SendMessage(a, TAG("ITEM"), item, c, 0, 0);
                }
            }
            if (msg == TAG("PUSH"))
            {
                SendMessage(menu, TAG("PUSH"), 0, 0, 0, 0);
                SendMessage(menu, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                selected = a;
                if (MENU_MODE_GROUP == mode)
                {
                    get_item_attr(selected, "id", id, 64);
                }
                menu_result = handle_menu_push(manager, side, mode, selected, b, title, group, id);
                if (MENU_MODE_GROUP == mode)
                {
                    apply_result(menu_result, selected, group, id);
                }
                if (menu_result < MENU_REOPEN)
                {
                    SendMessage(menu, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                }
            }
        }
        if (scroll_bar)
        {
            new scroll_top = GetPubVar(menu, "scroll_top");
            SetPubVar(scroll_bar, "top", float(scroll_top) / float(item_count));
            SetPubVar(scroll_bar, "length", float(rows * columns) / float(item_count));
        }
        if (menu_result >= MENU_REDRAW)
        {
            SetSystemParam(SYS_PADHELP, 0);
            SendMessage(menu, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
            break;
        }
        if (ScriptState(menu) == SCRIPT_FINISHED)
        {
            break;
        }
        sleep;
    }
    finish:
    if (menu_result >= MENU_REOPEN)
    {
        SendMessageStr(manager, TAG("TITL"), title);
        SendMessageStr(manager, TAG("GRUP"), group);
        SendMessageStr(manager, TAG("IDTY"), id);
        SendMessage(manager, TAG("FNSH"), menu_result, mode, selected, 0);
    }
    if (menu)
    {
        SendMessage(menu, TAG("FADE"), 0, 300.0, 0, 0);
    }
    if (title_label)
    {
        AnimColor(title_label, 0.902, 0.902, 0.215, 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    }
    wait_ms(300);
    if (menu)
    {
        Destroy(menu);
        menu = 0;
    }
    if (scroll_bar)
    {
        Destroy(scroll_bar);
        scroll_bar = 0;
    }
    if (title_label)
    {
        Destroy(title_label);
        title_label = 0;
    }
    if (menu_result == MENU_REDRAW)
    {
        goto rebuild;
    }
    return selected;
}

/*
 * Shows the option menu of the MENUGROUP group (prefix is not used) with item select selected, on the left of
 * the screen (flags & 0x3 is 0 or 2) or on the right (1), over a dark backdrop if flags & 0x300, and the menus
 * it leads to; returns the result, MENU_CLOSED or MENU_ITEM + the chosen index.
 */
pop_std_menu(prefix[], group[], flags, select)
{
    new menu_result;
    new mode;
    new side = flags & 0x3;
    new Float:screen_w;
    new Float:screen_h;
    get_screen_size5(screen_w, screen_h);
    new menu_obj = CreateNull(Disp2D(0));
    if (MENU_SIDE_LEFT == side || side == 2)
    {
        new Float:x;
        new Float:y;
        get_list_pos(MENU_SIDE_LEFT, 5, 1, x, y);
        SetPos(menu_obj, screen_w * x, 0.0, 0.0, 0);
    }
    else if (side == MENU_SIDE_RIGHT)
    {
        new Float:x;
        new Float:y;
        get_list_pos(MENU_SIDE_RIGHT, 5, 1, x, y);
        SetPos(menu_obj, screen_w * x, 0.0, 0.0, 0);
    }
    new backdrop;
    new backdrop_kind = flags & 0x300;
    if (0 != backdrop_kind)
    {
        backdrop = CreatePlane(Disp2D(0));
        if (MENU_SIDE_LEFT == side)
        {
            SetPos(backdrop, 0.25 * screen_w, 0.5 * screen_h, 0.0, 0);
        }
        else if (side == MENU_SIDE_RIGHT)
        {
            SetPos(backdrop, 0.75 * screen_w, 0.5 * screen_h, 0.0, 0);
        }
        SetLocalScale(backdrop, 0.5 * screen_w, 2.0 * (0.364 * screen_h), 0.0);
        SetColor(backdrop, 0.0, 0.0, 0.0, 0.0);
        AnimColor(backdrop, 0.0, 0.0, 0.0, 0.3, 500.0, 0.0, ANIM_FUNC_LINEAR);
        SetOptionI(backdrop, TAG("ZSRT"), ZSORT_BACKGROUND);
        if (backdrop_kind == MENU_BACKDROP_FADED)
        {
            SetOptionI(backdrop, TAG("FDEG"), 1);
        }
    }
    new cur_title[64];
    new cur_group[64];
    new cur_id[64];
    copy_string(cur_group, prefix, 64);
    copy_string(cur_group, group, 64);
    find_menu_group(cur_group);
    if (!get_item_attr(select, "id", cur_id, 64))
    {
        cur_id = "";
    }
    sleep;
    for (; ; )
    {
        new args[256];
        sprintf(args, "title = %s ; group = %s ; id = %s", cur_title, cur_group, cur_id);
        AttachScript(menu_obj, "std_menu_core", "", args);
        SendMessage(menu_obj, TAG("INIT"), Self(), side, menu_result, mode);
        for (; ; )
        {
            new msg;
            new a;
            new b;
            new c;
            new str[64];
            while (RecvMessageStr(msg, str, 64))
            {
                if (msg == TAG("TITL"))
                {
                    copy_string(cur_title, str, 64);
                }
                if (msg == TAG("GRUP"))
                {
                    copy_string(cur_group, str, 64);
                }
                if (msg == TAG("IDTY"))
                {
                    copy_string(cur_id, str, 64);
                }
            }
            if (FindMessage(TAG("FNSH"), a, b, c))
            {
                menu_result = a;
                mode = b;
            }
            if (ScriptState(menu_obj) == SCRIPT_FINISHED)
            {
                break;
            }
            sleep;
        }
        if (menu_result >= MENU_CANCEL)
        {
            break;
        }
    }
    if (0 != backdrop)
    {
        AnimColor(backdrop, 0.0, 0.0, 0.0, 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
        wait_ms(300);
        Destroy(backdrop);
    }
    if (menu_obj)
    {
        Destroy(menu_obj);
    }
    return menu_result;
}

/* Sets the group and id of the menu that menu_result leads to from item index; MENU_CLOSED if there is none. */
apply_result(&menu_result, index, group[], id[])
{
    if (menu_result == MENU_RETURN)
    {
    }
    else if (menu_result == MENU_SUBMENU)
    {
        new sub_group[64];
        get_item_attr(index, "subgroup", sub_group, 64);
        printf("%s -------------\n", sub_group);
        if (!strcmp(sub_group, ""))
        {
            printf("WARNING: nothing sub item !!\n");
        }
        else
        {
            set_menu_return(sub_group, group, id);
            copy_string(group, sub_group, 64);
        }
    }
    else if (menu_result == MENU_BACK)
    {
        new ret_group[64];
        new ret_item[64];
        get_menu_return(ret_group, ret_item, 64, 64);
        printf(" %d, %s, %s, %s, %s \n", index, group, id, ret_group, ret_item);
        if (!strcmp(ret_group, "") || !strcmp(ret_item, ""))
        {
            printf("WARNING: nothing return menu !!\n");
            menu_result = MENU_CLOSED;
        }
        else
        {
            copy_string(group, ret_group, 64);
            copy_string(id, ret_item, 64);
        }
    }
}

/* Creates a shadowed label item of an option menu. */
create_label_item(parent, label[])
{
    new item;
    item = CreateLabelShadow(parent);
    SetOptionP(item, TAG("SCL4"), g_shadow_colors, 4);
    SetOptionI(item, TAG("SX64"), 2 * 64);
    SetOptionI(item, TAG("SY64"), 2 * 64);
    SetColor(item, 0.215, 0.708, 0.999, 0.0);
    SetOptionI(item, TAG("ALPH"), BLEND_ADD);
    SetOptionI(item, TAG("FONT"), FontID(0));
    SetOptionS(item, TAG("UTF8"), label);
    SetOptionI(item, TAG("FSIZ"), 15);
    SetOptionI(item, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(item, TAG("VALN"), ALIGN_CENTER);
    AttachScript(item, "label_item", "label_item");
    SetPubVar(item, "pad_enable", PAD_CIRCLE);
    return item;
}

/* Sets the variable name to the number value. */
set_aval_int(name[], value)
{
    new text[16];
    sprintf(text, "%d", value);
    avalSet(name, text);
}

public idx; /* unused, but part of the layout */
public focus; /* the focus object of the dialogs */
public focus_glowfly; /* the focus object of the option menus */
public result; /* set when the albums changed, read by the page */
/* Lines of TEXT elements, joined into g_msg or shown in a progress window. */
new g_line1[256];
new g_line2[256];
new g_msg[512];
new g_detail[512];
new g_yes[64];
new g_no[64];
new g_ok[64];
new g_cancel[64];
new g_percent_format[64];
new Float:g_screen_w;
new Float:g_screen_h;
/* The messages of ripping_main, by its error. */
new g_rip_results[9][] = {
    "msgmusic_addalbum_complete", "msgmusic_addalbum_fail", "msgmusic_addalbum_import",
    "msgmusic_addalbum_nospace", "msgmusic_addalbum_max", "msgmusic_addalbum_notreading",
    "msgmusic_addalbum_rec_cancel_complete", "msgmusic_addalbum_recarea_full",
    "msgmusic_addalbum_warn_max"
};

/* Returns the number in the variable name, 0 if there is none. */
get_aval_int(name[])
{
    new text[16];
    if (0 != avalGetValue(name, 0, text, 16))
    {
        return 0;
    }
    return atoi(text);
}

/*
 * Rips the audio CD into a new album of the hard disk, after asking: first creates the recording area of the disk if
 * the player has none, then shows the progress until the rip ends or is cancelled.
 */
public ripping_main()
{
    new progress_window;
    SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
    result = 0;
    new error;
    get_screen_size5(g_screen_w, g_screen_h);
    get_tag_attr2("TEXT", "msgmusic_common_yes", "value", g_yes, 64);
    get_tag_attr2("TEXT", "msgmusic_common_no", "value", g_no, 64);
    get_tag_attr2("TEXT", "msgmusic_common_ok", "value", g_ok, 64);
    get_tag_attr2("TEXT", "msgmusic_common_cancel", "value", g_cancel, 64);
    get_tag_attr2("TEXT", "msgmusic_addalbum_percent", "value", g_percent_format, 64);
    get_tag_attr2("TEXT", "msgmusic_addalbum_ask", "value", g_line1, 256);
    new answer = ask_yes_no(g_line1, g_yes, g_no, 0);
    if (0 != answer)
    {
        goto finish;
    }
    new album_count = get_aval_int("MusicAlbumNum");
    printf("+++++++++++++++ ALBUMNUM = %d\n", album_count);
    if (album_count >= MAX_ITEMS)
    {
        error = RIP_ALBUMS_FULL;
        goto show_result;
    }
    new need_area;
    playerExecCommand(PLAYER_GET_AREA_STATE, need_area);
    printf("ripping status = %d\n", need_area);
    if (need_area)
    {
        get_tag_attr2("TEXT", "msgmusic_addalbum_recarea", "value", g_line1, 256);
        get_tag_attr2("TEXT", "msgmusic_warn_console_power", "value", g_line2, 256);
        progress_window = open_progress("", g_line1, g_line2, g_percent_format);
        playerExecCommand(PLAYER_MAKE_AREA);
        result = 1;
        new frame;
        while (result > 0)
        {
            playerExecCommand(PLAYER_PD_GET_RESULT, result);
            frame = clamp(frame, 0, 1500);
            SetPubVar(progress_window, "progress", frame / 15);
            frame++;
            sleep;
        }
        printf("====== %d\n", frame);
        if (result == -1)
        {
            printf("MAKING PARTITION FAILED\n");
            error = RIP_AREA_FULL;
            goto close_progress;
        }
        send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        Destroy(progress_window);
    }
    printf("ripping\n");
    result = 1;
    get_tag_attr2("TEXT", "msgmusic_addalbum_rec", "value", g_line1, 256);
    get_tag_attr2("TEXT", "msgmusic_warn_console_disctray", "value", g_line2, 256);
    get_tag_attr2("TEXT", "msgmusic_warn_console_power", "value", g_msg, 512);
    progress_window = open_progress(g_line1, g_line2, g_msg, g_percent_format, g_cancel);
    new rip_state;
    new rip_track;
    new percent;
    new rip_error;
    new drive_state;
    new idle_count;
    new unit;
    new last_state;
    playerExecCommand(PLAYER_SET_DIN_THRU, 0);
    unit = PLAYER_UNIT_CD;
    last_state = PLAYER_STATE_STOPPED;
    /* Waits until the drive has been stopped or idle for four frames. */
    while (1)
    {
        playerExecCommand(PLAYER_GET_UNIT_STATE, unit, drive_state, 0, 0);
        if (PLAYER_STATE_STOPPED != drive_state && PLAYER_STATE_IDLE != drive_state)
        {
            if (last_state != drive_state)
            {
                playerExecCommand(PLAYER_STOP, unit, 0, 0);
                last_state = drive_state;
            }
            printf("WAIT:ripping  %d\n", drive_state);
            idle_count = 0;
        }
        else
        {
            printf("WAIT:ripping  %d\n", drive_state);
            last_state = drive_state;
            idle_count++;
            if (idle_count > 3)
            {
                break;
            }
        }
        sleep;
    }
    playerExecCommand(PLAYER_RIP_START, rip_error, 0, 0);
    if (rip_error == RIP_START_NO_SPACE)
    {
        printf("RIPPING FAILED\n");
        error = RIP_NO_SPACE;
        wait_ms(2400);
        goto close_progress;
    }
    if (rip_error == RIP_START_NO_AREA)
    {
        printf("MAKING PARTITION FAILED\n");
        error = RIP_AREA_FULL;
        goto close_progress;
    }
    new stall_frames;
    for (; ; )
    {
        if (ScriptState(progress_window) == SCRIPT_FINISHED)
        {
            error = RIP_CANCELLED;
            playerExecCommand(PLAYER_RIP_CANCEL, 0, 0, 0);
            break;
        }
        playerExecCommand(PLAYER_RIP_GET_STATE, rip_state, rip_track, percent);
        if (rip_state < 0)
        {
            error = RIP_FAILED;
            break;
        }
        if (percent >= 100)
        {
            break;
        }
        new shown = percent;
        if (0 == shown)
        {
            stall_frames++;
            if (stall_frames >= 900)
            {
                shown = stall_frames / 900;
            }
        }
        shown = clamp(shown, 0, 100);
        SetPubVar(progress_window, "progress", shown);
        sleep;
    }
    close_progress:
    send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    SetPubVar(progress_window, "progress", 100);
    sleep;
    Destroy(progress_window);
    show_result:
    new back_label[64];
    get_tag_attr2("TEXT", "msgmusic_btn_back", "value", back_label, 64);
    SetSystemParamStr(SYS_STR_PADHELP_RD, back_label);
    if (RIP_COMPLETE == error)
    {
        album_count++;
        set_aval_int("MusicAlbumNum", album_count);
    }
    if (error == RIP_NO_SPACE)
    {
        get_tag_attr2("TEXT", g_rip_results[RIP_FAILED], "value", g_line2, 256);
        get_tag_attr2("TEXT", g_rip_results[error], "value", g_msg, 512);
        sprintf(g_line1, "%s\n\n%s", g_line2, g_msg);
    }
    else
    {
        get_tag_attr2("TEXT", g_rip_results[error], "value", g_line1, 256);
    }
    show_message(g_line1);
    finish:
    playerExecCommand(PLAYER_SET_DIN_THRU, 1);
    SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
}

/* The messages of favorite_main, by its error. */
new g_favorite_results[5][] = {
    "msgmusic_regtrack_complete", "msgmusic_regtrack_fail", "msgmusic_regtrack_import",
    "msgmusic_regtrack_nospace", "msgmusic_regtrack_max"
};

/*
 * Adds the track of ContentURI in the aval file to the favorite album chosen in a menu of the favorite albums
 * (argument album_mode: the album the track is played from).
 */
public favorite_main()
{
    result = 0;
    new error;
    get_screen_size5(g_screen_w, g_screen_h);
    new arg[64];
    new album_mode = MUSIC_ALBUM_COLLECTION;
    if (GetArg("album_mode", arg, 64))
    {
        album_mode = atoi(arg);
    }
    get_tag_attr2("TEXT", "msgmusic_common_yes", "value", g_yes, 64);
    get_tag_attr2("TEXT", "msgmusic_common_no", "value", g_no, 64);
    get_tag_attr2("TEXT", "msgmusic_common_ok", "value", g_ok, 64);
    focus_glowfly = create_focus_plane2("focus");
    new favorite = pop_std_menu("optmenu_favorite", "optmenu_favorite", MENU_SIDE_RIGHT, 0);
    favorite = favorite - MENU_ITEM;
    if (favorite < 0)
    {
        goto finish;
    }
    printf("add to favorite %d\n", favorite);
    get_tag_attr2("TEXT", "msgmusic_regtrack_add", "value", g_line1, 256);
    new progress_window = open_progress("", "", g_line1);
    new uri[256];
    avalGetValue("ContentURI", 0, uri, 256);
    printf("+++++++++++ CONTENTURI = %s\n", uri);
    new sql[512];
    new status;
    status = datOpen(DAT_DATABASE, g_db_name, DAT_ORDER_MTIME_DESC);
    {
        new i;
        for (; i <= 30; i++)
        {
            SetPubVar(progress_window, "progress", i);
            sleep;
        }
    }
    status = wait_database();
    sprintf(sql, "SELECT entrydate FROM scei_jukebox_favorite_color WHERE favorite = %d", favorite + 1);
    status = datSQLExec(sql);
    if (0 != status)
    {
        error = FAVORITE_FAILED;
        goto close_db;
    }
    {
        new i = 30;
        for (; i <= 50; i++)
        {
            SetPubVar(progress_window, "progress", i);
            sleep;
        }
    }
    status = wait_database();
    if (status < 0)
    {
        error = FAVORITE_FAILED;
        goto close_db;
    }
    {
        new i = 50;
        for (; i <= 75; i++)
        {
            SetPubVar(progress_window, "progress", i);
            sleep;
        }
    }
    new count = datGetNum();
    if (count >= MAX_ITEMS)
    {
        error = FAVORITE_MAX;
        goto close_db;
    }
    new hour;
    new minute;
    new second;
    new year;
    new month;
    new day;
    time(hour, minute, second, 0);
    date(year, month, day, 0);
    sprintf(sql,
            "INSERT INTO scei_jukebox_favorite_color (url, favorite, entrydate) VALUES ('%s', %d, '%04d-%02d-%02d %02d:%02d:%02d')",
            uri, favorite + 1, year, month, day, hour, minute, second);
    status = datSQLExec(sql);
    if (0 != status)
    {
        error = FAVORITE_FAILED;
        goto close_db;
    }
    {
        new i = 75;
        for (; i <= 100; i++)
        {
            SetPubVar(progress_window, "progress", i);
            sleep;
        }
    }
    status = wait_database();
    if (status < 0)
    {
        error = FAVORITE_FAILED;
        goto close_db;
    }
    close_db:
    datClose();
    Destroy(progress_window);
    new album_no = get_aval_int("MusicAlbumNo");
    if (MUSIC_ALBUM_FAVORITE == album_mode)
    {
        playerExecCommand(PLAYER_SET_FAVORITE, album_no + 1);
        if (album_no == favorite)
        {
            result = 1;
        }
    }
    show_result:
    get_tag_attr2("TEXT", g_favorite_results[error], "value", g_line1, 256);
    show_message(g_line1);
    finish:
    Destroy(focus_glowfly);
}

/* The texts of checkin_main, by MusicChannelMode; the errors by 6 * channel mode + error + type of the device. */
new g_checkin_asks[2][7] = {"msgmusic_album_chkin_ask", "msgmusic_track_chkin_ask"};
new g_checkin_cannots[2][7] = {"msgmusic_album_chkin_cannot", "msgmusic_track_chkin_cannot"};
new g_checkin_titles[2][6] = {"msgmusic_album_chkin", "msgmusic_track_chkin"};
new g_checkin_results[4][] = {
    "msgmusic_album_chkin_complete", "msgmusic_album_chkin_fail", "msgmusic_track_chkin_complete",
    "msgmusic_track_chkin_fail"
};
new g_checkin_errors[12][] = {
    "dummy", "dummy", "msgmusic_album_chkin_fail_cinfonone", "msgmusic_device_connect_confirm",
    "msgmusic_chkin_wdisable", "msgmusic_chkin_wdisable", "dummy", "dummy",
    "msgmusic_track_chkin_fail_cinfonone", "msgmusic_device_connect_confirm",
    "msgmusic_chkin_wdisable", "msgmusic_chkin_wdisable"
};

/*
 * Checks in the tracks of CheckoutTrack in the aval file from the portable device, after asking: -n is all the n
 * tracks of the device, n the track n (argument channel_mode: an album or a track).
 */
public checkin_main()
{
    result = 0;
    new track;
    track = get_aval_int("CheckoutTrack");
    new error;
    get_screen_size5(g_screen_w, g_screen_h);
    new arg[64];
    new channel_mode;
    if (GetArg("channel_mode", arg, 64))
    {
        channel_mode = atoi(arg);
    }
    get_tag_attr2("TEXT", "msgmusic_common_yes", "value", g_yes, 64);
    get_tag_attr2("TEXT", "msgmusic_common_no", "value", g_no, 64);
    get_tag_attr2("TEXT", "msgmusic_common_ok", "value", g_ok, 64);
    get_tag_attr2("TEXT", "msgmusic_addalbum_percent", "value", g_percent_format, 64);
    get_tag_attr2("TEXT", g_checkin_asks[channel_mode], "value", g_line1, 256);
    get_tag_attr2("TEXT", g_checkin_cannots[channel_mode], "value", g_line2, 256);
    sprintf(g_msg, "%s\n\n%s", g_line1, g_line2);
    new answer = ask_yes_no(g_msg, g_yes, g_no, 0);
    if (0 != answer)
    {
        goto finish;
    }
    new last_track;
    new ticks;
    new base_progress;
    new frame;
    new first_track = 1;
    new progress2;
    new partial;
    new pd_type;
    pd_type = get_aval_int("PDType");
    printf("checkin\n");
    result = 1;
    new progress_window;
    new pd_tracks = -1;
    new media_changed = get_aval_int("MusicMediaChanged");
    if (media_changed == 2)
    {
        printf("media changed\n");
        error = CHECKIN_NO_DEVICE;
        set_aval_int("MusicMediaChanged", 0);
        pd_tracks = -2;
        goto show_result;
    }
    ticks = 0;
    frame = 0;
    get_tag_attr2("TEXT", g_checkin_titles[channel_mode], "value", g_line1, 256);
    get_tag_attr2("TEXT", "msgmusic_warn_external_device_eject", "value", g_line2, 256);
    get_tag_attr2("TEXT", "msgmusic_warn_console_power", "value", g_msg, 512);
    progress_window = open_progress(g_line1, g_line2, g_msg, g_percent_format);
    base_progress = 0;
    last_track = first_track = abs(track);
    new tries;
    new ready = -1;
    while (ready < 0)
    {
        if (0 == tries % 6)
        {
            playerExecCommand(PLAYER_GET_STATUS, ready);
        }
        tries++;
        if (tries >= 3000)
        {
            break;
        }
        sleep;
    }
    if (last_track > 0)
    {
        for (new t = last_track; t >= first_track; t--)
        {
            if (track < 0)
            {
                playerExecCommand(PLAYER_PD_CHECKIN, 0, 0, 0);
            }
            else
            {
                playerExecCommand(PLAYER_PD_CHECKIN, 0, t);
            }
            SetPubVar(progress_window, "progress", t * 80 / last_track);
            result = 1;
            ticks = 0;
            new slow_frames;
            new shown;
            while (0 != result)
            {
                if (frame == 60)
                {
                    playerExecCommand(PLAYER_PD_GET_RESULT, result);
                    if (result < 0)
                    {
                        printf("checkin break = %d\n", result);
                        break;
                    }
                    frame = 0;
                }
                else if (0 == frame % 15)
                {
                    ticks++;
                    progress2 = 0;
                    if (track < 0)
                    {
                        new scaled;
                        scaled = ticks * 2;
                        if (scaled >= -track * 80)
                        {
                            scaled = -track * 80;
                        }
                        base_progress = scaled / -track;
                    }
                    else
                    {
                        ticks = clamp(ticks, 0, 80);
                        base_progress = ticks;
                    }
                }
                else
                {
                    progress2 = progress2 + 5;
                    progress2 = clamp(progress2, 0, 99);
                }
                frame++;
                if (base_progress == 80)
                {
                    if (shown > 85)
                    {
                        if (slow_frames == 240)
                        {
                            shown++;
                            slow_frames = 0;
                        }
                    }
                    else if (slow_frames == 120)
                    {
                        shown++;
                        slow_frames = 0;
                    }
                    slow_frames++;
                }
                else
                {
                    shown = base_progress;
                }
                shown = clamp(shown, 0, 90);
                SetPubVar(progress_window, "progress", shown);
                SetPubVar(progress_window, "progress2", progress2);
                sleep;
            }
        }
        ticks = 90;
        SetPubVar(progress_window, "progress", ticks);
        SetPubVar(progress_window, "progress2", 0);
        if (0 == result)
        {
            playerExecCommand(PLAYER_PD_CHECK, 0);
            new pd_ready = -1;
            frame = 0;
            while (pd_ready < 0)
            {
                playerExecCommand(PLAYER_GET_STATUS, pd_ready);
                if (++frame == 60)
                {
                    ticks++;
                    ticks = clamp(ticks, 0, 100);
                    frame = 0;
                }
                SetPubVar(progress_window, "progress", ticks);
                sleep;
            }
            playerExecCommand(PLAYER_PD_GET_INFO, pd_tracks, pd_type, 0);
            set_aval_int("PDTracks", pd_tracks);
            if (track < 0)
            {
                if (pd_tracks > 0)
                {
                    printf("===== some tracks can't be checked-in = %d\n", pd_tracks);
                    error = CHECKIN_PARTIAL;
                }
            }
            else
            {
                result = 1;
            }
        }
        else
        {
            pd_tracks = -1;
            printf("=============== FAILED %d\n", result);
            if (track < 0)
            {
                if (result == PD_ERR_NO_INFO)
                {
                    error = CHECKIN_PARTIAL;
                }
                else if (result == PD_ERR_WRITE_PROTECTED)
                {
                    error = CHECKIN_WRITE_PROTECTED;
                }
                else
                {
                    error = CHECKIN_NO_DEVICE;
                }
            }
            else if (result == PD_ERR_NO_INFO)
            {
                error = CHECKIN_PARTIAL;
            }
            else if (result == PD_ERR_WRITE_PROTECTED)
            {
                error = CHECKIN_WRITE_PROTECTED;
            }
            else
            {
                error = CHECKIN_NO_DEVICE;
            }
        }
    }
    else
    {
        result = -1;
        pd_tracks = -1;
        error = CHECKIN_NO_DEVICE;
        get_tag_attr2("TEXT", "msgmusic_album_chkin_track_none", "value", g_msg, 512);
        Destroy(progress_window);
        goto show;
    }
    if (result > 0 && partial == 1) /* redundant as written */
    {
        error = CHECKIN_PARTIAL;
    }
    SetPubVar(progress_window, "progress", 100);
    while (SCRIPT_FINISHED != ScriptState(progress_window))
    {
        sleep;
    }
    Destroy(progress_window);
    show_result:
    result = 1;
    if (0 != pd_type && 1 != pd_type)
    {
        pd_type = 0;
    }
    if (0 != error)
    {
        get_tag_attr2("TEXT", g_checkin_results[channel_mode * 2 + 1], "value", g_line1, 256);
        get_tag_attr2("TEXT", g_checkin_errors[channel_mode * 6 + error + pd_type], "value", g_line2, 256);
        if ((track < 0 && error == CHECKIN_PARTIAL) || 0 == last_track)
        {
            if (pd_tracks == -1)
            {
                get_tag_attr2("TEXT", g_checkin_results[channel_mode * 2 + 1], "value", g_line1, 256);
                get_tag_attr2("TEXT", "msgmusic_album_chkin_fail_otherdev", "value", g_detail, 512);
            }
            else if (pd_tracks < -1)
            {
                get_tag_attr2("TEXT", g_checkin_results[channel_mode * 2 + 1], "value", g_line1, 256);
                get_tag_attr2("TEXT", "msgmusic_device_connect_confirm", "value", g_detail, 512);
            }
            else
            {
                get_tag_attr2("TEXT", g_checkin_results[channel_mode * 2 + 0], "value", g_line1, 256);
                sprintf(g_detail, g_line2, pd_tracks);
            }
            sprintf(g_msg, "%s\n\n%s", g_line1, g_detail);
        }
        else
        {
            sprintf(g_msg, "%s\n\n%s", g_line1, g_line2);
        }
        goto show;
    }
    get_tag_attr2("TEXT", g_checkin_results[channel_mode * 2 + 0], "value", g_msg, 512);
    show:
    show_message(g_msg);
    finish:
}

/* The texts of checkout_main, by MusicChannelMode; the errors by 14 * channel mode + error + type of the device. */
new g_checkout_asks[2][7] = {"msgmusic_album_chkout_ask", "msgmusic_track_chkout_ask"};
new g_checkout_notices[2][9] = {"msgmusic_album_chkout_announce_md", "msgmusic_track_chkout_announce_md"};
new g_checkout_titles[2][6] = {"msgmusic_album_chkout", "msgmusic_track_chkout"};
new g_checkout_results[6][] = {
    "msgmusic_album_chkout_complete", "msgmusic_album_chkout_fail",
    "msgmusic_chkout_cancel_complete", "msgmusic_track_chkout_complete",
    "msgmusic_track_chkout_fail", "msgmusic_chkout_cancel_complete"
};
new g_checkout_errors[28][] = {
    "dummy", "dummy", "dummy", "msgmusic_device_connect_confirm",
    "msgmusic_album_chkout_fail_cntlimit", "msgmusic_album_chkout_fail_cntlimit",
    "msgmusic_chkout_fail_halfway", "msgmusic_chkout_fail_halfway", "msgmusic_chkout_fail_space",
    "msgmusic_chkout_fail_space", "msgmusic_chkout_fail_wdisable", "msgmusic_chkout_fail_wdisable",
    "msgmusic_album_chkout_fail_trkcp_prohibit", "msgmusic_album_chkout_fail_trkcp_prohibit",
    "dummy", "dummy", "dummy", "msgmusic_device_connect_confirm",
    "msgmusic_track_chkout_fail_cntlimit", "msgmusic_track_chkout_fail_cntlimit",
    "msgmusic_chkout_fail_space", "msgmusic_chkout_fail_space", "msgmusic_chkout_fail_space",
    "msgmusic_chkout_fail_space", "msgmusic_chkout_fail_wdisable", "msgmusic_chkout_fail_wdisable",
    "msgmusic_track_chkout_prohibit", "msgmusic_track_chkout_prohibit"
};

/*
 * Checks out the tracks of CheckoutTrack in the aval file to the portable device, after asking: -n is the tracks 1 to
 * n of the album, n the track n. The cross in the progress window cancels after the current track.
 */
public checkout_main()
{
    result = 0;
    new error;
    get_screen_size5(g_screen_w, g_screen_h);
    new track;
    track = get_aval_int("CheckoutTrack");
    new arg[64];
    new channel_mode;
    if (GetArg("channel_mode", arg, 64))
    {
        channel_mode = atoi(arg);
    }
    new album_mode = MUSIC_ALBUM_COLLECTION;
    if (GetArg("album_mode", arg, 64))
    {
        album_mode = atoi(arg);
    }
    get_tag_attr2("TEXT", "msgmusic_common_yes", "value", g_yes, 64);
    get_tag_attr2("TEXT", "msgmusic_common_no", "value", g_no, 64);
    get_tag_attr2("TEXT", "msgmusic_common_ok", "value", g_ok, 64);
    get_tag_attr2("TEXT", "msgmusic_common_cancel", "value", g_cancel, 64);
    get_tag_attr2("TEXT", "msgmusic_addalbum_percent", "value", g_percent_format, 64);
    get_tag_attr2("TEXT", g_checkout_asks[channel_mode], "value", g_line1, 256);
    get_tag_attr2("TEXT", g_checkout_notices[channel_mode], "value", g_line2, 256);
    sprintf(g_msg, "%s\n\n%s", g_line1, g_line2);
    new answer = ask_yes_no(g_msg, g_yes, g_no, 0);
    if (0 != answer)
    {
        goto finish;
    }
    printf("checkout\n");
    result = 1;
    new last_track;
    new first_track = 1;
    new percent;
    new progress2;
    new shown;
    new frame;
    new cancel;
    new progress_window;
    if (track < 0)
    {
        last_track = -track;
        first_track = 1;
    }
    else
    {
        last_track = track;
        first_track = track;
    }
    get_tag_attr2("TEXT", g_checkout_titles[channel_mode], "value", g_line1, 256);
    get_tag_attr2("TEXT", "msgmusic_warn_external_device_eject", "value", g_line2, 256);
    get_tag_attr2("TEXT", "msgmusic_warn_console_power", "value", g_msg, 512);
    progress_window = open_progress(g_line1, g_line2, g_msg, g_percent_format, g_cancel, "break");
    new tries;
    new ready = -1;
    while (ready < 0)
    {
        if (0 == tries % 6)
        {
            playerExecCommand(PLAYER_GET_STATUS, ready);
        }
        tries++;
        if (tries >= 3000)
        {
            printf("timeout\n");
            break;
        }
        sleep;
    }
    new pd_type;
    new pd_tracks = -1;
    new write_disabled;
    new check_mode;
    new done_count;
    tries = 0;
    ready = -1;
    playerExecCommand(PLAYER_PD_CHECK, check_mode);
    while (ready < 0)
    {
        if (0 == tries % 6)
        {
            playerExecCommand(PLAYER_GET_STATUS, ready);
        }
        tries++;
        if (tries >= 3000)
        {
            printf("timeout\n");
            break;
        }
        sleep;
    }
    if (tries < 3000)
    {
        playerExecCommand(PLAYER_PD_GET_INFO, pd_tracks, pd_type, write_disabled);
        set_aval_int("PDTracks", pd_tracks);
        set_aval_int("PDType", pd_type);
    }
    else
    {
        pd_tracks = -1;
        set_aval_int("PDTracks", pd_tracks);
    }
    printf("+++++++++++++++++ PDTYPE %d, PDTRACKS = %d\n", pd_type, pd_tracks);
    if (0 != pd_type && 1 != pd_type)
    {
        pd_type = 0;
    }
    if (pd_tracks >= 0)
    {
        if (write_disabled)
        {
            error = CHECKOUT_WRITE_PROTECTED;
            goto close_progress;
        }
        /* The device takes 255 tracks, or 400 for type 1. */
        if ((0 == pd_type && pd_tracks + (last_track - first_track + 1) > 255)
                || (pd_type == 1 && pd_tracks + (last_track - first_track + 1) > 400))
        {
            error = CHECKOUT_DEVICE_FULL;
            goto close_progress;
        }
    }
    else if (pd_tracks < 0)
    {
        error = CHECKOUT_NO_DEVICE;
        goto close_progress;
    }
    set_aval_int("ProgressCancelFlag", 0);
    frame = 0;
    cancel = 0;
    shown = 0;
    if (last_track > 0)
    {
        {
            new t = first_track;
            for (; t <= last_track; t++)
            {
                percent = 0;
                cancel = get_aval_int("ProgressCancelFlag");
                if (!cancel)
                {
                    playerExecCommand(PLAYER_PD_CHECKOUT, 0, t);
                }
                else
                {
                    error = CHECKOUT_INTERRUPTED;
                    progress2 = 0;
                }
                result = 0;
                while (100 != percent)
                {
                    if (2 != cancel)
                    {
                        cancel = get_aval_int("ProgressCancelFlag");
                    }
                    if (cancel == 1)
                    {
                        get_tag_attr2("TEXT", "msgmusic_chkout_cancel", "value", g_line1, 256);
                        SendMessageStr(progress_window, TAG("MESG"), g_line1);
                        if (progress2 >= 0)
                        {
                            error = CHECKOUT_INTERRUPTED;
                            playerExecCommand(PLAYER_CANCEL);
                            cancel = 2;
                            if (0 == percent)
                            {
                                break;
                            }
                        }
                        else
                        {
                            cancel = 0;
                        }
                    }
                    if (0 == frame % 30)
                    {
                        new track_percent;
                        playerExecCommand(PLAYER_PD_GET_PROGRESS, track_percent, progress2);
                        if (track_percent == 100 && cancel >= 1)
                        {
                            done_count++;
                        }
                        if ((0 == track_percent || track_percent >= 90) && frame == 150)
                        {
                            percent++;
                            percent = clamp(percent, 0, 99);
                        }
                        else if (track_percent > percent)
                        {
                            percent = track_percent;
                        }
                        if (cancel)
                        {
                            if (percent > shown)
                            {
                                shown = percent;
                            }
                        }
                        else
                        {
                            shown = ((t - first_track) * 100 + percent) / (last_track - first_track + 1);
                        }
                        playerExecCommand(PLAYER_PD_GET_RESULT, result);
                        if (result < 0 || result == 2)
                        {
                            percent = 100;
                        }
                        if (frame == 150)
                        {
                            frame = 0;
                        }
                    }
                    SetPubVar(progress_window, "progress", shown);
                    SetPubVar(progress_window, "progress2", progress2);
                    frame++;
                    sleep;
                }
                printf("+++ COMMA %d\n", progress2);
                if (error == CHECKOUT_INTERRUPTED && progress2 < 0)
                {
                    error = 0;
                }
                if (cancel >= 1)
                {
                    break;
                }
                printf("++++++++++++++++++ FINISH = %d\n", t);
                if (result == PD_ERR_NO_SPACE)
                {
                    error = CHECKOUT_DEVICE_FULL;
                    printf("+++++++++++= INCOMPLETE %d\n", result);
                    break;
                }
                else if (result == PD_ERR_WRITE_PROTECTED)
                {
                    if (done_count > 0)
                    {
                        error = CHECKOUT_INTERRUPTED;
                    }
                    else
                    {
                        error = CHECKOUT_WRITE_PROTECTED;
                    }
                    printf("+++++++++++= INCOMPLETE %d\n", result);
                    break;
                }
                else if (result < 0)
                {
                    printf("++ err code = %d\n", result);
                    if (track < 0)
                    {
                        if (result <= PD_ERR_COUNT_LIMIT && result >= PD_ERR_COUNT_LIMIT_LAST)
                        {
                            error = CHECKOUT_COUNT_LIMIT;
                        }
                        else if (result == PD_ERR_PROHIBITED)
                        {
                            error = CHECKOUT_PROHIBITED;
                        }
                        else if (done_count > 0)
                        {
                            error = CHECKOUT_INTERRUPTED;
                        }
                        else
                        {
                            error = CHECKOUT_NO_DEVICE;
                        }
                        break;
                    }
                    else if (result <= PD_ERR_COUNT_LIMIT && result >= PD_ERR_COUNT_LIMIT_LAST)
                    {
                        error = CHECKOUT_COUNT_LIMIT;
                    }
                    else if (result == PD_ERR_PROHIBITED)
                    {
                        error = CHECKOUT_PROHIBITED;
                    }
                    else
                    {
                        error = CHECKOUT_NO_DEVICE;
                    }
                }
                else if (result >= 0)
                {
                    printf("+++++++++++++++ SUCCESS %d\n", result);
                    done_count++;
                }
            }
        }
    }
    printf("+++++++++++++++++ COMPLETE %d/%d\n", done_count, last_track);
    close_progress:
    SetPubVar(progress_window, "progress", 100);
    Destroy(progress_window);
    show_result:
    pd_type = get_aval_int("PDType");
    if (pd_type < 0)
    {
        pd_type = get_aval_int("PrevPDType");
        if (pd_type < 0)
        {
            pd_type = 0;
        }
    }
    if (0 != pd_type && 1 != pd_type)
    {
        pd_type = 0;
    }
    new back_label[64];
    get_tag_attr2("TEXT", "msgmusic_btn_back", "value", back_label, 64);
    SetSystemParamStr(SYS_STR_PADHELP_RD, back_label);
    if (0 != error && CHECKOUT_INTERRUPTED != error)
    {
        printf("==== checkout_end = %d\n", error);
        get_tag_attr2("TEXT", g_checkout_results[channel_mode * 3 + 1], "value", g_line1, 256);
        get_tag_attr2("TEXT", g_checkout_errors[channel_mode * 14 + error + pd_type], "value", g_line2, 256);
        if (track < 0 && 0 != done_count
                && (error == CHECKOUT_COUNT_LIMIT || error == CHECKOUT_DEVICE_FULL || error == CHECKOUT_PROHIBITED))
        {
            get_tag_attr2("TEXT", g_checkout_results[channel_mode * 3 + 0], "value", g_line1, 256);
            sprintf(g_detail, g_line2, done_count);
            sprintf(g_msg, "%s\n\n%s", g_line1, g_detail);
        }
        else if (track < 0 && error == CHECKOUT_COUNT_LIMIT)
        {
            get_tag_attr2("TEXT", "msgmusic_album_chkout_fail_cntover", "value", g_line2, 256);
            sprintf(g_msg, "%s\n\n%s", g_line1, g_line2);
        }
        else if (track < 0 && error == CHECKOUT_PROHIBITED)
        {
            get_tag_attr2("TEXT", "msgmusic_album_chkout_prohibit", "value", g_line2, 256);
            sprintf(g_msg, "%s\n\n%s", g_line1, g_line2);
        }
        else if (track < 0 && error == CHECKOUT_DEVICE_FULL)
        {
            error = CHECKOUT_NO_SPACE;
            get_tag_attr2("TEXT", g_checkout_errors[channel_mode * 14 + error + pd_type], "value", g_line2, 256);
            sprintf(g_msg, "%s\n\n%s", g_line1, g_line2);
        }
        else
        {
            sprintf(g_msg, "%s\n\n%s", g_line1, g_line2);
        }
    }
    else if (track < 0 && 0 != done_count && error == CHECKOUT_INTERRUPTED)
    {
        get_tag_attr2("TEXT", g_checkout_results[channel_mode * 3 + error], "value", g_line1, 256);
        get_tag_attr2("TEXT", "msgmusic_track_chkout_progress", "value", g_line2, 256);
        sprintf(g_detail, g_line2, done_count);
        sprintf(g_msg, "%s\n\n%s", g_line1, g_detail);
    }
    else if (0 == last_track)
    {
        get_tag_attr2("TEXT", "msgmusic_album_chkout_prohibit", "value", g_msg, 512);
    }
    else
    {
        get_tag_attr2("TEXT", g_checkout_results[channel_mode * 3 + error], "value", g_msg, 512);
    }
    show_message(g_msg);
    finish:
}

/* The texts of delete_main, by MusicChannelMode. */
new g_delete_asks[2][6] = {"msgmusic_album_del_ask", "msgmusic_track_del_ask"};
new g_delete_asks2[4][] = {
    "msgmusic_album_del_ask_entity", "msgmusic_album_del_ask_buy", "msgmusic_track_del_ask_entity",
    "msgmusic_track_del_ask_buy"
};
new g_delete_asks3[2][8] = {"msgmusic_album_del_ask_entity2", "msgmusic_track_del_ask_entity2"};
new g_delete_titles[2][5] = {"msgmusic_album_del", "msgmusic_track_del"};
new g_delete_results[8][] = {
    "msgmusic_album_del_complete", "msgmusic_album_del_fail", "msgmusic_album_del_cannot",
    "msgmusic_track_warn_track_none", "msgmusic_track_del_complete", "msgmusic_track_del_fail",
    "msgmusic_track_del_cannot", "msgmusic_track_warn_track_none"
};
/* By the type of the device. */
new g_write_disabled[2][6] = {"msgmusic_chkin_wdisable", "msgmusic_chkin_wdisable"};

/*
 * Deletes, after asking, the album or the track (argument channel_mode) of the hard disk, of the favorite album or of
 * the portable device (argument album_mode): the track of CheckoutTrack in the aval file, all of them if below 0.
 */
public delete_main()
{
    result = 0;
    new error;
    get_screen_size5(g_screen_w, g_screen_h);
    new arg[64];
    new channel_mode;
    if (GetArg("channel_mode", arg, 64))
    {
        channel_mode = atoi(arg);
    }
    new album_mode = MUSIC_ALBUM_COLLECTION;
    if (GetArg("album_mode", arg, 64))
    {
        album_mode = atoi(arg);
    }
    get_tag_attr2("TEXT", "msgmusic_common_yes", "value", g_yes, 64);
    get_tag_attr2("TEXT", "msgmusic_common_no", "value", g_no, 64);
    get_tag_attr2("TEXT", "msgmusic_common_ok", "value", g_ok, 64);
    if (MUSIC_ALBUM_FAVORITE == album_mode && channel_mode == CHANNEL_TRACK)
    {
        get_tag_attr2("TEXT", "msgmusic_track_del_favorite_ask", "value", g_line1, 256);
    }
    else
    {
        get_tag_attr2("TEXT", g_delete_asks[channel_mode], "value", g_line1, 256);
    }
    new answer = ask_yes_no(g_line1, g_yes, g_no, 1);
    if (0 != answer)
    {
        goto finish;
    }
    new album_tracks = get_aval_int("AlbumTracks");
    if (album_mode == MUSIC_ALBUM_COLLECTION && album_tracks > 0)
    {
        new bought;
        get_tag_attr2("TEXT", g_delete_asks2[channel_mode * 2 + bought], "value", g_line1, 256);
        get_tag_attr2("TEXT", g_delete_asks3[channel_mode + bought], "value", g_line2, 256);
        sprintf(g_msg, "%s\n%s", g_line1, g_line2);
        new answer2 = ask_yes_no(g_msg, g_yes, g_no, 1);
        if (0 != answer2)
        {
            goto finish;
        }
    }
    printf("delete\n");
    result = 1;
    get_tag_attr2("TEXT", g_delete_titles[channel_mode], "value", g_line1, 256);
    new progress_window = open_progress("", "", g_line1);
    new track;
    new frame;
    new poll;
    new pd_type;
    if (album_mode == MUSIC_ALBUM_COLLECTION)
    {
        track = get_aval_int("CheckoutTrack");
        printf("DELETE TRACK = %d\n", track);
        playerExecCommand(PLAYER_HDD_DELETE, 0, track);
        new scale = track < 0 ? 1 : 3;
        new busy = 1;
        frame = 0;
        for (; frame < scale * 100; frame++)
        {
            if (poll == 10)
            {
                playerExecCommand(PLAYER_GET_STATUS, busy);
                poll = 0;
            }
            else
            {
                poll++;
            }
            if (0 != busy && frame >= scale * 95)
            {
                frame = scale * 95;
            }
            SetPubVar(progress_window, "progress", frame / scale);
            sleep;
        }
    }
    else if (MUSIC_ALBUM_FAVORITE == album_mode)
    {
        new uri[256];
        new entry_date[64];
        new sql[512];
        new status;
        avalGetValue("ContentURI", 0, uri, 256);
        avalGetValue("EntryDate", 0, entry_date, 64);
        status = datOpen(DAT_DATABASE, g_db_name, DAT_ORDER_MTIME_DESC);
        status = wait_database();
        sprintf(sql, "DELETE from scei_jukebox_favorite_color WHERE url = '%s' AND entrydate = '%s'", uri, entry_date);
        status = datSQLExec(sql);
        frame = 0;
        for (; frame < 300; frame++)
        {
            SetPubVar(progress_window, "progress", frame / 3);
            sleep;
        }
        status = wait_database();
        if (status < 0)
        {
            error = DELETE_FAILED;
        }
        datClose();
    }
    else if (album_mode == MUSIC_ALBUM_PD)
    {
        track = get_aval_int("CheckoutTrack");
        new pd_tracks;
        new old_tracks;
        pd_tracks = get_aval_int("PDTracks");
        old_tracks = pd_tracks;
        if (pd_tracks > 0)
        {
            printf("+++++++++ DELETE_PD = %d\n", track);
            if (track > 0)
            {
                playerExecCommand(PLAYER_PD_DELETE, 0, track);
            }
            else
            {
                playerExecCommand(PLAYER_PD_DELETE_ALL);
            }
            new busy = 1;
            frame = 0;
            for (; frame < 300; frame++)
            {
                if (poll == 10)
                {
                    playerExecCommand(PLAYER_GET_STATUS, busy);
                    poll = 0;
                }
                else
                {
                    poll++;
                }
                if (0 == busy)
                {
                    break;
                }
                new shown = frame;
                shown = clamp(shown, 0, 90);
                SetPubVar(progress_window, "progress", shown);
                sleep;
                if (frame == 299 && 0 != busy)
                {
                    frame--;
                }
            }
            playerExecCommand(PLAYER_PD_GET_RESULT, busy);
            printf("STATE %d\n", busy);
            SetPubVar(progress_window, "progress", 90);
            printf("+++++++++++next\n");
            playerExecCommand(PLAYER_PD_CHECK, 0);
            new ready = -1;
            while (ready < 0)
            {
                playerExecCommand(PLAYER_GET_STATUS, ready);
                sleep;
            }
            playerExecCommand(PLAYER_PD_GET_INFO, pd_tracks, pd_type, 0);
            set_aval_int("PDTracks", pd_tracks);
            SetPubVar(progress_window, "progress", 99);
            if (0 != busy || (pd_tracks > 0 && track <= 0) || (1 != old_tracks - pd_tracks && track > 0))
            {
                if (pd_type < 0)
                {
                    pd_type = get_aval_int("PDType");
                    if (pd_type < 0)
                    {
                        pd_type = get_aval_int("PrevPDType");
                        if (pd_type < 0)
                        {
                            pd_type = 0;
                        }
                    }
                }
                error = DELETE_WRITE_PROTECTED;
            }
        }
        else if (pd_tracks < 0)
        {
            error = DELETE_NO_DEVICE;
        }
        else
        {
            error = DELETE_NO_TRACK;
        }
    }
    SetPubVar(progress_window, "progress", 100);
    Destroy(progress_window);
    if (0 != pd_type && 1 != pd_type)
    {
        pd_type = 0;
    }
    show_result:
    if (error == DELETE_NO_DEVICE)
    {
        get_tag_attr2("TEXT", g_delete_results[channel_mode * 4 + 1], "value", g_line2, 256);
        get_tag_attr2("TEXT", "msgmusic_device_connect_confirm", "value", g_msg, 512);
        sprintf(g_line1, "%s\n\n%s", g_line2, g_msg);
        goto show;
    }
    if (error == DELETE_WRITE_PROTECTED)
    {
        get_tag_attr2("TEXT", g_write_disabled[pd_type], "value", g_msg, 512);
        get_tag_attr2("TEXT", g_delete_results[channel_mode * 4 + 1], "value", g_line2, 256);
        sprintf(g_line1, "%s\n\n%s", g_line2, g_msg);
        goto show;
    }
    get_tag_attr2("TEXT", g_delete_results[channel_mode * 4 + error], "value", g_line1, 256);
    show:
    show_message(g_line1);
    finish:
    printf("---------------- delete result = %d\n", result);
}

/* Asks msg with a yes/no dialog on button select; returns its select, 0 for yes (1 when closed without an answer). */
ask_yes_no(msg[], yes[], no[], select)
{
    new args[512];
    sprintf(args, "msg=%s;yes=%s;no=%s", msg, yes, no);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 0.5 * g_screen_w, 0.5 * g_screen_h, 0.0, 0);
    AttachScript(dialog, _, "confirm_yesno", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus);
    SetPubVar(dialog, "select", select);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    new answer = GetPubVar(dialog, "select");
    if (answer == -1)
    {
        answer = 1;
    }
    Destroy(dialog);
    return answer;
}

/* Shows msg in a dialog and waits until it is closed. */
show_message(msg[])
{
    new args[512];
    sprintf(args, "msg=%s", msg);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 0.5 * g_screen_w, 0.5 * g_screen_h, 0.0, 0);
    AttachScript(dialog, _, "confirm_d", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    Destroy(dialog);
    return;
}

/*
 * Opens a progress_window with up to three lines of message, the percent format, the label of the cancel button and
 * break_flag (the cross sets ProgressCancelFlag instead of closing it); returns it.
 */
open_progress(msg[], msg2[], msg3[], const percent[] = "", const cancel[] = "", const break_flag[] = "")
{
    new args[512];
    sprintf(args, "msg=%s;msg2=%s;msg3=%s;cancel=%s;percent=%s;breakflag=%s", msg, msg2, msg3, cancel, percent,
            break_flag);
    new window = CreateNull(Disp2D(0));
    SetPos(window, 0.5 * g_screen_w, 0.5 * g_screen_h, 0.0, 0);
    AttachScript(window, _, "progress_window", args);
    SetPubVar(window, "manager", Self());
    SetPubVar(window, "focus", focus);
    return window;
}

/* Returns the result of a push of item index in the menu of the favorite albums: circle chooses it, cross closes it. */
handle_menu_push(manager, side, &mode, index, button, title[], group[], id[])
{
    datClose();
    if (button == TAG("RRGT"))
    {
        return index + MENU_ITEM;
    }
    if (button == TAG("RDWN"))
    {
        return MENU_CANCEL;
    }
    return MENU_REOPEN;
}

/*
 * Returns the number of the favorite albums, the items of the menu of favorite_main; creates the first one when there
 * is none.
 */
count_extra_items(side, menu_result, mode, title[], group[], id[], &selected)
{
    new count;
    new buffer[64];
    count = side;
    count = menu_result;
    count = mode;
    count = selected;
    copy_string(buffer, title, 64);
    copy_string(buffer, group, 64);
    copy_string(buffer, id, 64);
    count = 0;
    new status;
    new sql[512];
    status = datOpen(DAT_DATABASE, g_db_name, DAT_ORDER_MTIME_DESC);
    if (0 != status)
    {
        return -1;
    }
    status = wait_database();
    if (status < 0)
    {
        return -1;
    }
    sprintf(sql, "SELECT albumname FROM scei_jukebox WHERE format = -2");
    status = datSQLExec(sql);
    if (0 != status)
    {
        return -1;
    }
    status = wait_database();
    if (status < 0)
    {
        return -1;
    }
    count = datGetNum();
    printf("============== NUM = %d\n", count);
    if (0 == count)
    {
        new name[64];
        get_tag_attr2("TEXT", "msgmusic_optmenut_library_favorite", "value", name, 64);
        new hour;
        new minute;
        new second;
        new year;
        new month;
        new day;
        time(hour, minute, second, 0);
        date(year, month, day, 0);
        sprintf(sql,
                "INSERT INTO scei_jukebox (url, albumname, format, importdate) VALUES ('0', '%s1', -2, '%04d-%02d-%02d %02d:%02d:%02d')",
                name, year, month, day, hour, minute, second);
        status = datSQLExec(sql);
        if (0 != status)
        {
            return -1;
        }
        status = wait_database();
        if (status < 0)
        {
            return -1;
        }
        sprintf(sql, "SELECT albumname FROM scei_jukebox WHERE format = -2");
        status = datSQLExec(sql);
        if (0 != status)
        {
            return -1;
        }
        status = wait_database();
        if (status < 0)
        {
            return -1;
        }
        count = datGetNum();
    }
    return count;
}

/* Gives item index of the menu the name of favorite album index. */
fill_menu_item(side, mode, index, group[], label[])
{
    new unused; /* takes the parameters the function does not use */
    new buffer[64];
    unused = side;
    unused = mode;
    unused = index;
    unused = 64;
    copy_string(buffer, group, unused);
    new name[64];
    if (index >= 0)
    {
        new status;
        status = datGetData(index, 1, name, 64);
        if (0 == status)
        {
            copy_string(label, name, 64);
            printf("===== FAVCOLOR name = %s\n", name);
        }
    }
}

#include "playerctl.inc"
