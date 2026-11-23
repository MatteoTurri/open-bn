/*
 * /opt0/bn/script/photo/menu_process.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The work of the option menus of photo on a photo or an album: move, copy, import from a USB device, add to a
 * favorite album. The page attaches one of the *_proc functions with the argument dst_album and sends it "SLCT"
 * (the index of the photo); the result goes to menu_ret_val of the parent after a message.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1560

#include "photo.inc"

/*
 * The kinds of album (the window variable "albummode", album_kind of get_photo_field): a list of albums (ALBUM_*) or
 * the photos of an album of that kind (PHOTOS_* = ALBUM_* + PHOTOS_LIBRARY).
 */
#include "photodb.inc"

/* Same as TAG of tag.inc. */
TAG(fourcc[])
{
    return swapchars(fourcc[0]);
}

/* Sleeps until a message msg arrives. */
wait_message(msg, &a, &b=0, &c=0, &d=0)
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

/* Copies the first count characters of the packed string src into dest (maxlen cells). */
copy_head(dest[], src[], count, maxlen)
{
    new len = strlen(src);
    if (!is_packed(src))
    {
        count = count * 4;
    }
    count = min(count, maxlen * 4);
    count = min(count, len);
    new cells = count / 4;
    new rest = count % 4;
    for (new i; i < cells; i++)
    {
        dest[i] = src[i];
    }
    for (new j = cells * 4; j - cells * 4 < rest; j++)
    {
        dest{j} = src{j};
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
get_tag_attr(tag[], const id[], attr[], value[], maxlen)
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

new g_photo_db_name[] = "photo";

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

/*
 * Shows the message of result, a TEXT id and an optional second line: msg0 for 0 or more, msg1 to msg5 for -1 to
 * -5; returns 1 if there was one to show.
 */
show_result_message(result, msg0[], detail0[] = "", msg1[], detail1[] = "", msg2[], detail2[] = "", msg3[],
        detail3[] = "", msg4[], detail4[] = "", msg5[], detail5[] = "")
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

public focus; /* the focus of the dialogs */

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

/*
 * Opens a progress_window with the TEXT of msg, msg2 and msg3 and, if given, a cancel button labelled with the TEXT
 * of cancel; returns its object.
 */
open_progress_window(msg[], const msg2[] = "", const msg3[] = "", const cancel[] = "", const breakflag[] = "")
{
    new window = CreateNull(Disp2D(0));
    {
        new args[256];
        new text[64];
        new arg[64];
        get_tag_attr("TEXT", msg, "value", text, 64);
        sprintf(args, "msg=%s;", text);
        if (strlen(msg2) > 0)
        {
            get_tag_attr("TEXT", msg2, "value", text, 64);
            sprintf(arg, "msg2=%s;", text);
            append_string(args, arg, 256);
        }
        if (strlen(msg3) > 0)
        {
            get_tag_attr("TEXT", msg3, "value", text, 64);
            sprintf(arg, "msg3=%s;", text);
            append_string(args, arg, 256);
        }
        if (strlen(cancel) > 0)
        {
            get_tag_attr("TEXT", cancel, "value", text, 64);
            sprintf(arg, "cancel=%s;", text);
            append_string(args, arg, 256);
        }
        if (strlen(breakflag) > 0)
        {
            get_tag_attr("TEXT", cancel, "value", text, 64); /* NOTE: bug: the text of cancel, not of breakflag */
            sprintf(arg, "breakflag=%s;", text);
            append_string(args, arg, 256);
        }
        SetPos(window, 320.0, 240.0, 0.0, 0);
        AttachScript(window, _, "progress_window", args);
        SetPubVar(window, "manager", Self());
        SetPubVar(window, "focus", focus);
    }
    return window;
}

/* Same as find_last_nocase of string.inc. */
find_last_nocase(str[], sub[])
{
    new len = strlen(str);
    new sub_len = strlen(sub);
    new skip;
    new j;
    new matched;
    while (skip < len)
    {
        while (skip < len)
        {
            if (toupper(str{len - 1 - skip}) == toupper(sub{sub_len - 1}))
            {
                break;
            }
            skip++;
        }
        if (skip >= len)
        {
            return -1;
        }
        matched = 1;
        j = 0;
        while (skip + j < len && j < sub_len && matched)
        {
            if (toupper(str{len - 1 - skip - j}) != toupper(sub{sub_len - 1 - j}))
            {
                matched = 0;
                skip++;
                break;
            }
            j++;
        }
        if (matched)
        {
            return len - skip - j;
        }
    }
    return -1;
}

/* Same as copy_tail of string.inc. */
copy_tail(dest[], src[], start, maxlen)
{
    new i;
    new max_chars;
    new count = strlen(src) - start;
    max_chars = maxlen * 4;
    i = 0;
    for (; i < count && i < max_chars; i++)
    {
        dest{i} = src{i + start};
    }
    return;
}

/* Same as get_extension of string.inc. */
get_extension(dest[], path[], maxlen)
{
    new count;
    new i;
    new max_chars;
    new dot = find_last_nocase(path, ".");
    count = strlen(path) - dot;
    max_chars = maxlen * 4;
    i = 0;
    for (; i < count && i < max_chars; i++)
    {
        dest{i} = path{i + dot};
    }
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

/* Same as replace_quotes of string.inc. */
replace_quotes(text[])
{
    new len = strlen(text);
    for (new i; i < len; i++)
    {
        if (text{i} == '\'')
        {
            text{i} = '0';
        }
    }
    return;
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
 * Makes url the name of a file that does not exist yet: its quotes become 0, and while the file is there a number
 * goes before the extension; returns 0, or -2 on an error.
 */
make_unique_url(url[], maxlen)
{
    new status;
    new n;
    new ext[4];
    new base[128];
    replace_quotes(url);
    new dot = find_last_nocase(url, ".");
    get_extension(ext, url, 4);
    copy_head(base, url, dot, 128);
    for (; ; )
    {
        new st_mode;
        new st_size;
        status = fileLstat(url, st_mode, st_size);
        printf("dst_url check %d , %s\n", status, url);
        if (status == -2)
        {
            break;
        }
        else
        {
            if (status < 0)
            {
                return -2;
            }
            sprintf(url, "%s%d%s", base, n, ext, maxlen);
            n++;
        }
        sleep;
    }
    return 0;
}

/* Same as wait_file_op of fileop.inc. */
wait_file_op(&status, &error, progress_win, from, to)
{
    new percent;
    for (; ; )
    {
        status = fileSync(percent, error);
        if (progress_win)
        {
            SetPubVar(progress_win, "progress", from + percent * (to - from) / 100);
        }
        if (status < 0)
        {
            return -1;
        }
        if (status == FILESYNC_DONE)
        {
            if (0 != error)
            {
                return -1;
            }
            break;
        }
        sleep;
    }
    return 0;
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

/*
 * Checks that adding more photos fit in the album album_uri (kind ALBUM_LIBRARY or ALBUM_FAVORITE), which holds at
 * most MAX_ITEMS; returns 0, -3 if they do not, -1 on an error.
 */
check_album_room(album_uri[], kind, adding)
{
    new status;
    new sql[128];
    if (ALBUM_LIBRARY == kind)
    {
        sprintf(sql, "SELECT content_uri FROM sce_photo WHERE info_type = 1 AND album_uri = '%s'", album_uri);
    }
    else if (kind == ALBUM_FAVORITE)
    {
        sprintf(sql, "SELECT content_uri FROM sce_photo WHERE info_type = 2 AND album_uri = '%s'", album_uri);
    }
    else
    {
        return -1;
    }
    printf("sql com = %s\n", sql);
    status = datSQLExec(sql);
    if (status < 0)
    {
        return -1;
    }
    status = wait_database();
    if (status < 0)
    {
        return -1;
    }
    new count = datGetNum();
    if (count + adding > MAX_ITEMS)
    {
        return -3;
    }
    if (count < 0)
    {
        return -1;
    }
    return 0;
}

/* The work directories of pfs.inc, and the status of fileLstat for a missing file. */
new g_pfs_contents[] = "pfs:/__contents/";
new g_contents_dir[] = "pfs:/__contents/contents/";
new g_photo_dir[] = "pfs:/__contents/contents/photo/";
new g_photo_tmp_dir[] = "pfs:/__contents/contents/photo/tmp/";
new g_not_found = -2;

/* Same as check_pfs_free of pfs.inc. */
check_pfs_free(need_kb)
{
    new ret;
    new free_kb;
    ret = fileGetFreeSize(g_pfs_contents, free_kb);
    printf("check pfs free %d Mb, req %d Kbyte\n", free_kb / 1024, need_kb);
    if (ret < 0)
    {
        return -2;
    }
    if (free_kb <= need_kb)
    {
        return -4;
    }
    return 0;
}

/* Same as make_photo_dirs of pfs.inc. */
make_photo_dirs()
{
    new st_mode;
    new st_size;
    new ret = fileLstat(g_contents_dir, st_mode, st_size);
    if (ret == g_not_found)
    {
        ret = fileMkdir(g_contents_dir);
        if (ret < 0)
        {
            return -2;
        }
    }
    else if (ret < 0)
    {
        return -2;
    }
    ret = fileLstat(g_photo_dir, st_mode, st_size);
    if (ret == g_not_found)
    {
        ret = fileMkdir(g_photo_dir);
        if (ret < 0)
        {
            return -2;
        }
    }
    else if (ret < 0)
    {
        return -2;
    }
    return 0;
}

/* Same as prepare_photo_tmp of pfs.inc. */
prepare_photo_tmp(progress_win, from, to)
{
    printf("check filetmp\n");
    new st_mode;
    new st_size;
    new ret = make_photo_dirs();
    if (ret < 0)
    {
        return -2;
    }
    ret = fileLstat(g_photo_tmp_dir, st_mode, st_size);
    if (ret == g_not_found)
    {
        ret = fileMkdir(g_photo_tmp_dir);
        if (ret < 0)
        {
            return -2;
        }
    }
    else if (ret >= 0)
    {
        ret = fileDelete(g_photo_tmp_dir);
        if (ret < 0)
        {
            return -2;
        }
        new error;
        if (wait_file_op(ret, error, progress_win, from, to) < 0)
        {
            if (ret < 0)
            {
                return -2;
            }
        }
        ret = fileMkdir(g_photo_tmp_dir);
        if (ret < 0)
        {
            return -2;
        }
    }
    else if (ret < 0)
    {
        return -2;
    }
    return 0;
}

/* Same as clear_photo_tmp of pfs.inc. */
clear_photo_tmp(progress_win, from, to)
{
    printf("clear filetmp\n");
    new st_mode;
    new st_size;
    new ret = fileLstat(g_photo_tmp_dir, st_mode, st_size);
    if (ret >= 0)
    {
        ret = fileDelete(g_photo_tmp_dir);
        if (ret < 0)
        {
            return -2;
        }
        new error;
        if (wait_file_op(ret, error, progress_win, from, to) < 0)
        {
            if (ret < 0)
            {
                return -2;
            }
        }
    }
    else
    {
        if (ret == g_not_found)
        {
            return 0;
        }
        if (ret < 0)
        {
            return -2;
        }
    }
    return 0;
}

/* Same as copy_to_photo_tmp of validate.inc. */
copy_to_photo_tmp(dest[], src[], missing_ok, ignored, progress_win, from, to)
{
    printf("copy to filetmp\n");
    new st_mode;
    new st_size;
    new ret = fileLstat(src, st_mode, st_size);
    if (ret == g_not_found)
    {
        if (missing_ok)
        {
            return 1;
        }
        return -2;
    }
    new name[64];
    new slash = find_last_nocase(src, "/");
    copy_tail(name, src, slash + 1, 64);
    sprintf(dest, "%s%s", g_photo_tmp_dir, name, ignored);
    printf("COPY to TMP name = %s\n", src);
    new error;
    ret = fileCopy(src, dest);
    if (ret < 0)
    {
        return -2;
    }
    if (wait_file_op(ret, error, progress_win, from, to) < 0)
    {
        if (ret < 0)
        {
            return -2;
        }
        if (error == 2)
        {
            return -4;
        }
    }
    return 0;
}

/* Same as move_from_photo_tmp of validate.inc. */
move_from_photo_tmp(dest[], src[], missing_ok)
{
    printf("move from filetmp\n");
    new st_mode;
    new st_size;
    new ret = fileLstat(src, st_mode, st_size);
    if (ret == g_not_found)
    {
        if (missing_ok)
        {
            return 1;
        }
        return -2;
    }
    printf("MOVEfrom DST name = %s\n", dest);
    ret = fileMove(src, dest);
    if (ret < 0)
    {
        return -2;
    }
    if (wait_file_op(ret, st_mode, 0, 0, 100) < 0)
    {
        if (ret < 0)
        {
            return -2;
        }
    }
    return 0;
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

/*
 * Moves photo item of the open album to the album dst_album: the file through the temporary directory, then its
 * row; the covers of both albums are set again.
 */
public content_move_proc()
{
    new item;
    wait_message(TAG("SLCT"), item);
    new dst_album[128];
    GetArg("dst_album", dst_album, 128);
    printf("move proc, dest album = %s\n", dst_album);
    new ret;
    new progress_win;
    ret = open_album_table(PHOTOS_LIBRARY, 0);
    if (ret < 0)
    {
        ret = -1;
        goto done;
    }
    ret = check_album_room(dst_album, ALBUM_LIBRARY, 1);
    if (ret < 0)
    {
        ret = -3;
        goto done;
    }
    progress_win = open_progress_window("msgphoto_move");
    ret = select_album_rows(PHOTOS_LIBRARY, 0);
    if (ret < 0)
    {
        ret = -1;
        goto done;
    }
    new progress;
    printf("selected item %d ------------------\n", item);
    {
        new src_url[128];
        get_photo_field(item, FIELD_URL, src_url, PHOTOS_LIBRARY, 0, 128);
        {
            new sql[256];
            new tmp_url[64];
            ret = prepare_photo_tmp(0, 0, 0);
            if (ret < 0)
            {
                goto done;
            }
            ret = copy_to_photo_tmp(tmp_url, src_url, 0, 64, 0, 0, 0);
            if (ret < 0)
            {
                goto done;
            }
            new dst_url[64];
            {
                new file_name[32];
                new slash = find_last_nocase(src_url, "/");
                copy_tail(file_name, src_url, slash + 1, 32);
                copy_string(dst_url, dst_album, 64);
                append_string(dst_url, file_name, 64);
                new st_mode;
                new st_size;
                if (fileLstat(dst_album, st_mode, st_size) == g_not_found)
                {
                    ret = fileMkdir(dst_album);
                    if (ret < 0)
                    {
                        ret = -2;
                        goto done;
                    }
                }
            }
            ret = make_unique_url(dst_url, 64);
            if (ret < 0)
            {
                goto done;
            }
            {
                new create_date[16];
                new year;
                new month;
                new day;
                new hour;
                new minute;
                new second;
                date(year, month, day, 1);
                time(hour, minute, second, 1);
                sprintf(create_date, "%04d/%02d/%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
                new album_name_sql[128];
                sprintf(album_name_sql, "SELECT album_name FROM sce_photo WHERE album_uri = '%s' AND info_type = 3",
                        dst_album);
                sprintf(sql,
                        "INSERT INTO sce_photo ( info_type, album_name, title, content_uri, album_uri, create_date ) SELECT 1, ( %s ), title, '%s', '%s', '%s' FROM sce_photo WHERE info_type = 1 AND content_uri = '%s'",
                        album_name_sql, dst_url, dst_album, create_date, src_url);
                printf("sql com = %s\n", sql);
                ret = datSQLExec(sql);
                if (ret < 0)
                {
                    ret = -1;
                    goto done;
                }
                ret = wait_database();
                if (ret < 0)
                {
                    ret = -1;
                    goto done;
                }
            }
            progress = progress + 33;
            SetPubVar(progress_win, "progress", progress);
            ret = move_from_photo_tmp(dst_url, tmp_url, 0);
            if (ret < 0)
            {
                goto done;
            }
            ret = fileDelete(src_url);
            if (ret < 0)
            {
                ret = -2;
                goto done;
            }
            new error;
            if (wait_file_op(ret, error, progress_win, progress, progress + 33) < 0)
            {
                if (ret < 0)
                {
                    ret = -2;
                    goto done;
                }
            }
            sprintf(sql, "DELETE FROM sce_photo WHERE content_uri = '%s' AND info_type =1 ", src_url);
            printf("sql com = %s\n", sql);
            ret = datSQLExec(sql);
            if (ret < 0)
            {
                ret = -1;
                goto done;
            }
            ret = wait_database();
            if (ret < 0)
            {
                ret = -1;
                goto done;
            }
            {
                new album_name_sql[128];
                sprintf(album_name_sql, "SELECT album_name FROM sce_photo WHERE album_uri = '%s' AND info_type = 3",
                        dst_album);
                sprintf(sql,
                        "UPDATE sce_photo SET album_name = (%s), content_uri = '%s' WHERE content_uri = '%s' AND info_type <= 2 ",
                        album_name_sql, dst_url, src_url);
            }
            printf("sql com = %s\n", sql);
            ret = datSQLExec(sql);
            if (ret < 0)
            {
                ret = -1;
                goto done;
            }
            ret = wait_database();
            if (ret < 0)
            {
                ret = -1;
                goto done;
            }
            progress = progress + 33;
            SetPubVar(progress_win, "progress", progress);
            clear_album_ids();
            add_album_id(dst_album);
            ret = collect_photo_albums(src_url, 0);
            if (ret < 0)
            {
                ret = -1;
                goto done;
            }
            ret = update_album_covers(ALBUM_LIBRARY);
            if (ret < 0)
            {
                ret = -1;
                goto done;
            }
            ret = clear_photo_tmp(0, 0, 0);
            if (ret < 0)
            {
                goto done;
            }
        }
        ret = 0;
    }
    done:
    if (progress_win)
    {
        SetPubVar(progress_win, "progress", 100);
        while (SCRIPT_FINISHED != ScriptState(progress_win))
        {
            sleep;
        }
        Destroy(progress_win);
    }
    show_result_message(ret, "msgphoto_move_complete", _, "msgphoto_move_fail", _, "msgphoto_move_fail", _,
            "msgphoto_move_warn_filemax", _, "msgphoto_copy_warn_freespace", _, "");
    SetPubVar(Parent(0), "menu_ret_val", ret);
    return ret;
}

/* Copies photo item of the open album to the album dst_album; the result is 1 when dst_album is the open album. */
public content_copy_proc()
{
    new item;
    wait_message(TAG("SLCT"), item);
    new dst_album[128];
    GetArg("dst_album", dst_album, 128);
    printf("copy proc , dest album = %s\n", dst_album);
    new ret;
    new progress_win;
    {
        new list_path[128];
        new st_mode;
        new size_kb;
        ret = photoListGet(item, list_path, 128);
        if (ret < 0)
        {
            ret = -2;
            goto done;
        }
        ret = fileLstat(list_path, st_mode, size_kb);
        if (ret != g_not_found)
        {
            size_kb = (size_kb + 8191) / 8192 * 8;
        }
        else if (ret < 0)
        {
            ret = -2;
            goto done;
        }
        ret = check_pfs_free(size_kb);
        if (ret < 0)
        {
            goto done;
        }
    }
    ret = open_album_table(PHOTOS_LIBRARY, 0);
    if (ret < 0)
    {
        ret = -1;
        goto done;
    }
    ret = check_album_room(dst_album, ALBUM_LIBRARY, 1);
    if (ret < 0)
    {
        ret = -3;
        goto done;
    }
    progress_win = open_progress_window("msgphoto_copy");
    ret = select_album_rows(PHOTOS_LIBRARY, 0);
    if (ret < 0)
    {
        ret = -1;
        goto done;
    }
    new progress;
    printf("selected item %d ------------------\n", item);
    {
        new src_url[128];
        new title[128];
        get_photo_field(item, FIELD_URL, src_url, PHOTOS_LIBRARY, 0, 128);
        {
            new raw_title[64];
            get_photo_field(item, FIELD_TITLE, raw_title, PHOTOS_LIBRARY, 0, 64);
            escape_quotes(title, raw_title, 128);
        }
        new tmp_url[64];
        ret = prepare_photo_tmp(0, 0, 0);
        if (ret < 0)
        {
            goto done;
        }
        ret = copy_to_photo_tmp(tmp_url, src_url, 0, 64, 0, 0, 0);
        if (ret < 0)
        {
            goto done;
        }
        new dst_url[64];
        {
            new file_name[32];
            new slash = find_last_nocase(src_url, "/");
            copy_tail(file_name, src_url, slash + 1, 32);
            copy_string(dst_url, dst_album, 64);
            append_string(dst_url, file_name, 64);
            new st_mode;
            new st_size;
            if (fileLstat(dst_album, st_mode, st_size) == g_not_found)
            {
                ret = fileMkdir(dst_album);
                if (ret < 0)
                {
                    ret = -2;
                    goto done;
                }
            }
        }
        ret = make_unique_url(dst_url, 64);
        if (ret < 0)
        {
            goto done;
        }
        {
            new create_date[16];
            new year;
            new month;
            new day;
            new hour;
            new minute;
            new second;
            date(year, month, day, 1);
            time(hour, minute, second, 1);
            sprintf(create_date, "%04d/%02d/%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
            new sql[256];
            new album_name_sql[128];
            sprintf(album_name_sql, "SELECT album_name FROM sce_photo WHERE album_uri = '%s' AND info_type = 3",
                    dst_album);
            sprintf(sql,
                    "INSERT INTO sce_photo ( info_type, album_name, title, content_uri, album_uri, create_date ) SELECT 1, ( %s ), title, '%s', '%s', '%s' FROM sce_photo WHERE info_type = 1 AND content_uri = '%s'",
                    album_name_sql, dst_url, dst_album, create_date, src_url);
            printf("sql com = %s\n", sql);
            ret = datSQLExec(sql);
            if (ret < 0)
            {
                ret = -1;
                goto done;
            }
            ret = wait_database();
            if (ret < 0)
            {
                ret = -1;
                goto done;
            }
        }
        progress = progress + 33;
        SetPubVar(progress_win, "progress", progress);
        ret = move_from_photo_tmp(dst_url, tmp_url, 0);
        if (ret < 0)
        {
            goto done;
        }
        progress = progress + 33;
        SetPubVar(progress_win, "progress", progress);
        clear_album_ids();
        add_album_id(dst_album);
        ret = update_album_covers(ALBUM_LIBRARY);
        if (ret < 0)
        {
            ret = -1;
            goto done;
        }
        ret = clear_photo_tmp(0, 0, 0);
        if (ret < 0)
        {
            goto done;
        }
        {
            new current_album[64];
            avalGetValue("PhotoAccessAlbum", 0, current_album, 64);
            if (!strcmp(dst_album, current_album))
            {
                ret = 1;
            }
            else
            {
                ret = 0;
            }
        }
    }
    done:
    if (progress_win)
    {
        SetPubVar(progress_win, "progress", 100);
        while (SCRIPT_FINISHED != ScriptState(progress_win))
        {
            sleep;
        }
        Destroy(progress_win);
    }
    show_result_message(ret, "msgphoto_copy_complete", _, "msgphoto_copy_fail", _, "msgphoto_copy_fail", _,
            "msgphoto_copy_warn_filemax", _, "msgphoto_copy_warn_freespace", _, "");
    SetPubVar(Parent(0), "menu_ret_val", ret);
    return ret;
}

/* Imports file item of the photo list (a USB device) into the album dst_album. */
public content_import_proc()
{
    new item;
    wait_message(TAG("SLCT"), item);
    new dst_album[128];
    GetArg("dst_album", dst_album, 128);
    printf("import proc , dest album = %s\n", dst_album);
    new ret;
    new invalid;
    new progress_win;
    new src_url[128];
    {
        new st_mode;
        new size_kb;
        ret = photoListGet(item, src_url, 128);
        if (ret < 0)
        {
            ret = -2;
            goto done;
        }
        ret = fileLstat(src_url, st_mode, size_kb);
        if (ret != g_not_found)
        {
            size_kb = (size_kb + 8191) / 8192 * 8;
        }
        else if (ret < 0)
        {
            ret = -2;
            goto done;
        }
        ret = check_pfs_free(size_kb);
        if (ret < 0)
        {
            goto done;
        }
    }
    ret = open_album_table(PHOTOS_LIBRARY, 0);
    if (ret < 0)
    {
        ret = -1;
        goto done;
    }
    if (get_wnd_var_int("usb_invalid", invalid))
    {
        if (0 != invalid)
        {
            ret = -5;
            goto done;
        }
    }
    ret = check_album_room(dst_album, ALBUM_LIBRARY, 1);
    if (ret < 0)
    {
        ret = -3;
        goto done;
    }
    sleep;
    if (get_wnd_var_int("usb_invalid", invalid))
    {
        if (0 != invalid)
        {
            ret = -5;
            goto done;
        }
    }
    progress_win = open_progress_window("msgphoto_pict_checkin", "msgphoto_warn_external_device_eject",
            "msgphoto_warn_console_power");
    if (get_wnd_var_int("usb_invalid", invalid))
    {
        if (0 != invalid)
        {
            ret = -5;
            goto done;
        }
    }
    new progress;
    printf("selected item %d ------------------\n", item);
    {
        new dst_url[128];
        new title[128];
        new ext[16];
        get_extension(ext, src_url, 16);
        {
            new file_name[128];
            new slash = find_last_nocase(src_url, "/");
            copy_tail(file_name, src_url, slash + 1, 128);
            photoSJIStoUTF8(file_name, file_name);
            escape_quotes(title, file_name, 128);
        }
        if (!ends_with_nocase(title, ext))
        {
            append_string(title, ext, 128);
        }
        if (strlen(src_url) <= 0)
        {
            ret = -1;
            goto done;
        }
        sprintf(dst_url, "%s%s", dst_album, title);
        printf("dl  src: %s\n", src_url);
        printf("hdd dst: %s\n", dst_url);
        sleep;
        if (get_wnd_var_int("usb_invalid", invalid))
        {
            if (0 != invalid)
            {
                ret = -5;
                goto done;
            }
        }
        new tmp_url[64];
        ret = prepare_photo_tmp(0, 0, 0);
        if (ret < 0)
        {
            goto done;
        }
        ret = copy_to_photo_tmp(tmp_url, src_url, 0, 64, progress_win, progress, progress + 33);
        if (ret < 0)
        {
            goto done;
        }
        progress = progress + 33;
        if (get_wnd_var_int("usb_invalid", invalid))
        {
            if (0 != invalid)
            {
                ret = -5;
                goto done;
            }
        }
        {
            new st_mode;
            new st_size;
            if (fileLstat(dst_album, st_mode, st_size) == g_not_found)
            {
                ret = fileMkdir(dst_album);
                if (ret < 0)
                {
                    ret = -2;
                    goto done;
                }
            }
        }
        ret = make_unique_url(dst_url, 128);
        if (ret < 0)
        {
            goto done;
        }
        {
            new create_date[16];
            new year;
            new month;
            new day;
            new hour;
            new minute;
            new second;
            date(year, month, day, 1);
            time(hour, minute, second, 1);
            sprintf(create_date, "%04d/%02d/%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
            new sql[256];
            new album_name_sql[128];
            sprintf(album_name_sql, "SELECT album_name FROM sce_photo WHERE album_uri = '%s' AND info_type = 3",
                    dst_album);
            sprintf(sql,
                    "INSERT INTO sce_photo ( info_type, album_name, title, content_uri, album_uri, create_date ) VALUES( 1, ( %s ), '%s', '%s', '%s', '%s')",
                    album_name_sql, title, dst_url, dst_album, create_date);
            printf("sql com = %s\n", sql);
            ret = datSQLExec(sql);
            if (ret < 0)
            {
                ret = -1;
                goto done;
            }
            ret = wait_database();
            if (ret < 0)
            {
                ret = -1;
                goto done;
            }
        }
        progress = progress + 33;
        SetPubVar(progress_win, "progress", progress);
        ret = move_from_photo_tmp(dst_url, tmp_url, 0);
        if (ret < 0)
        {
            goto done;
        }
        progress = progress + 33;
        SetPubVar(progress_win, "progress", progress);
        clear_album_ids();
        add_album_id(dst_album);
        ret = update_album_covers(ALBUM_LIBRARY);
        if (ret < 0)
        {
            ret = -1;
            goto done;
        }
        ret = clear_photo_tmp(0, 0, 0);
        if (ret < 0)
        {
            goto done;
        }
        ret = 0;
    }
    done:
    if (progress_win)
    {
        SetPubVar(progress_win, "progress", 100);
        while (SCRIPT_FINISHED != ScriptState(progress_win))
        {
            sleep;
        }
        Destroy(progress_win);
    }
    show_result_message(ret, "msgphoto_pict_checkin_complete", _, "msgphoto_pict_checkin_fail",
            "msgphoto_pict_checkin_fail_possible", "msgphoto_pict_checkin_fail", "msgphoto_pict_checkin_fail_possible",
            "msgphoto_pict_usb_max", _, "msgphoto_copy_warn_freespace", _, "msgphoto_pict_checkin_fail",
            "msgphoto_warn_usb_connect");
    SetPubVar(Parent(0), "menu_ret_val", ret);
    return ret;
}

/* Adds photo item of the open album to the favorite album dst_album. */
public content_addfav_proc()
{
    new item;
    wait_message(TAG("SLCT"), item);
    new dst_album[128];
    GetArg("dst_album", dst_album, 128);
    printf("addfav process , dest album = %s\n", dst_album);
    new ret;
    new progress_win;
    {
        new decstate[32];
        GetArg("decstate", decstate, 32);
        ret = atoi(decstate);
        if (ret < 0)
        {
            ret = -2;
            goto done;
        }
    }
    ret = open_album_table(PHOTOS_LIBRARY, 0);
    if (ret < 0)
    {
        ret = -1;
        goto done;
    }
    ret = check_album_room(dst_album, ALBUM_FAVORITE, 1);
    if (ret < 0)
    {
        ret = -3;
        goto done;
    }
    progress_win = open_progress_window("msgphoto_regpict_add");
    ret = select_album_rows(PHOTOS_LIBRARY, 0);
    if (ret < 0)
    {
        ret = -1;
        goto done;
    }
    new progress;
    printf("selected item %d ------------------\n", item);
    {
        new url[128];
        get_photo_field(item, FIELD_URL, url, PHOTOS_LIBRARY, 0, 128);
        progress = progress + 33;
        SetPubVar(progress_win, "progress", progress);
        {
            new sql[256];
            new create_date[16];
            new year;
            new month;
            new day;
            new hour;
            new minute;
            new second;
            date(year, month, day, 1);
            time(hour, minute, second, 1);
            sprintf(create_date, "%04d/%02d/%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
            sprintf(sql,
                    "INSERT INTO sce_photo ( info_type, album_name, title, content_uri, album_uri, create_date ) SELECT 2, album_name, title, content_uri, '%s', '%s' FROM sce_photo WHERE info_type = 1 AND content_uri = '%s'",
                    dst_album, create_date, url);
            printf("sql com = %s\n", sql);
            ret = datSQLExec(sql);
            if (ret < 0)
            {
                ret = -1;
                goto done;
            }
            ret = wait_database();
            if (ret < 0)
            {
                ret = -1;
                goto done;
            }
            clear_album_ids();
            add_album_id(dst_album);
            ret = update_album_covers(ALBUM_FAVORITE);
            if (ret < 0)
            {
                ret = -1;
                goto done;
            }
        }
        ret = 0;
    }
    done:
    if (progress_win)
    {
        SetPubVar(progress_win, "progress", 100);
        while (SCRIPT_FINISHED != ScriptState(progress_win))
        {
            sleep;
        }
        Destroy(progress_win);
    }
    show_result_message(ret, "msgphoto_regpict_complete", _, "msgphoto_regpict_fail", _, "msgphoto_regpict_fail", _,
            "msgphoto_regpict_max", _, "", _, "");
    SetPubVar(Parent(0), "menu_ret_val", ret);
    return ret;
}

/* Adds the photos of the open album (ALBUM_OFFS tells its kind) to the favorite album dst_album. */
public album_addfav_proc()
{
    new item;
    wait_message(TAG("SLCT"), item);
    new src_kind;
    {
        new offs[32];
        avalGetValue("ALBUM_OFFS", 0, offs, 32);
        if (!strcmp("LIBRARY", offs))
        {
            src_kind = ALBUM_LIBRARY;
        }
        else if (!strcmp("FAVORITE", offs))
        {
            src_kind = ALBUM_FAVORITE;
        }
        else
        {
            return -1;
        }
    }
    new dst_album[128];
    GetArg("dst_album", dst_album, 128);
    printf("addfav process , destination album = %s\n", dst_album);
    new ret;
    new progress_win;
    new count = open_album_db(PHOTOS_LIBRARY, 0);
    printf("new db opened %d\n", count);
    if (count < 0)
    {
        ret = -1;
        goto done;
    }
    if (0 == count)
    {
        ret = -5;
        goto done;
    }
    ret = check_album_room(dst_album, ALBUM_FAVORITE, count);
    if (ret < 0)
    {
        ret = -3;
        goto done;
    }
    progress_win = open_progress_window("msgphoto_regpict_add");
    new progress;
    new src_album[128];
    avalGetValue("PhotoAccessAlbum", 0, src_album, 128);
    {
        new sql[256];
        new create_date[16];
        new year;
        new month;
        new day;
        new hour;
        new minute;
        new second;
        date(year, month, day, 1);
        time(hour, minute, second, 1);
        sprintf(create_date, "%04d/%02d/%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
        if (ALBUM_LIBRARY == src_kind)
        {
            sprintf(sql,
                    "INSERT INTO sce_photo ( info_type, album_name, title, content_uri, album_uri, create_date ) SELECT 2, album_name, title, content_uri, '%s', '%s' FROM sce_photo WHERE info_type = 1 AND album_uri = '%s' ORDER BY create_date,title",
                    dst_album, create_date, src_album);
        }
        else if (src_kind == ALBUM_FAVORITE)
        {
            sprintf(sql,
                    "INSERT INTO sce_photo ( info_type, album_name, title, content_uri, album_uri, create_date ) SELECT 2, album_name, title, content_uri, '%s', '%s' FROM sce_photo WHERE info_type = 2 AND album_uri = '%s' ORDER BY create_date,title",
                    dst_album, create_date, src_album);
        }
        printf("sql com = %s\n", sql);
        ret = datSQLExec(sql);
        if (ret < 0)
        {
            ret = -1;
            goto done;
        }
        ret = wait_database();
        if (ret < 0)
        {
            ret = -1;
            goto done;
        }
        progress = 40;
        SetPubVar(progress_win, "progress", progress);
        clear_album_ids();
        add_album_id(dst_album);
        ret = update_album_covers(ALBUM_FAVORITE);
        if (ret < 0)
        {
            ret = -1;
            goto done;
        }
        ret = 0;
    }
    done:
    if (progress_win)
    {
        SetPubVar(progress_win, "progress", 100);
        while (SCRIPT_FINISHED != ScriptState(progress_win))
        {
            sleep;
        }
        Destroy(progress_win);
    }
    show_result_message(ret, "msgphoto_regpict_complete", _, "msgphoto_regpict_fail", _, "msgphoto_regpict_fail", _,
            "msgphoto_regpict_max", _, "", _, "msgphoto_regpict_empty");
    SetPubVar(Parent(0), "menu_ret_val", ret);
    return ret;
}

#include "photousb.inc"
