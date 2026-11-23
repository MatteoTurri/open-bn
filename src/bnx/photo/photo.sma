/*
 * /opt0/bn/script/photo/photo.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The photo page: radio buttons for the albums of the hard disk and of a USB device (or the favorite albums alone) and
 * a curve of album icons; an album opens into a grid of its photos, the one under the cursor shown large. The triangle
 * opens the option menus, the circle on a photo the slide show page.
 */

#include <core>
#include <bn>

#pragma dynamic 1560

#include "photodb.inc"

/* As in text.inc. */
const EDIT_MODE_USER_ID = 1;
const EDIT_MODE_PASSWORD = 2;
const EDIT_MODE_KATAKANA = 3;
const EDIT_MODE_NUMBER = 4;
const EDIT_MODE_KATAKANA_FIXED = 7;
const EDIT_MODE_QWERTY = 9;

#include "devimage.inc"

#include "stdmenu.inc"
#include "genmenu.inc"
#include "scroll.inc"

/* The states of the two views of the page, the albums and the photos of an album. */
const VIEW_ON = 0;
const VIEW_ENTER = 1; /* to be prepared */
const VIEW_SHOW = 2;  /* to be shown */
const VIEW_HIDDEN = 3;
const VIEW_OFF = 4;   /* not built */
const VIEW_HIDE = 5;  /* to be hidden */
const VIEW_CLOSE = 6; /* to be destroyed */

/* Values of g_page_mode: the view that runs, or the switch to the other one. */
const PAGE_ALBUMS = 0;
const PAGE_PHOTOS = 1;
const PAGE_SPARE = 2; /* never set: its update does nothing */
const PAGE_TO_PHOTOS = 3;
const PAGE_TO_ALBUMS = 4;
const PAGE_QUIT = 5;

/* Results of handle_menu_push besides MENU_*: what an option of the menu changed. */
const MENU_STAY = 0;     /* the menu stays open */
const MENU_USB_LOST = 6; /* the USB device is no longer valid */
const MENU_CHANGED = 8;  /* the photos and the albums */
const MENU_ALBUMS_CHANGED = 9;
const MENU_PHOTOS_CHANGED = 10;
const MENU_RENAMED = 11; /* the information page renamed the album or the photo */

/* The lists of the option menu (the mode of std_menu_core). */
const LIST_GROUP = 0;    /* the items of its MENUGROUP */
const LIST_LIBRARY = 1;  /* the albums of the library, as the destination */
const LIST_FAVORITE = 2; /* the favorite albums, as the destination */

#pragma library "photo"
native photoListClear();
native photoListAdd(const path[]);
native photoListNum();
native photoListGet(index, const buf[], maxlength);
native photoDateGetFromDB(const date_str[], maxlength, date_notation, time_notation);
native photoSJIStoUTF8(const dest[], const src[]);
native photoUTF8toSJIS(const dest[], const src[]);
#pragma library

forward Float:get_node_x(node);
forward Float:get_node_y(node);
forward Float:get_node_z(node);

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

/* Same as wait_xml_load of xmlsync.inc. */
wait_xml_load()
{
    new status;
    for (; ; )
    {
        status = xmlLoadSync();
        if (0 != status)
        {
            return status;
        }
        sleep;
    }
    return 0;
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
new g_shadow_colors[4] = {0x5F000000, 0x5F000000, 0x5F000000, 0x5F000000}; /* the "SCL4" of the shadowed labels */
new Float:g_screen_width;
new Float:g_screen_height;
new Float:g_screen_depth;
new g_common_bg_name[] = "SCEI:CommonBG";

/* Same as start_image_decode of text.inc. */
start_image_decode(url[])
{
    return imgDecStart(url, 0, 0, -1, 100, 0);
}

/* Same as finish_image_decode of imgdec.inc. */
finish_image_decode()
{
    return wait_image_decode();
}

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

/* Same as map_screen_point of dom.inc. */
map_screen_point(&Float:x, &Float:y, &Float:z, Float:screen_x, Float:screen_y, rel)
{
    if (!rel)
    {
        rel = Parent(0);
    }
    if (!rel)
    {
        rel = Self();
    }
    GetOffPos(Disp2D(0), x, y, z, screen_x, screen_y, 0.0, rel);
}

/* Same as start_top_page of disc.inc. */
start_top_page(keep_history)
{
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
    if (!keep_history)
    {
        hisClear();
    }
}

/* Same as get_list_height of layout.inc. */
get_list_height(count, has_title, &Float:list_h)
{
    if (count <= 5)
    {
        list_h = 0.0583 * 4;
        if (has_title)
        {
            list_h = 0.0875 + list_h;
        }
    }
    else if (count >= 8)
    {
        list_h = 0.0583 * 7;
        if (has_title)
        {
            list_h = 0.0875 + list_h;
        }
    }
    else
    {
        list_h = 0.0583 * (count - 1);
        if (has_title)
        {
            list_h = 0.0875 + list_h;
        }
    }
}

/* Same as get_list_pos of layout.inc. */
get_list_pos(side, count, has_title, &Float:x, &Float:y)
{
    new count_copy = count; /* redundant as written */
    new title_copy = has_title;
    count = count_copy;
    has_title = title_copy;
    if (0 == side)
    {
        x = 0.275;
    }
    else if (side == 1)
    {
        x = 0.74;
    }
    y = 0.5;
}

/* Same as get_list_body_height of layout.inc. */
get_list_body_height(count, has_title, &Float:list_h)
{
    new title_copy = has_title; /* redundant as written */
    has_title = title_copy;
    if (count <= 5)
    {
        list_h = 0.0583 * 4;
    }
    else if (count >= 8)
    {
        list_h = 0.0583 * 7;
    }
    else
    {
        list_h = 0.0583 * (count - 1);
    }
}

/* Same as get_list_title_pos of layout.inc. */
get_list_title_pos(side, count, has_title, &Float:x, &Float:y)
{
    new count_copy = count; /* redundant as written */
    count = count_copy;
    if (!has_title)
    {
        return -1;
    }
    if (0 == side)
    {
        x = 0.275;
    }
    else if (side == 1)
    {
        x = 0.74;
    }
    new Float:list_h;
    get_list_height(count, has_title, list_h);
    y = 0.5 - 0.5 * list_h;
    return 0;
}

/* Same as get_list_body_pos of layout.inc. */
get_list_body_pos(side, count, has_title, &Float:x, &Float:y)
{
    new count_copy = count; /* redundant as written */
    count = count_copy;
    if (0 == side)
    {
        x = 0.275;
    }
    else if (side == 1)
    {
        x = 0.74;
    }
    if (!has_title)
    {
        y = 0.5;
    }
    else
    {
        y = 0.5 + 0.5 * 0.0875;
    }
}

/* Same as get_list_top of layout.inc. */
get_list_top(count, has_title, &Float:y)
{
    new Float:list_h;
    get_list_height(count, has_title, list_h);
    if (has_title)
    {
        y = 0.0875 + (0.5 - 0.5 * list_h);
    }
    else
    {
        y = 0.5 - 0.5 * list_h;
    }
}

/* Same as get_scroll_bar_pos of layout.inc. */
get_scroll_bar_pos(side, count, has_title, &Float:x, &Float:y)
{
    new Float:list_x;
    get_list_body_pos(side, count, has_title, list_x, y);
    if (0 == side)
    {
        x = 0.047;
    }
    else if (side == 1)
    {
        x = 0.953;
    }
}

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
    xmlLoadStart(url, g_common_bg_name, _, XMLLOAD_BG);
    wait_xml_load();
    fade_in_background(fade_time);
}

/* Same as fade_in_background of background.inc. */
fade_in_background(Float:duration)
{
    new scene = wndGetScene(WND_BG);
    if (scene)
    {
        SendMessage(scene, TAG("STRT"), duration, 0, 0, 0);
    }
}

/* Same as fade_background of background.inc. */
fade_background(alpha0, alpha1, Float:duration)
{
    new scene = wndGetScene(WND_BG);
    if (scene)
    {
        SendMessage(scene, TAG("FADE"), alpha0, alpha1, duration, 0);
    }
}

/* Same as set_background_poor of bgmode.inc. */
set_background_poor(Float:fade_in_time, fade_out_time)
{
    new scene = wndGetScene(WND_BG);
    if (scene)
    {
        SendMessage(scene, TAG("POOR"), fade_in_time, fade_out_time, 0, 0);
    }
}

/* Makes the camera of the background window follow this camera, by these shares of its position and rotation. */
link_bg_camera(Float:pos_follow, rot_follow)
{
    new bg_scene = wndGetScene(WND_BG);
    if (bg_scene)
    {
        SendMessage(bg_scene, TAG("LINK"), Camera(0), pos_follow, rot_follow, 0);
    }
}

/* Same as reset_bg_camera of fileop.inc. */
reset_bg_camera()
{
    new scene = wndGetScene(WND_BG);
    if (scene)
    {
        SendMessage(scene, TAG("CRCT"), Camera(0), 0, 0, 0);
    }
}

/* Sends the camera of the background window back home ("RELS"), animated over duration ms if animate is 1. */
release_bg_camera(animate, Float:duration)
{
    new bg_scene = wndGetScene(WND_BG);
    if (bg_scene)
    {
        SendMessage(bg_scene, TAG("RELS"), animate, duration, 0, 0);
    }
}

/*
 * Creates the sce_photo table where missing and the first favorite album if there is none, and records the number of
 * albums of the library in PhotoLibraryNum; returns 0, or -1 on an error.
 */
init_photo_db()
{
    new ret;
    ret = datOpen(DAT_DATABASE, g_photo_db_name, DAT_ORDER_MTIME_DESC);
    if (ret < 0)
    {
        return -1;
    }
    ret = wait_database();
    if (ret < 0)
    {
        return -1;
    }
    new sql[128];
    sql = "CREATE TABLE sce_photo ( ";
    append_string(sql, "info_type INTEGER DEFAULT 0, ", 128);
    append_string(sql, "info_flag INTEGER DEFAULT 0, ", 128);
    append_string(sql, "title VARCHAR DEFAULT 'unknown title', ", 128);
    append_string(sql, "album_name VARCHAR DEFAULT 'unknown album', ", 128);
    append_string(sql, "content_uri VARCHAR DEFAULT NULL,", 128);
    append_string(sql, "album_uri VARCHAR DEFAULT NULL,", 128);
    append_string(sql, "create_date TIMESTAMP DEFAULT '2000/01/01 00:00:00', ", 128);
    append_string(sql, "play_order INTEGER DEFAULT 0, ", 128);
    append_string(sql, "publish_id CHAR(16) DEFAULT 'BNIMG-00000', ", 128);
    append_string(sql, "table_verion INTEGER DEFAULT 200 ", 128);
    append_string(sql, " );", 128);
    ret = datSQLExec(sql);
    ret = wait_database();
    printf("db sce_photo create %s, %d !!!!!!!!!\n", sql, ret);
    ret = datOpen(DAT_DATABASE, g_photo_db_name, DAT_ORDER_MTIME_DESC);
    if (ret < 0)
    {
        return -1;
    }
    ret = wait_database();
    if (ret < 0)
    {
        return -1;
    }
    sprintf(sql, "SELECT album_name FROM sce_photo WHERE info_type = 4");
    ret = datSQLExec(sql);
    if (ret < 0)
    {
        return -1;
    }
    ret = wait_database();
    if (ret < 0)
    {
        return -1;
    }
    new count = datGetNum();
    if (0 == count)
    {
        new album_uri[128];
        album_uri = "pfs:/__contents/contents/";
        ret = fileMkdir(album_uri);
        append_string(album_uri, "photo/", 128);
        ret = fileMkdir(album_uri);
        append_string(album_uri, "favorite1/", 128);
        ret = fileMkdir(album_uri);
        new date_str[16];
        new year;
        new month;
        new day;
        new hour;
        new minute;
        new second;
        date(year, month, day, 1);
        time(hour, minute, second, 1);
        sprintf(date_str, "%04d/%02d/%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
        new album_name[32];
        get_tag_attr("TEXT", "msgphoto_optmenu_pict_title_album", "value", album_name, 32);
        append_string(album_name, "1", 32);
        sprintf(sql,
                "INSERT INTO sce_photo ( album_name, title, info_type, album_uri, create_date ) values('%s','%s',4,'%s','%s')",
                album_name, album_name, album_uri, date_str);
        printf("test sce_photo add album %s !!!!!!!!!\n", sql);
        ret = datSQLExec(sql);
        if (ret < 0)
        {
            return -1;
        }
        ret = wait_database();
        if (ret < 0)
        {
            return -1;
        }
    }
    sprintf(sql, "SELECT album_name FROM sce_photo WHERE info_type = 3");
    ret = datSQLExec(sql);
    if (ret < 0)
    {
        return -1;
    }
    ret = wait_database();
    if (ret < 0)
    {
        return -1;
    }
    count = datGetNum();
    if (count < 0)
    {
        return -1;
    }
    set_wnd_var_int("PhotoLibraryNum", count);
    printf("init album num = %d\n", count);
    return 0;
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

/* Same as send_focus of dom.inc. */
send_focus(obj, target, z_offset, Float:sharp, Float:extra_w, Float:extra_h, Float:move_time, smoothing, blinking)
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

/* Same as set_menu_return of validate.inc. */
set_menu_return(group[], ret_group[], ret_item[])
{
    new node = domFindTag(domRoot(), "MENUGROUP", group, 0);
    if (!node)
    {
        return 0;
    }
    domSetData(node, "retgroup", ret_group);
    domSetData(node, "retitem", ret_item);
    return 1;
}

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
    new title_plane;
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
    title_plane = CreatePlane(self);
    {
        new Float:x;
        new Float:y;
        get_list_title_pos(0, item_count, 1, x, y);
        SetPos(title_plane, 0.0 * screen_w, screen_h * y, 0.0, 0);
        SetColor(title_plane, 0.902, 0.902, 0.215, 0.0);
        SetOptionI(title_plane, TAG("ZSRT"), ZSORT_OVERLAY);
        imgDecStringStart(title, 17, 64, 64, 0);
        while (IMG_DONE != imgDecSync())
        {
            sleep;
        }
        SetTexture(title_plane, imgTexID(), 0, -1);
        SetLocalScale(title_plane, float(imgTexWidth()), float(imgTexHeight()), 1.0);
        AnimColor(title_plane, 0.902, 0.902, 0.215, 1.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
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
                    new item = create_item_label(b, label);
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
    if (title_plane)
    {
        AnimColor(title_plane, 0.902, 0.902, 0.215, 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
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
    if (title_plane)
    {
        Destroy(title_plane);
        title_plane = 0;
    }
    if (result == MENU_REDRAW)
    {
        goto rebuild;
    }
    return selected;
}

/*
 * Shows the option menu of the MENUGROUP group (prefix is not used) with item selection selected, on the left of
 * the screen (flags & 0x3 is 0 or 2) or on the right (1), over a dark backdrop if flags & 0x300, and the menus
 * it leads to; returns the result, the MENU_* of handle_menu_push that ended it.
 */
pop_std_menu(prefix[], group[], flags, selection)
{
    new result;
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
    if (!get_item_attr(selection, "id", cur_id, 64))
    {
        cur_id = "";
    }
    sleep;
    for (; ; )
    {
        new args[256];
        sprintf(args, "title = %s ; group = %s ; id = %s", cur_title, cur_group, cur_id);
        AttachScript(menu_obj, "std_menu_core", "", args);
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

/* Creates a menu item: a label_texture with the label, answering circle. */
create_item_label(parent, label[])
{
    new args[128];
    new item;
    item = CreatePlane(parent);
    SetOptionI(item, TAG("ZSRT"), ZSORT_OVERLAY);
    SetColor(item, 0.215, 0.708, 0.999, 0.0);
    sprintf(args, "string = %s", label);
    AttachScript(item, "label_texture", "label_texture", args);
    SetPubVar(item, "pad_enable", PAD_CIRCLE);
    return item;
}

new g_device_image;
new Float:g_device_x = 130.0;
new Float:g_device_y = 350.0;
new Float:g_device_w = 512.0;
new Float:g_device_h = 512.0;

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

new g_focus_box;

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

new g_caption_labels[2][2]; /* two pairs of lines, shown in turn */
new g_caption_index;
new Float:g_caption_x;
new Float:g_caption_y;

set_box_title(text[])
{
    set_box_header(0, text);
}

/* Creates the labels of show_caption, with a shadow if shadow. */
create_captions(shadow)
{
    new i;
    new j;
    {
        new Float:screen_w;
        new Float:screen_h;
        get_screen_size(screen_w, screen_h);
        g_caption_x = 0.546875 * screen_w;
        g_caption_y = 0.5 * screen_h;
    }
    i = 0;
    for (; i < 2; i++)
    {
        j = 0;
        for (; j < 2; j++)
        {
            if (!shadow)
            {
                g_caption_labels[i][j] = CreateLabel(Disp2D(0));
            }
            else
            {
                g_caption_labels[i][j] = CreateLabelShadow(Disp2D(0));
            }
            SetPos(g_caption_labels[i][j], g_caption_x, g_caption_y, 0.0, 0);
            SetOptionI(g_caption_labels[i][j], TAG("FONT"), FontID(0));
            SetOptionI(g_caption_labels[i][j], TAG("HALN"), ALIGN_LEFT);
            SetOptionI(g_caption_labels[i][j], TAG("VALN"), ALIGN_CENTER);
            SetOptionI(g_caption_labels[i][j], TAG("FSIZ"), 17);
            if (shadow)
            {
                SetOptionP(g_caption_labels[i][j], TAG("SCL4"), g_shadow_colors, 4);
                SetOptionI(g_caption_labels[i][j], TAG("SX64"), 2 * 64);
                SetOptionI(g_caption_labels[i][j], TAG("SY64"), 2 * 64);
            }
            SetColor(g_caption_labels[i][j], 0.7843, 0.7843, 0.7843, 0.0);
        }
    }
}

/* Fades the caption shown last to target_a. */
fade_caption(Float:target_a, Float:duration, Float:delay)
{
    new shown = g_caption_index + 1 & 1;
    AnimColor(g_caption_labels[shown][0], 0.7843, 0.7843, 0.7843, target_a, duration, delay, ANIM_FUNC_LINEAR);
    AnimColor(g_caption_labels[shown][1], 0.7843, 0.7843, 0.7843, target_a, duration, delay, ANIM_FUNC_LINEAR);
}

/* Same as show_caption of string.inc. */
show_caption(line1[] = "", line2[] = "", Float:target_a, Float:duration, Float:delay)
{
    new incoming = g_caption_index;
    g_caption_index = ++g_caption_index & 1;
    new outgoing = g_caption_index;
    SetOptionS(g_caption_labels[incoming][0], TAG("UTF8"), line1);
    SetOptionS(g_caption_labels[incoming][1], TAG("UTF8"), line2);
    if (strlen(line2) > 0)
    {
        SetPos(g_caption_labels[incoming][0], g_caption_x, g_caption_y - 12.0, 0.0, 0);
        SetPos(g_caption_labels[incoming][1], g_caption_x, 12.0 + g_caption_y, 0.0, 0);
    }
    else
    {
        SetPos(g_caption_labels[incoming][0], g_caption_x, g_caption_y, 0.0, 0);
        SetPos(g_caption_labels[incoming][1], g_caption_x, g_caption_y, 0.0, 0);
    }
    AnimColor(g_caption_labels[incoming][0], 0.7843, 0.7843, 0.7843, target_a, duration, delay, ANIM_FUNC_LINEAR);
    AnimColor(g_caption_labels[incoming][1], 0.7843, 0.7843, 0.7843, target_a, duration, delay, ANIM_FUNC_LINEAR);
    AnimColor(g_caption_labels[outgoing][0], 0.7843, 0.7843, 0.7843, 0.0, 0.5 * duration, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_caption_labels[outgoing][1], 0.7843, 0.7843, 0.7843, 0.0, 0.5 * duration, 0.0, ANIM_FUNC_LINEAR);
}

new g_album_device[4] = {DEVICE_HDD, DEVICE_HDD, DEVICE_DISC, DEVICE_USB}; /* the device image of each ALBUM_* */

/* Same as set_caption_pos of misc.inc. */
set_caption_pos(Float:x, Float:y)
{
    g_caption_x = x;
    g_caption_y = y;
}

/* Returns the ALBUM_* of radio button button: the library and the USB device, or the favorite albums alone. */
get_button_mode(button, start_mode)
{
    if (0 == start_mode)
    {
        switch (button)
        {
            case 0:
            {
                return ALBUM_LIBRARY;
            }
            case 1:
            {
                return ALBUM_USB;
            }
            default:
            {
                return ALBUM_LIBRARY;
            }
        }
    }
    else if (start_mode == ALBUM_FAVORITE)
    {
        switch (button)
        {
            case 0:
            {
                return ALBUM_FAVORITE;
            }
            default:
            {
                return ALBUM_FAVORITE;
            }
        }
    }
    return ALBUM_LIBRARY;
}

/*
 * Shows the message of result: the TEXT done_id (and done_id2 below it) for 0 or more, err1_id for -1 and so on to
 * err5_id for -5; returns 1 if there was one to show.
 */
show_result_message(result, done_id[] = "", done_id2[] = "", err1_id[] = "", err1_id2[] = "", err2_id[] = "",
        err2_id2[] = "", err3_id[] = "", err3_id2[] = "", err4_id[] = "", err4_id2[] = "", err5_id[] = "",
        err5_id2[] = "")
{
    new msg[256] = {0};
    new line[128] = {0};
    if (result > 0)
    {
        result = 0;
    }
    switch (result)
    {
        case 0:
        {
            get_tag_attr("TEXT", done_id, "value", msg, 256);
            if (strlen(done_id2) > 0)
            {
                get_tag_attr("TEXT", done_id2, "value", line, 128);
                sprintf(msg, "%s\n%s", msg, line);
            }
        }
        case -1:
        {
            get_tag_attr("TEXT", err1_id, "value", msg, 256);
            if (strlen(err1_id2) > 0)
            {
                get_tag_attr("TEXT", err1_id2, "value", line, 128);
                sprintf(msg, "%s\n%s", msg, line);
            }
        }
        case -2:
        {
            get_tag_attr("TEXT", err2_id, "value", msg, 256);
            if (strlen(err2_id2) > 0)
            {
                get_tag_attr("TEXT", err2_id2, "value", line, 128);
                sprintf(msg, "%s\n%s", msg, line);
            }
        }
        case -3:
        {
            get_tag_attr("TEXT", err3_id, "value", msg, 256);
            if (strlen(err3_id2) > 0)
            {
                get_tag_attr("TEXT", err3_id2, "value", line, 128);
                sprintf(msg, "%s\n%s", msg, line);
            }
        }
        case -4:
        {
            get_tag_attr("TEXT", err4_id, "value", msg, 256);
            if (strlen(err4_id2) > 0)
            {
                get_tag_attr("TEXT", err4_id2, "value", line, 128);
                sprintf(msg, "%s\n%s", msg, line);
            }
        }
        case -5:
        {
            get_tag_attr("TEXT", err5_id, "value", msg, 256);
            if (strlen(err5_id2) > 0)
            {
                get_tag_attr("TEXT", err5_id2, "value", line, 128);
                sprintf(msg, "%s\n%s", msg, line);
            }
        }
    }
    if (strlen(msg) > 0)
    {
        show_message(msg, "");
        return 1;
    }
    return 0;
}

public focus; /* the focus object */

/* Shows msg in a confirm_d dialog with ok on its button and waits until it is closed. */
show_message(msg[], ok[])
{
    new args[256];
    sprintf(args, "msg=%s;ok=%s", msg, ok);
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

/* Asks the TEXT msg_id (and msg_id2, msg_id3 below it), yes selected if yes_first; returns 0 for yes, 1 for no. */
ask_yes_no(msg_id[], const msg_id2[] = "", const msg_id3[] = "", yes_first)
{
    new args[256];
    new msg[128];
    new yes[32];
    new no[32];
    get_tag_attr("TEXT", msg_id, "value", msg, 128);
    if (strlen(msg_id2) > 0)
    {
        new line[128];
        get_tag_attr("TEXT", msg_id2, "value", line, 128);
        append_string(msg, "\n", 128);
        append_string(msg, line, 128);
    }
    if (strlen(msg_id3) > 0)
    {
        new line[128];
        get_tag_attr("TEXT", msg_id3, "value", line, 128);
        append_string(msg, "\n", 128);
        append_string(msg, line, 128);
    }
    get_tag_attr("TEXT", "msgphoto_common_yes", "value", yes, 32);
    get_tag_attr("TEXT", "msgphoto_common_no", "value", no, 32);
    sprintf(args, "msg=%s;yes=%s;no=%s", msg, yes, no);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 240.0, 0.0, 0);
    AttachScript(dialog, _, "confirm_yesno", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus);
    if (yes_first)
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

/* Opens a progress_window with the TEXTs msg_id, msg_id2, msg_id3, cancel_id and breakflag_id; returns it. */
open_progress_window(msg_id[], const msg_id2[] = "", const msg_id3[] = "", const cancel_id[] = "",
        const breakflag_id[] = "")
{
    new window = CreateNull(Disp2D(0));
    {
        new args[256];
        new text[64];
        new arg[64];
        get_tag_attr("TEXT", msg_id, "value", text, 64);
        sprintf(args, "msg=%s;", text);
        if (strlen(msg_id2) > 0)
        {
            get_tag_attr("TEXT", msg_id2, "value", text, 64);
            sprintf(arg, "msg2=%s;", text);
            append_string(args, arg, 256);
        }
        if (strlen(msg_id3) > 0)
        {
            get_tag_attr("TEXT", msg_id3, "value", text, 64);
            sprintf(arg, "msg3=%s;", text);
            append_string(args, arg, 256);
        }
        if (strlen(cancel_id) > 0)
        {
            get_tag_attr("TEXT", cancel_id, "value", text, 64);
            sprintf(arg, "cancel=%s;", text);
            append_string(args, arg, 256);
        }
        if (strlen(breakflag_id) > 0)
        {
            /* NOTE: bug: the text is read from cancel_id, not breakflag_id */
            get_tag_attr("TEXT", cancel_id, "value", text, 64);
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

/*
 * Makes a Windows file name of the Shift JIS title src: \ / : * ? " < > | become spaces (it then returns 1), and the
 * double-byte characters that a file system takes badly become others; returns -1 if dest is full, 0 otherwise.
 */
make_file_name(dest[], src[], maxlen)
{
    new len = strlen(src);
    new max_chars = maxlen * 4 - 2;
    new out;
    new i;
    new ret;
    i = 0;
    for (; i < len && out < max_chars; i++)
    {
        if ((src{i} >= 0x81 && src{i} <= 0x9F) || (src{i} >= 0xE0 && src{i} <= 0xEF))
        {
            if (src{i + 1} == 0x5C || src{i + 1} == 0x7C || src{i + 1} == 0xDF)
            {
                if (src{i} == 0x83)
                {
                    dest{out++} = 0xBF;
                    i++;
                }
                else
                {
                    printf("check 5c&7c code 0x%02x 0x%02x\n", src{i}, src{i + 1});
                    dest{out++} = 0x81;
                    i++;
                    dest{out++} = 0xA0;
                }
            }
            else if (src{i} == 0x87
                    && ((src{i + 1} >= 0x54 && src{i + 1} <= 0x5E) || (src{i + 1} >= 0x80 && src{i + 1} <= 0x8C)))
            {
                printf("check ROMANcode 0x%02x 0x%02x\n", src{i}, src{i + 1});
                dest{out++} = 0x81;
                i++;
                dest{out++} = 0xA0;
            }
            else
            {
                dest{out++} = src{i};
                i++;
                dest{out++} = src{i};
            }
            continue;
        }
        if (src{i} >= 0xA1 && src{i} <= 0xDF)
        {
            dest{out++} = src{i};
            continue;
        }
        if (src{i} == '\\' || src{i} == '/' || src{i} == ':' || src{i} == '*' || src{i} == '?' || src{i} == '"'
                || src{i} == '<' || src{i} == '>' || src{i} == '|')
        {
            dest{out++} = ' ';
            ret = 1;
        }
        else
        {
            dest{out++} = src{i};
        }
    }
    dest{out++} = 0;
    if (out >= max_chars)
    {
        ret = -1;
    }
    printf("SJIS to Win Filesystem error code check %d\n", ret);
    return ret;
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
    if (0 == db_mode || db_mode == 1)
    {
        ret = datOpen(DAT_DATABASE, g_photo_db_name, DAT_ORDER_MTIME_DESC);
    }
    else if (db_mode == 3)
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

/* Moves the directory dir in place of the photo tmp directory; returns 1 if it is missing and missing_ok, 0, or -2. */
move_dir_to_photo_tmp(dir[], missing_ok)
{
    printf("mvdir filetmp\n");
    new st_mode;
    new st_size;
    new ret = fileLstat(dir, st_mode, st_size);
    if (ret == g_not_found)
    {
        if (missing_ok)
        {
            return 1;
        }
        return -2;
    }
    ret = clear_photo_tmp(0, 0, 0);
    if (ret < 0)
    {
        return -2;
    }
    ret = fileMove(dir, g_photo_tmp_dir);
    printf("MVDIR to TMP = %s\n", dir);
    if (ret < 0)
    {
        return -2;
    }
    new error;
    if (wait_file_op(ret, error, 0, 0, 100) < 0)
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
 * Makes name the first of name1 to name999 that no album of mode has; returns 1, 0 if no album name starts with name,
 * -1 on an error.
 */
make_unique_album_name(name[], mode, maxlen)
{
    new ret;
    new sql[128];
    new cond[32];
    new count;
    sprintf(cond, " album_name >= '%s' ", name);
    if (ALBUM_LIBRARY == mode)
    {
        sprintf(sql, "SELECT album_name FROM sce_photo WHERE info_type = 3 AND %s ORDER BY album_name", cond);
    }
    else if (mode == ALBUM_FAVORITE)
    {
        sprintf(sql, "SELECT album_name FROM sce_photo WHERE info_type = 4 AND %s ORDER BY album_name", cond);
    }
    else
    {
        return -1;
    }
    printf("sql com = %s\n", sql);
    ret = datSQLExec(sql);
    if (ret < 0)
    {
        return -1;
    }
    ret = wait_database();
    if (ret < 0)
    {
        return -1;
    }
    count = datGetNum();
    if (count < 0)
    {
        return -1;
    }
    if (0 == count)
    {
        return 0;
    }
    printf("check album name %s\n", name);
    new album_name[64];
    avalDelete("photo_album_name");
    for (new i; i < count; i++)
    {
        ret = get_photo_field(i, 1, album_name, -1, 0, 64);
        if (ret >= 0)
        {
            avalAdd("photo_album_name", album_name);
        }
        if (0 == i / 100)
        {
            sleep;
        }
    }
    for (new n = 1; n < 1000; n++)
    {
        new i;
        new candidate[64];
        sprintf(candidate, "%s%d", name, n);
        printf("check name %s\n", candidate);
        i = 0;
        for (; i < count; i++)
        {
            avalGetValue("photo_album_name", i, album_name, 64);
            if (!strcmp(album_name, candidate))
            {
                break;
            }
        }
        if (i >= count)
        {
            copy_string(name, candidate, maxlen);
            printf("hit dst %s\n", candidate);
            avalDelete("photo_album_name");
            return 1;
        }
        if (0 == n / 100)
        {
            sleep;
        }
    }
    avalDelete("photo_album_name");
    return -1;
}

/* Asks the name of a new album of the library or of the favorite albums, adds it and shows the result. */
make_album(focus_obj, item)
{
    new mode;
    {
        new offs_name[32];
        avalGetValue("ALBUM_OFFS", 0, offs_name, 32);
        if (!strcmp("LIBRARY", offs_name))
        {
            mode = ALBUM_LIBRARY;
        }
        else if (!strcmp("FAVORITE", offs_name))
        {
            mode = ALBUM_FAVORITE;
        }
        else
        {
            return -1;
        }
    }
    item = item * 0;
    new ret;
    focus = focus_obj;
    new progress_win;
    ret = check_pfs_free(16);
    if (ret < 0)
    {
        ret = -4;
        goto done;
    }
    ret = open_album_table(ALBUM_LIBRARY, 0);
    if (ret < 0)
    {
        ret = -1;
        goto done;
    }
    ret = check_album_count(0, 1);
    if (ret < 0)
    {
        goto done;
    }
    {
        new date_str[16];
        new name[64];
        new dir_name[16];
        new year;
        new month;
        new day;
        new hour;
        new minute;
        new second;
        date(year, month, day, 1);
        time(hour, minute, second, 1);
        sprintf(date_str, "%04d/%02d/%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
        sprintf(dir_name, "%04d-%02d-%02d_%02d-%02d-%02d", year, month, day, hour, minute, second);
        printf("create date %s!!\n", date_str);
        for (; ; )
        {
            new title[128];
            if (ALBUM_LIBRARY == mode)
            {
                get_tag_attr("TEXT", "msgphoto_album_new", "value", name, 64);
            }
            else if (mode == ALBUM_FAVORITE)
            {
                get_tag_attr("TEXT", "msgphoto_optmenu_pict_title_album", "value", name, 64);
            }
            ret = make_unique_album_name(name, mode, 64);
            if (ret < 0)
            {
                ret = -2;
                goto done;
            }
            if (0 == ret)
            {
                append_string(name, "1", 64);
            }
            get_tag_attr("TEXT", "msgphoto_album_name", "value", title, 128);
            if (run_edit_dialog(title, name, 0, 256, 256))
            {
                if (strlen(name) > 0)
                {
                    break;
                }
                else
                {
                    show_result_message(-5, _, _, _, _, _, _, _, _, _, _, "msgphoto_input_fail_noname");
                }
            }
            else
            {
                ret = -5;
                goto done;
            }
        }
        progress_win = open_progress_window("msgphoto_album_make");
        new album_uri[128];
        ret = make_photo_dirs();
        if (ret < 0)
        {
            goto done;
        }
        copy_string(album_uri, g_photo_dir, 128);
        append_string(album_uri, dir_name, 128);
        append_string(album_uri, "/", 128);
        printf("album_uri %s!!\n", album_uri);
        {
            new sql[256];
            new escaped[128];
            escape_quotes(escaped, name, 128);
            if (ALBUM_LIBRARY == mode)
            {
                sprintf(sql,
                        "INSERT INTO sce_photo ( album_name, title, info_type, album_uri, create_date ) values('%s','%s',3,'%s','%s')",
                        escaped, escaped, album_uri, date_str);
            }
            else if (mode == ALBUM_FAVORITE)
            {
                sprintf(sql,
                        "INSERT INTO sce_photo ( album_name, title, info_type, album_uri, create_date ) values('%s','%s',4,'%s','%s')",
                        escaped, escaped, album_uri, date_str);
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
        }
        SetPubVar(progress_win, "progress", 50);
        new count;
        if (ALBUM_LIBRARY == mode)
        {
            if (!get_wnd_var_int("PhotoLibraryNum", count))
            {
                count = 0;
            }
            count++;
            set_wnd_var_int("PhotoLibraryNum", count);
        }
        else if (mode == ALBUM_FAVORITE)
        {
            if (!get_wnd_var_int("PhotoFavoriteNum", count))
            {
                count = 0;
            }
            count++;
            set_wnd_var_int("PhotoFavoriteNum", count);
        }
        if (ALBUM_LIBRARY == mode)
        {
            ret = fileMkdir(album_uri);
        }
        if (ret < 0)
        {
            ret = -2;
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
    show_result_message(ret, "msgphoto_album_make_complete", _, "msgphoto_album_make_fail", _,
            "msgphoto_album_make_fail", _, "msgphoto_album_make_max", _, "msgphoto_album_make_fail", _, "");
    return ret;
}

/* Asks, deletes the album PhotoAccessAlbum (with its photos, for the library) and shows the result. */
delete_album(focus_obj, item)
{
    new mode;
    {
        new offs_name[32];
        avalGetValue("ALBUM_OFFS", 0, offs_name, 32);
        if (!strcmp("LIBRARY", offs_name))
        {
            mode = PHOTOS_LIBRARY;
        }
        else if (!strcmp("FAVORITE", offs_name))
        {
            mode = PHOTOS_FAVORITE;
        }
        else
        {
            return -1;
        }
    }
    item = item * 0;
    new ret;
    focus = focus_obj;
    {
        new answer = ask_yes_no("msgphoto_album_del_ask", _, _, 0);
        if (0 != answer)
        {
            return -1;
        }
    }
    new count = open_album_db(mode, 0);
    printf("new db opened %d\n", count);
    if (count < 0)
    {
        ret = -1;
        goto done;
    }
    if (mode == PHOTOS_LIBRARY && count > 0)
    {
        new answer = ask_yes_no("msgphoto_album_del_ask_entity", "msgphoto_album_del_ask_entity2", _, 0);
        if (0 != answer)
        {
            return -1;
        }
    }
    new progress_win = open_progress_window("msgphoto_album_del");
    clear_album_ids();
    new progress;
    {
        new album_uri[128];
        avalGetValue("PhotoAccessAlbum", 0, album_uri, 128);
        if (mode == PHOTOS_LIBRARY)
        {
            ret = move_dir_to_photo_tmp(album_uri, 1);
            if (ret < 0)
            {
                goto done;
            }
        }
        progress = 30;
        SetPubVar(progress_win, "progress", progress);
        new sql[128];
        if (mode == PHOTOS_LIBRARY)
        {
            sprintf(sql, "DELETE FROM sce_photo WHERE info_type <= 2 AND ( content_uri LIKE '%s", album_uri);
            append_string(sql, "%' )", 128);
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
        ret = 0;
        progress = 60;
        SetPubVar(progress_win, "progress", progress);
    }
    {
        new album_uri[128];
        avalGetValue("PhotoAccessAlbum", 0, album_uri, 128);
        new sql[64];
        if (mode == PHOTOS_LIBRARY)
        {
            sprintf(sql, "DELETE FROM sce_photo WHERE info_type = 3 AND album_uri = '%s'", album_uri);
        }
        else if (mode == PHOTOS_FAVORITE)
        {
            sprintf(sql, "DELETE FROM sce_photo WHERE info_type = 4 AND album_uri = '%s'", album_uri);
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
        new album_count;
        /* NOTE: bug: mode is a PHOTOS_* kind here, so the numbers of albums are never decremented */
        if (ALBUM_LIBRARY == mode)
        {
            if (!get_wnd_var_int("PhotoLibraryNum", album_count))
            {
                album_count = 1;
            }
            album_count--;
            set_wnd_var_int("PhotoLibraryNum", album_count);
        }
        else if (mode == ALBUM_FAVORITE)
        {
            if (!get_wnd_var_int("PhotoFavoriteNum", album_count))
            {
                album_count = 1;
            }
            album_count--;
            set_wnd_var_int("PhotoFavoriteNum", album_count);
        }
        add_album_id("pfs:/__contents/contents/photo/favorite1/");
        ret = update_album_covers(0);
        if (ret < 0)
        {
            ret = -1;
            goto done;
        }
        progress = progress + 10;
        SetPubVar(progress_win, "progress", progress);
        if (mode == PHOTOS_LIBRARY)
        {
            ret = clear_photo_tmp(0, 0, 0);
            if (ret < 0)
            {
                goto done;
            }
        }
        progress = 100;
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
    show_result_message(ret, "msgphoto_album_del_complete", _, "msgphoto_album_del_fail", _, "msgphoto_album_del_fail",
            _, "", _, "", _, "");
    return ret;
}

/* Asks, deletes photo item (with its file for the library, else from the favorite album only), shows the result. */
delete_photo(focus_obj, item)
{
    new mode;
    {
        new mode_name[32];
        avalGetValue("ALBUM_MODE", 0, mode_name, 32);
        if (!strcmp("LIBRARY", mode_name))
        {
            mode = ALBUM_LIBRARY;
        }
        else if (!strcmp("FAVORITE", mode_name))
        {
            mode = ALBUM_FAVORITE;
        }
        else
        {
            return -1;
        }
    }
    new ret;
    focus = focus_obj;
    if (ALBUM_LIBRARY == mode)
    {
        new answer = ask_yes_no("msgphoto_pict_del_ask", _, _, 0);
        if (0 != answer)
        {
            return -1;
        }
        answer = ask_yes_no("msgphoto_pict_del_ask_entity", "msgphoto_pict_del_ask_entity2", _, 0);
        if (0 != answer)
        {
            return -1;
        }
    }
    else if (mode == ALBUM_FAVORITE)
    {
        new answer = ask_yes_no("msgphoto_pict_del_favorite_ask", _, _, 0);
        if (0 != answer)
        {
            return -1;
        }
    }
    new progress_win = open_progress_window("msgphoto_pict_del");
    new count = open_album_db(mode + PHOTOS_LIBRARY, 0);
    printf("new db opened %d\n", count);
    if (count < 0)
    {
        ret = -1;
        goto done;
    }
    new progress = 33;
    SetPubVar(progress_win, "progress", progress);
    printf("selected item %d ------------------\n", item);
    {
        new oid[32];
        new uri[128];
        get_photo_field(item, FIELD_OID, oid, mode + PHOTOS_LIBRARY, 0, 32);
        get_photo_field(item, FIELD_URL, uri, mode + PHOTOS_LIBRARY, 0, 128);
        if (strlen(uri) <= 0)
        {
            ret = -1;
            goto done;
        }
        new tmp_path[128];
        if (ALBUM_LIBRARY == mode)
        {
            ret = prepare_photo_tmp(0, 0, 0);
            if (ret < 0)
            {
                goto done;
            }
            ret = move_to_photo_tmp(tmp_path, uri, 1, 128, 0, 0, 0);
            if (ret < 0)
            {
                goto done;
            }
        }
        {
            new sql[64];
            if (ALBUM_LIBRARY == mode)
            {
                sprintf(sql, "DELETE FROM sce_photo WHERE info_type <= 2 AND content_uri = '%s'", uri);
            }
            else if (mode == ALBUM_FAVORITE)
            {
                sprintf(sql, "DELETE FROM sce_photo WHERE info_type = 2 AND content_uri = '%s' AND oid = '%s'", uri,
                        oid);
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
            clear_album_ids();
            ret = collect_photo_albums(uri, mode);
            if (ret < 0)
            {
                ret = -1;
                goto done;
            }
            ret = update_album_covers(mode);
            if (ret < 0)
            {
                ret = -1;
                goto done;
            }
            progress = progress + 50;
            SetPubVar(progress_win, "progress", progress);
        }
        if (ALBUM_LIBRARY == mode)
        {
            ret = clear_photo_tmp(0, 0, 0);
            if (ret < 0)
            {
                goto done;
            }
        }
        progress = 100;
        SetPubVar(progress_win, "progress", progress);
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
    show_result_message(ret, "msgphoto_pict_del_complete", _, "msgphoto_pict_del_fail", _, "msgphoto_pict_del_fail", _,
            "", _, "", _, "");
    return ret;
}

/* Same as delete_usb_photo of photousb.inc. */
delete_usb_photo(focus_obj, item)
{
    new album_mode;
    {
        new mode_text[32];
        avalGetValue("ALBUM_MODE", 0, mode_text, 32);
        if (!strcmp("USB", mode_text))
        {
            album_mode = ALBUM_USB;
        }
        else
        {
            return -1;
        }
    }
    new ret;
    new progress_win;
    focus = focus_obj;
    {
        new answer = ask_yes_no("msgphoto_pict_del_ask", _, _, 0);
        if (0 != answer)
        {
            return -1;
        }
    }
    new invalid;
    if (get_wnd_var_int("usb_invalid", invalid))
    {
        if (0 != invalid)
        {
            ret = -5;
            goto done;
        }
    }
    new path[128];
    photoListGet(item, path, 128);
    if (strlen(path) <= 0)
    {
        ret = -1;
        goto done;
    }
    {
        new st_mode;
        new st_size;
        ret = fileLstat(path, st_mode, st_size);
        if (!(st_mode & 0x80)) /* not writable by its owner */
        {
            new answer2 = ask_yes_no("msgphoto_pict_del_ask_protect", "msgphoto_pict_del_ask_entity2", _, 0);
            if (0 != answer2)
            {
                return -1;
            }
        }
    }
    progress_win = open_progress_window("msgphoto_pict_del");
    SetPubVar(progress_win, "progress", 15);
    if (get_wnd_var_int("usb_invalid", invalid))
    {
        if (0 != invalid)
        {
            ret = -5;
            goto done;
        }
    }
    SetPubVar(progress_win, "progress", 50);
    printf("selected item %d ------------------\n", item);
    if (get_wnd_var_int("usb_invalid", invalid))
    {
        if (0 != invalid)
        {
            ret = -5;
            goto done;
        }
    }
    if (album_mode == ALBUM_USB)
    {
        new error;
        ret = fileDeleteUSB(path);
        if (ret == 1)
        {
            ret = -3;
            goto done;
        }
        if (ret < 0)
        {
            ret = -2;
            goto done;
        }
        if (wait_file_op(ret, error, progress_win, 33, 99) < 0)
        {
            if (ret < 0)
            {
                ret = -2;
                goto done;
            }
            if (error == 2)
            {
                ret = -4;
                goto done;
            }
        }
        if (get_wnd_var_int("usb_invalid", invalid))
        {
            if (0 != invalid)
            {
                ret = -5;
                goto done;
            }
        }
    }
    sleep;
    {
        new st_mode2;
        new st_size2;
        ret = fileLstat(path, st_mode2, st_size2);
        if (ret != g_not_found)
        {
            ret = -2;
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
    printf("delete ret %d\n", ret);
    show_result_message(ret, "msgphoto_pict_del_complete", _, "msgphoto_pict_del_fail", _, "msgphoto_pict_del_fail", _,
            "msgphoto_pict_del_fail", "msgphoto_pict_checkout_mswrite_disable", "", _, "msgphoto_pict_del_fail",
            "msgphoto_warn_usb_connect");
    return ret;
}

/*
 * Asks, copies photo item into the photo folder of the USB storage (made where missing), named after its title, and
 * shows the result; returns 0 or a negative error.
 */
export_photo_to_usb(focus_obj, item)
{
    focus = focus_obj;
    new ret;
    new error;
    new progress_win;
    new invalid;
    ret = ask_yes_no("msgphoto_pict_checkout_ask", _, _, 0);
    if (0 != ret)
    {
        return -1;
    }
    if (get_wnd_var_int("usb_invalid", invalid))
    {
        if (invalid > 0)
        {
            ret = -5;
            goto done;
        }
        if (invalid <= 0)
        {
            if (get_wnd_var_int("usb_found", invalid))
            {
                if (invalid)
                {
                    set_wnd_var_int("usb_invalid", 0);
                }
                else
                {
                    ret = -5;
                    goto done;
                }
            }
        }
    }
    progress_win = open_progress_window("msgphoto_pict_checkin", "msgphoto_warn_external_device_eject",
            "msgphoto_warn_console_power");
    new db = open_album_db(4, 0);
    printf("new db opened %d\n", db);
    if (db < 0)
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
    {
        new buf[128];
        new dir[128];
        new st_mode;
        new st_size;
        get_tag_attr("TEXT", "text_USBMOUNT", "value", dir, 128);
        append_string(dir, "/", 128);
        sleep;
        ret = fileLstat(dir, st_mode, st_size);
        if (get_wnd_var_int("usb_invalid", invalid))
        {
            if (0 != invalid)
            {
                ret = -5;
                goto done;
            }
        }
        if (ret == g_not_found)
        {
            ret = fileMkdir(dir);
            if (ret < 0)
            {
                ret = -2;
                goto done;
            }
        }
        else if (ret < 0)
        {
            ret = -2;
            goto done;
        }
        get_tag_attr("TEXT", "text_USBMSSONY", "value", buf, 128);
        append_string(dir, buf, 128);
        append_string(dir, "/", 128);
        sleep;
        ret = fileLstat(dir, st_mode, st_size);
        if (get_wnd_var_int("usb_invalid", invalid))
        {
            if (0 != invalid)
            {
                ret = -5;
                goto done;
            }
        }
        if (ret == g_not_found)
        {
            ret = fileMkdir(dir);
            if (ret < 0)
            {
                ret = -2;
                goto done;
            }
        }
        else if (ret < 0)
        {
            ret = -2;
            goto done;
        }
        get_tag_attr("TEXT", "text_USBPSBB", "value", buf, 128);
        append_string(dir, buf, 128);
        append_string(dir, "/", 128);
        sleep;
        ret = fileLstat(dir, st_mode, st_size);
        if (get_wnd_var_int("usb_invalid", invalid))
        {
            if (0 != invalid)
            {
                ret = -5;
                goto done;
            }
        }
        if (ret == g_not_found)
        {
            ret = fileMkdir(dir);
            if (ret < 0)
            {
                ret = -2;
                goto done;
            }
        }
        else if (ret < 0)
        {
            ret = -2;
            goto done;
        }
        get_tag_attr("TEXT", "text_USBPHOTO", "value", buf, 128);
        append_string(dir, buf, 128);
        append_string(dir, "/", 128);
        sleep;
        ret = fileLstat(dir, st_mode, st_size);
        if (get_wnd_var_int("usb_invalid", invalid))
        {
            if (0 != invalid)
            {
                ret = -5;
                goto done;
            }
        }
        if (ret == g_not_found)
        {
            ret = fileMkdir(dir);
            if (ret < 0)
            {
                ret = -2;
                goto done;
            }
        }
        else if (ret < 0)
        {
            ret = -2;
            goto done;
        }
        SetPubVar(progress_win, "progress", 33);
        new ext[32];
        new src[128];
        photoListGet(item, src, 128);
        get_extension(ext, src, 32);
        printf("usb checkout dir %s\n", dir);
        {
            new file_name[128];
            get_photo_field(item, FIELD_TITLE, buf, PHOTOS_LIBRARY, 0, 128);
            photoUTF8toSJIS(buf, buf);
            ret = make_file_name(file_name, buf, 128);
            if (ret == 1)
            {
                ret = -255;
                goto done;
            }
            if (!ends_with_nocase(file_name, ext))
            {
                append_string(file_name, ext, 128);
            }
            sprintf(buf, "%s%s", dir, file_name);
            sleep;
            ret = fileLstat(buf, st_mode, st_size);
            if (get_wnd_var_int("usb_invalid", invalid))
            {
                if (0 != invalid)
                {
                    ret = -5;
                    goto done;
                }
            }
            printf("dst_url check %d , %s\n", ret, buf);
            if (ret == g_not_found)
            {
                copy_string(dir, buf, 128);
            }
            else
            {
                if (ret < 0)
                {
                    ret = -2;
                    goto done;
                }
                if (ret >= 0)
                {
                    ret = -3;
                    goto done;
                }
            }
        }
        printf("src %s\n", src);
        printf("dst %s\n", dir);
        SetPubVar(progress_win, "progress", 50);
        sleep;
        if (get_wnd_var_int("usb_invalid", invalid))
        {
            if (0 != invalid)
            {
                ret = -5;
                goto done;
            }
        }
        ret = fileCopy(src, dir);
        if (ret < 0)
        {
            ret = -2;
            goto done;
        }
        if (wait_file_op(ret, error, progress_win, 50, 99) < 0)
        {
            if (ret < 0)
            {
                ret = -2;
                goto done;
            }
            if (error == 2)
            {
                ret = -4;
                goto done;
            }
        }
        if (get_wnd_var_int("usb_invalid", invalid))
        {
            if (0 != invalid)
            {
                ret = -5;
                goto done;
            }
        }
        sleep;
        ret = fileLstat(dir, st_mode, st_size);
        if (ret == g_not_found)
        {
            ret = -2;
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
    if (ret == -255)
    {
        new msg[256];
        new line_text[128];
        get_tag_attr("TEXT", "msgphoto_pict_checkin_fail", "value", msg, 256);
        get_tag_attr("TEXT", "msgphoto_pict_checkout_warn_string", "value", line_text, 128);
        append_string(msg, "\n", 256);
        append_string(msg, line_text, 256);
        append_string(msg, "\n", 256);
        sprintf(line_text, "                     \\ / : * ? \" < > | ");
        append_string(msg, "\n", 256);
        append_string(msg, line_text, 256);
        show_message(msg, "");
    }
    else
    {
        show_result_message(ret, "msgphoto_pict_checkin_complete", _, "msgphoto_pict_checkin_fail",
                "msgphoto_pict_checkin_fail_possible", "msgphoto_pict_checkin_fail",
                "msgphoto_pict_checkin_fail_possible", "msgphoto_pict_checkin_fail", "msgphoto_usb_samedata",
                "msgphoto_pict_checkin_fail", "msgphoto_pict_external_device_nospace", "msgphoto_pict_checkin_fail",
                "msgphoto_warn_usb_connect");
    }
    return ret;
}

public focus_glowfly; /* the focus plane of the page */
public select = -1; /* the album or photo of the option menu, read by handle_menu_push */
public decstate = DECSTATE_BROKEN; /* the photo under the cursor, DECSTATE_* */
new g_page_mode; /* PAGE_*: the view that runs, or the switch in progress */
new Float:g_screen_w;
new Float:g_screen_h;
new g_usb_checker; /* runs usb_media_check of usb_process */
new g_album_mode = ALBUM_FAVORITE; /* ALBUM_*: the albums shown */
new g_start_mode; /* the albums the page was opened on: ALBUM_FAVORITE keeps the page on the favorite albums */
new g_usb_changed; /* the navigator menu opened on the USB albums: reload them */
new g_next_link[128]; /* the page to leave for: a "LINK" id or "top" */
new g_next_args[128];

/* Builds the page: the background, the camera, the USB checker, the focus, the box and the captions. */
init_page()
{
    start_top_page(0);
    load_common_bg("bg", 1000.0);
    get_screen_size(g_screen_w, g_screen_h);
    g_next_link = "top";
    g_next_args = "";
    photoListClear();
    create_camera();
    link_bg_camera(2.0, 0);
    reset_bg_camera();
    set_background_poor(600.0, 0);
    g_usb_checker = CreateNull(Self());
    AttachScript(g_usb_checker, "usb_media_check", "usb_process");
    focus_glowfly = create_focus_plane("focus");
    SetOptionF(focus_glowfly, TAG("ZOFS"), 5.0);
    create_device_image();
    create_focus_box(0.7625 * g_screen_w, 0.5 * g_screen_h);
    fade_box(0.0, 100.0, 1.0, 1.0, 1.0);
    set_box_focus(focus_glowfly);
    set_box_title("");
    create_captions(0);
}

/* Leaves for the page of g_next_link ("top": the built-in top menu) with g_next_args. */
load_next_page()
{
    fade_background(0, 0, 600.0);
    release_bg_camera(1, 2000.0);
    if (!strcmp(g_next_link, "top"))
    {
        xmlLoadStart("$$BUILDIN_TOPMENU$$", _, g_next_args, XMLLOAD_MAIN);
    }
    else
    {
        new url[256];
        get_tag_attr("LINK", g_next_link, "href", url, 256);
        xmlLoadStart(url, _, _, XMLLOAD_MAIN);
    }
    wait_xml_load();
}

public camera_focus; /* the object the camera looks at */

/* Points the camera at a new camera_focus and attaches camera_main, which keeps it there. */
create_camera()
{
    new camera = Camera(0);
    camera_focus = CreateNull(Self());
    SetPos(camera_focus, 0.0, 0.0, -100.0, 0);
    SetPos(camera, 0.0, 0.0, 0.0, 0);
    LookAt(camera, camera_focus, 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
    AttachScript(Camera(0), "camera_main", "photo_parts");
    SetPubVar(Camera(0), "camera_focus", camera_focus);
}

/* Moves the camera to the left (dir 0) or to the right for a menu. */
slide_camera(dir)
{
    if (0 == dir)
    {
        AnimMoveHermite(Camera(0), -280.0, 0.0, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
        AnimMoveHermite(camera_focus, -280.0, 0.0, -100.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
    }
    else if (dir == 1)
    {
        AnimMoveHermite(Camera(0), 300.0, 0.0, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
        AnimMoveHermite(camera_focus, 300.0, 0.0, -100.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
    }
    wait_ms(floatround(750.0, 0));
}

center_camera()
{
    AnimMoveHermite(Camera(0), 0.0, 0.0, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
    AnimMoveHermite(camera_focus, 0.0, 0.0, -100.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
    wait_ms(floatround(750.0, 0));
}

/*
 * Returns the number of items an option menu has besides those of its MENUGROUP: in the LIST_LIBRARY and LIST_FAVORITE
 * lists, the albums that can be the destination; with none, it shows why and goes back (MENU_BACK).
 */
count_extra_items(side, &result, &mode, title[], group[], id[], &selected)
{
    new count;
    new buffer[64];
    count = side;
    count = result;
    count = mode;
    count = selected;
    copy_string(buffer, title, 64);
    copy_string(buffer, group, 64);
    copy_string(buffer, id, 64);
    count = 0;
    switch (mode)
    {
        case LIST_GROUP:
        {
            count = 0;
        }
        case LIST_LIBRARY:
        {
            new ret = open_album_table(ALBUM_LIBRARY, 0);
            new menu_mode[16];
            get_wnd_var_str("MENU_MODE", menu_mode, 16);
            {
                new sql[128];
                new album_uri[64];
                avalGetValue("PhotoAccessAlbum", 0, album_uri, 64);
                if (!strcmp(menu_mode, "COPY") || !strcmp(menu_mode, "IMPORT"))
                {
                    sprintf(sql, "SELECT *, oid FROM sce_photo WHERE info_type = 3 ORDER BY create_date DESC");
                }
                else if (!strcmp(menu_mode, "MOVE"))
                {
                    sprintf(sql,
                            "SELECT *, oid FROM sce_photo WHERE info_type = 3 AND album_uri != '%s' ORDER BY create_date DESC",
                            album_uri);
                }
                printf("sql com = %s\n", sql);
                ret = datSQLExec(sql);
                if (ret < 0)
                {
                    ret = -1;
                }
                ret = wait_database();
                if (ret < 0)
                {
                    ret = -1;
                }
                count = datGetNum();
            }
            printf("list DB0 LIBRARY open %d------!!!!\n", count);
            if (count <= 0 || ret < 0)
            {
                if (0 == count)
                {
                    ret = -5;
                }
                if (!strcmp(menu_mode, "COPY"))
                {
                    show_result_message(ret, _, _, "msgphoto_copy_fail", _, "msgphoto_copy_fail");
                }
                else if (!strcmp(menu_mode, "MOVE"))
                {
                    show_result_message(ret, _, _, "msgphoto_move_fail", _, "msgphoto_move_fail", _, _, _, _, _,
                            "msgphoto_move_warn_nodevice");
                }
                else if (!strcmp(menu_mode, "IMPORT"))
                {
                    show_result_message(ret, _, _, "msgphoto_pict_checkin_fail", _, "msgphoto_pict_checkin_fail", _, _,
                            _, _, _, "msgphoto_pict_usb_nodevice");
                }
                mode = LIST_GROUP;
                result = MENU_BACK;
            }
        }
        case LIST_FAVORITE:
        {
            new ret = open_album_table(ALBUM_FAVORITE, 0);
            {
                new sql[128];
                new album_uri[64];
                avalGetValue("PhotoAccessAlbum", 0, album_uri, 64);
                sprintf(sql, "SELECT oid FROM sce_photo WHERE info_type = 1 AND album_uri = '%s'", album_uri);
                printf("sql com = %s\n", sql);
                ret = datSQLExec(sql);
                if (ret < 0)
                {
                    ret = -1;
                }
                ret = wait_database();
                if (ret < 0)
                {
                    ret = -1;
                }
                if (datGetNum() < 0)
                {
                    ret = -1;
                }
                else if (0 == datGetNum())
                {
                    ret = -5;
                }
                else
                {
                    sprintf(sql, "SELECT *, oid FROM sce_photo WHERE info_type = 4 ORDER BY create_date DESC");
                    printf("sql com = %s\n", sql);
                    ret = datSQLExec(sql);
                    if (ret < 0)
                    {
                        ret = -1;
                    }
                    ret = wait_database();
                    if (ret < 0)
                    {
                        ret = -1;
                    }
                    count = datGetNum();
                    if (0 == count)
                    {
                        ret = -1;
                    }
                }
            }
            printf("list DB0 FAVORITE open %d------!!!!\n", count);
            if (count <= 0 || ret < 0)
            {
                show_result_message(ret, _, _, "msgphoto_regpict_fail", _, "msgphoto_regpict_fail", _, _, _, _, _,
                        "msgphoto_regpict_empty");
                mode = LIST_GROUP;
                result = MENU_BACK;
            }
        }
    }
    return count;
}

public menu_ret_val; /* the result the processes of menu_process give back */

/* Gives the item index of the album lists the name of its album. */
fill_menu_item(side, mode, index, group[], label[])
{
    new unused; /* takes the parameters the function does not use */
    new buffer[64];
    unused = side;
    unused = mode;
    unused = index;
    unused = 64;
    copy_string(buffer, group, unused);
    switch (mode)
    {
        case LIST_GROUP:
        {
        }
        case LIST_LIBRARY:
        {
            new name[64];
            get_photo_field(index, FIELD_ALBUM_NAME, name, ALBUM_LIBRARY, 0, 64);
            copy_string(label, name, 64);
        }
        case LIST_FAVORITE:
        {
            new name[64];
            get_photo_field(index, FIELD_ALBUM_NAME, name, ALBUM_FAVORITE, 0, 64);
            copy_string(label, name, 64);
        }
    }
}

/*
 * Runs the option of a push in an option menu and returns its MENU_* result: circle runs item id (or, in the album
 * lists, adds, copies, moves or imports into album index), cross goes back.
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
    copy_string(buffer, group, unused);
    copy_string(buffer, title, unused);
    menu_ret_val = 0;
    new result = MENU_REOPEN;
    if (button == TAG("RRGT"))
    {
        printf("stdmenu btn RRGT\n");
        new focus_obj = GetPubVar(manager, "focus_glowfly");
        new item = GetPubVar(manager, "select");
        new item_decstate = GetPubVar(manager, "decstate");
        printf("stdmenu get select %d !!!!!!!!!!!\n", item);
        switch (mode)
        {
            case LIST_GROUP:
            {
                if (!strcmp("DEVOPT_CREATE", id))
                {
                    new ret = make_album(focus_obj, item);
                    mode = LIST_GROUP;
                    if (ret >= 0)
                    {
                        result = MENU_ALBUMS_CHANGED;
                    }
                    else
                    {
                        result = MENU_STAY;
                    }
                }
                else if (!strcmp("ALBUMOPT_ADDFAV", id))
                {
                    get_tag_attr("TEXT", "msgphoto_regpict_ask", "value", title, 64);
                    mode = LIST_FAVORITE;
                    result = MENU_SUBMENU;
                }
                else if (!strcmp("ALBUMOPT_DELETE", id))
                {
                    new ret = delete_album(focus_obj, item);
                    mode = LIST_GROUP;
                    if (ret >= 0)
                    {
                        result = MENU_ALBUMS_CHANGED;
                    }
                    else
                    {
                        result = MENU_STAY;
                    }
                }
                else if (!strcmp("ALBUMOPT_INFO", id) || !strcmp("ALBUMUSB_INFO", id))
                {
                    {
                        new args[128];
                        new url[64];
                        new device[32];
                        avalGetValue("ALBUM_MODE", 0, device, 32);
                        sprintf(args, "mode=INFO_ALBUM; device=%s; select = %d", device, item);
                        get_tag_attr("LINK", "infomenu_album", "href", url, 64);
                        new st_mode;
                        new st_size;
                        if (0 == fileLstat(url, st_mode, st_size))
                        {
                            printf("load xml %s, %s\n", url, args);
                            SetSystemParam(SYS_PADHELP, 0);
                            xmlLoadStart(url, _, args, XMLLOAD_MODAL);
                            wait_xml_load();
                        }
                    }
                    mode = LIST_GROUP;
                    new status[32];
                    avalGetValue("InfoStatus", 0, status, 32);
                    if (!strcmp(status, "INVALID"))
                    {
                        result = MENU_RENAMED;
                    }
                    else if (!strcmp(status, "ERROR"))
                    {
                        result = MENU_CLOSED;
                    }
                    else if (!strcmp(status, "NONE"))
                    {
                        result = MENU_STAY;
                    }
                }
                else if (!strcmp("THUMBOPT_ADDFAV", id))
                {
                    get_tag_attr("TEXT", "msgphoto_regpict_ask", "value", title, 64);
                    mode = LIST_FAVORITE;
                    result = MENU_SUBMENU;
                }
                else if (!strcmp("THUMBOPT_COPY", id))
                {
                    get_tag_attr("TEXT", "msgphoto_copy_select_devices", "value", title, 64);
                    set_wnd_var_str("MENU_MODE", "COPY");
                    mode = LIST_LIBRARY;
                    result = MENU_SUBMENU;
                }
                else if (!strcmp("THUMBOPT_MOVE", id))
                {
                    get_tag_attr("TEXT", "msgphoto_move_select_devices", "value", title, 64);
                    set_wnd_var_str("MENU_MODE", "MOVE");
                    mode = LIST_LIBRARY;
                    result = MENU_SUBMENU;
                }
                else if (!strcmp("THUMBOPT_EXPORT", id))
                {
                    new ret = export_photo_to_usb(focus_obj, item);
                    mode = LIST_GROUP;
                    if (ret >= 0)
                    {
                        result = MENU_STAY;
                    }
                    else
                    {
                        result = MENU_STAY;
                    }
                }
                else if (!strcmp("THUMBOPT_DELETE", id) || !strcmp("THUMBFAV_DELETE", id))
                {
                    new ret = delete_photo(focus_obj, item);
                    mode = LIST_GROUP;
                    if (ret >= 0)
                    {
                        result = MENU_CHANGED;
                    }
                    else
                    {
                        result = MENU_STAY;
                    }
                }
                else if (!strcmp("THUMBUSB_DELETE", id))
                {
                    new ret = delete_usb_photo(focus_obj, item);
                    mode = LIST_GROUP;
                    if (ret >= 0)
                    {
                        result = MENU_CHANGED;
                    }
                    else
                    {
                        result = MENU_STAY;
                    }
                    new usb_invalid;
                    if (get_wnd_var_int("usb_invalid", usb_invalid))
                    {
                        if (0 != usb_invalid)
                        {
                            result = MENU_USB_LOST;
                        }
                    }
                }
                else if (!strcmp("THUMBUSB_IMPORT", id))
                {
                    get_tag_attr("TEXT", "msgphoto_pict_send_device_list", "value", title, 64);
                    set_wnd_var_str("MENU_MODE", "IMPORT");
                    mode = LIST_LIBRARY;
                    result = MENU_SUBMENU;
                }
                else if (!strcmp("THUMBOPT_INFO", id) || !strcmp("THUMBFAV_INFO", id) || !strcmp("THUMBUSB_INFO", id))
                {
                    {
                        new args[128];
                        new url[64];
                        new device[32];
                        avalGetValue("ALBUM_MODE", 0, device, 32);
                        if (!strcmp("THUMBUSB_INFO", id))
                        {
                            sprintf(args, "mode=INFO_USBTMB; device=%s; select=%d", device, item);
                        }
                        else
                        {
                            sprintf(args, "mode=INFO_THUMB; device=%s; select=%d", device, item);
                        }
                        get_tag_attr("LINK", "infomenu_album", "href", url, 64);
                        new st_mode;
                        new st_size;
                        if (0 == fileLstat(url, st_mode, st_size))
                        {
                            printf("load xml %s, %s\n", url, args);
                            SetSystemParam(SYS_PADHELP, 0);
                            xmlLoadStart(url, _, args, XMLLOAD_MODAL);
                            wait_xml_load();
                        }
                    }
                    mode = LIST_GROUP;
                    new status[32];
                    avalGetValue("InfoStatus", 0, status, 32);
                    if (!strcmp(status, "INVALID"))
                    {
                        result = MENU_RENAMED;
                    }
                    else if (!strcmp(status, "USBINVALID"))
                    {
                        set_wnd_var_int("usb_invalid", 1);
                        result = MENU_USB_LOST;
                    }
                    else if (!strcmp(status, "ERROR"))
                    {
                        result = MENU_CLOSED;
                    }
                    else if (!strcmp(status, "NONE"))
                    {
                        result = MENU_STAY;
                    }
                    if (!strcmp("THUMBUSB_INFO", id))
                    {
                        sleep;
                        new usb_invalid;
                        if (get_wnd_var_int("usb_invalid", usb_invalid))
                        {
                            if (0 != usb_invalid)
                            {
                                result = MENU_USB_LOST;
                            }
                        }
                    }
                }
            }
            case LIST_FAVORITE:
            {
                new album_uri[128];
                get_photo_field(index, FIELD_ALBUM_URI, album_uri, ALBUM_FAVORITE, 0, 128);
                printf("destination album %d, %s\n", index, album_uri);
                if (!strcmp("ALBUMOPT_ADDFAV", id) || !strcmp("THUMBOPT_ADDFAV", id))
                {
                    new proc = CreateNull(Self());
                    {
                        new args[128];
                        sprintf(args, "dst_album = %s; decstate = %d", album_uri, item_decstate);
                        if (!strcmp("ALBUMOPT_ADDFAV", id))
                        {
                            AttachScript(proc, "album_addfav_proc", "menu_process", args);
                        }
                        else
                        {
                            AttachScript(proc, "content_addfav_proc", "menu_process", args);
                        }
                        SendMessage(proc, TAG("SLCT"), item, 0, 0, 0);
                    }
                    SetPubVar(proc, "focus", focus_obj);
                    while (SCRIPT_FINISHED != ScriptState(proc))
                    {
                        sleep;
                    }
                    Destroy(proc);
                    mode = LIST_GROUP;
                    if (menu_ret_val >= 0)
                    {
                        result = MENU_RETURN;
                    }
                    else
                    {
                        result = MENU_RETURN;
                    }
                }
            }
            case LIST_LIBRARY:
            {
                new album_uri[128];
                get_photo_field(index, FIELD_ALBUM_URI, album_uri, ALBUM_LIBRARY, 0, 128);
                printf("destination album %d, %s\n", index, album_uri);
                if (!strcmp("THUMBOPT_MOVE", id))
                {
                    new proc = CreateNull(Self());
                    {
                        new args[128];
                        sprintf(args, "dst_album = %s", album_uri);
                        AttachScript(proc, "content_move_proc", "menu_process", args);
                        SendMessage(proc, TAG("SLCT"), item, 0, 0, 0);
                    }
                    SetPubVar(proc, "focus", focus_obj);
                    while (SCRIPT_FINISHED != ScriptState(proc))
                    {
                        sleep;
                    }
                    Destroy(proc);
                    mode = LIST_GROUP;
                    if (menu_ret_val >= 0)
                    {
                        result = MENU_CHANGED;
                    }
                    else
                    {
                        result = MENU_RETURN;
                    }
                }
                else if (!strcmp("THUMBOPT_COPY", id))
                {
                    new proc = CreateNull(Self());
                    {
                        new args[128];
                        sprintf(args, "dst_album = %s", album_uri);
                        AttachScript(proc, "content_copy_proc", "menu_process", args);
                        SendMessage(proc, TAG("SLCT"), item, 0, 0, 0);
                    }
                    SetPubVar(proc, "focus", focus_obj);
                    while (SCRIPT_FINISHED != ScriptState(proc))
                    {
                        sleep;
                    }
                    Destroy(proc);
                    mode = LIST_GROUP;
                    if (menu_ret_val >= 1)
                    {
                        result = MENU_CHANGED;
                    }
                    else if (0 == menu_ret_val)
                    {
                        result = MENU_RETURN;
                    }
                    else
                    {
                        result = MENU_RETURN;
                    }
                }
                else if (!strcmp("THUMBUSB_IMPORT", id))
                {
                    new proc = CreateNull(Self());
                    {
                        new args[128];
                        sprintf(args, "dst_album = %s", album_uri);
                        AttachScript(proc, "content_import_proc", "menu_process", args);
                        SendMessage(proc, TAG("SLCT"), item, 0, 0, 0);
                    }
                    SetPubVar(proc, "focus", focus_obj);
                    while (SCRIPT_FINISHED != ScriptState(proc))
                    {
                        sleep;
                    }
                    Destroy(proc);
                    mode = LIST_GROUP;
                    if (menu_ret_val >= 0)
                    {
                        result = MENU_RETURN;
                    }
                    else
                    {
                        result = MENU_RETURN;
                    }
                    new usb_invalid;
                    if (get_wnd_var_int("usb_invalid", usb_invalid))
                    {
                        if (0 != usb_invalid)
                        {
                            result = MENU_USB_LOST;
                        }
                    }
                }
            }
        }
    }
    else if (button == TAG("RDWN"))
    {
        printf("stdmenu btn RDWN\n");
        switch (mode)
        {
            case LIST_GROUP:
            {
                mode = LIST_GROUP;
                result = MENU_BACK;
            }
            case LIST_LIBRARY:
            {
                mode = LIST_GROUP;
                result = MENU_RETURN;
            }
            case LIST_FAVORITE:
            {
                mode = LIST_GROUP;
                result = MENU_RETURN;
            }
        }
    }
    return result;
}

new g_usb_invalid;
new g_usb_found;
new g_mode_buttons;
new g_active; /* g_mode_buttons or g_album: the one that takes the pad */
new g_album;
new g_album_count;
new g_enter_album; /* the focus goes into the album once it is idle */
new g_album_idle; /* the album has stopped moving */
new g_default_icon;
new g_db_server; /* the object of database_process (photo_db) */
new g_album_select = -1; /* the selected album; -1 for none */
new g_first_load = 1; /* the first load checks the database first (init_photo_db) */
new g_albums_state = VIEW_OFF; /* VIEW_* of the album view */

/* Builds the album view: the database object, the radio buttons and the album of the argument mode. */
init_albums()
{
    g_db_server = CreateNull(Self());
    AttachScript(g_db_server, "database_process", "photo_db");
    g_default_icon = load_icon_texture("default_icon");
    g_album_mode = ALBUM_LIBRARY;
    new arg[64];
    GetArg("mode", arg, 64);
    if (!strcmp("favorite", arg))
    {
        g_album_mode = ALBUM_FAVORITE;
        g_start_mode = ALBUM_FAVORITE;
        avalSet("ALBUM_MODE", "FAVORITE");
        avalSet("ALBUM_OFFS", "FAVORITE");
    }
    else
    {
        g_album_mode = ALBUM_LIBRARY;
        g_start_mode = ALBUM_LIBRARY;
        avalSet("ALBUM_MODE", "LIBRARY");
        avalSet("ALBUM_OFFS", "LIBRARY");
    }
    g_mode_buttons = create_mode_buttons();
    show_device_image(g_album_device[g_album_mode]);
    g_enter_album = 0;
    SetPubVar(g_mode_buttons, "select", 0);
    SetPubVar(g_mode_buttons, "cursor", 0);
    SendMessage(g_mode_buttons, TAG("FADE"), 1.0, 400.0, 0, 0);
    new from_photos;
    new from[32];
    GetArg("from", from, 32);
    if (!strcmp("PHOTO_THUMBNAIL", from))
    {
        from_photos = 1;
    }
    new reload_mode = g_album_mode;
    g_album_mode--; /* differs from the mode, so that change_album reloads the album */
    change_album(reload_mode, 0, from_photos, 0);
    g_albums_state = VIEW_ON;
}

/* Has nothing to release. */
close_albums()
{
}

/* Steps the album view in: VIEW_ENTER hides the radio buttons, VIEW_SHOW reloads the album. */
enter_albums()
{
    switch (g_albums_state)
    {
        case VIEW_ENTER:
        {
            if (g_mode_buttons)
            {
                SendMessage(g_mode_buttons, TAG("FADE"), 0, 0, 0, 0);
            }
            dim_buttons();
            g_albums_state = VIEW_HIDDEN;
        }
        case VIEW_SHOW:
        {
            set_caption_pos(0.546875 * g_screen_w, 0.5 * g_screen_h);
            change_album(g_album_mode, 0, 1, 0);
            g_album_idle = 0;
            g_albums_state = VIEW_ON;
        }
    }
}

/* Steps the album view out: VIEW_HIDE fades it out, VIEW_CLOSE destroys the album. */
leave_albums()
{
    switch (g_albums_state)
    {
        case VIEW_HIDE:
        {
            SetSystemParam(SYS_PADHELP, 0);
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            if (IsValidObject(g_active))
            {
                SendMessage(g_active, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
            }
            if (IsValidObject(g_mode_buttons))
            {
                SendMessage(g_mode_buttons, TAG("FADE"), 0, 400.0, 0, 0);
            }
            g_albums_state = VIEW_HIDDEN;
        }
        case VIEW_CLOSE:
        {
            if (IsValidObject(g_album))
            {
                SendMessage(g_album, TAG("FADE"), 0, 400.0, 0, 0);
            }
            fade_box(0.0, 400.0, 1.0, 1.0, 1.0);
            wait_ms(400);
            if (IsValidObject(g_album))
            {
                Destroy(g_album);
                g_album = 0;
                g_album_db_state = DB_KEEP;
            }
            g_albums_state = VIEW_OFF;
        }
    }
}

/* A frame of the album view: its steps in and out, or its messages and the USB device. */
update_albums()
{
    if (g_albums_state == VIEW_ENTER || g_albums_state == VIEW_SHOW)
    {
        enter_albums();
        return;
    }
    if (g_albums_state == VIEW_HIDE || g_albums_state == VIEW_CLOSE)
    {
        leave_albums();
        return;
    }
    if (g_albums_state == VIEW_OFF || g_albums_state == VIEW_HIDDEN)
    {
        return;
    }
    reload_usb_album();
    new msg;
    new a;
    new b;
    new c;
    while (RecvMessage(msg, a, b, c))
    {
        if (handle_album_message(msg, a, b, c))
        {
        }
        else if (g_active == g_mode_buttons)
        {
            on_buttons_message(msg, a, b);
        }
        else if (g_active == g_album)
        {
            on_album_message(msg, a, b, c);
        }
    }
    check_usb_albums();
    update_buttons_pad_help();
    update_album_info();
}

/* Reloads the album after a change of the USB device. */
reload_usb_album()
{
    if (g_usb_changed)
    {
        new reload_mode = g_album_mode;
        g_album_mode--; /* differs from the mode, so that change_album reloads the album */
        change_album(reload_mode, 0, 1, 0);
        g_usb_changed = 0;
    }
}

/*
 * The messages of both parts of the album view: MSG_NAVIMENU_OPENED (on the USB device), "ITEM" and "PUSH"; returns 1
 * if handled.
 */
handle_album_message(msg, a, b, c)
{
    if (msg == MSG_NAVIMENU_OPENED && g_album_mode == ALBUM_USB)
    {
        while (RecvMessage(msg))
        {
        }
        g_usb_changed = 1;
        return 1;
    }
    if (msg == TAG("ITEM"))
    {
        if (send_album_item(a, b, c))
        {
            return 1;
        }
    }
    else if (msg == TAG("PUSH"))
    {
        if (on_push(b))
        {
            return 1;
        }
    }
    return 0;
}

/* Creates the album item of index under parent and sends it to the album menu ("ITEM"). */
send_album_item(menu, parent, index)
{
    new item = create_album_item(parent, index);
    if (item)
    {
        SendMessage(menu, TAG("ITEM"), item, parent, 0, 0);
    }
    return 1;
}

/* Does nothing. */
on_push(button)
{
    button = button; /* redundant as written */
}

/* The messages of the radio buttons: "CHNG" (button) shows its album, triangle opens the device menu, cross leaves. */
on_buttons_message(msg, a, b)
{
    if (msg == TAG("FOCS"))
    {
        SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
    }
    else if (msg == TAG("CHNG"))
    {
        change_album(get_button_mode(a, g_start_mode), 1, 1, 0);
    }
    else if (msg == TAG("PUSH"))
    {
        if (b == TAG("RUP"))
        {
            open_device_menu(a);
        }
        else if (b == TAG("RDWN"))
        {
            go_back();
        }
        else
        {
            SendMessage(g_active, TAG("PUSH"), 0, 0, 0, 0);
        }
    }
}

/*
 * Shows album mode, loading it if it is another one or if its rows are not selected; with enter the focus goes into
 * it.
 */
change_album(mode, with_se, enter, se_if_found)
{
    if (with_se)
    {
        sndSePlay(SE_DECIDE);
    }
    send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    SendMessage(g_mode_buttons, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
    dim_buttons();
    SendMessage(g_mode_buttons, TAG("CHNG"), 0, 0, 0, 0);
    SetPubVar(g_mode_buttons, "disable_selected", 1);
    if (g_album_mode != mode)
    {
        g_album_db_state = DB_CLOSED;
        g_album_mode = mode;
    }
    if (g_album_db_state >= DB_KEEP)
    {
        show_device_image(g_album_device[g_album_mode]);
        g_album_count = load_album(g_album_mode);
        if (enter)
        {
            g_enter_album = 1;
        }
        if (se_if_found && g_album_count)
        {
            sndSePlay(SE_DECIDE);
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
    if (g_album_count <= 0)
    {
        g_album_count = 0;
        g_active = g_mode_buttons;
        SendMessage(g_mode_buttons, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        light_buttons();
        g_enter_album = 0;
    }
    else if (g_album_count > 0 && !enter)
    {
        if (g_active)
        {
            SendMessage(g_active, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
        }
        unfocus_box();
        dim_album();
        g_enter_album = 0;
        sleep;
        sleep;
        g_active = g_mode_buttons;
        light_buttons();
        SendMessage(g_mode_buttons, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
    }
    printf("album mode has changed ! new itemnum = %d\n", g_album_count);
    SetPubVar(g_mode_buttons, "disable_selected", 0);
}

/* Leaves for the top page, on the channel of the albums the page was opened on. */
go_back()
{
    sndSePlay(SE_CANCEL);
    g_next_link = "top";
    if (g_start_mode == ALBUM_FAVORITE)
    {
        g_next_args = "menu_id = CHANNEL-PHOTO ; channel_id = BNCH-PHOTO-FAVORITE ;";
    }
    else if (ALBUM_LIBRARY == g_start_mode)
    {
        g_next_args = "menu_id = CHANNEL-PHOTO ; channel_id = BNCH-PHOTO-COLLECTION ;";
    }
    g_page_mode = PAGE_QUIT;
}

/* Opens the option menu of the device of the radio buttons, on the library only. */
open_device_menu(index)
{
    if (ALBUM_LIBRARY != g_start_mode)
    {
        SendMessage(g_active, TAG("PUSH"), 0, 0, 0, 0);
        return;
    }
    if (g_album_count <= 0)
    {
        fade_caption(0.0, 400.0, 0.0);
    }
    run_option_menu(index, MENU_SIDE_RIGHT, -1);
    if (g_album_count <= 0)
    {
        fade_caption(1.0, 400.0, 0.0);
    }
}

/* Has nothing to do: the radio buttons set their own pad help. */
update_buttons_pad_help()
{
}

/* The messages of the album: circle opens the album, triangle its option menu, cross gives the pad back. */
on_album_message(msg, a, b, c)
{
    if (msg == TAG("PUSH"))
    {
        if (b == TAG("RRGT"))
        {
            sndSePlay(SE_DECIDE);
            enter_album(g_album_mode, a, c, g_album);
            g_page_mode = PAGE_TO_PHOTOS;
            return;
        }
        if (b == TAG("RUP"))
        {
            open_album_menu(a);
        }
        else if (b == TAG("RDWN"))
        {
            leave_album();
        }
        else
        {
            SendMessage(g_active, TAG("PUSH"), 0, 0, 0, 0);
        }
    }
    else if (msg == TAG("FOCS"))
    {
        SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
    }
}

/* Gives the pad back from the album to the radio buttons. */
leave_album()
{
    sndSePlay(SE_CANCEL);
    SendMessage(g_album, TAG("PUSH"), 0, 0, 0, 0);
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

open_album_menu(index)
{
    run_option_menu(index, MENU_SIDE_RIGHT, g_album_mode);
}

/*
 * Runs the option menu of album index of mode (mode -1: of the device), sliding the box and the camera aside; reloads
 * the album if the menu changed it.
 */
run_option_menu(index, side, mode)
{
    sndSePlay(SE_OPEN);
    sndSePlay(SE_WINDOW_OPEN);
    SetSystemParam(SYS_PADHELP, 0);
    SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
    SendMessage(g_active, TAG("PUSH"), 0, 0, 0, 0);
    SendMessage(g_active, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
    send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    if (side == MENU_SIDE_RIGHT)
    {
        if (!g_album_idle && mode >= 0)
        {
            new name[128];
            get_photo_field(index, FIELD_ALBUM_NAME, name, g_album_mode, g_db_server, 128);
            set_box_header(0, name);
            light_album();
        }
        SetPos(g_focus_box, 0.7625 * g_screen_w, 0.5 * g_screen_h, 0.0, 0);
        AnimMoveHermite(g_focus_box, 0.7625 * g_screen_w - 300.0, 0.5 * g_screen_h, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT,
                1.0);
        fade_box_footer(0.0, 400.0, 0.7843, 0.7843, 0.7843);
    }
    else
    {
        fade_box(0.0, 400.0, 1.0, 1.0, 1.0);
    }
    if (g_active == g_album)
    {
        unfocus_box();
    }
    SendMessage(g_mode_buttons, TAG("FADE"), 0, 400.0, 0, 0);
    slide_camera(side);
    new result;
    if (side == MENU_SIDE_RIGHT)
    {
        new group[64];
        if (mode < 0)
        {
            copy_string(group, "DEVICE_OPTION", 64);
        }
        else
        {
            new value[128];
            get_photo_field(index, FIELD_ALBUM_NAME, value, g_album_mode, g_db_server, 128);
            avalSet("PhotoAccessAlbumName", value);
            get_photo_field(index, FIELD_ALBUM_URI, value, g_album_mode, g_db_server, 128);
            avalSet("PhotoAccessAlbum", value);
            printf("set album_id %s\n", value);
            if (ALBUM_LIBRARY == mode)
            {
                copy_string(group, "ALBUM_OPTION", 64);
            }
            else if (mode == ALBUM_FAVORITE)
            {
                copy_string(group, "ALBUM_FAVOPT", 64);
            }
            else if (mode == ALBUM_USB)
            {
                copy_string(group, "ALBUM_USBOPT", 64);
            }
        }
        select = g_album_select;
        result = pop_std_menu("optmenu", group, side, 0);
        if (result == MENU_RENAMED)
        {
            new name[64];
            avalGetValue("InfoAlbumname", 0, name, 64);
            avalDelete("InfoTitlename");
            avalDelete("InfoAlbumname");
            set_box_header(0, name);
            wait_ms(100);
        }
    }
    else
    {
    }
    if (result == MENU_CLOSED)
    {
        printf("menu err finish !\n");
    }
    sndSePlay(SE_WINDOW_CLOSE);
    SetSystemParam(SYS_PADHELP, 0);
    if (side == MENU_SIDE_RIGHT)
    {
        SetPos(g_focus_box, 0.7625 * g_screen_w - 300.0, 0.5 * g_screen_h, 0.0, 0);
        AnimMoveHermite(g_focus_box, 0.7625 * g_screen_w, 0.5 * g_screen_h, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
    }
    center_camera();
    if (mode < 0)
    {
        dim_album();
        light_buttons();
    }
    else
    {
        light_album();
        dim_buttons();
        if (g_active == g_album)
        {
            focus_box_slot(0);
        }
    }
    SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
    if (result == MENU_CHANGED || result == MENU_ALBUMS_CHANGED || result == MENU_RENAMED)
    {
        g_album_db_state = DB_OPEN;
        new reload_mode = g_album_mode;
        g_album_mode--; /* differs from the mode, so that change_album reloads the album */
        change_album(reload_mode, 0, 1, 0);
    }
    SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
}

update_album_info()
{
    if (g_album_count <= 0 || !g_album)
    {
        return;
    }
    new idle = GetPubVar(g_album, "idle");
    g_album_select = GetPubVar(g_album, "select");
    if (g_album_idle != idle)
    {
        g_album_idle = idle;
        if (g_album_idle)
        {
            new name[128];
            get_photo_field(g_album_select, FIELD_ALBUM_NAME, name, g_album_mode, g_db_server, 128);
            new counter[64];
            sprintf(counter, "%d/%d", g_album_select + 1, g_album_count);
            set_box_header(0, name);
            set_box_footer(counter);
            if (!g_enter_album)
            {
                dim_album();
            }
            else
            {
                g_active = g_album;
                focus_box_slot(0);
                light_album();
                SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
            }
        }
        else
        {
            fade_box(0.0, 200.0, 1.0, 1.0, 1.0);
            if (g_active == g_album)
            {
                unfocus_box();
            }
        }
    }
}

/* On the USB album, reloads it when the USB device changes or becomes invalid. */
check_usb_albums()
{
    if (ALBUM_USB != g_album_mode && ALBUM_DISC != g_album_mode)
    {
        return;
    }
    get_wnd_var_int("usb_found", g_usb_found);
    get_wnd_var_int("usb_invalid", g_usb_invalid);
    if (0 == g_usb_invalid)
    {
        return;
    }
    printf("USB stat has changed !! reset albumlist!\n");
    if (g_usb_invalid > 0)
    {
        if (g_active == g_album)
        {
            leave_album();
        }
        g_album_db_state = DB_OPEN;
        g_album_mode = ALBUM_LIBRARY;
        change_album(ALBUM_USB, 0, 0, 0);
    }
    else
    {
        g_album_db_state = DB_OPEN;
        g_album_mode = ALBUM_LIBRARY;
        change_album(ALBUM_USB, 0, 1, 1);
    }
}

/* Takes the pad from album and makes album index PhotoAccessAlbum, for the photo view. */
enter_album(mode, index, item, album)
{
    mode = mode; /* redundant as written */
    if (IsValidObject(album) && IsValidObject(item))
    {
        SendMessage(album, TAG("PUSH"), 0, 0, 0, 0);
        SendMessage(album, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        unfocus_box();
        dim_album();
        sleep;
        sleep;
        new value[128];
        get_photo_field(index, FIELD_ALBUM_NAME, value, g_album_mode, g_db_server, 128);
        avalSet("PhotoAccessAlbumName", value);
        get_photo_field(index, FIELD_ALBUM_URI, value, g_album_mode, g_db_server, 128);
        avalSet("PhotoAccessAlbum", value);
    }
}

create_mode_buttons()
{
    new args[256];
    new Float:y;
    new buttons = CreateNull(Disp2D(0));
    if (ALBUM_FAVORITE != g_start_mode)
    {
        new library[64];
        new camera[64];
        new count = 2;
        get_tag_attr("TEXT", "msgphoto_menu_library", "value", library, 64);
        get_tag_attr("TEXT", "msgphoto_menu_camera", "value", camera, 64);
        y = 0.5 - 0.08 * float(count - 1) / 2.0;
        sprintf(args, "btn0 = %s ; btn1 = %s ; interval = %d", library, camera, floatround(0.08 * g_screen_h, 0));
    }
    else
    {
        new favorite[64];
        new count = 1;
        get_tag_attr("TEXT", "msgphoto_menu_favorite", "value", favorite, 64);
        y = 0.5 - 0.08 * float(count - 1) / 2.0;
        sprintf(args, "btn0 = %s ; interval = %d", favorite, floatround(0.08 * g_screen_h, 0));
    }
    AttachScript(buttons, _, "radio_button_v", args);
    SetPos(buttons, 0.14 * g_screen_w, g_screen_h * y, 0.0, 0);
    SetColor(buttons, 0.215, 0.708, 0.999, 0.0);
    SetOptionI(buttons, TAG("ZSRT"), ZSORT_OVERLAY);
    SetPubVar(buttons, "manager", Self());
    SetPubVar(buttons, "focus", focus_glowfly);
    SetPubVar(buttons, "padhelp", PADHELP_CIRCLE | PADHELP_CROSS | PADHELP_TRIANGLE);
    SetPubVar(buttons, "disable_selected", 0);
    SetPubVar(buttons, "se_decide", -1);
    if (g_start_mode == ALBUM_FAVORITE)
    {
        SendMessage(buttons, TAG("MASK"), 0, 3, 0, 0);
    }
    SendMessage(buttons, TAG("MASK"), 1, 3, 0, 0);
    sleep;
    SendMessage(buttons, TAG("FADE"), 0, 0, 0, 0);
    return buttons;
}

/* Creates the album item of album index: the default icon, then the cover of the album. */
create_album_item(parent, index)
{
    if (!IsValidObject(parent))
    {
        return 0;
    }
    new item = CreatePlane(parent);
    AttachScript(item, "album_item", "album_item");
    if (ALBUM_LIBRARY == g_album_mode || g_album_mode == ALBUM_FAVORITE)
    {
        SetPubVar(item, "pad_enable", PAD_TRIANGLE | PAD_CIRCLE);
    }
    else if (g_album_mode == ALBUM_USB || g_album_mode == ALBUM_DISC)
    {
        SetPubVar(item, "pad_enable", PAD_CIRCLE);
    }
    SetOptionI(item, TAG("FDEG"), 1);
    SetOptionF(item, TAG("ZOFS"), -50.0);
    SetColor(item, 1.0, 1.0, 1.0, 0.0);
    if (index >= 0 && index < g_album_count)
    {
        SendMessage(item, TAG("TEX"), g_default_icon, 1.0, 0, 0);
        new uri[256];
        get_photo_field(index, FIELD_URL, uri, g_album_mode, g_db_server, 256);
        SendMessageStr(item, TAG("DCDE"), uri);
    }
    return item;
}

/* Replaces the album with the one of mode; returns its number of items, showing why when there are none. */
load_album(mode)
{
    if (ALBUM_LIBRARY == mode)
    {
        avalSet("ALBUM_MODE", "LIBRARY");
    }
    else if (mode == ALBUM_FAVORITE)
    {
        avalSet("ALBUM_MODE", "FAVORITE");
    }
    else if (mode == ALBUM_USB)
    {
        avalSet("ALBUM_MODE", "USB");
    }
    else if (mode == ALBUM_DISC)
    {
        avalSet("ALBUM_MODE", "DISC");
    }
    else
    {
        return -2;
    }
    SetSystemParam(SYS_PADHELP, 0);
    if (g_active == g_album)
    {
        unfocus_box();
    }
    AnimTimerManager(Self(), 600.0, 0.0, ANIM_FUNC_LINEAR);
    fade_box(0.0, 600.0, 1.0, 1.0, 1.0);
    if (g_album)
    {
        SendMessage(g_album, TAG("FADE"), 0, 800.0, 0, 0);
    }
    if (g_album)
    {
        SendMessage(g_album, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
    }
    {
        new text[64];
        if (ALBUM_USB != mode)
        {
            get_tag_attr("TEXT", "msgphoto_pict_finding", "value", text, 64);
        }
        else
        {
            get_tag_attr("TEXT", "msgphoto_pict_connecting", "value", text, 64);
        }
        show_caption(text, _, 1.0, 400.0, 0.0);
        wait_ms(floatround(400.0, 0));
    }
    if (g_first_load)
    {
        init_photo_db();
        g_first_load = 0;
    }
    new count;
    if (g_album_db_state == DB_CLOSED)
    {
        count = open_album_db(mode, g_db_server);
    }
    else if (g_album_db_state == DB_OPEN)
    {
        count = select_album_rows(mode, g_db_server);
    }
    else if (g_album_db_state == DB_KEEP)
    {
        count = g_album_count;
        g_album_db_state = DB_READY;
    }
    if (mode == ALBUM_USB)
    {
        get_wnd_var_int("usb_found", g_usb_found);
        get_wnd_var_int("usb_invalid", g_usb_invalid);
    }
    if (ALBUM_LIBRARY == mode)
    {
        set_wnd_var_int("PhotoLibraryNum", count);
    }
    printf("new db opened %d\n", count);
    while (ANIM_DONE != AnimGetState(Self(), ANIM_TIMER))
    {
        sleep;
    }
    if (g_album)
    {
        Destroy(g_album);
    }
    g_album = 0;
    set_box_header(0, "");
    set_box_footer("");
    if (count > 0)
    {
        if ((mode == ALBUM_USB || mode == ALBUM_DISC) && count > 1)
        {
            count = 1;
        }
        if (g_album_select >= count)
        {
            g_album_select = count - 1;
        }
        g_album = create_album(count, g_album_select);
        show_caption(_, _, 0.0, 400.0, 0.0);
    }
    else if (count <= 0)
    {
        new line1[64] = {0};
        new line2[64] = {0};
        if (ALBUM_LIBRARY == mode || mode == ALBUM_FAVORITE)
        {
            get_tag_attr("TEXT", "msgphoto_pict_none", "value", line1, 64);
        }
        else if (mode == ALBUM_USB)
        {
            if (g_usb_found && 0 == g_usb_invalid)
            {
                get_tag_attr("TEXT", "msgphoto_pict_none", "value", line1, 64);
            }
            else
            {
                get_tag_attr("TEXT", "msgphoto_usb_nodevice", "value", line1, 64);
                get_tag_attr("TEXT", "msgphoto_usb_nodevice2", "value", line2, 64);
            }
        }
        show_caption(line1, line2, 1.0, 400.0, 0.0);
        count = 0;
    }
    SendMessage(g_mode_buttons, TAG("DSEL"), count <= 0, 0, 0, 0);
    return count;
}

/* Creates the album: a general_menu of count items on the curve of get_node_x/y/z, item selection in front. */
create_album(count, selection)
{
    new album;
    album = CreateNull(Self());
    {
        new Float:x;
        new Float:y;
        new Float:z;
        map_screen_point(x, y, z, 0.7625 * g_screen_w, 0.5 * g_screen_h, Self());
        SetPos(album, x, y, z, 0);
        AttachScript(album, "general_menu", "general_menu");
        SetPubVar(album, "manager", Self());
        SetPubVar(album, "focus", focus_glowfly);
        SetPubVar(album, "pad_enable", PAD_CROSS);
        SendMessage(album, TAG("INUM"), count, 0, 0, 0);
        SendMessage(album, TAG("CLRW"), 1, 1, 0, 0);
        SendMessage(album, TAG("PADL"), 0, 0, 1, -1);
        SendMessage(album, TAG("MGIN"), 1, 7, 0, 0);
        SendMessage(album, TAG("MODE"), MENU_TYPE_PAGE, SCROLL_ALONG_CURVE, 0, 0);
        SendMessage(album, TAG("ANIM"), 250.0, 0, ANIM_FUNC_EXPONENT, 0);
        SendMessage(album, TAG("REPT"), 50.01, 83.34, 0, 0);
        SendMessage(album, TAG("SLCT"), selection, 0, 0, 0);
        for (new node; node < 9; node++)
        {
            SendMessage(album, TAG("NPOS"), node, get_node_x(node - 1), get_node_y(node - 1), get_node_z(node - 1));
        }
    }
    return album;
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
    fade_box_footer(1.0, 400.0, 0.7843, 0.7843, 0.7843);
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
    fade_box_footer(1.0, 400.0, 0.7843, 0.7843, 0.7843);
    if (!g_album)
    {
        return;
    }
    SendMessage(g_album, TAG("FADE"), 0.25, 400.0, 0, 0);
}

/* Decodes the image of IMG id and keeps its texture; returns it, 0 on failure. */
load_icon_texture(id[])
{
    new url[256];
    get_tag_attr("IMG", id, "src", url, 256);
    start_image_decode(url);
    if (IMG_DONE != finish_image_decode())
    {
        return 0;
    }
    new texture = imgTexID();
    imgKeepTex(texture);
    return texture;
}

new g_photos_usb_invalid;
new g_photos_usb_found;
new g_photo_count;
new g_photos_active; /* g_grid or g_toolbar: the one that takes the pad */
new g_grid;
new g_toolbar; /* the import button of the USB photos */
new g_preview; /* the large photo under the cursor */
new g_grid_idle;
new g_preview_on; /* the cursor has stayed: the preview shows its photo */
new g_slideshow_url[64];
new g_photo_mode = -1; /* PHOTOS_*: the photos shown */
new g_photo_select = -1; /* the photo under the cursor */
new g_photos_state = VIEW_OFF; /* VIEW_* of the photo view */
new g_photos_pad_help; /* the PADHELP_* shown; -1 while an option runs */
new Float:g_grid_x; /* the rest position of the grid */
new Float:g_grid_y;
new Float:g_grid_z;

/* Builds what the photo view keeps: the large photo under the cursor and the rest position of the grid. */
init_photos()
{
    g_preview_on = 0;
    g_preview = create_preview();
    map_screen_point(g_grid_x, g_grid_y, g_grid_z, 0.25 * g_screen_w, 0.5 * g_screen_h, Self());
    g_grid_z = g_grid_z - 50.0;
}

/* Fades out and destroys the grid and the toolbar. */
close_photos()
{
    if (g_grid)
    {
        SendMessage(g_grid, TAG("FADE"), 0, 400.0, 0, 0);
    }
    if (g_grid)
    {
        SendMessage(g_grid, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
    }
    fade_toolbar(0.0, 400.0);
    fade_box(0.0, 400.0, 1.0, 1.0, 1.0);
    if (g_grid)
    {
        Destroy(g_grid);
    }
    if (g_toolbar)
    {
        Destroy(g_toolbar);
    }
}

/* Steps the photo view in: VIEW_ENTER loads the photos of the album, VIEW_SHOW shows them. */
enter_photos()
{
    switch (g_photos_state)
    {
        case VIEW_ENTER:
        {
            g_photo_select = 0;
            g_photos_pad_help = 0;
            if (g_photo_mode != g_album_mode + 4)
            {
                g_photo_mode = g_album_mode + 4;
                g_thumb_db_state = DB_CLOSED;
            }
            else
            {
                g_thumb_db_state = DB_OPEN;
            }
            if (g_preview)
            {
                SendMessage(g_preview, TAG("WAKE"), 0, 400.0, 1, 0);
            }
            set_caption_pos(0.08125 * g_screen_w, 0.5 * g_screen_h);
            g_toolbar = create_toolbar();
            fade_toolbar(0.0, 20.0);
            g_photo_count = load_photos(g_photo_mode);
            if (g_photo_count > 0)
            {
                SendMessage(g_grid, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
                g_photos_active = g_grid;
                g_preview_on = 0;
            }
            else
            {
                SetSystemParam(SYS_PADHELP, PADHELP_CROSS);
            }
            g_photos_state = VIEW_HIDDEN;
        }
        case VIEW_SHOW:
        {
            fade_toolbar(1.0, 400.0);
            set_box_title("");
            set_box_footer("");
            fade_box(0.0, 400.0, 1.0, 1.0, 1.0);
            fade_box_header(1.0, 400.0, 1.0, 1.0, 1.0);
            fade_box_footer(1.0, 400.0, 0.7843, 0.7843, 0.7843);
            if (g_photo_count <= 0 && g_photo_mode == PHOTOS_USB)
            {
                g_page_mode = PAGE_TO_ALBUMS;
                g_album_db_state = DB_OPEN;
                g_thumb_db_state = DB_OPEN;
            }
            g_photos_state = VIEW_ON;
        }
    }
}

/* Steps the photo view out: VIEW_HIDE fades it out, VIEW_CLOSE destroys the grid and the toolbar. */
leave_photos()
{
    switch (g_photos_state)
    {
        case VIEW_HIDE:
        {
            if (g_grid)
            {
                SendMessage(g_grid, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
            }
            if (g_grid)
            {
                SendMessage(g_grid, TAG("FADE"), 0, 400.0, 0, 0);
            }
            if (g_preview)
            {
                SendMessage(g_preview, TAG("SLEP"), 0, 400.0, 0, 0);
            }
            fade_toolbar(0.0, 400.0);
            fade_box(0.0, 400.0, 1.0, 1.0, 1.0);
            show_caption(_, _, 0.0, 400.0, 0.0);
            g_photos_state = VIEW_HIDDEN;
        }
        case VIEW_CLOSE:
        {
            photoListClear();
            if (IsValidObject(g_grid))
            {
                Destroy(g_grid);
                g_grid = 0;
            }
            if (IsValidObject(g_toolbar))
            {
                Destroy(g_toolbar);
                g_toolbar = 0;
            }
            g_photos_state = VIEW_OFF;
        }
    }
}

/* A frame of the photo view: its steps in and out, or its messages, the USB device and the preview. */
update_photos()
{
    if (g_photos_state == VIEW_ENTER || g_photos_state == VIEW_SHOW)
    {
        enter_photos();
        return;
    }
    if (g_photos_state == VIEW_HIDE || g_photos_state == VIEW_CLOSE)
    {
        leave_photos();
        return;
    }
    if (g_photos_state == VIEW_OFF || g_photos_state == VIEW_HIDDEN)
    {
        return;
    }
    check_empty_back();
    new msg;
    new a;
    new b;
    new c;
    while (RecvMessage(msg, a, b, c))
    {
        if (handle_photos_message(msg, a, b))
        {
        }
        else if (g_photos_active == g_toolbar && g_toolbar)
        {
            on_toolbar_message(msg, a, b);
        }
        else if (g_photos_active == g_grid)
        {
            on_grid_message(msg, a, b, c);
        }
    }
    check_usb_photos();
    update_preview();
    update_photos_pad_help();
}

/*
 * Sets the pad help as the photo under the cursor is shown (circle, cross, triangle), broken (cross, triangle) or not
 * decoded yet (cross), and decstate the same way.
 */
update_photos_pad_help()
{
    if (g_photos_pad_help < 0)
    {
        return;
    }
    new old = g_photos_pad_help;
    new grid_state;
    new preview_state;
    if (g_grid)
    {
        grid_state = GetPubVar(g_grid, "decstate");
    }
    if (g_preview)
    {
        preview_state = GetPubVar(g_preview, "decstate");
    }
    if (DECSTATE_PENDING == grid_state || DECSTATE_PENDING == preview_state || !g_preview_on)
    {
        g_photos_pad_help = PADHELP_CROSS;
        decstate = DECSTATE_PENDING;
    }
    else if (grid_state < 0 || preview_state < 0)
    {
        g_photos_pad_help = PADHELP_CROSS | PADHELP_TRIANGLE;
        decstate = DECSTATE_BROKEN;
    }
    else if (DECSTATE_PENDING != grid_state && DECSTATE_PENDING != preview_state)
    {
        g_photos_pad_help = PADHELP_CIRCLE | PADHELP_CROSS | PADHELP_TRIANGLE;
        decstate = DECSTATE_SHOWN;
    }
    if (old != g_photos_pad_help)
    {
        SetSystemParam(SYS_PADHELP, g_photos_pad_help);
    }
}

/* With no photos, the cross goes back to the albums. */
check_empty_back()
{
    if (g_photo_count <= 0)
    {
        new held;
        new pressed;
        new released;
        new repeated;
        PadState(held, pressed, released, repeated, -1);
        if (pressed & PAD_CROSS)
        {
            sndSePlay(SE_CANCEL);
            g_page_mode = PAGE_TO_ALBUMS;
        }
    }
}

/*
 * Handles the messages of both parts of the photo view: MSG_NAVIMENU_OPENED (on the USB device) and cross; returns 1 if
 * done.
 */
handle_photos_message(msg, a, b)
{
    a = a; /* redundant as written */
    if (msg == MSG_NAVIMENU_OPENED && g_album_mode == ALBUM_USB)
    {
        while (RecvMessage(msg))
        {
        }
        g_usb_changed = 1;
        return 1;
    }
    if (msg == TAG("PUSH"))
    {
        if (b == TAG("RDWN"))
        {
            sndSePlay(SE_CANCEL);
            SendMessage(g_photos_active, TAG("PUSH"), 0, 0, 0, 0);
            g_page_mode = PAGE_TO_ALBUMS;
            return 1;
        }
    }
    return 0;
}

/* Goes back to the albums when the USB device of the photos becomes invalid. */
check_usb_photos()
{
    get_wnd_var_int("usb_found", g_photos_usb_found);
    get_wnd_var_int("usb_invalid", g_photos_usb_invalid);
    if (g_photos_usb_invalid && g_photo_mode == PHOTOS_USB)
    {
        g_page_mode = PAGE_TO_ALBUMS;
        g_album_db_state = DB_OPEN;
        g_thumb_db_state = DB_OPEN;
    }
}

/* The messages of the toolbar of the USB photos: left gives the pad back to the grid, circle imports the photos. */
on_toolbar_message(msg, a, b)
{
    if (msg == TAG("FOCS"))
    {
        if (a == TAG("LEFT"))
        {
            g_photos_pad_help = 0;
            g_photos_active = g_grid;
            SendMessage(g_photos_active, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
            AnimMoveLine(g_photos_active, g_grid_x, g_grid_y, g_grid_z, 200.0, 0.0, ANIM_FUNC_EXPONENT);
        }
        else
        {
            SendMessage(g_photos_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        }
    }
    if (msg == TAG("PUSH"))
    {
        if (b == TAG("RRGT"))
        {
            SendMessage(g_photos_active, TAG("PUSH"), 0, 0, 0, 0);
            SendMessage(g_photos_active, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
            SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
            SetPubVar(g_grid, "decwait", 1);
            wait_ms(400);
            new proc = CreateNull(Self());
            AttachScript(proc, "usb_checkin_process", "usb_process");
            SetPubVar(proc, "focus", focus_glowfly);
            while (SCRIPT_FINISHED != ScriptState(proc))
            {
                sleep;
            }
            Destroy(proc);
            SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
            SetPubVar(g_grid, "decwait", 0);
            SendMessage(g_photos_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        }
        else
        {
            SendMessage(g_photos_active, TAG("PUSH"), 0, 0, 0, 0);
        }
    }
}

/*
 * The messages of the grid: circle shows the photo in the slide show page (start: as a slide show), triangle opens
 * the option menu, right moves to the toolbar.
 */
on_grid_message(msg, a, b, c)
{
    if (msg == TAG("ITEM"))
    {
        new item = create_thumb_item(b, c);
        if (item)
        {
            SendMessage(a, TAG("ITEM"), item, b, 0, 0);
        }
    }
    else if (msg == TAG("PUSH"))
    {
        if (b == TAG("RRGT") || b == TAG("STRT"))
        {
            if (!(g_photos_pad_help & PADHELP_CIRCLE) || g_photos_pad_help < 0)
            {
                SendMessage(g_photos_active, TAG("PUSH"), 0, 0, 0, 0);
                return;
            }
            g_photos_pad_help = -1;
            SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
            SetSystemParam(SYS_PADHELP, 0);
            SetPubVar(g_grid, "decwait", 1);
            sndSePlay(SE_DECIDE);
            SendMessage(g_photos_active, TAG("PUSH"), 0, 0, 0, 0);
            SendMessage(g_photos_active, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
            new args[128];
            new device[32];
            avalGetValue("ALBUM_MODE", 0, device, 32);
            if (b == TAG("RRGT"))
            {
                sprintf(args, "mode = %s ; select = %d ; slideshow = off ;", device, a);
            }
            else if (b == TAG("STRT"))
            {
                sprintf(args, "mode = %s ; select = %d ; slideshow = on ;", device, a);
            }
            get_tag_attr("LINK", "slideshow", "href", g_slideshow_url, 64);
            new st_mode;
            new st_size;
            if (0 == fileLstat(g_slideshow_url, st_mode, st_size))
            {
                printf("load xml %s, %s\n", g_slideshow_url, args);
                xmlLoadStart(g_slideshow_url, _, args, XMLLOAD_MODAL);
                wait_xml_load();
            }
            new index;
            if (FindMessage(TAG("RTRN"), index))
            {
                printf("finish slideshow %d!!!!!!!!!!\n", index);
            }
            SendMessage(g_photos_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
            SendMessage(g_photos_active, TAG("CURS"), index, 0, 0, 0);
            SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
            SetPubVar(g_grid, "decwait", 0);
            g_photos_pad_help = 0;
        }
        else if (b == TAG("RUP"))
        {
            if (!(g_photos_pad_help & PADHELP_TRIANGLE) || g_photos_pad_help < 0)
            {
                SendMessage(g_photos_active, TAG("PUSH"), 0, 0, 0, 0);
                return;
            }
            if (g_preview)
            {
                SendMessage(g_preview, TAG("SLEP"), 0, 400.0, 1, 0);
            }
            g_photos_pad_help = -1;
            SendMessage(g_photos_active, TAG("PUSH"), 0, 0, 0, 0);
            SendMessage(g_photos_active, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
            SetSystemParam(SYS_PADHELP, 0);
            SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
            SetPubVar(g_photos_active, "decwait", 1);
            sndSePlay(SE_OPEN);
            sndSePlay(SE_WINDOW_OPEN);
            SetPos(g_focus_box, 0.7625 * g_screen_w, 0.5 * g_screen_h, 0.0, 0);
            AnimMoveHermite(g_focus_box, 0.7625 * g_screen_w - 300.0, 0.5 * g_screen_h, 0.0, 750.0, 0.0,
                    ANIM_FUNC_EXPONENT, 1.0);
            fade_box_footer(0.0, 400.0, 0.7843, 0.7843, 0.7843);
            if (g_grid)
            {
                SendMessage(g_grid, TAG("FADE"), 0, 400.0, 0, 0);
            }
            fade_toolbar(0.0, 400.0);
            slide_camera(1);
            new group[64];
            if (g_photo_mode == PHOTOS_LIBRARY)
            {
                sprintf(group, "THUMB_OPTION");
            }
            else if (g_photo_mode == PHOTOS_FAVORITE)
            {
                sprintf(group, "THUMB_FAVOPT");
            }
            else if (g_photo_mode == PHOTOS_USB)
            {
                sprintf(group, "THUMB_USBOPT");
            }
            select = g_photo_select;
            new result = pop_std_menu("menu_title", group, MENU_SIDE_RIGHT, 0);
            if (result == MENU_RENAMED)
            {
                new name[64];
                avalGetValue("InfoTitlename", 0, name, 64);
                avalDelete("InfoTitlename");
                avalDelete("InfoAlbumname");
                set_box_header(0, name);
                wait_ms(100);
            }
            sndSePlay(SE_WINDOW_CLOSE);
            SetSystemParam(SYS_PADHELP, 0);
            SetPos(g_focus_box, 0.7625 * g_screen_w - 300.0, 0.5 * g_screen_h, 0.0, 0);
            AnimMoveHermite(g_focus_box, 0.7625 * g_screen_w, 0.5 * g_screen_h, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT,
                    1.0);
            fade_box_footer(1.0, 400.0, 0.7843, 0.7843, 0.7843);
            if (g_grid)
            {
                SendMessage(g_grid, TAG("FADE"), 1.0, 400.0, 0, 0);
            }
            center_camera();
            if (g_preview)
            {
                SendMessage(g_preview, TAG("WAKE"), 1.0, 400.0, 0, 0);
            }
            apply_photos_result(result, g_photo_mode);
            if (g_photo_count > 0)
            {
                fade_box(1.0, 400.0, 1.0, 1.0, 1.0);
                fade_toolbar(1.0, 400.0);
            }
            if (g_grid)
            {
                SendMessage(g_grid, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
            }
            SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
            if (g_grid)
            {
                SetPubVar(g_grid, "decwait", 0);
            }
            g_photos_pad_help = 0;
        }
        else
        {
            SendMessage(g_photos_active, TAG("PUSH"), 0, 0, 0, 0);
        }
    }
    else if (msg == TAG("FOCS"))
    {
        if (a == TAG("RIGT"))
        {
            if (g_toolbar)
            {
                g_photos_pad_help = -1;
                g_photos_active = g_toolbar;
                SendMessage(g_photos_active, TAG("FOCS"), TAG("LEFT"), 0, 0, 0);
                new Float:x;
                new Float:y;
                new Float:z;
                GetOffPos(g_grid, x, y, z, 0.0, 0.0, 50.0, Self());
                AnimMoveLine(g_grid, x, y, z, 200.0, 0.0, ANIM_FUNC_EXPONENT);
            }
            else
            {
                SendMessage(g_photos_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
            }
        }
        else
        {
            SendMessage(g_photos_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        }
    }
}

/* Shows the photo under the cursor large, with its title and number, once the cursor stays on it. */
update_preview()
{
    if (g_photo_count < 0 || !IsValidObject(g_grid))
    {
        return;
    }
    new old_select = g_photo_select;
    new was_on = g_preview_on;
    g_grid_idle = GetPubVar(g_grid, "idle");
    g_photo_select = GetPubVar(g_grid, "cursor");
    if (old_select != g_photo_select)
    {
        g_preview_on = 0;
    }
    else if (g_grid_idle == 1)
    {
        g_preview_on = 1;
    }
    if (!g_preview_on)
    {
        SendMessage(g_preview, TAG("BUSY"), 0, 0, 0, 0);
    }
    if (g_preview_on != was_on)
    {
        if (g_preview_on)
        {
            new uri[128];
            get_photo_field(g_photo_select, FIELD_URL, uri, g_photo_mode, 0, 128);
            SendMessageStr(g_preview, TAG("DCDE"), uri);
            new title[128];
            get_photo_field(g_photo_select, FIELD_TITLE, title, g_photo_mode, 0, 128);
            if (g_photo_mode == PHOTOS_FAVORITE)
            {
                new album_name[64];
                get_photo_field(g_photo_select, FIELD_ALBUM_NAME, album_name, g_photo_mode, 0, 64);
                append_string(title, " / ", 128);
                append_string(title, album_name, 128);
            }
            new counter[128];
            sprintf(counter, "%d/%d", g_photo_select + 1, g_photo_count);
            set_box_header(0, title);
            printf("title %s\n", title);
            set_box_footer(counter);
            fade_box(1.0, 1000.0, 1.0, 1.0, 1.0);
        }
        else
        {
            fade_box(0.0, 200.0, 1.0, 1.0, 1.0);
            SendMessage(g_preview, TAG("FADE"), 0, 200.0, 0, 0);
        }
    }
}

/* Creates the large photo under the cursor (photo_focused_main of photo_parts). */
create_preview()
{
    new preview;
    preview = CreatePlane(Scene(0));
    {
        new Float:x;
        new Float:y;
        new Float:z;
        map_screen_point(x, y, z, 0.7625 * g_screen_w, 0.5 * g_screen_h, Scene(0));
        SetPos(preview, x, y, z, 0);
        SetColor(preview, 1.0, 1.0, 1.0, 0.0);
        AttachScript(preview, "photo_focused_main", "photo_parts");
    }
    return preview;
}

new g_toolbar_label;

/* Fades the toolbar and its label to target_a. */
fade_toolbar(Float:target_a, Float:duration)
{
    if (!g_toolbar)
    {
        return;
    }
    SendMessage(g_toolbar, TAG("FADE"), target_a, duration, 0, 0);
    AnimColor(g_toolbar_label, 0.7843, 0.7843, 0.7843, target_a, duration, 0.0, ANIM_FUNC_LINEAR);
}

/* For the USB photos, creates the toolbar with the button that imports them and its label; returns it, else 0. */
create_toolbar()
{
    if (PHOTOS_USB != g_photo_mode)
    {
        return 0;
    }
    new toolbar;
    toolbar = CreateNull(Disp2D(0));
    {
        new image[128];
        new args[256];
        get_tag_attr("IMG", "rec", "src", image, 128);
        SetPos(toolbar, 0.6 * g_screen_w, 0.8 * g_screen_h, 0.0, 0);
        SetColor(toolbar, 1.0, 1.0, 1.0, 0.0);
        sprintf(args, "btn0=%s", image);
        AttachScript(toolbar, _, "toolbar", args);
        SetPubVar(toolbar, "manager", Self());
        SetPubVar(toolbar, "focus", focus_glowfly);
        SetPubVar(toolbar, "alpha_blend_mode", BLEND_ADD);
        SetPubVar(toolbar, "padhelp", PADHELP_CROSS);
        SetPubVar(toolbar, "expand", 0);
    }
    new label;
    {
        new text[32];
        get_tag_attr("TEXT", "msgphoto_optmenu_pict_export", "value", text, 32);
        label = CreateLabel(Disp2D(0));
        SetPos(label, g_screen_w * (0.6 + 0.03), 0.8 * g_screen_h, 0.0, 0);
        SetColor(label, 0.7843, 0.7843, 0.7843, 0.0);
        SetOptionI(label, TAG("FONT"), FontID(0));
        SetOptionI(label, TAG("HALN"), ALIGN_LEFT);
        SetOptionI(label, TAG("VALN"), ALIGN_CENTER);
        SetOptionI(label, TAG("FSIZ"), 15);
        SetOptionS(label, TAG("UTF8"), text);
        AnimColor(label, 0.7843, 0.7843, 0.7843, 1.0, 600.0, 0.0, ANIM_FUNC_LINEAR);
    }
    g_toolbar_label = label;
    return toolbar;
}

/* Creates the thumbnail item of photo index under parent. */
create_thumb_item(parent, index)
{
    if (!IsValidObject(parent))
    {
        return 0;
    }
    new item = CreateNull(parent);
    AttachScript(item, "thumb_item", "thumb_item");
    SetPubVar(item, "pad_enable", PAD_TRIANGLE | PAD_CIRCLE);
    SetColor(item, 1.0, 1.0, 1.0, 0.0);
    if (index >= 0 && index < g_photo_count)
    {
        new uri[128];
        get_photo_field(index, FIELD_URL, uri, g_photo_mode, 0, 128);
        SendMessageStr(item, TAG("DCDE"), uri);
    }
    return item;
}

/*
 * Selects the photos of mode, fills the photo list and makes the grid; returns their number, showing why when there
 * are none. PhotoDataSize gets their size in kilobytes.
 */
load_photos(mode)
{
    SetSystemParam(SYS_PADHELP, 0);
    AnimTimerManager(Self(), 600.0, 0.0, ANIM_FUNC_LINEAR);
    fade_box(0.0, 600.0, 1.0, 1.0, 1.0);
    if (g_grid)
    {
        SendMessage(g_grid, TAG("FADE"), 0, 600.0, 0, 0);
    }
    if (g_grid)
    {
        SendMessage(g_grid, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
    }
    {
        new text[64];
        get_tag_attr("TEXT", "msgphoto_pict_finding", "value", text, 64);
        show_caption(text, _, 1.0, 400.0, 0.0);
        wait_ms(floatround(400.0, 0));
    }
    new count;
    query:
    if (g_thumb_db_state == DB_CLOSED)
    {
        count = open_album_db(mode, 0);
    }
    else if (g_thumb_db_state == DB_OPEN)
    {
        count = select_album_rows(mode, 0);
    }
    else if (g_thumb_db_state == DB_KEEP)
    {
        count = g_photo_count;
        g_thumb_db_state = DB_READY;
    }
    if (mode == PHOTOS_USB)
    {
        get_wnd_var_int("usb_found", g_photos_usb_found);
        get_wnd_var_int("usb_invalid", g_photos_usb_invalid);
        if (0 != g_photos_usb_invalid)
        {
            g_thumb_db_state = DB_OPEN;
            goto query;
        }
    }
    printf("new db opened %d\n", count);
    while (ANIM_DONE != AnimGetState(Self(), ANIM_TIMER))
    {
        sleep;
    }
    send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    if (g_grid)
    {
        Destroy(g_grid);
    }
    g_grid = 0;
    photoListClear();
    new size_kb;
    if (count > 0)
    {
        new uri[128];
        for (new i; i < count; i++)
        {
            get_photo_field(i, FIELD_URL, uri, mode, 0, 128);
            photoListAdd(uri);
            {
                new ret;
                new st_mode;
                new st_size;
                ret = fileLstat(uri, st_mode, st_size);
                if (ret != g_not_found)
                {
                    size_kb = size_kb + (st_size + 8191) / 8192 * 8;
                }
            }
            if (0 == (i + 1) % 3)
            {
                sleep;
            }
        }
        if (g_photo_select >= count)
        {
            g_photo_select = count - 1;
        }
        g_grid = create_grid(count, g_photo_select);
        if (g_grid)
        {
            SendMessage(g_grid, TAG("FADE"), 1.0, 400.0, 0, 0);
        }
        set_box_header(0, "");
        set_box_footer("");
        fade_box(1.0, 400.0, 1.0, 1.0, 1.0);
        show_caption(_, _, 0.0, 400.0, 0.0);
    }
    else if (count <= 0)
    {
        new line1[64] = {0};
        new line2[64] = {0};
        if (mode == PHOTOS_LIBRARY || mode == PHOTOS_FAVORITE)
        {
            get_tag_attr("TEXT", "msgphoto_pict_none", "value", line1, 64);
        }
        else if (mode == PHOTOS_USB)
        {
            if (g_photos_usb_found && 0 == g_photos_usb_invalid)
            {
                get_tag_attr("TEXT", "msgphoto_pict_none", "value", line1, 64);
            }
            else
            {
                get_tag_attr("TEXT", "msgphoto_usb_nodevice", "value", line1, 64);
                get_tag_attr("TEXT", "msgphoto_usb_nodevice2", "value", line2, 64);
            }
        }
        show_caption(line1, line2, 1.0, 400.0, 0.0);
        count = 0;
    }
    set_wnd_var_int("PhotoDataSize", size_kb);
    printf("thumb datasize %d Kbyte\n", size_kb);
    return count;
}

/* Creates the photo_grid of count photos, the cursor on selection, tilted on the left of the screen. */
create_grid(count, selection)
{
    new Float:screen_w;
    new Float:screen_h;
    get_screen_size(screen_w, screen_h);
    new grid = CreateNull(Self());
    AttachScript(grid, _, "photo_grid");
    SetPubVar(grid, "owner", Self());
    SetPubVar(grid, "focus", focus_glowfly);
    SetPubVar(grid, "size", count);
    SetPubVar(grid, "yview", 2);
    SendMessage(grid, TAG("CURS"), selection, 0, 0, 0);
    SetPubVar(grid, "padhelp", PADHELP_CIRCLE | PADHELP_CROSS | PADHELP_TRIANGLE);
    if (g_album_mode == ALBUM_USB)
    {
        SetPubVar(grid, "padexit", PAD_RIGHT);
    }
    else
    {
        SetPubVar(grid, "padexit", 0);
    }
    new Float:x;
    new Float:y;
    new Float:z;
    map_screen_point(x, y, z, 0.25 * screen_w, 0.5 * screen_h, Self());
    SetPos(grid, x, y, z - 50.0, 0);
    SetRot(grid, -0.23, 0.52, 0.0, 0, 0);
    return grid;
}

/* Reloads the photos of mode and marks the albums to reload, as the option result changed them. */
apply_photos_result(result, mode)
{
    if (result == MENU_CHANGED || result == MENU_PHOTOS_CHANGED || result == MENU_RENAMED)
    {
        g_thumb_db_state = DB_OPEN;
    }
    if (result == MENU_CHANGED || result == MENU_ALBUMS_CHANGED || result == MENU_RENAMED)
    {
        g_album_db_state = DB_OPEN;
    }
    if (result == MENU_CHANGED || result == MENU_PHOTOS_CHANGED || result == MENU_RENAMED)
    {
        g_photo_count = load_photos(mode);
        printf("refreshed item num = %d\n", g_photo_count);
        if (g_photo_count <= 0)
        {
            g_photos_active = 0;
            SendMessage(g_preview, TAG("SLEP"), 0, 400.0, 0, 0);
        }
        else if (g_photo_count > 0)
        {
            g_photos_active = g_grid;
            SendMessage(g_photos_active, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
            g_preview_on = 0;
        }
    }
}

new g_last_albums_state = VIEW_OFF; /* the states of the views at the previous step of a switch */
new g_last_photos_state = VIEW_OFF;

/* A step from the album view to the photo view. */
switch_to_photos()
{
    switch (g_albums_state)
    {
        case VIEW_OFF:
        {
            g_page_mode = PAGE_PHOTOS;
        }
        case VIEW_ON:
        {
            g_albums_state = VIEW_HIDE;
            g_photos_state = VIEW_ENTER;
        }
        case VIEW_HIDDEN:
        {
            if (g_photos_state == VIEW_HIDDEN && g_last_photos_state != g_photos_state)
            {
                wait_ms(floatround(400.0, 0));
                g_page_mode = PAGE_TO_PHOTOS;
            }
            else
            {
                g_albums_state = VIEW_CLOSE;
                g_photos_state = VIEW_SHOW;
            }
        }
    }
    g_last_albums_state = g_albums_state;
    g_last_photos_state = g_photos_state;
}

/* A step from the photo view back to the album view. */
switch_to_albums()
{
    switch (g_photos_state)
    {
        case VIEW_OFF:
        {
            g_page_mode = PAGE_ALBUMS;
            decstate = DECSTATE_BROKEN;
        }
        case VIEW_ON:
        {
            g_photos_state = VIEW_HIDE;
            g_albums_state = VIEW_ENTER;
        }
        case VIEW_HIDDEN:
        {
            if (g_albums_state == VIEW_HIDDEN && g_last_albums_state != g_albums_state)
            {
                wait_ms(floatround(400.0, 0));
                g_page_mode = PAGE_TO_ALBUMS;
            }
            else
            {
                g_photos_state = VIEW_CLOSE;
                g_albums_state = VIEW_SHOW;
            }
        }
    }
    g_last_albums_state = g_albums_state;
    g_last_photos_state = g_photos_state;
}

/* Has nothing to do. */
update_spare()
{
}

/* Runs the step of the switch g_page_mode asks for. */
update_page_mode()
{
    switch (g_page_mode)
    {
        case PAGE_TO_PHOTOS:
        {
            switch_to_photos();
        }
        case PAGE_TO_ALBUMS:
        {
            switch_to_albums();
        }
        case PAGE_SPARE:
        {
            update_spare();
        }
    }
}

/* Goes back from the photos of the USB device to the albums after the navigation menu opened. */
check_usb_message()
{
    if (FindMessage(MSG_NAVIMENU_OPENED) && g_album_mode == ALBUM_USB)
    {
        g_usb_changed = 1;
    }
    if (g_usb_changed && g_album_mode == ALBUM_USB)
    {
        if (g_page_mode == PAGE_PHOTOS)
        {
            g_page_mode = PAGE_TO_ALBUMS;
            g_album_db_state = DB_OPEN;
            g_thumb_db_state = DB_OPEN;
            g_usb_changed = 0;
        }
    }
}

main()
{
    init_page();
    init_albums();
    init_photos();
    sleep;
    while (PAGE_QUIT != g_page_mode)
    {
        check_usb_message();
        update_page_mode();
        update_albums();
        update_photos();
        sleep;
    }
    close_albums();
    close_photos();
    load_next_page();
}
