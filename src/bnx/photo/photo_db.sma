/*
 * /opt0/bn/script/photo/photo_db.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The database server of the photo page: an object of the page that opens and queries the photo database and reads
 * its values on request, so that the page goes on drawing meanwhile.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 512

#include "photodb.inc"

#include "photo.inc"

#include "tag.inc"

/* Returns whether str is a packed string. */
is_packed(str[])
{
    return str[0] <= 0 || str[0] > 255;
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

new g_photo_db_name[] = "photo";

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

#include "fileop.inc"

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

/*
 * "OPEN" (mode, sender) opens the database of mode and answers "OPEN" (result); "FIND" (sender) queries it and
 * answers "DNUM" (number of rows); "DGET" (row, column, sender) answers "DSTR" with the value or "DERR"; "FNSH" closes
 * the database and ends.
 */
public database_process()
{
    new mode;
    for (; ; )
    {
        new new_mode;
        new sender;
        new row;
        new column;
        if (FindMessage(TAG("OPEN"), new_mode, sender))
        {
            mode = new_mode;
            new ret = open_album_table(mode, 0);
            if (IsValidObject(sender))
            {
                SendMessage(sender, TAG("OPEN"), ret, 0, 0, 0);
            }
        }
        if (FindMessage(TAG("FIND"), sender))
        {
            new count = select_album_rows(mode, 0);
            if (IsValidObject(sender))
            {
                SendMessage(sender, TAG("DNUM"), count, 0, 0, 0);
            }
        }
        if (FindMessage(TAG("DGET"), row, column, sender))
        {
            new ret;
            new reply = TAG("DSTR");
            new text[128];
            ret = datGetData(row, column, text, 128);
            if (ret < 0)
            {
                reply = TAG("DERR");
                sprintf(text, "DB WARNING : can't get data!!");
            }
            if (IsValidObject(sender))
            {
                SendMessageStr(sender, reply, text);
            }
        }
        if (FindMessage(TAG("FNSH")))
        {
            break;
        }
        sleep;
    }
    datClose();
    return;
}

#include "photodate.inc"
