/*
 * /opt0/bn/script/music/infomenu.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The information page of an album or a track of the music pages: a general_menu of the TEXT elements of a group of
 * the page (name, artist, length, size, format, ...), filled from the values the music pages left in the aval file.
 * The name items open the software keyboard to rename the song, the album or the artist.
 */

#include <core>
#include <bn>

#pragma dynamic 2048

#include "infomenu.inc"
#include "genmenu.inc"
#include "scroll.inc"

#include "musicdb.inc"

#include "player.inc"

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

new Float:g_screen_width6;
new Float:g_screen_height6;
new Float:g_screen_depth6;

/* Same as get_screen_size of text.inc, with globals of its own. */
get_screen_size6(&Float:screen_w, &Float:screen_h)
{
    GetScreen(Camera(0), g_screen_width6, g_screen_height6, g_screen_depth6);
    screen_w = g_screen_width6;
    screen_h = g_screen_height6;
}

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
    get_screen_size6(screen_w, screen_h);
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
    item_count = item_count + load_music_info(side, result, mode, title, group, id, selected);
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
    get_screen_size6(screen_w, screen_h);
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
        AttachScript(menu_obj, "info_menu_core", _, args);
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

/* Same as run_edit_dialog of text.inc, with bg_bytes split in two arrays. */
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
        static bg_bytes[2];
        pack_color_bytes(bg_colors, bg_bytes);
        static bg_bytes_rest[14]; /* unused, but part of the layout */
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
/* The group of the menu, by MusicChannelMode + 2 * editmode (0: editable, 1: read only, 2: only the album name). */
new g_info_groups[6][] = {
    "album_info", "track_info", "album_info_noedit", "track_info_noedit", "album_info_editalbum",
    "track_info_editalbum"
};
public focus; /* the focus of the message dialog */
public focus_glowfly; /* the focus object */
public select = -1; /* unused, but part of the layout */
public thumb; /* the icon of the album or the track, an info_thumb plane */
new g_text[256]; /* the value of an item */
new g_format[256];

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
 * Arguments: editmode (see g_info_groups), media_watcher (1: watch the disc, for an audio CD), select. Leaves
 * MusicInfoEdit 1 in the aval file when a name was changed.
 */
public main()
{
    new arg[64];
    new channel_mode;
    channel_mode = get_aval_int("MusicChannelMode");
    new album_mode = MUSIC_ALBUM_COLLECTION;
    if (GetArg("album_mode", arg, 64))
    {
        album_mode = atoi(arg);
    }
    avalSet("MusicMediaChanged", "0");
    if (GetArg("media_watcher", arg, 64))
    {
        new watch = atoi(arg);
        if (watch)
        {
            printf("========= MEDIA_WATCHER START\n");
            new args[256];
            sprintf(args, "owner = %d", Self());
            new watcher = CreateNull(Self());
            AttachScript(watcher, "media_watcher_main", _, args);
        }
    }
    new select_arg[32];
    if (GetArg("select", select_arg, 32))
    {
        select = atoi(select_arg);
    }
    focus_glowfly = create_focus_plane2("focus");
    new edit_mode;
    if (GetArg("editmode", arg, 64))
    {
        edit_mode = atoi(arg);
    }
    printf("============== editmode = %d\n", edit_mode);
    new result = pop_info_menu("menu_title", g_info_groups[channel_mode + edit_mode * 2], INFOMENU_SIDE_LEFT);
    if (result == INFOMENU_RENAMED)
    {
        printf("info invalid return!!!!!!!!!!!!!!!!!!!!!!\n");
        avalSet("MusicInfoEdit", "1");
    }
    else
    {
        avalSet("MusicInfoEdit", "0");
    }
    Destroy(Scene(0));
}

/* Returns the type of the disc in the drive: no disc, detecting, an audio CD or CDVD_ILLEGAL for any other. */
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
    if (type == CDVD_CDDA)
    {
        return CDVD_CDDA;
    }
    return CDVD_ILLEGAL;
}

/* Sets MusicMediaChanged in the aval file when the disc in the drive changes. */
public media_watcher_main()
{
    new owner;
    new arg[64];
    if (GetArg("owner", arg, 64))
    {
        owner = atoi(arg);
    }
    new old_type = get_disc_type();
    for (; ; )
    {
        new type = get_disc_type();
        if (type != old_type)
        {
            printf("================= CHANGED \n");
            avalSet("MusicMediaChanged", "1");
            old_type = type;
        }
        wait_ms(500);
    }
    return;
}

/* The TEXT ids of the recording quality and of the bitrate, for BITRATE_132K, BITRATE_105K and BITRATE_66K. */
new g_quality_names[3][] = {
    "msgmusic_settting_rec_quality_high", "msgmusic_settting_rec_quality_normal",
    "msgmusic_settting_rec_quality_low"
};
new g_bitrate_names[3][5] = {"msgmusic_tinfo_132k", "msgmusic_tinfo_105k", "msgmusic_tinfo_66k"};

/*
 * Sends "INFO" to item with the value its id names, from the aval file: the names, the length (without the hours
 * when they are 00), the size (also of the compressed copy of a track), the format, the quality and the number of
 * check-outs; "-" for what the album does not have.
 */
fill_info_item(manager, item, side, mode, index, group[], text[])
{
    if (0 != mode)
    {
        return;
    }
    new item_id[64];
    get_group_tag_attr2("TEXT", group, index, "id", item_id, 64);
    new checkout_times = get_aval_int("CheckoutTimes");
    new code;
    new album_mode = get_aval_int("MusicAlbumMode");
    new channel_mode = get_aval_int("MusicChannelMode");
    g_text = "-";
    new value[256];
    if (!strcmp("info_songname", item_id))
    {
        avalGetValue("SongName", 0, value, 256);
        copy_string(g_text, value, 256);
    }
    else if (!strcmp("info_albumname", item_id))
    {
        avalGetValue("MusicAlbumName", 0, value, 256);
        copy_string(g_text, value, 256);
    }
    else if (!strcmp("info_artist", item_id))
    {
        if (album_mode == MUSIC_ALBUM_PD || (MUSIC_ALBUM_FAVORITE == album_mode && CHANNEL_ALBUM == channel_mode))
        {
            sprintf(g_text, "-");
        }
        else
        {
            avalGetValue("ArtistName", 0, value, 256);
            copy_string(g_text, value, 256);
        }
    }
    else if (!strcmp("info_year", item_id))
    {
        sprintf(g_text, "-");
    }
    else if (!strcmp("info_genre", item_id))
    {
        sprintf(g_text, "-");
    }
    else if (!strcmp("info_checkout", item_id))
    {
        if (album_mode == MUSIC_ALBUM_PD)
        {
            sprintf(g_text, "-");
        }
        else if (album_mode == MUSIC_ALBUM_CD)
        {
            sprintf(g_text, "-");
        }
        else if (checkout_times < 0)
        {
            sprintf(g_text, "-");
        }
        else
        {
            get_tag_attr("TEXT", "msgmusic_info_playcnt_n", "value", g_format, 256);
            sprintf(g_text, g_format, checkout_times);
        }
    }
    else if (!strcmp("info_length", item_id))
    {
        avalGetValue("Length", 0, value, 256);
        if (value{0} == '0' && value{1} == '0')
        {
            copy_chars(g_text, value, 3, -1);
        }
        else
        {
            copy_string(g_text, value, 256);
        }
    }
    else if (!strcmp("info_datasize", item_id))
    {
        if (album_mode == MUSIC_ALBUM_CD || album_mode == MUSIC_ALBUM_COLLECTION || MUSIC_ALBUM_FAVORITE == album_mode)
        {
            new size = get_aval_int("DataSize");
            if (size >= 10000)
            {
                size = size / 1024;
                get_tag_attr("TEXT", "msgmusic_tinfo_size_MB", "value", g_format, 256);
            }
            else
            {
                get_tag_attr("TEXT", "msgmusic_tinfo_size_KB", "value", g_format, 256);
            }
            sprintf(g_text, g_format, size);
            if (checkout_times < 0 || CHANNEL_ALBUM == channel_mode)
            {
            }
            else
            {
                new mqt_size = get_aval_int("MQTDataSize");
                if (mqt_size >= 10000)
                {
                    mqt_size = mqt_size / 1024;
                    get_tag_attr("TEXT", "msgmusic_tinfo_size_MB", "value", g_format, 256);
                }
                else
                {
                    get_tag_attr("TEXT", "msgmusic_tinfo_size_KB", "value", g_format, 256);
                }
                new mqt_text[256];
                sprintf(mqt_text, g_format, mqt_size);
                get_tag_attr("TEXT", "msgmusic_tinfo_slash", "value", g_format, 256);
                append_string(g_text, g_format, 256);
                append_string(g_text, mqt_text, 256);
            }
        }
        else
        {
            sprintf(g_text, "-");
        }
    }
    else if (!strcmp("info_format", item_id))
    {
        if (album_mode == MUSIC_ALBUM_PD)
        {
            /* The ATRAC formats of a track of the portable device: -7 mono, -6 stereo, -3 and -4 LP2, -5 LP4. */
            new format_text[16];
            avalGetValue("Format", 0, format_text, 16);
            code = atoi(format_text);
            if (code == -7)
            {
                get_tag_attr("TEXT", "msgmusic_tinfo_filetype_atrac", "value", g_text, 256);
                get_tag_attr("TEXT", "msgmusic_tinfo_format_mono", "value", g_format, 256);
            }
            else if (code == -6)
            {
                get_tag_attr("TEXT", "msgmusic_tinfo_filetype_atrac", "value", g_text, 256);
                get_tag_attr("TEXT", "msgmusic_tinfo_format_stereo", "value", g_format, 256);
            }
            else
            {
                get_tag_attr("TEXT", "msgmusic_tinfo_format_atrac", "value", g_text, 256);
                if (code == -3 || code == -4)
                {
                    get_tag_attr("TEXT", "msgmusic_tinfo_format_lp2", "value", g_format, 256);
                }
                else if (code == -5)
                {
                    get_tag_attr("TEXT", "msgmusic_tinfo_format_lp4", "value", g_format, 256);
                }
            }
            append_string(g_text, g_format, 256);
        }
        else if (album_mode == MUSIC_ALBUM_CD)
        {
            get_tag_attr("TEXT", "msgmusic_tinfo_format_pcm", "value", g_text, 256);
        }
        else if (checkout_times < 0)
        {
            get_tag_attr("TEXT", "msgmusic_tinfo_format_pcm", "value", g_text, 256);
        }
        else
        {
            get_tag_attr("TEXT", "msgmusic_tinfo_format_pcm", "value", g_text, 256);
            code = get_aval_int("Bitrate");
            get_tag_attr("TEXT", "msgmusic_tinfo_slash", "value", g_format, 256);
            append_string(g_text, g_format, 256);
            get_tag_attr("TEXT", "msgmusic_tinfo_format_atrac", "value", g_format, 256);
            append_string(g_text, g_format, 256);
            if (code == BITRATE_132K)
            {
                get_tag_attr("TEXT", g_bitrate_names[0], "value", g_format, 256);
            }
            else if (code == BITRATE_105K)
            {
                get_tag_attr("TEXT", g_bitrate_names[1], "value", g_format, 256);
            }
            else if (code == BITRATE_66K)
            {
                get_tag_attr("TEXT", g_bitrate_names[2], "value", g_format, 256);
            }
            append_string(g_text, g_format, 256);
        }
    }
    else if (!strcmp("info_quality", item_id))
    {
        if (album_mode == MUSIC_ALBUM_PD)
        {
            sprintf(g_text, "-");
        }
        else if (album_mode == MUSIC_ALBUM_CD)
        {
            sprintf(g_text, "-");
        }
        else if (checkout_times < 0)
        {
            sprintf(g_text, "-");
        }
        else
        {
            code = get_aval_int("Bitrate");
            if (code == BITRATE_132K)
            {
                get_tag_attr("TEXT", g_quality_names[0], "value", g_text, 256);
            }
            else if (code == BITRATE_105K)
            {
                get_tag_attr("TEXT", g_quality_names[1], "value", g_text, 256);
            }
            else if (code == BITRATE_66K)
            {
                get_tag_attr("TEXT", g_quality_names[2], "value", g_text, 256);
            }
            else
            {
                sprintf(g_text, "-");
            }
        }
    }
    SendMessageStr(item, TAG("INFO"), g_text);
}

new g_icon_url[64];

/* Shows the default icon of a track or an album; adds no menu items. */
load_music_info(side, result, mode, title[], group[], id[], &selected)
{
    thumb = CreatePlane(Disp2D(0));
    AttachScript(thumb, "info_thumb", "info_thumb");
    SetColor(thumb, 1.0, 1.0, 1.0, 0.0);
    new channel_mode = get_aval_int("MusicChannelMode");
    if (channel_mode == CHANNEL_TRACK)
    {
        get_tag_attr("IMG", "default_icon_track", "src", g_icon_url, 64);
    }
    else
    {
        get_tag_attr("IMG", "default_icon_album", "src", g_icon_url, 64);
    }
    SendMessageStr(thumb, TAG("DCDE"), g_icon_url);
}

new g_renamed; /* a name was changed */

/* Copies the characters first to last of the packed string src into dest; -1 or out of range means the last one. */
copy_chars(dest[], src[], first, last)
{
    new len = strlen(src);
    if (first < 0 || first >= len)
    {
        first = len - 1;
    }
    if (last < 0 || last >= len)
    {
        last = len - 1;
    }
    new out;
    for (new i = first; i <= last; i++)
    {
        dest{out++} = src{i};
    }
    dest{out++} = 0;
}

/*
 * Handles a push of item index: the circle on a name item asks for a new name with the software keyboard until one
 * is entered or the edit is cancelled, and keeps it in the aval file; the cross closes the menu, with
 * INFOMENU_RENAMED after a rename.
 */
handle_push(manager, item, side, &mode, index, button, title[], group[], id[])
{
    new result = INFOMENU_REOPEN;
    if (button == TAG("RRGT"))
    {
        printf("infomenu btn RRGT\n");
        mode = INFOMENU_MODE_GROUP;
        result = INFOMENU_STAY;
        new label[128];
        new name[128];
        if (!strcmp("info_songname", id))
        {
            edit_song_name:
            get_group_tag_attr2("TEXT", group, index, "value", label, 128);
            avalGetValue("SongName", 0, name, 128);
            if (run_edit_dialog2(label, name, 0, 256, 256))
            {
                printf("select info_songname %s!!\n", name);
                if (strlen(name) > 0)
                {
                    avalSet("SongName", name);
                    SendMessageStr(item, TAG("INFO"), name);
                    g_renamed = 1;
                }
                else
                {
                    if (1 != g_renamed)
                    {
                        g_renamed = 0;
                    }
                    get_tag_attr("TEXT", "msgmusic_input_fail_noname", "value", name, 128);
                    show_message(name);
                    goto edit_song_name;
                }
            }
        }
        else if (!strcmp("info_albumname", id))
        {
            edit_album_name:
            get_group_tag_attr2("TEXT", group, index, "value", label, 128);
            avalGetValue("MusicAlbumName", 0, name, 128);
            if (run_edit_dialog2(label, name, 0, 256, 256))
            {
                printf("select info_albumname %s!!\n", name);
                if (strlen(name) > 0)
                {
                    avalSet("MusicAlbumName", name);
                    SendMessageStr(item, TAG("INFO"), name);
                    g_renamed = 1;
                }
                else
                {
                    if (1 != g_renamed)
                    {
                        g_renamed = 0;
                    }
                    get_tag_attr("TEXT", "msgmusic_input_fail_noname", "value", name, 128);
                    show_message(name);
                    goto edit_album_name;
                }
            }
        }
        else if (!strcmp("info_artist", id))
        {
            edit_artist_name:
            get_group_tag_attr2("TEXT", group, index, "value", label, 128);
            avalGetValue("ArtistName", 0, name, 128);
            if (run_edit_dialog2(label, name, 0, 256, 256))
            {
                printf("select info_artist %s!!\n", name);
                if (strlen(name) > 0)
                {
                    avalSet("ArtistName", name);
                    SendMessageStr(item, TAG("INFO"), name);
                    g_renamed = 1;
                }
                else
                {
                    if (1 != g_renamed)
                    {
                        g_renamed = 0;
                    }
                    get_tag_attr("TEXT", "msgmusic_input_fail_noname", "value", name, 128);
                    show_message(name);
                    goto edit_artist_name;
                }
            }
        }
    }
    if (button == TAG("RDWN"))
    {
        printf("infomenu btn RDWN\n");
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
    return result;
}

/* Shows msg in a dialog and waits until it is closed. */
show_message(msg[])
{
    new args[512];
    sprintf(args, "msg=%s", msg);
    new Float:screen_w;
    new Float:screen_h;
    get_screen_size6(screen_w, screen_h);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 0.5 * screen_w, 0.5 * screen_h, 0.0, 0);
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

#include "playerctl.inc"
