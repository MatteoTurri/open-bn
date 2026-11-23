/*
 * /opt0/bn/script/music/music.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The music page: radio buttons for the albums (the favorite albums, or the hard disk, the audio CD and the portable
 * device) beside a curve of the albums of the one chosen. The circle opens an album in the music player page, the
 * triangle opens the option menu (check-in and check-out with the device, delete, information).
 */

#include <core>
#include <bn>

#pragma dynamic 1500

#include "musicdb.inc"

#include "devimage.inc"

#include "stdmenu.inc"
#include "genmenu.inc"
#include "scroll.inc"

/* Results of run_album_option besides the 0 or 1 of show_information. */
const OPTION_CLOSE = -1;
const OPTION_ALBUM_CHANGED = 2; /* and the menu closes */
const OPTION_RENAMED = 3;

/* The items of the option menu of an album, by album: optmenu0, optmenu1 (favorite), optmenu2 (CD), optmenu3 (PD). */
const ITEM_CHECKOUT = 0;
const ITEM_DELETE = 1;
const ITEM_INFORMATION = 2;
const ITEM_FAV_CHECKOUT = 0;
const ITEM_FAV_INFORMATION = 1;
const ITEM_CD_INFORMATION = 0;
const ITEM_PD_CHECKIN = 0;
const ITEM_PD_DELETE = 1;
const ITEM_PD_INFORMATION = 2;

/* The radio button of the portable device, the third when the page starts on the collection. */
const BUTTON_PD = 2;

#include "player.inc"

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

/* Appends src to dest (maxlen cells), packed or unpacked like dest. */
append_string(dest[], src[], maxlen)
{
    new dest_len;
    new src_len;
    new i;
    new room;
    dest_len = strlen(dest);
    src_len = strlen(src);
    if (is_packed(dest))
    {
        room = maxlen * 4 - dest_len - 1;
        if (is_packed(src))
        {
            i = 0;
            for (; i < src_len && i < room; i++)
            {
                dest{dest_len + i} = src{i};
            }
        }
        else
        {
            i = 0;
            for (; i < src_len && i < room; i++)
            {
                dest{dest_len + i} = src[i];
            }
        }
        new pad = 4 - (dest_len + i) % 4;
        for (new j; j < pad; j++)
        {
            dest{dest_len + i + j} = 0;
        }
    }
    else
    {
        room = maxlen - dest_len - 1;
        if (is_packed(src))
        {
            i = 0;
            for (; i < src_len && i < room; i++)
            {
                dest[dest_len + i] = src{i};
            }
        }
        else
        {
            i = 0;
            for (; i < src_len && i < room; i++)
            {
                dest[dest_len + i] = src[i];
            }
        }
        dest[dest_len + i] = 0;
    }
    return;
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

#include "imgdec.inc"

#include "misc.inc"

new g_db_name[] = "music";
new g_shadow_colors[4] = {0x5F000000, 0x5F000000, 0x5F000000, 0x5F000000}; /* the "SCL4" of the shadowed labels */

#include "string.inc"

#include "dom.inc"

#include "disc.inc"

#include "layout.inc"

new g_common_bg_name[] = "SCEI:CommonBG";

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

new g_focus_box; /* the box beside the album with the name and the number of the selected album */

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

/* The option menu of an album, and the image of its device, by MUSIC_ALBUM_*. */
new g_option_groups[4][3] = {"optmenu1", "optmenu0", "optmenu2", "optmenu3"};
new g_album_device[4] = {DEVICE_HDD, DEVICE_HDD, DEVICE_DISC, DEVICE_USB};

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

/* Opens the information page ("LINK" link_id) as a modal window, with its editmode and media_watcher arguments. */
open_info_page(link_id[], edit_mode, watch)
{
    new url[256];
    new args[16];
    sprintf(args, "editmode=%d;media_watcher=%d", edit_mode, watch);
    get_tag_attr("LINK", link_id, "href", url, 256);
    xmlLoadStart(url, _, args, XMLLOAD_MODAL);
    wait_xml_load();
}

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

/* Sets the variable name to the number value if it is not set yet. */
init_aval_int(name[], value)
{
    new text[16];
    if (0 != avalGetValue(name, 0, text, 16))
    {
        set_aval_int(name, value);
    }
}

#include "playerctl.inc"

public idx = -1; /* the player, from reset_player; below 0 the watchers are not started */
new Float:g_screen_w;
new Float:g_screen_h;
public camera_focus; /* the point the camera looks at */
public camera_uptgt; /* unused, but part of the layout */
public focus_glowfly; /* the focus object */
new g_active; /* g_mode_buttons or g_album: the one that takes the pad */
new g_mode_buttons;
new g_album;
new g_default_icon;
new g_next_link[256]; /* the page to leave for: a "LINK" id, "top" or "player" */
new g_next_args[256];
new g_quit;
new g_disc_type; /* CDVD_* */
new g_pd_event; /* unused, but part of the layout */
new g_start_mode; /* the page: MUSIC_ALBUM_FAVORITE for the favorite albums, MUSIC_ALBUM_COLLECTION for the others */
new g_album_title[256];
new g_album_mode; /* MUSIC_ALBUM_* */
new g_album_select;
new g_album_count;
new g_album_idle; /* the album has stopped moving */
new g_enter_album; /* the focus goes into the album once it is idle */
new g_content_path[256]; /* the folder of the selected album of the hard disk */
new g_album_tracks[16];
public music_quality; /* the recording quality of the player */
new g_pd_tracks = -2; /* the tracks on the portable device, below 0 if there is none */
new g_cd_items; /* the albums of the list before the rows of the database: the audio CD, */
new g_pd_items; /* and the portable device */
new g_db_open;
new g_pd_result; /* what query_album returned for the portable device */
new g_album_no; /* MusicAlbumNo of the last visit: the album to select, or ALBUM_NO_* */
new g_idle_since = -1; /* the second the pad became idle on the button of the portable device */
new g_change_done = -1; /* set to 1 by the player when it has taken the change of album */

/* Copies src into dest doubling each single quote, for a quoted SQL string. */
escape_sql(dest[], src[])
{
    new len = strlen(src);
    new out;
    for (new i; i < len; i++)
    {
        if (src{i} == '\'')
        {
            dest{out++} = '\'';
            dest{out++} = '\'';
        }
        else if (src{i} == '\\')
        {
            dest{out++} = '\\';
        }
        else
        {
            dest{out++} = src{i};
        }
    }
    dest{out} = 0;
}

/* Builds the page, runs it until it leaves, and loads the next page; the player keeps playing for "player". */
public main()
{
    g_next_link = "top";
    g_next_args = "";
    init_page();
    while (1 != g_quit)
    {
        handle_messages();
        sleep;
    }
    close_page();
    if (0 != strcmp(g_next_link, "player"))
    {
        stop_player();
    }
    else if (MUSIC_ALBUM_PD != g_album_mode)
    {
        set_aval_int("MusicCheckoutCancel", 1);
    }
    fade_background(0, 0, 600.0);
    load_page(g_next_link, g_next_args);
}

/*
 * Builds the page on the albums of the argument mode ("favorite" or "collection"), or on the album of the last visit
 * with the focus in it, and starts the watchers of the disc and of the portable device.
 */
init_page()
{
    get_screen_size3(g_screen_w, g_screen_h);
    start_top_page(0);
    load_common_bg("bg", 1000.0);
    set_background_poor(600.0, 0);
    create_camera();
    link_bg_camera(2.0, 0);
    reset_bg_camera();
    focus_glowfly = create_focus_plane2("focus");
    g_quit = 0;
    g_disc_type = CDVD_NO_DISC;
    g_pd_event = -1;
    g_album_title = "";
    g_album_select = 0;
    g_album_idle = 0;
    set_aval_int("MediaWatcherFlag", 1);
    set_aval_int("MusicMediaChanged", 0);
    set_aval_int("MusicChannelMode", CHANNEL_ALBUM);
    init_aval_int("MusicAlbumMode", MUSIC_ALBUM_COLLECTION);
    init_aval_int("MusicPrefNewDisc", 0);
    new from_arg;
    {
        new mode_arg[64];
        GetArg("mode", mode_arg, 64);
        if (!strcmp("favorite", mode_arg))
        {
            g_album_mode = MUSIC_ALBUM_FAVORITE;
            g_start_mode = MUSIC_ALBUM_FAVORITE;
            from_arg = 1;
        }
        else if (!strcmp("collection", mode_arg))
        {
            g_album_mode = MUSIC_ALBUM_COLLECTION;
            g_start_mode = MUSIC_ALBUM_COLLECTION;
            from_arg = 1;
        }
        else
        {
            g_album_mode = get_aval_int("MusicAlbumMode");
            g_start_mode = g_album_mode;
            g_album_no = get_aval_int("MusicAlbumNo");
            if (g_album_no == ALBUM_NO_CD)
            {
                g_album_mode = MUSIC_ALBUM_CD;
            }
            else if (g_album_no == ALBUM_NO_PD)
            {
                g_album_mode = MUSIC_ALBUM_PD;
            }
            from_arg = 0;
        }
        printf("============ current_album_no = %d\n", g_album_no);
    }
    g_default_icon = load_icon_texture("default_icon");
    create_device_image();
    show_device_image(g_album_device[g_album_mode]);
    create_focus_box(0.7625 * g_screen_w, 0.5 * g_screen_h);
    fade_box(0.0, 0.0, 1.0, 1.0, 1.0);
    set_box_focus(focus_glowfly);
    set_box_title("");
    create_captions(0);
    set_caption_pos2(0.546875 * g_screen_w, 0.5 * g_screen_h);
    g_mode_buttons = create_mode_buttons();
    /* The radio button of the CD or of the portable device: -g_album_no. */
    if (g_album_no < 0)
    {
        SetPubVar(g_mode_buttons, "select", -g_album_no);
        SetPubVar(g_mode_buttons, "cursor", -g_album_no);
        if (-g_album_no + 1 == MUSIC_ALBUM_PD)
        {
            set_aval_int("MediaWatcherFlag", 0);
        }
    }
    else
    {
        SetPubVar(g_mode_buttons, "select", 0);
        SetPubVar(g_mode_buttons, "cursor", 0);
    }
    g_active = g_mode_buttons;
    g_album = 0;
    new enter;
    if (from_arg)
    {
        enter = 0;
    }
    else
    {
        enter = 1;
    }
    /* Another mode than the one to show, so that change_album loads it. */
    new mode = g_album_mode;
    g_album_mode--; /* differs from the mode, so that change_album reloads the album */
    change_album(mode, 0, enter);
    if (idx >= 0)
    {
        new args[256];
        sprintf(args, "owner = %d", Self());
        new media_watcher = CreateNull(Self());
        AttachScript(media_watcher, "media_watcher_main", _, args);
        SetPubVar(media_watcher, "idx", idx);
        new pd_watcher = CreateNull(Self());
        sprintf(args, "owner = %d; initial = %d", Self(), get_aval_int("PDTracks"));
        set_aval_int("MusicCheckoutCancel", 0);
        AttachScript(pd_watcher, "pd_watcher_main", _, args);
        SetPubVar(pd_watcher, "idx", idx);
    }
    else
    {
        printf("===================== NOT INITIALIZED\n");
    }
    new repeat_mode;
    new shuffle_mode;
    playerExecCommand(PLAYER_GET_PREFS, music_quality, repeat_mode, shuffle_mode);
    set_aval_int("MusicPrefQuality", music_quality);
    set_aval_int("MusicRepeatMode", repeat_mode);
    set_aval_int("MusicShuffleMode", shuffle_mode);
}

/* Takes the focus away, closes the database and keeps the album mode in the aval file. */
close_page()
{
    if (g_mode_buttons)
    {
        SendMessage(g_mode_buttons, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
    }
    if (g_album)
    {
        SendMessage(g_album, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
    }
    datClose();
    set_aval_int("MusicAlbumMode", g_album_mode);
}

new g_init_status = -2; /* the initialization of the player: below 0 until it is known, 0 on an error */

/*
 * Handles the messages of one frame: "CHNG" (button) and "FOCS", "PUSH" (_, button) of the radio buttons or of the
 * album, whichever takes the pad; "ITEM" (menu, path, index) of the album; "DISC" (type) of the disc watcher and
 * "PDEV" (tracks) of the device watcher. The device watcher runs while the pad is idle on the button of the portable
 * device (MediaWatcherFlag 0), for half an hour at most.
 */
handle_messages()
{
    if (g_init_status < 0)
    {
        playerExecCommand(PLAYER_GET_INIT, g_init_status);
        if (0 == g_init_status)
        {
            printf("INIT ERR\n");
            show_unit_warning();
            go_to_top();
            return 0;
        }
    }
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
                change_album(a + g_start_mode, 1, 1);
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
        if (msg == TAG("PDEV"))
        {
            on_pd_change(a);
        }
    }
    update_album_info();
    update_buttons_pad_help();
    new cursor = GetPubVar(g_mode_buttons, "cursor");
    if (cursor == BUTTON_PD)
    {
        new pressed;
        new released;
        new held;
        PadState(held, pressed, released, _, -1);
        if (0 == pressed && 0 == released)
        {
            if (g_idle_since > 0)
            {
                new elapsed = tickcount() / 1000;
                elapsed = elapsed - g_idle_since;
                if (elapsed > 1800 || elapsed < 0)
                {
                    set_aval_int("MediaWatcherFlag", 1);
                }
            }
            else
            {
                set_aval_int("MediaWatcherFlag", 0);
                g_idle_since = tickcount() / 1000;
            }
        }
        else
        {
            set_aval_int("MediaWatcherFlag", 0);
            g_idle_since = -1;
        }
    }
    else if (get_aval_int("MediaWatcherFlag"))
    {
        new pd_tracks;
        new prev_tracks;
        pd_tracks = get_aval_int("PDTracks");
        prev_tracks = get_aval_int("PrevTracks");
        if (g_album_mode == MUSIC_ALBUM_PD)
        {
            if (prev_tracks != pd_tracks || prev_tracks < 0)
            {
                set_aval_int("MediaWatcherFlag", 0);
            }
        }
        else
        {
            set_aval_int("MediaWatcherFlag", 1);
        }
        g_idle_since = -1;
    }
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
        go_to_top();
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
    }
}

/* Reloads the album of the portable device when its tracks change; the album mode is changed to force the reload. */
on_pd_change(tracks)
{
    g_pd_event = tracks;
    if (MUSIC_ALBUM_FAVORITE == g_start_mode)
    {
        return;
    }
    if (MUSIC_ALBUM_PD != g_album_mode)
    {
        return;
    }
    if (g_active == g_album)
    {
        go_back();
    }
    g_album_mode = MUSIC_ALBUM_COLLECTION;
    if (tracks < 0)
    {
        change_album(MUSIC_ALBUM_PD, 0, 1);
    }
    else
    {
        change_album(MUSIC_ALBUM_PD, 1, 1);
    }
}

/* Reloads the album of the CD when the disc changes, leaving it if there is no disc any more. */
on_disc_change(type)
{
    printf("======= on_any_disc\n");
    g_disc_type = type;
    if (MUSIC_ALBUM_FAVORITE == g_start_mode)
    {
        return;
    }
    if (MUSIC_ALBUM_CD != g_album_mode)
    {
        return;
    }
    if (g_disc_type <= CDVD_DETECTING)
    {
        if (g_active == g_album)
        {
            go_back();
        }
        g_album_mode = MUSIC_ALBUM_COLLECTION;
        change_album(MUSIC_ALBUM_CD, 0, 0);
    }
    else
    {
        g_album_mode = MUSIC_ALBUM_COLLECTION;
        change_album(MUSIC_ALBUM_CD, 1, 1);
    }
}

/* Shows album mode, loading it if it is another one; with enter the focus goes into it. */
change_album(mode, with_se, enter)
{
    g_change_done = mode == MUSIC_ALBUM_COLLECTION && 0 == with_se && 0 == enter ? 0 : -1;
    playerExecCommand(PLAYER_HOLD_COMPRESSION, g_change_done);
    while (1 != g_change_done)
    {
        sleep;
    }
    printf("====== on_menu_change %d,%d,%d  albummode=%d \n", mode, with_se, enter, g_album_mode);
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
        if ((MUSIC_ALBUM_FAVORITE == g_album_mode || g_album_mode == MUSIC_ALBUM_COLLECTION) && g_album_no >= 0)
        {
            g_album_count = load_album(g_album, g_album_mode, g_album_no);
            g_album_no = -1;
        }
        else
        {
            g_album_count = load_album(g_album, g_album_mode, 0);
        }
        SendMessage(g_mode_buttons, TAG("DSEL"), g_album_count <= 0, 0, 0, 0);
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
    /* -8: the portable device is being connected. */
    if (g_album_count <= 0 && -8 != g_album_count)
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
    printf("=== mode = %d album_mode = %d, after_idle = %d\n", mode, g_album_mode, g_enter_album);
    SetPubVar(g_mode_buttons, "disable_selected", 0);
}

/* The focus stays on g_active whichever way (direction, a TAG) the pad leaves it. */
on_buttons_focus(direction)
{
    printf("==== on_menu_focus = %s\n", direction);
    SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
}

/*
 * Circle on a radio button goes into its album; triangle on the button of the portable device opens its option
 * menu.
 */
on_buttons_push(button)
{
    printf("============ on_menu_push\n");
    new cursor = GetPubVar(g_mode_buttons, "cursor");
    if (button == TAG("RRGT"))
    {
        printf("---------- SELECT = %d\n", cursor);
        g_active = g_album;
        change_album(cursor + g_start_mode, 1, 1);
    }
    else if (button == TAG("RUP") && cursor == BUTTON_PD)
    {
        sndSePlay(SE_OPEN);
        sndSePlay(SE_WINDOW_OPEN);
        SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
        SendMessage(g_active, TAG("PUSH"), TAG("RUP"), 0, 0, 0);
        SendMessage(g_active, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
        send_focus2(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        if (g_active == g_album)
        {
            unfocus_box();
        }
        fade_menus(0.0, 400.0);
        slide_camera(1);
        wait_ms(floatround(750.0, 0));
        new result = MENU_ITEM;
        new unused; /* unused, but part of the layout */
        new unused2; /* unused, but part of the layout */
        while (result == MENU_ITEM)
        {
            result = pop_std_menu("optmenu", "optmenu4", MENU_SIDE_RIGHT, 0);
            printf("status = %d\n", result - MENU_ITEM);
        }
        sndSePlay(SE_WINDOW_CLOSE);
        center_camera();
        wait_ms(floatround(750.0, 0));
        fade_menus(1.0, 400.0);
        if (g_album_count > 0)
        {
            dim_album();
        }
        else if (g_album_mode == MUSIC_ALBUM_PD)
        {
            show_album_error(g_pd_result);
        }
        else
        {
            show_album_error(g_album_count);
        }
        light_buttons();
        SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
    }
    else
    {
        SendMessage(g_active, TAG("PUSH"), 0, 0, 0, 0);
    }
}

/* Sets the pad help of the radio buttons, triangle also on the button of the portable device. */
update_buttons_pad_help()
{
    if (g_enter_album == 1)
    {
        return;
    }
    new cursor = GetPubVar(g_mode_buttons, "cursor");
    if (g_active == g_mode_buttons)
    {
        if (cursor == BUTTON_PD)
        {
            SetPubVar(g_mode_buttons, "padhelp", PADHELP_CIRCLE | PADHELP_CROSS | PADHELP_TRIANGLE);
            set_aval_int("MediaWatcherFlag", 0);
        }
        else
        {
            set_aval_int("MediaWatcherFlag", 1);
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
        SendMessage(menu, TAG("ITEM"), item, path, 0, 0);
    }
    else
    {
        printf("invalid parent %d\n", path);
    }
}

/* The focus stays on g_active whichever way (direction, a TAG) the pad leaves it. */
on_album_focus(direction)
{
    printf("===== on_list_focus %s\n", direction);
    SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
}

/*
 * Circle on the album opens the selected album in the music player page; triangle opens the option menu of the
 * album and reloads it if an option changed it.
 */
on_album_push(button)
{
    printf("==== on_list_push \n");
    if (button == TAG("RRGT"))
    {
        sndSePlay(SE_DECIDE);
        SendMessage(g_active, TAG("PUSH"), 0, 0, 0, 0);
        if (g_album_idle)
        {
            set_aval_int("MusicAlbumMode", g_start_mode);
            if (g_album_mode == MUSIC_ALBUM_PD)
            {
                set_aval_int("MusicAlbumNo", ALBUM_NO_PD);
            }
            else
            {
                set_aval_int("MusicAlbumNo", g_album_select - g_cd_items - g_pd_items);
            }
            avalSet("MusicAlbumName", g_album_title);
            if (g_album_mode == MUSIC_ALBUM_COLLECTION)
            {
                playerExecCommand(PLAYER_SET_ALBUM_PATH, g_content_path);
            }
            g_quit = 1;
            g_next_link = "player";
        }
        return;
    }
    new can_open = 1;
    if (g_album_mode == MUSIC_ALBUM_COLLECTION)
    {
        if (0 == strlen(g_content_path))
        {
            can_open = 0;
        }
    }
    if (button == TAG("RUP") && can_open && g_album_idle)
    {
        sndSePlay(SE_OPEN);
        sndSePlay(SE_WINDOW_OPEN);
        SendMessage(g_active, TAG("PUSH"), TAG("RUP"), 0, 0, 0);
        SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
        new redraw;
        avalSet("MusicAlbumName", g_album_title);
        if (g_album_mode == MUSIC_ALBUM_COLLECTION)
        {
            playerExecCommand(PLAYER_SET_ALBUM_PATH, g_content_path);
        }
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
        slide_camera(1);
        wait_ms(floatround(750.0, 0));
        new result;
        new action;
        new media_changed;
        set_aval_int("MusicMediaChanged", 0);
        /* action: OPTION_CLOSE ends the menu, 1 or more reloads the album (OPTION_RENAMED: after a rename). */
        while (OPTION_CLOSE != action)
        {
            result = pop_std_menu("optmenu", g_option_groups[g_album_mode], MENU_SIDE_RIGHT, result - MENU_ITEM);
            action = run_album_option(result);
            if (action >= 1)
            {
                redraw = action;
            }
            if (action == OPTION_ALBUM_CHANGED)
            {
                action = OPTION_CLOSE;
            }
            printf("redraw = %d\n", redraw);
            if (action == OPTION_RENAMED)
            {
                new album_name[128];
                avalGetValue("MusicAlbumName", 0, album_name, 128);
                set_box_header(0, g_album_title);
            }
            media_changed = get_aval_int("MusicMediaChanged");
            if (g_album_mode == MUSIC_ALBUM_CD && media_changed & 1)
            {
                break;
            }
        }
        sndSePlay(SE_WINDOW_CLOSE);
        center_camera();
        SetPos(g_focus_box, 0.7625 * g_screen_w - 300.0, 0.5 * g_screen_h, 0.0, 0);
        AnimMoveHermite(g_focus_box, 0.7625 * g_screen_w, 0.5 * g_screen_h, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
        wait_ms(floatround(750.0, 0));
        fade_box_footer(1.0, 400.0, 0.7843, 0.7843, 0.7843);
        SendMessage(g_mode_buttons, TAG("FADE"), 1.0, 400.0, 0, 0);
        media_changed = get_aval_int("MusicMediaChanged");
        if (media_changed)
        {
            playerExecCommand(PLAYER_CD_READ_TOC, 1);
            redraw = 1;
        }
        if (!redraw)
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
            new mode;
            if (redraw == OPTION_RENAMED)
            {
                query_album(g_album_mode);
                mode = g_album_mode;
                g_album_idle = 0;
            }
            else
            {
                go_back();
                mode = g_album_mode;
                g_album_mode--; /* differs from the mode, so that change_album reloads the album */
            }
            change_album(mode, 0, 1);
        }
        SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        return;
    }
    SendMessage(g_active, TAG("PUSH"), 0, 0, 0, 0);
}

/*
 * When the album stops, shows the name and the number of the selected album in the box and keeps its values in the
 * aval file for the information page: the length and the size of the audio CD, or the row of the album.
 */
update_album_info()
{
    if (g_album_count <= 0 || 0 == g_album)
    {
        return;
    }
    new was_idle = g_album_idle;
    new idle = GetPubVar(g_album, "idle");
    g_album_select = GetPubVar(g_album, "select");
    g_album_idle = idle == 1;
    if (g_album_idle != was_idle)
    {
        if (g_album_idle)
        {
            g_album_title = "    ";
            set_box_header(0, g_album_title);
            if (g_start_mode == MUSIC_ALBUM_COLLECTION)
            {
                if (g_album_mode == MUSIC_ALBUM_CD)
                {
                    get_tag_attr("TEXT", "msgmusic_ainfo_audiocd", "value", g_album_title, 256);
                    new tracks = -1;
                    new toc_status = -1;
                    new tries;
                    while (tracks == -1)
                    {
                        playerExecCommand(PLAYER_CD_GET_TOC, tracks, toc_status);
                        tries++;
                        if (tries >= 600)
                        {
                            break;
                        }
                        sleep;
                    }
                    new minutes;
                    new seconds;
                    new frames;
                    new length[128];
                    playerExecCommand(PLAYER_CD_GET_TOC_TIME, tracks, minutes, seconds, frames);
                    new size_kb;
                    /* 75 sectors of 2352 bytes a second. */
                    size_kb = ((minutes * 60 + seconds) * 75 + frames) * 2352;
                    size_kb = size_kb / 1024;
                    set_aval_int("DataSize", size_kb);
                    if (minutes >= 60)
                    {
                        new hours;
                        hours = minutes / 60;
                        minutes = minutes - hours * 60;
                        sprintf(length, "%02d:%02d:%02d", hours, minutes, seconds);
                    }
                    else
                    {
                        sprintf(length, "00:%02d:%02d", minutes, seconds);
                    }
                    avalSet("Length", length);
                    new artist[256];
                    get_tag_attr("TEXT", "msgmusic_ainfo_artist_default", "value", artist, 256);
                    avalSet("ArtistName", artist);
                }
                else if (g_album_mode == MUSIC_ALBUM_PD)
                {
                    get_tag_attr("TEXT", "msgmusic_menu_pd", "value", g_album_title, 256);
                }
                else
                {
                    datGetData(g_album_select - g_cd_items - g_pd_items, ALBUM_COL_NAME, g_album_title, 256);
                    datGetData(g_album_select - g_cd_items - g_pd_items, ALBUM_COL_URL, g_content_path, 256);
                    avalSet("ContentPath", g_content_path);
                    new path_len = strlen(g_content_path);
                    new pad_count = path_len % 4;
                    /* The path is padded with spaces, as many as its length modulo 4. */
                    for (new i; i < pad_count; i++)
                    {
                        append_string(g_content_path, " ", 256);
                    }
                    datGetData(g_album_select - g_cd_items - g_pd_items, ALBUM_COL_TRACKS, g_album_tracks, 16);
                    set_aval_int("AlbumTracks", atoi(g_album_tracks));
                    new value[256];
                    datGetData(g_album_select - g_cd_items - g_pd_items, ALBUM_COL_ARTIST, value, 256);
                    avalSet("ArtistName", value);
                    datGetData(g_album_select - g_cd_items - g_pd_items, ALBUM_COL_LENGTH, value, 256);
                    avalSet("Length", value);
                    datGetData(g_album_select - g_cd_items - g_pd_items, ALBUM_COL_SIZE, value, 256);
                    avalSet("DataSize", value);
                }
            }
            else if (MUSIC_ALBUM_FAVORITE == g_start_mode)
            {
                datGetData(g_album_select, ALBUM_COL_NAME, g_album_title, 256);
                playerExecCommand(PLAYER_SET_FAVORITE, g_album_select + 1);
            }
            else if (g_album_mode == MUSIC_ALBUM_CD)
            {
                get_tag_attr("TEXT", "msgmusic_ainfo_audiocd", "value", g_album_title, 256);
            }
            else
            {
                get_tag_attr("TEXT", "msgmusic_menu_pd", "value", g_album_title, 256);
            }
            set_box_header(0, g_album_title);
            new counter[256];
            sprintf(counter, "%d/%d", g_album_select + 1, g_album_count);
            set_box_footer(counter);
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

/*
 * Asks the player for the state of the portable device (mode is passed on) and keeps PDTracks and PDType in the aval
 * file; returns the number of tracks, -1 if there is no device, the check times out or MusicCheckoutCancel is set.
 */
check_pd(mode)
{
    if (idx < 0)
    {
        g_pd_tracks = -1;
        return -1;
    }
    playerExecCommand(PLAYER_PD_CHECK, mode);
    new status = -1;
    new tries;
    while (status < 0)
    {
        new cancel = get_aval_int("MusicCheckoutCancel");
        if (cancel)
        {
            playerExecCommand(PLAYER_CANCEL);
            return -1;
        }
        if (0 == tries % 6)
        {
            playerExecCommand(PLAYER_GET_STATUS, status);
        }
        tries++;
        if (tries >= 3000)
        {
            printf("======= TIMEOUT in pd_check()\n");
            break;
        }
        sleep;
    }
    new pd_type;
    if (tries < 3000)
    {
        playerExecCommand(PLAYER_PD_GET_INFO, g_pd_tracks, pd_type, 0);
        set_aval_int("PDTracks", g_pd_tracks);
        set_aval_int("PDType", pd_type);
        if (pd_type >= 0)
        {
            set_aval_int("PrevPDType", pd_type);
        }
    }
    else
    {
        g_pd_tracks = -1;
        set_aval_int("PDTracks", g_pd_tracks);
    }
    return g_pd_tracks;
}

/*
 * Runs the option result of the menu of an album, by page and album: check-in, check-out, delete, information;
 * returns an OPTION_* or the result of show_information.
 */
run_album_option(result)
{
    new action;
    result = result - MENU_ITEM;
    switch (g_start_mode)
    {
        case MUSIC_ALBUM_COLLECTION:
        {
            if (g_album_mode == MUSIC_ALBUM_CD)
            {
                switch (result)
                {
                    case ITEM_CD_INFORMATION:
                    {
                        action = show_information();
                    }
                    default:
                    {
                        action = OPTION_CLOSE;
                    }
                }
            }
            else if (g_album_mode == MUSIC_ALBUM_PD)
            {
                switch (result)
                {
                    case ITEM_PD_CHECKIN:
                    {
                        action = run_checkin();
                        if (action)
                        {
                            action = OPTION_ALBUM_CHANGED;
                        }
                        check_pd(2);
                    }
                    case ITEM_PD_DELETE:
                    {
                        action = run_delete();
                        if (action)
                        {
                            action = OPTION_ALBUM_CHANGED;
                        }
                    }
                    case ITEM_PD_INFORMATION:
                    {
                        action = show_information();
                    }
                    default:
                    {
                        action = OPTION_CLOSE;
                    }
                }
            }
            else
            {
                switch (result)
                {
                    case ITEM_CHECKOUT:
                    {
                        run_checkout();
                    }
                    case ITEM_DELETE:
                    {
                        action = run_delete();
                        if (action)
                        {
                            action = OPTION_ALBUM_CHANGED;
                        }
                    }
                    case ITEM_INFORMATION:
                    {
                        action = show_information();
                        if (action)
                        {
                            action = OPTION_RENAMED;
                        }
                    }
                    default:
                    {
                        action = OPTION_CLOSE;
                    }
                }
            }
        }
        case MUSIC_ALBUM_FAVORITE:
        {
            switch (result)
            {
                case ITEM_FAV_CHECKOUT:
                {
                    run_checkout();
                }
                case ITEM_FAV_INFORMATION:
                {
                    action = show_information();
                    if (action)
                    {
                        action = OPTION_RENAMED;
                    }
                }
                default:
                {
                    action = OPTION_CLOSE;
                }
            }
        }
        case MUSIC_ALBUM_CD:
        {
            switch (result)
            {
                case ITEM_CD_INFORMATION:
                {
                    action = show_information();
                }
                default:
                {
                    action = OPTION_CLOSE;
                }
            }
        }
        case MUSIC_ALBUM_PD:
        {
            switch (result)
            {
                case ITEM_PD_CHECKIN:
                {
                    action = run_checkin();
                    if (action)
                    {
                        action = OPTION_ALBUM_CHANGED;
                    }
                    check_pd(2);
                }
                case ITEM_PD_DELETE:
                {
                    action = run_delete();
                    if (action)
                    {
                        action = OPTION_ALBUM_CHANGED;
                    }
                }
                case ITEM_PD_INFORMATION:
                {
                    action = show_information();
                }
                default:
                {
                    action = OPTION_CLOSE;
                }
            }
        }
    }
    return action;
}

/* Runs the check-in dialog of music_dialog for the portable device; returns its result. */
run_checkin()
{
    set_aval_int("MediaWatcherFlag", 1);
    new args[64] = {0};
    sprintf(args, "channel_mode=%d;album_mode=%d", CHANNEL_ALBUM, g_album_mode);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 0.5 * g_screen_w, 0.5 * g_screen_h, 0.0, 0);
    if (g_pd_tracks < 0)
    {
        set_aval_int("CheckoutTrack", -1);
    }
    else
    {
        set_aval_int("CheckoutTrack", -g_pd_tracks);
    }
    AttachScript(dialog, "checkin_main", "music_dialog", args);
    SetPubVar(dialog, "focus", focus_glowfly);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    new result = GetPubVar(dialog, "result");
    Destroy(dialog);
    set_aval_int("MediaWatcherFlag", 0);
    return result;
}

/* Runs the check-out dialog of music_dialog for the selected album; returns its result. */
run_checkout()
{
    set_aval_int("MediaWatcherFlag", 1);
    new args[64] = {0};
    sprintf(args, "channel_mode=%d;album_mode=%d", CHANNEL_ALBUM, g_album_mode);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 0.5 * g_screen_w, 0.5 * g_screen_h, 0.0, 0);
    if (MUSIC_ALBUM_FAVORITE == g_album_mode)
    {
        new status;
        status = open_db(0);
        new query[512];
        sprintf(query,
                "select scei_jukebox.url from scei_jukebox_favorite_color,scei_jukebox where scei_jukebox.url = scei_jukebox_favorite_color.url AND favorite = %d ORDER BY entrydate ASC",
                g_album_select + 1);
        status = datSQLExec(query);
        status = wait_database();
        new count;
        count = datGetNum();
        set_aval_int("CheckoutTrack", -count);
    }
    else
    {
        set_aval_int("CheckoutTrack", -atoi(g_album_tracks));
    }
    AttachScript(dialog, "checkout_main", "music_dialog", args);
    SetPubVar(dialog, "focus", focus_glowfly);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    new result = GetPubVar(dialog, "result");
    Destroy(dialog);
    return result;
}

/* Runs the delete dialog of music_dialog for the selected album; returns its result. */
run_delete()
{
    new args[64] = {0};
    sprintf(args, "channel_mode=%d;album_mode=%d", CHANNEL_ALBUM, g_album_mode);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 0.5 * g_screen_w, 0.5 * g_screen_h, 0.0, 0);
    set_aval_int("CheckoutTrack", -atoi(g_album_tracks));
    AttachScript(dialog, "delete_main", "music_dialog", args);
    SetPubVar(dialog, "focus", focus_glowfly);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    new result = GetPubVar(dialog, "result");
    Destroy(dialog);
    return result;
}

/*
 * Keeps the length and the size of the selected album in the aval file and opens the information page; if a name
 * was changed there, writes the album and artist names to the database. Returns 1 after a rename, -1 on an error.
 */
show_information()
{
    set_aval_int("MusicAlbumMode", g_album_mode);
    if (MUSIC_ALBUM_FAVORITE == g_album_mode)
    {
        new query[512];
        new status;
        sprintf(query,
                "select sum(scei_jukebox.datasize/1024),sum(substr(scei_jukebox.length,1,2)*3600)+sum(substr(scei_jukebox.length,4,2)*60+substr(scei_jukebox.length,7,2)) from scei_jukebox_favorite_color,scei_jukebox where scei_jukebox.url = scei_jukebox_favorite_color.url AND favorite = %d",
                g_album_select + 1);
        status = open_db(0);
        status = datSQLExec(query);
        status = wait_database();
        /* NOTE: bug: the sums are row 0, read only when g_album_select is 0; else query keeps the SQL */
        datGetData(g_album_select, 1, query, 512);
        avalSet("DataSize", query);
        printf("DATASIZE = %s\n", query);
        new total;
        new sec;
        new minutes;
        new hours;
        datGetData(g_album_select, 2, query, 512);
        total = atoi(query);
        hours = total / 3600;
        minutes = (total - hours * 60 * 60) / 60;
        sec = total - (hours * 60 + minutes) * 60;
        sprintf(query, "%02d:%02d:%02d", hours, minutes, sec);
        avalSet("Length", query);
        printf("LENGTH = %s\n", query);
    }
    else if (g_album_mode == MUSIC_ALBUM_PD)
    {
        new query[512];
        new status;
        g_pd_tracks = get_aval_int("PDTracks");
        if (g_pd_tracks > 0)
        {
            copy_string(query, "select sum(scei_jukebox.datasize) from scei_jukebox where format <= -3", 512);
            status = open_db(0);
            status = datSQLExec(query);
            status = wait_database();
            new total;
            new sec;
            new minutes;
            new hours;
            datGetData(g_album_select, 1, query, 512);
            total = atoi(query) / 1000;
            hours = total / 3600;
            minutes = (total - hours * 60 * 60) / 60;
            sec = total - (hours * 60 + minutes) * 60;
            sprintf(query, "%02d:%02d:%02d", hours, minutes, sec);
            avalSet("Length", query);
            printf("LENGTH = %s\n", query);
        }
        else
        {
            avalSet("Length", "00:00:00");
        }
    }
    if (g_start_mode == MUSIC_ALBUM_COLLECTION)
    {
        if (g_album_mode == MUSIC_ALBUM_CD)
        {
            open_info_page("info", 1, 1);
            return 0;
        }
        if (g_album_mode == MUSIC_ALBUM_PD)
        {
            open_info_page("info", 1, 0);
            return 0;
        }
        open_info_page("info", 0, 0);
    }
    else
    {
        open_info_page("info", 2, 0);
    }
    new edited = get_aval_int("MusicInfoEdit");
    if (0 == edited)
    {
        return 0;
    }
    new result;
    new path[129];
    new update[384];
    new artist[129];
    new escaped[257];
    open_db(0);
    avalGetValue("ContentPath", 0, path, 128);
    printf("url = %s\n", path);
    avalGetValue("MusicAlbumName", 0, g_album_title, 128);
    printf("albumname = %s\n", g_album_title);
    if (0 == strlen(g_album_title))
    {
        if (MUSIC_ALBUM_FAVORITE == g_album_mode)
        {
            get_tag_attr("TEXT", "msgmusic_optmenut_library_favorite", "value", escaped, 257);
        }
        else
        {
            get_tag_attr("TEXT", "msgmusic_ainfo_album_default", "value", escaped, 257);
        }
        sprintf(g_album_title, "%s%d", escaped, g_album_select + 1);
        avalSet("MusicAlbumName", g_album_title);
    }
    escape_sql(escaped, g_album_title);
    printf("+++++++ %s\n", escaped);
    if (MUSIC_ALBUM_FAVORITE == g_album_mode)
    {
        sprintf(update, "UPDATE scei_jukebox SET albumname = '%s', albumname2 = '' WHERE url = '%d'", escaped,
                g_album_select);
    }
    else
    {
        sprintf(update, "UPDATE scei_jukebox SET albumname = '%s', albumname2 = '' WHERE url like '%s%s'", escaped,
                path, "%");
    }
    result = datSQLExec(update);
    if (0 != result)
    {
        return -1;
    }
    result = wait_database();
    if (result < 0)
    {
        return -1;
    }
    if (MUSIC_ALBUM_FAVORITE == g_album_mode)
    {
        return 1;
    }
    avalGetValue("ArtistName", 0, artist, 128);
    printf("artistname = %s\n", artist);
    if (0 == strlen(artist))
    {
        get_tag_attr("TEXT", "msgmusic_ainfo_artist_default", "value", artist, 129);
        avalSet("ArtistName", artist);
    }
    escape_sql(escaped, artist);
    printf("+++++++ %s\n", escaped);
    sprintf(update, "UPDATE scei_jukebox SET artistname = '%s', artistname2 = '' WHERE url like '%s%s'", escaped, path,
            "%");
    result = datSQLExec(update);
    if (0 != result)
    {
        return -1;
    }
    result = wait_database();
    if (result < 0)
    {
        return -1;
    }
    return 1;
}

/* Creates the radio buttons of the albums: the hard disk, the CD and the portable device, or the favorite albums. */
create_mode_buttons()
{
    new buttons;
    buttons = CreateNull(Disp2D(0));
    new label0[64];
    new label1[64];
    new label2[64];
    new label3[64];
    new args[256];
    get_group_tag_attr2("TEXT", "album_menu", 0, "value", label0, 64);
    get_group_tag_attr2("TEXT", "album_menu", 1, "value", label1, 64);
    get_group_tag_attr2("TEXT", "album_menu", 2, "value", label2, 64);
    get_group_tag_attr2("TEXT", "album_menu", 3, "value", label3, 64);
    new count;
    if (g_start_mode == MUSIC_ALBUM_COLLECTION)
    {
        sprintf(args, "btn0 = %s ; btn1 = %s ; btn2 = %s ; interval = %d", label1, label2, label3,
                floatround(0.08 * g_screen_h, 0));
        count = 3;
    }
    else
    {
        sprintf(args, "btn0 = %s ; interval = %d", label0, floatround(0.08 * g_screen_h, 0));
        count = 1;
    }
    AttachScript(buttons, _, "radio_button_v", args);
    new Float:top = 0.5 - 0.08 * float(count - 1) / 2.0;
    SetPos(buttons, 0.14 * g_screen_w, g_screen_h * top, 0.0, 0);
    SetColor(buttons, 0.215, 0.708, 0.999, 0.0);
    SetOptionI(buttons, TAG("ZSRT"), ZSORT_OVERLAY);
    SetPubVar(buttons, "manager", Self());
    SetPubVar(buttons, "focus", focus_glowfly);
    SetPubVar(buttons, "padhelp", PADHELP_CIRCLE | PADHELP_CROSS);
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
    map_screen_point(x, y, z, 0.7625 * g_screen_w, 0.5 * g_screen_h, Parent(0));
    SetPos(album, x, y, z, 0);
    AttachScript(album, "general_menu", "general_menu");
    SetPubVar(album, "manager", Self());
    SetPubVar(album, "focus", focus_glowfly);
    SetPubVar(album, "pad_enable", PAD_TRIANGLE | PAD_CROSS);
    SetPubVar(album, "padhelp", PADHELP_CROSS);
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
    if (MUSIC_ALBUM_CD != g_album_mode)
    {
        AnimTimerManager(Self(), 1000.0, 0.0, ANIM_FUNC_LINEAR);
        wait_ms(200);
        new caption[64];
        if (g_album_mode == MUSIC_ALBUM_COLLECTION || MUSIC_ALBUM_FAVORITE == g_album_mode)
        {
            get_tag_attr("TEXT", "msgmusic_album_getinfo", "value", caption, 64);
        }
        else
        {
            get_tag_attr("TEXT", "msgmusic_usb_connect_confirm", "value", caption, 64);
        }
        show_caption2(caption, _, 1.0, 400.0, 0.0);
        wait_ms(400.0);
    }
    new count = query_album(mode);
    printf("new db opened %d = %d\n", mode, count);
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
        album = create_album(count, select);
        new pd_busy;
        SystemCommand(SYSCMD_BUSY, 1);
        do
        {
            printf(" +++++++++++++++++++++++++++++++++++++++++++  pd_db_status = %d\n", pd_busy);
            playerExecCommand(PLAYER_PD_GET_BUSY, pd_busy);
            wait_ms(1000);
        }
        while (pd_busy);
        SystemCommand(SYSCMD_BUSY, 0);
        show_caption2(_, _, 0.0, 400.0, 0.0);
    }
    else
    {
        show_caption2(_, _, 0.0, 400.0, 0.0);
        wait_ms(200);
        show_album_error(count);
        g_pd_result = count;
    }
    return count;
}

/* Creates the album item of album index: the default icon. */
create_album_item(path, index, count)
{
    new item = CreatePlane(path);
    AttachScript(item, "album_item", "album_item");
    SetPubVar(item, "pad_enable", PAD_CIRCLE);
    SetOptionI(item, TAG("FDEG"), 1);
    SetColor(item, 1.0, 1.0, 1.0, 0.0);
    if (index >= 0 && index < count)
    {
        SendMessage(item, TAG("TEX"), g_default_icon, 1.0, 0, 0);
    }
    return item;
}

/*
 * Lists the albums of mode: the rows of the database (retried while the database is busy), the CD in the drive or
 * the portable device; returns their number, negative on an error.
 */
query_album(mode)
{
    printf("==== album_db_open %d\n", mode);
    idx = reset_player();
    new status;
    new count;
    switch (mode)
    {
        case MUSIC_ALBUM_COLLECTION:
        {
            new ready = -1;
            new tries;
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
            status = open_db(0);
            status = datSQLExec(
                    "select albumname,url,trackno,artistname,length,datasize/1024 from scei_jukebox where format = -1 ORDER BY album_no DESC");
            if (0 != status)
            {
                return -1;
            }
            status = wait_database();
            if (status < 0)
            {
                return -1;
            }
            new value[256];
            datGetData(0, ALBUM_COL_URL, g_content_path, 256);
            new path_len = strlen(g_content_path);
            avalSet("ContentPath", g_content_path);
            datGetData(0, ALBUM_COL_TRACKS, value, 256);
            set_aval_int("AlbumTracks", atoi(value));
            datGetData(0, ALBUM_COL_ARTIST, value, 256);
            avalSet("ArtistName", value);
            datGetData(0, ALBUM_COL_LENGTH, value, 256);
            avalSet("Length", value);
            datGetData(0, ALBUM_COL_SIZE, value, 256);
            avalSet("DataSize", value);
            count = datGetNum();
            set_aval_int("MusicAlbumNum", count);
            g_cd_items = 0;
            g_pd_items = 0;
            return count + g_cd_items + g_pd_items;
        }
        case MUSIC_ALBUM_FAVORITE:
        {
            new retries;
            retry:
            if (retries > 20)
            {
                return -1;
            }
            status = open_db(0);
            printf("open ret %d\n", status);
            status = datSQLExec(
                    "select albumname,url,trackno,artistname,length,datasize/1024 from scei_jukebox where format = -2 ORDER BY album_no DESC");
            printf("exec ret %d\n", status);
            if (0 != status)
            {
                return -1;
            }
            status = wait_database();
            printf("wait ret %d\n", status);
            if (status < 0)
            {
                datClose();
                g_db_open = 0;
                wait_ms(100);
                retries++;
                goto retry;
            }
            count = datGetNum();
            if (count <= 0)
            {
                wait_ms(100);
                retries++;
                goto retry;
            }
            return count;
        }
        case MUSIC_ALBUM_CD:
        {
            g_disc_type = get_disc_type();
            g_cd_items = g_disc_type == CDVD_CDDA ? 1 : 0;
            return g_cd_items;
        }
        case MUSIC_ALBUM_PD:
        {
            printf("++++++++++++++++++++++++++++++++ from album_db_open\n");
            set_aval_int("MediaWatcherFlag", 1);
            new ready = -1;
            new tries;
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
            g_pd_tracks = get_aval_int("PDTracks");
            if (g_pd_tracks > 0)
            {
                check_pd(2);
            }
            else
            {
                check_pd(0);
            }
            set_aval_int("MediaWatcherFlag", 0);
            if (g_pd_tracks >= 0)
            {
                return 1;
            }
            else
            {
                return g_pd_tracks;
            }
        }
    }
    return 0;
}

/* Shows why the album is empty (count 0) or could not be read: no album, no disc, no device, ... */
show_album_error(count)
{
    new line1[64] = {0};
    new line2[64] = {0};
    if (g_album_mode == MUSIC_ALBUM_COLLECTION || MUSIC_ALBUM_FAVORITE == g_album_mode)
    {
        if (count <= 0)
        {
            get_tag_attr("TEXT", "msgmusic_album_none", "value", line1, 64);
        }
        else if (count < 0)
        {
            get_tag_attr("TEXT", "msgmusic_album_corrupt", "value", line1, 64);
        }
    }
    else if (g_album_mode == MUSIC_ALBUM_CD)
    {
        if (CDVD_NO_DISC == g_disc_type)
        {
            get_tag_attr("TEXT", "msgmusic_disc_none", "value", line1, 64);
        }
        else if (g_disc_type == CDVD_DETECTING)
        {
            get_tag_attr("TEXT", "msgmusic_disc_reading", "value", line1, 64);
        }
        else
        {
            get_tag_attr("TEXT", "msgmusic_disc_notreading", "value", line1, 64);
        }
    }
    else if (count == PD_NO_MEDIA)
    {
        get_tag_attr("TEXT", "msgmusic_usb_insert1", "value", line1, 64);
        get_tag_attr("TEXT", "msgmusic_usb_insert2", "value", line2, 64);
    }
    else if (count == PD_CONNECTING)
    {
        get_tag_attr("TEXT", "msgmusic_usb_connect_confirm", "value", line1, 64);
    }
    else
    {
        get_tag_attr("TEXT", "msgmusic_usb_nodevice", "value", line1, 64);
        get_tag_attr("TEXT", "msgmusic_usb_nodevice2", "value", line2, 64);
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

/* Returns the type of the disc in the drive: no disc, detecting, CDVD_CDDA for any audio CD, else CDVD_ILLEGAL. */
get_disc_type()
{
    new type = GetSystemParam(SYS_CDVD_TYPE);
    if (CDVD_NO_DISC == type)
    {
        return CDVD_NO_DISC;
    }
    if (type == CDVD_UNKNOWN)
    {
        return CDVD_ILLEGAL;
    }
    if (type < CDVD_PS1_CD)
    {
        return CDVD_DETECTING;
    }
    if (type == CDVD_CDDA || type == CDVD_PS1_CDDA || type == CDVD_PS2_CDDA)
    {
        return CDVD_CDDA;
    }
    return CDVD_ILLEGAL;
}

/* Runs on a child of the page (argument "owner"): sends "DISC" (type) to it when the disc type changes. */
public media_watcher_main()
{
    new owner;
    new arg[64];
    if (GetArg("owner", arg, 64))
    {
        owner = atoi(arg);
    }
    new old_type = get_disc_type();
    playerExecCommand(PLAYER_CD_READ_TOC, 1);
    for (; ; )
    {
        new type = get_disc_type();
        if (type != old_type)
        {
            playerExecCommand(PLAYER_CD_READ_TOC, 1);
            SendMessage(owner, TAG("DISC"), type, 0, 0, 0);
            old_type = type;
        }
        wait_ms(500);
    }
    return;
}

/*
 * Runs on a child of the page (arguments "owner" and "initial", the tracks known): while MediaWatcherFlag is 0, checks
 * the portable device every second and sends "PDEV" (tracks) to the owner when its tracks change.
 */
public pd_watcher_main()
{
    new owner;
    new initial;
    new arg[64];
    if (GetArg("owner", arg, 64))
    {
        owner = atoi(arg);
    }
    if (GetArg("initial", arg, 64))
    {
        initial = atoi(arg);
    }
    new prev_tracks = -1;
    if (prev_tracks != initial) /* redundant as written */
    {
        prev_tracks = initial;
    }
    new count;
    new unused = -1; /* unused, but part of the layout */
    new paused;
    for (; ; )
    {
        paused = get_aval_int("MediaWatcherFlag");
        if (!paused)
        {
            check_pd(1);
            g_pd_tracks = get_aval_int("PDTracks");
            if (prev_tracks != g_pd_tracks)
            {
                printf("=== count = %d PD found %d trks  prev = %d\n", count, g_pd_tracks, prev_tracks);
                printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! STATUS change\n");
                SendMessage(owner, TAG("PDEV"), g_pd_tracks, 0, 0, 0);
                prev_tracks = g_pd_tracks;
                set_aval_int("PrevTracks", g_pd_tracks);
            }
        }
        wait_ms(1000);
    }
    return;
}

/* Decodes the image of IMG id and keeps its texture; returns it, 0 on failure. */
load_icon_texture(id[])
{
    new url[256];
    get_tag_attr("IMG", id, "src", url, 256);
    start_image_decode2(url);
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

/* Fades the radio buttons, and the box if there are albums, to alpha; hides the caption. */
fade_menus(Float:alpha, Float:duration)
{
    SendMessage(g_mode_buttons, TAG("FADE"), alpha, duration, 0, 0);
    new Float:box_alpha;
    box_alpha = g_album_count <= 0 ? 0.0 : alpha;
    fade_box(box_alpha, duration, 1.0, 1.0, 1.0);
    box_alpha = g_album_count > 0 ? 0.0 : alpha;
    show_caption2(_, _, 0.0, 400.0, 0.0);
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
    SetPos(camera_focus, 0.0, 0.0, -100.0, 0);
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

/*
 * Returns the result of a push of item index in an option menu, by the prefix of its id: circle on a command_ item
 * chooses it, on a submenu_ item opens the submenu (the quality one on the current quality), on a radio_ item sets
 * the recording quality; cross goes back.
 */
handle_menu_push(manager, side, &mode, index, button, title[], group[], id[])
{
    new unused; /* takes the parameters the function does not use */
    new buffer[64];
    unused = manager;
    unused = side;
    unused = mode;
    unused = index;
    unused = 64;
    copy_string(buffer, id, unused);
    copy_string(buffer, group, unused);
    copy_string(buffer, title, unused);
    if (0 == strncmp(id, "command_", 8))
    {
        if (button == TAG("RRGT"))
        {
            return index + MENU_ITEM;
        }
        if (button == TAG("RDWN"))
        {
            return MENU_BACK;
        }
    }
    else if (0 == strncmp(id, "submenu_", 8))
    {
        if (button == TAG("RRGT"))
        {
            if (0 == strcmp(id, "submenu_preference"))
            {
                return MENU_SUBMENU;
            }
            if (0 == strcmp(id, "submenu_quality"))
            {
                new quality = get_aval_int("MusicPrefQuality");
                switch (quality)
                {
                    case QUALITY_HIGH:
                    {
                        sprintf(id, "radio_quality_high");
                    }
                    case QUALITY_NORMAL:
                    {
                        sprintf(id, "radio_quality_norm");
                    }
                    case QUALITY_LOW:
                    {
                        sprintf(id, "radio_quality_low");
                    }
                }
                return MENU_SUBMENU;
            }
        }
        if (button == TAG("RDWN"))
        {
            return MENU_BACK;
        }
    }
    else if (0 == strncmp(id, "radio_", 6))
    {
        if (button == TAG("RRGT"))
        {
            playerExecCommand(PLAYER_SET_QUALITY, index);
            set_aval_int("MusicPrefQuality", index);
            return MENU_BACK;
        }
        if (button == TAG("RDWN"))
        {
            return MENU_BACK;
        }
    }
    return MENU_REOPEN;
}

/* Returns the number of items an option menu has besides those of its MENUGROUP: none. */
count_extra_items(side, result, mode, title[], group[], id[], &selected)
{
    new unused; /* takes the parameters the function does not use */
    new buffer[64];
    unused = side;
    unused = result;
    unused = mode;
    unused = selected;
    unused = 64;
    copy_string(buffer, title, unused);
    copy_string(buffer, group, unused);
    copy_string(buffer, id, unused);
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

/* Opens the music database once; with no_wait it does not wait for it. Returns its status, -1 on an error. */
open_db(no_wait)
{
    new status;
    if (0 == g_db_open)
    {
        status = datOpen(DAT_DATABASE, g_db_name, DAT_ORDER_MTIME_DESC);
        if (0 != status)
        {
            return -1;
        }
        if (!no_wait)
        {
            status = wait_database();
            if (status < 0)
            {
                return -1;
            }
        }
        g_db_open = 1;
    }
    return status;
}

/* Leaves for the top menu, on the channel of the page. */
go_to_top()
{
    g_quit = 1;
    g_next_link = "top";
    if (MUSIC_ALBUM_FAVORITE == g_start_mode)
    {
        g_next_args = "channel_id = BNCH-MUSIC-FAVORITE ;";
    }
    else if (g_start_mode == MUSIC_ALBUM_COLLECTION)
    {
        g_next_args = "channel_id = BNCH-MUSIC-COLLECTION ;";
    }
}

/* Shows the warning that the player could not be initialized and waits until it is closed. */
show_unit_warning()
{
    new args[513];
    new line1[256];
    new line2[256];
    get_tag_attr("TEXT", "msgmusic_warn_unitwrong1", "value", line1, 256);
    get_tag_attr("TEXT", "msgmusic_warn_unitwrong2", "value", line2, 256);
    sprintf(args, "msg=%s\n%s", line1, line2);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 0.5 * g_screen_w, 0.5 * g_screen_h, 0.0, 0);
    AttachScript(dialog, _, "confirm_d", args);
    SetPubVar(dialog, "manager", Self());
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    Destroy(dialog);
}
