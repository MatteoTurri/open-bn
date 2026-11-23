/*
 * /opt0/bn/script/top_list.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The menus of the top page, one MENUGROUP element of the page at a time: an icon menu (icons on an arc that the
 * camera flies along) or a label menu (a list of labels with a crystal turning beside each). Pushing an item opens
 * its subgroup, a page or a command; the ISP channels of the top library are added to the group GROUP-ISPCH.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "top.inc"

/* Types of menu (the menutype of the MENUGROUP). */
const MENU_LABEL = 0;
const MENU_ICON = 1;

/* Orders of the icons on the arc (the order of the MENUGROUP). */
const ORDER_LEFT = 0;
const ORDER_RIGHT = 1;
const ORDER_CENTER = 2; /* the first item in the middle, the next ones alternately on either side */
const ORDER_CENTER_EVEN = 3; /* as ORDER_CENTER with an even count; get_order never returns it */
const ORDER_CENTER_ODD = 4;  /* as ORDER_CENTER with an odd count; get_order never returns it */

/* Results of the label menu (g_label_result); 3 is tested with LABEL_OPEN but never set. */
const LABEL_NONE = 0; /* the menu runs */
const LABEL_OPEN = 2; /* open the selected item */
const LABEL_BACK = 4;

/* The label menu shows LABEL_ROWS rows of LABEL_COLUMNS labels; LABEL_RING row planes are reused as it scrolls. */
const LABEL_ROWS = 8;
const LABEL_COLUMNS = 1;
const LABEL_RING = 9;

/* Modes of scroll_labels, show_row and hide_row: whether the crystal of a row flies in, and from where. */
const ROW_STILL = 0;
const ROW_FLY_FRONT = 1;
const ROW_FLY_BACK = 2;

/* Results of update_icon_menu. */
const ICON_STAY = 0;
const ICON_LEAVE = 1;     /* the page is left */
const ICON_NEXT_MENU = 2; /* another menu is to be shown */

/* Types of the link of an item (get_item_link, open_link). */
const LINK_DISABLED = -1;
const LINK_COMMAND = 0;
const LINK_PAGE = 1;
const LINK_SUBGROUP = 9; /* open_link takes 8 and 6 for it too */

/* Results of get_return_item besides the index of the item to select. */
const RETURN_FIRST = -1;    /* the first menu */
const RETURN_NEW_MENU = -2;
const RETURN_ERROR = -99;

/* Results of add_isp_item and load_isp_channels besides 0, 1 and -1 (an error). */
const ISPCH_NO_LIST = -2;    /* the ISP entry asks for no list: the item that leads to GROUP-ISPCH is disabled */
const ISPCH_NO_CHANNEL = -3; /* the ISP-CHANNEL item of the top menu is disabled */
const ISPCH_FAILED = -4;

#include "topmenu.inc"

forward Float:get_arc_radius(Float:best_dist);
forward Float:pseudo_random(seed);
forward Float:get_icon_x(slot, index, count, Float:best_dist);
forward Float:get_icon_y(slot, index, count, Float:best_dist);
forward Float:get_icon_z(slot, index, count, Float:best_dist);
forward Float:get_icon_alpha(slot);
forward Float:sin_deg(Float:degrees);

#include "wait.inc"

#include "tag.inc"

#include <float>

#include "imgsync.inc"

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

#include "imgdec.inc"

new g_common_bg_name[] = "SCEI:CommonBG";

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

#include "background.inc"

/* Switches the background scene back to its full mode, the opposite of set_background_poor. */
set_background_rich(Float:fade_in_time, fade_out_time)
{
    new scene = wndGetScene(WND_BG);
    if (scene)
    {
        SendMessage(scene, TAG("RICH"), fade_in_time, fade_out_time, 0, 0);
    }
}

#include "bgmode.inc"

/* Makes the camera of the background window follow this camera, by these shares of its position and rotation. */
link_bg_camera(Float:pos_follow, rot_follow)
{
    new bg_scene = wndGetScene(WND_BG);
    if (bg_scene)
    {
        SendMessage(bg_scene, TAG("LINK"), Camera(0), pos_follow, rot_follow, 0);
    }
}

#include "pfs.inc"

/* Sends the camera of the background window back home ("RELS"), animated over duration ms if animate is 1. */
release_bg_camera(animate, Float:duration)
{
    new bg_scene = wndGetScene(WND_BG);
    if (bg_scene)
    {
        SendMessage(bg_scene, TAG("RELS"), animate, duration, 0, 0);
    }
}

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

/* Returns the radius of the arc of the icons: four times the distance of the 2D display from the camera. */
Float:get_arc_radius(Float:best_dist)
{
    return 4.0 * best_dist;
}

/* Returns a fixed number from 0 to 999 for seed, which scatters the icons the same way every time. */
Float:pseudo_random(seed)
{
    new value = (seed * 12345 + 2371897) % 1000;
    return float(value);
}

/* Returns the x of the icon in slot: the sine of 0.25 rad per slot from the middle one, on a scattered radius. */
Float:get_icon_x(slot, index, count, Float:best_dist)
{
    new last = count - 1;
    return sin(0.25 * float(slot - last / 2)) * (get_arc_radius(best_dist) + 2.5 * pseudo_random(index + 19824));
}

/* Returns the y of an icon: a height from -950 to 848, scattered by index. */
Float:get_icon_y(slot, index, count, Float:best_dist)
{
    return 1.8 * pseudo_random(index + 1283) - 950.0 + 0.0 * slot + 0.0 * best_dist + 0.0 * count;
}

/* Returns the z of the icon in slot: 550 less the cosine of 0.15 rad per slot from the middle one, on a radius. */
Float:get_icon_z(slot, index, count, Float:best_dist)
{
    new last = count - 1;
    return 550.0
            + -1.0 * cos(0.15 * float(slot - last / 2))
                    * (get_arc_radius(best_dist) + 5.0 * pseudo_random(index + 19824));
}

#include "disc.inc"

/* Same as get_isp_count of topisp.inc. */
get_isp_count()
{
    return topIspGetCount();
}

/* Does nothing, and returns 0. */
open_isp_list(index, name[], maxlen)
{
    index = index; /* redundant as written */
    copy_string(name, name, maxlen);
}

/* Same as get_isp_value of topisp.inc. */
get_isp_value(index, field, value[], maxlen)
{
    index = index - 1; /* redundant as written */
    new key[32];
    switch (field)
    {
        case ISP_VER:
        {
            key = "VER";
        }
        case ISP_CATEGORY:
        {
            key = "CATEGORY";
        }
        case ISP_ID:
        {
            key = "ID";
        }
        case ISP_PRIORITY:
        {
            key = "PRIORITY";
        }
        case ISP_NAME:
        {
            key = "NAME";
        }
        case ISP_APPLICATION:
        {
            key = "APPLICATION";
        }
        case ISP_TYPE:
        {
            key = "TYPE";
        }
        case ISP_URL:
        {
            key = "URL";
        }
        case ISP_OPTION:
        {
            key = "OPTION";
        }
        case ISP_CONTENTS:
        {
            key = "CONTENTS";
        }
        default:
        {
            return -1;
        }
    }
    new ret = topIspGetValue(key, value, maxlen);
    printf("isp get data %s, val %s ret%d\n", key, value, ret);
    if (ret > 0)
    {
        return 0;
    }
    return -1;
}

new g_menu_group; /* the MENUGROUP element of the menu shown */

get_menu_group()
{
    return g_menu_group;
}

/* Makes the MENUGROUP element id the current one; returns whether it exists. */
find_menu_group(id[])
{
    g_menu_group = domFindTag(domRoot(), "MENUGROUP", id, 0);
    if (g_menu_group)
    {
        return 1;
    }
    return 0;
}

#include "validate.inc"

/* Reads the retgroup and retitem of the menu: the menu and the item to go back to; returns 0 if there are none. */
get_menu_return(ret_group[], ret_item[], group_len, item_len)
{
    if (!g_menu_group)
    {
        return 0;
    }
    if (!domGetData(g_menu_group, "retgroup", ret_group, group_len))
    {
        return 0;
    }
    if (!domGetData(g_menu_group, "retitem", ret_item, item_len))
    {
        return 0;
    }
    return 1;
}

/* Reads the label of the TITLE element of the menu; returns 0 if there is none. */
get_menu_title(label[], maxlen)
{
    if (!g_menu_group)
    {
        return 0;
    }
    new title = domFirstChild(g_menu_group, "TITLE");
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

/* Returns the MENU_* of the menutype of the menu, -1 if it has none. */
get_menu_type(value[], maxlen)
{
    if (!g_menu_group)
    {
        return -1;
    }
    if (!domGetData(g_menu_group, "menutype", value, maxlen))
    {
        return -1;
    }
    if (!strcmp(value, "label"))
    {
        return MENU_LABEL;
    }
    if (!strcmp(value, "icon"))
    {
        return MENU_ICON;
    }
    return -1;
}

/* Returns the ORDER_* of the order of the menu, -1 if it has none. */
get_menu_order(value[], maxlen)
{
    if (!g_menu_group)
    {
        return -1;
    }
    if (!domGetData(g_menu_group, "order", value, maxlen))
    {
        return -1;
    }
    if (!strcmp(value, "left"))
    {
        return ORDER_LEFT;
    }
    if (!strcmp(value, "right"))
    {
        return ORDER_RIGHT;
    }
    if (!strcmp(value, "center"))
    {
        return ORDER_CENTER;
    }
    return -1;
}

/* Returns the DETECT_* of the mediadetect of the menu, 0 if it has none. */
get_media_detect(value[], maxlen)
{
    if (!g_menu_group)
    {
        return 0;
    }
    if (!domGetData(g_menu_group, "mediadetect", value, maxlen))
    {
        return 0;
    }
    if (!strcmp(value, "all"))
    {
        return DETECT_ALL;
    }
    if (!strcmp(value, "game"))
    {
        return DETECT_GAME;
    }
    if (!strcmp(value, "music"))
    {
        return DETECT_MUSIC;
    }
    if (!strcmp(value, "movie"))
    {
        return DETECT_MOVIE;
    }
    if (!strcmp(value, "photo"))
    {
        return DETECT_PHOTO;
    }
    return 0;
}

/* Reads attribute attr of the menu group into value; returns -1 without a group. */
get_menu_attr(attr[], value[], maxlen)
{
    if (!g_menu_group)
    {
        return -1;
    }
    return domGetData(g_menu_group, attr, value, maxlen);
}

/* Returns the number of items of the current menu group, -1 without one. */
count_menu_items()
{
    new count;
    if (!g_menu_group)
    {
        return -1;
    }
    for (new node = domFirstChild(g_menu_group, "ITEM"); node; node = domNext(node, "ITEM"))
    {
        count++;
    }
    return count;
}

/* Returns the index of the item with that id in the current menu group, -1 if there is none. */
find_menu_item_index(id[])
{
    new index;
    if (!g_menu_group)
    {
        return -1;
    }
    for (new node = domFirstChild(g_menu_group, "ITEM"); node; node = domNext(node, "ITEM"))
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
    if (!g_menu_group)
    {
        return 0;
    }
    new node;
    node = domFirstChild(g_menu_group, "ITEM");
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

/* Reads attribute attr of the item id of the menu; returns 0 if there is no such item. */
get_named_item_attr(id[], attr[], value[], maxlen)
{
    if (!g_menu_group)
    {
        return 0;
    }
    new node = domFindTag(g_menu_group, "ITEM", id, 0);
    if (!node)
    {
        return 0;
    }
    return domGetData(node, attr, value, maxlen);
}

new Float:g_best_dist; /* the best_dist of the camera */
new g_item_count;
new Float:g_screen_w;
new Float:g_screen_h;
new g_menu_type = MENU_ICON;
new g_order = ORDER_CENTER;
new g_media_detect;
public init_status; /* INIT_* bits, read by top */

/*
 * Adds ISP entry isp_index of version 1 to group, as an ITEM with its ID, NAME and URL, as its OPTION says;
 * returns 0, -1 on an error, ISPCH_NO_LIST.
 */
add_isp_item(group, isp_index)
{
    if (topIspOpen(isp_index))
    {
        return 0;
    }
    new ver[32];
    if (get_isp_value(0, ISP_VER, ver, 32) < 0)
    {
        return -1;
    }
    new version = atoi(ver);
    if (version < 0)
    {
        return -1;
    }
    if (version > 1)
    {
        return 0;
    }
    new item = domInsertTag(domFirstChild(group, ""), "ITEM");
    if (!item)
    {
        return 0;
    }
    if (0 != domSetData(item, "id", "PSP-TEMP"))
    {
        return -1;
    }
    new value[128];
    if (get_isp_value(0, ISP_NAME, value, 128) < 0)
    {
        return -1;
    }
    if (0 != domSetData(item, "label", value))
    {
        return -1;
    }
    if (get_isp_value(0, ISP_OPTION, value, 128) < 0)
    {
        return -1;
    }
    if (!strcmp(value, "nolink"))
    {
        if (0 != domSetData(item, "link", "disable"))
        {
            return -1;
        }
    }
    else
    {
        if (!strcmp(value, "nodisp"))
        {
            if (0 != domRemoveTag(item))
            {
                return -1;
            }
            return 0;
        }
        if (!strcmp(value, "nolist"))
        {
            return ISPCH_NO_LIST;
        }
        if (!strcmp(value, "href"))
        {
            if (get_isp_value(0, ISP_URL, value, 128) < 0)
            {
                return -1;
            }
            if (0 != domSetData(item, "link", "href"))
            {
                return -1;
            }
            if (0 != domSetData(item, "href", value))
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }
    new id[32];
    if (get_isp_value(0, ISP_ID, id, 32) < 0)
    {
        return -1;
    }
    new old = domFindTag(group, "ITEM", id, 0);
    if (old)
    {
        if (0 != domRemoveTag(old))
        {
            return -1;
        }
    }
    if (0 != domSetData(item, "id", id))
    {
        return -1;
    }
    return 0;
}

/*
 * Fills the group GROUP-ISPCH with the ISP entries; returns 1, or a negative value when it has no entry: then the
 * item that leads to it (ISPCH_NO_LIST) or else the ISP-CHANNEL item of the top menu (ISPCH_NO_CHANNEL) is disabled;
 * ISPCH_FAILED when neither can be.
 */
load_isp_channels()
{
    new status;
    new group = domFindTag(domRoot(), "MENUGROUP", "GROUP-ISPCH", 0);
    if (!group)
    {
        return 0;
    }
    sleep;
    new count = get_isp_count();
    for (new i; i < count; i++)
    {
        status = add_isp_item(group, i);
        if (status <= -1)
        {
            printf("end_isp -1\n");
            new temp = domFindTag(group, "ITEM", "PSP-TEMP", 0);
            if (temp)
            {
                if (0 != domRemoveTag(temp))
                {
                    status = ISPCH_NO_LIST;
                    break;
                }
            }
        }
        if (status < -1)
        {
            break;
        }
    }
    if (!domFirstChild(group, "ITEM"))
    {
        status = ISPCH_NO_LIST;
    }
    if (status == ISPCH_NO_LIST)
    {
        printf("end_isp -2\n");
        new ret_group[64];
        new ret_item[64];
        if (!domGetData(group, "retgroup", ret_group, 64))
        {
            goto disable_channel;
        }
        if (!domGetData(group, "retitem", ret_item, 64))
        {
            goto disable_channel;
        }
        new parent_group = domFindTag(domRoot(), "MENUGROUP", ret_group, 0);
        sleep;
        if (!parent_group)
        {
            goto disable_channel;
        }
        new parent_item = domFindTag(parent_group, "ITEM", ret_item, 0);
        if (!parent_item)
        {
            goto disable_channel;
        }
        if (0 != domSetData(parent_item, "link", "disable"))
        {
            goto disable_channel;
        }
        return ISPCH_NO_LIST;
    }
    if (status == ISPCH_NO_CHANNEL)
    {
        disable_channel:
        printf("end_isp -3\n");
        new top_group = domFindTag(domRoot(), "MENUGROUP", "GROUP-TOPMENU", 0);
        sleep;
        if (!top_group)
        {
            return ISPCH_FAILED;
        }
        new isp_item = domFindTag(top_group, "ITEM", "ISP-CHANNEL", 0);
        if (!isp_item)
        {
            return ISPCH_FAILED;
        }
        if (0 != domSetData(isp_item, "link", "disable"))
        {
            return ISPCH_FAILED;
        }
        return ISPCH_NO_CHANNEL;
    }
    return 1;
}

/* Loads the ISP channels and records the menu and the channel of the arguments menu_id and channel_id as current. */
apply_page_args()
{
    new menu_id[64];
    new channel_id[64];
    new found;
    load_isp_channels();
    new group;
    if (GetArg("menu_id", menu_id, 64))
    {
        printf("getarg menu id = %s !!!!!\n", menu_id);
        group = domFindTag(domRoot(), "MENUGROUP", "GROUP-TOPMENU", 0);
        while (group)
        {
            new id[64];
            domGetData(group, "id", id, 64);
            if (!strcmp(menu_id, id))
            {
                found = 1;
                break;
            }
            group = domNext(group, "MENUGROUP");
        }
        if (found)
        {
            avalSet("ChannelCategory", menu_id);
            avalSet("ChannelTransit", "NEXT_HIER");
            found = 0;
        }
    }
    if (GetArg("channel_id", channel_id, 64))
    {
        printf("getarg channel id = %s !!!!!\n", channel_id);
        if (!group)
        {
            group = domFindTag(domRoot(), "MENUGROUP", "GROUP-TOPMENU", 0);
        }
        while (group)
        {
            new id[64];
            new item;
            item = domFirstChild(group, "ITEM");
            domGetData(group, "id", menu_id, 64);
            printf("find menu id = %s\n", menu_id);
            while (item)
            {
                domGetData(item, "id", id, 64);
                if (!strcmp(channel_id, id))
                {
                    found = 1;
                    break;
                }
                item = domNext(item, "ITEM");
            }
            if (found)
            {
                break;
            }
            group = domNext(group, "MENUGROUP");
        }
        if (found)
        {
            avalSet("ChannelIntoID", channel_id);
            avalSet("ChannelCategory", menu_id);
            avalSet("ChannelTransit", "OTHER_CHANNEL");
            found = 0;
        }
    }
}

close_isp_list()
{
    topIspClose();
}

/* Makes the MENUGROUP id current and reads its type, order and item count; returns -1 if it has no item. */
open_menu_group(id[])
{
    new type[32];
    find_menu_group(id);
    g_menu_type = get_menu_type(type, 32);
    if (g_menu_type < 0)
    {
        sprintf(type, "icon");
        g_menu_type = MENU_ICON;
    }
    avalSet("ChannelType", type);
    new order[32];
    g_order = ORDER_CENTER;
    g_order = get_menu_order(order, 32);
    if (g_order < 0)
    {
        sprintf(order, "left");
        g_order = ORDER_LEFT;
    }
    avalSet("ChannelOrder", order);
    g_item_count = 0;
    g_item_count = count_menu_items();
    if (g_item_count <= 0)
    {
        init_status = init_status | INIT_NO_ITEM;
        return -1;
    }
    return 0;
}

new g_slot; /* the selected slot of the icon menu */
new g_view = VIEW_WIDE;
new g_first_slot; /* the slot of the first icon of the list, which holds the slots within 8 of g_slot */
new g_zoom_enabled; /* up and down change the view; never set */
new g_pad_on; /* the pad_enable of the camera, last seen */

/* Does nothing. */
init_list()
{
}

reset_first_slot()
{
    g_first_slot = 0;
}

/* Creates the icon of g_slot and those within 8 slots of it, flying in if fly_in; returns the icon of g_slot. */
create_icons(fly_in)
{
    new selected;
    new more = 1;
    new index = get_slot_item(g_slot, g_order);
    new icon = create_icon(index);
    send_icon_image(icon, index);
    if (fly_in)
    {
        fly_in_icon(icon, index);
    }
    listAddLast(icon);
    printf("add item - %d, %d\n", index, g_slot);
    selected = icon;
    while (more)
    {
        more = 0;
        if (g_first_slot - 1 >= 0 && g_first_slot - 1 >= g_slot - 8)
        {
            printf("-------------- 4 --------------%d, %d\n", index, g_slot);
            index = get_slot_item(g_first_slot - 1, g_order);
            icon = create_icon(index);
            send_icon_image(icon, index);
            if (fly_in)
            {
                fly_in_icon(icon, index);
            }
            listInsert(0, icon);
            g_first_slot--;
            more = 1;
            continue;
        }
        if (g_first_slot + listSize() < g_item_count && g_first_slot + listSize() <= g_slot + 8)
        {
            printf("-------------- 5 --------------%d, %d\n", index, g_slot);
            index = get_slot_item(g_first_slot + listSize(), g_order);
            icon = create_icon(index);
            send_icon_image(icon, index);
            if (fly_in)
            {
                fly_in_icon(icon, index);
            }
            listAddLast(icon);
            more = 1;
            continue;
        }
    }
    return selected;
}

/* Shows the icon of item index from far ("FRST"), until the circle button or a timer of 500 ms ends it. */
show_intro(index)
{
    new camera = Camera(0);
    new icon;
    set_background_rich(600.0, 0);
    AnimTimerManager(Self(), 300.0, 0.0, ANIM_FUNC_LINEAR);
    g_slot = get_slot(index, g_order);
    g_first_slot = g_slot;
    icon = create_icons(1);
    while (ANIM_DONE != AnimGetState(Self(), ANIM_TIMER))
    {
        sleep;
    }
    g_view = VIEW_CLOSE;
    SendMessage(camera, TAG("DIST"), g_view, 0, 0, 0);
    SendMessage(camera, TAG("FRST"), icon, 1000.0, 0, 0);
    sleep;
    link_bg_camera(1.2, 0);
    adjust_bg_camera();
    wait_ms(500);
    AnimTimerManager(Self(), 500.0, 0.0, ANIM_FUNC_LINEAR);
    for (; ; )
    {
        new held;
        new pressed;
        new released;
        new repeated;
        PadState(held, pressed, released, repeated, -1);
        if (pressed & PAD_CIRCLE)
        {
            break;
        }
        if (ANIM_DONE == AnimGetState(Self(), ANIM_TIMER))
        {
            break;
        }
        sleep;
    }
}

/* Shows a new icon menu, the camera coming from far to the icon of item index. */
enter_icon_menu(index)
{
    new camera = Camera(0);
    new icon;
    set_background_rich(1000.0, 0);
    g_slot = get_slot(index, g_order);
    g_first_slot = g_slot;
    icon = create_icons(1);
    g_view = VIEW_CLOSE;
    SendMessage(camera, TAG("DIST"), g_view, 0, 0, 0);
    SendMessage(camera, TAG("FRST"), icon, 1200.0, 0, 0);
    wait_ms(500);
}

/* Shows the icon menu on return from item index, the camera backing away from its icon. */
return_to_icon_menu(index)
{
    new camera = Camera(0);
    new icon;
    set_background_rich(1000.0, 0);
    g_slot = get_slot(index, g_order);
    g_first_slot = g_slot;
    icon = create_icons(0);
    g_view = VIEW_CLOSE;
    SendMessage(camera, TAG("DIST"), g_view, 0, 0, 0);
    SendMessage(camera, TAG("BACK"), icon, 2000.0, 20.0, 0);
    wait_ms(1000);
}

/* Fades out the menu while the camera flies into the selected icon ("INTO"). */
fly_into_icon(Float:duration)
{
    new camera = Camera(0);
    SetSystemParam(SYS_PADHELP, 0);
    sndBgmPlay(2);
    fade_background(0, 0, 0.4 * duration);
    for (new i; i < listSize(); i++)
    {
        new icon = listGetAt(i);
        SendMessage(icon, TAG("FADE"), 0, 0.4 * duration, 0.07 * duration, 0);
        SendMessage(icon, TAG("SLEP"), 0.4 * duration, 0.07 * duration, 0, 0);
    }
    new target = listGetAt(g_slot - g_first_slot);
    SendMessage(camera, TAG("INTO"), target, duration, 0, 0);
    wait_ms(floatround(1.0 * duration, 0));
    AnimTimerManager(Self(), 0.6 * duration, 0.0, ANIM_FUNC_LINEAR);
}

/* Fades out the menu and the background for going back to the previous menu. */
leave_icon_menu()
{
    SetSystemParam(SYS_PADHELP, 0);
    sndSePlay(SE_CANCEL);
    fade_background(0, 0, 1000.0);
    SendMessage(Camera(0), TAG("RSET"), 0, 0, 0, 0);
    for (new i; i < listSize(); i++)
    {
        SendMessage(listGetAt(i), TAG("FADE"), 0, 1000.0, 0, 0);
    }
    AnimTimerManager(Self(), 1000.0, 0.0, ANIM_FUNC_LINEAR);
}

/*
 * One frame of the icon menu: the pad help, left and right, circle (open the item) and cross (go back); keeps the
 * icons within 8 slots of the selection. Returns an ICON_*.
 */
update_icon_menu()
{
    new camera = Camera(0);
    new moved;
    new zoomed;
    {
        new pad_on = GetPubVar(Camera(0), "pad_enable");
        if (g_pad_on && !pad_on)
        {
            SetSystemParam(SYS_PADHELP, GetSystemParam(SYS_PADHELP) & ~(PADHELP_CIRCLE | PADHELP_CROSS));
            g_pad_on = pad_on;
        }
        else if (!g_pad_on && pad_on)
        {
            new ret_group[64];
            new ret_item[64];
            if (!get_menu_return(ret_group, ret_item, 64, 64))
            {
                SetSystemParam(SYS_PADHELP, GetSystemParam(SYS_PADHELP) | PADHELP_CIRCLE);
            }
            else
            {
                SetSystemParam(SYS_PADHELP, GetSystemParam(SYS_PADHELP) | (PADHELP_CIRCLE | PADHELP_CROSS));
            }
            g_pad_on = pad_on;
        }
        new held;
        new pressed;
        new released;
        new repeated;
        PadState(held, pressed, released, repeated, -1);
        if (pressed & PAD_RIGHT)
        {
            if (g_slot + 1 <= g_item_count - 1)
            {
                g_slot++;
                moved = 1;
                sndSePlay(SE_ICON_MOVE);
            }
        }
        else if (pressed & PAD_LEFT)
        {
            if (g_slot - 1 >= 0)
            {
                g_slot--;
                moved = 1;
                sndSePlay(SE_ICON_MOVE);
            }
        }
        else if (pressed & PAD_CIRCLE && g_pad_on)
        {
            if (VIEW_CLOSE == g_view)
            {
                zoomed = 1;
                new target[64];
                new index = get_slot_item(g_slot, g_order);
                new link_type = get_item_link(index, target, 64);
                if (link_type > 0)
                {
                    fly_into_icon(1000.0);
                    link_type = open_link(index, link_type, target);
                }
                if (link_type == LINK_SUBGROUP)
                {
                    SendMessage(Camera(0), TAG("RSET"), 0, 0, 0, 0);
                    return ICON_NEXT_MENU;
                }
                else
                {
                    return ICON_LEAVE;
                }
            }
            else
            {
                sndSePlay(SE_ICON_MOVE);
                zoomed = 1;
                g_view = VIEW_CLOSE;
            }
        }
        else if (pressed & PAD_CROSS && g_pad_on)
        {
            new back = go_back();
            leave_icon_menu();
            if (back > 0)
            {
                return ICON_NEXT_MENU;
            }
        }
        else if (pressed & PAD_UP && g_zoom_enabled)
        {
            if (g_view == VIEW_WIDE)
            {
                sndSePlay(SE_ICON_MOVE);
                zoomed = 1;
                g_view = VIEW_CLOSE;
            }
        }
        else if (pressed & PAD_DOWN&& g_zoom_enabled)
        {
            if (VIEW_CLOSE == g_view)
            {
                sndSePlay(SE_ICON_MOVE);
                zoomed = 1;
                g_view = VIEW_WIDE;
            }
        }
    }
    if (moved || (g_item_count > 0 && 0 == listSize()))
    {
        while (g_item_count)
        {
            if (g_first_slot < g_slot - 8)
            {
                printf("---------------- 2 ----------------\n");
                new icon = listGetAt(0);
                Destroy(icon);
                listRemoveAt(0);
                printf("add item - %d\n", g_first_slot);
                g_first_slot++;
                continue;
            }
            if (g_first_slot + listSize() - 1 > g_slot + 8)
            {
                printf("---------------- 3 ----------------\n");
                new icon = listGetLast();
                Destroy(icon);
                listRemoveLast();
                printf("add item - %d\n", g_first_slot + listSize() - 1);
                continue;
            }
            if (g_first_slot - 1 >= 0 && g_first_slot - 1 >= g_slot - 8)
            {
                printf("---------------- 4 ----------------\n");
                new index = get_slot_item(g_first_slot - 1, g_order);
                new icon = create_icon(index);
                send_icon_image(icon, index);
                listInsert(0, icon);
                printf("add item - %d, %d\n", index, g_slot);
                g_first_slot--;
                continue;
            }
            if (g_first_slot + listSize() < g_item_count && g_first_slot + listSize() <= g_slot + 8)
            {
                printf("---------------- 5 ----------------\n");
                new index = get_slot_item(g_first_slot + listSize(), g_order);
                new icon = create_icon(index);
                send_icon_image(icon, index);
                listAddLast(icon);
                printf("add item - %d, %d\n", index, g_slot);
                continue;
            }
            break;
        }
    }
    if (zoomed)
    {
        SendMessage(camera, TAG("DIST"), g_view, 0, 0, 0);
        SendMessage(camera, TAG("LOOK"), listGetAt(g_slot - g_first_slot), 0, 0, 0);
    }
    if (moved)
    {
        new pos = g_slot - g_first_slot;
        if (pos < listSize())
        {
            SendMessage(camera, TAG("LOOK"), listGetAt(pos), 0, 0, 0);
        }
        for (new i; i < listSize(); i++)
        {
            SendMessage(listGetAt(i), TAG("FADE"), get_icon_alpha(i + g_first_slot), 0, 0, 0);
        }
    }
    return ICON_STAY;
}

/* Has icon decode the image of item index. */
send_icon_image(icon, index)
{
    new url[128];
    new follower[128];
    get_item_attr(index, "icon", url, 128);
    SendMessageStr(icon, TAG("DCDE"), url);
    SendMessageStr(icon, TAG("FLWR"), follower);
}

/* Creates the icon of item index at its place on the arc. */
create_icon(index)
{
    new icon = CreateNull(Self());
    new slot = get_slot(index, g_order);
    SetColor(icon, 1.0, 1.0, 1.0, 1.0);
    SetPos(icon, get_icon_x(slot, index, g_item_count, g_best_dist), get_icon_y(slot, index, g_item_count, g_best_dist),
            get_icon_z(slot, index, g_item_count, g_best_dist), 0);
    LookAt(icon, Scene(0), 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
    AttachScript(icon, "item_main", "top_item");
    return icon;
}

/*
 * The label menu shows 8 rows of the list; 9 row planes, each with its crystal, are used in turn (index % 9), the
 * ninth one for the row scrolling in or out.
 */
new g_crystal_tex[4];
new Float:g_row_height = 92.0;
new Float:g_list_x;
new Float:g_list_y;
new Float:g_move_time = 200.0; /* of a move: 200 ms, 100 ms while a direction repeats */
new g_focus;
new g_pad_help; /* PADHELP_* bits */
new g_label_count;
new g_select;
new g_top; /* the first row shown */
new g_ring_first; /* the first of the LABEL_RING rows that animate_rows turns */
new g_select_node; /* the ITEM element of g_select */
new g_repeating;
new g_label_done;
new g_repeat_delay; /* frames before up (bits 24-31) or down (bits 16-23) repeats */
new g_max_top;
new g_rows_root;
new g_rows[LABEL_RING];
new g_crystals[LABEL_RING];
new g_title;
new g_scroll_bar;
new g_row_enabled[LABEL_RING];
new g_label_result; /* LABEL_* */
new Float:g_crystal_time[LABEL_RING];
new g_crystal_seed; /* picks the motion of the first crystal, the next ones take the next motions */
new Float:g_crystal_x = 210.0; /* the motions of the crystals are around this point, at this scale */
new Float:g_crystal_y = -10.0;
new Float:g_crystal_z = 125.0;
new Float:g_crystal_scale = 25.0;

/* Moves icon from nearer the camera to its place on the arc. */
fly_in_icon(icon, index)
{
    new slot = get_slot(index, g_order);
    new Float:x = get_icon_x(slot, index, g_item_count, g_best_dist);
    new Float:y = get_icon_y(slot, index, g_item_count, g_best_dist);
    new Float:z = get_icon_z(slot, index, g_item_count, g_best_dist);
    SetPos(icon, 0.7 * x, 0.7 * y, 0.9 * z, 0);
    AnimMoveHermite(icon, x, y, z, 1500.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
}

/* Returns the alpha of the icon in slot: always 1.0. */
Float:get_icon_alpha(slot)
{
    if (g_slot == slot)
    {
        return 1.0;
    }
    if (abs(g_slot - slot) > 3)
    {
        return 1.0;
    }
    return 1.0;
}

/* Returns the slot of item index on the arc, in the order order. */
get_slot(index, order)
{
    if (ORDER_LEFT == order)
    {
        return index;
    }
    if (order == ORDER_RIGHT)
    {
        return g_item_count - 1 - index;
    }
    new last = g_item_count - 1 + g_item_count % 2;
    if (0 == index % 2)
    {
        index = last / 2 - index / 2;
    }
    else
    {
        index = last / 2 + (index + 1) / 2;
    }
    if (order == ORDER_CENTER_EVEN)
    {
        index = index; /* redundant as written */
    }
    else if (order == ORDER_CENTER_ODD)
    {
        index = g_item_count - 1 - index;
    }
    else if (order == ORDER_CENTER)
    {
        if (0 == g_item_count % 2)
        {
            index = index;
        }
        else
        {
            index = g_item_count - 1 - index;
        }
    }
    return index;
}

/* Returns the item in slot, the inverse of get_slot. */
get_slot_item(slot, order)
{
    if (ORDER_LEFT == order)
    {
        return slot;
    }
    if (order == ORDER_RIGHT)
    {
        return g_item_count - 1 - slot;
    }
    new last = g_item_count - 1 + g_item_count % 2;
    if (order == ORDER_CENTER_EVEN)
    {
        slot = slot; /* redundant as written */
    }
    else if (order == ORDER_CENTER_ODD)
    {
        slot = g_item_count - 1 - slot;
    }
    else if (order == ORDER_CENTER)
    {
        if (0 == g_item_count % 2)
        {
            slot = slot;
        }
        else
        {
            slot = g_item_count - 1 - slot;
        }
    }
    if (slot <= last / 2)
    {
        slot = last - slot * 2 - 1;
    }
    else
    {
        slot = slot * 2 - last;
    }
    return slot;
}

/* Shows the label menu with item select selected, the rows flying in from the front if fly_in, else from behind. */
init_label_menu(select, fly_in)
{
    SendMessage(Camera(0), TAG("RSET"), 0, 0, 0, 0);
    g_label_count = g_item_count;
    if (g_label_count <= 0)
    {
        g_pad_help = g_pad_help & ~(PADHELP_CIRCLE | PADHELP_TRIANGLE);
    }
    g_max_top = (g_label_count + LABEL_COLUMNS - 1) / LABEL_COLUMNS - LABEL_ROWS;
    if (g_max_top < 0)
    {
        g_max_top = 0;
    }
    g_top = 0;
    g_ring_first = 0;
    g_select = select;
    g_select = clamp(g_select, 0, g_label_count);
    new row = g_select / LABEL_COLUMNS;
    if (row < g_top)
    {
        g_top = row;
    }
    if (row >= g_top + LABEL_ROWS)
    {
        g_top = row - LABEL_ROWS + 1;
    }
    g_select_node = domFirstChild(get_menu_group(), "ITEM");
    for (new i; i < g_select; i++)
    {
        g_select_node = domNext(g_select_node, "ITEM");
    }
    g_label_done = 0;
    g_label_result = LABEL_NONE;
    g_row_height = 0.0583 * g_screen_h;
    {
        new hour;
        new minute;
        time(hour, minute, g_crystal_seed, 0);
        minute = g_crystal_seed / 10;
        g_crystal_seed = g_crystal_seed - minute;
        set_crystal_layout(g_label_count);
    }
    {
        new category[32];
        avalGetValue("ChannelCategory", 0, category, 32);
        if (!strcmp("GROUP-TOPMENU", category))
        {
            g_pad_help = 0;
        }
        else
        {
            g_pad_help = PADHELP_CROSS;
        }
    }
    create_label_menu();
    scroll_labels(g_top, 0.0, fly_in ? ROW_FLY_FRONT : ROW_FLY_BACK);
    select_label(g_select);
}

/* Fades out the label menu and opens the selected item if it was pushed; returns the selected index. */
close_label_menu()
{
    send_focus(g_focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    if (g_title)
    {
        AnimColor(g_title, 0.902, 0.902, 0.215, 0.0, 500.0, 0.0, ANIM_FUNC_LINEAR);
    }
    if (g_scroll_bar)
    {
        SendMessage(g_scroll_bar, TAG("FADE"), 0, 500.0, 0, 0);
    }
    if (g_label_result == LABEL_BACK)
    {
        sndSePlay(SE_CANCEL);
        g_crystal_z = g_crystal_z - 120.0;
    }
    if (g_label_result == LABEL_OPEN || g_label_result == 3)
    {
        sndSePlay(SE_DECIDE);
        g_crystal_z = 120.0 + g_crystal_z;
    }
    fade_out_labels(500.0);
    if (g_scroll_bar)
    {
        Destroy(g_scroll_bar);
        g_scroll_bar = 0;
    }
    if (g_title)
    {
        Destroy(g_title);
        g_title = 0;
    }
    if (g_rows_root)
    {
        Destroy(g_rows_root);
        g_rows_root = 0;
    }
    Destroy(g_focus);
    g_focus = 0;
    if (g_label_result == LABEL_OPEN || g_label_result == 3)
    {
        new target[64];
        new link_type = get_item_link(g_select, target, 64);
        if (link_type > 0)
        {
            link_type = open_link(g_select, link_type, target);
        }
    }
    return g_select;
}

/* One frame of the label menu; returns g_label_result. */
update_label_menu()
{
    if (!g_label_done)
    {
        handle_label_pad();
    }
    animate_rows();
    return g_label_result;
}

/* Counts down the delay of the repeating direction; a released direction stops repeating. */
count_down_repeat(held)
{
    if (g_repeat_delay & 0xFF0000)
    {
        if (~held & PAD_DOWN)
        {
            g_repeating = 0;
        }
        g_repeat_delay = g_repeat_delay - 0x10000;
    }
    else if (g_repeat_delay & 0xFF000000)
    {
        if (~held & PAD_UP)
        {
            g_repeating = 0;
        }
        g_repeat_delay = g_repeat_delay - 0x1000000;
    }
}

/* Up and down move the selection, repeating while held; circle opens the item, cross goes back. */
handle_label_pad()
{
    new held;
    new pressed;
    PadState(held, pressed, _, _, -1);
    count_down_repeat(held);
    new dir = held & (PAD_UP | PAD_RIGHT | PAD_DOWN | PAD_LEFT);
    if (g_repeat_delay > 0)
    {
        dir = 0;
    }
    if (0 == held)
    {
        return;
    }
    if (0 != dir)
    {
        if (dir & PAD_UP)
        {
            if (!g_repeating || pressed & PAD_UP)
            {
                g_repeating = 1;
                g_move_time = 200.0;
            }
            else
            {
                g_move_time = 100.0;
            }
            g_repeat_delay = floatround(0.001 * (float(FrameRate()) * g_move_time), 0) << 24;
            if (select_label(g_select - 1))
            {
                sndSePlay(SE_CURSOR);
            }
        }
        else if (dir & PAD_DOWN)
        {
            if (!g_repeating || pressed & PAD_DOWN)
            {
                g_move_time = 200.0;
                g_repeating = 1;
            }
            else
            {
                g_move_time = 100.0;
            }
            g_repeat_delay = floatround(0.001 * (float(FrameRate()) * g_move_time), 0) << 16;
            if (select_label(g_select + 1))
            {
                sndSePlay(SE_CURSOR);
            }
        }
    }
    if (0 != pressed)
    {
        if (pressed & PAD_CIRCLE && g_pad_help & PADHELP_CIRCLE)
        {
            if (g_label_count > 0)
            {
                g_label_result = LABEL_OPEN;
                g_label_done = 1;
            }
        }
        else if (pressed & PAD_CROSS && g_pad_help & PADHELP_CROSS)
        {
            new back = go_back();
            if (back > 0)
            {
                g_label_result = LABEL_BACK;
                g_label_done = 1;
            }
        }
    }
}

/* Selects row index, scrolling the list if it is out of sight; returns whether the selection changed. */
select_label(index)
{
    if (index < 0)
    {
        return 0;
    }
    if (index >= g_label_count)
    {
        if (index - (g_label_count - 1) / LABEL_COLUMNS * LABEL_COLUMNS >= LABEL_COLUMNS)
        {
            return 0;
        }
        if (g_label_count <= 1)
        {
            return 0;
        }
        if (g_select != g_label_count - 1)
        {
            index = g_label_count - 1;
        }
        else
        {
            index = g_label_count - 1;
        }
    }
    new old = g_select;
    g_select = index;
    new delta = index - old;
    if (delta > 0)
    {
        for (new i; g_select_node && i < delta; i++)
        {
            g_select_node = domNext(g_select_node, "ITEM");
        }
    }
    else if (delta < 0)
    {
        for (new i; g_select_node && i > delta; i--)
        {
            g_select_node = domPrev(g_select_node, "ITEM");
        }
    }
    new ring = index % LABEL_RING;
    new crystal = g_crystals[ring];
    AnimColor(crystal, 1.0, 1.0, 1.0, 1.0, g_move_time, 0.0, ANIM_FUNC_LINEAR);
    AnimTimerManager(crystal, 3000.0, 0.0, ANIM_FUNC_EXPONENT);
    AnimDoTimer(crystal, ANIM_TIMER, 1.0);
    g_crystal_time[ring] = 0.0;
    if (index < g_top * LABEL_COLUMNS)
    {
        if (g_top > 0)
        {
            scroll_labels(g_top - 1, g_move_time, ROW_STILL);
        }
    }
    else if (index >= g_top * LABEL_COLUMNS + LABEL_ROWS)
    {
        if (g_top < g_max_top)
        {
            scroll_labels(g_top + 1, g_move_time, ROW_STILL);
        }
    }
    new row = g_rows[ring];
    if (g_row_enabled[ring])
    {
        send_focus(g_focus, row, 0, 0.5, 60.0, 60.0, 0.8 * g_move_time, 1, 1);
        g_pad_help = g_pad_help | PADHELP_CIRCLE;
    }
    else
    {
        send_focus(g_focus, row, 0, 0.5, 60.0, 60.0, 0.8 * g_move_time, 1, 0);
        g_pad_help = g_pad_help & ~PADHELP_CIRCLE;
    }
    update_pad_help();
    return index != old;
}

/*
 * Scrolls the list to show 8 rows from top over duration ms: by one row the row planes are reused, else all 8 are
 * shown again; ROW_FLY_FRONT and ROW_FLY_BACK fly them in (show_row).
 */
scroll_labels(top, Float:duration, mode)
{
    if (top < 0)
    {
        return 0;
    }
    if (top > g_max_top)
    {
        return 0;
    }
    new old_top = g_top;
    g_top = top;
    new Float:x = g_list_x;
    new Float:y = g_list_y - g_row_height * float(g_top);
    if (200.0 == g_move_time)
    {
        AnimMoveLine(g_rows_root, x, y, 0.0, 1.2 * duration, 0.0, ANIM_FUNC_EXPONENT);
    }
    else
    {
        AnimMoveLine(g_rows_root, x, y, 0.0, duration, 0.0, ANIM_FUNC_LINEAR);
    }
    new delta = top - old_top;
    if (ROW_STILL == mode && (delta == -1 || delta == 1))
    {
        new show_index;
        new hide_index;
        if (delta < 0)
        {
            show_index = top * LABEL_COLUMNS;
            hide_index = show_index + LABEL_ROWS;
            g_ring_first = top;
        }
        else
        {
            hide_index = old_top * LABEL_COLUMNS;
            show_index = hide_index + LABEL_ROWS;
            g_ring_first = top - 1;
        }
        for (new i; i < LABEL_COLUMNS; i++)
        {
            hide_row(hide_index++, mode, duration);
            show_row(show_index++, mode, duration);
        }
    }
    else
    {
        new index = top * LABEL_COLUMNS;
        g_ring_first = top;
        for (new i; i < LABEL_ROWS; i++)
        {
            show_row(index++, mode, duration);
        }
    }
    return top != old_top;
}

/* Creates the focus, the title, the scroll bar for more than 8 items, and the 9 row planes and crystals. */
create_label_menu()
{
    g_focus = create_focus_plane2("focus");
    new has_title;
    new title[64];
    if (get_menu_title(title, 64))
    {
        new Float:x;
        new Float:y;
        g_title = CreatePlane(Disp2D(0));
        get_list_title_pos(0, g_label_count, 1, x, y);
        g_list_x = x * g_screen_w;
        SetPos(g_title, g_list_x, g_screen_h * y, 0.0, 0);
        SetColor(g_title, 0.902, 0.902, 0.215, 0.0);
        SetOptionI(g_title, TAG("ZSRT"), ZSORT_OVERLAY);
        imgDecStringStart(title, 17, 64, 64, floatround(300.0 / 0.6, 0));
        if (wait_image_decode() == IMG_DONE)
        {
            SetTexture(g_title, imgTexID(), 0, -1);
            new Float:w = float(imgTexWidth());
            new Float:h = float(imgTexHeight());
            if (w > 300.0)
            {
                w = 300.0;
            }
            SetLocalScale(g_title, w, h, 1.0);
        }
        AnimColor(g_title, 0.902, 0.902, 0.215, 1.0, 800.0, 0.0, ANIM_FUNC_SLOWER);
        has_title = 1;
    }
    if (g_item_count > LABEL_ROWS)
    {
        g_scroll_bar = CreateNull(Disp2D(0));
        new Float:x;
        new Float:y;
        get_scroll_bar_pos(0, g_item_count, has_title, x, y);
        SetPos(g_scroll_bar, g_screen_w * x, g_screen_h * y, 0.0, 0);
        get_list_body_height(g_item_count, has_title, y);
        SetOptionI(g_scroll_bar, TAG("ZSRT"), ZSORT_OVERLAY);
        AttachScript(g_scroll_bar, _, "scroll_bar");
        SetPubVar(g_scroll_bar, "manager", Self());
        SetPubVar(g_scroll_bar, "focus", g_focus);
        SetPubVar(g_scroll_bar, "height", g_screen_h * (y + 0.0583 * 2.0));
        SendMessage(g_scroll_bar, TAG("FADE"), 1.0, 800.0, 0, 0);
    }
    g_rows_root = CreateNull(Disp2D(0));
    get_list_top(g_item_count, has_title, g_list_y);
    g_list_y = g_list_y * g_screen_h;
    SetPos(g_rows_root, g_list_x, g_list_y, 0.0, 0);
    for (new i; i < LABEL_RING; i++)
    {
        new crystal = CreateCrystal(g_rows_root);
        g_crystals[i] = crystal;
        SetLocalScale(crystal, 25.0, 25.0, 25.0);
        SetTexture(crystal, g_crystal_tex[0], 0, -1);
        SetTexture(crystal, g_crystal_tex[1], 1, -1);
        SetTexture(crystal, g_crystal_tex[0], 2, -1);
        SetTexture(crystal, g_crystal_tex[2], 3, -1);
        SetTexture(crystal, g_crystal_tex[3], 4, -1);
        SetColor(crystal, 1.0, 1.0, 1.0, 0.0);
        SetOptionF(crystal, TAG("FADE"), 0.0);
        SetOptionF(crystal, TAG("RFF0"), 0.2);
        SetOptionF(crystal, TAG("RFF1"), 0.4);
    }
    for (new i; i < LABEL_RING; i++)
    {
        new row = CreatePlane(g_rows_root);
        g_rows[i] = row;
        SetColor(row, 0.215, 0.708, 0.999, 0.0);
        SetOptionI(row, TAG("ZSRT"), ZSORT_OVERLAY);
        SetOptionI(row, TAG("NUM"), 2);
        new Float:color[4] = {1.0, 1.0, 1.0, 1.0};
        SetItemOptionP(row, 1, TAG("COLR"), color, 4);
        SetItemOptionI(row, 1, TAG("ALPH"), BLEND_ADD);
        SetItemOptionI(row, 0, TAG("ALPH"), BLEND_ALPHA);
    }
}

/* Shows row index in its plane; ROW_FLY_FRONT or ROW_FLY_BACK also flies its crystal in from there. */
show_row(index, mode, Float:duration)
{
    new ring = (index + LABEL_RING) % LABEL_RING;
    new row = g_rows[ring];
    new crystal = g_crystals[ring];
    if (index < 0 || index >= g_label_count)
    {
        SetColor(row, 0.215, 0.708, 0.999, 0.0);
        return;
    }
    load_row_label(index);
    new Float:x;
    new Float:y = index * g_row_height;
    new Float:z;
    new Float:fade_time = duration;
    SetPos(row, x, y, z, 0);
    if (mode >= ROW_FLY_FRONT)
    {
        new Float:z_offset;
        if (mode == ROW_FLY_FRONT)
        {
            z_offset = -40.0;
        }
        else if (mode == ROW_FLY_BACK)
        {
            z_offset = 40.0;
        }
        new Float:dx;
        new Float:dy;
        new Float:dz;
        get_crystal_pos((ring + g_crystal_seed) % 10, 0.0, dx, dy, dz);
        x = x + dx;
        y = y + dy;
        z = z + dz;
        SetPos(crystal, x, y, z + z_offset, 0);
        fade_time = 800.0;
        AnimMoveLine(crystal, x, y, z, fade_time, 0.0, ANIM_FUNC_EXPONENT);
    }
    if (g_row_enabled[ring])
    {
        SetColor(row, 0.215, 0.708, 0.999, 0.0);
        AnimColor(row, 0.215, 0.708, 0.999, 1.0, fade_time, 0.0, ANIM_FUNC_EXPONENT);
    }
    else
    {
        SetColor(row, 0.03, 0.18, 0.36, 0.0);
        AnimColor(row, 0.03, 0.18, 0.36, 1.0, fade_time, 0.0, ANIM_FUNC_EXPONENT);
    }
}

/* Fades out row index; unless ROW_STILL its crystal goes back to the start of its motion. */
hide_row(index, mode, Float:duration)
{
    new ring = (index + LABEL_RING) % LABEL_RING;
    new row = g_rows[ring];
    new crystal = g_crystals[ring];
    new Float:fade_time = duration;
    if (mode >= ROW_FLY_FRONT)
    {
        new Float:x;
        new Float:y;
        new Float:z;
        GetPos(row, x, y, z, 0);
        new Float:dx;
        new Float:dy;
        new Float:dz;
        get_crystal_pos((ring + g_crystal_seed) % 10, 0.0, dx, dy, dz);
        x = x + dx;
        y = y + dy;
        z = z + dz;
        fade_time = 500.0;
        AnimMoveLine(crystal, x, y, z, fade_time, 0.0, ANIM_FUNC_EXPONENT);
    }
    if (g_row_enabled[ring])
    {
        AnimColor(g_rows[ring], 0.215, 0.708, 0.999, 0.0, fade_time, 0.0, ANIM_FUNC_EXPONENT);
    }
    else
    {
        AnimColor(g_rows[ring], 0.03, 0.18, 0.36, 0.0, fade_time, 0.0, ANIM_FUNC_EXPONENT);
    }
}

/* Shows the buttons of g_pad_help in the pad help. */
update_pad_help()
{
    SetSystemParam(SYS_PADHELP, g_pad_help);
}

/* Draws the label of item index into its row plane and notes whether its link is enabled. */
load_row_label(index)
{
    new ring = (index + LABEL_RING) % LABEL_RING;
    new row = g_rows[ring];
    new label[64];
    if (index == g_select)
    {
        domGetData(g_select_node, "label", label, 64);
    }
    else
    {
        get_item_attr(index, "label", label, 64);
    }
    new link[64];
    if (index == g_select)
    {
        domGetData(g_select_node, "link", link, 64);
    }
    else
    {
        get_item_attr(index, "link", link, 64);
    }
    if (!strcmp(link, "disable"))
    {
        g_row_enabled[ring] = 0;
    }
    else
    {
        g_row_enabled[ring] = 1;
    }
    imgDecStringStart(label, 15, 64, 64, floatround(260.0 / 0.6, 0));
    new status;
    for (; ; )
    {
        sleep;
        new held;
        PadState(held, _, _, _, -1);
        count_down_repeat(held);
        status = imgDecSync();
        if (IMG_BUSY != status)
        {
            break;
        }
    }
    if (status == IMG_DONE)
    {
        SetTexture(row, imgTexID(), 0, -1);
        new Float:w = float(imgTexWidth());
        new Float:h = float(imgTexHeight());
        new Float:clip[4] = {2.0, 2.0, 1.0, 1.0};
        new Float:color[4] = {0.0, 0.0, 0.0, 0.371};
        if (w > 260.0)
        {
            w = 260.0;
        }
        clip[0] /= w;
        clip[1] /= h;
        static clip_tag[2] = {0x434C4950, 0}; /* "CLIP", packed */
        SetItemOptionP(row, 0, TAG(clip_tag), clip, 4);
        SetItemOptionP(row, 0, TAG("COLR"), color, 4);
        SetLocalScale(row, w, h, 1.0);
    }
}

/* Fades out the rows and the background, animating the crystals until the fade ends. */
fade_out_labels(Float:duration)
{
    SetSystemParam(SYS_PADHELP, 0);
    fade_background(0, 0, duration);
    for (new i; i < LABEL_ROWS; i++)
    {
        new index = g_top * LABEL_COLUMNS + i;
        hide_row(index, ROW_FLY_BACK, 0.0);
    }
    AnimTimerManager(Self(), 500.0, 0.0, ANIM_FUNC_LINEAR);
    sleep;
    while (AnimGetState(Self(), ANIM_TIMER) == ANIM_RUN)
    {
        animate_rows();
        sleep;
    }
}

/*
 * Updates the scroll bar and turns each crystal along its motion; a crystal shows through its row alpha, and the
 * one of the selected row glows.
 */
animate_rows()
{
    if (g_scroll_bar)
    {
        SetPubVar(g_scroll_bar, "top", float(g_top) / float(g_label_count));
        SetPubVar(g_scroll_bar, "length", float(LABEL_ROWS) / float(g_label_count));
    }
    for (new i; i < LABEL_RING; i++)
    {
        new Float:phase;
        new ring = (g_ring_first * LABEL_COLUMNS + i) % LABEL_RING;
        new row = g_rows[ring];
        new crystal = g_crystals[ring];
        new Float:red;
        new Float:green;
        new Float:blue;
        new Float:alpha;
        GetColor(row, red, green, blue, alpha);
        if (alpha <= 0.02)
        {
            Hide(crystal);
            Hide(row);
            continue;
        }
        else
        {
            SetOptionF(crystal, TAG("FADE"), alpha);
            Show(crystal);
            Show(row);
        }
        if (ANIM_DONE == AnimGetState(row, ANIM_TIMER))
        {
            AnimTimerManager(row, 1000000.0, 0.0, ANIM_FUNC_LINEAR);
            phase = 0.0;
        }
        else
        {
            phase = 36000.0 * Float:AnimGetCurrTime(row, ANIM_TIMER);
        }
        if (AnimGetState(crystal, ANIM_TIMER) == ANIM_RUN)
        {
            new Float:timer = AnimGetCurrTime(crystal, ANIM_TIMER);
            new Float:step = 1000.0 * (timer - g_crystal_time[ring]);
            g_crystal_time[ring] = timer;
            if (step < 0.5)
            {
                AnimTimerManager(crystal, 0.0, 0.0, ANIM_FUNC_LINEAR);
            }
            else
            {
                AnimDoTimer(row, ANIM_TIMER, step);
            }
        }
        {
            new Float:x;
            new Float:y;
            new Float:z;
            new Float:rx;
            new Float:ry;
            new Float:rz;
            new pattern = (ring + g_crystal_seed) % 10;
            get_crystal_pos(pattern, phase, x, y, z);
            get_crystal_rot(pattern, phase, rx, ry, rz);
            AnimModMove(crystal, x, y + (g_ring_first + i) * g_row_height, z);
            SetRot(crystal, rx, ry, rz, 0, 0);
        }
        if (ANIM_DONE == AnimGetState(crystal, ANIM_COLOR))
        {
            GetColor(crystal, red, green, blue, alpha);
            if (alpha > 0.6 && g_ring_first * LABEL_COLUMNS + i == g_select)
            {
                AnimColor(crystal, 1.0, 1.0, 1.0, 0.6, 600.0, 0.0, ANIM_FUNC_ANGULER);
            }
            else if (alpha > 0.0 && g_ring_first * LABEL_COLUMNS + i != g_select)
            {
                AnimColor(crystal, 1.0, 1.0, 1.0, 0.0, 600.0, 0.0, ANIM_FUNC_ANGULER);
            }
        }
    }
}

new g_last_select;

/* Sets the point and the scale of the motions of the crystals for a list of count items. */
set_crystal_layout(count)
{
    count = count >= LABEL_ROWS ? LABEL_ROWS : count;
    if (count <= 3)
    {
        g_crystal_x = 195.0;
        g_crystal_y = 5.0;
        g_crystal_z = 230.0;
        g_crystal_scale = 15.0;
    }
    else if (count == 4)
    {
        g_crystal_x = 195.0;
        g_crystal_y = -5.0;
        g_crystal_z = 220.0;
        g_crystal_scale = 18.0;
    }
    else if (count == 5)
    {
        g_crystal_x = 195.0;
        g_crystal_y = -15.0;
        g_crystal_z = 205.0;
        g_crystal_scale = 20.5;
    }
    else if (count == 6)
    {
        g_crystal_x = 200.0;
        g_crystal_y = -15.0;
        g_crystal_z = 185.0;
        g_crystal_scale = 22.5;
    }
    else if (count == 7)
    {
        g_crystal_x = 202.5;
        g_crystal_y = -15.0;
        g_crystal_z = 170.0;
        g_crystal_scale = 24.0;
    }
    else if (count >= LABEL_ROWS)
    {
        g_crystal_x = 205.0;
        g_crystal_y = -15.0;
        g_crystal_z = 150.0;
        g_crystal_scale = 25.0;
    }
}

/* Returns the sine of an angle in degrees. */
Float:sin_deg(Float:degrees)
{
    return sin(0.017453289 * degrees);
}

/* Gets the position of a crystal on motion pattern 0 to 9 at phase: a sine swing in z around a fixed x. */
get_crystal_pos(pattern, Float:phase, &Float:x, &Float:y, &Float:z)
{
    switch (pattern)
    {
        case 0:
        {
            x = -0.0;
            y = 0.0;
            z = 1.0 + -1.5 * sin_deg(30.0 + 0.1 * phase);
        }
        case 1:
        {
            x = 1.2;
            y = 0.0;
            z = 4.0 + 1.0 * sin_deg(60.0 + 0.2 * phase);
        }
        case 2:
        {
            x = 2.0;
            y = 0.0;
            z = -2.5 * sin_deg(90.0 + -0.1 * phase) - 5.0;
        }
        case 3:
        {
            x = -1.8;
            y = 0.0;
            z = 4.5 + -1.0 * sin_deg(30.0 + -0.08 * phase);
        }
        case 4:
        {
            x = 2.5;
            y = 0.0;
            z = 3.5 + -2.0 * sin_deg(0.0 + 0.1 * phase);
        }
        case 5:
        {
            x = 0.0;
            y = 0.0;
            z = 1.5 + -2.5 * sin_deg(30.0 + 0.1 * phase);
        }
        case 6:
        {
            x = -2.5;
            y = 0.0;
            z = 3.0 * sin_deg(60.0 + 0.2 * phase) - 4.0;
        }
        case 7:
        {
            x = 1.5;
            y = 0.0;
            z = 1.5 + 1.0 * sin_deg(90.0 + -0.1 * phase);
        }
        case 8:
        {
            x = -1.5;
            y = 0.0;
            z = 5.0 + -1.5 * sin_deg(30.0 + -0.08 * phase);
        }
        case 9:
        {
            x = 5.5;
            y = 0.0;
            z = -2.0 * sin_deg(90.0 + 0.1 * phase) - 2.0;
        }
    }
    x = x * g_crystal_scale;
    x = x + g_crystal_x;
    y = y * g_crystal_scale;
    y = y + g_crystal_y;
    z = z * g_crystal_scale;
    z = z + g_crystal_z;
}

/* Gets the rotation in radians of a crystal on motion pattern 0 to 9 at phase: a turn proportional to phase. */
get_crystal_rot(pattern, Float:phase, &Float:rx, &Float:ry, &Float:rz)
{
    switch (pattern)
    {
        case 0:
        {
            rx = 0.05 * phase;
            ry = -0.15 * phase;
            rz = 0.1 * phase;
        }
        case 1:
        {
            rx = 0.05 * phase;
            ry = -0.1 * phase;
            rz = 45.0 + 0.1 * phase;
        }
        case 2:
        {
            rx = 45.0 + -0.1 * phase;
            ry = 0.15 * phase;
            rz = 0.1 * phase;
        }
        case 3:
        {
            rx = 0.1 * phase;
            ry = 45.0 + 0.15 * phase;
            rz = -0.05 * phase;
        }
        case 4:
        {
            rx = 30.0 + 0.1 * phase;
            ry = -0.05 * phase;
            rz = 0.1 * phase;
        }
        case 5:
        {
            rx = 0.1 * phase;
            ry = 0.14 * phase;
            rz = 0.05 * phase;
        }
        case 6:
        {
            rx = 0.12 * phase;
            ry = 0.15 * phase;
            rz = 0.1 * phase;
        }
        case 7:
        {
            rx = 0.1 * phase;
            ry = 45.0 + 0.1 * phase;
            rz = 45.0 + 0.1 * phase;
        }
        case 8:
        {
            rx = 45.0 + -0.1 * phase;
            ry = 0.15 * phase;
            rz = 0.1 * phase;
        }
        case 9:
        {
            rx = 0.1 * phase;
            ry = 45.0 + 0.15 * phase;
            rz = -0.05 * phase;
        }
    }
    rx = 0.017453289 * rx;
    ry = 0.017453289 * ry;
    rz = 0.017453289 * rz;
}

/* Starts the label menu on item select; with fly_in the crystals fly in from the front. */
start_label_menu(select, fly_in)
{
    init_label_menu(select, fly_in);
}

/* Runs a frame of the label menu; returns g_label_result. */
run_label_menu()
{
    new result;
    result = update_label_menu();
    return result;
}

/* Closes the label menu, keeping its selection in g_last_select; returns result. */
end_label_menu(result)
{
    g_last_select = close_label_menu();
    return result;
}

/* Decodes and keeps the four textures of the crystals, the IMG elements tex_crystal0 to tex_crystal3. */
load_crystal_textures()
{
    for (new i; i < 4; i++)
    {
        new id[64];
        new url[64];
        sprintf(id, "tex_crystal%d", i);
        get_tag_attr("IMG", id, "src", url, 64);
        start_image_decode2(url);
        if (finish_image_decode() == IMG_DONE)
        {
            imgKeepTex(imgTexID());
            g_crystal_tex[i] = imgTexID();
        }
    }
}

new g_media_detect_all;

/* Does nothing. */
init_menu()
{
}

/*
 * Sets the disc detection of the menu and reads where the page came from: returns the item to select on return
 * from it, or a RETURN_*.
 */
get_return_item()
{
    new category[32];
    avalGetValue("ChannelCategory", 0, category, 32);
    new into_id[32];
    avalGetValue("ChannelIntoID", 0, into_id, 32);
    new transit[32];
    avalGetValue("ChannelTransit", 0, transit, 32);
    printf("channel transit %s ---------!\n", transit);
    new value[32];
    g_media_detect = get_media_detect(value, 32);
    g_media_detect_all = g_media_detect_all | g_media_detect;
    if (strcmp("BNCH-PSBB", into_id) || !strcmp("OTHER_CHANNEL", transit))
    {
        set_media_detect(g_media_detect_all);
    }
    else if (!strcmp("BNCH-PSBB", into_id))
    {
        new arg[16];
        if (GetArg("discboot", arg, 16) && 0 == atoi(arg))
        {
            set_media_detect(g_media_detect_all);
        }
    }
    if (!strcmp(transit, "NEXT_HIER"))
    {
        return RETURN_NEW_MENU;
    }
    if (!strcmp(transit, "PREV_HIER") || !strcmp(transit, "OTHER_CHANNEL"))
    {
        printf("return from %s\n", into_id);
        new index = find_menu_item_index(into_id);
        if (index < 0)
        {
            return RETURN_NEW_MENU;
        }
        return index;
    }
    if (!strncmp(into_id, "FIRST", 5))
    {
        return RETURN_FIRST;
    }
    if (!strncmp(into_id, "WARN", 4))
    {
        printf("HIER Error!! id = %s\n", into_id);
        return RETURN_ERROR;
    }
    if (!strncmp(into_id, "NONE", 4))
    {
        return RETURN_FIRST;
    }
    return RETURN_FIRST;
    return RETURN_FIRST; /* redundant as written */
}

/* Gives the page the detection mode and the disc type it allows. */
set_media_detect(mode)
{
    new type = filter_disc_type(mode, GetSystemParam(SYS_CDVD_TYPE));
    SetPubVar(Parent(0), "enable_media_detect", mode);
    SetPubVar(Parent(0), "cdvd_media_status", type);
    printf("pre cdvd media status %d ----------\n", type);
}

/*
 * Follows the link of item index: LINK_SUBGROUP (or 8 or 6) opens the MENUGROUP target (returns LINK_SUBGROUP),
 * LINK_PAGE loads the page target, LINK_COMMAND runs the command target, others the built-in top menu.
 */
open_link(index, link_type, target[])
{
    new category[32];
    new into_id[32];
    avalGetValue("ChannelIntoID", 0, into_id, 32);
    avalGetValue("ChannelCategory", 0, category, 32);
    new link_args[64];
    get_named_item_attr(into_id, "linkargs", link_args, 64);
    if (link_type == LINK_SUBGROUP || link_type == 8 || link_type == 6)
    {
        new group[64];
        copy_string(group, target, 64);
        set_menu_return(group, category, into_id);
        avalSet("ChannelCategory", group);
        avalSet("ChannelTransit", "NEXT_HIER");
        return LINK_SUBGROUP;
    }
    if (link_type < 8)
    {
        release_bg_camera(1, 2000.0);
        avalSet("ChannelTransit", "OTHER_CHANNEL");
        sndBgmStop(1);
        SetPubVar(Parent(0), "enable_media_detect", 0);
        xmlLoadSetOptionI(XMLOPT_TIMEOUT, 0);
    }
    switch (link_type)
    {
        case LINK_PAGE:
        {
            xmlLoadStart(target, _, link_args, XMLLOAD_MAIN);
            wait_xml_load();
        }
        case LINK_COMMAND:
        {
            System(target);
        }
        default:
        {
            xmlLoadStart("$$BUILDIN_TOPMENU$$", _, _, XMLLOAD_MAIN);
            wait_xml_load();
        }
    }
    return 0;
}

/*
 * Reads the link of item index into target; returns its LINK_* (for LINK_COMMAND the link names the attribute that
 * holds it).
 */
get_item_link(index, target[], maxlen)
{
    new link_type[1];
    new id[32];
    {
        new link[64];
        get_item_attr(index, "id", id, 32);
        get_item_attr(index, "link", link, 64);
        if (!strcmp(link, "subgroup"))
        {
            link_type[0] = LINK_SUBGROUP;
        }
        else if (!strcmp(link, "href"))
        {
            link_type[0] = LINK_PAGE;
        }
        else if (!strcmp(link, "disable"))
        {
            return LINK_DISABLED;
        }
        get_item_attr(index, link, target, maxlen);
    }
    avalSet("ChannelIntoID", id);
    return link_type[0];
}

/* Records the return to the menu that the current one comes from; returns 1, 0 if it has none, -1 at the top. */
go_back()
{
    new category[32];
    new ret_item[32];
    new ret_group[32];
    avalGetValue("ChannelCategory", 0, category, 32);
    if (!strcmp("GROUP-TOPMENU", category))
    {
        return -1;
    }
    if (!get_menu_return(ret_group, ret_item, 32, 32))
    {
        return 0;
    }
    avalSet("ChannelIntoID", ret_item);
    avalSet("ChannelCategory", ret_group);
    printf("ret hierid %s, group = %s \n", ret_item, ret_group);
    avalSet("ChannelTransit", "PREV_HIER");
    return 1;
}

/* Starts or stops the BGM as the bgm attribute of the menu says, ret being what get_return_item returned. */
update_bgm(ret)
{
    if (ret == RETURN_FIRST)
    {
        sndBgmPlay(1);
        sndBgmPlay(3);
    }
    else if (ret == RETURN_NEW_MENU)
    {
        new bgm[32];
        get_menu_attr("bgm", bgm, 32);
        if (!strcmp("enable", bgm))
        {
            sndBgmPlay(1);
        }
        else if (!strcmp("disable", bgm))
        {
            sndBgmStop(1);
        }
    }
    else if (ret >= 0)
    {
        new bgm[32];
        get_menu_attr("bgm", bgm, 32);
        if (!strcmp("enable", bgm))
        {
            sndBgmPlay(1);
        }
        else if (!strcmp("disable", bgm))
        {
            sndBgmStop(1);
        }
        else if (!strcmp("continue", bgm))
        {
            new transit[32];
            avalGetValue("ChannelTransit", 0, transit, 32);
            if (!strcmp("OTHER_CHANNEL", transit))
            {
                sndBgmPlay(1);
            }
        }
    }
}

public list_active = 1; /* set by the page: 0 while a disc is being started */

/* Shows the menu of ChannelCategory (the logo menu at first) and runs it, then the next one, until the page is left. */
public list_main()
{
    init_status = 0;
    get_screen_size(g_screen_w, g_screen_h);
    g_best_dist = GetPubVar(Camera(0), "best_dist");
    {
        new isp_status = open_isp_list(1, "", 1);
        if (isp_status < 0)
        {
            init_status = init_status | INIT_ISP_FAILED;
        }
    }
    apply_page_args();
    load_crystal_textures();
    init_list();
    restart:
    {
        new category[32];
        if (avalGetValue("ChannelCategory", 0, category, 32) < 0)
        {
            avalSet("ChannelCategory", "GROUP-TOPLOGO");
            avalSet("ChannelTransit", "FIRST");
            avalSet("ChannelIntoID", "BNCH-PSBB");
            avalSet("ChannelOrder", "CENTER");
            avalSet("ChannelType", "icon");
            sprintf(category, "GROUP-TOPLOGO");
        }
        new status = open_menu_group(category);
        if (status < 0)
        {
            printf("WARNING : there is no item in '%s'\n!!!", category);
            avalDelete("ChannelCategory");
            goto restart;
        }
    }
    SetSystemParam(SYS_PADHELP, 0);
    new ret = get_return_item();
    printf("check return %d\n", ret);
    while (ANIM_DONE != AnimGetState(Self(), ANIM_TIMER))
    {
        sleep;
    }
    while (listSize())
    {
        Destroy(listGetAt(0));
        listRemoveAt(0);
    }
    SetPubVar(Parent(0), "enable_media_detect", g_media_detect);
    sleep;
    while (!list_active)
    {
        sleep;
    }
    if (ret == -98) /* redundant as written */
    {
        printf("WARNING::TOP system access DB failed!!\n");
        avalDelete("ChannelCategory");
        goto restart;
    }
    if (ret == RETURN_ERROR)
    {
        printf("WARNING::TOP system can't transit hierarchy!!\n");
        avalDelete("ChannelCategory");
        goto restart;
    }
    reset_first_slot();
    init_menu();
    load_common_bg("bg", 1000.0);
    update_bgm(ret);
    if (g_menu_type == MENU_ICON)
    {
        if (ret == RETURN_FIRST)
        {
            show_intro(0);
            new target[64];
            new index = get_slot_item(0, g_order);
            new link_type = get_item_link(index, target, 64);
            if (link_type > 0)
            {
                fly_into_icon(1000.0);
                link_type = open_link(index, 6, target);
            }
            SendMessage(Camera(0), TAG("RSET"), 0, 0, 0, 0);
            goto restart;
        }
        if (ret == RETURN_NEW_MENU)
        {
            enter_icon_menu(0);
        }
        else if (ret >= 0)
        {
            return_to_icon_menu(ret);
        }
    }
    else if (MENU_LABEL == g_menu_type)
    {
        set_background_poor(600.0, 0);
        new fly_in = 1;
        if (ret == RETURN_NEW_MENU)
        {
            fly_in = 1;
        }
        else if (ret >= 0)
        {
            fly_in = 0;
        }
        start_label_menu(ret, fly_in);
    }
    new result;
    for (; ; )
    {
        while (list_active)
        {
            if (g_menu_type == MENU_ICON)
            {
                result = update_icon_menu();
            }
            else if (MENU_LABEL == g_menu_type)
            {
                result = run_label_menu();
            }
            if (result >= 1)
            {
                goto leave;
            }
            sleep;
        }
        sleep;
    }
    leave:
    if (g_menu_type == MENU_ICON)
    {
        if (result >= ICON_NEXT_MENU)
        {
            goto restart;
        }
    }
    else if (MENU_LABEL == g_menu_type)
    {
        result = end_label_menu(result);
        if (result >= LABEL_OPEN)
        {
            goto restart;
        }
    }
    close_isp_list();
    while (ANIM_DONE != AnimGetState(Self(), ANIM_TIMER))
    {
        sleep;
    }
    Destroy(Scene(0));
    return;
}
