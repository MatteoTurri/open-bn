/*
 * /opt0/bn/script/utility/sysconf.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The system settings: the menu of the groups of settings, the menus of their items with the current values, and
 * the menus of the values, the clock and the time zone, which write the system configuration.
 */

#include <core>
#include <bn>

#pragma dynamic 2560

/* Held for two seconds in the console group, then released, before start shows the versions. */
const SECRET_KEYS = PAD_LEFT | PAD_L2 | PAD_R2 | PAD_L1 | PAD_R1 | PAD_SQUARE;
/* Held on the second controller on the model name, open the settings of the remote control. */
const REMOTE_MENU_KEYS = PAD_SELECT | PAD_START | PAD_L2 | PAD_R2 | PAD_L1 | PAD_R1;

/* The rows the menus show at a time (their argument visible), for the length of the scroll bar. */
const MAIN_MENU_ROWS = 5;
const VALUE_MENU_ROWS = 7;

/* The rows the scroll bar of a group or subgroup menu takes as visible: one more than the MAIN_MENU_ROWS shown. */
const GROUP_BAR_ROWS = 6;

/* As in sysconf.inc. */
const SUBGROUP_NONE = -1;
const SUBGROUP_REMOTE = 0;
const SUBGROUP_DVD_PLAYER = 2;

/* Same as ms_to_frames of wait.inc. */
ms_to_frames(ms)
{
    return ms * FrameRate() / 1000;
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

#include <float>

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

new g_shadow_colors[4] = {0x5F000000, 0x5F000000, 0x5F000000, 0x5F000000}; /* the "SCL4" of the shadowed labels */
new Float:g_screen_width;
new Float:g_screen_height;
new Float:g_screen_depth;
new g_common_bg_name[] = "SCEI:CommonBG";

/* Same as wait_scnf of text.inc. */
wait_scnf()
{
    new status;
    for (; ; )
    {
        status = scnfSync();
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
fade_in_background(Float:fade_time)
{
    new scene = wndGetScene(WND_BG);
    if (scene)
    {
        SendMessage(scene, TAG("STRT"), fade_time, 0, 0, 0);
    }
}

/* Same as fade_background of background.inc. */
fade_background(alpha0, alpha1, Float:fade_time)
{
    new scene = wndGetScene(WND_BG);
    if (scene)
    {
        SendMessage(scene, TAG("FADE"), alpha0, alpha1, fade_time, 0);
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

/* Same as create_focus_plane2 of focus.inc. */
create_focus_plane(script[])
{
    new plane = CreatePlane3x3(Scene(0));
    SetTexture(plane, imgGetResource(RES_FOCUS), 0, -1);
    SetOptionI(plane, TAG("ALPH"), BLEND_ADD);
    AttachScript(plane, _, script);
    return plane;
}

/* Same as kill_object of focus.inc. */
kill_object(obj)
{
    SendMessage(obj, TAG("KILL"), 0, 0, 0, 0);
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

/* Gets the text of a value of an item from its menu of values (sysconf_value_<group>[_<sub>]_<item>). */
get_value_text(group, sub, item, value, text[], maxlen)
{
    new menu_id[64];
    if (sub == SUBGROUP_NONE)
    {
        sprintf(menu_id, "sysconf_value_%d_%d", group, item);
    }
    else
    {
        sprintf(menu_id, "sysconf_value_%d_%d_%d", group, sub, item);
    }
    new node = domFindTag(domRoot(), "menu", menu_id, 0);
    if (node)
    {
        node = domFirstChild(node, "item");
        for (; node && value; node = domNext(node, "item"), value--)
        {
        }
        domGetData(node, "value", text, maxlen);
    }
}

/* Same as get_value_text for the values that are the ids of the items, as the cities of the time zones. */
get_value_text_by_id(group, item, value, text[], maxlen)
{
    new key[64];
    sprintf(key, "sysconf_value_%d_%d", group, item);
    new node = domFindTag(domRoot(), "menu", key, 0);
    if (node)
    {
        sprintf(key, "%d", value);
        node = domFindTag(node, "item", key, 0);
        domGetData(node, "value", text, maxlen);
    }
}

/* As in sysconf.inc. */
const GROUP_CONSOLE = 0;
const GROUP_CLOCK = 1;
const GROUP_SCREEN = 2;
const GROUP_SOUND = 3;
const GROUP_LANGUAGE = 4;
const GROUP_TEXT_INPUT = 5;
const GROUP_KEYBOARD = 6;
const GROUP_MOUSE = 7;
const GROUP_REMOTE = 8;
const CONSOLE_MODEL = 0;
const CONSOLE_MAC_ADDRESS = 1;
const CONSOLE_DVD_PLAYER = 2;
const CONSOLE_PS1_DRIVER = 3;
const CONSOLE_AUTO_DIAG = 4;
const DVD_PROGRESSIVE = 0;
const CLOCK_SET = 0;
const CLOCK_TIME_NOTATION = 1;
const CLOCK_DATE_NOTATION = 2;
const CLOCK_SUMMER_TIME = 3;
const CLOCK_TIME_ZONE = 4;
const SCREEN_SIZE = 0;
const SCREEN_COMPONENT_OUT = 1;
const SOUND_OPTICAL_OUT = 0;
const LANGUAGE_DISPLAY = 0;
const INPUT_KANA_MODE = 0;
const INPUT_KEY_BIND = 1;
const KEYBOARD_TYPE = 0;
const KEYBOARD_REPEAT_WAIT = 1;
const KEYBOARD_REPEAT_SPEED = 2;
const MOUSE_HAND = 0;
const MOUSE_SPEED = 1;
const MOUSE_DOUBLE_CLICK = 2;
const REMOTE_GAME_PLAY = 0;

/* The image of each group, shown beside the menu of the groups. */
new g_group_images[10][8] = {
    "sysconf_img_0", "sysconf_img_1", "sysconf_img_2", "sysconf_img_3", "sysconf_img_4",
    "sysconf_img_5", "sysconf_img_6", "sysconf_img_7", "sysconf_img_8", ""
};

/*
 * Gets the text of a value of an item, of its current value if value is negative; the first four items of the
 * console group show the model, the MAC address and the versions instead.
 */
get_setting_text(group, sub, item, value, text[], maxlen)
{
    if (value < 0)
    {
        value = get_sysconf_item(group, sub, item);
    }
    switch (group)
    {
        case GROUP_CONSOLE:
        {
            if (sub == SUBGROUP_NONE)
            {
                switch (item)
                {
                    case CONSOLE_MODEL:
                    {
                        sverGetModelName(text);
                    }
                    case CONSOLE_MAC_ADDRESS:
                    {
                        sverGetMACAddr(text);
                    }
                    case CONSOLE_DVD_PLAYER:
                    {
                        sverGetDVDVer(text);
                    }
                    case CONSOLE_PS1_DRIVER:
                    {
                        sverGetPS1DrvVer(text);
                    }
                    default:
                    {
                        get_value_text(group, SUBGROUP_NONE, item, value, text, maxlen);
                    }
                }
            }
            else
            {
                get_value_text(group, sub, item, value, text, maxlen);
            }
        }
        case GROUP_CLOCK:
        {
            switch (item)
            {
                case CLOCK_TIME_ZONE:
                {
                    get_value_text_by_id(group, item, value, text, maxlen);
                }
                default:
                {
                    get_value_text(group, SUBGROUP_NONE, item, value, text, maxlen);
                }
            }
        }
        default:
        {
            get_value_text(group, SUBGROUP_NONE, item, value, text, maxlen);
        }
    }
}

/* Same as get_sysconf_item of sysconf.inc. */
get_sysconf_item(group, sub, item)
{
    new notation;
    switch (group)
    {
        case GROUP_CONSOLE:
        {
            switch (sub)
            {
                case SUBGROUP_NONE:
                {
                    switch (item)
                    {
                        case CONSOLE_AUTO_DIAG:
                        {
                            return 1 - scnfGetAutoDiag();
                        }
                    }
                }
                case SUBGROUP_REMOTE:
                {
                    switch (item)
                    {
                        case REMOTE_GAME_PLAY:
                        {
                            if (scnfGetRmGA() == 1)
                            {
                                return 0;
                            }
                            return 1;
                        }
                    }
                }
                case SUBGROUP_DVD_PLAYER:
                {
                    switch (item)
                    {
                        case DVD_PROGRESSIVE:
                        {
                            if (scnfGetDVDProg() == 1)
                            {
                                return 1;
                            }
                            return 0;
                        }
                    }
                }
            }
        }
        case GROUP_CLOCK:
        {
            switch (item)
            {
                case CLOCK_SET:
                {
                    return -1;
                }
                case CLOCK_TIME_NOTATION:
                {
                    return 1 - scnfGetTimeNotation();
                }
                case CLOCK_DATE_NOTATION:
                {
                    if (0 != (notation = scnfGetDateNotation()))
                    {
                        notation = notation ^ 3;
                    }
                    return notation;
                }
                case CLOCK_SUMMER_TIME:
                {
                    return scnfGetSummerTime();
                }
                case CLOCK_TIME_ZONE:
                {
                    return scnfGetTimeZoneCity();
                }
            }
        }
        case GROUP_SCREEN:
        {
            switch (item)
            {
                case SCREEN_SIZE:
                {
                    return scnfGetAspect();
                }
                case SCREEN_COMPONENT_OUT:
                {
                    return 1 - scnfGetVideo();
                }
            }
        }
        case GROUP_SOUND:
        {
            switch (item)
            {
                case SOUND_OPTICAL_OUT:
                {
                    return scnfGetSpdif();
                }
            }
        }
        case GROUP_LANGUAGE:
        {
            switch (item)
            {
                case LANGUAGE_DISPLAY:
                {
                    return scnfGetLang();
                }
            }
        }
        case GROUP_TEXT_INPUT:
        {
            switch (item)
            {
                case INPUT_KANA_MODE:
                {
                    return scnfGetATOKMode();
                }
                case INPUT_KEY_BIND:
                {
                    return scnfGetATOKBind();
                }
            }
        }
        case GROUP_KEYBOARD:
        {
            switch (item)
            {
                case KEYBOARD_TYPE:
                {
                    return scnfGetKBType();
                }
                case KEYBOARD_REPEAT_WAIT:
                {
                    return scnfGetKBRepeatW();
                }
                case KEYBOARD_REPEAT_SPEED:
                {
                    return 2 - scnfGetKBRepeatS();
                }
            }
        }
        case GROUP_MOUSE:
        {
            switch (item)
            {
                case MOUSE_HAND:
                {
                    return scnfGetMSLr();
                }
                case MOUSE_SPEED:
                {
                    return 2 - scnfGetMSSpeed();
                }
                case MOUSE_DOUBLE_CLICK:
                {
                    return 2 - scnfGetMSDblclk();
                }
            }
        }
        case GROUP_REMOTE:
        {
            switch (item)
            {
                case REMOTE_GAME_PLAY:
                {
                    if (scnfGetRmGA() == 1)
                    {
                        return 0;
                    }
                    return 1;
                }
            }
        }
    }
    return -1;
}

/* Same as set_sysconf_item of sysconf.inc. */
set_sysconf_item(group, sub, item, value)
{
    switch (group)
    {
        case GROUP_CONSOLE:
        {
            switch (sub)
            {
                case SUBGROUP_NONE:
                {
                    switch (item)
                    {
                        case CONSOLE_AUTO_DIAG:
                        {
                            return scnfSetAutoDiag(1 - value);
                        }
                    }
                }
                case SUBGROUP_REMOTE:
                {
                    switch (item)
                    {
                        case REMOTE_GAME_PLAY:
                        {
                            return scnfSetRmGA(1 - value);
                        }
                    }
                }
                case SUBGROUP_DVD_PLAYER:
                {
                    switch (item)
                    {
                        case DVD_PROGRESSIVE:
                        {
                            return scnfSetDVDProg(value);
                        }
                    }
                }
            }
        }
        case GROUP_CLOCK:
        {
            switch (item)
            {
                case CLOCK_SET:
                {
                    return -1;
                }
                case CLOCK_TIME_NOTATION:
                {
                    return scnfSetTimeNotation(1 - value);
                }
                case CLOCK_DATE_NOTATION:
                {
                    if (value)
                    {
                        value = value ^ 3;
                    }
                    return scnfSetDateNotation(value);
                }
                case CLOCK_SUMMER_TIME:
                {
                    return scnfSetSummerTime(value);
                }
                case CLOCK_TIME_ZONE:
                {
                    return scnfSetTZCityAndVal(value);
                }
            }
        }
        case GROUP_SCREEN:
        {
            switch (item)
            {
                case SCREEN_SIZE:
                {
                    return scnfSetAspect(value);
                }
                case SCREEN_COMPONENT_OUT:
                {
                    return scnfSetVideo(1 - value);
                }
            }
        }
        case GROUP_SOUND:
        {
            switch (item)
            {
                case SOUND_OPTICAL_OUT:
                {
                    return scnfSetSpdif(value);
                }
            }
        }
        case GROUP_LANGUAGE:
        {
            switch (item)
            {
                case LANGUAGE_DISPLAY:
                {
                    return scnfSetLang(value);
                }
            }
        }
        case GROUP_TEXT_INPUT:
        {
            switch (item)
            {
                case INPUT_KANA_MODE:
                {
                    return scnfSetATOKMode(value);
                }
                case INPUT_KEY_BIND:
                {
                    return scnfSetATOKBind(value);
                }
            }
        }
        case GROUP_KEYBOARD:
        {
            switch (item)
            {
                case KEYBOARD_TYPE:
                {
                    return scnfSetKBType(value);
                }
                case KEYBOARD_REPEAT_WAIT:
                {
                    return scnfSetKBRepeatW(value);
                }
                case KEYBOARD_REPEAT_SPEED:
                {
                    return scnfSetKBRepeatS(2 - value);
                }
            }
        }
        case GROUP_MOUSE:
        {
            switch (item)
            {
                case MOUSE_HAND:
                {
                    return scnfSetMSLr(value);
                }
                case MOUSE_SPEED:
                {
                    return scnfSetMSSpeed(2 - value);
                }
                case MOUSE_DOUBLE_CLICK:
                {
                    return scnfSetMSDblclk(2 - value);
                }
            }
        }
        case GROUP_REMOTE:
        {
            switch (item)
            {
                case REMOTE_GAME_PLAY:
                {
                    return scnfSetRmGA(1 - value);
                }
            }
        }
    }
    return -1;
}

/* Same as find_menu_item of sysconf.inc. */
find_menu_item(menu_id[], index)
{
    new node = domFindTag(domRoot(), "menu", menu_id, 0);
    if (node)
    {
        node = domFirstChild(node, "item");
        for (; node && index; node = domNext(node, "item"), index--)
        {
        }
    }
    return node;
}

/* The steps of the key sequence that shows the versions (g_secret_state). */
enum
{
    SECRET_WAIT,       /* waits for SECRET_KEYS */
    SECRET_HELD,       /* SECRET_KEYS held */
    SECRET_RELEASING,  /* the keys are being released */
    SECRET_WAIT_START, /* waits for start */
    SECRET_SHOWN       /* the versions are shown while start is held */
};

new g_secret_state; /* SECRET_* */
new g_secret_time;
new g_secret_keys; /* the part of SECRET_KEYS still held */
new g_version_labels[3];
new g_versions_shown;

/* Shows the three versions of the system, the last one followed by " D" if the default ISP settings are there. */
show_versions(parent, Float:x, Float:y)
{
    new version[32];
    sysInitBNVersion("/opt0/bn/sysconf/tmversion.dat");
    for (new i; i < 3; i++)
    {
        if (sysGetBNVersion(i, version))
        {
            if (i == 2)
            {
                new mode;
                new size;
                if (0 == fileLstat("pfs:/__contents/bn.conf/default_isp.dat", mode, size))
                {
                    append_string(version, " D", 32);
                }
            }
            g_version_labels[i] = CreateLabelShadow(parent);
            SetOptionP(g_version_labels[i], TAG("SCL4"), g_shadow_colors, 4);
            SetOptionI(g_version_labels[i], TAG("SX64"), 2 * 64);
            SetOptionI(g_version_labels[i], TAG("SY64"), 2 * 64);
            SetPos(g_version_labels[i], x, y + 16.0 * i, 0.0, 0);
            SetColor(g_version_labels[i], 1.0, 1.0, 1.0, 0.0);
            SetOptionI(g_version_labels[i], TAG("FONT"), FontID(0));
            SetOptionI(g_version_labels[i], TAG("FSIZ"), 12);
            SetOptionI(g_version_labels[i], TAG("ZSRT"), ZSORT_OVERLAY);
            SetOptionS(g_version_labels[i], TAG("UTF8"), version);
            AnimColor(g_version_labels[i], 1.0, 1.0, 1.0, 1.0, 250.0, 0.0, ANIM_FUNC_LINEAR);
        }
    }
    g_versions_shown = 1;
}

hide_versions()
{
    if (g_versions_shown)
    {
        new i;
        i = 0;
        for (; i < 3; i++)
        {
            if (g_version_labels[i])
            {
                AnimColor(g_version_labels[i], 1.0, 1.0, 1.0, 0.0, 250.0, 0.0, ANIM_FUNC_LINEAR);
            }
        }
        wait_ms(250);
        i = 0;
        for (; i < 3; i++)
        {
            if (g_version_labels[i])
            {
                Destroy(g_version_labels[i]);
                g_version_labels[i] = 0;
            }
        }
        g_secret_state = SECRET_WAIT;
        g_versions_shown = 0;
    }
}

main()
{
    new scroll_pos = -1;
    new group_count = 9; /* 8 without the remote control */
    if (0 == scnfGetRmSupport() || 0 == scnfGetRmSysMenu())
    {
        new node = domFindTag(domRoot(), "menu", "sysconf_menu", 0);
        if (node)
        {
            node = domLastChild(node, "");
        }
        if (node)
        {
            domRemoveTag(node);
        }
        group_count = 8;
    }
    start_top_page(0);
    if (scnfRead() < 0)
    {
        return;
    }
    scnfSetSystemOption();
    load_common_bg("bg", 1000.0);
    set_background_poor(600.0, 0);
    SetPos(Camera(0), 0.0, 0.0, 300.0, 0);
    LookAt(Camera(0), Scene(0), 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
    new Float:screen_w;
    new Float:screen_h;
    get_screen_size(screen_w, screen_h);
    new focus = create_focus_plane("focus");
    SetOptionI(focus, TAG("ZSRT"), 1); /* acts as ZSORT_NONE */
    SetOptionF(focus, TAG("ZOFS"), -1.0);
    new window;
    window = CreatePlane(Disp2D(0));
    SetPos(window, 0.671875 * screen_w, 0.5 * screen_h, 0.0, 0);
    SetColor(window, 1.0, 1.0, 1.0, 0.0);
    SetLocalScale(window, 380.0, 380.0, 1.0);
    SetOptionI(window, TAG("ZSRT"), ZSORT_BACKGROUND);
    SetOptionI(window, TAG("ALPH"), BLEND_ADD);
    AttachScript(window, _, "window");
    new scroll_bar;
    scroll_bar = CreateNull(window);
    SetPos(scroll_bar, -400.0, 25.5, 0.0, 0);
    SetOptionI(scroll_bar, TAG("ZSRT"), ZSORT_OVERLAY);
    AttachScript(scroll_bar, _, "scroll_bar");
    SetPubVar(scroll_bar, "manager", Self());
    SetPubVar(scroll_bar, "focus", focus);
    SetPubVar(scroll_bar, "height", 176.0);
    SendMessage(scroll_bar, TAG("FADE"), 0, 0, 0, 0);
    new title;
    {
        new Float:x;
        new Float:y;
        title = CreateLabelShadow(window);
        SetOptionP(title, TAG("SCL4"), g_shadow_colors, 4);
        SetOptionI(title, TAG("SX64"), 2 * 64);
        SetOptionI(title, TAG("SY64"), 2 * 64);
        get_list_title_pos(0, 5, 1, x, y);
        SetPos(title, screen_w * (x - 0.671875), screen_h * (y - 0.5), 0.0, 0);
        SetOptionI(title, TAG("FONT"), FontID(0));
        SetOptionI(title, TAG("ZSRT"), ZSORT_OVERLAY);
        SetOptionI(title, TAG("FSIZ"), 17);
        SetOptionI(title, TAG("HALN"), ALIGN_CENTER);
        SetOptionI(title, TAG("VALN"), ALIGN_CENTER);
        AttachScript(title, _, "title");
    }
    new menu = create_main_menu(title, focus);
    {
        new text[64];
        get_tag_attr("TEXT", "sysconf", "value", text, 64);
        SendMessageStr(title, TAG("UTF8"), text);
    }
    SendMessage(menu, TAG("FADE"), 1.0, float(400), 0, 0);
    {
        new pos = GetPubVar(menu, "scroll_pos");
        SetPubVar(scroll_bar, "top", float(pos) / group_count);
        SetPubVar(scroll_bar, "length", float(MAIN_MENU_ROWS) / group_count);
        SendMessage(scroll_bar, TAG("FADE"), 1.0, float(400), 0, 0);
    }
    wait_ms(400);
    wait_scnf();
    new active = menu; /* the menu while it has the focus, 0 while a group runs */
    SendMessage(active, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
    for (new shown_group = -1; ; )
    {
        {
            new pos = GetPubVar(menu, "scroll_pos");
            if (scroll_pos != pos)
            {
                scroll_pos = pos;
                SetPubVar(scroll_bar, "top", float(pos) / group_count);
                SetPubVar(scroll_bar, "length", float(MAIN_MENU_ROWS) / group_count);
            }
        }
        {
            new cursor = GetPubVar(menu, "cursor");
            if (shown_group != cursor)
            {
                new url[256];
                get_tag_attr("IMG", g_group_images[cursor], "src", url, 256);
                SendMessageStr(window, TAG("IMAG"), url);
                shown_group = cursor;
            }
        }
        new msg;
        new a;
        new b;
        new c;
        /* From the menu: "FOCS" and "CHNG" are answered at once; "PUSH": a is the index, b the button. */
        if (RecvMessage(msg, a, b, c))
        {
            if (active == menu)
            {
                if (msg == TAG("FOCS"))
                {
                    SendMessage(active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                }
                else if (msg == TAG("CHNG"))
                {
                    SendMessage(active, TAG("CHNG"), 0, 0, 0, 0);
                }
                else if (msg == TAG("PUSH"))
                {
                    if (b == TAG("RRGT"))
                    {
                        sndSePlay(SE_OPEN);
                        active = 0;
                        send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                        SendMessage(menu, TAG("FADE"), 0, float(300), 0, 0);
                        SendMessageStr(title, TAG("UTF8"), "");
                        wait_ms(300);
                        run_group(a, scroll_bar, focus, window);
                        SendMessage(menu, TAG("FADE"), 1.0, float(400), 0, 0);
                        {
                            new text[64];
                            get_tag_attr("TEXT", "sysconf", "value", text, 64);
                            SendMessageStr(title, TAG("UTF8"), text);
                        }
                        new pos = GetPubVar(menu, "scroll_pos");
                        SetPubVar(scroll_bar, "top", float(pos) / group_count);
                        SetPubVar(scroll_bar, "length", float(MAIN_MENU_ROWS) / group_count);
                        SendMessage(scroll_bar, TAG("FADE"), 1.0, float(400), 0, 0);
                        wait_ms(400);
                        active = menu;
                        SendMessage(active, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
                    }
                    else if (b == TAG("RDWN"))
                    {
                        sndSePlay(SE_CANCEL);
                        break;
                    }
                    SendMessage(active, TAG("PUSH"), 0, 0, 0, 0);
                }
            }
        }
        sleep;
    }
    kill_object(focus);
    SendMessage(menu, TAG("FADE"), 0, float(300), 0, 0);
    SendMessage(scroll_bar, TAG("FADE"), 0, float(300), 0, 0);
    SendMessage(title, TAG("FADE"), 0, float(300), 0, 0);
    SendMessage(window, TAG("FADE"), 0, float(300), 0, 0);
    wait_ms(300);
    Destroy(window);
    fade_background(0, 0, 600.0);
    {
        new url[64];
        get_tag_attr("LINK", "top", "href", url, 64);
        xmlLoadStart(url, "", _, XMLLOAD_MAIN);
        wait_xml_load();
    }
    Destroy(Self());
}

/*
 * Runs the menu of the items of a group until the cross: the circle opens the menu of the values of an item,
 * the clock or the time zone. In the console group the triangle on the version of the DVD player opens its
 * subgroup, and SECRET_KEYS and REMOTE_MENU_KEYS work.
 */
run_group(group, scroll_bar, focus, window)
{
    new scroll_pos = -1;
    if (GROUP_CONSOLE == group)
    {
        SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
    }
    new title;
    title = CreateLabelShadow(window);
    SetOptionP(title, TAG("SCL4"), g_shadow_colors, 4);
    SetOptionI(title, TAG("SX64"), 2 * 64);
    SetOptionI(title, TAG("SY64"), 2 * 64);
    SetPos(title, -184.0, -78.0, 0.0, 0);
    SetOptionI(title, TAG("FONT"), FontID(0));
    SetOptionI(title, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(title, TAG("FSIZ"), 17);
    SetOptionI(title, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(title, TAG("VALN"), ALIGN_CENTER);
    AttachScript(title, _, "title");
    new count;
    new menu = create_group_menu(group, title, focus, count);
    new clock;
    if (group == GROUP_CLOCK)
    {
        clock = CreateNull(title);
        SetPos(clock, 102.0, 44.0, 0.0, 0);
        AttachScript(clock, _, "clock");
    }
    {
        new text[64];
        domGetData(find_menu_item("sysconf_menu", group), "value", text, 64);
        SendMessageStr(title, TAG("UTF8"), text);
    }
    SendMessage(menu, TAG("FADE"), 1.0, float(400), 0, 0);
    if (clock)
    {
        SendMessage(clock, TAG("FADE"), 1.0, float(400), 0, 0);
    }
    if (count > GROUP_BAR_ROWS) /* NOTE: bug: the menu of create_group_menu shows 5 rows, not 6 */
    {
        new pos = GetPubVar(menu, "scroll_pos");
        SetPubVar(scroll_bar, "top", float(pos) / count);
        SetPubVar(scroll_bar, "length", float(GROUP_BAR_ROWS) / count);
        SendMessage(scroll_bar, TAG("FADE"), 1.0, float(400), 0, 0);
    }
    else
    {
        SendMessage(scroll_bar, TAG("FADE"), 0, float(400), 0, 0);
    }
    wait_ms(400);
    new active = menu;
    SendMessage(active, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
    for (; ; )
    {
        if (count > GROUP_BAR_ROWS)
        {
            new pos = GetPubVar(menu, "scroll_pos");
            if (scroll_pos != pos)
            {
                scroll_pos = pos;
                SetPubVar(scroll_bar, "top", float(pos) / count);
                SetPubVar(scroll_bar, "length", float(GROUP_BAR_ROWS) / count);
            }
        }
        if (GROUP_CONSOLE == group)
        {
            new held;
            PadState(held, _, _, _, -1);
            switch (g_secret_state)
            {
                case SECRET_WAIT:
                {
                    if (held == SECRET_KEYS)
                    {
                        g_secret_time = tickcount();
                        g_secret_state++;
                    }
                }
                case SECRET_HELD:
                {
                    if (SECRET_KEYS != held)
                    {
                        if (tickcount() - g_secret_time >= 2000 && !(held & ~SECRET_KEYS))
                        {
                            g_secret_keys = held;
                            g_secret_state++;
                        }
                        else
                        {
                            g_secret_state = SECRET_WAIT;
                        }
                    }
                }
                case SECRET_RELEASING:
                {
                    if (~g_secret_keys & held)
                    {
                        g_secret_state = SECRET_WAIT;
                    }
                    else
                    {
                        if (!(g_secret_keys = held))
                        {
                            g_secret_state++;
                        }
                    }
                }
                case SECRET_WAIT_START:
                {
                    if (held == PAD_START)
                    {
                        show_versions(window, 80.0, 128.0);
                        g_secret_state++;
                    }
                    else if (held)
                    {
                        g_secret_state = SECRET_WAIT;
                    }
                }
                case SECRET_SHOWN:
                {
                    if (PAD_START != held)
                    {
                        hide_versions();
                        g_secret_state = SECRET_WAIT;
                    }
                }
            }
        }
        switch (group)
        {
            case GROUP_CONSOLE:
            {
                new held;
                new keys = REMOTE_MENU_KEYS;
                PadState(held, _, _, _, 1);
                if (held == keys && scnfGetRmSupport() == 1 && scnfGetRmMenuCMD() == 1)
                {
                    new selected = GetPubVar(menu, "cursor");
                    if (CONSOLE_MODEL == selected)
                    {
                        new saved_cursor;
                        sndSePlay(SE_OPEN);
                        active = 0;
                        send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                        SendMessage(menu, TAG("FADE"), 0, float(300), 0, 0);
                        saved_cursor = GetPubVar(menu, "cursor");
                        TimerDie(menu, ms_to_frames(300));
                        menu = 0;
                        if (clock)
                        {
                            SendMessage(clock, TAG("FADE"), 0, float(300), 0, 0);
                            TimerDie(clock, ms_to_frames(300));
                            clock = 0;
                        }
                        SendMessageStr(title, TAG("UTF8"), "");
                        wait_ms(300);
                        SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
                        run_subgroup(group, SUBGROUP_REMOTE, scroll_bar, focus, window);
                        SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
                        menu = create_group_menu(group, title, focus, count);
                        SetPubVar(menu, "cursor", saved_cursor);
                        SendMessage(menu, TAG("FADE"), 1.0, float(400), 0, 0);
                        if (group == GROUP_CLOCK)
                        {
                            clock = CreateNull(title);
                            SetPos(clock, 102.0, 44.0, 0.0, 0);
                            AttachScript(clock, _, "clock");
                            SendMessage(clock, TAG("FADE"), 1.0, float(400), 0, 0);
                        }
                        {
                            new text[64];
                            domGetData(find_menu_item("sysconf_menu", group), "value", text, 64);
                            SendMessageStr(title, TAG("UTF8"), text);
                        }
                        if (count > GROUP_BAR_ROWS)
                        {
                            new pos = GetPubVar(menu, "scroll_pos");
                            SetPubVar(scroll_bar, "top", float(pos) / count);
                            SetPubVar(scroll_bar, "length", float(GROUP_BAR_ROWS) / count);
                            SendMessage(scroll_bar, TAG("FADE"), 1.0, float(400), 0, 0);
                        }
                        else
                        {
                            SendMessage(scroll_bar, TAG("FADE"), 0, float(400), 0, 0);
                        }
                        wait_ms(400);
                        active = menu;
                        SendMessage(active, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
                        sleep;
                        continue;
                    }
                }
            }
        }
        new msg;
        new a;
        new b;
        new c;
        /* From the menu: "FOCS" and "CHNG" are answered at once; "PUSH": a is the index, b the button. */
        if (RecvMessage(msg, a, b, c))
        {
            if (active == menu)
            {
                if (msg == TAG("FOCS"))
                {
                    SendMessage(active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                }
                else if (msg == TAG("CHNG"))
                {
                    SendMessage(active, TAG("CHNG"), 0, 0, 0, 0);
                }
                else if (msg == TAG("PUSH"))
                {
                    if (b == TAG("RRGT"))
                    {
                        new saved_cursor;
                        sndSePlay(SE_OPEN);
                        active = 0;
                        send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                        SendMessage(menu, TAG("FADE"), 0, float(300), 0, 0);
                        saved_cursor = GetPubVar(menu, "cursor");
                        TimerDie(menu, ms_to_frames(300));
                        menu = 0;
                        if (clock)
                        {
                            SendMessage(clock, TAG("FADE"), 0, float(300), 0, 0);
                            TimerDie(clock, ms_to_frames(300));
                            clock = 0;
                        }
                        SendMessageStr(title, TAG("UTF8"), "");
                        wait_ms(300);
                        if (GROUP_CONSOLE == group)
                        {
                            SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
                        }
                        switch (group)
                        {
                            case GROUP_CONSOLE:
                            {
                                switch (a)
                                {
                                    case CONSOLE_AUTO_DIAG:
                                    {
                                        run_value_menu(group, a, scroll_bar, focus, title);
                                    }
                                    default:
                                    {
                                        sleep;
                                    }
                                }
                            }
                            case GROUP_CLOCK:
                            {
                                switch (a)
                                {
                                    case CLOCK_SET:
                                    {
                                        run_clock_set(group, a, scroll_bar, focus, title);
                                    }
                                    case CLOCK_TIME_ZONE:
                                    {
                                        run_timezone_menu(group, a, scroll_bar, focus, title);
                                    }
                                    default:
                                    {
                                        run_value_menu(group, a, scroll_bar, focus, title);
                                    }
                                }
                            }
                            case GROUP_SCREEN:
                            {
                                switch (a)
                                {
                                    case SCREEN_SIZE:
                                    {
                                        sleep;
                                    }
                                    default:
                                    {
                                        run_value_menu(group, a, scroll_bar, focus, title);
                                    }
                                }
                            }
                            case GROUP_LANGUAGE:
                            {
                                switch (a)
                                {
                                    case LANGUAGE_DISPLAY:
                                    {
                                        wait_ms(300);
                                    }
                                    default:
                                    {
                                        run_value_menu(group, a, scroll_bar, focus, title);
                                    }
                                }
                            }
                            default:
                            {
                                run_value_menu(group, a, scroll_bar, focus, title);
                            }
                        }
                        if (GROUP_CONSOLE == group)
                        {
                            SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
                        }
                        menu = create_group_menu(group, title, focus, count);
                        SetPubVar(menu, "cursor", saved_cursor);
                        SendMessage(menu, TAG("FADE"), 1.0, float(400), 0, 0);
                        if (group == GROUP_CLOCK)
                        {
                            clock = CreateNull(title);
                            SetPos(clock, 102.0, 44.0, 0.0, 0);
                            AttachScript(clock, _, "clock");
                            SendMessage(clock, TAG("FADE"), 1.0, float(400), 0, 0);
                        }
                        {
                            new text[64];
                            domGetData(find_menu_item("sysconf_menu", group), "value", text, 64);
                            SendMessageStr(title, TAG("UTF8"), text);
                        }
                        if (count > GROUP_BAR_ROWS)
                        {
                            new pos = GetPubVar(menu, "scroll_pos");
                            SetPubVar(scroll_bar, "top", float(pos) / count);
                            SetPubVar(scroll_bar, "length", float(GROUP_BAR_ROWS) / count);
                            SendMessage(scroll_bar, TAG("FADE"), 1.0, float(400), 0, 0);
                        }
                        else
                        {
                            SendMessage(scroll_bar, TAG("FADE"), 0, float(400), 0, 0);
                        }
                        wait_ms(400);
                        active = menu;
                        SendMessage(active, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
                    }
                    else if (b == TAG("RDWN"))
                    {
                        sndSePlay(SE_CANCEL);
                        break;
                    }
                    else if (b == TAG("RUP"))
                    {
                        switch (group)
                        {
                            case GROUP_CONSOLE:
                            {
                                switch (a)
                                {
                                    case CONSOLE_DVD_PLAYER:
                                    {
                                        if (0 == scnfGetDVDProg())
                                        {
                                            SendMessage(active, TAG("PUSH"), 0, 0, 0, 0);
                                            sleep;
                                            continue;
                                        }
                                        new saved_cursor;
                                        sndSePlay(SE_OPEN);
                                        active = 0;
                                        send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                                        SendMessage(menu, TAG("FADE"), 0, float(300), 0, 0);
                                        saved_cursor = GetPubVar(menu, "cursor");
                                        TimerDie(menu, ms_to_frames(300));
                                        menu = 0;
                                        if (clock)
                                        {
                                            SendMessage(clock, TAG("FADE"), 0, float(300), 0, 0);
                                            TimerDie(clock, ms_to_frames(300));
                                            clock = 0;
                                        }
                                        SendMessageStr(title, TAG("UTF8"), "");
                                        wait_ms(300);
                                        SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
                                        run_subgroup(group, a, scroll_bar, focus, window);
                                        SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
                                        menu = create_group_menu(group, title, focus, count);
                                        SetPubVar(menu, "cursor", saved_cursor);
                                        SendMessage(menu, TAG("FADE"), 1.0, float(400), 0, 0);
                                        if (group == GROUP_CLOCK)
                                        {
                                            clock = CreateNull(title);
                                            SetPos(clock, 102.0, 44.0, 0.0, 0);
                                            AttachScript(clock, _, "clock");
                                            SendMessage(clock, TAG("FADE"), 1.0, float(400), 0, 0);
                                        }
                                        {
                                            new text[64];
                                            domGetData(find_menu_item("sysconf_menu", group), "value", text, 64);
                                            SendMessageStr(title, TAG("UTF8"), text);
                                        }
                                        if (count > GROUP_BAR_ROWS)
                                        {
                                            new pos = GetPubVar(menu, "scroll_pos");
                                            SetPubVar(scroll_bar, "top", float(pos) / count);
                                            SetPubVar(scroll_bar, "length", float(GROUP_BAR_ROWS) / count);
                                            SendMessage(scroll_bar, TAG("FADE"), 1.0, float(400), 0, 0);
                                        }
                                        else
                                        {
                                            SendMessage(scroll_bar, TAG("FADE"), 0, float(400), 0, 0);
                                        }
                                        wait_ms(400);
                                        active = menu;
                                        SendMessage(active, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
                                        sleep;
                                        continue;
                                    }
                                    default:
                                    {
                                        SendMessage(active, TAG("PUSH"), 0, 0, 0, 0);
                                    }
                                }
                            }
                            default:
                            {
                                SendMessage(active, TAG("PUSH"), 0, 0, 0, 0);
                            }
                        }
                    }
                    else
                    {
                        SendMessage(active, TAG("PUSH"), 0, 0, 0, 0);
                    }
                }
            }
        }
        sleep;
    }
    active = 0;
    send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    SendMessage(title, TAG("FADE"), 0, float(300), 0, 0);
    if (clock)
    {
        SendMessage(clock, TAG("FADE"), 0, float(300), 0, 0);
    }
    SendMessage(menu, TAG("FADE"), 0, float(300), 0, 0);
    hide_versions();
    wait_ms(300);
    if (GROUP_CONSOLE == group)
    {
        SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
    }
    Destroy(title);
}

/* Runs the menu of the items of a subgroup until the cross: the circle opens the menu of the values of an item. */
run_subgroup(group, sub, scroll_bar, focus, window)
{
    new scroll_pos = -1;
    new title;
    title = CreateLabelShadow(window);
    SetOptionP(title, TAG("SCL4"), g_shadow_colors, 4);
    SetOptionI(title, TAG("SX64"), 2 * 64);
    SetOptionI(title, TAG("SY64"), 2 * 64);
    SetPos(title, -184.0, -78.0, 0.0, 0);
    SetOptionI(title, TAG("FONT"), FontID(0));
    SetOptionI(title, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(title, TAG("FSIZ"), 17);
    SetOptionI(title, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(title, TAG("VALN"), ALIGN_CENTER);
    AttachScript(title, _, "title");
    new count;
    new menu = create_subgroup_menu(group, sub, title, focus, count);
    {
        new text[64];
        domGetData(find_menu_item("sysconf_menu", group), "value", text, 64);
        SendMessageStr(title, TAG("UTF8"), text);
    }
    SendMessage(menu, TAG("FADE"), 1.0, float(400), 0, 0);
    if (count > GROUP_BAR_ROWS) /* NOTE: bug: the menu of create_subgroup_menu shows 5 rows, not 6 */
    {
        new pos = GetPubVar(menu, "scroll_pos");
        SetPubVar(scroll_bar, "top", float(pos) / count);
        SetPubVar(scroll_bar, "length", float(GROUP_BAR_ROWS) / count);
        SendMessage(scroll_bar, TAG("FADE"), 1.0, float(400), 0, 0);
    }
    else
    {
        SendMessage(scroll_bar, TAG("FADE"), 0, float(400), 0, 0);
    }
    wait_ms(400);
    new active = menu;
    SendMessage(active, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
    for (; ; )
    {
        if (count > GROUP_BAR_ROWS)
        {
            new pos = GetPubVar(menu, "scroll_pos");
            if (scroll_pos != pos)
            {
                scroll_pos = pos;
                SetPubVar(scroll_bar, "top", float(pos) / count);
                SetPubVar(scroll_bar, "length", float(GROUP_BAR_ROWS) / count);
            }
        }
        new msg;
        new a;
        new b;
        new c;
        /* From the menu: "FOCS" and "CHNG" are answered at once; "PUSH": a is the index, b the button. */
        if (RecvMessage(msg, a, b, c))
        {
            if (active == menu)
            {
                if (msg == TAG("FOCS"))
                {
                    SendMessage(active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                }
                else if (msg == TAG("CHNG"))
                {
                    SendMessage(active, TAG("CHNG"), 0, 0, 0, 0);
                }
                else if (msg == TAG("PUSH"))
                {
                    if (b == TAG("RRGT"))
                    {
                        new saved_cursor;
                        sndSePlay(SE_OPEN);
                        active = 0;
                        send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                        SendMessage(menu, TAG("FADE"), 0, float(300), 0, 0);
                        saved_cursor = GetPubVar(menu, "cursor");
                        TimerDie(menu, ms_to_frames(300));
                        menu = 0;
                        SendMessageStr(title, TAG("UTF8"), "");
                        wait_ms(300);
                        switch (group)
                        {
                            case GROUP_CONSOLE:
                            {
                                switch (sub)
                                {
                                    case SUBGROUP_REMOTE:
                                    {
                                        switch (a)
                                        {
                                            case REMOTE_GAME_PLAY:
                                            {
                                                run_sub_value_menu(group, sub, a, scroll_bar, focus, title);
                                            }
                                            default:
                                            {
                                                sleep;
                                            }
                                        }
                                    }
                                    default:
                                    {
                                        run_sub_value_menu(group, sub, a, scroll_bar, focus, title);
                                    }
                                }
                            }
                            default:
                            {
                                run_sub_value_menu(group, sub, a, scroll_bar, focus, title);
                            }
                        }
                        menu = create_subgroup_menu(group, sub, title, focus, count);
                        SetPubVar(menu, "cursor", saved_cursor);
                        SendMessage(menu, TAG("FADE"), 1.0, float(400), 0, 0);
                        {
                            new text[64];
                            domGetData(find_menu_item("sysconf_menu", group), "value", text, 64);
                            SendMessageStr(title, TAG("UTF8"), text);
                        }
                        if (count > GROUP_BAR_ROWS)
                        {
                            new pos = GetPubVar(menu, "scroll_pos");
                            SetPubVar(scroll_bar, "top", float(pos) / count);
                            SetPubVar(scroll_bar, "length", float(GROUP_BAR_ROWS) / count);
                            SendMessage(scroll_bar, TAG("FADE"), 1.0, float(400), 0, 0);
                        }
                        else
                        {
                            SendMessage(scroll_bar, TAG("FADE"), 0, float(400), 0, 0);
                        }
                        wait_ms(400);
                        active = menu;
                        SendMessage(active, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
                    }
                    else if (b == TAG("RDWN"))
                    {
                        sndSePlay(SE_CANCEL);
                        break;
                    }
                    else
                    {
                        SendMessage(active, TAG("PUSH"), 0, 0, 0, 0);
                    }
                }
            }
        }
        sleep;
    }
    active = 0;
    send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    SendMessage(title, TAG("FADE"), 0, float(300), 0, 0);
    SendMessage(menu, TAG("FADE"), 0, float(300), 0, 0);
    hide_versions();
    wait_ms(300);
    Destroy(title);
}

/* Runs the menu of the values of an item: the circle writes the value chosen to the system configuration. */
run_value_menu(group, item, scroll_bar, focus, title)
{
    new scroll_pos = -1;
    new changed;
    new count;
    new menu = create_value_menu(group, item, title, focus, count);
    {
        new menu_id[16];
        sprintf(menu_id, "sysconf_menu_%d", group);
        new text[64];
        domGetData(find_menu_item(menu_id, item), "value", text, 64);
        SendMessageStr(title, TAG("UTF8"), text);
    }
    SendMessage(menu, TAG("FADE"), 1.0, float(400), 0, 0);
    if (count > VALUE_MENU_ROWS)
    {
        new pos = GetPubVar(menu, "scroll_pos");
        SetPubVar(scroll_bar, "top", float(pos) / count);
        SetPubVar(scroll_bar, "length", float(VALUE_MENU_ROWS) / count);
        SendMessage(scroll_bar, TAG("FADE"), 1.0, float(400), 0, 0);
    }
    else
    {
        SendMessage(scroll_bar, TAG("FADE"), 0, float(400), 0, 0);
    }
    wait_ms(400);
    new active = menu;
    SendMessage(active, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
    for (; ; )
    {
        if (count > VALUE_MENU_ROWS)
        {
            new pos = GetPubVar(menu, "scroll_pos");
            if (scroll_pos != pos)
            {
                scroll_pos = pos;
                SetPubVar(scroll_bar, "top", float(pos) / count);
                SetPubVar(scroll_bar, "length", float(VALUE_MENU_ROWS) / count);
            }
        }
        new msg;
        new a;
        new b;
        new c;
        /* From the menu: "FOCS" and "CHNG" are answered at once; "PUSH": a is the index, b the button. */
        if (RecvMessage(msg, a, b, c))
        {
            if (active == menu)
            {
                if (msg == TAG("FOCS"))
                {
                    SendMessage(active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                }
                else if (msg == TAG("CHNG"))
                {
                    SendMessage(active, TAG("CHNG"), 0, 0, 0, 0);
                }
                else if (msg == TAG("PUSH"))
                {
                    if (b == TAG("RRGT"))
                    {
                        sndSePlay(SE_DECIDE);
                        set_sysconf_item(group, SUBGROUP_NONE, item, a);
                        scnfWrite();
                        scnfSetSystemOption();
                        changed = 1;
                        break;
                    }
                    else if (b == TAG("RDWN"))
                    {
                        sndSePlay(SE_CANCEL);
                        break;
                    }
                    SendMessage(active, TAG("PUSH"), 0, 0, 0, 0);
                }
            }
        }
        sleep;
    }
    active = 0;
    send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    SendMessageStr(title, TAG("UTF8"), "");
    SendMessage(menu, TAG("FADE"), 0, float(300), 0, 0);
    wait_ms(300);
    if (changed)
    {
        wait_scnf();
    }
    Destroy(menu);
}

/* Same as run_value_menu for an item of a subgroup. */
run_sub_value_menu(group, sub, item, scroll_bar, focus, title)
{
    new scroll_pos = -1;
    new changed;
    new count;
    new menu = create_sub_value_menu(group, sub, item, title, focus, count);
    {
        new menu_id[16];
        sprintf(menu_id, "sysconf_menu_%d_%d", group, sub);
        new text[64];
        domGetData(find_menu_item(menu_id, item), "value", text, 64);
        SendMessageStr(title, TAG("UTF8"), text);
    }
    SendMessage(menu, TAG("FADE"), 1.0, float(400), 0, 0);
    if (count > VALUE_MENU_ROWS)
    {
        new pos = GetPubVar(menu, "scroll_pos");
        SetPubVar(scroll_bar, "top", float(pos) / count);
        SetPubVar(scroll_bar, "length", float(VALUE_MENU_ROWS) / count);
        SendMessage(scroll_bar, TAG("FADE"), 1.0, float(400), 0, 0);
    }
    else
    {
        SendMessage(scroll_bar, TAG("FADE"), 0, float(400), 0, 0);
    }
    wait_ms(400);
    new active = menu;
    SendMessage(active, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
    for (; ; )
    {
        if (count > VALUE_MENU_ROWS)
        {
            new pos = GetPubVar(menu, "scroll_pos");
            if (scroll_pos != pos)
            {
                scroll_pos = pos;
                SetPubVar(scroll_bar, "top", float(pos) / count);
                SetPubVar(scroll_bar, "length", float(VALUE_MENU_ROWS) / count);
            }
        }
        new msg;
        new a;
        new b;
        new c;
        /* From the menu: "FOCS" and "CHNG" are answered at once; "PUSH": a is the index, b the button. */
        if (RecvMessage(msg, a, b, c))
        {
            if (active == menu)
            {
                if (msg == TAG("FOCS"))
                {
                    SendMessage(active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                }
                else if (msg == TAG("CHNG"))
                {
                    SendMessage(active, TAG("CHNG"), 0, 0, 0, 0);
                }
                else if (msg == TAG("PUSH"))
                {
                    if (b == TAG("RRGT"))
                    {
                        sndSePlay(SE_DECIDE);
                        set_sysconf_item(group, sub, item, a);
                        scnfWrite();
                        scnfSetSystemOption();
                        changed = 1;
                        break;
                    }
                    else if (b == TAG("RDWN"))
                    {
                        sndSePlay(SE_CANCEL);
                        break;
                    }
                    SendMessage(active, TAG("PUSH"), 0, 0, 0, 0);
                }
            }
        }
        sleep;
    }
    active = 0;
    send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    SendMessageStr(title, TAG("UTF8"), "");
    SendMessage(menu, TAG("FADE"), 0, float(300), 0, 0);
    wait_ms(300);
    if (changed)
    {
        wait_scnf();
    }
    Destroy(menu);
}

/* Same as run_value_menu for the time zone, with the list of the cities. */
run_timezone_menu(group, item, scroll_bar, focus, title)
{
    new scroll_pos = -1;
    new changed;
    new count;
    new menu = create_timezone_menu(group, item, title, focus, count);
    {
        new menu_id[16];
        sprintf(menu_id, "sysconf_menu_%d", group);
        new text[64];
        domGetData(find_menu_item(menu_id, item), "value", text, 64);
        SendMessageStr(title, TAG("UTF8"), text);
    }
    SendMessage(menu, TAG("FADE"), 1.0, float(400), 0, 0);
    wait_ms(400);
    {
        new pos = GetPubVar(menu, "scroll_pos");
        SetPubVar(scroll_bar, "top", float(pos) / count);
        SetPubVar(scroll_bar, "length", float(7) / count); /* NOTE: bug: the time zone menu shows 8 rows */
        SendMessage(scroll_bar, TAG("FADE"), 1.0, float(400), 0, 0);
    }
    new active = menu;
    SendMessage(active, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
    for (; ; )
    {
        {
            new pos = GetPubVar(menu, "scroll_pos");
            if (scroll_pos != pos)
            {
                scroll_pos = pos;
                SetPubVar(scroll_bar, "top", float(pos) / count);
                SetPubVar(scroll_bar, "length", float(7) / count);
            }
        }
        new msg;
        new a;
        new b;
        new c;
        /* From the menu: "FOCS" and "CHNG" are answered at once; "PUSH": a is the index, b the button. */
        if (RecvMessage(msg, a, b, c))
        {
            if (active == menu)
            {
                if (msg == TAG("FOCS"))
                {
                    SendMessage(active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                }
                else if (msg == TAG("CHNG"))
                {
                    SendMessage(active, TAG("CHNG"), 0, 0, 0, 0);
                }
                else if (msg == TAG("PUSH"))
                {
                    if (b == TAG("RRGT"))
                    {
                        sndSePlay(SE_DECIDE);
                        set_sysconf_item(group, SUBGROUP_NONE, item, a);
                        scnfWrite();
                        scnfSetSystemOption();
                        changed = 1;
                        break;
                    }
                    else if (b == TAG("RDWN"))
                    {
                        sndSePlay(SE_CANCEL);
                        break;
                    }
                    SendMessage(active, TAG("PUSH"), 0, 0, 0, 0);
                }
            }
        }
        sleep;
    }
    active = 0;
    send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    SendMessageStr(title, TAG("UTF8"), "");
    SendMessage(menu, TAG("FADE"), 0, float(300), 0, 0);
    wait_ms(300);
    if (changed)
    {
        wait_scnf();
    }
    Destroy(menu);
}

/* Lets the user set the date and the time with clock_set, and writes them. */
run_clock_set(group, item, scroll_bar, focus, title)
{
    new year;
    new month;
    new day;
    new hour;
    new minute;
    new second;
    {
        new menu_id[16];
        sprintf(menu_id, "sysconf_menu_%d", group);
        new text[64];
        domGetData(find_menu_item(menu_id, item), "value", text, 64);
        SendMessageStr(title, TAG("UTF8"), text);
    }
    SendMessage(scroll_bar, TAG("FADE"), 0, float(400), 0, 0);
    new clock_set;
    clock_set = CreateNull(title);
    SetPos(clock_set, -12.0, 44.0, 0.0, 0);
    AttachScript(clock_set, _, "clock_set");
    SetPubVar(clock_set, "manager", Self());
    SetPubVar(clock_set, "focus", focus);
    date(year, month, day, 0);
    time(hour, minute, second, 0);
    SetPubVar(clock_set, "year", year);
    SetPubVar(clock_set, "month", month);
    SetPubVar(clock_set, "day", day);
    SetPubVar(clock_set, "hour", hour);
    SetPubVar(clock_set, "minute", minute);
    SetPubVar(clock_set, "second", 0);
    wait_ms(400);
    while (SCRIPT_FINISHED != ScriptState(clock_set))
    {
        sleep;
    }
    SendMessageStr(title, TAG("UTF8"), "");
    sleep;
    year = GetPubVar(clock_set, "year");
    month = GetPubVar(clock_set, "month");
    day = GetPubVar(clock_set, "day");
    hour = GetPubVar(clock_set, "hour");
    minute = GetPubVar(clock_set, "minute");
    second = GetPubVar(clock_set, "second");
    if (-1 != year)
    {
        printf("%04d/%02d/%02d %02d:%02d:%02d \n", year, month, day, hour, minute, second);
        scnfSetDateTime(year, month, day, hour, minute, second, 0);
    }
    scnfWrite();
    scnfSetSystemOption();
    wait_ms(300);
    wait_scnf();
    Destroy(clock_set);
}

/* Creates the menu of the groups (list), five rows at a time. */
create_main_menu(parent, focus)
{
    new menu = CreateNull(parent);
    new i;
    new args[128];
    new labels[11][64];
    copy_string(args, "interval=28.0;fontsize=15;visible=5;", 128);
    new node = domFindTag(domRoot(), "menu", "sysconf_menu", 0);
    node = domFirstChild(node, "item");
    i = 0;
    for (; node; node = domNext(node, "item"), i++)
    {
        domGetData(node, "value", labels[i], 64);
        new button[64];
        sprintf(button, "btn%d=%s;", i, labels[i]);
        append_string(args, button, 128);
        sleep;
    }
    SetPos(menu, 0.0, 44.0, 0.0, 0);
    SetOptionI(menu, TAG("ZSRT"), ZSORT_OVERLAY);
    AttachScript(menu, _, "list", args);
    SetPubVar(menu, "manager", Self());
    SetPubVar(menu, "focus", focus);
    SetPubVar(menu, "padhelp", PADHELP_CROSS);
    return menu;
}

/* Creates the menu of the items of a group with their values (list2); count gets the number of items. */
create_group_menu(group, parent, focus, &count)
{
    new menu = CreateNull(parent);
    new i;
    new args[128];
    new labels[11][64];
    copy_string(args, "interval=28.0;fontsize=15;visible=5;separate=20.0;", 128);
    new menu_id[64];
    sprintf(menu_id, "sysconf_menu_%d", group);
    new node = domFindTag(domRoot(), "menu", menu_id, 0);
    node = domFirstChild(node, "item");
    i = 0;
    for (; node; node = domNext(node, "item"), i++)
    {
        domGetData(node, "value", labels[i], 64);
        if (!strlen(labels[i]))
        {
            count = i;
            break;
        }
        new setting[64];
        get_setting_text(group, SUBGROUP_NONE, i, -1, setting, 64);
        new row[64];
        sprintf(row, "title%d=%s;setting%d=%s;", i, labels[i], i, setting);
        append_string(args, row, 128);
        sleep;
    }
    SetPos(menu, 0.0, 44.0, 0.0, 0);
    SetOptionI(menu, TAG("ZSRT"), ZSORT_OVERLAY);
    AttachScript(menu, _, "list2", args);
    SetPubVar(menu, "manager", Self());
    SetPubVar(menu, "focus", focus);
    SetPubVar(menu, "padhelp", PADHELP_CROSS);
    if (GROUP_CONSOLE == group)
    {
        SendMessage(menu, TAG("ENBL"), CONSOLE_MODEL, 2, 0, 0);
        SendMessage(menu, TAG("ENBL"), CONSOLE_MAC_ADDRESS, 2, 0, 0);
        SendMessage(menu, TAG("ENBL"), CONSOLE_DVD_PLAYER, 2, 0, 0);
        SendMessage(menu, TAG("ENBL"), CONSOLE_PS1_DRIVER, 2, 0, 0);
        if (scnfGetDVDProg() == 1)
        {
            SendMessage(menu, TAG("ERUP"), CONSOLE_DVD_PLAYER, 1, 0, 0);
        }
    }
    if (group == GROUP_SCREEN)
    {
        SendMessage(menu, TAG("ENBL"), SCREEN_SIZE, 2, 0, 0);
    }
    if (group == GROUP_LANGUAGE)
    {
        SendMessage(menu, TAG("ENBL"), LANGUAGE_DISPLAY, 2, 0, 0);
    }
    return menu;
}

/* Creates the menu of the values of an item (list), on the current one; count gets the number of values. */
create_value_menu(group, item, parent, focus, &count)
{
    new menu = CreateNull(parent);
    new i;
    new args[128];
    new labels[11][64];
    copy_string(args, "interval=28.0;fontsize=15;visible=7;", 128);
    new menu_id[64];
    sprintf(menu_id, "sysconf_value_%d_%d", group, item);
    new node = domFindTag(domRoot(), "menu", menu_id, 0);
    node = domFirstChild(node, "item");
    i = 0;
    for (; node; node = domNext(node, "item"), i++)
    {
        domGetData(node, "value", labels[i], 64);
        if (!strlen(labels[i]))
        {
            count = i;
            break;
        }
        new button[64];
        sprintf(button, "btn%d=%s;", i, labels[i]);
        append_string(args, button, 128);
        sleep;
    }
    SetPos(menu, 0.0, 44.0, 0.0, 0);
    SetOptionI(menu, TAG("ZSRT"), ZSORT_OVERLAY);
    AttachScript(menu, _, "list", args);
    SetPubVar(menu, "manager", Self());
    SetPubVar(menu, "focus", focus);
    SetPubVar(menu, "padhelp", PADHELP_CROSS);
    SetPubVar(menu, "cursor", get_sysconf_item(group, SUBGROUP_NONE, item));
    return menu;
}

/* Same as create_value_menu for an item of a subgroup. */
create_sub_value_menu(group, sub, item, parent, focus, &count)
{
    new menu = CreateNull(parent);
    new i;
    new args[128];
    new labels[11][64];
    copy_string(args, "interval=28.0;fontsize=15;visible=7;", 128);
    new menu_id[64];
    sprintf(menu_id, "sysconf_value_%d_%d_%d", group, sub, item);
    new node = domFindTag(domRoot(), "menu", menu_id, 0);
    node = domFirstChild(node, "item");
    i = 0;
    for (; node; node = domNext(node, "item"), i++)
    {
        domGetData(node, "value", labels[i], 64);
        if (!strlen(labels[i]))
        {
            count = i;
            break;
        }
        new button[64];
        sprintf(button, "btn%d=%s;", i, labels[i]);
        append_string(args, button, 128);
        sleep;
    }
    SetPos(menu, 0.0, 44.0, 0.0, 0);
    SetOptionI(menu, TAG("ZSRT"), ZSORT_OVERLAY);
    AttachScript(menu, _, "list", args);
    SetPubVar(menu, "manager", Self());
    SetPubVar(menu, "focus", focus);
    SetPubVar(menu, "padhelp", PADHELP_CROSS);
    SetPubVar(menu, "cursor", get_sysconf_item(group, sub, item));
    return menu;
}

/* Creates the menu of the time zones (timezone), on the current one; count gets their number. */
create_timezone_menu(group, item, parent, focus, &count)
{
    new menu = CreateNull(parent);
    count = 128;
    SetPos(menu, 0.0, 44.0, 0.0, 0);
    SetOptionI(menu, TAG("ZSRT"), ZSORT_OVERLAY);
    AttachScript(menu, _, "timezone", "visible=8");
    SetPubVar(menu, "manager", Self());
    SetPubVar(menu, "focus", focus);
    SetPubVar(menu, "padhelp", PADHELP_CROSS);
    SetPubVar(menu, "cursor", get_sysconf_item(group, SUBGROUP_NONE, item));
    return menu;
}

/* Same as create_group_menu for a subgroup. */
create_subgroup_menu(group, sub, parent, focus, &count)
{
    new menu = CreateNull(parent);
    new i;
    new args[128];
    new labels[11][64];
    copy_string(args, "interval=28.0;fontsize=15;visible=5;separate=20.0;", 128);
    new menu_id[64];
    sprintf(menu_id, "sysconf_menu_%d_%d", group, sub);
    new node = domFindTag(domRoot(), "menu", menu_id, 0);
    node = domFirstChild(node, "item");
    i = 0;
    for (; node; node = domNext(node, "item"), i++)
    {
        domGetData(node, "value", labels[i], 64);
        if (!strlen(labels[i]))
        {
            count = i;
            break;
        }
        new setting[64];
        get_setting_text(group, sub, i, -1, setting, 64);
        new row[64];
        sprintf(row, "title%d=%s;setting%d=%s;", i, labels[i], i, setting);
        append_string(args, row, 128);
        sleep;
    }
    SetPos(menu, 0.0, 44.0, 0.0, 0);
    SetOptionI(menu, TAG("ZSRT"), ZSORT_OVERLAY);
    AttachScript(menu, _, "list2", args);
    SetPubVar(menu, "manager", Self());
    SetPubVar(menu, "focus", focus);
    SetPubVar(menu, "padhelp", PADHELP_CROSS);
    return menu;
}
