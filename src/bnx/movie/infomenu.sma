/*
 * /opt0/bn/script/movie/infomenu.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The information page of a movie or an album: a general_menu of the TEXT elements of a group of the page (title,
 * album, size, date, length, ...), filled from the row of the movie database that the arguments uri and attr name,
 * with its thumbnail. The title item opens the software keyboard to rename the movie or the album.
 */

#include <core>
#include <bn>

#pragma dynamic 2048

#include "infomenu.inc"
#include "genmenu.inc"
#include "scroll.inc"

#include "moviedb.inc"

#include "wait.inc"

#include "tag.inc"

#include <float>

/* The natives of the moviedata library (moviedata.so): the title and the thumbnail of a movie file. */
#pragma library "moviedata"
/*
 * Starts reading block entry of the user data of a movie file: kind 0 reads it as text (entry 0: the title), kind 1
 * as a TIM2 texture (entry 3: the thumbnail).
 */
native moviedataReadStart(const path[], kind, entry);
/* Returns 0 while reading, -1 on an error, 1 with the text in buf or, for kind 1, the texture in buf[0]. */
native moviedataReadSync(const buf[], maxlength);
#pragma library

#pragma library "movie"
native movieListClear();
native movieListAdd(const path[]);
native movieListNum();
native moviePlay(index);
/* Starts reading the length of a movie, for movieGetTimeSync. */
native movieGetTimeStart(const path[]);
/* Returns 0 while reading, 1 with the length, -1 on an error. */
native movieGetTimeSync(&hours, &minutes, &seconds);

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

#include "string.inc"

#include "focus.inc"

/*
 * Runs on the object of pop_info_menu: waits for "INIT" (manager, side, result, mode), shows the menu of the TEXT
 * elements of group, and sends "FNSH" (result, mode, selected index) to the manager when it ends.
 */
public info_menu_core()
{
    new title[64];
    GetArg("title", title, 64);
    new group[64];
    GetArg("group", group, 64);
    new id[64];
    GetArg("id", id, 64);
    new self = Self();
    new Float:screen_w;
    new Float:screen_h;
    get_screen_size4(screen_w, screen_h);
    new title_label;
    new menu;
    new scroll_bar;
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
    new selected;
    new item_count;
    if (INFOMENU_MODE_GROUP == mode)
    {
        item_count = item_count + count_group_tags("TEXT", group);
        selected = find_text_index(group, id);
    }
    item_count = item_count + load_movie_info(side, result, mode, title, group, id, selected);
    if (item_count <= 0 && selected < 0)
    {
        goto finish;
    }
    menu = CreateNull(self);
    new columns = 1;
    new rows = 6;
    SetPos(menu, 0.344 * screen_w, 0.42 * screen_h, 0.0, 0);
    SetOptionI(menu, TAG("ZSRT"), ZSORT_OVERLAY);
    AttachScript(menu, "general_menu", "general_menu");
    SetPubVar(menu, "manager", Self());
    SetPubVar(menu, "focus", GetPubVar(manager, "focus_glowfly"));
    SetPubVar(menu, "pad_enable", PAD_CROSS);
    SendMessage(menu, TAG("INUM"), item_count, 0, 0, 0);
    SendMessage(menu, TAG("CLRW"), columns, rows, 0, 0);
    SendMessage(menu, TAG("PADL"), -1, 1, 0, 0);
    SendMessage(menu, TAG("DIST"), 0, 0.06 * screen_h, 0, 0);
    SendMessage(menu, TAG("MGIN"), 2, 2, 0, 0);
    SendMessage(menu, TAG("MODE"), MENU_TYPE_POPUP, SCROLL_LINE, 0, 0);
    SendMessage(menu, TAG("ANIM"), 200.0, 0, ANIM_FUNC_EXPONENT, 0);
    SendMessage(menu, TAG("OFFA"), -1.0, 0, 0, 0);
    SendMessage(menu, TAG("IDLA"), 1.0, 0, 0, 0);
    SendMessage(menu, TAG("REPT"), 100.0, 100.0, 0, 0);
    SendMessage(menu, TAG("SLCT"), selected, 0, 0, 0);
    SendMessage(menu, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
    if (item_count > rows * columns)
    {
        scroll_bar = CreateNull(self);
        SetPos(scroll_bar, 0.05 * screen_w, 0.57 * screen_h, 0.0, 0);
        SetScale(scroll_bar, 2.0, 0.73, 1.0);
        SetOptionI(scroll_bar, TAG("ZSRT"), ZSORT_OVERLAY);
        AttachScript(scroll_bar, _, "scroll_bar");
        SetPubVar(scroll_bar, "manager", Self());
        SetPubVar(scroll_bar, "focus", GetPubVar(manager, "focus_glowfly"));
    }
    for (; ; )
    {
        new done;
        new msg;
        new a;
        new b;
        new c;
        /* "ITEM" (menu, path, index): create the item of index; "PUSH" (index, button, item). */
        while (RecvMessage(msg, a, b, c))
        {
            if (msg == TAG("ITEM"))
            {
                if (a == menu && IsValidObject(b))
                {
                    new index = c;
                    new label[64];
                    new edit[16];
                    if (INFOMENU_MODE_GROUP == mode)
                    {
                        get_group_tag_attr2("TEXT", group, index, "value", label, 64);
                        get_group_tag_attr2("TEXT", group, index, "edit", edit, 16);
                    }
                    new item = create_info_item(b, label, edit);
                    fill_info_item(manager, item, side, mode, index, group, label);
                    SendMessage(a, TAG("ITEM"), item, c, 0, 0);
                }
            }
            if (msg == TAG("PUSH"))
            {
                SendMessage(menu, TAG("PUSH"), 0, 0, 0, 0);
                SendMessage(menu, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                selected = a;
                if (INFOMENU_MODE_GROUP == mode)
                {
                    get_group_tag_attr2("TEXT", group, selected, "id", id, 64);
                }
                result = handle_push(manager, c, side, mode, selected, b, title, group, id);
                if (INFOMENU_MODE_GROUP == mode)
                {
                    apply_result(result, selected, title, group, id);
                }
                SendMessage(menu, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
                if (result >= INFOMENU_REOPEN)
                {
                    SendMessageStr(manager, TAG("TITL"), title);
                    SendMessageStr(manager, TAG("GRUP"), group);
                    SendMessageStr(manager, TAG("IDTY"), id);
                    done = 1;
                }
            }
        }
        if (scroll_bar)
        {
            new scroll_top = GetPubVar(menu, "scroll_top");
            SetPubVar(scroll_bar, "top", float(scroll_top) / float(item_count));
            SetPubVar(scroll_bar, "length", float(rows * columns) / float(item_count));
        }
        if (ScriptState(menu) == SCRIPT_FINISHED || done)
        {
            break;
        }
        sleep;
    }
    finish:
    SendMessage(manager, TAG("FNSH"), result, mode, selected, 0);
    if (menu)
    {
        SendMessage(menu, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
    }
    if (menu)
    {
        SendMessage(menu, TAG("FADE"), 0, 300.0, 0, 0);
    }
    if (title_label)
    {
        AnimColor(title_label, 0.7843, 0.7843, 0.7843, 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    }
    wait_ms(300);
    if (menu)
    {
        Destroy(menu);
    }
    if (scroll_bar)
    {
        Destroy(scroll_bar);
    }
    if (title_label)
    {
        Destroy(title_label);
    }
    return selected;
}

/* Shows the menu of the TEXT element id of group, and the menus it leads to; returns the last INFOMENU_* result. */
public pop_info_menu(group[], id[], side)
{
    new result;
    new mode;
    new pos = side;
    new Float:screen_w;
    new Float:screen_h;
    get_screen_size4(screen_w, screen_h);
    new menu_obj = CreateNull(Disp2D(0));
    if (INFOMENU_SIDE_LEFT == pos || pos == 2)
    {
        SetPos(menu_obj, 0.0 * screen_w, 0.0, 0.0, 0);
    }
    else if (pos == INFOMENU_SIDE_RIGHT)
    {
        SetPos(menu_obj, screen_w * (0.5 + 0.0625), 0.0, 0.0, 0);
    }
    new cur_title[64];
    new cur_group[64];
    new cur_id[64];
    new index = find_text_index(group, id);
    get_group_tag_attr2("TEXT", group, index, "value", cur_title, 64);
    get_group_tag_attr2("TEXT", group, index, "subgroup", cur_group, 64);
    for (; ; )
    {
        new args[256];
        sprintf(args, "title = %s ; group = %s ; id = %s", cur_title, cur_group, cur_id);
        AttachScript(menu_obj, "info_menu_core", "", args);
        SendMessage(menu_obj, TAG("INIT"), Self(), pos, result, mode);
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
        if (result >= INFOMENU_CLOSE)
        {
            break;
        }
    }
    if (menu_obj)
    {
        Destroy(menu_obj);
    }
    return result;
}

/* Returns the index of the TEXT element id among those of group, -1 if there is none. */
find_text_index(group[], id[])
{
    new index;
    for (new node = domFirstChild(domRoot(), "TEXT"); node; node = domNext(node, ""))
    {
        new value[64];
        domGetData(node, "group", value, 64);
        if (0 == strcmp(value, group))
        {
            domGetData(node, "id", value, 64);
            if (0 == strcmp(value, id))
            {
                return index;
            }
            index++;
        }
    }
    return -1;
}

/* Reads the retgroup and retid of item index of group: the menu it returns to. */
get_return_menu(index, group[], ret_group[], ret_id[])
{
    get_group_tag_attr2("TEXT", group, index, "retgroup", ret_group, 64);
    get_group_tag_attr2("TEXT", group, index, "retid", ret_id, 64);
}

/* Gets the title of the menu that the item id of group returns to. */
get_return_title(group[], id[], title[])
{
    new index = find_text_index(group, id);
    if (index < 0)
    {
        printf("WARNING: infomenu gettitle doesn't hit no id!\n");
        index = 0;
    }
    {
        new ret_group[64];
        new ret_id[64];
        get_return_menu(index, group, ret_group, ret_id);
        new ret_index = find_text_index(ret_group, ret_id);
        if (ret_index >= 0)
        {
            get_group_tag_attr2("TEXT", ret_group, ret_index, "value", title, 64);
        }
        else
        {
            printf("XML Can't find title!!!\n");
            sprintf(title, "");
        }
    }
}

/* Sets title, group and id of the menu that result leads to from item index; INFOMENU_ERROR if there is none. */
apply_result(&result, index, title[], group[], id[])
{
    if (result == INFOMENU_RETITLE)
    {
        new ret_title[64];
        get_return_title(group, id, ret_title);
        copy_string(title, ret_title, 64);
    }
    else if (result == INFOMENU_SUBMENU)
    {
        new sub_group[64];
        get_group_tag_attr2("TEXT", group, index, "subgroup", sub_group, 64);
        if (!strcmp(sub_group, ""))
        {
            printf("WARNING: nothing sub item !!\n");
        }
        else
        {
            new sub_title[64];
            get_group_tag_attr2("TEXT", group, index, "value", sub_title, 64);
            printf("group_sub %s, title %s\n", sub_group, sub_title);
            copy_string(group, sub_group, 64);
            copy_string(title, sub_title, 64);
        }
    }
    else if (result == INFOMENU_BACK)
    {
        new ret_group[64];
        new ret_id[64];
        get_return_menu(index, group, ret_group, ret_id);
        printf(" %d, %s, %s, %s, %s \n", index, group, id, ret_group, ret_id);
        if (!strcmp(ret_group, "") || !strcmp(ret_id, ""))
        {
            printf("WARNING: nothing return menu !!\n");
            result = INFOMENU_ERROR;
        }
        else
        {
            copy_string(group, ret_group, 64);
            copy_string(id, ret_id, 64);
            new ret_title[64];
            get_return_title(ret_group, ret_id, ret_title);
            copy_string(title, ret_title, 64);
            if (!strcmp(ret_title, ""))
            {
                printf(" cant found title & finish menu!!\n");
                result = INFOMENU_ERROR;
            }
        }
    }
}

/* Creates an info_item for label (with an edit field if edit is set), in a shadowed label, 320 pixels wide. */
create_info_item(parent, label[], edit[])
{
    new args[128];
    new item = CreateLabel(parent);
    sprintf(args, "label_string = %s ; edit = %s", label, edit);
    AttachScript(item, "info_item", "info_item", args);
    SetPubVar(item, "pad_enable", PAD_CIRCLE);
    return item;
}

/* Same as run_edit_dialog of text.inc. */
run_edit_dialog2(title[], text[], edit_mode, maxlen, max_chars)
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
public focus; /* the focus of the message dialog */
public focus_glowfly; /* the focus object */
public db_status; /* -1 if the movie is not in the database */
public thumb; /* the thumbnail, an info_thumb plane */
new g_sql[256];
/* Scratch text: the fields of the database and the TEXT formats the items are built from. */
new g_text[128];
new g_text2[128];
new g_uri[256];
new g_attr[4]; /* content_attr, ATTR_* */
new g_date[16];
new g_album_name[64];

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

/* Waits for the length of the movie (movieGetTimeStart); returns 1 with it, -1 on an error. */
wait_movie_time(&hours, &minutes, &seconds)
{
    for (; ; )
    {
        new result = movieGetTimeSync(hours, minutes, seconds);
        if (0 != result)
        {
            return result;
        }
        sleep;
    }
    return 0;
}

/*
 * Opens the movie database, checks that the row of the window variables uri and attr is there (db_status -1 if it
 * is not) and shows the thumbnail of the movie, or the icon of the album.
 */
load_movie_info(side, result, mode, title[], group[], id[], &selected)
{
    new status;
    new count;
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
    get_wnd_var_str("uri", g_uri, 256);
    get_wnd_var_str("attr", g_attr, 4);
    sprintf(g_sql, "select * from sce_movie where content_attr = %d and uri = '%s'", atoi(g_attr), g_uri);
    status = datSQLExec(g_sql);
    if (0 != status)
    {
        return;
    }
    status = wait_database();
    if (status < 0)
    {
        return;
    }
    count = datGetNum();
    if (count <= 0)
    {
        db_status = -1;
        return;
    }
    thumb = CreatePlane(Disp2D(0));
    AttachScript(thumb, "info_thumb", "info_thumb");
    SetColor(thumb, 1.0, 1.0, 1.0, 0.0);
    switch (atoi(g_attr))
    {
        case ATTR_MOVIE:
        {
            new file_mode;
            new file_size;
            if (0 != fileLstat(g_text, file_mode, file_size))
            {
                get_tag_attr("IMG", "default_icon", "src", g_text, 128);
            }
            new thumb_data[256];
            moviedataReadStart(g_uri, 1, 3);
            if (wait_moviedata(thumb_data, 256) > 0)
            {
                SetTexture(thumb, thumb_data[0], 0, -1);
                SetLocalScale(thumb, 78.0 * 1.0, 78.0, 78.0);
                AnimColor(thumb, 1.0, 1.0, 1.0, 1.0, 1000.0, 0.0, ANIM_FUNC_LINEAR);
            }
            else
            {
                get_tag_attr("IMG", "default_icon", "src", g_text, 128);
                SendMessageStr(thumb, TAG("DCDE"), g_text);
            }
        }
        case ATTR_FAV_MOVIE:
        {
            new file_mode;
            new file_size;
            if (0 != fileLstat(g_text, file_mode, file_size))
            {
                get_tag_attr("IMG", "default_icon", "src", g_text, 128);
            }
            new thumb_data[256];
            moviedataReadStart(g_uri, 1, 3);
            if (wait_moviedata(thumb_data, 256) > 0)
            {
                SetTexture(thumb, thumb_data[0], 0, -1);
                SetLocalScale(thumb, 78.0 * 1.0, 78.0, 78.0);
                AnimColor(thumb, 1.0, 1.0, 1.0, 1.0, 1000.0, 0.0, ANIM_FUNC_LINEAR);
            }
            else
            {
                get_tag_attr("IMG", "default_icon", "src", g_text, 128);
                SendMessageStr(thumb, TAG("DCDE"), g_text);
            }
        }
        case ATTR_ALBUM:
        {
            get_tag_attr("IMG", "default_album_icon", "src", g_text, 128);
            SendMessageStr(thumb, TAG("DCDE"), g_text);
        }
        case ATTR_FAV_ALBUM:
        {
            get_tag_attr("IMG", "default_album_icon", "src", g_text, 128);
            SendMessageStr(thumb, TAG("DCDE"), g_text);
        }
    }
    return;
}

/*
 * Sends "INFO" to item with the value its id names, read from the row of the movie or album: columns 1 title,
 * 2 album name, 3 date, 4 uri, 5 album id, 8 content_attr; the size and the count of an album sum its movies.
 */
fill_info_item(manager, item, side, mode, index, group[], text[])
{
    new item_id[64];
    new title[256];
    new file_mode;
    new file_size;
    new total_kb;
    new count;
    new i;
    new status;
    if (db_status < 0)
    {
        return;
    }
    title = "";
    datGetData(0, COL_TITLE, title, 256);
    get_group_tag_attr2("TEXT", group, index, "id", item_id, 64);
    if (!strcmp("INFO_TITLE", item_id))
    {
        get_wnd_var_str("attr", g_attr, 4);
        if (atoi(g_attr) == ATTR_FAV_MOVIE)
        {
            datGetData(0, COL_TITLE, g_text, 128);
            datGetData(0, COL_ALBUM_NAME, g_text2, 128);
            if (strlen(g_text) <= 0 || strlen(g_text2) <= 0)
            {
                get_tag_attr("TEXT", "msgutil_info_datatype_broken", "value", g_text, 128);
                sprintf(text, "%s", g_text);
            }
            else
            {
                sprintf(text, "%s / %s", g_text, g_text2);
            }
        }
        else
        {
            datGetData(0, COL_TITLE, g_text, 128);
            if (strlen(g_text) <= 0)
            {
                get_tag_attr("TEXT", "msgutil_info_datatype_broken", "value", g_text, 128);
            }
            sprintf(text, "%s", g_text);
        }
        SendMessageStr(item, TAG("INFO"), text);
    }
    else if (!strcmp("INFO_ALBUM", item_id))
    {
        g_text = "";
        get_wnd_var_str("album_name", g_text, 128);
        if (strlen(g_text) <= 0)
        {
            get_tag_attr("TEXT", "msgutil_info_datatype_broken", "value", g_text, 128);
        }
        sprintf(text, "%s", g_text);
        SendMessageStr(item, TAG("INFO"), text);
    }
    else if (!strcmp("INFO_SIZE", item_id))
    {
        datGetData(0, COL_CONTENT_ATTR, g_text2, 128);
        switch (atoi(g_text2))
        {
            case ATTR_MOVIE:
            {
                datGetData(0, COL_URI, g_text, 128);
                fileLstat(g_text, file_mode, file_size);
                if (file_size < 0)
                {
                    get_tag_attr("TEXT", "msgutil_info_datatype_broken", "value", g_text, 128);
                    sprintf(text, "%s", g_text);
                }
                else if (file_size < 10240000)
                {
                    get_tag_attr("TEXT", "msgmovie_info_size_kiro", "value", g_text2, 128);
                    file_size = (file_size + 1023) / 1024;
                    sprintf(text, g_text2, file_size);
                }
                else if (file_size < 1895825408)
                {
                    get_tag_attr("TEXT", "msgmovie_info_size_mega1", "value", g_text2, 128);
                    file_size = (file_size + 1023) / 1024 / 1024;
                    sprintf(text, g_text2, file_size);
                }
                else
                {
                    get_tag_attr("TEXT", "msgmovie_info_size_mega2", "value", g_text2, 128);
                    file_size = (file_size + 1023) / 1024 / 1024;
                    sprintf(text, g_text2, file_size / 1000, file_size % 1000);
                }
            }
            case ATTR_FAV_MOVIE:
            {
                datGetData(0, COL_URI, g_text, 128);
                fileLstat(g_text, file_mode, file_size);
                if (file_size < 0)
                {
                    get_tag_attr("TEXT", "msgutil_info_datatype_broken", "value", g_text, 128);
                    sprintf(text, "%s", g_text);
                }
                else if (file_size < 10240000)
                {
                    get_tag_attr("TEXT", "msgmovie_info_size_kiro", "value", g_text2, 128);
                    file_size = (file_size + 1023) / 1024;
                    sprintf(text, g_text2, file_size);
                }
                else if (file_size < 1895825408)
                {
                    get_tag_attr("TEXT", "msgmovie_info_size_mega1", "value", g_text2, 128);
                    file_size = (file_size + 1023) / 1024 / 1024;
                    sprintf(text, g_text2, file_size);
                }
                else
                {
                    get_tag_attr("TEXT", "msgmovie_info_size_mega2", "value", g_text2, 128);
                    file_size = (file_size + 1023) / 1024 / 1024;
                    sprintf(text, g_text2, file_size / 1000, file_size % 1000);
                }
            }
            case ATTR_ALBUM:
            {
                datGetData(0, COL_URI, g_text, 128);
                sprintf(g_sql, "select * from sce_movie where content_attr = %d and uri like '%s%s'", ATTR_MOVIE,
                        g_text, "%");
                status = datSQLExec(g_sql);
                if (0 != status)
                {
                    return;
                }
                status = wait_database();
                if (status < 0)
                {
                    return;
                }
                count = datGetNum();
                total_kb = 0;
                i = 0;
                for (; i < count; i++)
                {
                    datGetData(i, COL_URI, g_text, 128);
                    fileLstat(g_text, file_mode, file_size);
                    total_kb = total_kb + (file_size + 1023) / 1024;
                }
                if (total_kb < 10000)
                {
                    get_tag_attr("TEXT", "msgmovie_info_size_kiro", "value", g_text2, 128);
                    sprintf(text, g_text2, total_kb);
                }
                else if (total_kb < 10240000)
                {
                    get_tag_attr("TEXT", "msgmovie_info_size_mega1", "value", g_text2, 128);
                    total_kb = total_kb / 1024;
                    sprintf(text, g_text2, total_kb);
                }
                else
                {
                    get_tag_attr("TEXT", "msgmovie_info_size_mega2", "value", g_text2, 128);
                    total_kb = total_kb / 1024;
                    sprintf(text, g_text2, total_kb / 1000, total_kb % 1000);
                }
                get_wnd_var_str("uri", g_uri, 256);
                get_wnd_var_str("attr", g_attr, 4);
                sprintf(g_sql, "select * from sce_movie where content_attr = %d and uri = '%s'", atoi(g_attr), g_uri);
                status = datSQLExec(g_sql);
                if (0 != status)
                {
                    return;
                }
                status = wait_database();
                if (status < 0)
                {
                    return;
                }
                new rows = datGetNum();
                if (rows <= 0)
                {
                    db_status = -1;
                    return;
                }
            }
            case ATTR_FAV_ALBUM:
            {
                datGetData(0, COL_ALBUM_ID, g_text, 128);
                sprintf(g_sql, "select * from sce_movie where content_attr = %d and album_id = '%s'", ATTR_FAV_MOVIE,
                        g_text);
                status = datSQLExec(g_sql);
                if (0 != status)
                {
                    return;
                }
                status = wait_database();
                if (status < 0)
                {
                    return;
                }
                count = datGetNum();
                total_kb = 0;
                i = 0;
                for (; i < count; i++)
                {
                    datGetData(i, COL_URI, g_text, 128);
                    fileLstat(g_text, file_mode, file_size);
                    if (file_size < 0)
                    {
                        file_size = 0;
                    }
                    total_kb = total_kb + (file_size + 1023) / 1024;
                }
                if (total_kb < 10000)
                {
                    get_tag_attr("TEXT", "msgmovie_info_size_kiro", "value", g_text2, 128);
                    sprintf(text, g_text2, total_kb);
                }
                else if (total_kb < 10240000)
                {
                    get_tag_attr("TEXT", "msgmovie_info_size_mega1", "value", g_text2, 128);
                    total_kb = total_kb / 1024;
                    sprintf(text, g_text2, total_kb);
                }
                else
                {
                    get_tag_attr("TEXT", "msgmovie_info_size_mega2", "value", g_text2, 128);
                    total_kb = total_kb / 1024;
                    sprintf(text, g_text2, total_kb / 1000, total_kb % 1000);
                }
                get_wnd_var_str("uri", g_uri, 256);
                get_wnd_var_str("attr", g_attr, 4);
                sprintf(g_sql, "select * from sce_movie where content_attr = %d and uri = '%s'", atoi(g_attr), g_uri);
                status = datSQLExec(g_sql);
                if (0 != status)
                {
                    return;
                }
                status = wait_database();
                if (status < 0)
                {
                    return;
                }
                new rows = datGetNum();
                if (rows <= 0)
                {
                    db_status = -1;
                    return;
                }
            }
        }
        SendMessageStr(item, TAG("INFO"), text);
    }
    else if (!strcmp("INFO_DATE", item_id))
    {
        g_text = "";
        g_text2 = "";
        datGetData(0, COL_CREATE_DATE, g_text, 128);
        if (strlen(g_text) <= 0)
        {
            get_tag_attr("TEXT", "msggame_unknown", "value", g_text, 128);
            sprintf(text, "%s", g_text);
        }
        else
        {
            new year;
            new month;
            new day;
            new hour;
            new minute;
            new second;
            err10031(g_text, year, month, day, hour, minute, second);
            new date_notation;
            date_notation = GetSystemParam(SYS_DATE_NOTATION);
            switch (date_notation)
            {
                case DATE_YYYYMMDD:
                {
                    sprintf(g_text, "%04d/%02d/%02d", year, month, day);
                }
                case DATE_MMDDYYYY:
                {
                    sprintf(g_text, "%02d/%02d/%04d", month, day, year);
                }
                case DATE_DDMMYYYY:
                {
                    sprintf(g_text, "%02d/%02d/%04d", day, month, year);
                }
            }
            new twelve_hour;
            twelve_hour = GetSystemParam(SYS_TIME_NOTATION);
            if (twelve_hour)
            {
                if (hour >= 12 && hour < 24)
                {
                    sprintf(g_text2, "%02d:%02d:%02d PM", (hour - 1) % 12 + 1, minute, second);
                }
                else
                {
                    sprintf(g_text2, "%02d:%02d:%02d AM", (hour - 1) % 12 + 1, minute, second);
                }
            }
            else
            {
                sprintf(g_text2, "%02d:%02d:%02d", hour, minute, second);
            }
            sprintf(text, "%s %s", g_text, g_text2);
        }
        SendMessageStr(item, TAG("INFO"), text);
    }
    else if (!strcmp("INFO_PTIME", item_id))
    {
        datGetData(0, COL_URI, g_text, 128);
        new hours;
        new minutes;
        new seconds;
        hours = 0;
        minutes = 0;
        seconds = 0;
        movieGetTimeStart(g_text);
        if (wait_movie_time(hours, minutes, seconds) > 0)
        {
            sprintf(text, "%02d:%02d:%02d", hours, minutes, seconds);
        }
        else
        {
            sprintf(text, "--:--:--");
        }
        SendMessageStr(item, TAG("INFO"), text);
    }
    else if (!strcmp("INFO_CONT", item_id))
    {
        datGetData(0, COL_CONTENT_ATTR, g_text2, 128);
        switch (atoi(g_text2))
        {
            case ATTR_ALBUM:
            {
                datGetData(0, COL_URI, g_text, 128);
                sprintf(g_sql, "select * from sce_movie where content_attr = %d and uri like '%s%s'", ATTR_MOVIE,
                        g_text, "%");
            }
            case ATTR_FAV_ALBUM:
            {
                datGetData(0, COL_ALBUM_ID, g_text, 128);
                sprintf(g_sql, "select * from sce_movie where content_attr = %d and album_id = '%s'", ATTR_FAV_MOVIE,
                        g_text);
            }
        }
        datSQLExec(g_sql);
        wait_database();
        sprintf(text, "%d", datGetNum());
        SendMessageStr(item, TAG("INFO"), text);
    }
    return;
}

new g_renamed;

/*
 * Handles a push of item index: the circle on the title item asks for a new name with the software keyboard until
 * one is entered or the edit is cancelled, and writes it to the database (a movie also in its favorite copy, an
 * album also as the album name of its movies); the cross closes the menu, with INFOMENU_RENAMED after a rename.
 */
handle_push(manager, item, side, &mode, index, button, title[], group[], id[])
{
    new result = INFOMENU_REOPEN;
    new status;
    if (db_status < 0)
    {
        g_renamed = 0;
        return INFOMENU_ERROR;
    }
    if (button == TAG("RRGT"))
    {
        switch (mode)
        {
            case INFOMENU_MODE_GROUP:
            {
            }
        }
        mode = INFOMENU_MODE_GROUP;
        result = INFOMENU_STAY;
        if (!strcmp("INFO_TITLE", id))
        {
            for (; ; )
            {
                new label[128];
                get_group_tag_attr2("TEXT", group, index, "value", label, 128);
                get_wnd_var_str("uri", g_uri, 256);
                get_wnd_var_str("attr", g_attr, 4);
                sprintf(g_sql, "select * from sce_movie where content_attr = %d and uri = '%s'", atoi(g_attr), g_uri);
                status = datSQLExec(g_sql);
                if (0 != status)
                {
                    return 0;
                }
                status = wait_database();
                if (status < 0)
                {
                    return 0;
                }
                new rows = datGetNum();
                if (rows <= 0)
                {
                    db_status = -1;
                    return 0;
                }
                datGetData(0, COL_URI, g_text, 128);
                g_date = "";
                datGetData(0, COL_CREATE_DATE, g_date, 16);
                g_text2 = "";
                datGetData(0, COL_TITLE, g_text2, 128);
                if (run_edit_dialog2(label, g_text2, 0, 32, 256))
                {
                    if (strlen(g_text2) > 0)
                    {
                        escape_sql(label, g_text2, 128);
                        status = datOpen(DAT_DATABASE, g_db_name, DAT_ORDER_MTIME_DESC);
                        if (0 != status)
                        {
                            g_renamed = 0;
                            return INFOMENU_ERROR;
                        }
                        status = wait_database();
                        if (status < 0)
                        {
                            g_renamed = 0;
                            return INFOMENU_ERROR;
                        }
                        switch (atoi(g_attr))
                        {
                            case ATTR_MOVIE:
                            {
                                sprintf(g_sql,
                                        "update sce_movie set title = '%s' where content_attr = %d and uri = '%s'",
                                        label, atoi(g_attr), g_text);
                                status = datSQLExec(g_sql);
                                if (0 != status)
                                {
                                    g_renamed = 0;
                                    return INFOMENU_ERROR;
                                }
                                status = wait_database();
                                if (status < 0)
                                {
                                    g_renamed = 0;
                                    return INFOMENU_ERROR;
                                }
                                sprintf(g_sql,
                                        "update sce_movie set title = '%s' where content_attr = %d and uri = '%s'",
                                        label, ATTR_FAV_MOVIE, g_text);
                                status = datSQLExec(g_sql);
                                if (0 != status)
                                {
                                    g_renamed = 0;
                                    return INFOMENU_ERROR;
                                }
                                status = wait_database();
                                if (status < 0)
                                {
                                    g_renamed = 0;
                                    return INFOMENU_ERROR;
                                }
                            }
                            case ATTR_ALBUM:
                            {
                                sprintf(g_sql,
                                        "update sce_movie set title = '%s',album_name = '%s' where content_attr = %d and uri = '%s'",
                                        label, label, atoi(g_attr), g_text);
                                status = datSQLExec(g_sql);
                                if (0 != status)
                                {
                                    g_renamed = 0;
                                    return INFOMENU_ERROR;
                                }
                                status = wait_database();
                                if (status < 0)
                                {
                                    g_renamed = 0;
                                    return INFOMENU_ERROR;
                                }
                                sprintf(g_sql,
                                        "update sce_movie set album_name = '%s' where content_attr = %d and uri like '%s%s'",
                                        label, ATTR_MOVIE, g_text, "%");
                                status = datSQLExec(g_sql);
                                if (0 != status)
                                {
                                    g_renamed = 0;
                                    return INFOMENU_ERROR;
                                }
                                status = wait_database();
                                if (status < 0)
                                {
                                    g_renamed = 0;
                                    return INFOMENU_ERROR;
                                }
                                sprintf(g_sql,
                                        "update sce_movie set album_name = '%s' where content_attr = %d and uri like '%s%s'",
                                        label, ATTR_FAV_MOVIE, g_text, "%");
                                status = datSQLExec(g_sql);
                                if (0 != status)
                                {
                                    g_renamed = 0;
                                    return INFOMENU_ERROR;
                                }
                                status = wait_database();
                                if (status < 0)
                                {
                                    g_renamed = 0;
                                    return INFOMENU_ERROR;
                                }
                            }
                            case ATTR_FAV_ALBUM:
                            {
                                sprintf(g_sql,
                                        "update sce_movie set title = '%s',album_name = '%s' where content_attr = %d and uri = '%s'",
                                        label, label, atoi(g_attr), g_text);
                                status = datSQLExec(g_sql);
                                if (0 != status)
                                {
                                    g_renamed = 0;
                                    return INFOMENU_ERROR;
                                }
                                status = wait_database();
                                if (status < 0)
                                {
                                    g_renamed = 0;
                                    return INFOMENU_ERROR;
                                }
                            }
                        }
                        SendMessageStr(item, TAG("INFO"), g_text2);
                        set_wnd_var_str("TitleName", g_text2);
                        g_renamed = 1;
                        break;
                    }
                    else
                    {
                        g_renamed = 0;
                        new msg[64];
                        get_tag_attr("TEXT", "msgmovie_input_fail_noname", "value", msg, 64);
                        show_message(msg);
                    }
                }
                else
                {
                    break;
                }
            }
        }
    }
    if (button == TAG("RDWN"))
    {
        switch (mode)
        {
            case INFOMENU_MODE_GROUP:
            {
                if (thumb)
                {
                    SendMessage(thumb, TAG("FNSH"), 0, 0, 0, 0);
                }
                mode = INFOMENU_MODE_GROUP;
                if (g_renamed)
                {
                    result = INFOMENU_RENAMED;
                }
                else
                {
                    result = INFOMENU_CLOSE;
                }
            }
        }
    }
    return result;
}

/* Shows msg in a dialog and waits until it is closed. */
show_message(msg[])
{
    new args[64];
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    sprintf(args, "msg=%s", msg);
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

public main()
{
    new result = INFOMENU_CLOSE;
    focus_glowfly = create_focus_plane2("focus");
    GetArg("uri", g_uri, 256);
    set_wnd_var_str("uri", g_uri);
    GetArg("attr", g_attr, 4);
    set_wnd_var_str("attr", g_attr);
    GetArg("album_name", g_album_name, 64);
    set_wnd_var_str("album_name", g_album_name);
    switch (atoi(g_attr))
    {
        case ATTR_MOVIE:
        {
            result = pop_info_menu("menu_title", "info", INFOMENU_SIDE_LEFT);
        }
        case ATTR_FAV_MOVIE:
        {
            result = pop_info_menu("menu_title", "fav_info", INFOMENU_SIDE_LEFT);
        }
        case ATTR_ALBUM:
        {
            result = pop_info_menu("menu_title", "album_info", INFOMENU_SIDE_LEFT);
        }
        case ATTR_FAV_ALBUM:
        {
            result = pop_info_menu("menu_title", "favalbum_info", INFOMENU_SIDE_LEFT);
        }
    }
    switch (result)
    {
        case INFOMENU_RENAMED:
        {
            avalSet("InfoStatus", "INVALID");
            get_wnd_var_str("TitleName", g_text, 128);
            avalSet("TitleName", g_text);
        }
        case INFOMENU_CLOSE:
        {
            avalSet("InfoStatus", "NONE");
        }
        case INFOMENU_ERROR:
        {
            avalSet("InfoStatus", "ERROR");
        }
        default:
        {
            avalSet("InfoStatus", "ERROR");
        }
    }
    Destroy(Scene(0));
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

#include "button.inc"
#include "bgmode.inc"
#include "date.inc"
