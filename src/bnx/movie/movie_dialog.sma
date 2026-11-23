/*
 * /opt0/bn/script/movie/movie_dialog.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The dialogs of the movie pages, each a public function attached to an object of its own: create an album, move a
 * movie to another album, delete an album or a movie, add an album or a movie to a favorite album, show a title.
 * Each leaves in result whether the album changed.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "moviedb.inc"

/* The errors of move_contents, addfav_album and addfav_contents (status), each with its message. */
const STATUS_FAILED = -1;
const STATUS_FULL = -2;     /* MAX_ITEMS movies or albums already */
const STATUS_NO_ALBUM = -3; /* no album to move or add to */

/* As in text.inc. */
const EDIT_MODE_USER_ID = 1;
const EDIT_MODE_PASSWORD = 2;
const EDIT_MODE_KATAKANA = 3;
const EDIT_MODE_NUMBER = 4;
const EDIT_MODE_KATAKANA_FIXED = 7;
const EDIT_MODE_QWERTY = 9;

#include "stdmenu.inc"
#include "genmenu.inc"
#include "scroll.inc"

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

#include "misc.inc"

new g_db_name[] = "movie";
new g_shadow_colors[4] = {0x5F000000, 0x5F000000, 0x5F000000, 0x5F000000}; /* the "SCL4" of the shadowed labels */
new Float:g_screen_width;
new Float:g_screen_height;
new Float:g_screen_depth;

/* Same as get_screen_size of text.inc. */
get_screen_size(&Float:screen_w, &Float:screen_h)
{
    GetScreen(Camera(0), g_screen_width, g_screen_height, g_screen_depth);
    screen_w = g_screen_width;
    screen_h = g_screen_height;
}

#include "layout.inc"

#include "focus.inc"

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

/* Same as run_edit_dialog of text.inc. */
run_edit_dialog(title[], text[], edit_mode, maxlen, max_chars)
{
    new frame_tex = load_texture("file:/opt0/bn/script/common/texture/textfield.tm2", 0, 0, -1);
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
    restore_pad_help();
    return ok;
}

new g_pad_help_on;
new g_pad_help_cross[32];
new g_pad_help_triangle[32];
new g_pad_help_square[32];
new g_pad_help_circle[32];
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
new g_album_second;

/* Same as load_texture of dom.inc. */
load_texture(url[], tex_w, tex_h, dec_mode)
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

/* Same as save_pad_help of pfs.inc. */
save_pad_help()
{
    g_pad_help_on = GetSystemParam(SYS_PADHELP);
    GetSystemParamStr(SYS_STR_PADHELP_RD, g_pad_help_cross, 32);
    GetSystemParamStr(SYS_STR_PADHELP_RU, g_pad_help_triangle, 32);
    GetSystemParamStr(SYS_STR_PADHELP_RL, g_pad_help_square, 32);
    GetSystemParamStr(SYS_STR_PADHELP_RR, g_pad_help_circle, 32);
}

/* Same as restore_pad_help of background.inc. */
restore_pad_help()
{
    SetSystemParam(SYS_PADHELP, g_pad_help_on);
    SetSystemParamStr(SYS_STR_PADHELP_RD, g_pad_help_cross);
    SetSystemParamStr(SYS_STR_PADHELP_RU, g_pad_help_triangle);
    SetSystemParamStr(SYS_STR_PADHELP_RL, g_pad_help_square);
    SetSystemParamStr(SYS_STR_PADHELP_RR, g_pad_help_circle);
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
        get_tag_attr2("TEXT", "msgmovie_add_album_new", "value", g_text, 64);
    }
    else
    {
        get_tag_attr2("TEXT", "msgmovie_favalbum", "value", g_text, 64);
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
        get_tag_attr2("TEXT", "msgmovie_albumname", "value", g_album_id, 128);
        sprintf(g_text, g_album_name);
        if (!run_edit_dialog(g_album_id, g_text, 0, 64, 256))
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
    time(g_album_hour[0], g_album_minute[0], g_album_second, 0);
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

public result; /* 1 when the album changed (an album added, a movie moved or deleted) */
public focus; /* the focus object of the dialogs, set by the caller */
public focus_glowfly; /* the focus object of the option menus */
new g_dialog_args[128];
new g_year[1];
new g_month[1];
new g_day[1];
new g_hour[1];
new g_minute[1];
new g_second;
new g_field[64];
new g_item_label[64];
new g_msg[64];
/* The answers of a question, or two lines of it joined into g_msg. */
new g_msg1[64];
new g_msg2[64];
new g_yes[16];
new g_no[16];
new g_movie_title[64];
new g_movie_album[64];
new g_dest_album_name[64];
new g_dest_album_id[64];
new g_create_date[16];
new g_uri[64];
new g_src_uri[64];
new g_dest_folder[64];
new g_content_type[4];
new g_content_subtype[4];
new g_sql2[128];

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

/* Asks for the name of a new album (argument album_mode: ALBUM_FAVORITE for a favorite album) and adds it. */
public create_album()
{
    new album_mode;
    result = 0;
    g_msg = "";
    if (GetArg("album_mode", g_msg, 64))
    {
        album_mode = atoi(g_msg);
    }
    SetSystemParam(SYS_PADHELP, 0);
    if (ALBUM_COLLECTION == album_mode || album_mode == ALBUM_DISC)
    {
        for (; ; )
        {
            result = add_album(album_mode, 1, "");
            if (result == ADD_ALBUM_NO_NAME)
            {
                get_tag_attr2("TEXT", "msgmovie_input_fail_noname", "value", g_msg, 64);
                show_message(g_msg);
            }
            else
            {
                if (result == ADD_ALBUM_FULL)
                {
                    goto show_result;
                }
                break;
            }
        }
    }
    else
    {
        for (; ; )
        {
            result = add_album(album_mode, 1, "");
            if (result == ADD_ALBUM_NO_NAME)
            {
                get_tag_attr2("TEXT", "msgmovie_input_fail_noname", "value", g_msg, 64);
                show_message(g_msg);
            }
            else
            {
                if (result == ADD_ALBUM_FULL)
                {
                    goto show_result;
                }
                break;
            }
        }
    }
    if (0 != result)
    {
        result = 0;
        goto done;
    }
    new progress_win = CreateNull(Disp2D(0));
    g_msg = "";
    get_tag_attr2("TEXT", "msgmovie_add_album", "value", g_msg, 64);
    sprintf(g_dialog_args, "msg=%s;cancel=%s", g_msg, "");
    SetPos(progress_win, 320.0, 224.0, 0.0, 0);
    AttachScript(progress_win, _, "progress_window", g_dialog_args);
    SetPubVar(progress_win, "manager", Self());
    SetPubVar(progress_win, "focus", focus);
    while (SCRIPT_FINISHED != ScriptState(progress_win))
    {
        new progress = GetPubVar(progress_win, "progress");
        progress = progress + 1;
        progress = clamp(progress, 0, 100);
        SetPubVar(progress_win, "progress", progress);
        sleep;
    }
    SetPubVar(progress_win, "progress", 100);
    while (SCRIPT_FINISHED != ScriptState(progress_win))
    {
        SetSystemParam(SYS_PADHELP, 0);
        sleep;
    }
    if (progress_win)
    {
        Destroy(progress_win);
    }
    show_result:
    if (0 == result)
    {
        get_tag_attr2("TEXT", "msgmovie_add_album_complete", "value", g_msg, 64);
        result = 1;
    }
    else if (result == ADD_ALBUM_FAILED)
    {
        get_tag_attr2("TEXT", "msgmovie_add_album_fail", "value", g_msg, 64);
        result = 0;
    }
    else if (result == ADD_ALBUM_FULL)
    {
        get_tag_attr2("TEXT", "msgmovie_add_album_fail_max", "value", g_msg, 64);
        result = 0;
    }
    else
    {
        get_tag_attr2("TEXT", "msgmovie_add_album_fail", "value", g_msg, 64);
        result = 0;
    }
    show_message(g_msg);
    done:
    SetPubVar(Self(), "result", result);
    return result;
}

/*
 * Moves the movie of the argument uri to an album chosen in optmenu_move: the file goes into the folder of that
 * album, numbered when the name is taken, and its rows follow it.
 */
public move_contents()
{
    new album_mode;
    new status = STATUS_FAILED;
    new percent;
    new file_status;
    new choice;
    g_msg = "";
    if (GetArg("album_mode", g_msg, 64))
    {
        album_mode = atoi(g_msg);
    }
    if (ALBUM_COLLECTION != album_mode)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    g_src_uri = "";
    GetArg("uri", g_src_uri, 64);
    if (strlen(g_src_uri) <= 0)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    err20006(g_src_uri, g_uri);
    avalSet("uri", g_uri);
    status = datOpen(DAT_DATABASE, g_db_name, DAT_ORDER_MTIME_DESC);
    if (0 != status)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    status = wait_database();
    if (status < 0)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    sprintf(g_sql2, "select * from sce_movie where content_attr = %d and uri <> '%s' order by create_date desc",
            ATTR_ALBUM, g_uri);
    status = datSQLExec(g_sql2);
    if (0 != status)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    status = wait_database();
    if (status < 0)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    choice = datGetNum();
    datClose();
    if (choice <= 0)
    {
        status = STATUS_NO_ALBUM;
        goto show_result;
    }
    focus_glowfly = create_focus_plane2("focus");
    choice = pop_std_menu("optmenu_move", "optmenu_move", MENU_SIDE_RIGHT, 0);
    choice = choice - MENU_ITEM;
    if (choice < 0)
    {
        status = STATUS_FAILED;
        goto done;
    }
    SetSystemParam(SYS_PADHELP, 0);
    status = datOpen(DAT_DATABASE, g_db_name, DAT_ORDER_MTIME_DESC);
    if (0 != status)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    status = wait_database();
    if (status < 0)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    sprintf(g_sql2, "select * from sce_movie where content_attr = %d and uri <> '%s' order by create_date desc",
            ATTR_ALBUM, g_uri);
    status = datSQLExec(g_sql2);
    if (0 != status)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    status = wait_database();
    if (status < 0)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    datGetData(choice, COL_ALBUM_NAME, g_dest_album_name, 64);
    datGetData(choice, COL_ALBUM_ID, g_dest_album_id, 64);
    datGetData(choice, COL_URI, g_dest_folder, 64);
    sprintf(g_sql2, "select * from sce_movie where content_attr = %d and uri like '%s%s'", ATTR_MOVIE, g_dest_folder,
            "%");
    status = datSQLExec(g_sql2);
    if (0 != status)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    status = wait_database();
    if (status < 0)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    if (datGetNum() >= MAX_ITEMS)
    {
        status = STATUS_FULL;
        goto show_result;
    }
    new num;
    for (; ; )
    {
        err20005(g_dest_folder, g_src_uri, num, g_uri);
        new file_mode;
        new file_size;
        if (0 != fileLstat(g_uri, file_mode, file_size))
        {
            break;
        }
        num++;
    }
    new progress_win = CreateNull(Disp2D(0));
    g_msg = "";
    get_tag_attr2("TEXT", "msgmovie_move", "value", g_msg, 64);
    sprintf(g_dialog_args, "msg=%s;cancel=%s", g_msg, "");
    SetPos(progress_win, 320.0, 224.0, 0.0, 0);
    AttachScript(progress_win, _, "progress_window", g_dialog_args);
    SetPubVar(progress_win, "manager", Self());
    SetPubVar(progress_win, "focus", focus);
    date(g_year[0], g_month[0], g_day[0], 0);
    time(g_hour[0], g_minute[0], g_second, 0);
    sprintf(g_create_date, "%04d%02d%02d%02d%02d%02d", g_year, g_month, g_day, g_hour, g_minute, g_second);
    status = fileMove(g_src_uri, g_uri);
    if (status < 0)
    {
        status = STATUS_FAILED;
        goto close_progress;
    }
    for (; ; )
    {
        status = fileSync(percent, file_status);
        if (status == FILESYNC_DONE)
        {
            break;
        }
        if (status < 0)
        {
            status = STATUS_FAILED;
            goto close_progress;
        }
        SetPubVar(progress_win, "progress", percent / 2);
        sleep;
    }
    sprintf(g_sql2,
            "update sce_movie set album_name = '%s', uri = '%s', create_date = '%s', album_id = '%s' where uri = '%s' and content_attr = %d",
            g_dest_album_name, g_uri, g_create_date, g_dest_album_id, g_src_uri, ATTR_MOVIE);
    status = datSQLExec(g_sql2);
    if (0 != status)
    {
        status = STATUS_FAILED;
        goto close_progress;
    }
    status = wait_database();
    if (status < 0)
    {
        status = STATUS_FAILED;
        goto close_progress;
    }
    sprintf(g_sql2, "update sce_movie set album_name = '%s', uri = '%s' where uri = '%s' and content_attr = %d",
            g_dest_album_name, g_uri, g_src_uri, ATTR_FAV_MOVIE);
    status = datSQLExec(g_sql2);
    if (0 != status)
    {
        status = STATUS_FAILED;
        goto close_progress;
    }
    status = wait_database();
    if (status < 0)
    {
        status = STATUS_FAILED;
        goto close_progress;
    }
    datClose();
    while (SCRIPT_FINISHED != ScriptState(progress_win))
    {
        new progress = GetPubVar(progress_win, "progress");
        progress = progress + 1;
        progress = clamp(progress, 0, 100);
        SetPubVar(progress_win, "progress", progress);
        sleep;
    }
    close_progress:
    SetPubVar(progress_win, "progress", 100);
    while (SCRIPT_FINISHED != ScriptState(progress_win))
    {
        SetSystemParam(SYS_PADHELP, 0);
        sleep;
    }
    if (progress_win)
    {
        Destroy(progress_win);
    }
    show_result:
    if (status >= 0)
    {
        get_tag_attr2("TEXT", "msgmovie_move_complete", "value", g_msg, 64);
    }
    else if (status == STATUS_FAILED)
    {
        get_tag_attr2("TEXT", "msgmovie_move_fail", "value", g_msg, 64);
    }
    else if (status == STATUS_FULL)
    {
        get_tag_attr2("TEXT", "msgmovie_move_fail_max", "value", g_msg, 64);
    }
    else if (status == STATUS_NO_ALBUM)
    {
        get_tag_attr2("TEXT", "msgmovie_move_noalbum", "value", g_msg, 64);
    }
    else
    {
        get_tag_attr2("TEXT", "msgmovie_move_fail", "value", g_msg, 64);
    }
    show_message(g_msg);
    done:
    if (status >= 0)
    {
        result = 1;
    }
    else
    {
        result = 0;
    }
    SetPubVar(Self(), "result", result);
    return result;
}

/* Deletes the album of the argument uri after asking, with its folder and movies in the collection. */
public delete_album()
{
    new album_mode;
    new status = -1;
    new percent;
    new file_status;
    new answer;
    g_msg = "";
    if (GetArg("album_mode", g_msg, 64))
    {
        album_mode = atoi(g_msg);
    }
    g_uri = "";
    GetArg("uri", g_uri, 64);
    if (strlen(g_uri) <= 0)
    {
        goto show_result;
    }
    g_msg = "";
    get_tag_attr2("TEXT", "msgmovie_album_del_ask", "value", g_msg, 64);
    g_msg1 = "";
    get_tag_attr2("TEXT", "msgmovie_common_yes", "value", g_msg1, 64);
    g_msg2 = "";
    get_tag_attr2("TEXT", "msgmovie_common_no", "value", g_msg2, 64);
    answer = ask_yes_no(g_msg, g_msg1, g_msg2);
    if (0 != answer)
    {
        goto done;
    }
    status = datOpen(DAT_DATABASE, g_db_name, DAT_ORDER_MTIME_DESC);
    if (0 != status)
    {
        status = -1;
        goto show_result;
    }
    status = wait_database();
    if (status < 0)
    {
        status = -1;
        goto show_result;
    }
    sprintf(g_sql2, "select * from sce_movie where uri like '%s%s' and content_attr = %d", g_uri, "%", ATTR_MOVIE);
    status = datSQLExec(g_sql2);
    if (0 != status)
    {
        status = -1;
        goto show_result;
    }
    status = wait_database();
    if (status < 0)
    {
        status = -1;
        goto show_result;
    }
    if (datGetNum() > 0)
    {
        g_msg1 = "";
        get_tag_attr2("TEXT", "msgmovie_album_del_ask_entity", "value", g_msg1, 64);
        g_msg2 = "";
        get_tag_attr2("TEXT", "msgmovie_album_del_ask_entity2", "value", g_msg2, 64);
        g_yes = "";
        get_tag_attr2("TEXT", "msgmovie_common_yes", "value", g_yes, 16);
        g_no = "";
        get_tag_attr2("TEXT", "msgmovie_common_no", "value", g_no, 16);
        sprintf(g_msg, "%s\n%s", g_msg1, g_msg2);
        answer = ask_yes_no(g_msg, g_yes, g_no);
        if (0 != answer)
        {
            status = -1;
            goto done;
        }
    }
    SetSystemParam(SYS_PADHELP, 0);
    new progress_win = CreateNull(Disp2D(0));
    g_msg = "";
    get_tag_attr2("TEXT", "msgmovie_album_del", "value", g_msg, 64);
    sprintf(g_dialog_args, "msg=%s;cancel=%s", g_msg, "");
    SetPos(progress_win, 320.0, 224.0, 0.0, 0);
    AttachScript(progress_win, _, "progress_window", g_dialog_args);
    SetPubVar(progress_win, "manager", Self());
    SetPubVar(progress_win, "focus", focus);
    if (ALBUM_COLLECTION == album_mode)
    {
        status = fileDelete(g_uri);
        if (status < 0)
        {
            goto close_progress;
        }
        for (; ; )
        {
            status = fileSync(percent, file_status);
            if (status == FILESYNC_DONE)
            {
                break;
            }
            if (status < 0)
            {
                goto close_progress;
            }
            SetPubVar(progress_win, "progress", percent / 2);
            sleep;
        }
    }
    sprintf(g_sql2, "delete from sce_movie where uri like '%s%s'", g_uri, "%");
    status = datSQLExec(g_sql2);
    if (status < 0)
    {
        goto close_progress;
    }
    status = wait_database();
    if (status < 0)
    {
        goto close_progress;
    }
    status = datClose();
    while (SCRIPT_FINISHED != ScriptState(progress_win))
    {
        new progress = GetPubVar(progress_win, "progress");
        progress = progress + 1;
        progress = clamp(progress, 0, 100);
        SetPubVar(progress_win, "progress", progress);
        sleep;
    }
    close_progress:
    SetPubVar(progress_win, "progress", 100);
    while (SCRIPT_FINISHED != ScriptState(progress_win))
    {
        SetSystemParam(SYS_PADHELP, 0);
        sleep;
    }
    if (progress_win)
    {
        Destroy(progress_win);
    }
    show_result:
    if (status >= 0)
    {
        get_tag_attr2("TEXT", "msgmovie_album_del_complete", "value", g_msg, 64);
    }
    else
    {
        get_tag_attr2("TEXT", "msgmovie_album_del_fail", "value", g_msg, 64);
    }
    show_message(g_msg);
    done:
    if (status >= 0)
    {
        result = 1;
    }
    else
    {
        result = 0;
    }
    SetPubVar(Self(), "result", result);
    return result;
}

/*
 * Deletes the movie of the arguments uri and create_date after asking: in the collection the file and its rows, in
 * a favorite album its row.
 */
public delete_contents()
{
    new album_mode;
    new status = -1;
    new percent;
    new file_status;
    new answer;
    new create_date[16];
    g_msg = "";
    if (GetArg("album_mode", g_msg, 64))
    {
        album_mode = atoi(g_msg);
    }
    g_uri = "";
    GetArg("uri", g_uri, 64);
    if (strlen(g_uri) <= 0)
    {
        status = -1;
        goto show_result;
    }
    create_date = "";
    GetArg("create_date", create_date, 16);
    if (strlen(create_date) <= 0)
    {
        status = -1;
        goto show_result;
    }
    if (ALBUM_COLLECTION == album_mode)
    {
        g_msg = "";
        get_tag_attr2("TEXT", "msgmovie_movie_del_ask", "value", g_msg, 64);
        g_msg1 = "";
        get_tag_attr2("TEXT", "msgmovie_common_yes", "value", g_msg1, 64);
        g_msg2 = "";
        get_tag_attr2("TEXT", "msgmovie_common_no", "value", g_msg2, 64);
        answer = ask_yes_no(g_msg, g_msg1, g_msg2);
        if (0 != answer)
        {
            status = -1;
            goto done;
        }
        g_msg1 = "";
        get_tag_attr2("TEXT", "msgmovie_movie_del_ask_entity", "value", g_msg1, 64);
        g_msg2 = "";
        get_tag_attr2("TEXT", "msgmovie_movie_del_ask_entity2", "value", g_msg2, 64);
        g_yes = "";
        get_tag_attr2("TEXT", "msgmovie_common_yes", "value", g_yes, 16);
        g_no = "";
        get_tag_attr2("TEXT", "msgmovie_common_no", "value", g_no, 16);
        sprintf(g_msg, "%s\n%s", g_msg1, g_msg2);
        answer = ask_yes_no(g_msg, g_yes, g_no);
        if (0 != answer)
        {
            status = -1;
            goto done;
        }
    }
    else if (album_mode == ALBUM_FAVORITE)
    {
        g_msg = "";
        get_tag_attr2("TEXT", "msgmovie_del_favorite_ask", "value", g_msg, 64);
        g_msg1 = "";
        get_tag_attr2("TEXT", "msgmovie_common_yes", "value", g_msg1, 64);
        g_msg2 = "";
        get_tag_attr2("TEXT", "msgmovie_common_no", "value", g_msg2, 64);
        answer = ask_yes_no(g_msg, g_msg1, g_msg2);
        if (0 != answer)
        {
            status = -1;
            goto done;
        }
    }
    SetSystemParam(SYS_PADHELP, 0);
    new progress_win = CreateNull(Disp2D(0));
    g_msg = "";
    get_tag_attr2("TEXT", "msgmovie_movie_del", "value", g_msg, 64);
    sprintf(g_dialog_args, "msg=%s;cancel=%s", g_msg, "");
    SetPos(progress_win, 320.0, 224.0, 0.0, 0);
    AttachScript(progress_win, _, "progress_window", g_dialog_args);
    SetPubVar(progress_win, "manager", Self());
    SetPubVar(progress_win, "focus", focus);
    if (ALBUM_COLLECTION == album_mode)
    {
        status = fileDelete(g_uri);
        if (status < 0)
        {
            goto close_progress;
        }
        for (; ; )
        {
            status = fileSync(percent, file_status);
            if (status == FILESYNC_DONE)
            {
                break;
            }
            if (status < 0)
            {
                goto close_progress;
            }
            SetPubVar(progress_win, "progress", percent / 2);
            sleep;
        }
    }
    status = datOpen(DAT_DATABASE, g_db_name, DAT_ORDER_MTIME_DESC);
    if (0 != status)
    {
        goto close_progress;
    }
    status = wait_database();
    if (status < 0)
    {
        goto close_progress;
    }
    if (ALBUM_COLLECTION == album_mode)
    {
        sprintf(g_sql2, "delete from sce_movie where uri = '%s'", g_uri);
    }
    else if (album_mode == ALBUM_FAVORITE)
    {
        sprintf(g_sql2, "delete from sce_movie where create_date = '%s' and uri = '%s'", create_date, g_uri);
    }
    status = datSQLExec(g_sql2);
    if (status < 0)
    {
        goto close_progress;
    }
    status = wait_database();
    if (status < 0)
    {
        goto close_progress;
    }
    status = datClose();
    while (SCRIPT_FINISHED != ScriptState(progress_win))
    {
        new progress = GetPubVar(progress_win, "progress");
        progress = progress + 1;
        progress = clamp(progress, 0, 100);
        SetPubVar(progress_win, "progress", progress);
        sleep;
    }
    close_progress:
    SetPubVar(progress_win, "progress", 100);
    while (SCRIPT_FINISHED != ScriptState(progress_win))
    {
        SetSystemParam(SYS_PADHELP, 0);
        sleep;
    }
    if (progress_win)
    {
        Destroy(progress_win);
    }
    show_result:
    if (status >= 0)
    {
        get_tag_attr2("TEXT", "msgmovie_movie_del_complete", "value", g_msg, 64);
    }
    else
    {
        get_tag_attr2("TEXT", "msgmovie_movie_del_fail", "value", g_msg, 64);
    }
    show_message(g_msg);
    done:
    if (status >= 0)
    {
        result = 1;
    }
    else
    {
        result = 0;
    }
    SetPubVar(Self(), "result", result);
    return result;
}

/* Adds the movies of the album of the argument uri to a favorite album chosen in optmenu_favorite. */
public addfav_album()
{
    new album_mode;
    new status = STATUS_FAILED;
    new choice;
    new fav_count;
    new count;
    new i;
    g_msg = "";
    if (GetArg("album_mode", g_msg, 64))
    {
        album_mode = atoi(g_msg);
    }
    if (ALBUM_COLLECTION != album_mode)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    g_src_uri = "";
    GetArg("uri", g_src_uri, 64);
    if (strlen(g_src_uri) <= 0)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    status = datOpen(DAT_DATABASE, g_db_name, DAT_ORDER_MTIME_DESC);
    if (0 != status)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    status = wait_database();
    if (status < 0)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    sprintf(g_sql2, "select * from sce_movie where uri like '%s%s' and content_attr = %d", g_src_uri, "%", ATTR_MOVIE);
    status = datSQLExec(g_sql2);
    if (0 != status)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    status = wait_database();
    if (status < 0)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    if (datGetNum() <= 0)
    {
        status = STATUS_NO_ALBUM;
        goto show_result;
    }
    focus_glowfly = create_focus_plane2("focus");
    choice = pop_std_menu("optmenu_favorite", "optmenu_favorite", MENU_SIDE_RIGHT, 0);
    choice = choice - MENU_ITEM;
    if (choice < 0)
    {
        status = STATUS_FAILED;
        goto done;
    }
    SetSystemParam(SYS_PADHELP, 0);
    sprintf(g_sql2, "select * from sce_movie where content_attr = %d", ATTR_FAV_ALBUM);
    status = datSQLExec(g_sql2);
    if (0 != status)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    status = wait_database();
    if (status < 0)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    datGetData(choice, COL_ALBUM_NAME, g_dest_album_name, 64);
    datGetData(choice, COL_ALBUM_ID, g_dest_album_id, 64);
    sprintf(g_sql2, "select * from sce_movie where content_attr = 1 and album_id = '%s'", g_dest_album_id);
    status = datSQLExec(g_sql2);
    if (0 != status)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    status = wait_database();
    if (status < 0)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    fav_count = datGetNum();
    sprintf(g_sql2, "select * from sce_movie where uri like '%s%s' and content_attr = %d", g_src_uri, "%", ATTR_MOVIE);
    status = datSQLExec(g_sql2);
    if (0 != status)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    status = wait_database();
    if (status < 0)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    count = datGetNum();
    if (fav_count + count > MAX_ITEMS)
    {
        status = STATUS_FULL;
        goto show_result;
    }
    new progress_win = CreateNull(Disp2D(0));
    g_msg = "";
    get_tag_attr2("TEXT", "msgmovie_add", "value", g_msg, 64);
    sprintf(g_dialog_args, "msg=%s;cancel=%s", g_msg, "");
    SetPos(progress_win, 320.0, 224.0, 0.0, 0);
    AttachScript(progress_win, _, "progress_window", g_dialog_args);
    SetPubVar(progress_win, "manager", Self());
    SetPubVar(progress_win, "focus", focus);
    i = 0;
    for (; i < count; i++)
    {
        sprintf(g_sql2, "select * from sce_movie where uri like '%s%s' and content_attr = %d", g_src_uri, "%",
                ATTR_MOVIE);
        status = datSQLExec(g_sql2);
        if (0 != status)
        {
            goto close_progress;
        }
        status = wait_database();
        if (status < 0)
        {
            goto close_progress;
        }
        if (datGetNum() <= 0)
        {
            break;
        }
        datGetData(i, COL_TITLE, g_field, 64);
        escape_sql(g_movie_title, g_field, 64);
        datGetData(i, COL_ALBUM_NAME, g_field, 64);
        escape_sql(g_movie_album, g_field, 64);
        datGetData(i, COL_URI, g_uri, 64);
        datGetData(i, COL_CONTENT_TYPE, g_content_type, 4);
        datGetData(i, COL_CONTENT_SUBTYPE, g_content_subtype, 4);
        date(g_year[0], g_month[0], g_day[0], 0);
        time(g_hour[0], g_minute[0], g_second, 0);
        sprintf(g_create_date, "%04d%02d%02d%02d%02d%02d", g_year, g_month, g_day, g_hour, g_minute, g_second);
        sprintf(g_sql2,
                "insert into sce_movie (title,album_name,create_date,uri,album_id,content_type,content_subtype,content_attr,table_version) values('%s','%s','%s','%s','%s',%d,%d,%d,%d)",
                g_movie_title, g_movie_album, g_create_date, g_uri, g_dest_album_id, atoi(g_content_type),
                atoi(g_content_subtype), ATTR_FAV_MOVIE, 512);
        status = datSQLExec(g_sql2);
        if (0 != status)
        {
            goto close_progress;
        }
        status = wait_database();
        if (status < 0)
        {
            goto close_progress;
        }
        SetPubVar(progress_win, "progress", i * 100 / count);
        sleep;
    }
    status = datClose();
    close_progress:
    SetPubVar(progress_win, "progress", 100);
    while (SCRIPT_FINISHED != ScriptState(progress_win))
    {
        SetSystemParam(SYS_PADHELP, 0);
        sleep;
    }
    if (progress_win)
    {
        Destroy(progress_win);
    }
    show_result:
    if (status >= 0)
    {
        get_tag_attr2("TEXT", "msgmovie_add_complete", "value", g_msg, 64);
    }
    else if (status == STATUS_FAILED)
    {
        get_tag_attr2("TEXT", "msgmovie_add_fail", "value", g_msg, 64);
    }
    else if (status == STATUS_FULL)
    {
        get_tag_attr2("TEXT", "msgmovie_add_warn_filemax", "value", g_msg, 64);
    }
    else if (status == STATUS_NO_ALBUM)
    {
        get_tag_attr2("TEXT", "msgmovie_regmovie_empty", "value", g_msg, 64);
    }
    else
    {
        get_tag_attr2("TEXT", "msgmovie_add_fail", "value", g_msg, 64);
    }
    show_message(g_msg);
    done:
    result = 0;
    SetPubVar(Self(), "result", result);
    return result;
}

/* Adds the movie of the argument uri to a favorite album chosen in optmenu_favorite. */
public addfav_contents()
{
    new album_mode;
    new status = STATUS_FAILED;
    g_msg = "";
    if (GetArg("album_mode", g_msg, 64))
    {
        album_mode = atoi(g_msg);
    }
    if (ALBUM_COLLECTION != album_mode)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    g_uri = "";
    GetArg("uri", g_uri, 64);
    if (strlen(g_uri) <= 0)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    focus_glowfly = create_focus_plane2("focus");
    new choice = pop_std_menu("optmenu_favorite", "optmenu_favorite", MENU_SIDE_RIGHT, 0);
    choice = choice - MENU_ITEM;
    if (choice < 0)
    {
        status = STATUS_FAILED;
        goto done;
    }
    SetSystemParam(SYS_PADHELP, 0);
    status = datOpen(DAT_DATABASE, g_db_name, DAT_ORDER_MTIME_DESC);
    if (0 != status)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    status = wait_database();
    if (status < 0)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    sprintf(g_sql2, "select * from sce_movie where content_attr = %d", ATTR_FAV_ALBUM);
    status = datSQLExec(g_sql2);
    if (0 != status)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    status = wait_database();
    if (status < 0)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    datGetData(choice, COL_ALBUM_NAME, g_dest_album_name, 64);
    datGetData(choice, COL_ALBUM_ID, g_dest_album_id, 64);
    sprintf(g_sql2, "select * from sce_movie where content_attr = 1 and album_id = '%s'", g_dest_album_id);
    status = datSQLExec(g_sql2);
    if (0 != status)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    status = wait_database();
    if (status < 0)
    {
        status = STATUS_FAILED;
        goto show_result;
    }
    if (datGetNum() >= MAX_ITEMS)
    {
        status = STATUS_FULL;
        goto show_result;
    }
    new progress_win = CreateNull(Disp2D(0));
    g_msg = "";
    get_tag_attr2("TEXT", "msgmovie_add", "value", g_msg, 64);
    sprintf(g_dialog_args, "msg=%s;cancel=%s", g_msg, "");
    SetPos(progress_win, 320.0, 224.0, 0.0, 0);
    AttachScript(progress_win, _, "progress_window", g_dialog_args);
    SetPubVar(progress_win, "manager", Self());
    SetPubVar(progress_win, "focus", focus);
    sprintf(g_sql2, "select * from sce_movie where uri = '%s' and content_attr = %d", g_uri, ATTR_MOVIE);
    status = datSQLExec(g_sql2);
    if (0 != status)
    {
        status = STATUS_FAILED;
        goto close_progress;
    }
    status = wait_database();
    if (status < 0)
    {
        status = STATUS_FAILED;
        goto close_progress;
    }
    datGetData(0, COL_TITLE, g_field, 64);
    escape_sql(g_movie_title, g_field, 64);
    datGetData(0, COL_ALBUM_NAME, g_field, 64);
    escape_sql(g_movie_album, g_field, 64);
    datGetData(0, COL_URI, g_uri, 64);
    datGetData(0, COL_CONTENT_TYPE, g_content_type, 4);
    datGetData(0, COL_CONTENT_SUBTYPE, g_content_subtype, 4);
    date(g_year[0], g_month[0], g_day[0], 0);
    time(g_hour[0], g_minute[0], g_second, 0);
    sprintf(g_create_date, "%04d%02d%02d%02d%02d%02d", g_year, g_month, g_day, g_hour, g_minute, g_second);
    sprintf(g_sql2,
            "insert into sce_movie (title,album_name,create_date,uri,album_id,content_type,content_subtype,content_attr,table_version) values('%s','%s','%s','%s','%s',%d,%d,%d,%d)",
            g_movie_title, g_movie_album, g_create_date, g_uri, g_dest_album_id, atoi(g_content_type),
            atoi(g_content_subtype), ATTR_FAV_MOVIE, 512);
    status = datSQLExec(g_sql2);
    if (0 != status)
    {
        status = STATUS_FAILED;
        goto close_progress;
    }
    status = wait_database();
    if (status < 0)
    {
        status = STATUS_FAILED;
        goto close_progress;
    }
    status = datClose();
    while (SCRIPT_FINISHED != ScriptState(progress_win))
    {
        new progress = GetPubVar(progress_win, "progress");
        progress = progress + 1;
        progress = clamp(progress, 0, 100);
        SetPubVar(progress_win, "progress", progress);
        sleep;
    }
    close_progress:
    SetPubVar(progress_win, "progress", 100);
    while (SCRIPT_FINISHED != ScriptState(progress_win))
    {
        SetSystemParam(SYS_PADHELP, 0);
        sleep;
    }
    if (progress_win)
    {
        Destroy(progress_win);
    }
    show_result:
    if (status >= 0)
    {
        get_tag_attr2("TEXT", "msgmovie_add_complete", "value", g_msg, 64);
    }
    else if (status == STATUS_FAILED)
    {
        get_tag_attr2("TEXT", "msgmovie_add_fail", "value", g_msg, 64);
    }
    else if (status == STATUS_FULL)
    {
        get_tag_attr2("TEXT", "msgmovie_add_warn_filemax", "value", g_msg, 64);
    }
    else
    {
        get_tag_attr2("TEXT", "msgmovie_add_fail", "value", g_msg, 64);
    }
    show_message(g_msg);
    done:
    result = 0;
    SetPubVar(Self(), "result", result);
    return result;
}

/* Shows the argument title in a window until the cross is pressed. */
public information_main()
{
    result = 0;
    new title[64] = {0};
    GetArg("title", title, 64);
    new window = CreateNull(Disp2D(0));
    SetPos(window, 320.0, 224.0, 0.0, 0);
    SetOptionI(window, TAG("ZSRT"), 2); /* acts as ZSORT_NONE */
    AttachScript(window, _, "window");
    SetPubVar(window, "manager", Self());
    SetPubVar(window, "focus", focus);
    SetPubVar(window, "width", 500.0);
    SetPubVar(window, "height", 250.0);
    new label = CreateLabel(Disp2D(0));
    SetPos(label, 192.0, 160.0, 0.0, 0);
    SetColor(label, 1.0, 1.0, 1.0, 1.0);
    SetOptionI(label, TAG("FSIZ"), 20);
    SetOptionI(label, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(label, TAG("HALN"), ALIGN_LEFT);
    SetOptionS(label, TAG("UTF8"), title);
    new held;
    do
    {
        PadState(held, _, _, _, -1);
        sleep;
    }
    while (0 == held & PAD_CROSS); /* until cross is pressed: & binds tighter than == */
    Destroy(label);
    Destroy(window);
    return result;
}

/* Asks msg with the buttons yes and no, no selected first; returns 0 for yes, 1 for no or cross. */
ask_yes_no(msg[], yes[], no[])
{
    sprintf(g_dialog_args, "msg=%s;yes=%s;no=%s", msg, yes, no);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    AttachScript(dialog, _, "confirm_yesno", g_dialog_args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus);
    SetPubVar(dialog, "select", 1);
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
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    sprintf(g_dialog_args, "msg=%s", msg);
    AttachScript(dialog, _, "confirm_d", g_dialog_args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    Destroy(dialog);
    return;
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

/*
 * Returns the number of items an option menu has besides those of its MENUGROUP: the favorite albums in
 * optmenu_favorite, the albums other than the one of the variable uri in optmenu_move.
 */
count_extra_items(side, menu_result, mode, title[], group[], id[], &selected)
{
    new status;
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
    if (0 == strcmp(group, "optmenu_favorite"))
    {
        sprintf(g_sql2, "select * from sce_movie where content_attr = %d order by create_date desc", ATTR_FAV_ALBUM);
    }
    else if (0 == strcmp(group, "optmenu_move"))
    {
        g_uri = "";
        avalGetValue("uri", 0, g_uri, 64);
        sprintf(g_sql2, "select * from sce_movie where content_attr = %d and uri <> '%s' order by create_date desc",
                ATTR_ALBUM, g_uri);
    }
    status = datSQLExec(g_sql2);
    if (0 != status)
    {
        return -1;
    }
    status = wait_database();
    if (status < 0)
    {
        return -1;
    }
    return datGetNum();
}

/* Sets the label of item index to the name of album index of count_extra_items. */
fill_menu_item(side, mode, index, group[], label[])
{
    if (index >= 0)
    {
        new len;
        len = datGetData(index, COL_ALBUM_NAME, g_item_label, 64);
        if (0 == len)
        {
            copy_string(label, g_item_label, 64);
        }
    }
}
