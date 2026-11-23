/*
 * /opt0/bn/script/movie/movie.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The movie page: radio buttons for the albums of the hard disk and the DVD video in the drive (or the favorite
 * albums alone), and a curve of album icons with the name of the selected one in a focus box. The circle opens the
 * album in the movie player page or boots the DVD, the triangle opens the option menus.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "moviedb.inc"

/* Indexes of g_option_groups besides the albums. */
const OPTIONS_BUTTONS = 3; /* the menu of the radio buttons */
const OPTIONS_SORT = 4;

/* Results of run_album_option besides 0, bits of the changed of the menu. */
const OPTION_CHANGED = 1; /* the albums changed: the menu closes */
const OPTION_RENAMED = 2;

/*
 * The items of the option menus: of the radio buttons (optmenu4), of an album (optmenu1) and of a favorite album
 * (optmenu3).
 */
const ITEM_NEW_ALBUM = 0;
const ITEM_SORT = 1;
const ITEM_ADD_TO_FAVORITE = 0;
const ITEM_DELETE = 1;
const ITEM_INFORMATION = 2;
const ITEM_FAV_INFORMATION = 0;

#include "devimage.inc"

#include "stdmenu.inc"
#include "genmenu.inc"
#include "scroll.inc"

forward Float:get_node_x(node);
forward Float:get_node_y(node);
forward Float:get_node_z(node);

#include "wait.inc"

#include "tag.inc"

#include <float>

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
get_tag_attr(tag[], id[], attr[], value[], maxlen)
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

#include "text.inc"

#include "wndvar.inc"

#include "imgdec.inc"

#include "misc.inc"

new g_db_name[] = "movie";
new g_shadow_colors[4] = {0x5F000000, 0x5F000000, 0x5F000000, 0x5F000000}; /* the "SCL4" of the shadowed labels */
new Float:g_screen_width3;
new Float:g_screen_height3;
new Float:g_screen_depth3;
new g_common_bg_name[] = "SCEI:CommonBG";

/* Same as get_screen_size of text.inc, with globals of its own. */
get_screen_size3(&Float:screen_w, &Float:screen_h)
{
    GetScreen(Camera(0), g_screen_width3, g_screen_height3, g_screen_depth3);
    screen_w = g_screen_width3;
    screen_h = g_screen_height3;
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
    get_tag_attr("LINK", link_id, "href", url, 256);
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
    get_screen_size3(screen_w, screen_h);
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
    get_screen_size3(screen_w, screen_h);
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

/* Fades the header of the box to alpha and its color to red, green, blue. */
fade_box_header(Float:alpha, Float:duration, Float:red, Float:green, Float:blue)
{
    if (g_focus_box)
    {
        SendMessage(g_focus_box, TAG("FAD1"), alpha, duration, 0, 0);
        SendMessage(g_focus_box, TAG("FCL1"), red, green, blue, duration);
    }
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

set_box_focus(focus_obj)
{
    if (g_focus_box)
    {
        SetPubVar(g_focus_box, "focus", focus_obj);
    }
}

/* Moves the focus onto slot of the box. */
focus_box_slot(slot)
{
    if (g_focus_box)
    {
        SendMessage(g_focus_box, TAG("FOCS"), slot, 0, 0, 0);
    }
}

unfocus_box()
{
    if (g_focus_box)
    {
        SendMessage(g_focus_box, TAG("FOUT"), 0, 0, 0, 0);
    }
}

new g_device_image;
new Float:g_device_x = 130.0;
new Float:g_device_y = 350.0;
new Float:g_device_w = 512.0;
new Float:g_device_h = 512.0;

set_box_title(text[])
{
    set_box_header(0, text);
}

/* Shows text, the position of the save in the album, in the footer of the box. */
set_box_counter(text[])
{
    set_box_footer(text);
}

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

new g_caption_labels2[2][2];
new g_caption_index2;
new Float:g_caption_x2;
new Float:g_caption_y2;

/* Creates the labels of show_caption2, with a shadow if shadow. */
create_captions(shadow)
{
    new i;
    new j;
    {
        new Float:screen_w;
        new Float:screen_h;
        get_screen_size3(screen_w, screen_h);
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
        get_tag_attr("LINK", link_id, "href", url, 256);
        xmlLoadStart(url, _, args, XMLLOAD_MAIN);
    }
    wait_xml_load();
}

/* Same as apply_rot13 of misc.inc. */
apply_rot13_2(text[])
{
    for (new i; text{i}; i++)
    {
        if ('a' <= text{i} <= 'z')
        {
            text{i} = (text{i} - 'a' + 13) % 26 + 'a';
        }
        else if ('A' <= text{i} <= 'Z')
        {
            text{i} = (text{i} - 'A' + 13) % 26 + 'A';
        }
    }
}

/* Same as encode_base64 of misc.inc. */
encode_base64_2(data[], len, out[], out_max)
{
    out_max = out_max * 4;
    new out_len;
    for (new i; i < len; i = i + 3)
    {
        new pad;
        new chunk[1];
        chunk{0} = data{i};
        if (i + 1 < len)
        {
            chunk{1} = data{i + 1};
        }
        else
        {
            pad++;
        }
        if (i + 2 < len)
        {
            chunk{2} = data{i + 2};
        }
        else
        {
            pad++;
        }
        for (new j; j < 4 - pad; j++)
        {
            out{out_len} = get_base64_char2(chunk[0] >> 8 >> (3 - j) * 6 & 0x3F);
            out_len++;
        }
        for (new k; k < pad; k++)
        {
            out{out_len} = '=';
            out_len++;
        }
    }
    return out_len;
}

/* Same as get_base64_char of dom.inc. */
get_base64_char2(value)
{
    if (0 <= value <= 25)
    {
        return value - 0 + 'A';
    }
    if (26 <= value <= 51)
    {
        return value - 26 + 'a';
    }
    if (52 <= value <= 61)
    {
        return value - 52 + '0';
    }
    if (value == 62)
    {
        return '+';
    }
    if (value == 63)
    {
        return '/';
    }
    return 0;
}

/* Same as run_edit_dialog of text.inc. */
run_edit_dialog2(title[], text[], edit_mode, maxlen, max_chars)
{
    new frame_tex = load_texture2("file:/opt0/bn/script/common/texture/textfield.tm2", 0, 0, -1);
    if (maxlen * 4 < max_chars)
    {
        max_chars = maxlen * 4;
    }
    new backdrop = CreateDefocus(Disp2D(0));
    SetOptionI(backdrop, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(backdrop, TAG("LEVL"), 3);
    SetColor(backdrop, 1.0, 1.0, 1.0, 0.0);
    AnimColor(backdrop, 0.3, 0.3, 0.3, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
    wait_ms(200);
    new title_label = CreateLabel(backdrop);
    SetPos(title_label, 95.0, 85.0, 0.0, 0);
    SetColor(title_label, 1.0, 1.0, 1.0, 1.0);
    SetColor(title_label, 1.0, 1.0, 1.0, 1.0);
    SetOptionI(title_label, TAG("HALN"), ALIGN_LEFT);
    SetOptionI(title_label, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(title_label, TAG("FSIZ"), 16);
    SetOptionS(title_label, TAG("UTF8"), title);
    new edit = CreateEdit(backdrop, 1024);
    if (edit_mode == EDIT_MODE_PASSWORD)
    {
        SetOptionI(edit, TAG("PWMD"), '*');
    }
    SetPos(edit, 100.0, 110.0, 0.0, 0);
    SetColor(edit, 1.0, 1.0, 1.0, 1.0);
    SetOptionF(edit, TAG("ZOFS"), 2.0);
    SetOptionI(edit, TAG("HALN"), ALIGN_LEFT);
    SetOptionI(edit, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(edit, TAG("FSIZ"), 15);
    SetOptionS(edit, TAG("UTF8"), text);
    SetOptionI(edit, TAG("W64 "), 396 * 64);
    SetOptionI(edit, TAG("H64 "), 20 * 64);
    SetOptionI(edit, TAG("LM64"), 60 * 64);
    SetOptionI(edit, TAG("RM64"), 60 * 64);
    {
        new bg_colors[4];
        static bg_bytes[16];
        pack_color_bytes(bg_colors, bg_bytes);
        SetOptionP(edit, TAG("BGC4"), bg_colors, 4);
    }
    new frame;
    frame = CreatePlane3x3(backdrop);
    SetPos(frame, 300.0, 120.0, 0.0, 0);
    SetScale(frame, 420.0, 48.0, 1.0);
    SetColor(frame, 1.0, 1.0, 1.0, 1.0);
    SetTexture(frame, frame_tex, 0, -1);
    SetOptionI(frame, TAG("ALPH"), BLEND_ADD);
    SetOptionF(frame, TAG("ZOFS"), 1.0);
    {
        new Float:tex_margins[4] = {0.5, 0.5, 0.5, 0.5};
        SetOptionP(frame, TAG("MGNT"), tex_margins, 4);
        new Float:scale[3];
        GetScale(frame, scale[0], scale[1], scale[2]);
        new Float:margins[4];
        if (scale[0] > scale[1])
        {
            margins[0] = margins[1] = 0.5 * scale[1] / scale[0];
            margins[2] = margins[3] = 0.5;
        }
        else
        {
            margins[2] = margins[3] = 0.5 * scale[0] / scale[1];
            margins[0] = margins[1] = 0.5;
        }
        SetOptionP(frame, TAG("MGN"), margins, 4);
    }
    save_pad_help();
    new skb;
    /* The keyboard, the input mode and the lock flags of skbStart are those of TextIM::Open of textim.cpp. */
    switch (edit_mode)
    {
        case EDIT_MODE_USER_ID:
        {
            new ascii_chars[96] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q',
                    'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', ' ', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
                    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '1', '2', '3', '4',
                    '5', '6', '7', '8', '9', '0', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '-', '=', '\\', '`',
                    '_', '+', '|', '~', '[', ']', '{', '}', ';', ':', '\'', '"', ',', '.', '<', '>', '/', '?', 0};
            skb = skbStart(edit, SKB_KEYBOARD_QWERTY, SKB_INPUT_DIRECT,
                    SKB_LOCK_KEYBOARD | SKB_LOCK_INPUT_MODE | SKB_KANA_LOCK_FIXED, SKB_LENGTH_BYTES, max_chars,
                    ascii_chars, 1);
        }
        case EDIT_MODE_PASSWORD:
        {
            new password_chars[96] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
                    'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', ' ', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
                    'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '1', '2', '3',
                    '4', '5', '6', '7', '8', '9', '0', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '-', '=', '\\',
                    '`', '_', '+', '|', '~', '[', ']', '{', '}', ';', ':', '\'', '"', ',', '.', '<', '>', '/', '?', 0};
            skb = skbStart(edit, SKB_KEYBOARD_QWERTY, SKB_INPUT_DIRECT,
                    SKB_LOCK_KEYBOARD | SKB_LOCK_INPUT_MODE | SKB_KANA_LOCK_FIXED, SKB_LENGTH_BYTES, max_chars,
                    password_chars, 1);
        }
        case EDIT_MODE_KATAKANA:
        {
            skb = skbStart(edit, SKB_KEYBOARD_KATAKANA, SKB_INPUT_KATAKANA,
                    SKB_LOCK_KEYBOARD | SKB_LOCK_INPUT_MODE | SKB_KANA_LOCK_FIXED, SKB_LENGTH_BYTES, max_chars, _, 0);
        }
        case 5:
        {
            skb = skbStart(edit, SKB_KEYBOARD_KATAKANA, SKB_INPUT_KATAKANA_FIXED,
                    SKB_LOCK_KEYBOARD | SKB_LOCK_INPUT_MODE | SKB_KANA_LOCK_FIXED | SKB_FIXED_INPUT, SKB_LENGTH_BYTES,
                    max_chars, _, 0);
        }
        case 6:
        {
            skb = skbStart(edit, SKB_KEYBOARD_KATAKANA, SKB_INPUT_HALF_KATAKANA_FIXED,
                    SKB_LOCK_KEYBOARD | SKB_LOCK_INPUT_MODE | SKB_KANA_LOCK_FIXED | SKB_FIXED_INPUT | SKB_HALF_WIDTH,
                    SKB_LENGTH_BYTES, max_chars, _, 0);
        }
        case EDIT_MODE_KATAKANA_FIXED:
        {
            skb = skbStart(edit, SKB_KEYBOARD_KATAKANA, SKB_INPUT_KATAKANA_FIXED,
                    SKB_LOCK_KEYBOARD | SKB_LOCK_INPUT_MODE | SKB_KANA_LOCK_FIXED | SKB_FIXED_INPUT | SKB_FULL_WIDTH,
                    SKB_LENGTH_BYTES, max_chars, _, 0);
        }
        case 8:
        {
            skb = skbStart(edit, SKB_KEYBOARD_HIRAGANA, SKB_INPUT_HIRAGANA_FIXED,
                    SKB_LOCK_KEYBOARD | SKB_LOCK_INPUT_MODE | SKB_KANA_LOCK_FIXED | SKB_FIXED_INPUT, SKB_LENGTH_BYTES,
                    max_chars, _, 0);
        }
        case EDIT_MODE_NUMBER:
        {
            new digits[11] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 0};
            skb = skbStart(edit, SKB_KEYBOARD_NUMBERS, SKB_INPUT_DIRECT,
                    SKB_LOCK_KEYBOARD | SKB_LOCK_INPUT_MODE | SKB_KANA_LOCK_FIXED, SKB_LENGTH_BYTES, max_chars, digits,
                    1);
        }
        case EDIT_MODE_QWERTY:
        {
            printf("_EDIT_MODE_ASCII == option = %x\n", SKB_LOCK_INPUT_MODE | SKB_HALF_WIDTH);
            skb = skbStart(edit, SKB_KEYBOARD_ALPHANUMERIC, SKB_INPUT_DIRECT, SKB_LOCK_INPUT_MODE | SKB_HALF_WIDTH,
                    SKB_LENGTH_BYTES, max_chars, _, 0);
        }
        default:
        {
            skb = skbStart(edit, SKB_KEYBOARD_DEFAULT, SKB_INPUT_DEFAULT, 0, SKB_LENGTH_BYTES, max_chars, _, 0);
        }
    }
    new ok;
    if (skb == -1)
    {
        printf("edit:");
        getstring(text, maxlen);
        sleep;
        ok = 1;
    }
    else if (wait_skb() == 1)
    {
        skbGetString(text, maxlen);
        ok = 1;
    }
    else
    {
        ok = 0;
    }
    Destroy(title_label);
    Destroy(edit);
    Destroy(frame);
    AnimColor(backdrop, 0.3, 0.3, 0.3, 0.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
    wait_ms(200);
    Destroy(backdrop);
    restore_pad_help2();
    return ok;
}

new g_saved_pad_help;
new g_saved_cross[32];
new g_saved_triangle[32];
new g_saved_square[32];
new g_saved_circle[32];
new g_sql[128];
new g_album_name[32];
new g_album_uri[64];
new g_text[64];
new g_title[64];
new g_album_date[8];
new g_album_id[128];
new g_album_year[1];
new g_album_month[1];
new g_album_day[1];
new g_album_hour[1];
new g_album_minute[1];
new g_album_second[1];

/* Same as load_texture of dom.inc. */
load_texture2(url[], tex_w, tex_h, dec_mode)
{
    if (0 == strlen(url))
    {
        return 0;
    }
    new path[256];
    sprintf(path, url);
    imgDecStart(path, tex_w, tex_h, dec_mode, 100, 0);
    new status = wait_image_decode();
    if (IMG_DONE != status)
    {
        return 0;
    }
    return imgTexID();
}

/* Same as save_pad_help of pfs.inc, with globals of its own. */
save_pad_help()
{
    g_saved_pad_help = GetSystemParam(SYS_PADHELP);
    GetSystemParamStr(SYS_STR_PADHELP_RD, g_saved_cross, 32);
    GetSystemParamStr(SYS_STR_PADHELP_RU, g_saved_triangle, 32);
    GetSystemParamStr(SYS_STR_PADHELP_RL, g_saved_square, 32);
    GetSystemParamStr(SYS_STR_PADHELP_RR, g_saved_circle, 32);
}

/* Same as restore_pad_help of background.inc, with the globals of save_pad_help. */
restore_pad_help2()
{
    SetSystemParam(SYS_PADHELP, g_saved_pad_help);
    SetSystemParamStr(SYS_STR_PADHELP_RD, g_saved_cross);
    SetSystemParamStr(SYS_STR_PADHELP_RU, g_saved_triangle);
    SetSystemParamStr(SYS_STR_PADHELP_RL, g_saved_square);
    SetSystemParamStr(SYS_STR_PADHELP_RR, g_saved_circle);
}

/*
 * Adds an album (a favorite album for ALBUM_FAVORITE) named name, or the first free "<new album>N", asked with the
 * software keyboard if ask_name; an album gets a folder of pfs:/__contents/contents/video named by its date. Sets
 * the window variables title, uri and album_id; returns 0 or ADD_ALBUM_*.
 */
add_album(album_mode, ask_name, name[])
{
    new status;
    new count;
    new i;
    status = datOpen(DAT_DATABASE, g_db_name, DAT_ORDER_MTIME_DESC);
    if (0 != status)
    {
        return ADD_ALBUM_FAILED;
    }
    status = wait_database();
    if (status < 0)
    {
        return ADD_ALBUM_FAILED;
    }
    if (ALBUM_FAVORITE != album_mode)
    {
        sprintf(g_sql, "select * from sce_movie where content_attr = %d", ATTR_ALBUM);
    }
    else
    {
        sprintf(g_sql, "select * from sce_movie where content_attr = %d", ATTR_FAV_ALBUM);
    }
    status = datSQLExec(g_sql);
    if (0 != status)
    {
        return ADD_ALBUM_FAILED;
    }
    status = wait_database();
    if (status < 0)
    {
        return ADD_ALBUM_FAILED;
    }
    count = datGetNum();
    if (count >= MAX_ITEMS)
    {
        return ADD_ALBUM_FULL;
    }
    g_text = "";
    if (ALBUM_FAVORITE != album_mode)
    {
        get_tag_attr("TEXT", "msgmovie_add_album_new", "value", g_text, 64);
    }
    else
    {
        get_tag_attr("TEXT", "msgmovie_favalbum", "value", g_text, 64);
    }
    err20007();
    i = 0;
    for (; i < count; i++)
    {
        datGetData(i, COL_TITLE, g_title, 64);
        err20008(g_title);
        if (0 == i % 200)
        {
            sleep;
        }
    }
    i = 0;
    for (; i < MAX_ITEMS; i++)
    {
        g_album_name = "";
        if (ALBUM_FAVORITE != album_mode)
        {
            if (strlen(name) > 0)
            {
                sprintf(g_album_name, name);
            }
            else
            {
                sprintf(g_album_name, "%s%d", g_text, i + 1);
            }
        }
        else
        {
            sprintf(g_album_name, "%s%d", g_text, i + 1);
        }
        if (0 == err20009(g_album_name))
        {
            break;
        }
        if (0 == i % 50)
        {
            sleep;
        }
    }
    err20007();
    if (0 != ask_name)
    {
        get_tag_attr("TEXT", "msgmovie_albumname", "value", g_album_id, 128);
        sprintf(g_text, g_album_name);
        if (!run_edit_dialog2(g_album_id, g_text, 0, 64, 256))
        {
            return ADD_ALBUM_FAILED;
        }
        if (strlen(g_text) > 0)
        {
            escape_sql(g_album_name, g_text, 32);
        }
        else
        {
            return ADD_ALBUM_NO_NAME;
        }
    }
    date(g_album_year[0], g_album_month[0], g_album_day[0], 0);
    time(g_album_hour[0], g_album_minute[0], g_album_second[0], 0);
    sprintf(g_album_date, "%04d%02d%02d%02d%02d%02d", g_album_year, g_album_month, g_album_day, g_album_hour,
            g_album_minute, g_album_second);
    if (ALBUM_FAVORITE != album_mode)
    {
        sprintf(g_text, "pfs:/__contents/contents/video");
        fileMkdir(g_text);
        sprintf(g_album_uri, "pfs:/__contents/contents/video/%s", g_album_date);
        fileMkdir(g_album_uri);
        sprintf(g_album_id, "%s%s%s", g_album_name, g_album_date, g_album_uri);
        sprintf(g_sql,
                "insert into sce_movie (title,album_name,create_date,uri,album_id,content_type,content_subtype,content_attr,table_version) values('%s','%s','%s','%s','%s',%d,%d,%d,%d)",
                g_album_name, g_album_name, g_album_date, g_album_uri, g_album_id, 0, 0, ATTR_ALBUM, 512);
    }
    else
    {
        sprintf(g_album_uri, "%s%s", g_album_name, g_album_date);
        sprintf(g_sql,
                "insert into sce_movie (title,album_name,create_date,uri,album_id,content_type,content_subtype,content_attr,table_version) values('%s','%s','%s','%s','%s',%d,%d,%d,%d)",
                g_album_name, g_album_name, g_album_date, g_album_uri, g_album_id, 0, 0, ATTR_FAV_ALBUM, 512);
    }
    status = datSQLExec(g_sql);
    if (0 != status)
    {
        return ADD_ALBUM_FAILED;
    }
    status = wait_database();
    if (status < 0)
    {
        return ADD_ALBUM_FAILED;
    }
    set_wnd_var_str("title", g_album_name);
    set_wnd_var_str("uri", g_album_uri);
    set_wnd_var_str("album_id", g_album_id);
    return 0;
}

/*
 * Copies src into dest doubling each single quote and backslash, for a quoted SQL string; returns -1 when it wrote
 * maxlen characters or more.
 */
escape_sql(dest[], src[], maxlen)
{
    new len = strlen(src);
    new out;
    for (new i; i < len && out < maxlen * 4 - 2; i++)
    {
        if (src{i} == '\'')
        {
            dest{out++} = '\'';
            dest{out++} = '\'';
        }
        else if (src{i} == '\\')
        {
            dest{out++} = '\\';
            dest{out++} = '\\';
        }
        else
        {
            dest{out++} = src{i};
        }
    }
    dest{out++} = 0;
    if (out >= maxlen)
    {
        return -1;
    }
    return 0;
}

/* Same as write_current_channel of sysconf.inc. */
write_current_channel(channel[])
{
    new tries;
    new ret;
    avalDelete("PS1DiscSpeed");
    avalDelete("PS1TextureMapping");
    if (0 == strcmp(channel, "Top"))
    {
        avalSet("ChannelIntoID", "PSBB-GUIDE");
        avalSet("ChannelCategory", "GROUP-TOPMENU");
    }
    avalSet("CurrentCh", channel);
    tries = 0;
    for (; tries < 3; tries++)
    {
        ret = avalWriteFile();
        if (ret >= 0)
        {
            break;
        }
    }
    return ret;
}

new Float:g_screen_w;
new Float:g_screen_h;
public camera_focus; /* the point the camera looks at */
public camera_uptgt; /* unused, but part of the layout */
public focus_glowfly; /* the focus object */
new g_active; /* g_mode_buttons or g_album: the one that takes the pad */
new g_mode_buttons;
new g_album;
new g_default_icon;
new g_album_icon;
new g_next_link[256]; /* the page to leave for: a "LINK" id or "top" */
new g_next_args[256];
new g_args[128];
new g_quit;
new g_disc_type; /* CDVD_* */
new g_album_mode; /* ALBUM_* */
new g_start_mode; /* the album the page was opened on: ALBUM_FAVORITE keeps the page on the favorite albums */
new g_album_title[256];
new g_album_count;
new g_album_idle; /* the album has stopped moving */
new g_enter_album; /* the focus goes into the album once it is idle */
new g_script_args[512];
new g_msg[128];
/* Lines of TEXT elements joined into g_msg. */
new g_line1[32];
new g_line2[32];
new g_line3[32];
new g_line4[32];
new g_uri[256];
new g_attr[4];
new g_player_url[256]; /* the URL of the movie player page */
new g_album_folder[256];
new g_fav_album_id[64];
new g_album_select; /* the selected album, kept across the pages in the aval file; -1 for none */
new g_sort_order; /* SORT_* */
new g_sql2[128];

main()
{
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
 * Builds the page: the album and the selection of the last visit, or the album of the argument mode; makes sure the
 * database has the favorite album and the row of the sort order.
 */
init_page()
{
    new value[4];
    start_top_page(0);
    load_common_bg("bg", 1000.0);
    set_background_poor(600.0, 0);
    get_screen_size3(g_screen_w, g_screen_h);
    create_camera();
    link_bg_camera(2.0, 0);
    reset_bg_camera();
    g_next_link = "top";
    g_next_args = "";
    g_quit = 0;
    g_disc_type = CDVD_NO_DISC;
    g_album_idle = 0;
    g_album_mode = ALBUM_COLLECTION;
    g_start_mode = ALBUM_COLLECTION;
    if (0 == avalGetValue("MovieAlbumMode", 0, value, 4))
    {
        g_album_mode = atoi(value);
        g_start_mode = atoi(value);
        if (ALBUM_COLLECTION != g_album_mode && ALBUM_DISC != g_album_mode)
        {
            g_album_mode = ALBUM_COLLECTION;
            g_start_mode = ALBUM_COLLECTION;
        }
        avalDelete("MovieAlbumMode");
    }
    else
    {
        GetArg("mode", value, 4);
        if (0 == strcmp("favorite", value))
        {
            g_album_mode = ALBUM_FAVORITE;
            g_start_mode = ALBUM_FAVORITE;
        }
        else if (0 == strcmp("collection", value))
        {
            g_album_mode = ALBUM_COLLECTION;
            g_start_mode = ALBUM_COLLECTION;
        }
    }
    g_album_select = -1;
    if (0 == avalGetValue("MovieAlbumCurNo", 0, value, 4))
    {
        g_album_select = atoi(value);
        if (g_album_select < 0)
        {
            g_album_select = -1;
        }
        avalDelete("MovieAlbumCurNo");
    }
    else if (GetArg("currentAlbumNo", value, 4))
    {
        g_album_select = atoi(value);
        if (g_album_select < 0)
        {
            g_album_select = -1;
        }
    }
    focus_glowfly = create_focus_plane2("focus");
    sprintf(g_script_args, "owner = %d", Self());
    {
        new watcher = CreateNull(Self());
        AttachScript(watcher, "media_watcher_main", _, g_script_args);
    }
    err20004();
    new status;
    status = datOpen(DAT_DATABASE, g_db_name, DAT_ORDER_MTIME_DESC);
    if (0 != status)
    {
        return;
    }
    status = wait_database();
    if (status < 0)
    {
        return;
    }
    sprintf(g_sql2, "select * from sce_movie where content_attr = %d", ATTR_FAV_ALBUM);
    status = datSQLExec(g_sql2);
    if (0 != status)
    {
        return;
    }
    status = wait_database();
    if (status < 0)
    {
        return;
    }
    if (datGetNum() <= 0)
    {
        add_album(ALBUM_FAVORITE, 0, "");
    }
    g_sort_order = SORT_NEWEST;
    sprintf(g_sql2, "select * from sce_movie where content_attr = %d", ATTR_SETTINGS);
    status = datSQLExec(g_sql2);
    if (0 != status)
    {
        return;
    }
    status = wait_database();
    if (status < 0)
    {
        return;
    }
    if (datGetNum() <= 0)
    {
        sprintf(g_sql2,
                "insert into sce_movie (title,album_name,create_date,uri,album_id,content_type,content_subtype,content_attr,table_version) values('%s','%s','%s','%s','%s',%d,%d,%d,%d)",
                "", "", "", "", "", g_sort_order, 0, ATTR_SETTINGS, 512);
        status = datSQLExec(g_sql2);
        if (0 != status)
        {
            return;
        }
        status = wait_database();
        if (status < 0)
        {
            return;
        }
    }
    else
    {
        datGetData(0, COL_CONTENT_TYPE, g_sql2, 128);
        g_sort_order = atoi(g_sql2);
        if (g_sort_order < 0 || g_sort_order > 1)
        {
            g_sort_order = SORT_NEWEST;
        }
    }
    g_default_icon = load_icon_texture("default_icon");
    g_album_icon = load_icon_texture("default_album_icon");
    create_device_image();
    show_device_image(g_album_device[g_album_mode]);
    create_focus_box(0.7625 * g_screen_w, 0.5 * g_screen_h);
    set_box_focus(focus_glowfly);
    set_box_title("");
    fade_box(0.0, 0.0, 1.0, 1.0, 1.0);
    create_captions(0);
    set_caption_pos2(0.546875 * g_screen_w, 0.5 * g_screen_h);
    g_mode_buttons = create_mode_buttons();
    SetPubVar(g_mode_buttons, "select", g_album_mode);
    SetPubVar(g_mode_buttons, "cursor", g_album_mode);
    new enter;
    if (g_album_select < 0)
    {
        enter = 0;
    }
    else
    {
        enter = 1;
    }
    g_active = g_mode_buttons;
    g_album = 0;
    new mode = g_album_mode;
    g_album_mode--; /* differs from the mode, so that change_album reloads the album */
    change_album(mode, 0, enter);
}

close_db()
{
    datClose();
}

/*
 * Handles the messages of one frame: "CHNG" (album) and "FOCS", "PUSH" (_, button) of the radio buttons or of the
 * album, whichever takes the pad; "ITEM" (menu, path, index) of the album; "DISC" (type) of the disc watcher.
 */
handle_messages()
{
    new msg;
    new a;
    new b;
    new c;
    while (RecvMessage(msg, a, b, c))
    {
        if (g_active == g_mode_buttons)
        {
            if (msg == TAG("CHNG"))
            {
                change_album(a, 1, 1);
            }
            if (msg == TAG("FOCS"))
            {
                on_buttons_focus(a);
            }
            if (msg == TAG("PUSH"))
            {
                on_buttons_push(b);
            }
        }
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
            send_album_item(a, b, c, g_album_count);
        }
        if (msg == TAG("PUSH"))
        {
            on_push(b);
        }
        if (msg == TAG("DISC"))
        {
            on_disc_change(a);
        }
    }
    update_album_info();
    update_buttons_pad_help();
    return 1;
}

/* From the album, gives the pad back to the radio buttons; from the radio buttons, leaves for the top page. */
go_back()
{
    if (g_active == g_album)
    {
        sndSePlay(SE_CANCEL);
        SendMessage(g_album, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
        unfocus_box();
        dim_album();
        sleep;
        sleep;
        g_active = g_mode_buttons;
        light_buttons();
        SendMessage(g_mode_buttons, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
        g_enter_album = 0;
    }
    else
    {
        SendMessage(g_active, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
        g_quit = 1;
        g_next_link = "top";
        switch (g_album_mode)
        {
            case ALBUM_COLLECTION:
            {
                g_next_args = "channel_id = BNCH-MOVIE-COLLECTION ;";
            }
            case ALBUM_FAVORITE:
            {
                g_next_args = "channel_id = BNCH-MOVIE-FAVORITE ;";
            }
            default:
            {
                g_next_args = "channel_id = BNCH-MOVIE-COLLECTION ;";
            }
        }
        sndSePlay(SE_CANCEL);
        return;
    }
}

/* The cross goes back to the page before. */
on_push(button)
{
    if (button == TAG("RDWN"))
    {
        go_back();
        return;
    }
}

/* Reloads the disc album when the disc changes, leaving it if there is no disc any more. */
on_disc_change(type)
{
    g_disc_type = type;
    if (ALBUM_DISC != g_album_mode)
    {
        return;
    }
    if (g_disc_type <= CDVD_DETECTING)
    {
        if (g_active == g_album)
        {
            go_back();
        }
        g_album_mode = ALBUM_COLLECTION;
        change_album(ALBUM_DISC, 0, 0);
    }
    else
    {
        g_album_mode = ALBUM_COLLECTION;
        change_album(ALBUM_DISC, 1, 1);
    }
}

/* Shows album mode, loading it if it is another one; with enter the focus goes into it. */
change_album(mode, with_se, enter)
{
    if (g_start_mode == ALBUM_FAVORITE && ALBUM_COLLECTION == mode)
    {
        mode = ALBUM_FAVORITE;
    }
    switch (mode)
    {
        case ALBUM_COLLECTION:
        {
            SetPubVar(g_mode_buttons, "padhelp", PADHELP_CIRCLE | PADHELP_CROSS | PADHELP_TRIANGLE);
        }
        case ALBUM_DISC:
        {
            SetPubVar(g_mode_buttons, "padhelp", PADHELP_CIRCLE | PADHELP_CROSS);
        }
        case ALBUM_FAVORITE:
        {
            SetPubVar(g_mode_buttons, "padhelp", PADHELP_CIRCLE | PADHELP_CROSS);
        }
    }
    if (with_se)
    {
        sndSePlay(SE_DECIDE);
    }
    send_focus2(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    SendMessage(g_mode_buttons, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
    dim_buttons();
    SendMessage(g_active, TAG("CHNG"), 0, 0, 0, 0);
    SetPubVar(g_mode_buttons, "disable_selected", 1);
    if (g_album_mode != mode)
    {
        g_album_mode = mode;
        show_device_image(g_album_device[g_album_mode]);
        if (g_album_select < 0)
        {
            g_album_count = load_album(g_album, g_album_mode, 0);
        }
        else
        {
            g_album_count = load_album(g_album, g_album_mode, g_album_select);
        }
        if (enter)
        {
            g_enter_album = 1;
        }
    }
    else if (g_album_count > 0 && enter)
    {
        g_active = g_album;
        focus_box_slot(0);
        SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        light_album();
        g_enter_album = 1;
    }
    if (g_album_count <= 0)
    {
        g_album_count = 0;
        g_active = g_mode_buttons;
        SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        light_buttons();
        g_enter_album = 0;
    }
    else if (g_album_count > 0 && !enter)
    {
        SendMessage(g_album, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
        unfocus_box();
        dim_album();
        g_enter_album = 0;
        sleep;
        sleep;
        g_active = g_mode_buttons;
        light_buttons();
        SendMessage(g_mode_buttons, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
    }
    SetPubVar(g_mode_buttons, "disable_selected", 0);
}

/* The focus stays on g_active whichever way (direction, a TAG) the pad leaves it. */
on_buttons_focus(direction)
{
    new value = direction; /* redundant as written */
    direction = value;
    SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
}

/* Triangle on the first radio button, but for the favorite albums, opens the option menu of the radio buttons. */
on_buttons_push(button)
{
    new cursor = GetPubVar(g_mode_buttons, "cursor");
    if (ALBUM_FAVORITE != g_album_mode && 0 == cursor && button == TAG("RUP"))
    {
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
        SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
        sndSePlay(SE_OPEN);
        sndSePlay(SE_WINDOW_OPEN);
        SendMessage(g_active, TAG("PUSH"), TAG("RUP"), 0, 0, 0);
        SendMessage(g_active, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
        send_focus2(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        if (g_active == g_album)
        {
            unfocus_box();
        }
        SendMessage(g_mode_buttons, TAG("FADE"), 0, 400.0, 0, 0);
        SetPos(g_focus_box, 0.7625 * g_screen_w, 0.5 * g_screen_h, 0.0, 0);
        AnimMoveHermite(g_focus_box, 0.7625 * g_screen_w - 300.0, 0.5 * g_screen_h, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT,
                1.0);
        fade_box_footer(0.0, 400.0, 0.7843, 0.7843, 0.7843);
        show_caption2(_, _, 0.0, 400.0, 0.0);
        slide_camera(1);
        wait_ms(floatround(750.0, 0));
        new changed;
        new result;
        for (; ; )
        {
            result = pop_std_menu("optmenu", g_option_groups[OPTIONS_BUTTONS], MENU_SIDE_RIGHT, result - MENU_ITEM);
            if (result == MENU_CLOSED)
            {
                break;
            }
            changed = changed | run_buttons_option(result);
            SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
            break;
        }
        sndSePlay(SE_WINDOW_CLOSE);
        center_camera();
        SetPos(g_focus_box, 0.7625 * g_screen_w - 300.0, 0.5 * g_screen_h, 0.0, 0);
        AnimMoveHermite(g_focus_box, 0.7625 * g_screen_w, 0.5 * g_screen_h, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
        wait_ms(floatround(750.0, 0));
        fade_box_footer(1.0, 400.0, 0.7843, 0.7843, 0.7843);
        SendMessage(g_mode_buttons, TAG("FADE"), 1.0, 400.0, 0, 0);
        if (!changed)
        {
            if (g_album_count > 0)
            {
                dim_album();
            }
            else
            {
                show_album_error(g_album_count);
            }
            dim_buttons();
            if (g_active == g_album)
            {
                focus_box_slot(0);
            }
            SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        }
        else
        {
            new mode = g_album_mode;
            g_album_mode--; /* differs from the mode, so that change_album reloads the album */
            change_album(mode, 0, 1);
        }
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
        return;
    }
    SendMessage(g_active, TAG("PUSH"), 0, 0, 0, 0);
}

/* Sets the pad help of the radio buttons: triangle also on the first button, but for the favorite albums. */
update_buttons_pad_help()
{
    if (g_enter_album == 1)
    {
        return;
    }
    new cursor = GetPubVar(g_mode_buttons, "cursor");
    if (g_active == g_mode_buttons)
    {
        if (0 == cursor)
        {
            switch (g_album_mode)
            {
                case ALBUM_COLLECTION:
                {
                    SetPubVar(g_mode_buttons, "padhelp", PADHELP_CIRCLE | PADHELP_CROSS | PADHELP_TRIANGLE);
                }
                case ALBUM_DISC:
                {
                    SetPubVar(g_mode_buttons, "padhelp", PADHELP_CIRCLE | PADHELP_CROSS | PADHELP_TRIANGLE);
                }
                case ALBUM_FAVORITE:
                {
                    SetPubVar(g_mode_buttons, "padhelp", PADHELP_CIRCLE | PADHELP_CROSS);
                }
            }
        }
        else
        {
            SetPubVar(g_mode_buttons, "padhelp", PADHELP_CIRCLE | PADHELP_CROSS);
        }
        SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
    }
}

/* Creates the album item of index and sends it to the album ("ITEM"). */
send_album_item(menu, path, index, count)
{
    if (IsValidObject(path))
    {
        new item = create_album_item(path, index, count);
        SetOptionF(item, TAG("ZOFS"), -50.0);
        SendMessage(menu, TAG("ITEM"), item, path, 0, 0);
    }
}

/* The focus stays on g_active whichever way (direction, a TAG) the pad leaves it. */
on_album_focus(direction)
{
    new value = direction; /* redundant as written */
    direction = value;
    SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
}

/*
 * Circle on the album opens the selected album in the movie player page, or boots the DVD video; triangle opens the
 * option menu of the album and reloads it if an option changed it.
 */
on_album_push(button)
{
    if (button == TAG("RRGT"))
    {
        SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
        sndSePlay(SE_DECIDE);
        SendMessage(g_active, TAG("PUSH"), 0, 0, 0, 0);
        if (g_album_idle)
        {
            if (ALBUM_COLLECTION == g_album_mode)
            {
                new select = GetPubVar(g_album, "select");
                new type_text[8];
                datGetData(select, COL_CONTENT_TYPE, type_text, 8);
                datGetData(select, COL_URI, g_album_folder, 256);
                datGetData(select, COL_ALBUM_NAME, g_album_title, 256);
                sprintf(g_script_args,
                        "album_mode = %d ; file = %s ; bref = %s ; uri = %s ; currentAlbumNo = %d ; album_name = %s",
                        g_album_mode, g_uri, g_player_url, g_album_folder, select, g_album_title);
                SetSystemParam(SYS_PADHELP, 0);
                get_tag_attr("LINK", "movie_player", "href", g_player_url, 256);
                xmlLoadStart(g_player_url, _, g_script_args, XMLLOAD_MAIN);
                wait_xml_load();
            }
            else if (g_album_mode == ALBUM_FAVORITE)
            {
                new select = GetPubVar(g_album, "select");
                new type_text[8];
                datGetData(select, COL_CONTENT_TYPE, type_text, 8);
                datGetData(select, COL_ALBUM_ID, g_fav_album_id, 64);
                datGetData(select, COL_ALBUM_NAME, g_album_title, 256);
                sprintf(g_script_args,
                        "album_mode = %d ; file = %s ; bref = %s ; uri = %s ; currentAlbumNo = %d ; album_name = %s",
                        g_album_mode, g_uri, g_player_url, g_fav_album_id, select, g_album_title);
                SetSystemParam(SYS_PADHELP, 0);
                get_tag_attr("LINK", "movie_player", "href", g_player_url, 256);
                xmlLoadStart(g_player_url, _, g_script_args, XMLLOAD_MAIN);
                wait_xml_load();
            }
            else
            {
                new dvd_version[16];
                dvd_version = "";
                err11012();
                sverGetDVDVer(dvd_version);
                err11013();
                if (strlen(dvd_version) > 2)
                {
                    err11012();
                    boot_dvd_video();
                    err11013();
                }
                else
                {
                    new msg[256];
                    new ok[64];
                    get_tag_attr("TEXT", "msgmovie_dvd_noboot", "value", msg, 256);
                    get_tag_attr("TEXT", "msgmovie_common_ok", "value", ok, 64);
                    SendMessage(g_active, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                    send_focus2(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                    if (g_active == g_album)
                    {
                        unfocus_box();
                    }
                    show_message(msg, ok);
                    if (g_active == g_album)
                    {
                        focus_box_slot(0);
                    }
                    SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                }
            }
        }
        SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
        return;
    }
    if (button == TAG("RUP"))
    {
        if (ALBUM_COLLECTION == g_album_mode)
        {
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
            SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
            sndSePlay(SE_OPEN);
            sndSePlay(SE_WINDOW_OPEN);
            SendMessage(g_active, TAG("PUSH"), TAG("RUP"), 0, 0, 0);
            SendMessage(g_active, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
            send_focus2(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            if (g_active == g_album)
            {
                unfocus_box();
            }
            SendMessage(g_mode_buttons, TAG("FADE"), 0, 400.0, 0, 0);
            SetPos(g_focus_box, 0.7625 * g_screen_w, 0.5 * g_screen_h, 0.0, 0);
            AnimMoveHermite(g_focus_box, 0.7625 * g_screen_w - 300.0, 0.5 * g_screen_h, 0.0, 750.0, 0.0,
                    ANIM_FUNC_EXPONENT, 1.0);
            fade_box_footer(0.0, 400.0, 0.7843, 0.7843, 0.7843);
            slide_camera(1);
            wait_ms(floatround(750.0, 0));
            new changed;
            new result;
            for (; ; )
            {
                result = pop_std_menu("optmenu", g_option_groups[g_album_mode], MENU_SIDE_RIGHT, result - MENU_ITEM);
                if (result == MENU_CLOSED)
                {
                    break;
                }
                changed = changed | run_album_option(result);
                SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
                if (changed == OPTION_CHANGED)
                {
                    break;
                }
            }
            sndSePlay(SE_WINDOW_CLOSE);
            center_camera();
            SetPos(g_focus_box, 0.7625 * g_screen_w - 300.0, 0.5 * g_screen_h, 0.0, 0);
            AnimMoveHermite(g_focus_box, 0.7625 * g_screen_w, 0.5 * g_screen_h, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT,
                    1.0);
            wait_ms(floatround(750.0, 0));
            fade_box_footer(1.0, 400.0, 0.7843, 0.7843, 0.7843);
            SendMessage(g_mode_buttons, TAG("FADE"), 1.0, 400.0, 0, 0);
            if (!changed)
            {
                light_album();
                dim_buttons();
                if (g_active == g_album)
                {
                    focus_box_slot(0);
                }
                SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
            }
            else
            {
                go_back();
                new mode = g_album_mode;
                g_album_mode--; /* differs from the mode, so that change_album reloads the album */
                change_album(mode, 0, 1);
            }
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
            SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
            return;
        }
        if (g_album_mode == ALBUM_FAVORITE)
        {
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
            SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
            sndSePlay(SE_OPEN);
            sndSePlay(SE_WINDOW_OPEN);
            SendMessage(g_active, TAG("PUSH"), TAG("RUP"), 0, 0, 0);
            SendMessage(g_active, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
            send_focus2(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            if (g_active == g_album)
            {
                unfocus_box();
            }
            SendMessage(g_mode_buttons, TAG("FADE"), 0, 400.0, 0, 0);
            SetPos(g_focus_box, 0.7625 * g_screen_w, 0.5 * g_screen_h, 0.0, 0);
            AnimMoveHermite(g_focus_box, 0.7625 * g_screen_w - 300.0, 0.5 * g_screen_h, 0.0, 750.0, 0.0,
                    ANIM_FUNC_EXPONENT, 1.0);
            fade_box_footer(0.0, 400.0, 0.7843, 0.7843, 0.7843);
            slide_camera(1);
            wait_ms(floatround(750.0, 0));
            new changed;
            new result;
            for (; ; )
            {
                result = pop_std_menu("optmenu", g_option_groups[g_album_mode], MENU_SIDE_RIGHT, result - MENU_ITEM);
                if (result == MENU_CLOSED)
                {
                    break;
                }
                changed = changed | run_album_option(result);
                SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
            }
            sndSePlay(SE_WINDOW_CLOSE);
            center_camera();
            SetPos(g_focus_box, 0.7625 * g_screen_w - 300.0, 0.5 * g_screen_h, 0.0, 0);
            AnimMoveHermite(g_focus_box, 0.7625 * g_screen_w, 0.5 * g_screen_h, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT,
                    1.0);
            wait_ms(floatround(750.0, 0));
            fade_box_footer(1.0, 400.0, 0.7843, 0.7843, 0.7843);
            SendMessage(g_mode_buttons, TAG("FADE"), 1.0, 400.0, 0, 0);
            if (!changed)
            {
                light_album();
                dim_buttons();
                if (g_active == g_album)
                {
                    focus_box_slot(0);
                }
                SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
            }
            else
            {
                go_back();
                new mode = g_album_mode;
                g_album_mode--; /* differs from the mode, so that change_album reloads the album */
                change_album(mode, 0, 1);
            }
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
            SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
            return;
        }
    }
    SendMessage(g_active, TAG("PUSH"), 0, 0, 0, 0);
}

/* Shows the name and the number of the selected album in the box when the album stops. */
update_album_info()
{
    if (g_album_count <= 0)
    {
        return;
    }
    new was_idle = g_album_idle;
    new select = GetPubVar(g_album, "select");
    new idle = GetPubVar(g_album, "idle");
    g_album_idle = idle == 1;
    g_album_select = select;
    if (g_album_idle != was_idle)
    {
        if (g_album_idle)
        {
            new text[256];
            if (ALBUM_COLLECTION == g_album_mode)
            {
                datGetData(select, COL_TITLE, text, 256);
                set_box_header(0, text);
                sprintf(text, "%d/%d", select + 1, g_album_count);
                set_box_counter(text);
            }
            else if (g_album_mode == ALBUM_FAVORITE)
            {
                datGetData(select, COL_TITLE, text, 256);
                set_box_header(0, text);
                sprintf(text, "%d/%d", select + 1, g_album_count);
                set_box_counter(text);
            }
            else
            {
                get_tag_attr("TEXT", "msgmovie_dev_dvdvideo", "value", text, 256);
                set_box_header(0, text);
                set_box_counter("1/1");
            }
            if (g_enter_album)
            {
                g_active = g_album;
                focus_box_slot(0);
                light_album();
                SendMessage(g_album, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
            }
            if (g_active == g_album)
            {
                focus_box_slot(0);
            }
        }
        else
        {
            if (g_active == g_album)
            {
                unfocus_box();
            }
            fade_box(0.0, 200.0, 1.0, 1.0, 1.0);
        }
    }
}

/* Runs the option result of the menu of the radio buttons: a new album or the sort order; returns 1 if it changed. */
run_buttons_option(result)
{
    result = result - MENU_ITEM;
    switch (g_album_mode)
    {
        case ALBUM_COLLECTION:
        {
            switch (result)
            {
                case ITEM_NEW_ALBUM:
                {
                    return add_new_album();
                }
                case ITEM_SORT:
                {
                    return choose_sort_order();
                }
            }
        }
        case ALBUM_DISC:
        {
            switch (result)
            {
                case ITEM_NEW_ALBUM:
                {
                    return add_new_album();
                }
                case ITEM_SORT:
                {
                    return choose_sort_order();
                }
            }
        }
    }
    return 0;
}

/*
 * Runs the option result of the menu of an album: add it to a favorite album, delete it, information; returns
 * OPTION_CHANGED or OPTION_RENAMED if the albums changed.
 */
run_album_option(result)
{
    result = result - MENU_ITEM;
    switch (g_album_mode)
    {
        case ALBUM_COLLECTION:
        {
            switch (result)
            {
                case ITEM_ADD_TO_FAVORITE:
                {
                    return add_album_to_favorites();
                }
                case ITEM_DELETE:
                {
                    return remove_album();
                }
                case ITEM_INFORMATION:
                {
                    if (0 != show_information())
                    {
                        new title[32];
                        /* NOTE: bug: title has 32 cells, avalGetValue may fill 128 */
                        avalGetValue("TitleName", 0, title, 128);
                        set_box_header(0, title);
                        return OPTION_RENAMED;
                    }
                    return 0;
                }
            }
        }
        case ALBUM_FAVORITE:
        {
            switch (result)
            {
                case ITEM_FAV_INFORMATION:
                {
                    if (0 != show_information())
                    {
                        new title[32];
                        /* NOTE: bug: title has 32 cells, avalGetValue may fill 128 */
                        avalGetValue("TitleName", 0, title, 128);
                        set_box_header(0, title);
                        return OPTION_RENAMED;
                    }
                    return 0;
                }
            }
        }
    }
    return 0;
}

/* Runs create_album of movie_dialog and returns its result. */
add_new_album()
{
    SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    sprintf(g_script_args, "album_mode = %d", g_album_mode);
    AttachScript(dialog, "create_album", "movie_dialog", g_script_args);
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

/* Asks for the sort order of the albums and saves it in the settings row; returns 1 if it changed. */
choose_sort_order()
{
    new order;
    new changed;
    new result;
    result = pop_std_menu("optmenu", g_option_groups[OPTIONS_SORT], MENU_SIDE_RIGHT, g_sort_order);
    if (result == MENU_CLOSED)
    {
    }
    else
    {
        switch (result - MENU_ITEM)
        {
            case SORT_NEWEST:
            {
                order = SORT_NEWEST;
            }
            case SORT_OLDEST:
            {
                order = SORT_OLDEST;
            }
        }
        if (g_sort_order != order)
        {
            g_sort_order = order;
            changed = 1;
            new status;
            sprintf(g_sql2, "update sce_movie set content_type = %d where content_attr = 256", g_sort_order);
            status = datSQLExec(g_sql2);
            if (0 != status)
            {
                return 0;
            }
            status = wait_database();
            if (status < 0)
            {
                return 0;
            }
        }
    }
    return changed;
}

/* Runs addfav_album of movie_dialog on the selected album and returns its result. */
add_album_to_favorites()
{
    SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    new select = GetPubVar(g_album, "select");
    datGetData(select, COL_ALBUM_ID, g_fav_album_id, 64);
    datGetData(select, COL_URI, g_fav_album_id, 64);
    sprintf(g_script_args, "album_mode = %d ; uri = %s", g_album_mode, g_fav_album_id);
    AttachScript(dialog, "addfav_album", "movie_dialog", g_script_args);
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

/* Runs delete_album of movie_dialog on the selected album and returns its result. */
remove_album()
{
    SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    new select = GetPubVar(g_album, "select");
    datGetData(select, COL_URI, g_uri, 256);
    sprintf(g_script_args, "album_mode = %d ; uri = %s", g_album_mode, g_uri);
    AttachScript(dialog, "delete_album", "movie_dialog", g_script_args);
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

/* Opens the information page of the selected album; returns 1 if it was renamed there. */
show_information()
{
    new renamed;
    new select;
    new url[128];
    SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
    select = GetPubVar(g_album, "select");
    datGetData(select, COL_CONTENT_ATTR, g_attr, 4);
    datGetData(select, COL_URI, g_uri, 256);
    datGetData(select, COL_ALBUM_NAME, g_album_title, 256);
    sprintf(g_args, "attr = %d ; uri = %s ; album_name = %s", atoi(g_attr), g_uri, g_album_title);
    get_tag_attr("LINK", "infomenu", "href", url, 128);
    xmlLoadStart(url, _, g_args, XMLLOAD_MODAL);
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

/* Creates the radio buttons of the albums: the hard disk and the DVD, or the favorite albums alone. */
create_mode_buttons()
{
    new buttons;
    buttons = CreateNull(Disp2D(0));
    new label0[64];
    new label1[64];
    new count = 2;
    if (g_album_mode == ALBUM_FAVORITE)
    {
        get_group_tag_attr2("TEXT", "albummenu", 2, "value", label0, 64);
        count = 1;
        sprintf(g_args, "btn0 = %s ; interval = %d", label0, floatround(0.08 * g_screen_h, 0));
    }
    else
    {
        get_group_tag_attr2("TEXT", "albummenu", 0, "value", label0, 64);
        get_group_tag_attr2("TEXT", "albummenu", 1, "value", label1, 64);
        sprintf(g_args, "btn0 = %s ; btn1 = %s ; interval = %d", label0, label1, floatround(0.08 * g_screen_h, 0));
    }
    AttachScript(buttons, _, "radio_button_v", g_args);
    new Float:y = 0.5 - 0.5 * (0.08 * float(count - 1));
    SetPos(buttons, 0.14 * g_screen_w, g_screen_h * y, 0.0, 0);
    SetColor(buttons, 0.215, 0.708, 0.999, 0.0);
    SetOptionI(buttons, TAG("ZSRT"), ZSORT_OVERLAY);
    SetPubVar(buttons, "manager", Self());
    SetPubVar(buttons, "focus", focus_glowfly);
    SetPubVar(buttons, "padhelp", PADHELP_CIRCLE | PADHELP_CROSS | PADHELP_TRIANGLE);
    SetPubVar(buttons, "disable_selected", 0);
    SetPubVar(buttons, "se_decide", -1);
    return buttons;
}

/* Creates the album: a general_menu of count items on the curve of get_node_x/y/z, item select in front. */
create_album(count, select)
{
    new album;
    album = CreateNull(Self());
    new Float:x;
    new Float:y;
    new Float:z;
    map_screen_point(x, y, z, 0.7625 * g_screen_w, 0.5 * g_screen_h, Self());
    SetPos(album, x, y, z, 0);
    AttachScript(album, "general_menu", "general_menu");
    SetPubVar(album, "manager", Self());
    SetPubVar(album, "focus", focus_glowfly);
    SetPubVar(album, "pad_enable", PAD_CROSS);
    SendMessage(album, TAG("INUM"), count, 0, 0, 0);
    SendMessage(album, TAG("CLRW"), 1, 1, 0, 0);
    SendMessage(album, TAG("PADL"), 0, 0, 1, -1);
    SendMessage(album, TAG("MGIN"), 1, 7, 0, 0);
    SendMessage(album, TAG("MODE"), MENU_TYPE_PAGE, SCROLL_ALONG_CURVE, 0, 0);
    SendMessage(album, TAG("ANIM"), 250.0, 0, ANIM_FUNC_EXPONENT, 0);
    SendMessage(album, TAG("REPT"), 50.01, 83.34, 0, 0);
    SendMessage(album, TAG("SLCT"), select, 0, 0, 0);
    for (new node; node < 9; node++)
    {
        SendMessage(album, TAG("NPOS"), node, get_node_x(node - 1), get_node_y(node - 1), get_node_z(node - 1));
    }
    if (0 == count)
    {
        SetSystemParam(SYS_PADHELP, PADHELP_CROSS);
    }
    return album;
}

/* Replaces album with the one of mode, item select selected; returns its number of items, negative on an error. */
load_album(&album, mode, select)
{
    SetSystemParam(SYS_PADHELP, 0);
    show_caption2(_, _, 0.0, 400.0, 0.0);
    if (g_active == album)
    {
        unfocus_box();
    }
    fade_box(0.0, 1000.0, 1.0, 1.0, 1.0);
    if (album)
    {
        SendMessage(album, TAG("FADE"), 0, 1000.0, 0, 0);
    }
    if (album)
    {
        SendMessage(album, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
    }
    if (ALBUM_DISC != g_album_mode)
    {
        AnimTimerManager(Self(), 1000.0, 0.0, ANIM_FUNC_LINEAR);
        wait_ms(200);
        new text[64];
        get_tag_attr("TEXT", "msgmovie_hdd_reading", "value", text, 64);
        show_caption2(text, _, 1.0, 400.0, 0.0);
        wait_ms(400.0);
    }
    new count = query_album(mode);
    while (ANIM_DONE != AnimGetState(Self(), ANIM_TIMER))
    {
        sleep;
    }
    if (album)
    {
        Destroy(album);
    }
    album = 0;
    if (count > 0)
    {
        if (select >= count)
        {
            select = count - 1;
        }
        album = create_album(count, select);
        show_caption2(_, _, 0.0, 400.0, 0.0);
    }
    else if (0 == count)
    {
        show_caption2(_, _, 0.0, 400.0, 0.0);
        wait_ms(200);
        show_album_error(count);
    }
    else
    {
        show_caption2(_, _, 0.0, 400.0, 0.0);
        wait_ms(200);
    }
    return count;
}

/* Creates the album item of album index: the album icon, or the default icon for the DVD. */
create_album_item(path, index, count)
{
    new item = CreatePlane(path);
    AttachScript(item, "album_item", "album_item");
    if (ALBUM_COLLECTION == g_album_mode)
    {
        SetPubVar(item, "pad_enable", PAD_CIRCLE | PAD_TRIANGLE);
    }
    else if (g_album_mode == ALBUM_FAVORITE)
    {
        SetPubVar(item, "pad_enable", PAD_CIRCLE | PAD_TRIANGLE);
    }
    else
    {
        SetPubVar(item, "pad_enable", PAD_CIRCLE);
    }
    SetOptionI(item, TAG("FDEG"), 1);
    SetColor(item, 1.0, 1.0, 1.0, 0.0);
    if (index >= 0 && index < count)
    {
        if (ALBUM_COLLECTION == g_album_mode)
        {
            SendMessage(item, TAG("TEX"), g_album_icon, 1.0, 0, 0);
        }
        else if (g_album_mode == ALBUM_FAVORITE)
        {
            SendMessage(item, TAG("TEX"), g_album_icon, 1.0, 0, 0);
        }
        else
        {
            SendMessage(item, TAG("TEX"), g_default_icon, 1.0, 0, 0);
        }
    }
    return item;
}

/* Shows why the album is empty (count 0) or could not be read. */
show_album_error(count)
{
    new line1[32] = {0};
    new line2[32] = {0};
    if (ALBUM_COLLECTION == g_album_mode || g_album_mode == ALBUM_FAVORITE)
    {
        if (0 == count)
        {
            get_tag_attr("TEXT", "msgmovie_hdd_none", "value", line1, 32);
        }
        else if (count < 0)
        {
            get_tag_attr("TEXT", "msgmovie_hdd_none", "value", line1, 32);
        }
    }
    else if (CDVD_NO_DISC == g_disc_type)
    {
        get_tag_attr("TEXT", "msgmovie_dvd_none", "value", line1, 32);
    }
    else if (g_disc_type == CDVD_DETECTING)
    {
        get_tag_attr("TEXT", "msgmovie_dvd_reading", "value", line1, 32);
    }
    else
    {
        get_tag_attr("TEXT", "msgmovie_dvd_notplay", "value", line1, 32);
    }
    set_caption_pos2(0.546875 * g_screen_w, 0.5 * g_screen_h);
    if (strlen(line2) > 0)
    {
        show_caption2(line1, line2, 1.0, 400.0, 0.0);
    }
    else
    {
        show_caption2(line1, _, 1.0, 400.0, 0.0);
    }
    return 1;
}

/*
 * Lists the albums of mode in the sort order, or checks for a DVD video; returns their number, -1 when the cross
 * cancels the reading.
 */
query_album(mode)
{
    switch (mode)
    {
        case ALBUM_COLLECTION:
        {
            if (0 != check_cancel())
            {
                return -1;
            }
            new count;
            count = datOpen(DAT_DATABASE, g_db_name, DAT_ORDER_MTIME_DESC);
            if (0 != count)
            {
                return 0;
            }
            count = wait_database();
            if (count < 0)
            {
                return 0;
            }
            if (SORT_NEWEST == g_sort_order)
            {
                g_sql2 = "select * from sce_movie where content_attr = 128 order by create_date desc";
            }
            else
            {
                g_sql2 = "select * from sce_movie where content_attr = 128 order by create_date asc";
            }
            count = datSQLExec(g_sql2);
            if (0 != count)
            {
                return 0;
            }
            count = wait_database();
            if (count < 0)
            {
                return 0;
            }
            if (0 != check_cancel())
            {
                return -1;
            }
            count = datGetNum();
            if (count < 0)
            {
                count = 0;
            }
            return count;
        }
        case ALBUM_FAVORITE:
        {
            if (0 != check_cancel())
            {
                return -1;
            }
            new count;
            count = datOpen(DAT_DATABASE, g_db_name, DAT_ORDER_MTIME_DESC);
            if (0 != count)
            {
                return 0;
            }
            count = wait_database();
            if (count < 0)
            {
                return 0;
            }
            if (SORT_NEWEST == g_sort_order)
            {
                g_sql2 = "select * from sce_movie where content_attr = 129 order by create_date desc";
            }
            else
            {
                g_sql2 = "select * from sce_movie where content_attr = 129 order by create_date asc";
            }
            count = datSQLExec(g_sql2);
            if (0 != count)
            {
                return 0;
            }
            count = wait_database();
            if (count < 0)
            {
                return 0;
            }
            if (0 != check_cancel())
            {
                return -1;
            }
            count = datGetNum();
            if (count < 0)
            {
                count = 0;
            }
            return count;
        }
        case ALBUM_DISC:
        {
            g_disc_type = get_disc_type();
            if (g_disc_type == CDVD_DVD_VIDEO || g_disc_type == CDVD_DVD_VR)
            {
                return 1;
            }
            return 0;
        }
    }
    return 0;
}

/* Lights the radio buttons: blue and opaque. */
light_buttons()
{
    if (!g_mode_buttons)
    {
        return;
    }
    SendMessage(g_mode_buttons, TAG("FCOL"), 0.215, 0.708, 0.999, 0);
    SendMessage(g_mode_buttons, TAG("FADE"), 1.0, 400.0, 0, 0);
}

/* Dims the radio buttons: grey and half transparent. */
dim_buttons()
{
    if (!g_mode_buttons)
    {
        return;
    }
    SendMessage(g_mode_buttons, TAG("FCOL"), 0.7843, 0.7843, 0.7843, 0);
    SendMessage(g_mode_buttons, TAG("FADE"), 0.5, 400.0, 0, 0);
}

/* Lights the album and the box. */
light_album()
{
    fade_box(1.0, 400.0, 1.0, 1.0, 1.0);
    fade_box_header(1.0, 400.0, 0.215, 0.708, 0.999);
    if (!g_album)
    {
        return;
    }
    SendMessage(g_album, TAG("FADE"), 1.0, 400.0, 0, 0);
}

/* Dims the album and the box to a quarter of their alpha. */
dim_album()
{
    fade_box(0.25, 400.0, 1.0, 1.0, 1.0);
    fade_box_header(1.0, 400.0, 1.0, 1.0, 1.0);
    if (!g_album)
    {
        return;
    }
    SendMessage(g_album, TAG("FADE"), 0.25, 400.0, 0, 0);
}

/* Moves the camera to the left (dir 0) or to the right for a menu. */
slide_camera(dir)
{
    if (0 == dir)
    {
        AnimMoveHermite(Camera(0), -280.0, 0.0, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
        AnimMoveHermite(camera_focus, -280.0, 0.0, -100.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
    }
    else
    {
        AnimMoveHermite(Camera(0), 300.0, 0.0, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
        AnimMoveHermite(camera_focus, 300.0, 0.0, -100.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
    }
}

/* Moves the camera back to the center. */
center_camera()
{
    AnimMoveHermite(Camera(0), 0.0, 0.0, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
    AnimMoveHermite(camera_focus, 0.0, 0.0, -100.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
}

/* Points the camera at a new camera_focus and attaches camera_main, which keeps it there. */
create_camera()
{
    new camera = Camera(0);
    camera_focus = CreateNull(Self());
    camera_uptgt = CreateNull(Self());
    SetPos(camera_focus, 0.0, 0.0, -100.0, 0);
    SetPos(camera_uptgt, 0.0, 0.0 + 1000.0, 0.0, 0);
    SetPos(camera, 0.0, 0.0, 0.0, 0);
    LookAt(camera, camera_focus, 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
    AttachScript(Camera(0), "camera_main");
    SetPubVar(Camera(0), "camera_focus", camera_focus);
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

/* Returns the disc type, CDVD_DETECTING_* as CDVD_DETECTING and all but the DVD video types as CDVD_ILLEGAL. */
get_disc_type()
{
    new type = GetSystemParam(SYS_CDVD_TYPE);
    switch (type)
    {
        case CDVD_NO_DISC:
        {
            return type;
        }
        case CDVD_DETECTING:
        {
            return type;
        }
        case CDVD_DETECTING_CD:
        {
            return CDVD_DETECTING;
        }
        case CDVD_DETECTING_DVD:
        {
            return CDVD_DETECTING;
        }
        case CDVD_DETECTING_DVD_DL:
        {
            return CDVD_DETECTING;
        }
        case CDVD_DVD_VIDEO:
        {
            return type;
        }
        case CDVD_DVD_VR:
        {
            return type;
        }
    }
    return CDVD_ILLEGAL;
}

/* Runs on a child of the page (argument "owner"): sends "DISC" (type) to it when the disc type changes. */
public media_watcher_main()
{
    new owner;
    new arg[16];
    if (GetArg("owner", arg, 16))
    {
        owner = atoi(arg);
    }
    new old_type = get_disc_type();
    for (; ; )
    {
        new type = get_disc_type();
        if (type != old_type)
        {
            SendMessage(owner, TAG("DISC"), type, 0, 0, 0);
            old_type = type;
        }
        wait_ms(500);
    }
    return;
}

/* Decodes the image of IMG id and keeps its texture; returns it, 0 on failure. */
load_icon_texture(id[])
{
    get_tag_attr("IMG", id, "src", g_uri, 256);
    start_image_decode2(g_uri);
    if (IMG_DONE != finish_image_decode())
    {
        return 0;
    }
    new texture = imgTexID();
    imgKeepTex(texture);
    return texture;
}

/* Returns the x of node of the album curve, the selected item being node 0: a parabola, -80 * (node / 2)^2. */
Float:get_node_x(node)
{
    new Float:t = 0.5 * float(node);
    return 80.0 * (-t * t);
}

/* Returns the y of node of the album curve: 0. */
Float:get_node_y(node)
{
    new n = node;
    return 0.0 * n;
}

/* Returns the z of node of the album curve: -400 per node, -380 per node before the selected one. */
Float:get_node_z(node)
{
    new Float:n = float(node);
    if (n < 0)
    {
        return 380.0 * -n;
    }
    return 400.0 * -n;
}

/* Returns the result of a push of item index in an option menu: circle chooses it, cross goes back. */
handle_menu_push(manager, side, &mode, index, button, title[], group[], id[])
{
    new unused; /* takes the parameters the function does not use */
    new buffer[64];
    unused = manager;
    unused = side;
    unused = mode;
    unused = index;
    unused = 64;
    copy_string(buffer, group, unused);
    copy_string(buffer, title, unused);
    copy_string(buffer, id, unused);
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
    new unused; /* takes the parameters the function does not use */
    unused = side;
    unused = result;
    unused = mode;
    unused = selected;
    unused = 64;
    copy_string(g_msg, title, unused);
    copy_string(g_msg, group, unused);
    copy_string(g_msg, id, unused);
    return;
}

/* Leaves the label of an option menu item as it is. */
fill_menu_item(side, mode, index, group[], label[])
{
    new unused; /* takes the parameters the function does not use */
    new buffer[64];
    unused = side;
    unused = mode;
    unused = index;
    unused = 64;
    copy_string(buffer, group, unused);
    copy_string(buffer, label, unused);
}

/* Shows msg in a dialog and waits until it is closed; ok is not used. */
show_message(msg[], ok[])
{
    sprintf(g_script_args, "msg=%s;ok=%s", msg, ok);
    sprintf(g_script_args, "msg=%s", msg);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    AttachScript(dialog, _, "confirm_d", g_script_args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus_glowfly);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    Destroy(dialog);
    return;
}

/* Shows msg for life ms. */
show_moment(msg[], life)
{
    sprintf(g_script_args, "msg=%s", msg);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    AttachScript(dialog, _, "confirm_moment", g_script_args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus_glowfly);
    SetPubVar(dialog, "life", life);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
}

/* Reboots into the DVD video in the drive, after a warning; returns if there is none. */
boot_dvd_video()
{
    new keys[2][16];
    new encoded[2][64];
    new len;
    new data[256];
    new i;
    g_disc_type = get_disc_type();
    if (CDVD_DVD_VIDEO != g_disc_type)
    {
        if (CDVD_DVD_VR != g_disc_type)
        {
            return;
        }
    }
    SetSystemParam(SYS_PADHELP, 0);
    get_tag_attr("TEXT", "msgmovie_warn_wait", "value", g_line1, 32);
    get_tag_attr("TEXT", "msgmovie_warn_wait2", "value", g_line2, 32);
    get_tag_attr("TEXT", "msgmovie_warn_console_power", "value", g_line3, 32);
    get_tag_attr("TEXT", "msgmovie_warn_console_disctray", "value", g_line4, 32);
    sprintf(g_msg, "%s\n%s\n\n%s\n%s", g_line1, g_line2, g_line3, g_line4);
    show_moment(g_msg, 5000);
    err10019(keys[0]);
    encode_base64_2(keys[0], 16, encoded[0], 64);
    err10020();
    new count = 2;
    keys[1] = "BootDvdVideo";
    i = 1;
    for (; i < count; i++)
    {
        len = err10021(keys[i], data);
        encode_base64_2(data, len, encoded[i], 64);
        apply_rot13_2(encoded[i]);
    }
    err10022(0, encoded[0]);
    err10022(1, encoded[1]);
    save_album_state();
    write_current_channel("Movie");
    scnfwriteDVDProgBit();
    err10023(count);
    return;
}

/* Returns -1, sending itself a cross "PUSH", when the cross is pressed. */
check_cancel()
{
    new held;
    new pressed;
    PadState(held, pressed, _, _, -1);
    if (pressed & PAD_CROSS)
    {
        SendMessage(Self(), TAG("PUSH"), 0, TAG("RDWN"), 0, 0);
        return -1;
    }
    return 0;
}

/* Records the album and the selected album in the aval file for the next visit. */
save_album_state()
{
    new value[4];
    sprintf(value, "%d", g_album_mode);
    avalSet("MovieAlbumMode", value);
    sprintf(value, "%d", g_album_select);
    avalSet("MovieAlbumCurNo", value);
}
