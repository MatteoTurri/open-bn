/*
 * /opt0/bn/script/utility/browser_infomenu.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The information page of a save in the file browser (the source BRSRCURL; the device, the memory card and the
 * icon in the arguments BR*): a general_menu of its location, size, date, kind and operations beside its turning
 * icon and its titles. On a memory card it closes when the card is removed.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "browser.inc"

#include "infomenu.inc"

#include "scroll.inc"

#include "genmenu.inc"

/* As in string.inc. */
const MC_TYPE_PS1 = 1;
const MC_TYPE_PS2 = 2;
const MC_TYPE_PDA = 3;
const MC_CHANGED = -1;
const MC_CHANGED_UNFORMATTED = -2;
const SIZE_MB_THOUSANDS = 1;
const SIZE_MB = 2;
const SIZE_KB_THOUSANDS = 3;
const SIZE_KB = 4;

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
        vars = domInsertTag(domFirstChild(domRoot(), ""), "__wndvar__");
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

new g_card_types[4]; /* the type of the card in each port, as last seen */
public device_mode; /* window variable BRDEVMODE: DEVMODE_* */
public mc_port; /* the port of the memory card shown (window variable BRMCPORT) */
public mc_slot; /* its slot (window variable BRMCSLOT) */

/* Same as update_card_info of misc.inc. */
update_card_info(&unused, one_card, port, slot)
{
    new ret;
    new job;
    if (one_card)
    {
        ret = svdatMCInfo(port, slot, job);
        if (ret < 0)
        {
            return ret;
        }
        for (; ; )
        {
            new percent;
            new status;
            ret = svdatSync(percent, status, job);
            if (ret == 1)
            {
                break;
            }
            if (ret < 0)
            {
                break;
            }
            sleep;
        }
        if (ret < 0)
        {
            return ret;
        }
    }
    else
    {
        ret = svdatMCInfo(0, 0, job);
        if (ret < 0)
        {
            return ret;
        }
        for (; ; )
        {
            new percent;
            new status;
            ret = svdatSync(percent, status, job);
            if (ret == 1)
            {
                break;
            }
            if (ret < 0)
            {
                break;
            }
            sleep;
        }
        if (ret < 0)
        {
            return ret;
        }
        ret = svdatMCInfo(1, 0, job);
        if (ret < 0)
        {
            return ret;
        }
        for (; ; )
        {
            new percent;
            new status;
            ret = svdatSync(percent, status, job);
            if (ret == 1)
            {
                break;
            }
            if (ret < 0)
            {
                break;
            }
            sleep;
        }
        if (ret < 0)
        {
            return ret;
        }
    }
    return 0;
}

/* Same as get_card_info of string.inc. */
get_card_info(port, slot, &type, &free_blocks, &formatted, &status)
{
    new ret = svdatGetMCInfo(port, slot, type, free_blocks, formatted, status);
    if (ret < 0)
    {
        return ret;
    }
    return 0;
}

/* Same as get_size_units of string.inc. */
get_size_units(bytes, &unit, &whole, &fraction)
{
    new count;
    printf(".............size = %d\n", bytes);
    unit = 0;
    whole = 0;
    fraction = 0;
    count = bytes / 1048576000;
    if (count > 0)
    {
        unit = SIZE_MB_THOUSANDS;
        whole = count;
        fraction = (bytes - count * 1048576000) / 1048576;
        return;
    }
    count = bytes / 1024000;
    if (count > 0)
    {
        unit = SIZE_KB_THOUSANDS;
        whole = count;
        fraction = (bytes - count * 1024000) / 1024;
        return;
    }
    unit = SIZE_KB;
    whole = bytes / 1024;
    return;
}

new g_shadow_colors[4] = {0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000}; /* the "SCL4" of the shadowed labels */
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
    item_count = item_count + load_save_info(side, result, mode, title, group, id, selected);
    if (item_count <= 0 && selected < 0)
    {
        result = INFOMENU_ERROR;
        goto finish;
    }
    get_wnd_var_int("BRMCPORT", mc_port);
    get_wnd_var_int("BRDEVMODE", device_mode);
    new type;
    new free_blocks;
    new formatted;
    new status;
    /* A memory card that is gone or in error (-1, -2) closes the page with the window variable BRMCEJECT. */
    if (device_mode == DEVMODE_MC)
    {
        get_card_info(mc_port, mc_slot, type, free_blocks, formatted, status);
        g_card_types[mc_port] = type;
        if (0 == type || status == MC_CHANGED || status == MC_CHANGED_UNFORMATTED)
        {
            result = INFOMENU_CLOSE;
            new ejected = 1;
            set_wnd_var_int("BRMCEJECT", ejected);
            goto finish;
        }
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
    SendMessage(menu, TAG("ANIM"), 150.0, 0, ANIM_FUNC_EXPONENT, 0);
    SendMessage(menu, TAG("OFFA"), -1.0, 0, 0, 0);
    SendMessage(menu, TAG("IDLA"), 1.0, 0, 0, 0);
    SendMessage(menu, TAG("REPT"), 150.0, 0, 0, 0);
    SendMessage(menu, TAG("SLCT"), selected, 0, 0, 0);
    SendMessage(menu, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
    SendMessage(menu, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
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
    new frames;
    for (; ; )
    {
        {
            new held;
            new pressed;
            PadState(held, pressed, _, _, -1);
            if (pressed & PAD_CROSS)
            {
                SendMessage(Self(), TAG("PUSH"), 0, TAG("RDWN"), 0, 0);
                sndSePlay(SE_CANCEL);
            }
        }
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
        /* Every 30 frames: a card that is gone, of another type or in error closes the page. */
        if (device_mode == DEVMODE_MC)
        {
            if (frames > 30)
            {
                new unused; /* unused, but part of the layout */
                update_card_info(unused, 0, 0, 0);
                get_card_info(mc_port, mc_slot, type, free_blocks, formatted, status);
                if (g_card_types[mc_port] != type || 0 == type || status == MC_CHANGED
                        || status == MC_CHANGED_UNFORMATTED)
                {
                    result = INFOMENU_CLOSE;
                    new ejected = 1;
                    set_wnd_var_int("BRMCEJECT", ejected);
                    break;
                }
                g_card_types[mc_port] = type;
                frames = 0;
            }
            else
            {
                frames++;
            }
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
    new item = CreateLabelShadow(parent);
    sprintf(args, "label_string = %s ; edit = %s ; info_width = 320", label, edit);
    AttachScript(item, "info_item", "info_item", args);
    SetPubVar(item, "pad_enable", PAD_CIRCLE);
    SetPubVar(item, "alpha_blend", BLEND_ALPHA);
    SetPubVar(item, "text_effect", 1);
    SetOptionP(item, TAG("SCL4"), g_shadow_colors, 4);
    SetOptionI(item, TAG("SX64"), 2 * 64);
    SetOptionI(item, TAG("SY64"), 2 * 64);
    return item;
}

public focus_glowfly; /* the focus object */
public select = -1; /* unused, but part of the layout */
public db_status; /* -1 if the save could not be read */
public thumb;     /* the icon of the save */
new g_file_name[512]; /* the name, and from cell 256 the directory */
new g_file_size;
new g_title1[256]; /* the title of the save, its first line */
new g_title2[256]; /* and its second line */
new g_save_kind; /* KIND_* */
new g_no_copy;   /* 1: the save may not be copied or moved */
new g_damaged;
new g_year;
new g_month;
new g_day;
new g_hour;
new g_minute;
new g_second;
new g_caption_label;
new g_title1_label;
new g_title2_label;

/*
 * Reads the information of the save BRSRCURL and shows its icon (cache file BRICONCACHE, turned by thumb_main) and
 * its titles; returns the number of items it adds to the menu, 0, or -100000 on failure. The address 2464 given to
 * svdatGetInfoData for the directory is g_file_name[256].
 */
load_save_info(side, result, mode, title[], group[], id[], &selected)
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
        case 0:
        {
            ret = 0;
            new url[256];
            if (avalGetNum("BRSRCURL") <= 0)
            {
                db_status = -1;
                return -100000;
            }
            if (avalGetValue("BRSRCURL", 0, url, 256) < 0)
            {
                db_status = -1;
                return -100000;
            }
            new job;
            new err = svdatInformation(url, job);
            if (err < 0)
            {
                db_status = -1;
                return -100000;
            }
            new percent;
            new status;
            new waiting = 1;
            while (waiting)
            {
                err = svdatSync(percent, status, job);
                if (err == 1)
                {
                    break;
                }
                if (err < 0)
                {
                    break;
                }
                sleep;
            }
            if (status < 0 || err < 0)
            {
                printf("error 1 = %d,%d\n", status, err);
                g_damaged = 1;
            }
            else
            {
                err = svdatGetInfoData(job, g_file_name, 2464, g_file_size, g_title1, g_title2, g_save_kind, g_no_copy,
                        g_year, g_month, g_day, g_hour, g_minute, g_second, g_damaged, 256, 256, 256, 256);
                if (err < 0)
                {
                    printf("error 2 = %d\n", err);
                    db_status = -1;
                    return -100000;
                }
            }
            new damaged;
            get_wnd_var_int("BRDAMAGED", damaged);
            if (damaged == STATE_DAMAGED)
            {
                g_damaged = 1;
            }
            thumb = CreateIcon(Disp2D(0));
            {
                new cache_index;
                get_wnd_var_int("BRICONCACHE", cache_index);
                new path[64];
                sprintf(path, "/opt0/cache/ictmp%04d", cache_index);
                SetColor(thumb, 1.0, 1.0, 1.0, 0.0);
                AnimColor(thumb, 1.0, 1.0, 1.0, 1.0, 500.0, 0.0, ANIM_FUNC_LINEAR);
                SetPos(thumb, 190.0, 130.0, 0.0, 0);
                SetLocalScale(thumb, 80.0, 80.0, 80.0);
                SetOptionI(thumb, TAG("SYNC"), 1);
                SetOptionI(thumb, TAG("LITS"), 1);
                SetOptionI(thumb, TAG("AMBS"), 1);
                SetOptionI(thumb, TAG("FBUF"), 1);
                SetOptionS(thumb, TAG("FILE"), path);
                AttachScript(thumb, "thumb_main");
            }
            SetPos(Self(), 0.0, 50.0, 0.0, 0);
            new backdrop = CreatePlane(Disp2D(0));
            SetOptionI(backdrop, TAG("ZSRT"), ZSORT_BACKGROUND);
            SetPos(backdrop, 320.0, 240.0, 0.0, 0);
            SetLocalScale(backdrop, 640.0, 480.0, 1.0);
            SetColor(backdrop, 1.0, 1.0, 1.0, 0.0);
            AnimColor(backdrop, 1.0, 1.0, 1.0, 1.0, 500.0, 0.0, ANIM_FUNC_LINEAR);
            /* the backdrop takes the background colors of the icon */
            static colors[4];
            GetOptionP(thumb, TAG("COL4"), colors, 4);
            SetOptionP(backdrop, TAG("COL4"), colors, 4);
            g_caption_label = CreateLabelShadow(Disp2D(0));
            SetPos(g_caption_label, 220.0, 205.0, 0.0, 0);
            SetColor(g_caption_label, 0.7843, 0.7843, 0.7843, 0.0);
            AnimColor(g_caption_label, 0.7843, 0.7843, 0.7843, 1.0, 200.0, 150.0, ANIM_FUNC_LINEAR);
            SetOptionI(g_caption_label, TAG("FONT"), FontID(0));
            SetOptionI(g_caption_label, TAG("HALN"), ALIGN_RIGHT);
            SetOptionI(g_caption_label, TAG("VALN"), ALIGN_CENTER);
            SetOptionI(g_caption_label, TAG("FSIZ"), 15);
            SetOptionP(g_caption_label, TAG("SCL4"), g_shadow_colors, 4);
            SetOptionI(g_caption_label, TAG("SX64"), 2 * 64);
            SetOptionI(g_caption_label, TAG("SY64"), 2 * 64);
            {
                new caption[32];
                get_tag_attr("TEXT", "msgutil_devinfo_title", "value", caption, 32);
                SetOptionS(g_caption_label, TAG("UTF8"), caption);
            }
            g_title1_label = CreateLabelShadow(Disp2D(0));
            {
                new Float:y = strcmp(g_title2, "") ? 200.0 : 210.0;
                SetPos(g_title1_label, 240.0, y, 0.0, 0);
                SetColor(g_title1_label, 0.7843, 0.7843, 0.7843, 0.0);
                AnimColor(g_title1_label, 0.7843, 0.7843, 0.7843, 1.0, 200.0, 150.0, ANIM_FUNC_LINEAR);
                SetOptionI(g_title1_label, TAG("FONT"), FontID(0));
                SetOptionI(g_title1_label, TAG("FSIZ"), 15);
                SetOptionP(g_title1_label, TAG("SCL4"), g_shadow_colors, 4);
                SetOptionI(g_title1_label, TAG("SX64"), 2 * 64);
                SetOptionI(g_title1_label, TAG("SY64"), 2 * 64);
                if (g_damaged == 1)
                {
                    new text[128];
                    get_tag_attr("TEXT", "msgutil_info_datatype_broken", "value", text, 128);
                    SetOptionS(g_title1_label, TAG("UTF8"), text);
                }
                else
                {
                    SetOptionS(g_title1_label, TAG("UTF8"), g_title1);
                }
            }
            g_title2_label = CreateLabelShadow(Disp2D(0));
            SetPos(g_title2_label, 240.0, 220.0, 0.0, 0);
            SetColor(g_title2_label, 0.7843, 0.7843, 0.7843, 0.0);
            AnimColor(g_title2_label, 0.7843, 0.7843, 0.7843, 1.0, 200.0, 150.0, ANIM_FUNC_LINEAR);
            SetOptionI(g_title2_label, TAG("FONT"), FontID(0));
            SetOptionI(g_title2_label, TAG("FSIZ"), 15);
            SetOptionP(g_title2_label, TAG("SCL4"), g_shadow_colors, 4);
            SetOptionI(g_title2_label, TAG("SX64"), 2 * 64);
            SetOptionI(g_title2_label, TAG("SY64"), 2 * 64);
            SetOptionS(g_title2_label, TAG("UTF8"), g_title2);
        }
    }
    return ret;
}

/* Runs on the icon: once it is loaded, a PS1 icon sways, a PS2 icon turns round. */
public thumb_main()
{
    new icon = Self();
    new type;
    new Float:angle;
    new light_rot[3];
    while (0 == type)
    {
        GetOptionI(icon, TAG("TYPE"), type);
        sleep;
    }
    if (type == ICON_TYPE_PS1)
    {
        for (; ; )
        {
            new Float:rot_y = 0.2 * cos(angle) - 0.1;
            new Float:rot_x = 0.2 * cos(2.0 * angle);
            SetRot(icon, rot_x, rot_y, 0.0, 0, 0);
            angle = angle - 6.28 / 480;
            sleep;
        }
    }
    else
    {
        for (; ; )
        {
            SetRot(icon, 0.0, angle, 0.0, 0, 0);
            SetOptionP(icon, TAG("ROTL"), light_rot, 3);
            angle = angle - 6.28 / 720;
            sleep;
        }
    }
    return;
}

/* Sends "INFO" to item with the value its id names: location, size, date, kind or operations of the save. */
fill_info_item(manager, item, side, mode, index, group[], text[])
{
    new unused; /* takes the parameters the function does not use */
    new unused2; /* unused, but part of the layout */
    new buf[64];
    unused = manager;
    unused = side;
    unused = mode;
    unused = index;
    unused = 64;
    copy_string(buf, group, unused);
    if (db_status < 0)
    {
        return -100000;
    }
    switch (mode)
    {
        case 0:
        {
            new device;
            new mc_type;
            if (!get_wnd_var_int("BRDEVMODE", device))
            {
                return -100000;
            }
            if (!get_wnd_var_int("BRMCPORT", mc_port))
            {
                return -100000;
            }
            if (!get_wnd_var_int("BRMCSLOT", mc_slot))
            {
                return -100000;
            }
            if (device == DEVMODE_MC)
            {
                if (!get_wnd_var_int("BRMCTYPE", mc_type))
                {
                    return -100000;
                }
            }
            new text_id[64];
            get_group_tag_attr("TEXT", group, index, "id", text_id, 64);
            if (!strcmp("msgutil_devinfo_location", text_id))
            {
                new location[64];
                switch (device)
                {
                    case DEVMODE_MC:
                    {
                        switch (mc_type)
                        {
                            case MC_TYPE_PS1:
                            {
                                new part[64];
                                get_tag_attr("TEXT", "msgutil_devinfo_location_mc", "value", part, 64);
                                sprintf(location, "%s", part);
                                switch (mc_port)
                                {
                                    case 0:
                                    {
                                        get_tag_attr("TEXT", "msgutil_devinfo_location1", "value", part, 64);
                                    }
                                    case 1:
                                    {
                                        get_tag_attr("TEXT", "msgutil_devinfo_location2", "value", part, 64);
                                    }
                                }
                                append_string(location, part, 64);
                            }
                            case MC_TYPE_PS2:
                            {
                                new part[64];
                                get_tag_attr("TEXT", "msgutil_devinfo_location_mcps2", "value", part, 64);
                                sprintf(location, "%s", part);
                                switch (mc_port)
                                {
                                    case 0:
                                    {
                                        get_tag_attr("TEXT", "msgutil_devinfo_location1", "value", part, 64);
                                    }
                                    case 1:
                                    {
                                        get_tag_attr("TEXT", "msgutil_devinfo_location2", "value", part, 64);
                                    }
                                }
                                append_string(location, part, 64);
                            }
                            case MC_TYPE_PDA:
                            {
                                new part[64];
                                get_tag_attr("TEXT", "msgutil_devinfo_location_pocket", "value", part, 64);
                                sprintf(location, "%s", part);
                                switch (mc_port)
                                {
                                    case 0:
                                    {
                                        get_tag_attr("TEXT", "msgutil_devinfo_location1", "value", part, 64);
                                    }
                                    case 1:
                                    {
                                        get_tag_attr("TEXT", "msgutil_devinfo_location2", "value", part, 64);
                                    }
                                }
                                append_string(location, part, 64);
                            }
                            default:
                            {
                                get_tag_attr("TEXT", "msgutil_dev_unknown", "value", location, 64);
                            }
                        }
                    }
                    case DEVMODE_VMC:
                    {
                        new part[64];
                        get_tag_attr("TEXT", "msgutil_devinfo_location_vmc_e", "value", part, 64);
                        sprintf(location, "%s", part);
                        switch (mc_port)
                        {
                            case 0:
                            {
                                get_tag_attr("TEXT", "msgutil_devinfo_location1", "value", part, 64);
                            }
                            case 1:
                            {
                                get_tag_attr("TEXT", "msgutil_devinfo_location2", "value", part, 64);
                            }
                        }
                        append_string(location, part, 64);
                    }
                    case DEVMODE_HDD:
                    {
                        if (avalGetNum("BrowserHDDNAME") <= 0)
                        {
                            return -1;
                        }
                        avalGetValue("BrowserHDDNAME", 0, location, 64);
                    }
                }
                copy_string(text, location, 64);
                SendMessageStr(item, TAG("INFO"), text);
            }
            else if (!strcmp("msgutil_info_size", text_id))
            {
                new format[64];
                new size_text[64];
                new unit;
                new whole;
                new fraction;
                get_size_units(g_file_size, unit, whole, fraction);
                switch (unit)
                {
                    case SIZE_MB_THOUSANDS:
                    {
                        get_tag_attr("TEXT", "msgutil_info_size_mbyte2", "value", format, 64);
                        sprintf(size_text, format, whole, fraction);
                    }
                    case SIZE_MB:
                    {
                        get_tag_attr("TEXT", "msgutil_info_size_mbyte1", "value", format, 64);
                        sprintf(size_text, format, whole);
                    }
                    case SIZE_KB_THOUSANDS:
                    {
                        get_tag_attr("TEXT", "msgutil_info_size_kbyte2", "value", format, 64);
                        sprintf(size_text, format, whole, fraction);
                    }
                    case SIZE_KB:
                    {
                        get_tag_attr("TEXT", "msgutil_info_size_kbyte1", "value", format, 64);
                        sprintf(size_text, format, whole);
                    }
                }
                copy_string(text, size_text, 64);
                SendMessageStr(item, TAG("INFO"), text);
            }
            else if (!strcmp("msgutil_info_date", text_id))
            {
                new date_text[64];
                new time_notation = GetSystemParam(SYS_TIME_NOTATION);
                new date_notation = GetSystemParam(SYS_DATE_NOTATION);
                if (time_notation < 0 || time_notation > 1)
                {
                    time_notation = 0;
                }
                if (date_notation < 0 || date_notation > 2)
                {
                    date_notation = 0;
                }
                if (device == DEVMODE_VMC
                        || (device == DEVMODE_MC && (mc_type == MC_TYPE_PDA || mc_type == MC_TYPE_PS1)) || g_year == -1
                        || g_month == -1 || g_day == -1 || g_hour == -1 || g_minute == -1 || g_second == -1)
                {
                    switch (time_notation)
                    {
                        case 0:
                        {
                            switch (date_notation)
                            {
                                case DATE_YYYYMMDD:
                                {
                                    get_tag_attr("TEXT", "msgutil_info_date_bar1", "value", date_text, 64);
                                }
                                case DATE_MMDDYYYY:
                                {
                                    get_tag_attr("TEXT", "msgutil_info_date_bar2", "value", date_text, 64);
                                }
                                case DATE_DDMMYYYY:
                                {
                                    get_tag_attr("TEXT", "msgutil_info_date_bar3", "value", date_text, 64);
                                }
                            }
                        }
                        case 1:
                        {
                            switch (date_notation)
                            {
                                case DATE_YYYYMMDD:
                                {
                                    get_tag_attr("TEXT", "msgutil_info_date_bar11", "value", date_text, 64);
                                }
                                case DATE_MMDDYYYY:
                                {
                                    get_tag_attr("TEXT", "msgutil_info_date_bar12", "value", date_text, 64);
                                }
                                case DATE_DDMMYYYY:
                                {
                                    get_tag_attr("TEXT", "msgutil_info_date_bar13", "value", date_text, 64);
                                }
                            }
                        }
                    }
                }
                else
                {
                    new format[64];
                    switch (time_notation)
                    {
                        case 0:
                        {
                            switch (date_notation)
                            {
                                case DATE_YYYYMMDD:
                                {
                                    get_tag_attr("TEXT", "msgutil_info_date_num1", "value", format, 64);
                                    sprintf(date_text, format, g_year, g_month, g_day, g_hour, g_minute, g_second);
                                }
                                case DATE_MMDDYYYY:
                                {
                                    get_tag_attr("TEXT", "msgutil_info_date_num2", "value", format, 64);
                                    sprintf(date_text, format, g_month, g_day, g_year, g_hour, g_minute, g_second);
                                }
                                case DATE_DDMMYYYY:
                                {
                                    get_tag_attr("TEXT", "msgutil_info_date_num3", "value", format, 64);
                                    sprintf(date_text, format, g_day, g_month, g_year, g_hour, g_minute, g_second);
                                }
                            }
                        }
                        case 1:
                        {
                            new ampm[1]; /* "AM" or "PM", packed */
                            if (g_hour >= 12)
                            {
                                g_hour = g_hour - 12; /* NOTE: bug: noon and midnight show as 0, not 12 */
                                sprintf(ampm, "PM");
                            }
                            else
                            {
                                sprintf(ampm, "AM");
                            }
                            switch (date_notation)
                            {
                                case DATE_YYYYMMDD:
                                {
                                    get_tag_attr("TEXT", "msgutil_info_date_num11", "value", format, 64);
                                    sprintf(date_text, format, g_year, g_month, g_day, g_hour, g_minute, g_second,
                                            ampm);
                                }
                                case DATE_MMDDYYYY:
                                {
                                    get_tag_attr("TEXT", "msgutil_info_date_num12", "value", format, 64);
                                    sprintf(date_text, format, g_month, g_day, g_year, g_hour, g_minute, g_second,
                                            ampm);
                                }
                                case DATE_DDMMYYYY:
                                {
                                    get_tag_attr("TEXT", "msgutil_info_date_num13", "value", format, 64);
                                    sprintf(date_text, format, g_day, g_month, g_year, g_hour, g_minute, g_second,
                                            ampm);
                                }
                            }
                        }
                    }
                }
                copy_string(text, date_text, 64);
                SendMessageStr(item, TAG("INFO"), text);
            }
            else if (!strcmp("msgutil_info_datatype", text_id))
            {
                new kind_text[64];
                if (0 == g_damaged)
                {
                    switch (g_save_kind)
                    {
                        case KIND_PS1_SAVE:
                        {
                            get_tag_attr("TEXT", "msgutil_info_datatype_saveps1", "value", kind_text, 64);
                        }
                        case KIND_PS2_SAVE:
                        {
                            get_tag_attr("TEXT", "msgutil_info_datatype_saveps2", "value", kind_text, 64);
                        }
                        case KIND_PDA_SAVE:
                        {
                            get_tag_attr("TEXT", "msgutil_info_datatype_pocket", "value", kind_text, 64);
                        }
                        case KIND_PS2_SOFTWARE:
                        {
                            get_tag_attr("TEXT", "msgutil_info_datatype_ps2software", "value", kind_text, 64);
                        }
                        case KIND_UNKNOWN:
                        {
                            get_tag_attr("TEXT", "msgutil_info_datatype_unknown", "value", kind_text, 64);
                        }
                        case KIND_PDA:
                        {
                            get_tag_attr("TEXT", "msgutil_info_datatype_pocket", "value", kind_text, 64);
                        }
                        case KIND_SETTING:
                        {
                            get_tag_attr("TEXT", "msgutil_info_datatype_setting", "value", kind_text, 64);
                        }
                        case KIND_SYSTEM_DRIVER:
                        {
                            get_tag_attr("TEXT", "msgutil_info_datatype_systemdrv", "value", kind_text, 64);
                        }
                        default:
                        {
                            get_tag_attr("TEXT", "msgutil_info_datatype_unknown", "value", kind_text, 64);
                        }
                    }
                }
                else
                {
                    get_tag_attr("TEXT", "msgutil_info_datatype_broken", "value", kind_text, 64);
                }
                copy_string(text, kind_text, 64);
                SendMessageStr(item, TAG("INFO"), text);
            }
            else if (!strcmp("msgutil_info_ope", text_id))
            {
                new ops_text[64];
                new copy_text[32];
                new move_text[32];
                if (0 == g_damaged)
                {
                    if (g_no_copy == 1)
                    {
                        get_tag_attr("TEXT", "msgutil_info_ope_copy_prohibit", "value", copy_text, 32);
                        get_tag_attr("TEXT", "msgutil_info_ope_move_prohibit", "value", move_text, 32);
                        sprintf(ops_text, "%s %s", copy_text, move_text);
                    }
                    else
                    {
                        get_tag_attr("TEXT", "msgutil_info_ope_none", "value", ops_text, 64);
                    }
                }
                else
                {
                    get_tag_attr("TEXT", "msgutil_info_ope_copy_prohibit", "value", copy_text, 32);
                    get_tag_attr("TEXT", "msgutil_info_ope_move_prohibit", "value", move_text, 32);
                    sprintf(ops_text, "%s %s", copy_text, move_text);
                }
                copy_string(text, ops_text, 64);
                SendMessageStr(item, TAG("INFO"), text);
            }
            else
            {
                SendMessageStr(item, TAG("INFO"), "NOITEM");
            }
        }
    }
    return 0;
}

/* Handles a push: the cross fades out the icon and the titles and closes the menu. */
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
            case 0:
            {
            }
        }
        mode = 0;
        result = INFOMENU_STAY;
    }
    if (button == TAG("RDWN"))
    {
        switch (mode)
        {
            case 0:
            {
                printf("infomenu btn RDWN\n");
                if (thumb)
                {
                    AnimColor(thumb, 1.0, 1.0, 1.0, 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
                }
                mode = 0;
                result = INFOMENU_CLOSE;
            }
        }
        AnimColor(g_caption_label, 0.7843, 0.7843, 0.7843, 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(g_title1_label, 0.7843, 0.7843, 0.7843, 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(g_title2_label, 0.7843, 0.7843, 0.7843, 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    }
    return result;
}

/*
 * Copies the arguments BR* into window variables, shows the menu and leaves its outcome in the album variables
 * InfoStatusMC (NONE, or MCEJECT when the card was removed) and InfoStatus (NONE or ERROR).
 */
public main()
{
    new result = INFOMENU_CLOSE;
    focus_glowfly = create_focus_plane("focus");
    SetClipRange(Camera(0), 100.0, 32767.0);
    {
        new value[64];
        GetArg("BRDEVMODE", value, 64);
        set_wnd_var_int("BRDEVMODE", atoi(value));
        GetArg("BRMCPORT", value, 64);
        set_wnd_var_int("BRMCPORT", atoi(value));
        GetArg("BRMCSLOT", value, 64);
        set_wnd_var_int("BRMCSLOT", atoi(value));
        GetArg("BRMCTYPE", value, 64);
        set_wnd_var_int("BRMCTYPE", atoi(value));
        GetArg("BRICONCACHE", value, 64);
        set_wnd_var_int("BRICONCACHE", atoi(value));
        GetArg("BRDAMAGED", value, 64);
        set_wnd_var_int("BRDAMAGED", atoi(value));
    }
    result = pop_info_menu("menu_title", "info_thumb", 0);
    new ejected;
    get_wnd_var_int("BRMCEJECT", ejected);
    if (ejected == 1)
    {
        avalSet("InfoStatusMC", "MCEJECT");
        goto set_status;
    }
    avalSet("InfoStatusMC", "NONE");
    set_status:
    if (result == INFOMENU_ERROR)
    {
        avalSet("InfoStatus", "ERROR");
    }
    else
    {
        avalSet("InfoStatus", "NONE");
    }
    Destroy(Scene(0));
}
