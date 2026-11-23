/*
 * /opt0/bn/script/photo/usb_process.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The import of photos into new albums of the library: from a USB storage device (usb_checkin_process) or from
 * the download folder (pc_checkin_process), with the watch of the USB device (usb_media_check).
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#pragma library "photo"
native photoListClear();
native photoListAdd(const path[]);
native photoListNum();
native photoListGet(index, const buf[], maxlength);
/*
 * Rewrites a date read from the database (in UTC) in local time, in the date notation (0: Y/M/D, 1: M/D/Y, 2: D/M/Y)
 * and the time notation (0: 24 hours, 1: 12 hours) of the system settings.
 */
native photoDateGetFromDB(const date_str[], maxlength, date_notation, time_notation);
native photoSJIStoUTF8(const dest[], const src[]);
native photoUTF8toSJIS(const dest[], const src[]);
#pragma library

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

/* Same as wait_message_str of sound.inc. */
wait_message_str(msg, text[], maxlen)
{
    while (!FindMessageStr(msg, text, maxlen))
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

#include <float>

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
 * Asks the TEXT of msg, msg2 and msg3 (one line each) in a confirm_yesno dialog, with the focus on yes if
 * default_yes; returns 0 for yes, 1 for no or a cancel.
 */
ask_yes_no(msg[], msg2[], msg3[], default_yes)
{
    new args[256];
    new text[128];
    new yes_label[32];
    new no_label[32];
    get_tag_attr("TEXT", msg, "value", text, 128);
    if (strlen(msg2) > 0)
    {
        new line[128];
        get_tag_attr("TEXT", msg2, "value", line, 128);
        append_string(text, "\n", 128);
        append_string(text, line, 128);
    }
    if (strlen(msg3) > 0)
    {
        new line[128];
        get_tag_attr("TEXT", msg3, "value", line, 128);
        append_string(text, "\n", 128);
        append_string(text, line, 128);
    }
    get_tag_attr("TEXT", "msgphoto_common_yes", "value", yes_label, 32);
    get_tag_attr("TEXT", "msgphoto_common_no", "value", no_label, 32);
    sprintf(args, "msg=%s;yes=%s;no=%s", text, yes_label, no_label);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 240.0, 0.0, 0);
    AttachScript(dialog, _, "confirm_yesno", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus);
    if (default_yes)
    {
        SetPubVar(dialog, "select", 0);
    }
    else
    {
        SetPubVar(dialog, "select", 1);
    }
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    new answer = GetPubVar(dialog, "select");
    Destroy(dialog);
    if (answer == -1)
    {
        answer = 1;
    }
    return answer;
}

/*
 * Opens a progress_window with the TEXT of msg, msg2 and msg3 and, if given, a cancel button labelled with the TEXT
 * of cancel; returns its object.
 */
open_progress_window(msg[], msg2[], msg3[], cancel[], breakflag[])
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
 * Checks that adding more albums fit in the library (kind ALBUM_LIBRARY), counted by the window variable
 * PhotoLibraryNum, at most MAX_ITEMS; returns 0, -3 if they do not, -1 on an error.
 */
check_album_count(kind, adding)
{
    new count = kind;
    if (ALBUM_LIBRARY == kind)
    {
        if (!get_wnd_var_int("PhotoLibraryNum", count))
        {
            return -1;
        }
    }
    else if (kind == ALBUM_FAVORITE)
    {
        count = 1;
    }
    printf("photo album num check %d ----------\n", count);
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

/* Moves src into the photo tmp directory, its path in dest; returns 1 if src is missing and missing_ok, 0, or < 0. */
move_to_photo_tmp(dest[], src[], missing_ok, ignored, progress_win, from, to)
{
    printf("move to filetmp\n");
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
    printf("MOVE to TMP name = %s\n", src);
    ret = fileMove(src, dest);
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
        if (error == 2)
        {
            return -4;
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

/*
 * Moves the images of the download folder into new albums of the library named after the date, at most MAX_ITEMS photos
 * each, after asking; usb_sql_process writes their rows.
 */
public pc_checkin_process()
{
    new ret;
    new progress_win;
    new sql_proc;
    ret = datOpen(DAT_FILE_LIST, "", DAT_ORDER_NAME_ASC);
    if (0 != ret)
    {
        return -1;
    }
    ret = wait_database();
    if (ret < 0)
    {
        return -1;
    }
    ret = datFind("image", "file:/net/download");
    if (0 != ret)
    {
        return -1;
    }
    ret = wait_database();
    if (ret < 0)
    {
        return -1;
    }
    new total = datGetNum();
    printf("down_load db num %d\n", total);
    if (total < 0)
    {
        datClose();
        return -1;
    }
    if (0 == total)
    {
        printf("No image data found in download!!\n");
        datClose();
        return 0;
    }
    {
        new answer = ask_yes_no("msgphoto_hddimport_copy_found1", "msgphoto_hddimport_copy_found2",
                "msgphoto_hddimport_copy_ask1", 0);
        if (0 != answer)
        {
            return -1;
        }
    }
    {
        new size_kb;
        new url[128];
        {
            new i;
            for (; i < total; i++)
            {
                new st_mode;
                new st_size;
                get_photo_field(i, FIELD_URL, url, PHOTOS_USB, 0, 128);
                ret = fileLstat(url, st_mode, st_size);
                if (ret != g_not_found)
                {
                    size_kb = size_kb + (st_size + 8191) / 8192 * 8;
                }
                else if (ret < 0)
                {
                    ret = -2;
                    goto done;
                }
            }
        }
        ret = check_pfs_free(size_kb);
        if (ret < 0)
        {
            goto done;
        }
    }
    progress_win = open_progress_window("msgphoto_hddimport_copy", "msgphoto_warn_console_power", "", "", "");
    new album_no;
    new done_count;
    new progress;
    new batch;
    new album_name[16];
    new create_date[32];
    new album_uri[128];
    new photo_dir[128];
    copy_string(photo_dir, g_contents_dir, 128);
    fileMkdir(photo_dir);
    append_string(photo_dir, "photo/", 128);
    fileMkdir(photo_dir);
    next_album:
    ret = prepare_photo_tmp(0, 0, 0);
    if (ret < 0)
    {
        goto done;
    }
    sql_proc = CreateNull(Self());
    AttachScript(sql_proc, "usb_sql_process");
    name_album:
    {
        new year;
        new month;
        new day;
        new hour;
        new minute;
        new second;
        date(year, month, day, 1);
        time(hour, minute, second, 1);
        sprintf(create_date, "%04d/%02d/%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
        sprintf(album_name, "%04d-%02d-%02d_%02d-%02d-%02d", year, month, day, hour, minute, second);
    }
    copy_string(album_uri, photo_dir, 128);
    append_string(album_uri, album_name, 128);
    append_string(album_uri, "/", 128);
    printf("create new album %s\n", album_uri);
    sleep;
    SendMessageStr(sql_proc, TAG("AURI"), album_uri);
    batch = total - done_count;
    {
        new exists;
        new photo_count;
        wait_message(TAG("EXST"), exists, photo_count, ret);
        if (ret < 0)
        {
            ret = -1;
            goto done;
        }
        if (!exists)
        {
            ret = check_album_count(ALBUM_LIBRARY, 1);
            if (ret < 0)
            {
                goto done;
            }
        }
        if (photo_count + batch > MAX_ITEMS)
        {
            batch = MAX_ITEMS - photo_count;
        }
    }
    if (batch <= 0)
    {
        album_no++;
        SendMessage(sql_proc, TAG("AOTH"), 1, 0, 0, 0);
        goto name_album;
    }
    SendMessage(sql_proc, TAG("AOTH"), 0, 0, 0, 0);
    printf("import data num %d\n", batch);
    get_tag_attr("TEXT", "msgphoto_hddimport_title_album", "value", album_name, 16);
    if (album_no > 0)
    {
        sprintf(album_name, "%s%d", album_name, album_no);
    }
    SendMessageStr(sql_proc, TAG("ALBM"), album_name);
    SendMessageStr(sql_proc, TAG("DATE"), create_date);
    {
        new i = done_count;
        for (; i < batch + done_count; i++)
        {
            new dst_url[128];
            new src_url[128];
            new title[128];
            new ext[16];
            get_photo_field(i, FIELD_URL, src_url, PHOTOS_USB, 0, 128);
            get_extension(ext, src_url, 16);
            get_photo_field(i, FIELD_TITLE, title, PHOTOS_USB, 0, 128);
            if (!ends_with_nocase(title, ext))
            {
                append_string(title, ext, 128);
            }
            if (strlen(src_url) <= 0)
            {
                ret = -1;
                goto done;
            }
            sprintf(dst_url, "%s%s", album_uri, title);
            printf("dl  src: %s\n", src_url);
            printf("hdd dst: %s\n", dst_url);
            ret = make_unique_url(dst_url, 128);
            if (ret < 0)
            {
                ret = -2;
                goto done;
            }
            new tmp_url[128];
            ret = move_to_photo_tmp(tmp_url, src_url, 0, 128, 0, 0, 0);
            if (ret < 0)
            {
                goto done;
            }
            progress = (i + 1) * 99 / total;
            SetPubVar(progress_win, "progress", progress);
            if (ScriptState(sql_proc) == SCRIPT_FINISHED)
            {
                ret = -1;
                goto done;
            }
            SendMessageStr(sql_proc, TAG("TITL"), title);
            SendMessageStr(sql_proc, TAG("CURI"), dst_url);
            for (; ; )
            {
                new index;
                if (FindMessage(TAG("SQOK"), index))
                {
                    if (index == i - done_count)
                    {
                        break;
                    }
                }
                if (FindMessage(TAG("SQER"), index))
                {
                    ret = -1;
                    goto done;
                }
                sleep;
            }
            ret = move_from_photo_tmp(dst_url, tmp_url, 0);
            if (ret < 0)
            {
                goto done;
            }
        }
    }
    SendMessage(sql_proc, TAG("FNSH"), 0, 0, 0, 0);
    for (; ; )
    {
        if (FindMessage(TAG("SQOK"), ret))
        {
            break;
        }
        if (FindMessage(TAG("SQER"), ret))
        {
            goto done;
        }
        sleep;
    }
    done_count = done_count + batch;
    printf("\ncurrent down load num = %d\n", done_count);
    if (total > done_count)
    {
        album_no++;
        if (IsValidObject(sql_proc))
        {
            Destroy(sql_proc);
            sql_proc = 0;
        }
        sleep;
        goto next_album;
    }
    progress = 100;
    ret = clear_photo_tmp(0, 0, 0);
    if (ret < 0)
    {
        goto done;
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
        progress_win = 0;
    }
    show_result_message(ret, "msgphoto_hddimport_copy_complete", _, "msgphoto_hddimport_copy_fail", _,
            "msgphoto_hddimport_copy_fail", _, "msgphoto_album_make_max", _, "msgphoto_copy_warn_freespace", _, "");
    printf(" ------- end = %d\n", ret);
    if (IsValidObject(sql_proc))
    {
        Destroy(sql_proc);
    }
    if (IsValidObject(progress_win))
    {
        Destroy(progress_win);
    }
    if (ret < 0)
    {
        return -1;
    }
    return 0;
}

/*
 * Copies the photos of the USB device into new albums of the library named after the date, at most MAX_ITEMS photos
 * each, after asking; the progress window can cancel it. usb_sql_process writes their rows.
 */
public usb_checkin_process()
{
    new ret = -1;
    new progress_win;
    new sql_proc;
    new invalid;
    ret = ask_yes_no("msgphoto_pict_checkinall_ask", "", "", 1);
    if (0 != ret)
    {
        return -1;
    }
    if (get_wnd_var_int("usb_invalid", invalid))
    {
        if (0 != invalid)
        {
            ret = -2;
            goto done;
        }
    }
    {
        new data_size;
        if (get_wnd_var_int("PhotoDataSize", data_size))
        {
            ret = check_pfs_free(data_size);
            if (ret < 0)
            {
                goto done;
            }
        }
        else
        {
            ret = -1;
            goto done;
        }
    }
    ret = check_album_count(ALBUM_LIBRARY, 1);
    if (ret < 0)
    {
        goto done;
    }
    progress_win = open_progress_window("msgphoto_pict_checkin", "msgphoto_warn_external_device_eject",
            "msgphoto_warn_console_power", "msgphoto_common_cancel", "break");
    new total = open_album_db(PHOTOS_USB, 0);
    printf("new db opened %d\n", total);
    if (total < 0)
    {
        ret = -1;
        goto done;
    }
    if (get_wnd_var_int("usb_invalid", invalid))
    {
        if (0 != invalid)
        {
            ret = -2;
            goto done;
        }
    }
    new album_no;
    new done_count;
    new progress;
    new create_date[32];
    new album_name[64];
    new batch;
    new album_uri[128];
    new photo_dir[128];
    copy_string(photo_dir, g_contents_dir, 128);
    fileMkdir(photo_dir);
    append_string(photo_dir, "photo/", 128);
    fileMkdir(photo_dir);
    next_album:
    ret = prepare_photo_tmp(0, 0, 0);
    if (ret < 0)
    {
        goto done;
    }
    sql_proc = CreateNull(Self());
    AttachScript(sql_proc, "usb_sql_process");
    name_album:
    {
        new year;
        new month;
        new day;
        new hour;
        new minute;
        new second;
        date(year, month, day, 1);
        time(hour, minute, second, 1);
        sprintf(create_date, "%04d/%02d/%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
        sprintf(album_name, "%04d-%02d-%02d_%02d-%02d-%02d", year, month, day, hour, minute, second);
    }
    copy_string(album_uri, photo_dir, 128);
    append_string(album_uri, album_name, 128);
    append_string(album_uri, "/", 128);
    printf("create new album %s\n", album_uri);
    {
        new year;
        new month;
        new day;
        new hour;
        new minute;
        new second;
        date(year, month, day, 0);
        time(hour, minute, second, 0);
        sprintf(album_name, "%04d/%02d/%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
    }
    sleep;
    if (get_wnd_var_int("usb_invalid", invalid))
    {
        if (0 != invalid)
        {
            ret = -2;
            goto done;
        }
    }
    SendMessageStr(sql_proc, TAG("AURI"), album_uri);
    batch = total - done_count;
    {
        new exists;
        new photo_count;
        wait_message(TAG("EXST"), exists, photo_count, ret);
        if (ret < 0)
        {
            ret = -1;
            goto done;
        }
        if (!exists)
        {
            ret = check_album_count(ALBUM_LIBRARY, 1);
            if (ret < 0)
            {
                goto done;
            }
        }
        if (photo_count + batch > MAX_ITEMS)
        {
            batch = MAX_ITEMS - photo_count;
        }
    }
    if (batch <= 0)
    {
        album_no++;
        SendMessage(sql_proc, TAG("AOTH"), 1, 0, 0, 0);
        goto name_album;
    }
    SendMessage(sql_proc, TAG("AOTH"), 0, 0, 0, 0);
    printf("import data num %d\n", batch);
    if (get_wnd_var_int("usb_invalid", invalid))
    {
        if (0 != invalid)
        {
            ret = -2;
            goto done;
        }
    }
    SendMessageStr(sql_proc, TAG("ALBM"), album_name);
    SendMessageStr(sql_proc, TAG("DATE"), create_date);
    {
        new i = done_count;
        for (; i < batch + done_count; i++)
        {
            new dst_url[128];
            new src_url[128];
            new title[128];
            new ext[16];
            get_photo_field(i, FIELD_URL, src_url, PHOTOS_USB, 0, 128);
            get_extension(ext, src_url, 16);
            get_photo_field(i, FIELD_TITLE, title, PHOTOS_USB, 0, 128);
            if (!ends_with_nocase(title, ext))
            {
                append_string(title, ext, 128);
            }
            if (strlen(src_url) <= 0)
            {
                ret = -1;
                goto done;
            }
            sprintf(dst_url, "%s%s", album_uri, title);
            printf("usb src: %s\n", src_url);
            printf("hdd dst: %s\n", dst_url);
            ret = make_unique_url(dst_url, 128);
            if (ret < 0)
            {
                ret = -1;
                goto done;
            }
            new tmp_url[128];
            ret = copy_to_photo_tmp(tmp_url, src_url, 0, 128, progress_win, progress, (i + 1) * 99 / total);
            if (ret < 0)
            {
                goto done;
            }
            progress = (i + 1) * 99 / total;
            if (get_wnd_var_int("usb_invalid", invalid))
            {
                if (0 != invalid)
                {
                    ret = -2;
                    goto done;
                }
            }
            if (ScriptState(sql_proc) == SCRIPT_FINISHED)
            {
                ret = -1;
                goto done;
            }
            SendMessageStr(sql_proc, TAG("TITL"), title);
            SendMessageStr(sql_proc, TAG("CURI"), dst_url);
            for (; ; )
            {
                new index;
                if (FindMessage(TAG("SQOK"), index))
                {
                    if (index == i - done_count)
                    {
                        break;
                    }
                }
                if (FindMessage(TAG("SQER"), index))
                {
                    ret = -1;
                    goto done;
                }
                if (get_wnd_var_int("usb_invalid", invalid))
                {
                    if (0 != invalid)
                    {
                        ret = -2;
                        goto done;
                    }
                }
                sleep;
            }
            ret = move_from_photo_tmp(dst_url, tmp_url, 0);
            if (ret < 0)
            {
                goto done;
            }
            {
                new cancel;
                if (get_wnd_var_int("ProgressCancelFlag", cancel))
                {
                    if (cancel)
                    {
                        done_count = i;
                        ret = -5;
                        goto done;
                    }
                }
            }
            if (get_wnd_var_int("usb_invalid", invalid))
            {
                if (0 != invalid)
                {
                    ret = -2;
                    goto done;
                }
            }
        }
    }
    SendMessage(sql_proc, TAG("FNSH"), 0, 0, 0, 0);
    for (; ; )
    {
        if (FindMessage(TAG("SQOK"), ret))
        {
            break;
        }
        if (FindMessage(TAG("SQER"), ret))
        {
            goto done;
        }
        sleep;
    }
    if (get_wnd_var_int("usb_invalid", invalid))
    {
        if (0 != invalid)
        {
            ret = -2;
            goto done;
        }
    }
    done_count = done_count + batch;
    printf("\ncurrent down load num = %d\n", done_count);
    if (total > done_count)
    {
        album_no++;
        if (IsValidObject(sql_proc))
        {
            Destroy(sql_proc);
            sql_proc = 0;
        }
        sleep;
        goto next_album;
    }
    progress = 100;
    ret = clear_photo_tmp(0, 0, 0);
    if (ret < 0)
    {
        goto done;
    }
    done:
    if (progress_win)
    {
        if (-5 != ret)
        {
            SetPubVar(progress_win, "progress", 100);
        }
        while (SCRIPT_FINISHED != ScriptState(progress_win))
        {
            sleep;
        }
        Destroy(progress_win);
    }
    if (-5 != ret)
    {
        show_result_message(ret, "msgphoto_pict_checkin_complete", _, "msgphoto_pict_checkin_fail",
                "msgphoto_pict_checkin_fail_possible", "msgphoto_pict_checkin_fail", "msgphoto_warn_usb_connect",
                "msgphoto_album_make_max", _, "msgphoto_copy_warn_freespace", _, "msgphoto_pict_checkin_cancel");
    }
    else
    {
        new args[256];
        new text[128];
        get_tag_attr("TEXT", "msgphoto_pict_checkin_cancel", "value", text, 128);
        sprintf(args, "msg=%s\n%d", text, done_count + 1);
        get_tag_attr("TEXT", "msgphoto_pict_checkin_passing", "value", text, 128);
        append_string(args, text, 256);
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
    }
    printf(" ------- end = %d\n", ret);
    if (IsValidObject(sql_proc))
    {
        Destroy(sql_proc);
    }
    if (ret < 0)
    {
        return -1;
    }
    return 0;
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

/*
 * Runs on a child object and writes the rows of an import: "AURI" (album uri) is answered by "EXST" (rows of that
 * album, its photos, status); "AOTH" 1 asks for another uri, 0 goes on with "ALBM" (name) and "DATE"; each "TITL"
 * and "CURI" (title, url) adds a photo, the first also the album, and is answered by "SQOK" (index); "FNSH" ends
 * it. "SQER" reports an error.
 */
public usb_sql_process()
{
    new new_album;
    new status;
    status = datOpen(DAT_DATABASE, g_photo_db_name, DAT_ORDER_MTIME_DESC);
    if (0 != status)
    {
        return -1;
    }
    status = wait_database();
    if (status < 0)
    {
        return -1;
    }
    next_album:
    new album_uri[64];
    wait_message_str(TAG("AURI"), album_uri, 64);
    printf("usb_sql get AURI %s!!\n", album_uri);
    {
        new sql[128];
        new exists;
        new photo_count;
        sprintf(sql, "SELECT album_name FROM sce_photo WHERE album_uri = '%s' AND info_type=3 ", album_uri);
        printf("sql com = %s\n", sql);
        status = datSQLExec(sql);
        if (status < 0)
        {
            goto reply;
        }
        status = wait_database();
        if (status < 0)
        {
            goto reply;
        }
        exists = datGetNum();
        sprintf(sql, "SELECT title FROM sce_photo WHERE album_uri = '%s' AND info_type=1 ", album_uri);
        printf("sql com = %s\n", sql);
        status = datSQLExec(sql);
        if (status < 0)
        {
            goto reply;
        }
        status = wait_database();
        if (status < 0)
        {
            goto reply;
        }
        photo_count = datGetNum();
        reply:
        SendMessage(Parent(0), TAG("EXST"), exists, photo_count, status, 0);
        if (status < 0 || exists < 0 || photo_count < 0)
        {
            goto done;
        }
        new other;
        wait_message(TAG("AOTH"), other);
        if (other)
        {
            goto next_album;
        }
        if (0 == exists)
        {
            new_album = 1;
        }
    }
    new album_name[128];
    new album_date[32];
    {
        new name[64];
        wait_message_str(TAG("ALBM"), name, 64);
        escape_quotes(album_name, name, 128);
    }
    wait_message_str(TAG("DATE"), album_date, 32);
    printf("usb_sql ALBM %s!!\n", album_name);
    printf("usb_sql DATE %s!!\n", album_date);
    new index;
    for (; ; )
    {
        new sql[256];
        new title[64];
        new content_uri[128];
        if (FindMessage(TAG("FNSH")))
        {
            break;
        }
        if (FindMessageStr(TAG("TITL"), title, 64))
        {
            new quoted_title[128];
            escape_quotes(quoted_title, title, 128);
            if (FindMessageStr(TAG("CURI"), content_uri, 128))
            {
                printf("usb_sql TITL %s\n", title);
                printf("usb_sql CURI %s\n", content_uri);
                if (new_album && 0 == index)
                {
                    sprintf(sql,
                            "INSERT INTO sce_photo ( info_type, album_name, title, content_uri, album_uri, create_date ) VALUES( 3, '%s', '%s', '%s', '%s', '%s')",
                            album_name, album_name, content_uri, album_uri, album_date);
                    printf("sql com = %s\n", sql);
                    status = datSQLExec(sql);
                    if (status < 0)
                    {
                        goto done;
                    }
                    status = wait_database();
                    if (status < 0)
                    {
                        goto done;
                    }
                    new album_count;
                    if (!get_wnd_var_int("PhotoLibraryNum", album_count))
                    {
                        album_count = 0;
                    }
                    album_count++;
                    set_wnd_var_int("PhotoLibraryNum", album_count);
                    new st_mode;
                    new st_size;
                    status = fileLstat(album_uri, st_mode, st_size);
                    if (status == g_not_found)
                    {
                        status = fileMkdir(album_uri);
                        if (status < 0)
                        {
                            goto done;
                        }
                    }
                    else
                    {
                        status = -1;
                        goto done;
                    }
                }
                {
                    new create_date[64];
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
                            "INSERT INTO sce_photo ( info_type, album_name, title, content_uri, album_uri, create_date ) VALUES(1,'%s','%s','%s','%s','%s' )",
                            album_name, quoted_title, content_uri, album_uri, create_date);
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
                if (status >= 0)
                {
                    SendMessage(Parent(0), TAG("SQOK"), index, 0, 0, 0);
                    index++;
                }
            }
        }
        sleep;
    }
    done:
    if (status < 0)
    {
        printf("usb sql process error!!\n");
        SendMessage(Parent(0), TAG("SQER"), status, 0, 0, 0);
        return -1;
    }
    SendMessage(Parent(0), TAG("SQOK"), 0, 0, 0, 0);
    status = wait_database();
    if (status < 0)
    {
        return -1;
    }
    datClose();
    return 0;
}
