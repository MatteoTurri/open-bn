/*
 * /opt0/bn/script/photo/infomenu.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The information page of a photo or an album (argument mode INFO_THUMB, INFO_USBTMB or INFO_ALBUM, select the
 * index of the photo or album): a general_menu of the TEXT elements of its group, filled from the photo database or
 * the files of a USB device, with the thumbnail. The title or the album name can be changed with the keyboard.
 */

#include <core>
#include <bn>

#pragma dynamic 2560

#include "infomenu.inc"
#include "genmenu.inc"
#include "scroll.inc"

#include "photo.inc"

/*
 * The kinds of album (the window variable "albummode", album_kind of get_photo_field): a list of albums (ALBUM_*) or
 * the photos of an album of that kind (PHOTOS_* = ALBUM_* + PHOTOS_LIBRARY).
 */
#include "photodb.inc"

/* As in text.inc. */
const EDIT_MODE_USER_ID = 1;
const EDIT_MODE_PASSWORD = 2;
const EDIT_MODE_KATAKANA = 3;
const EDIT_MODE_NUMBER = 4;
const EDIT_MODE_KATAKANA_FIXED = 7;
const EDIT_MODE_QWERTY = 9;

/* How split_size splits a size. */
enum
{
    SIZE_MB_THOUSANDS = 1,
    SIZE_MB,
    SIZE_KB_THOUSANDS,
    SIZE_KB
}

/* Same as wait_ms of wait.inc. */
wait_ms(ms)
{
    new frames = ms * FrameRate() / 1000;
    for (new i; i < frames; i++)
    {
        sleep;
    }
}

/* Same as TAG of tag.inc. */
TAG(fourcc[])
{
    return swapchars(fourcc[0]);
}

/* Same as pack_color_bytes of tag.inc. */
pack_color_bytes(packed[], bytes[])
{
    for (new i; i < 4; i++)
    {
        for (new j; j < 4; j++)
        {
            packed{i * 4 + 3 - j} = bytes[i * 4 + j];
        }
    }
}

#include <float>

/* Same as wait_image_decode of imgsync.inc. */
wait_image_decode()
{
    for (; ; )
    {
        new status = imgDecSync();
        if (IMG_BUSY != status)
        {
            return status;
        }
        sleep;
    }
    return 0;
}

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

/* Same as wait_skb of xmlsync.inc. */
wait_skb()
{
    new status;
    for (; ; )
    {
        status = skbSync();
        if (0 != status)
        {
            return status;
        }
        sleep;
    }
    return 0;
}

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

/* Same as count_group_tags of text.inc. */
count_group_tags(tag[], group[])
{
    new count;
    new dom_root = domRoot();
    for (new node = domFirstChild(dom_root, tag); node; node = domNext(node, tag))
    {
        new name[64];
        domGetData(node, "group", name, 64);
        if (0 == strcmp(name, group))
        {
            count++;
        }
    }
    return count;
}

/* Same as get_group_tag_attr of text.inc. */
get_group_tag_attr(tag[], group[], index, attr[], value[], maxlen)
{
    new count;
    new dom_root = domRoot();
    for (new node = domFirstChild(dom_root, tag); node; node = domNext(node, tag))
    {
        new name[64];
        domGetData(node, "group", name, 64);
        if (0 == strcmp(name, group))
        {
            if (count == index)
            {
                new len = domGetData(node, attr, value, maxlen);
                if (0 == strcmp(attr, "src") || 0 == strcmp(attr, "href"))
                {
                    xmlMakeFullPath(value, maxlen);
                }
                return len;
            }
            count++;
        }
    }
    return 0;
}

/* Same as set_wnd_var_int of text.inc. */
set_wnd_var_int(name[], value)
{
    new vars = domFindTag(domRoot(), "__wndvar__", _, 0);
    if (!vars)
    {
        vars = domInsertTag(domFirstChild(domRoot()), "__wndvar__");
    }
    new node = domFindTag(vars, "__wndvar_int__", name, 0);
    if (!node)
    {
        node = domAddTag(vars, "__wndvar_int__");
        domSetData(node, "id", name);
    }
    new text[8];
    sprintf(text, "%d", value);
    domSetData(node, "value", text);
}

/* Same as get_wnd_var_int of wndvar.inc. */
get_wnd_var_int(name[], &value)
{
    new vars = domFindTag(domRoot(), "__wndvar__", _, 0);
    if (!vars)
    {
        return 0;
    }
    new node = domFindTag(vars, "__wndvar_int__", name, 0);
    if (!node)
    {
        return 0;
    }
    new text[8];
    domGetData(node, "value", text, 8);
    value = atoi(text);
    return 1;
}

/* Same as set_wnd_var_str of wndvar.inc. */
set_wnd_var_str(name[], value[])
{
    new vars = domFindTag(domRoot(), "__wndvar__", _, 0);
    if (!vars)
    {
        vars = domInsertTag(domFirstChild(domRoot()), "__wndvar__");
    }
    new node = domFindTag(vars, "__wndvar_str__", name, 0);
    if (!node)
    {
        node = domAddTag(vars, "__wndvar_str__");
        domSetData(node, "id", name);
    }
    domSetData(node, "value", value);
}

/* Same as get_wnd_var_str of imgdec.inc. */
get_wnd_var_str(name[], value[], maxlen)
{
    new vars = domFindTag(domRoot(), "__wndvar__", _, 0);
    if (!vars)
    {
        return 0;
    }
    new node = domFindTag(vars, "__wndvar_str__", name, 0);
    if (!node)
    {
        return 0;
    }
    domGetData(node, "value", value, maxlen);
    return 1;
}

new g_photo_db_name[] = "photo";
new Float:g_screen_width;
new Float:g_screen_height;
new Float:g_screen_depth;

/* Same as wait_database of misc.inc. */
wait_database()
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

/* Same as get_screen_size of text.inc. */
get_screen_size(&Float:screen_w, &Float:screen_h)
{
    GetScreen(Camera(0), g_screen_width, g_screen_height, g_screen_depth);
    screen_w = g_screen_width;
    screen_h = g_screen_height;
}

/* Same as create_focus_plane2 of focus.inc. */
create_focus_plane(script[])
{
    new plane = CreatePlane3x3(Scene(0));
    SetTexture(plane, imgGetResource(RES_FOCUS), 0, -1);
    SetOptionI(plane, TAG("ALPH"), BLEND_ADD);
    AttachScript(plane, _, script);
    return plane;
}

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
    get_screen_size(screen_w, screen_h);
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
    item_count = item_count + load_photo_info(side, result, mode, title, group, id, selected);
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
                        get_group_tag_attr("TEXT", group, index, "value", label, 64);
                        get_group_tag_attr("TEXT", group, index, "edit", edit, 16);
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
                    get_group_tag_attr("TEXT", group, selected, "id", id, 64);
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
    get_screen_size(screen_w, screen_h);
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
    get_group_tag_attr("TEXT", group, index, "value", cur_title, 64);
    get_group_tag_attr("TEXT", group, index, "subgroup", cur_group, 64);
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
    get_group_tag_attr("TEXT", group, index, "retgroup", ret_group, 64);
    get_group_tag_attr("TEXT", group, index, "retid", ret_id, 64);
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
            get_group_tag_attr("TEXT", ret_group, ret_index, "value", title, 64);
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
        get_group_tag_attr("TEXT", group, index, "subgroup", sub_group, 64);
        if (!strcmp(sub_group, ""))
        {
            printf("WARNING: nothing sub item !!\n");
        }
        else
        {
            new sub_title[64];
            get_group_tag_attr("TEXT", group, index, "value", sub_title, 64);
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
 * Shows the message of result, a TEXT id and an optional second line: msg0 for 0 or more, msg1 to msg5 for -1 to
 * -5; returns 1 if there was one to show.
 */
show_result_message(result, msg0[] = "", detail0[] = "", msg1[] = "", detail1[] = "", msg2[] = "", detail2[] = "",
        msg3[] = "", detail3[] = "", msg4[] = "", detail4[] = "", msg5[], detail5[] = "")
{
    new text[256] = {0};
    new detail[128] = {0};
    if (result > 0)
    {
        result = 0;
    }
    switch (result)
    {
        case 0:
        {
            get_tag_attr("TEXT", msg0, "value", text, 256);
            if (strlen(detail0) > 0)
            {
                get_tag_attr("TEXT", detail0, "value", detail, 128);
                sprintf(text, "%s\n%s", text, detail);
            }
        }
        case -1:
        {
            get_tag_attr("TEXT", msg1, "value", text, 256);
            if (strlen(detail1) > 0)
            {
                get_tag_attr("TEXT", detail1, "value", detail, 128);
                sprintf(text, "%s\n%s", text, detail);
            }
        }
        case -2:
        {
            get_tag_attr("TEXT", msg2, "value", text, 256);
            if (strlen(detail2) > 0)
            {
                get_tag_attr("TEXT", detail2, "value", detail, 128);
                sprintf(text, "%s\n%s", text, detail);
            }
        }
        case -3:
        {
            get_tag_attr("TEXT", msg3, "value", text, 256);
            if (strlen(detail3) > 0)
            {
                get_tag_attr("TEXT", detail3, "value", detail, 128);
                sprintf(text, "%s\n%s", text, detail);
            }
        }
        case -4:
        {
            get_tag_attr("TEXT", msg4, "value", text, 256);
            if (strlen(detail4) > 0)
            {
                get_tag_attr("TEXT", detail4, "value", detail, 128);
                sprintf(text, "%s\n%s", text, detail);
            }
        }
        case -5:
        {
            get_tag_attr("TEXT", msg5, "value", text, 256);
            if (strlen(detail5) > 0)
            {
                get_tag_attr("TEXT", detail5, "value", detail, 128);
                sprintf(text, "%s\n%s", text, detail);
            }
        }
    }
    if (strlen(text) > 0)
    {
        show_message(text, "");
        return 1;
    }
    return 0;
}

public focus; /* the focus of the message dialog */

/* Shows msg in a confirm_d dialog with ok_label on its button and waits until it is closed. */
show_message(msg[], ok_label[])
{
    new args[256];
    sprintf(args, "msg=%s;ok=%s", msg, ok_label);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 240.0, 0.0, 0);
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

/* Same as escape_quotes of string.inc. */
escape_quotes(dest[], src[], maxlen)
{
    new len = strlen(src);
    new max_chars = maxlen * 4 - 2;
    new out;
    for (new i; i < len && out < max_chars; i++)
    {
        if (src{i} == '\'')
        {
            dest{out++} = '\'';
            dest{out++} = '\'';
        }
        else
        {
            dest{out++} = src{i};
        }
    }
    dest{out++} = 0;
    if (out >= max_chars)
    {
        return -1;
    }
    return 0;
}

/* Same as ends_with_nocase of string.inc. */
ends_with_nocase(str[], suffix[])
{
    new len = strlen(str);
    new suffix_len = strlen(suffix);
    for (new i; i < suffix_len; i++)
    {
        if (toupper(str{len - 1 - i}) != toupper(suffix{suffix_len - 1 - i}))
        {
            return 0;
        }
    }
    return 1;
}

/*
 * Splits size (bytes) for format_size: in MB from about 10 MB up, in KB below; from 1000 of the unit up, whole is
 * the thousands and fraction the rest.
 */
split_size(size, &unit, &whole, &fraction)
{
    new count;
    printf(".............size = %d\n", size);
    unit = 0;
    whole = 0;
    fraction = 0;
    count = size / 1048576000;
    if (count > 0)
    {
        unit = SIZE_MB_THOUSANDS;
        whole = count;
        fraction = (size - count * 1048576000) / 1048576;
        return;
    }
    count = size / 10240000;
    if (count > 0)
    {
        unit = SIZE_MB;
        whole = size / 1048576;
        return;
    }
    count = size / 1024000;
    if (count > 0)
    {
        unit = SIZE_KB_THOUSANDS;
        whole = count;
        fraction = (size - count * 1024000) / 1024;
        return;
    }
    unit = SIZE_KB;
    whole = size / 1024;
    return;
}

/* Writes size (bytes) into text in the TEXT format of its unit. */
format_size(text[], size)
{
    new format[64];
    new unit;
    new whole;
    new fraction;
    split_size(size, unit, whole, fraction);
    switch (unit)
    {
        case SIZE_MB_THOUSANDS:
        {
            get_tag_attr("TEXT", "msgphoto_info_size_mbyte2", "value", format, 64);
            sprintf(text, format, whole, fraction);
        }
        case SIZE_MB:
        {
            get_tag_attr("TEXT", "msgphoto_info_size_mbyte1", "value", format, 64);
            sprintf(text, format, whole);
        }
        case SIZE_KB_THOUSANDS:
        {
            get_tag_attr("TEXT", "msgphoto_info_size_kbyte2", "value", format, 64);
            sprintf(text, format, whole, fraction);
        }
        case SIZE_KB:
        {
            get_tag_attr("TEXT", "msgphoto_info_size_kbyte1", "value", format, 64);
            sprintf(text, format, whole);
        }
    }
}

/*
 * Selects the albums of kind (ALBUM_*) in the photo database, or the files of the USB device; returns their count,
 * 1 for ALBUM_DISC, 0 without a USB device, -1 on an error.
 */
select_albums(kind)
{
    new status;
    if (ALBUM_LIBRARY == kind)
    {
        status = datSQLExec("SELECT *, oid FROM sce_photo WHERE info_type = 3 ORDER BY create_date DESC");
    }
    else if (kind == ALBUM_FAVORITE)
    {
        new sql[128];
        sprintf(sql, "SELECT *, oid FROM sce_photo WHERE info_type = 4 ORDER BY create_date DESC");
        status = datSQLExec(sql);
    }
    else
    {
        if (kind == ALBUM_DISC)
        {
            return 1;
        }
        if (kind == ALBUM_USB)
        {
            new usb_found;
            get_wnd_var_int("usb_found", usb_found);
            set_wnd_var_int("usb_invalid", 0);
            if (usb_found)
            {
                new mount_dir[64];
                get_tag_attr("TEXT", "text_USBMOUNT", "value", mount_dir, 64);
                status = datFind("image", mount_dir);
                printf("get photo from %s\n", mount_dir);
            }
            else
            {
                return 0;
            }
        }
    }
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

/* Same as open_photo_db of fileop.inc. */
open_photo_db(db_mode)
{
    new ret;
    if (ALBUM_LIBRARY == db_mode || db_mode == ALBUM_FAVORITE)
    {
        ret = datOpen(DAT_DATABASE, g_photo_db_name, DAT_ORDER_MTIME_DESC);
    }
    else if (db_mode == ALBUM_USB)
    {
        ret = datOpen(DAT_FILE_LIST, "", DAT_ORDER_NAME_ASC);
    }
    if (0 != ret)
    {
        return -1;
    }
    ret = wait_database();
    if (ret < 0)
    {
        return -1;
    }
    printf("album db open %d -----------\n", db_mode);
    return 0;
}

/*
 * Selects the photos of the album PhotoAccessAlbum (PHOTOS_LIBRARY, PHOTOS_FAVORITE) or the files of the USB device
 * (PHOTOS_USB); returns their count, 0 without a USB device, -1 on an error.
 */
select_album_photos(kind)
{
    new album_uri[128];
    if (avalGetNum("PhotoAccessAlbum") <= 0)
    {
        return -1;
    }
    avalGetValue("PhotoAccessAlbum", 0, album_uri, 128);
    new status;
    if (kind == PHOTOS_LIBRARY)
    {
        new sql[256];
        sql = "SELECT *, oid FROM sce_photo WHERE info_type = 1 AND album_uri = '";
        append_string(sql, album_uri, 256);
        append_string(sql, "' ORDER BY create_date,title", 256);
        status = datSQLExec(sql);
    }
    else if (kind == PHOTOS_FAVORITE)
    {
        new sql[256];
        sql = "SELECT *, oid FROM sce_photo WHERE info_type = 2 AND album_uri = '";
        append_string(sql, album_uri, 256);
        append_string(sql, "' ORDER BY create_date,title", 256);
        status = datSQLExec(sql);
    }
    else if (kind == PHOTOS_USB)
    {
        new usb_found;
        get_wnd_var_int("usb_found", usb_found);
        set_wnd_var_int("usb_invalid", 0);
        if (usb_found)
        {
            new mount_dir[64];
            get_tag_attr("TEXT", "text_USBMOUNT", "value", mount_dir, 64);
            status = datFind("image", mount_dir);
            printf("get photo from %s\n", mount_dir);
        }
        else
        {
            return 0;
        }
    }
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

/* Opens the database of the photos of kind (PHOTOS_*); returns 0 or -1. */
open_thumb_db(kind)
{
    new status = -1;
    if (kind == PHOTOS_LIBRARY || kind == PHOTOS_FAVORITE)
    {
        status = datOpen(DAT_DATABASE, g_photo_db_name, DAT_ORDER_MTIME_DESC);
    }
    else if (kind == PHOTOS_USB)
    {
        status = datOpen(DAT_FILE_LIST, "", DAT_ORDER_NAME_ASC);
    }
    if (0 != status)
    {
        return -1;
    }
    status = wait_database();
    if (status < 0)
    {
        return -1;
    }
    printf("thumb db open %d -----------\n", kind);
    return 0;
}

new g_album_db_state = DB_CLOSED;
new g_thumb_db_state = DB_CLOSED;

/* Opens the database of kind and selects its rows; returns their count. */
open_album_db(kind, server)
{
    open_album_table(kind, server);
    return select_album_rows(kind, server);
}

/* Opens the database of kind, here or through the database script server ("OPEN"); returns 0 or -1. */
open_album_table(kind, server)
{
    new status;
    if (server)
    {
        SendMessage(server, TAG("OPEN"), kind, Self(), 0, 0);
        for (; ; )
        {
            sleep;
            if (FindMessage(TAG("OPEN"), status))
            {
                break;
            }
        }
    }
    else if (kind < PHOTOS_LIBRARY)
    {
        status = open_photo_db(kind);
    }
    else
    {
        status = open_thumb_db(kind);
    }
    if (kind < PHOTOS_LIBRARY)
    {
        g_album_db_state = DB_OPEN;
    }
    else
    {
        g_thumb_db_state = DB_OPEN;
    }
    return status;
}

/*
 * Selects the rows of kind, here or through the database script server ("FIND", answered by "DNUM"); returns their
 * count.
 */
select_album_rows(kind, server)
{
    new count;
    if (server)
    {
        SendMessage(server, TAG("FIND"), Self(), 0, 0, 0);
        for (; ; )
        {
            sleep;
            if (FindMessage(TAG("DNUM"), count))
            {
                break;
            }
        }
    }
    else if (kind < PHOTOS_LIBRARY)
    {
        count = select_albums(kind);
    }
    else
    {
        count = select_album_photos(kind);
    }
    if (kind < PHOTOS_LIBRARY)
    {
        g_album_db_state = DB_READY;
    }
    else
    {
        g_thumb_db_state = DB_READY;
    }
    return count;
}

/* Same as get_photo_field of photodate.inc. */
get_photo_field(index, field, text[], album_kind, server, maxlen)
{
    new ret;
    new column;
    if (ALBUM_LIBRARY == album_kind || album_kind == ALBUM_FAVORITE)
    {
        switch (field)
        {
            case FIELD_URL:
            {
                column = COL_CONTENT_URI;
            }
            case FIELD_ALBUM_NAME:
            {
                column = COL_ALBUM_NAME;
            }
            case FIELD_ALBUM_URI:
            {
                column = COL_ALBUM_URI;
            }
            case FIELD_DATE:
            {
                column = COL_CREATE_DATE;
            }
            case FIELD_OID:
            {
                column = COL_OID;
            }
            default:
            {
                return -1;
            }
        }
    }
    else if (album_kind == PHOTOS_LIBRARY || album_kind == PHOTOS_FAVORITE)
    {
        switch (field)
        {
            case FIELD_URL:
            {
                column = COL_CONTENT_URI;
            }
            case FIELD_ALBUM_NAME:
            {
                column = COL_ALBUM_NAME;
            }
            case FIELD_TITLE:
            {
                column = COL_TITLE;
            }
            case FIELD_DATE:
            {
                column = COL_CREATE_DATE;
            }
            case FIELD_OID:
            {
                column = COL_OID;
            }
            default:
            {
                return -1;
            }
        }
    }
    else if (album_kind == ALBUM_USB || album_kind == PHOTOS_USB)
    {
        switch (field)
        {
            case FIELD_URL:
            {
                column = USB_COL_PATH;
            }
            case FIELD_ALBUM_NAME:
            {
                get_tag_attr("TEXT", "msgphoto_menu_camera", "value", text, maxlen);
                return 0;
            }
            case FIELD_TITLE:
            {
                column = USB_COL_NAME;
            }
            default:
            {
                return -1;
            }
        }
    }
    else
    {
        column = field;
    }
    if (server)
    {
        SendMessage(server, TAG("DGET"), index, column, Self(), 0);
        for (; ; )
        {
            sleep;
            if (FindMessageStr(TAG("DERR"), text, maxlen))
            {
                ret = -1;
                break;
            }
            if (FindMessageStr(TAG("DSTR"), text, maxlen))
            {
                break;
            }
        }
    }
    else
    {
        ret = datGetData(index, column, text, maxlen);
    }
    if ((album_kind == ALBUM_USB || album_kind == PHOTOS_USB) && column == USB_COL_NAME)
    {
    }
    if (field == FIELD_DATE)
    {
        new time_notation = GetSystemParam(SYS_TIME_NOTATION);
        new date_notation = GetSystemParam(SYS_DATE_NOTATION);
        photoDateGetFromDB(text, maxlen, date_notation, time_notation);
    }
    return ret;
}

/* Empties the AlbumID list: the albums whose cover update_album_covers sets again. */
clear_album_ids()
{
    avalDelete("AlbumID");
}

/* Adds album_uri to the AlbumID list; returns -1 if it was already there. */
add_album_id(album_uri[])
{
    new value[64];
    new count = avalGetNum("AlbumID");
    for (new i; i < count; i++)
    {
        avalGetValue("AlbumID", i, value, 64);
        if (!strcmp(value, album_uri))
        {
            return -1;
        }
    }
    avalAdd("AlbumID", album_uri);
    printf("update album %s\n", album_uri);
    return 0;
}

/*
 * Adds the albums that hold the photo content_uri to the AlbumID list (kind ALBUM_FAVORITE: only the favorite
 * ones); returns their count or -1.
 */
collect_photo_albums(content_uri[], kind)
{
    new count;
    new sql[128];
    if (ALBUM_LIBRARY == kind)
    {
        sprintf(sql, "SELECT album_uri FROM sce_photo WHERE content_uri = '%s'", content_uri);
    }
    else if (kind == ALBUM_FAVORITE)
    {
        sprintf(sql, "SELECT album_uri FROM sce_photo WHERE ( info_type = 2 OR info_type = 4 ) AND content_uri = '%s'",
                content_uri);
    }
    else
    {
        return -1;
    }
    printf("sql com = %s\n", sql);
    count = datSQLExec(sql);
    if (count < 0)
    {
        return -1;
    }
    count = wait_database();
    if (count < 0)
    {
        return -1;
    }
    count = datGetNum();
    if (0 == count)
    {
        return 0;
    }
    if (count < 0)
    {
        if (count < 0)
        {
            return -1;
        }
    }
    new i;
    new rows = count;
    new album_uri[64];
    i = 0;
    for (; i < rows; i++)
    {
        get_photo_field(i, 1, album_uri, -1, 0, 64);
        add_album_id(album_uri);
    }
    return count;
}

/*
 * Makes the first photo of each album of the AlbumID list its cover (kind ALBUM_FAVORITE or PHOTOS_FAVORITE: of the
 * favorite albums) and empties the list.
 */
update_album_covers(kind)
{
    new status;
    new album_uri[64];
    new count = avalGetNum("AlbumID");
    for (new i; i < count; i++)
    {
        new select_sql[128];
        new sql[256];
        avalGetValue("AlbumID", i, album_uri, 64);
        if (ALBUM_LIBRARY == kind || kind == PHOTOS_LIBRARY)
        {
            sprintf(select_sql,
                    "SELECT content_uri FROM sce_photo WHERE album_uri = '%s' AND info_type <= 2 ORDER BY create_date,title LIMIT 1",
                    album_uri);
            sprintf(sql, "UPDATE sce_photo SET content_uri = ( %s ) WHERE album_uri = '%s' AND info_type >= 3 ",
                    select_sql, album_uri);
        }
        else if (kind == ALBUM_FAVORITE || kind == PHOTOS_FAVORITE)
        {
            sprintf(select_sql,
                    "SELECT content_uri FROM sce_photo WHERE album_uri = '%s' AND info_type = 2 ORDER BY create_date,title LIMIT 1",
                    album_uri);
            sprintf(sql, "UPDATE sce_photo SET content_uri = ( %s ) WHERE album_uri = '%s' AND info_type = 4 ",
                    select_sql, album_uri);
        }
        printf("sql com = %s\n", sql);
        status = datSQLExec(sql);
        if (status < 0)
        {
            break;
        }
        status = wait_database();
        if (status < 0)
        {
            break;
        }
    }
    avalDelete("AlbumID");
    return status;
}

public focus_glowfly; /* the focus object */
public select = -1;   /* index of the photo or album, argument select */
public db_status;     /* -1 if the photo or album could not be read */
public thumb;         /* the thumbnail, an info_thumb plane */

/*
 * Opens and selects the database of the window variable albummode, stores the fields of photo or album ITEM_NO in
 * window variables (INFO_*: date, url, oid, title, album name, size, photo count) and shows its thumbnail; result
 * becomes INFOMENU_USB_REMOVED when the USB device goes away.
 */
load_photo_info(side, &result, mode, title[], group[], id[], &selected)
{
    new ret;
    new buf[64];
    ret = side;
    ret = result;
    ret = mode;
    ret = selected;
    copy_string(buf, title, 64);
    copy_string(buf, group, 64);
    copy_string(buf, id, 64);
    ret = 0;
    switch (mode)
    {
        case INFOMENU_MODE_GROUP:
        {
            ret = 0;
            new count;
            if (avalGetNum("PhotoAccessAlbum") <= 0)
            {
                db_status = -1;
            }
            new kind;
            new invalid;
            if (get_wnd_var_int("albummode", kind))
            {
                if (kind == ALBUM_USB || kind == PHOTOS_USB)
                {
                    if (get_wnd_var_int("usb_invalid", invalid))
                    {
                        if (0 != invalid)
                        {
                            result = INFOMENU_USB_REMOVED;
                            return 0;
                        }
                    }
                }
                count = open_album_db(kind, 0);
            }
            else
            {
                db_status = -1;
                return 0;
            }
            printf("new db opened %d\n", count);
            if (count <= 0)
            {
                db_status = -1;
                return 0;
            }
            if (kind == ALBUM_USB || kind == PHOTOS_USB)
            {
                if (get_wnd_var_int("usb_invalid", invalid))
                {
                    if (0 != invalid)
                    {
                        result = INFOMENU_USB_REMOVED;
                        return 0;
                    }
                }
            }
            thumb = CreatePlane(Disp2D(0));
            new oid[32];
            new text[64];
            AttachScript(thumb, "info_thumb", "info_thumb");
            SetColor(thumb, 1.0, 1.0, 1.0, 0.0);
            {
                new item_no;
                get_wnd_var_int("ITEM_NO", item_no);
                get_photo_field(item_no, FIELD_DATE, text, kind, 0, 64);
                set_wnd_var_str("INFO_IMPORTDATE", text);
                printf("info import date %s\n", text);
                get_photo_field(item_no, FIELD_URL, text, kind, 0, 64);
                set_wnd_var_str("INFO_IMAGEURL", text);
                printf("info image url %s\n", text);
                get_photo_field(item_no, FIELD_OID, oid, kind, 0, 32);
                set_wnd_var_str("INFO_OID", oid);
                printf("info OID %s\n", oid);
                SendMessageStr(thumb, TAG("DCDE"), text);
                new album_name[64];
                new title_name[64];
                get_photo_field(item_no, FIELD_ALBUM_NAME, album_name, kind, 0, 64);
                get_photo_field(item_no, FIELD_TITLE, title_name, kind, 0, 64);
                if (kind == PHOTOS_FAVORITE)
                {
                    append_string(title_name, " / ", 64);
                    append_string(title_name, album_name, 64);
                }
                set_wnd_var_str("INFO_TITLENAME", title_name);
                avalGetValue("PhotoAccessAlbumName", 0, album_name, 64);
                set_wnd_var_str("INFO_ALBUMNAME", album_name);
            }
            sleep;
            if (kind == ALBUM_USB || kind == PHOTOS_USB)
            {
                if (get_wnd_var_int("usb_invalid", invalid))
                {
                    if (0 != invalid)
                    {
                        result = INFOMENU_USB_REMOVED;
                        return 0;
                    }
                }
            }
            if (kind < PHOTOS_LIBRARY)
            {
                count = select_album_rows(kind + PHOTOS_LIBRARY, 0);
                set_wnd_var_int("INFO_IMAGENUM", count);
                printf("info image num %d\n", count);
                new url[64];
                new status;
                new file_mode;
                new file_size;
                new total_kb;
                for (new i; i < count; i++)
                {
                    get_photo_field(i, FIELD_URL, url, kind + PHOTOS_LIBRARY, 0, 64);
                    status = fileLstat(url, file_mode, file_size);
                    if (status >= 0)
                    {
                        total_kb = total_kb + (file_size + 1023) / 1024;
                    }
                    else if (status == -2 && (kind == ALBUM_USB || kind == PHOTOS_USB))
                    {
                        db_status = -1;
                        return 0;
                    }
                }
                set_wnd_var_int("INFO_SIZE", total_kb * 1024);
                count = select_album_rows(kind, 0);
            }
            else
            {
                new status;
                new file_mode;
                new file_size;
                status = fileLstat(text, file_mode, file_size);
                if (status >= 0)
                {
                    set_wnd_var_int("INFO_SIZE", file_size);
                }
                else
                {
                    set_wnd_var_int("INFO_SIZE", 0);
                }
            }
            if (kind == PHOTOS_FAVORITE)
            {
                new node = domFindTag(domRoot(), "TEXT", "INFO_THUMB_NAME", 0);
                if (node)
                {
                    domSetData(node, "edit", "disable");
                }
            }
            sleep;
            if (kind == ALBUM_USB || kind == PHOTOS_USB)
            {
                if (get_wnd_var_int("usb_invalid", invalid))
                {
                    if (0 != invalid)
                    {
                        result = INFOMENU_USB_REMOVED;
                        return 0;
                    }
                }
            }
        }
    }
    return ret;
}

/* Sends "INFO" to item with the value its id names, from the window variables of load_photo_info. */
fill_info_item(manager, item, side, mode, index, group[], text[])
{
    new unused; /* takes the parameters the function does not use */
    new buf[64];
    unused = manager;
    unused = side;
    unused = mode;
    unused = index;
    unused = 64;
    copy_string(buf, group, unused);
    if (db_status < 0)
    {
        return;
    }
    switch (mode)
    {
        case INFOMENU_MODE_GROUP:
        {
            printf("stdmenu btn RRGT\n");
            new item_no;
            get_wnd_var_int("ITEM_NO", item_no);
            new item_id[64];
            get_group_tag_attr("TEXT", group, index, "id", item_id, 64);
            if (!strcmp("INFO_THUMB_TITLE", item_id) || !strcmp("INFO_ALBUM_TITLE", item_id))
            {
                new name[64];
                if (get_wnd_var_str("INFO_ALBUMNAME", name, 64))
                {
                    copy_string(text, name, 64);
                }
                SendMessageStr(item, TAG("INFO"), text);
            }
            else if (!strcmp("INFO_THUMB_SIZE", item_id) || !strcmp("INFO_ALBUM_SIZE", item_id)
                    || !strcmp("INFO_USBTMB_SIZE", item_id))
            {
                new size;
                if (get_wnd_var_int("INFO_SIZE", size))
                {
                    format_size(text, size);
                }
                else
                {
                    format_size(text, 0);
                }
                SendMessageStr(item, TAG("INFO"), text);
            }
            else if (!strcmp("INFO_THUMB_DATE", item_id) || !strcmp("INFO_ALBUM_DATE", item_id))
            {
                new date_text[64];
                if (get_wnd_var_str("INFO_IMPORTDATE", date_text, 64))
                {
                    copy_string(text, date_text, 64);
                }
                SendMessageStr(item, TAG("INFO"), text);
            }
            else if (!strcmp("INFO_THUMB_RESO", item_id) || !strcmp("INFO_USBTMB_RESO", item_id))
            {
                new width;
                new height;
                while (0 == width)
                {
                    sleep;
                    width = GetPubVar(thumb, "img_org_w");
                    height = GetPubVar(thumb, "img_org_h");
                }
                if (width < 0)
                {
                    width = 0;
                }
                if (height < 0)
                {
                    height = 0;
                }
                get_tag_attr("TEXT", "msgphoto_photoinfo_pixel", "value", buf, 64);
                sprintf(text, "%d x %d ", width, height);
                append_string(text, buf, 64);
                SendMessageStr(item, TAG("INFO"), text);
            }
            else if (!strcmp("INFO_ALBUM_NUM", item_id))
            {
                new count;
                if (get_wnd_var_int("INFO_IMAGENUM", count))
                {
                    get_tag_attr("TEXT", "msgphoto_info_pict_num", "value", buf, 64);
                    sprintf(text, buf, count);
                }
                SendMessageStr(item, TAG("INFO"), text);
            }
            else if (!strcmp("INFO_THUMB_NAME", item_id) || !strcmp("INFO_USBTMB_NAME", item_id))
            {
                new name[64];
                if (get_wnd_var_str("INFO_TITLENAME", name, 64))
                {
                    copy_string(text, name, 64);
                }
                SendMessageStr(item, TAG("INFO"), text);
            }
            else if (!strcmp("INFO_THUMB_TYPE", item_id) || !strcmp("INFO_USBTMB_TYPE", item_id))
            {
                new type[32];
                new url[64];
                if (!get_wnd_var_str("INFO_IMAGEURL", url, 64))
                {
                    get_tag_attr("TEXT", "FMT_ERR", "value", type, 32);
                }
                if (ends_with_nocase(url, ".JPG") || ends_with_nocase(url, ".JPEG"))
                {
                    get_tag_attr("TEXT", "JPEG", "value", type, 32);
                }
                else if (ends_with_nocase(url, ".TIF") || ends_with_nocase(url, ".TIFF"))
                {
                    get_tag_attr("TEXT", "TIFF", "value", type, 32);
                }
                else if (ends_with_nocase(url, ".BMP"))
                {
                    get_tag_attr("TEXT", "BMP", "value", type, 32);
                }
                else if (ends_with_nocase(url, ".GIF"))
                {
                    get_tag_attr("TEXT", "GIF", "value", type, 32);
                }
                else if (ends_with_nocase(url, ".PNG"))
                {
                    get_tag_attr("TEXT", "PNG", "value", type, 32);
                }
                SendMessageStr(item, TAG("INFO"), type);
            }
            else
            {
                SendMessageStr(item, TAG("INFO"), "NOITEM");
            }
        }
    }
}

new g_renamed; /* the album name or the title was changed */

/*
 * Handles a push of item index: the circle on the album name or the photo title asks for a new name with the
 * keyboard until one is entered or the edit is cancelled, and writes it to the database (a photo also sets again
 * the covers of its albums); the cross closes the menu, with INFOMENU_RENAMED after a rename.
 */
handle_push(manager, item, side, &mode, index, button, title[], group[], id[])
{
    new unused; /* takes the parameters the function does not use */
    new buf[64];
    unused = manager;
    unused = item;
    unused = side;
    unused = mode;
    unused = index;
    unused = 64;
    copy_string(buf, group, unused);
    copy_string(buf, title, unused);
    copy_string(buf, id, unused);
    new result = INFOMENU_REOPEN;
    if (db_status < 0)
    {
        return INFOMENU_ERROR;
    }
    if (button == TAG("RRGT"))
    {
        printf("infomenu btn RRGT\n");
        switch (mode)
        {
            case INFOMENU_MODE_GROUP:
            {
                if (!strcmp("INFO_ALBUM_TITLE", id))
                {
                    new entered;
                    new name[64];
                    for (; ; )
                    {
                        new prompt[64];
                        get_tag_attr("TEXT", "msgphoto_albuminfo_album", "value", prompt, 64);
                        get_wnd_var_str("INFO_ALBUMNAME", name, 64);
                        if (run_edit_dialog(prompt, name, 0, 256, 256))
                        {
                            if (strlen(name) > 0)
                            {
                                entered = 1;
                                break;
                            }
                            else
                            {
                                show_result_message(-5, _, _, _, _, _, _, _, _, _, _, "msgphoto_input_fail_noname");
                            }
                        }
                        else
                        {
                            entered = 0;
                            break;
                        }
                    }
                    if (entered)
                    {
                        new sql[256];
                        new kind;
                        get_wnd_var_int("albummode", kind);
                        new album_uri[64];
                        avalGetValue("PhotoAccessAlbum", 0, album_uri, 64);
                        new quoted[128];
                        escape_quotes(quoted, name, 128);
                        if (ALBUM_LIBRARY == kind)
                        {
                            sprintf(sql,
                                    "UPDATE sce_photo SET album_name = '%s' WHERE ( album_uri = '%s' AND info_type IN ( 1, 3 ) ) OR ( info_type = 2 AND content_uri IN ( SELECT content_uri FROM sce_photo WHERE album_uri = '%s' AND info_type = 1) ) ",
                                    quoted, album_uri, album_uri);
                        }
                        else if (kind == ALBUM_FAVORITE)
                        {
                            sprintf(sql,
                                    "UPDATE sce_photo SET album_name = '%s' WHERE album_uri = '%s' AND info_type = 4 ",
                                    quoted, album_uri);
                        }
                        else
                        {
                            printf("WARNING : wrong album id!!\n");
                            return INFOMENU_ERROR;
                        }
                        printf("sql com = %s\n", sql);
                        new status = datSQLExec(sql);
                        if (status < 0)
                        {
                            return INFOMENU_ERROR;
                        }
                        status = wait_database();
                        if (status < 0)
                        {
                            return INFOMENU_ERROR;
                        }
                        avalSet("PhotoAccessAlbumName", name);
                        set_wnd_var_str("INFO_ALBUMNAME", name);
                        SendMessageStr(item, TAG("INFO"), name);
                        g_renamed = 1;
                    }
                }
                else if (!strcmp("INFO_THUMB_NAME", id))
                {
                    new entered;
                    new name[64];
                    for (; ; )
                    {
                        new prompt[64];
                        get_tag_attr("TEXT", "msgphoto_photoinfo_pict", "value", prompt, 64);
                        get_wnd_var_str("INFO_TITLENAME", name, 64);
                        if (run_edit_dialog(prompt, name, 0, 256, 256))
                        {
                            if (strlen(name) > 0)
                            {
                                entered = 1;
                                break;
                            }
                            else
                            {
                                show_result_message(-5, _, _, _, _, _, _, _, _, _, _, "msgphoto_input_fail_noname");
                            }
                        }
                        else
                        {
                            entered = 0;
                            break;
                        }
                    }
                    if (entered)
                    {
                        new sql[128];
                        new kind;
                        get_wnd_var_int("albummode", kind);
                        new url[64];
                        get_wnd_var_str("INFO_IMAGEURL", url, 64);
                        new oid[32];
                        get_wnd_var_str("INFO_OID", oid, 32);
                        new quoted[128];
                        escape_quotes(quoted, name, 128);
                        if (kind == PHOTOS_LIBRARY)
                        {
                            sprintf(sql,
                                    "UPDATE sce_photo SET title = '%s' WHERE content_uri = '%s' AND info_type IN (1,2) ",
                                    quoted, url);
                        }
                        else if (kind == PHOTOS_FAVORITE)
                        {
                            sprintf(sql,
                                    "UPDATE sce_photo SET title = '%s' WHERE content_uri = '%s' AND info_type = 2 AND oid = '%s'",
                                    quoted, url, oid);
                        }
                        else
                        {
                            printf("WARNING : wrong thumb id!!\n");
                            return INFOMENU_ERROR;
                        }
                        printf("sql com = %s\n", sql);
                        new status = datSQLExec(sql);
                        if (status < 0)
                        {
                            return INFOMENU_ERROR;
                        }
                        status = wait_database();
                        if (status < 0)
                        {
                            return INFOMENU_ERROR;
                        }
                        clear_album_ids();
                        status = collect_photo_albums(url, 0);
                        if (status < 0)
                        {
                            return INFOMENU_ERROR;
                        }
                        status = update_album_covers(kind);
                        if (status < 0)
                        {
                            return INFOMENU_ERROR;
                        }
                        set_wnd_var_str("INFO_TITLENAME", name);
                        SendMessageStr(item, TAG("INFO"), name);
                        g_renamed = 1;
                    }
                }
            }
        }
        mode = INFOMENU_MODE_GROUP;
        result = INFOMENU_STAY;
    }
    if (button == TAG("RDWN"))
    {
        switch (mode)
        {
            case INFOMENU_MODE_GROUP:
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
        }
    }
    return result;
}

/*
 * Shows the menu and leaves its outcome to the page in the album variable InfoStatus: NONE, INVALID after a rename
 * (with InfoAlbumname and InfoTitlename), USBINVALID or ERROR.
 */
public main()
{
    new result = INFOMENU_CLOSE;
    new usb_checker = CreateNull(Self());
    AttachScript(usb_checker, "usb_media_check");
    sleep;
    focus_glowfly = create_focus_plane("focus");
    SetOptionI(focus_glowfly, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(focus_glowfly, TAG("ZOFS"), -1.0);
    {
        new value[32];
        GetArg("select", value, 32);
        select = atoi(value);
        set_wnd_var_int("ITEM_NO", select);
        new level;
        GetArg("mode", value, 32);
        if (!strcmp("INFO_ALBUM", value))
        {
            level = ALBUM_LIBRARY;
        }
        else if (!strcmp("INFO_THUMB", value))
        {
            level = PHOTOS_LIBRARY;
        }
        else if (!strcmp("INFO_USBTMB", value))
        {
            level = PHOTOS_LIBRARY;
        }
        else
        {
            result = INFOMENU_ERROR;
            goto finish;
        }
        avalGetValue("ALBUM_MODE", 0, value, 32);
        new kind;
        if (!strcmp(value, "FAVORITE"))
        {
            kind = level + ALBUM_FAVORITE;
        }
        else if (!strcmp(value, "LIBRARY"))
        {
            kind = level + ALBUM_LIBRARY;
        }
        else if (!strcmp(value, "DISC"))
        {
            kind = level + ALBUM_DISC;
        }
        else if (!strcmp(value, "USB"))
        {
            kind = level + ALBUM_USB;
        }
        else
        {
            printf("album mode take miss !!\n");
            result = INFOMENU_ERROR;
            goto finish;
        }
        set_wnd_var_int("albummode", kind);
        if (kind == ALBUM_USB || kind == PHOTOS_USB)
        {
            new found;
            if (get_wnd_var_int("usb_found", found))
            {
                if (!found)
                {
                    result = INFOMENU_USB_REMOVED;
                    goto finish;
                }
                set_wnd_var_int("usb_invalid", 0);
            }
        }
    }
    {
        new mode[32];
        GetArg("mode", mode, 32);
        result = pop_info_menu("menu_title", mode, INFOMENU_SIDE_LEFT);
    }
    finish:
    {
        new kind;
        new invalid;
        get_wnd_var_int("albummode", kind);
        if (kind == ALBUM_USB || kind == PHOTOS_USB)
        {
            if (get_wnd_var_int("usb_invalid", invalid))
            {
                if (0 != invalid)
                {
                    result = INFOMENU_USB_REMOVED;
                }
            }
        }
    }
    if (result == INFOMENU_ERROR)
    {
        avalSet("InfoStatus", "ERROR");
    }
    else if (result == INFOMENU_RENAMED)
    {
        new name[64];
        if (get_wnd_var_str("INFO_ALBUMNAME", name, 64))
        {
            avalSet("InfoAlbumname", name);
        }
        if (get_wnd_var_str("INFO_TITLENAME", name, 64))
        {
            avalSet("InfoTitlename", name);
        }
        avalSet("InfoStatus", "INVALID");
    }
    else if (result == INFOMENU_USB_REMOVED)
    {
        printf("usb invalid finish !!!!!!!!!!!!!!!!!!!!\n");
        avalSet("InfoStatus", "USBINVALID");
    }
    else
    {
        avalSet("InfoStatus", "NONE");
    }
    Destroy(Scene(0));
}

new g_usb_found; /* the last state of usb_found */

/*
 * Runs on its own object: keeps the window variable usb_found (a USB storage device is connected) up to date and
 * sets usb_invalid when it changes (1 removed, -1 connected).
 */
public usb_media_check()
{
    new media[256];
    new found;
    g_usb_found = 0;
    set_wnd_var_int("usb_found", 0);
    set_wnd_var_int("usb_invalid", 0);
    for (; ; )
    {
        new count = avalGetNum("BB_USB_MEDIA");
        if (count <= 0)
        {
            found = 0;
        }
        else if (count == 1)
        {
            new product[128];
            avalGetValue("BB_USB_MEDIA", 0, media, 256);
            if (0 == strcmp(media, "STORAGE"))
            {
                found = 1;
                count = avalGetNum("BB_USB_MEDIA_PRODUCT");
                if (count == 1)
                {
                    avalGetValue("BB_USB_MEDIA_PRODUCT", 0, product, 128);
                }
            }
            else
            {
                found = 0;
            }
        }
        else
        {
            found = 0;
        }
        set_wnd_var_int("usb_found", found);
        if (g_usb_found != found)
        {
            printf("usb_invalid !!!!\n");
            if (!found)
            {
                set_wnd_var_int("usb_invalid", 1);
            }
            else
            {
                set_wnd_var_int("usb_invalid", -1);
            }
            g_usb_found = found;
        }
        sleep;
    }
}

#include "photousb.inc"
