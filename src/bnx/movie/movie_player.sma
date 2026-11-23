/*
 * /opt0/bn/script/movie/movie_player.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The page of an album of movies: a grid of thumbnails with the title and the thumbnail of the selected movie in a
 * focus box. The circle plays it (RealMedia in the built-in player page, PSS with the movie library), the triangle
 * opens the option menu (add to a favorite album, move, delete, information), the cross goes back to the albums.
 */

#include <core>
#include <bn>

#pragma dynamic 2048

#include "moviedb.inc"

#include "devimage.inc"

#include "stdmenu.inc"
#include "genmenu.inc"
#include "scroll.inc"

/* Results of run_movie_option. */
const OPTION_CLOSE = -1;
const OPTION_STAY = 0;
const OPTION_CHANGED = 1;       /* the album changed */
const OPTION_ALBUM_CHANGED = 2; /* the album changed and the menu closes */

/* The items of the option menu of a movie (optmenu1) and of a movie of a favorite album (optmenu3). */
const ITEM_ADD_TO_FAVORITE = 0;
const ITEM_MOVE = 1;
const ITEM_DELETE = 2;
const ITEM_INFORMATION = 3;
const ITEM_FAV_DELETE = 0;
const ITEM_FAV_INFORMATION = 1;

forward Float:get_node_x(node, columns, count);
forward Float:get_node_y(node, columns, count);
forward Float:get_node_z(node, columns, count);

#include "wait.inc"

#include "tag.inc"

#include <float>

#pragma library "movie"
native movieListClear();
native movieListAdd(const path[]);
native movieListNum();
native moviePlay(index);
/* Starts reading the length of a movie, for movieGetTimeSync. */
native movieGetTimeStart(const path[]);
/* Returns 0 while reading, 1 with the length, -1 on an error. */
native movieGetTimeSync(&hours, &minutes, &seconds);
#pragma library

/* The natives of the moviedata library (moviedata.so): the title and the thumbnail of a movie file. */
#pragma library "moviedata"
/*
 * Starts reading block entry of the user data of a movie file: kind 0 reads it as text (entry 0: the title), kind 1
 * as a TIM2 texture (entry 3: the thumbnail).
 */
native moviedataReadStart(const path[], kind, entry);
/* Returns 0 while reading, -1 on an error, 1 with the text in buf or, for kind 1, the texture in buf[0]. */
native moviedataReadSync(const buf[], maxlength);

#include "imgsync.inc"

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

#include "xmlsync.inc"

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

#include "wndvar.inc"

#include "imgdec.inc"

new g_shadow_colors[4] = {0x5F000000, 0x5F000000, 0x5F000000, 0x5F000000}; /* the "SCL4" of the shadowed labels */
new Float:g_screen_width;
new Float:g_screen_height;
new Float:g_screen_depth;
new g_common_bg_name[] = "SCEI:CommonBG";

/* Same as get_screen_size of text.inc. */
get_screen_size(&Float:screen_w, &Float:screen_h)
{
    GetScreen(Camera(0), g_screen_width, g_screen_height, g_screen_depth);
    screen_w = g_screen_width;
    screen_h = g_screen_height;
}

#include "dom.inc"

#include "disc.inc"

#include "layout.inc"

/* Loads the page of "LINK" link_id as the common background, unless the background window already shows it. */
load_common_bg(link_id[], Float:fade_time)
{
    {
        new name[256];
        if (wndGetName(WND_BG, name, 256) && 0 == strcmp(name, g_common_bg_name))
        {
            return;
        }
    }
    new url[256];
    get_tag_attr2("LINK", link_id, "href", url, 256);
    xmlLoadStart(url, g_common_bg_name, "", XMLLOAD_BG);
    wait_xml_load();
    fade_in_background(fade_time);
}

#include "background.inc"

#include "bgmode.inc"

/* Makes the camera of the background window follow this camera, by these shares of its position and rotation. */
link_bg_camera(Float:pos_follow, rot_follow)
{
    new bg_scene = wndGetScene(WND_BG);
    if (bg_scene)
    {
        SendMessage(bg_scene, TAG("LINK"), Camera(0), pos_follow, rot_follow, 0);
    }
}

#include "fileop.inc"

#include "focus.inc"

/* Same as send_focus of dom.inc. */
send_focus2(obj, target, z_offset, Float:sharp, Float:extra_w, Float:extra_h, Float:move_time, smoothing, blinking)
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
    get_screen_size(screen_w, screen_h);
    new manager;
    new result;
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
        result = init_result;
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
    item_count = item_count + count_extra_items(side, result, mode, title, group, id, selected);
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
        result = 0;
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
                result = handle_menu_push(manager, side, mode, selected, b, title, group, id);
                if (MENU_MODE_GROUP == mode)
                {
                    apply_result(result, selected, group, id);
                }
                if (result < MENU_REOPEN)
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
        if (result >= MENU_REDRAW)
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
    if (result >= MENU_REOPEN)
    {
        SendMessageStr(manager, TAG("TITL"), title);
        SendMessageStr(manager, TAG("GRUP"), group);
        SendMessageStr(manager, TAG("IDTY"), id);
        SendMessage(manager, TAG("FNSH"), result, mode, selected, 0);
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
    if (result == MENU_REDRAW)
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
    new result;
    new mode;
    new side = flags & 0x3;
    new Float:screen_w;
    new Float:screen_h;
    get_screen_size(screen_w, screen_h);
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
        AttachScript(menu_obj, "std_menu_core", _, args);
        SendMessage(menu_obj, TAG("INIT"), Self(), side, result, mode);
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
                result = a;
                mode = b;
            }
            if (ScriptState(menu_obj) == SCRIPT_FINISHED)
            {
                break;
            }
            sleep;
        }
        if (result >= MENU_CANCEL)
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
    return result;
}

/* Sets the group and id of the menu that result leads to from item index; MENU_CLOSED if there is none. */
apply_result(&result, index, group[], id[])
{
    if (result == MENU_RETURN)
    {
    }
    else if (result == MENU_SUBMENU)
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
    else if (result == MENU_BACK)
    {
        new ret_group[64];
        new ret_item[64];
        get_menu_return(ret_group, ret_item, 64, 64);
        printf(" %d, %s, %s, %s, %s \n", index, group, id, ret_group, ret_item);
        if (!strcmp(ret_group, "") || !strcmp(ret_item, ""))
        {
            printf("WARNING: nothing return menu !!\n");
            result = MENU_CLOSED;
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

new g_focus_box;

/* Creates the box at x, y (a focus_box part) unless it exists. */
create_focus_box(Float:x, Float:y)
{
    if (!g_focus_box)
    {
        g_focus_box = CreateNull(Disp2D(0));
        SetPos(g_focus_box, x, y, 0.0, 0);
        SetOptionI(g_focus_box, TAG("ZSRT"), ZSORT_OVERLAY);
        AttachScript(g_focus_box, _, "focus_box");
    }
}

/* Sets header line 0 to 2 of the box. */
set_box_header(line, text[])
{
    if (g_focus_box)
    {
        switch (line)
        {
            case 0:
            {
                SendMessageStr(g_focus_box, TAG("HED0"), text);
            }
            case 1:
            {
                SendMessageStr(g_focus_box, TAG("HED1"), text);
            }
            case 2:
            {
                SendMessageStr(g_focus_box, TAG("HED2"), text);
            }
        }
    }
}

/* Moves header line 0 to 2 of the box by (dx, dy, dz) over duration ms. */
move_box_header(line, dx, Float:dy, dz, duration)
{
    if (g_focus_box)
    {
        switch (line)
        {
            case 0:
            {
                SendMessage(g_focus_box, TAG("OFS0"), dx, dy, dz, duration);
            }
            case 1:
            {
                SendMessage(g_focus_box, TAG("OFS1"), dx, dy, dz, duration);
            }
            case 2:
            {
                SendMessage(g_focus_box, TAG("OFS2"), dx, dy, dz, duration);
            }
        }
    }
}

set_box_footer(text[])
{
    if (g_focus_box)
    {
        SendMessageStr(g_focus_box, TAG("FOOT"), text);
    }
}

/* Fades the whole box to alpha and its color to red, green, blue. */
fade_box(Float:alpha, Float:duration, Float:red, Float:green, Float:blue)
{
    if (g_focus_box)
    {
        SendMessage(g_focus_box, TAG("FADE"), alpha, duration, 0, 0);
        SendMessage(g_focus_box, TAG("FCOL"), red, green, blue, duration);
    }
}

/* Does nothing. */
setup_box(x, y)
{
}

/* Fades the footer of the box to alpha and its color to red, green, blue. */
fade_box_footer(Float:alpha, Float:duration, Float:red, Float:green, Float:blue)
{
    if (g_focus_box)
    {
        SendMessage(g_focus_box, TAG("FAD2"), alpha, duration, 0, 0);
        SendMessage(g_focus_box, TAG("FCL2"), red, green, blue, duration);
    }
}

new g_device_image;
new Float:g_device_x = 130.0;
new Float:g_device_y = 350.0;
new Float:g_device_w = 512.0;
new Float:g_device_h = 512.0;

/* Creates the plane of the device image (a device_image part), transparent until shown. */
create_device_image()
{
    g_device_image = CreatePlane(Camera(0));
    SetPos(g_device_image, g_device_x, g_device_y, 0.0, Disp2D(0));
    SetRot(g_device_image, 0.0, 0.0, 0.0, Disp2D(0), 0);
    SetColor(g_device_image, 1.0, 1.0, 1.0, 0.0);
    SetScale(g_device_image, g_device_w, g_device_h, 1.0);
    SetOptionI(g_device_image, TAG("ZSRT"), ZSORT_BACKGROUND);
    SetOptionI(g_device_image, TAG("ALPH"), BLEND_ADD);
    SetOptionF(g_device_image, TAG("ZOFS"), -100.0);
    AttachScript(g_device_image, _, "device_image");
}

/* Shows the image of device, DEVICE_*, or none. */
show_device_image(device)
{
    if (!g_device_image)
    {
        return;
    }
    if (device == DEVICE_DISC)
    {
        SendMessageStr(g_device_image, TAG("IMAG"), "file:/opt0/bn/data/tex/device_image/disc.tm2");
        SendMessage(g_device_image, TAG("POS"), g_device_x, g_device_y - 15.0, 0, 0);
    }
    else if (device == DEVICE_HDD)
    {
        SendMessageStr(g_device_image, TAG("IMAG"), "file:/opt0/bn/data/tex/device_image/hdd.tm2");
        SendMessage(g_device_image, TAG("POS"), g_device_x, g_device_y, 0, 0);
    }
    else if (device == DEVICE_MC)
    {
        SendMessageStr(g_device_image, TAG("IMAG"), "file:/opt0/bn/data/tex/device_image/mc.tm2");
        SendMessage(g_device_image, TAG("POS"), g_device_x - 30.0, 5.0 + g_device_y, 0, 0);
    }
    else if (device == DEVICE_USB)
    {
        SendMessageStr(g_device_image, TAG("IMAG"), "file:/opt0/bn/data/tex/device_image/usb.tm2");
        SendMessage(g_device_image, TAG("POS"), 30.0 + g_device_x, g_device_y, 0, 0);
    }
    else
    {
        SendMessageStr(g_device_image, TAG("IMAG"), "");
    }
}

new g_db_name[] = "movie";
public pad_enable; /* unused, but part of the layout */
public padhelp; /* unused, but part of the layout */
public focus; /* unused, but part of the layout */
new g_caption_labels2[2][2];
new g_caption_index2;
new Float:g_caption_x2;
new Float:g_caption_y2;

/* Same as wait_database of misc.inc. */
wait_database2()
{
    new status;
    for (; ; )
    {
        status = datSync();
        if (0 != status)
        {
            return status;
        }
        sleep;
    }
    return 0;
}

/* Creates the labels of show_caption2, with a shadow if shadow. */
create_captions(shadow)
{
    new i;
    new j;
    {
        new Float:screen_w;
        new Float:screen_h;
        get_screen_size(screen_w, screen_h);
        g_caption_x2 = 0.546875 * screen_w;
        g_caption_y2 = 0.5 * screen_h;
    }
    i = 0;
    for (; i < 2; i++)
    {
        j = 0;
        for (; j < 2; j++)
        {
            if (!shadow)
            {
                g_caption_labels2[i][j] = CreateLabel(Disp2D(0));
            }
            else
            {
                g_caption_labels2[i][j] = CreateLabelShadow(Disp2D(0));
            }
            SetPos(g_caption_labels2[i][j], g_caption_x2, g_caption_y2, 0.0, 0);
            SetOptionI(g_caption_labels2[i][j], TAG("FONT"), FontID(0));
            SetOptionI(g_caption_labels2[i][j], TAG("HALN"), ALIGN_LEFT);
            SetOptionI(g_caption_labels2[i][j], TAG("VALN"), ALIGN_CENTER);
            SetOptionI(g_caption_labels2[i][j], TAG("FSIZ"), 17);
            if (shadow)
            {
                SetOptionP(g_caption_labels2[i][j], TAG("SCL4"), g_shadow_colors, 4);
                SetOptionI(g_caption_labels2[i][j], TAG("SX64"), 2 * 64);
                SetOptionI(g_caption_labels2[i][j], TAG("SY64"), 2 * 64);
            }
            SetColor(g_caption_labels2[i][j], 0.7843, 0.7843, 0.7843, 0.0);
        }
    }
}

/* Same as show_caption of string.inc, with globals of its own. */
show_caption2(line1[] = "", line2[] = "", Float:target_a, Float:duration, Float:delay)
{
    new incoming = g_caption_index2;
    g_caption_index2 = ++g_caption_index2 & 1;
    new outgoing = g_caption_index2;
    SetOptionS(g_caption_labels2[incoming][0], TAG("UTF8"), line1);
    SetOptionS(g_caption_labels2[incoming][1], TAG("UTF8"), line2);
    if (strlen(line2) > 0)
    {
        SetPos(g_caption_labels2[incoming][0], g_caption_x2, g_caption_y2 - 12.0, 0.0, 0);
        SetPos(g_caption_labels2[incoming][1], g_caption_x2, 12.0 + g_caption_y2, 0.0, 0);
    }
    else
    {
        SetPos(g_caption_labels2[incoming][0], g_caption_x2, g_caption_y2, 0.0, 0);
        SetPos(g_caption_labels2[incoming][1], g_caption_x2, g_caption_y2, 0.0, 0);
    }
    AnimColor(g_caption_labels2[incoming][0], 0.7843, 0.7843, 0.7843, target_a, duration, delay, ANIM_FUNC_LINEAR);
    AnimColor(g_caption_labels2[incoming][1], 0.7843, 0.7843, 0.7843, target_a, duration, delay, ANIM_FUNC_LINEAR);
    AnimColor(g_caption_labels2[outgoing][0], 0.7843, 0.7843, 0.7843, 0.0, 0.5 * duration, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_caption_labels2[outgoing][1], 0.7843, 0.7843, 0.7843, 0.0, 0.5 * duration, 0.0, ANIM_FUNC_LINEAR);
}

/* The option menus: 0 to 2 of an album of ALBUM_*, 3 of the radio buttons, 4 the sort order. */
new g_option_groups[5][] = {"optmenu1", "optmenu2", "optmenu3", "optmenu4", "optmenu_sort"};
new g_album_device[3] = {DEVICE_HDD, DEVICE_DISC, DEVICE_HDD};

/* Same as set_caption_pos of misc.inc, for show_caption2. */
set_caption_pos2(Float:x, Float:y)
{
    g_caption_x2 = x;
    g_caption_y2 = y;
}

/* Loads the page of "LINK" link_id, or the built-in top menu, with args. */
load_page(link_id[], args[])
{
    if (!strcmp(link_id, "top"))
    {
        xmlLoadStart("$$BUILDIN_TOPMENU$$", _, args, XMLLOAD_MAIN);
    }
    else
    {
        new url[256];
        get_tag_attr2("LINK", link_id, "href", url, 256);
        xmlLoadStart(url, _, args, XMLLOAD_MAIN);
    }
    wait_xml_load();
}

/* Plays url in the built-in player page, which then loads next_url (the current page when it is empty). */
play_url(url[], next_url[])
{
    new args[512];
    new cref[256];
    new nextref[256];
    copy_string(cref, url, 256);
    xmlMakeFullPath(cref, 256);
    if (next_url[0])
    {
        copy_string(nextref, next_url, 256);
    }
    else
    {
        xmlGetURI(nextref, 256);
    }
    xmlMakeFullPath(nextref, 256);
    sprintf(args, "cref=%s;nextref=%s", cref, nextref);
    xmlLoadStart("$$BUILDIN_PLAY$$", "", args, XMLLOAD_MAIN);
    wait_xml_load();
}

/* Plays the first movie of the play list. */
play_list()
{
    if (0 == movieListNum())
    {
        printf("playlist is empty.\n");
        return;
    }
    moviePlay(0);
}

new g_camera;
new Float:g_screen_w;
new Float:g_screen_h;
public camera_focus; /* the point the camera looks at */
public camera_uptgt; /* unused, but part of the layout */
public focus_glowfly; /* the focus object */
new g_active;
new g_album;
new g_jacket;
new g_sql[128];
new g_album_uri[256]; /* the folder of the album, or the album_id of a favorite album */
new g_next_link[256];
new g_next_args[256];
new g_info_args[128];
new g_quit;
new g_album_padhelp; /* the pad help buttons of the album */
new g_title_line[256];
new g_album_name[256];
new g_album_mode; /* ALBUM_* */
new g_movie_count; /* the movies of the album, negative on an error */
new g_select;
new g_idle; /* 1 once the selection has stopped on a movie */
new Float:g_jacket_pos[3];
new Float:g_jacket_menu_pos[3];
new g_album_no;
new g_db_open;
new g_dialog_args[512];
new g_movie_uri[256];
new g_movie_url[1028];
new g_attr[4]; /* the content_attr of the selected movie */
new g_thumb_uri[32];
new g_thumb_data[64];
new g_album_busy; /* 1 while the selection moves: no movie can be played */
new g_default_icon;

/* Sets the variable name to the number value. */
set_aval_int(name[], value)
{
    new text[16];
    sprintf(text, "%d", value);
    avalSet(name, text);
}

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

public main()
{
    g_next_link = "top";
    init_page();
    while (1 != g_quit)
    {
        handle_messages();
        sleep;
    }
    SetSystemParam(SYS_PADHELP, 0);
    close_db();
    fade_background(0, 0, 600.0);
    load_page(g_next_link, g_next_args);
}

/*
 * Builds the page: the album of the arguments album_mode, uri, album_name and currentAlbumNo, or of the variables
 * saved before playing a RealMedia movie (RealPlayer set), with the movie selectNo selected.
 */
init_page()
{
    new value[64];
    g_camera = Camera(0);
    get_screen_size(g_screen_w, g_screen_h);
    g_quit = 0;
    g_album_padhelp = PADHELP_CIRCLE | PADHELP_CROSS;
    g_movie_count = 0;
    g_select = -1;
    g_idle = 0;
    g_title_line = "";
    g_album_mode = ALBUM_COLLECTION;
    g_album_no = 0;
    g_album_name = "";
    if (0 == get_aval_int("RealPlayer"))
    {
        value = "";
        if (GetArg("album_mode", value, 64))
        {
            g_album_mode = atoi(value);
        }
        GetArg("uri", g_album_uri, 256);
        GetArg("album_name", g_album_name, 256);
        set_wnd_var_str("album_name", g_album_name);
        value = "";
        if (GetArg("currentAlbumNo", value, 64))
        {
            g_album_no = atoi(value);
        }
    }
    else
    {
        set_aval_int("RealPlayer", 0);
        value = "";
        if (0 == avalGetValue("album_mode", 0, value, 64))
        {
            g_album_mode = atoi(value);
        }
        avalGetValue("uri", 0, g_album_uri, 256);
        avalGetValue("album_name", 0, g_album_name, 256);
        set_wnd_var_str("album_name", g_album_name);
        value = "";
        if (0 == avalGetValue("currentAlbumNo", 0, value, 64))
        {
            g_album_no = atoi(value);
        }
        value = "";
        if (0 == avalGetValue("selectNo", 0, value, 64))
        {
            g_select = atoi(value);
            if (g_select <= 0)
            {
                g_select = -1;
            }
        }
    }
    g_movie_count = query_album(g_album_mode);
    g_default_icon = load_icon_texture("default_icon");
    start_top_page(0);
    load_common_bg("bg", 1000.0);
    set_background_poor(600.0, 0);
    camera_focus = CreateNull(Self());
    camera_uptgt = CreateNull(Self());
    SetPos(camera_focus, 0.0, 0.0, -100.0, 0);
    SetPos(camera_uptgt, 0.0, 0.0 + 1000.0, 0.0, 0);
    SetPos(g_camera, 0.0, 0.0, 0.0, 0);
    LookAt(g_camera, camera_focus, 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
    AttachScript(g_camera, "camera_main");
    SetPubVar(g_camera, "camera_focus", camera_focus);
    link_bg_camera(4.0, 0);
    reset_bg_camera();
    focus_glowfly = create_focus_plane2("focus");
    create_device_image();
    g_album = create_album(g_movie_count);
    if (-1 != g_select)
    {
        SendMessage(g_album, TAG("SLCT"), g_select, 0, 0, 0);
    }
    g_jacket = create_jacket_panel();
    create_captions(0);
    create_focus_box(0.7625 * g_screen_w, 0.5 * g_screen_h);
    fade_box(0.0, 0.0, 1.0, 1.0, 1.0);
    setup_box(0, 0);
    move_box_header(0, 0, -24.0, 0, 0);
    move_box_header(1, 0, -24.0, 0, 0);
    show_device_image(g_album_device[g_album_mode]);
    show_album_error();
    g_active = g_album;
    if (g_movie_count <= 0)
    {
        SetPubVar(g_album, "padhelp", PADHELP_CROSS);
    }
    SendMessage(g_active, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
    new icon_url[256];
    get_tag_attr2("IMG", "default_icon", "src", icon_url, 256);
    SendMessageStr(g_jacket, TAG("DCDE"), icon_url);
}

close_db()
{
    SendMessage(g_album, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
    datClose();
}

/*
 * Handles the messages of one frame: "FOCS" (index) and "PUSH" (_, button) of the album when it has the pad, "ITEM"
 * (menu, path, index), "PUSH" (_, button) of any part, and -100, which empties the disc album.
 */
handle_messages()
{
    new msg;
    new a;
    new b;
    new c;
    new d;
    while (RecvMessage(msg, a, b, c, d))
    {
        if (g_active == g_album)
        {
            if (msg == TAG("FOCS"))
            {
                on_album_focus(a);
            }
            if (msg == TAG("PUSH"))
            {
                on_album_push(b);
            }
        }
        if (msg == TAG("ITEM"))
        {
            send_album_item(a, b, c, g_movie_count);
        }
        if (msg == TAG("PUSH"))
        {
            on_push(b);
        }
        if (msg == MSG_NAVIMENU_OPENED && g_album_mode == ALBUM_DISC)
        {
            g_movie_count = 0;
            show_album_error();
        }
    }
    check_back_button();
    update_album_info();
    return 1;
}

/* The cross leaves for the albums of the movie page. */
check_back_button()
{
    new held;
    new pressed;
    new released;
    PadState(held, pressed, released, _, -1);
    if (0 == pressed && 0 == released)
    {
        return;
    }
    if (pressed & PAD_CROSS)
    {
        g_quit = 1;
        g_next_link = "movie";
        if (ALBUM_COLLECTION == g_album_mode)
        {
            sprintf(g_next_args, "mode = collection ; currentAlbumNo = %d", g_album_no);
        }
        else if (g_album_mode == ALBUM_FAVORITE)
        {
            sprintf(g_next_args, "mode = favorite ; currentAlbumNo = %d", g_album_no);
        }
        sndSePlay(SE_CANCEL);
        return;
    }
}

/* The cross leaves for the movie page, on the album shown. */
on_push(button)
{
    if (button == TAG("RDWN"))
    {
        g_quit = 1;
        g_next_link = "movie";
        if (ALBUM_COLLECTION == g_album_mode)
        {
            sprintf(g_next_args, "mode = collection ; currentAlbumNo = %d", g_album_no);
        }
        else if (g_album_mode == ALBUM_FAVORITE)
        {
            sprintf(g_next_args, "mode = favorite ; currentAlbumNo = %d", g_album_no);
        }
        sndSePlay(SE_CANCEL);
        return;
    }
}

/* Reloads the album after an option changed it; with no movie left, the jacket panel takes the pad. */
reload_album()
{
    send_focus2(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    SendMessage(g_album, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
    fade_box(0.0, 1000.0, 1.0, 1.0, 1.0);
    AnimColor(g_jacket, 1.0, 1.0, 1.0, 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    g_movie_count = load_album(g_album);
    show_album_error();
    if (g_movie_count > 0)
    {
        SendMessage(g_album, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        g_active = g_album;
    }
    else
    {
        g_active = g_jacket;
        SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        SetSystemParam(SYS_PADHELP, PADHELP_CROSS);
    }
}

/* Creates the album item of index and sends it to the album ("ITEM"). */
send_album_item(menu, path, index, count)
{
    if (IsValidObject(path))
    {
        new item = create_album_item(index, count, path);
        SendMessage(menu, TAG("ITEM"), item, path, 0, 0);
    }
}

/* The focus stays on g_active whichever way (direction, a TAG) the pad leaves it. */
on_album_focus(direction)
{
    SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
}

/*
 * Handles a push of the selected movie: circle or start plays it, triangle opens the option menu until an option
 * closes it, reloading the album if one changed it.
 */
on_album_push(button)
{
    if ((button == TAG("RRGT") || button == TAG("STRT")) && 0 == g_album_busy)
    {
        SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
        sndSePlay(SE_DECIDE);
        if (g_album_mode == ALBUM_FAVORITE || ALBUM_COLLECTION == g_album_mode)
        {
            new type_text[8];
            datGetData(g_select, COL_CONTENT_TYPE, type_text, 8);
            switch (atoi(type_text))
            {
                case CONTENT_REAL:
                {
                    datGetData(g_select, COL_URI, g_movie_uri, 256);
                    err20001(g_movie_uri, g_movie_url);
                    set_aval_int("RealPlayer", 1);
                    set_aval_int("album_mode", g_album_mode);
                    avalSet("uri", g_album_uri);
                    avalSet("album_name", g_album_name);
                    set_aval_int("currentAlbumNo", g_album_no);
                    set_aval_int("selectNo", g_select);
                    play_url(g_movie_url, "");
                }
                case CONTENT_PSS:
                {
                    new unused; /* unused, but part of the layout */
                    datGetData(g_select, COL_URI, g_movie_uri, 256);
                    movieListClear();
                    movieListAdd(g_movie_uri);
                    wait_ms(1000);
                    play_list();
                }
            }
        }
        SendMessage(g_active, TAG("PUSH"), 0, 0, 0, 0);
        SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
        return;
    }
    new content_uri[256];
    avalGetValue("ContentURI", 0, content_uri, 256);
    new has_content = 1;
    if (g_album_mode == ALBUM_FAVORITE || ALBUM_COLLECTION == g_album_mode)
    {
        if (0 == strlen(content_uri))
        {
            has_content = 0;
        }
    }
    if (button == TAG("RUP") && 0 == g_album_busy)
    {
        show_movie_info();
        sndSePlay(SE_OPEN);
        sndSePlay(SE_WINDOW_OPEN);
        SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
        SendMessage(g_active, TAG("PUSH"), TAG("RUP"), 0, 0, 0);
        SendMessage(g_active, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
        send_focus2(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        hide_caption(0.0, 400.0);
        fade_box_footer(0.0, 400.0, 0.7843, 0.7843, 0.7843);
        slide_camera(1);
        SendMessage(g_album, TAG("FADE"), 0, 1000.0, 0, 0);
        wait_ms(1000);
        new result;
        new status;
        new changed;
        while (OPTION_CLOSE != status)
        {
            result = pop_std_menu("optmenu", g_option_groups[g_album_mode], MENU_SIDE_RIGHT, result - MENU_ITEM);
            status = run_movie_option(result);
            SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
            if (status >= OPTION_CHANGED)
            {
                changed = 1;
            }
            if (status == OPTION_ALBUM_CHANGED)
            {
                status = OPTION_CLOSE;
            }
        }
        sndSePlay(SE_WINDOW_CLOSE);
        center_camera(1);
        SendMessage(g_album, TAG("FADE"), 1.0, 1000.0, 0, 0);
        wait_ms(1000);
        SendMessage(g_album, TAG("FADE"), 1.0, 500.0, 0, 0);
        hide_caption(1.0, 400.0);
        fade_box_footer(1.0, 400.0, 0.7843, 0.7843, 0.7843);
        if (!changed)
        {
            SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        }
        else
        {
            g_select = -1;
            reload_album();
        }
        SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
        return;
    }
    SendMessage(g_active, TAG("PUSH"), 0, 0, 0, 0);
}

/*
 * Follows the selection of the album: once it stops on a movie, shows its title and thumbnail and lets it be played;
 * while it moves, hides them.
 */
update_album_info()
{
    if (g_movie_count <= 0)
    {
        return;
    }
    new prev_select = g_select;
    new idle = GetPubVar(g_album, "idle");
    g_select = GetPubVar(g_album, "select");
    new prev_idle = g_idle;
    if (prev_select != g_select)
    {
        g_idle = 0;
    }
    else if (0 != idle)
    {
        g_idle = 1;
    }
    if (g_idle != prev_idle)
    {
        if (g_idle)
        {
            show_movie_info();
            g_album_busy = 0;
        }
        else
        {
            g_album_busy = 1;
            fade_box(0.0, 500.0, 1.0, 1.0, 1.0);
            AnimColor(g_jacket, 1.0, 1.0, 1.0, 0.0, 500.0, 0.0, ANIM_FUNC_LINEAR);
        }
    }
    if (0 == g_album_busy)
    {
        SetSystemParam(SYS_PADHELP, PADHELP_CIRCLE | PADHELP_CROSS | PADHELP_TRIANGLE);
    }
    else
    {
        SetSystemParam(SYS_PADHELP, PADHELP_CROSS);
    }
}

/*
 * Runs the option of result (MENU_ITEM + index) of the option menu of the album (with ALBUM_COLLECTION: add to a
 * favorite album, move, delete, information; with ALBUM_FAVORITE: delete, information); returns an OPTION_*.
 */
run_movie_option(result)
{
    new status;
    SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
    result = result - MENU_ITEM;
    switch (g_album_mode)
    {
        case ALBUM_COLLECTION:
        {
            switch (result)
            {
                case ITEM_ADD_TO_FAVORITE:
                {
                    return add_to_favorites();
                }
                case ITEM_MOVE:
                {
                    status = move_movie();
                    if (status)
                    {
                        status = OPTION_ALBUM_CHANGED;
                    }
                }
                case ITEM_DELETE:
                {
                    status = delete_movie();
                    if (status)
                    {
                        status = OPTION_ALBUM_CHANGED;
                    }
                }
                case ITEM_INFORMATION:
                {
                    if (0 != show_information())
                    {
                        new title[32];
                        /* NOTE: bug: title has 32 cells, avalGetValue may fill 128 */
                        avalGetValue("TitleName", 0, title, 128);
                        set_box_header(1, title);
                        status = OPTION_ALBUM_CHANGED;
                    }
                    else
                    {
                        status = OPTION_STAY;
                    }
                }
                default:
                {
                    status = OPTION_CLOSE;
                }
            }
        }
        case ALBUM_FAVORITE:
        {
            switch (result)
            {
                case ITEM_FAV_DELETE:
                {
                    status = delete_movie();
                    if (status)
                    {
                        status = OPTION_ALBUM_CHANGED;
                    }
                }
                case ITEM_FAV_INFORMATION:
                {
                    if (0 != show_information())
                    {
                        new title[32];
                        /* NOTE: bug: title has 32 cells, avalGetValue may fill 128 */
                        avalGetValue("TitleName", 0, title, 128);
                        set_box_header(1, title);
                        status = OPTION_ALBUM_CHANGED;
                    }
                    /* NOTE: bug: the OPTION_ALBUM_CHANGED of a rename is lost, the album is not reloaded */
                    status = OPTION_STAY;
                }
                default:
                {
                    status = OPTION_CLOSE;
                }
            }
        }
    }
    SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
    return status;
}

/* Runs addfav_contents of movie_dialog on the selected movie and returns its result. */
add_to_favorites()
{
    SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    datGetData(g_select, COL_URI, g_movie_uri, 256);
    new type_text[8];
    datGetData(g_select, COL_CONTENT_TYPE, type_text, 8);
    sprintf(g_dialog_args, "album_mode = %d ; uri = %s ; type = %d", g_album_mode, g_movie_uri, atoi(type_text));
    AttachScript(dialog, "addfav_contents", "movie_dialog", g_dialog_args);
    SetPubVar(dialog, "focus", focus_glowfly);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    new result = GetPubVar(dialog, "result");
    Destroy(dialog);
    SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
    return result;
}

/* Runs move_contents of movie_dialog on the selected movie and returns its result. */
move_movie()
{
    SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    datGetData(g_select, COL_URI, g_movie_uri, 256);
    new album_id[128];
    datGetData(g_select, COL_ALBUM_ID, album_id, 128);
    sprintf(g_dialog_args, "album_mode = %d ; uri = %s ; album_id = %s", g_album_mode, g_movie_uri, album_id);
    AttachScript(dialog, "move_contents", "movie_dialog", g_dialog_args);
    SetPubVar(dialog, "focus", focus_glowfly);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    new result = GetPubVar(dialog, "result");
    Destroy(dialog);
    SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
    return result;
}

/* Runs delete_contents of movie_dialog on the selected movie and returns its result. */
delete_movie()
{
    SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    datGetData(g_select, COL_URI, g_movie_uri, 256);
    new create_date[16];
    datGetData(g_select, COL_CREATE_DATE, create_date, 16);
    sprintf(g_dialog_args, "album_mode = %d ; uri = %s ; create_date = %s", g_album_mode, g_movie_uri, create_date);
    new type_text[8];
    datGetData(g_select, COL_CONTENT_TYPE, type_text, 8);
    switch (atoi(type_text))
    {
        case CONTENT_REAL:
        {
            AttachScript(dialog, "delete_contents", "movie_dialog", g_dialog_args);
        }
        case CONTENT_PSS:
        {
            AttachScript(dialog, "delete_contents", "movie_dialog", g_dialog_args);
        }
    }
    SetPubVar(dialog, "focus", focus_glowfly);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    new result = GetPubVar(dialog, "result");
    Destroy(dialog);
    SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
    return result;
}

/* Opens the information page of the selected movie; returns 1 if it was renamed there. */
show_information()
{
    new renamed;
    new url[128];
    SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
    datGetData(g_select, COL_CONTENT_ATTR, g_attr, 4);
    datGetData(g_select, COL_URI, g_movie_uri, 256);
    get_wnd_var_str2("album_name", g_album_name, 256);
    sprintf(g_info_args, "attr = %d ; uri = %s ; album_name = %s", atoi(g_attr), g_movie_uri, g_album_name);
    get_tag_attr2("LINK", "infomenu", "href", url, 128);
    xmlLoadStart(url, _, g_info_args, XMLLOAD_MODAL);
    wait_xml_load();
    if (avalGetNum("InfoStatus") > 0)
    {
        new status[32];
        avalGetValue("InfoStatus", 0, status, 32);
        if (!strcmp(status, "INVALID"))
        {
            renamed = 1;
        }
        else if (!strcmp(status, "ERROR"))
        {
            renamed = 0;
        }
        else if (!strcmp(status, "NONE"))
        {
            renamed = 0;
        }
    }
    SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
    return renamed;
}

/* Creates the album: a general_menu of count items, four by four, on the grid of get_node_x/y/z. */
create_album(count)
{
    new album;
    album = CreateNull(Self());
    {
        new Float:x;
        new Float:y;
        new Float:z;
        map_screen_point(x, y, z, 0.25 * g_screen_w, 0.5 * g_screen_h, Self());
        SetPos(album, x, 10.0 + y, z - 75.0, 0);
        SetRot(album, -0.3, 0.7, 0.0, 0, 0);
        AttachScript(album, "general_menu", "general_menu");
        SetPubVar(album, "manager", Self());
        SetPubVar(album, "focus", focus_glowfly);
        SetPubVar(album, "padhelp", g_album_padhelp);
        SendMessage(album, TAG("INUM"), count, 0, 0, 0);
        SendMessage(album, TAG("CLRW"), 4, 4, 0, 0);
        SendMessage(album, TAG("MGIN"), 4, 4, 0, 0);
        SendMessage(album, TAG("ANIM"), 300.0, 0, ANIM_FUNC_EXPONENT, 0);
        SendMessage(album, TAG("OFFA"), 0, 0, 0, 0);
        new node_count = 24;
        for (new node; node < node_count; node++)
        {
            SendMessage(album, TAG("NPOS"), node, get_node_x(node, 4, node_count), get_node_y(node, 4, node_count),
                    get_node_z(node, 4, node_count));
        }
    }
    return album;
}

/* Replaces album with a new one of the movies of the album; returns their number, negative on an error. */
load_album(&album)
{
    SendMessage(album, TAG("FADE"), 0, 500.0, 0, 0);
    wait_ms(500);
    Destroy(album);
    album = 0;
    new count = query_album(g_album_mode);
    album = create_album(count < 0 ? 0 : count);
    return count;
}

/* Creates the thumb_item of movie index, with the thumbnail of the movie file or the default icon. */
create_album_item(index, count, path)
{
    if (!IsValidObject(path))
    {
        return 0;
    }
    new item = CreateNull(path);
    AttachScript(item, "thumb_item", "thumb_item");
    SetPubVar(item, "pad_enable", PAD_START | PAD_TRIANGLE | PAD_CIRCLE | PAD_CROSS);
    SetColor(item, 1.0, 1.0, 1.0, 0.0);
    if (index >= 0 && index < count)
    {
        datGetData(index, COL_URI, g_thumb_uri, 32);
        moviedataReadStart(g_thumb_uri, 1, 3);
        if (wait_moviedata(g_thumb_data, 64) > 0)
        {
            SendMessage(item, TAG("TEX"), g_thumb_data[0], 1.0, 0, 0);
        }
        else
        {
            SendMessage(item, TAG("TEX"), g_default_icon, 1.0, 0, 0);
        }
    }
    return item;
}

/* Creates the jacket panel, the large thumbnail of the selected movie. */
create_jacket_panel()
{
    new panel;
    {
        new Float:x;
        new Float:y;
        new Float:z;
        map_screen_point(x, y, z, 0.7625 * g_screen_w, 0.5 * g_screen_h, Scene(0));
        g_jacket_pos[0] = x;
        g_jacket_pos[1] = y;
        g_jacket_pos[2] = z;
        map_screen_point(x, y, z, 0.7625 * g_screen_w, 0.5 * g_screen_h, Scene(0));
        g_jacket_menu_pos[0] = x;
        g_jacket_menu_pos[1] = y;
        g_jacket_menu_pos[2] = z;
        panel = CreatePlane(Scene(0));
        SetPos(panel, g_jacket_pos[0], g_jacket_pos[1], g_jacket_pos[2], 0);
        SetColor(panel, 1.0, 1.0, 1.0, 0.0);
        SetOptionI(panel, TAG("FDEG"), 1);
        AttachScript(panel, "jacket_panel_main", _, "");
    }
    return panel;
}

/*
 * Runs on the jacket panel: shows the texture of "TEX" (texture, width / height) or the image of the url of "DCDE",
 * fitted in 256 pixels; "FADE" (alpha) fades it once shown.
 */
public jacket_panel_main()
{
    new unused; /* unused, but part of the layout */
    new is_tm2;
    new self = Self();
    new shown;
    new Float:width = 1.0;
    new Float:height = 1.0;
    new Float:depth = 1.0;
    SetColor(self, 1.0, 1.0, 1.0, 0.0);
    new has_tex;
    for (; ; )
    {
        new msg;
        new Float:a;
        new Float:b;
        new c;
        new url[256];
        new status;
        has_tex = 0;
        while (RecvMessage(msg, a, b, c))
        {
            if (msg == TAG("FADE"))
            {
                if (shown)
                {
                    AnimColor(self, 1.0, 1.0, 1.0, a, 300.0, 0.0, ANIM_FUNC_LINEAR);
                }
            }
            if (msg == TAG("TEX"))
            {
                has_tex = 1;
                status = IMG_DONE;
            }
        }
        while (RecvMessageStr(msg, url, 256))
        {
            if (msg == TAG("DCDE"))
            {
                if (has_extension(url, "tm2"))
                {
                    is_tm2 = 1;
                    start_image_decode2(url);
                }
                else
                {
                    is_tm2 = 0;
                    imgDecStart(url, floatround(256.0, 0), floatround(256.0, 0), 0, 100, 0);
                }
                shown = 0;
                status = is_tm2 ? poll_image_decode() : imgDecSync();
            }
        }
        if (status == IMG_DONE)
        {
            if (0 == has_tex)
            {
                SetTexture(self, imgTexID(), 0, -1);
                new Float:ratio = 1.0 * imgTexOrgWidth() / imgTexOrgHeight();
                if (ratio > 1.0)
                {
                    width = 256.0;
                    height = 256.0 / ratio;
                }
                else
                {
                    width = 256.0 * ratio;
                    height = 256.0;
                }
                depth = 256.0;
                shown = 1;
                AnimColor(self, 1.0, 1.0, 1.0, 1.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
                SetLocalScale(self, width, height, depth);
            }
            else
            {
                SetTexture(self, a, 0, -1);
                new Float:ratio = b;
                AnimColor(self, 1.0, 1.0, 1.0, 1.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
                SetLocalScale(self, 256.0 * ratio, 256.0, 256.0);
            }
        }
        sleep;
    }
    return;
}

/* Shows the album name, the title, the position and the thumbnail of the selected movie. */
show_movie_info()
{
    if (g_select < 0 || g_select >= g_movie_count)
    {
        return;
    }
    new text[64];
    datGetData(g_select, COL_TITLE, text, 64);
    if (g_album_mode == ALBUM_FAVORITE)
    {
        datGetData(g_select, COL_ALBUM_NAME, g_album_name, 256);
        sprintf(g_title_line, "%s / %s", text, g_album_name);
    }
    else
    {
        copy_string(g_title_line, text, 256);
    }
    sprintf(text, "%d/%d", g_select + 1, g_movie_count);
    get_wnd_var_str2("album_name", g_album_name, 256);
    set_box_header(0, g_album_name);
    set_box_header(1, g_title_line);
    set_box_footer(text);
    fade_box(1.0, 1000.0, 1.0, 1.0, 1.0);
    datGetData(g_select, COL_URI, g_thumb_uri, 32);
    moviedataReadStart(g_thumb_uri, 1, 3);
    if (wait_moviedata(g_thumb_data, 64) > 0)
    {
        SendMessage(g_jacket, TAG("TEX"), g_thumb_data[0], 1.0, 0, 0);
    }
    else
    {
        SendMessage(g_jacket, TAG("TEX"), g_default_icon, 1.0, 0, 0);
    }
}

/* Selects the movies of the album of mode, oldest first; returns their number, -1 on an error. */
query_album(mode)
{
    new status;
    open_db();
    switch (mode)
    {
        case ALBUM_COLLECTION:
        {
            sprintf(g_sql,
                    "select * from sce_movie where content_attr = 0 and uri like '%s%s' order by create_date asc",
                    g_album_uri, "%");
        }
        case ALBUM_FAVORITE:
        {
            sprintf(g_sql,
                    "select * from sce_movie where content_attr = 1 and album_id = '%s' order by create_date asc",
                    g_album_uri);
        }
    }
    status = datSQLExec(g_sql);
    if (0 != status)
    {
        return -1;
    }
    status = wait_database2();
    if (status < 0)
    {
        return -1;
    }
    return datGetNum();
}

/* Shows the message of an empty album, or hides the caption when there are movies; returns 1 if it showed it. */
show_album_error()
{
    new text[64] = {0};
    if (g_movie_count > 0)
    {
        show_caption2(_, _, 0.0, 400.0, 0.0);
        return 0;
    }
    if (ALBUM_COLLECTION == g_album_mode || g_album_mode == ALBUM_FAVORITE)
    {
        if (0 == g_movie_count)
        {
            get_tag_attr2("TEXT", "msgmovie_hdd_none", "value", text, 64);
        }
        else if (g_movie_count < 0)
        {
            get_tag_attr2("TEXT", "msgmovie_hdd_none", "value", text, 64);
        }
    }
    else if (0 == g_movie_count)
    {
        get_tag_attr2("TEXT", "msgmovie_hdd_none", "value", text, 64);
    }
    else if (g_movie_count < 0)
    {
        get_tag_attr2("TEXT", "msgmovie_hdd_none", "value", text, 64);
    }
    set_caption_pos2(0.1 * g_screen_w, 0.5 * g_screen_h);
    show_caption2(text, _, 1.0, 400.0, 0.0);
    return 1;
}

/* Hides the caption; alpha is not used. */
hide_caption(Float:alpha, duration)
{
    new Float:target_a = g_movie_count > 0 ? 0.0 : alpha;
    show_caption2(_, _, 0.0, 400.0, 0.0);
}

/* Moves the camera to the left (dir 0) or to the right, taking the box and the jacket panel to the left side. */
slide_camera(dir)
{
    if (0 == dir)
    {
        AnimMoveHermite(Camera(0), -280.0, 0.0, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
        AnimMoveHermite(camera_focus, -280.0, 0.0, -100.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
        fade_box(0.0, 500.0, 1.0, 1.0, 1.0);
    }
    else
    {
        AnimMoveHermite(Camera(0), 300.0, 0.0, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
        AnimMoveHermite(camera_focus, 300.0, 0.0, -100.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
        SetPos(g_focus_box, 0.7625 * g_screen_w, 0.5 * g_screen_h, 0.0, 0);
        AnimMoveHermite(g_focus_box, 0.30625 * g_screen_w, 0.5 * g_screen_h, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
        AnimMoveHermite(g_jacket, g_jacket_menu_pos[0], g_jacket_menu_pos[1], g_jacket_menu_pos[2], 750.0, 0.0,
                ANIM_FUNC_EXPONENT, 1.0);
    }
}

/* Moves the camera back to the center, and the box and the jacket panel back to the right if from_menu. */
center_camera(from_menu)
{
    AnimMoveHermite(Camera(0), 0.0, 0.0, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
    AnimMoveHermite(camera_focus, 0.0, 0.0, -100.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
    if (0 == from_menu)
    {
        fade_box(1.0, 2000.0, 1.0, 1.0, 1.0);
    }
    else
    {
        SetPos(g_focus_box, 0.30625 * g_screen_w, 0.5 * g_screen_h, 0.0, 0);
        AnimMoveHermite(g_focus_box, 0.7625 * g_screen_w, 0.5 * g_screen_h, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
        AnimMoveHermite(g_jacket, g_jacket_pos[0], g_jacket_pos[1], g_jacket_pos[2], 750.0, 0.0, ANIM_FUNC_EXPONENT,
                1.0);
    }
}

/* The script of the camera of the page: it keeps looking at camera_focus. */
public camera_main()
{
    for (; ; )
    {
        LookAt(Self(), camera_focus, 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
        sleep;
    }
}

/* Decodes the image of IMG id and keeps its texture; returns it, 0 on failure. */
load_icon_texture(id[])
{
    new url[256];
    get_tag_attr2("IMG", id, "src", url, 256);
    start_image_decode2(url);
    if (IMG_DONE != finish_image_decode())
    {
        return 0;
    }
    new texture = imgTexID();
    imgKeepTex(texture);
    return texture;
}

/* Returns the x of node of the album grid: its column, the columns centered, 70.4 apart. */
Float:get_node_x(node, columns, count)
{
    new n = count;
    n = n * n;
    new Float:left = 1.1 * -(64.0 * float(columns - 1) / 2.0);
    return 1.1 * (64.0 * float(node % columns)) + left;
}

/*
 * Returns the y of node of the album grid: its row, the rows centered, 70.4 apart; the first and the last row are
 * pushed out by two rows more.
 */
Float:get_node_y(node, columns, count)
{
    new Float:top = 1.1 * (64.0 * float(count / columns - 1) / 2.0);
    new Float:offset;
    if (node < columns)
    {
        offset = 2.0 * (64.0 * 1.1);
    }
    if (node >= count - columns)
    {
        offset = 2.0 * (-64.0 * 1.1);
    }
    return top + offset - 1.1 * (64.0 * float(node / columns));
}

/* Returns the z of node of the album grid: 0. */
Float:get_node_z(node, columns, count)
{
    new n = node;
    n = columns;
    n = count;
    return 0.0 * n;
}

/* Same as has_extension of string.inc. */
has_extension(name[], ext[])
{
    new len = strlen(name);
    new ext_len = strlen(ext);
    for (new i; i < ext_len; i++)
    {
        if (toupper(name{len - 1 - i}) != toupper(ext{ext_len - 1 - i}))
        {
            return 0;
        }
    }
    if ('.' != toupper(name{len - 1 - ext_len}))
    {
        return 0;
    }
    return 1;
}

/* Returns the result of a push of item index in an option menu: circle chooses it, cross goes back. */
handle_menu_push(manager, side, &mode, index, button, title[], group[], id[])
{
    if (button == TAG("RRGT"))
    {
        return index + MENU_ITEM;
    }
    if (button == TAG("RDWN"))
    {
        return MENU_BACK;
    }
    return MENU_REOPEN;
}

/* Returns the number of items an option menu has besides those of its MENUGROUP: none. */
count_extra_items(side, result, mode, title[], group[], id[], &selected)
{
}

/* Leaves the label of an option menu item as it is. */
fill_menu_item(side, mode, index, group[], label[])
{
}

/* Opens the movie database once. */
open_db()
{
    new status;
    if (0 == g_db_open)
    {
        status = datOpen(DAT_DATABASE, g_db_name, DAT_ORDER_MTIME_DESC);
        if (0 != status)
        {
            return -1;
        }
        status = wait_database2();
        if (status < 0)
        {
            return -1;
        }
        g_db_open = 1;
    }
    return status;
}

/* Waits for the data that moviedataReadStart asked for; returns the status of moviedataReadSync. */
wait_moviedata(buf[], maxlen)
{
    for (; ; )
    {
        new result = moviedataReadSync(buf, maxlen);
        if (0 != result)
        {
            return result;
        }
        sleep;
    }
    return 0;
}

#include "misc.inc"
#include "date.inc"
#include "sound.inc"
