/*
 * /opt0/bn/script/game/infomenu.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The information page of a game: a general_menu of the TEXT elements of a group of the page (title, type, date,
 * size, ...), filled from the row "select" of the game database, with its icon. A menu item can open another menu.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

#include "gamedb.inc"

#include "infomenu.inc"
#include "genmenu.inc"
#include "scroll.inc"

#include "wait.inc"

#include "tag.inc"

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

#include "text.inc"

#include "wndvar.inc"

#include "misc.inc"

new g_db_name[] = "game";
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
    item_count = item_count + load_game_info(side, result, mode, title, group, id, selected);
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

public focus_glowfly; /* the focus object */
public select; /* the row of the game in the result of the query */
public db_status; /* -1 if there is no game */
public thumb; /* the icon of the game, an info_thumb plane */
new g_sql[128];
/* Scratch text: the fields of the database and the TEXT formats the items are built from. */
new g_text[64];
new g_text2[32];
new g_sort_order; /* SORT_* */

/*
 * Opens the game database, adding the row that keeps the sort order if it is missing, lists the games and shows the
 * icon of the selected one; returns the number of menu items it adds, none.
 */
load_game_info(side, result, mode, title[], group[], id[], &selected)
{
    new jackets[8];
    new extra_items;
    new status;
    new count;
    g_sort_order = SORT_NEWEST;
    status = datOpen(DAT_DATABASE, g_db_name, DAT_ORDER_MTIME_DESC);
    if (0 != status)
    {
        return 0;
    }
    status = wait_database();
    if (status < 0)
    {
        return 0;
    }
    sprintf(g_sql, "select * from sce_game where content_attr = %d", ATTR_SETTINGS);
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
    if (datGetNum() <= 0)
    {
        sprintf(g_sql,
                "insert into sce_game values(%d,%d,%d,'%s','%s','%s','%s','%s','%s','%s','%s','%s',%d,%d,'%s','%s',%d,%d,%d,%d,%d,%d,'%s',%d,'%s','%s','%s',%d,%d,%d,%d)",
                0, 0, 0, "", "", "", "", "", "", "", "", "", 512, 0, "", "", 0, 0, 0, 0, 0, 0, "", 0, "", "", "", 0,
                g_sort_order, 0, ATTR_SETTINGS);
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
    }
    else
    {
        datGetData(0, COL_CONTENT_TYPE, g_sql, 128);
        g_sort_order = atoi(g_sql);
        if (g_sort_order < 0 || g_sort_order > 1)
        {
            g_sort_order = SORT_NEWEST;
        }
    }
    if (SORT_NEWEST == g_sort_order)
    {
        g_sql = "select * from sce_game where content_attr = 0 order by install_date desc";
    }
    else
    {
        g_sql = "select * from sce_game where content_attr = 0 order by install_date asc";
    }
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
    count = datGetNum();
    if (count <= 0)
    {
        db_status = -1;
        return 0;
    }
    thumb = CreatePlane(Disp2D(0));
    AttachScript(thumb, "info_thumb", "info_thumb");
    SetColor(thumb, 1.0, 1.0, 1.0, 0.0);
    get_wnd_var_int2("select", select);
    datGetData(select, COL_IMAGE_COUNT, jackets, 8);
    datGetData(select, COL_RESOURCE_PATH, g_text, 64);
    datGetData(select, COL_TITLE, g_text2, 32);
    if (strlen(g_text2) < 1)
    {
        get_tag_attr("IMG", "broken_icon", "src", g_text, 64);
    }
    else
    {
        jackets = "1";
        if (atoi(jackets) >= 1 && atoi(jackets) <= 999)
        {
            append_string(g_text, "/jkt_001.png", 64);
            new file_mode;
            new file_size;
            if (0 != fileLstat(g_text, file_mode, file_size))
            {
                get_tag_attr("IMG", "default_icon", "src", g_text, 64);
            }
        }
        else
        {
            get_tag_attr("IMG", "default_icon", "src", g_text, 64);
        }
    }
    SendMessageStr(thumb, TAG("DCDE"), g_text);
    return extra_items;
}

/*
 * Sends "INFO" to item with the value its id names, read from the selected game: columns 3 size in MB, 4 title,
 * 5 name, 9 copyright, 12 date, 30 type.
 */
fill_info_item(manager, item, side, mode, index, group[], text[])
{
    new item_id[64];
    new title[256];
    new row;
    if (db_status < 0)
    {
        return;
    }
    row = GetPubVar(manager, "select");
    title = "";
    datGetData(row, COL_TITLE, title, 256);
    get_group_tag_attr2("TEXT", group, index, "id", item_id, 64);
    if (!strcmp("INFO_TITLE", item_id))
    {
        datGetData(row, COL_TITLE, g_text, 64);
        if (strlen(g_text) <= 0)
        {
            get_tag_attr("TEXT", "msggame_info_corrupted", "value", g_text, 64);
        }
        sprintf(text, "%s", g_text);
        SendMessageStr(item, TAG("INFO"), text);
    }
    else if (!strcmp("INFO_COPY", item_id))
    {
        if (strlen(title) <= 0)
        {
            get_tag_attr("TEXT", "msggame_unknown", "value", g_text, 64);
        }
        else
        {
            datGetData(row, COL_PUBLISHER_ID, g_text, 64);
            if (strlen(g_text) <= 0)
            {
                get_tag_attr("TEXT", "msggame_unknown", "value", g_text, 64);
            }
        }
        sprintf(text, "%s", g_text);
        SendMessageStr(item, TAG("INFO"), text);
    }
    else if (!strcmp("INFO_NAME", item_id))
    {
        if (strlen(title) <= 0)
        {
            get_tag_attr("TEXT", "msggame_unknown", "value", g_text, 64);
        }
        else
        {
            datGetData(row, COL_TITLE_ID, g_text, 64);
            if (strlen(g_text) <= 0)
            {
                get_tag_attr("TEXT", "msggame_unknown", "value", g_text, 64);
            }
        }
        sprintf(text, "%s", g_text);
        SendMessageStr(item, TAG("INFO"), text);
    }
    else if (!strcmp("INFO_TYPE", item_id))
    {
        if (strlen(title) <= 0)
        {
            get_tag_attr("TEXT", "msggame_info_corrupted", "value", g_text, 64);
        }
        else
        {
            datGetData(row, COL_CONTENT_SUBTYPE, g_text, 64);
            new type = atoi(g_text);
            if (SUBTYPE_PS2 == type)
            {
                get_tag_attr("TEXT", "msggame_type_ps2", "value", g_text, 64);
            }
            else if (type == SUBTYPE_PS1)
            {
                get_tag_attr("TEXT", "msggame_type_ps1", "value", g_text, 64);
            }
            else
            {
                get_tag_attr("TEXT", "msggame_unknown", "value", g_text, 64);
            }
        }
        sprintf(text, "%s", g_text);
        SendMessageStr(item, TAG("INFO"), text);
    }
    else if (!strcmp("INFO_DATE", item_id))
    {
        datGetData(row, COL_INSTALL_DATE, g_text, 64);
        if (strlen(g_text) <= 0)
        {
            get_tag_attr("TEXT", "msggame_unknown", "value", g_text, 64);
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
        }
        sprintf(text, "%s %s", g_text, g_text2);
        SendMessageStr(item, TAG("INFO"), text);
    }
    else if (!strcmp("INFO_SIZE", item_id))
    {
        datGetData(row, COL_INST_SIZE, g_text, 64);
        new size_mb = atoi(g_text);
        if (size_mb < 0)
        {
            get_tag_attr("TEXT", "msggame_unknown", "value", g_text, 64);
            sprintf(text, "%s", g_text);
        }
        else if (size_mb < 1000)
        {
            get_tag_attr("TEXT", "msggame_info_size_mega1", "value", g_text2, 32);
            sprintf(text, g_text2, size_mb);
        }
        else
        {
            get_tag_attr("TEXT", "msggame_info_size_mega2", "value", g_text2, 32);
            sprintf(text, g_text2, size_mb / 1000, size_mb % 1000);
        }
        SendMessageStr(item, TAG("INFO"), text);
    }
    else
    {
        SendMessageStr(item, TAG("INFO"), "--");
    }
}

/* Handles a push of item index: circle stays in the menu, cross closes it. */
handle_push(manager, item, side, &mode, index, button, title[], group[], id[])
{
    new result = INFOMENU_REOPEN;
    if (db_status < 0)
    {
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
                result = INFOMENU_CLOSE;
            }
        }
    }
    return result;
}

public main()
{
    new result = INFOMENU_CLOSE;
    focus_glowfly = create_focus_plane2("focus");
    GetArg("select", g_text, 64);
    select = 0;
    select = atoi(g_text);
    set_wnd_var_int("select", select);
    result = pop_info_menu("menu_title", "info", INFOMENU_SIDE_LEFT);
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
