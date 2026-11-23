/*
 * /opt0/bn/script/utility/menu/ps2init.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The first setup of the console, in the window of the system settings: the time zone, the summer time and, when
 * the clock is out of range (argument time=1), the date and the time; then it opens the initial setup page.
 */

#include <core>
#include <bn>

#pragma dynamic 2560

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
    xmlLoadStart(url, g_common_bg_name, "", XMLLOAD_BG);
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

/* Opens the dialog script with its arguments args at the center of the screen and returns its object. */
open_dialog(args[])
{
    new screen_w;
    new screen_h;
    new screen_d;
    wndGetScreen(0, screen_w, screen_h, screen_d);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 0.5 * float(screen_w), 0.5 * float(screen_h), 0.0, 0);
    AttachScript(dialog, _, "dialog", args);
    SetPubVar(dialog, "manager", Self());
    return dialog;
}

/* Shows msg in a dialog and waits until it is closed. */
show_message_dialog(focus, msg[])
{
    new args[128];
    sprintf(args, "msg=%s", msg);
    new dialog = open_dialog(args);
    SetPubVar(dialog, "focus", focus);
    SetPubVar(dialog, "padhelp", PADHELP_CIRCLE);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    Destroy(dialog);
}

new g_group_images[2][8] = {"sysconf_img_1", ""}; /* the image of the window: the one of the date and time */

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
const REMOTE_GAME_PLAY = 0; /* also the item of the remote control subgroup */
/* The subgroups of the console group (sub): the item that opens each, or none. */
const SUBGROUP_NONE = -1;
const SUBGROUP_REMOTE = 0;     /* the remote control: CONSOLE_MODEL */
const SUBGROUP_DVD_PLAYER = 2; /* CONSOLE_DVD_PLAYER: the progressive output */

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

main()
{
    new value[4];
    new set_clock; /* the clock is out of range: the date and the time are set too */
    if (GetArg("time", value, 4))
    {
        if (0 != atoi(value))
        {
            set_clock = 1;
        }
    }
    start_top_page(0);
    if (scnfRead() < 0)
    {
        return;
    }
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
    if (set_clock == 1)
    {
        new msg[64];
        get_tag_attr("TEXT", "msgutil_inittime_outofrange", "value", msg, 64);
        show_message_dialog(focus, msg);
    }
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
    title = CreateLabelShadow(window);
    SetOptionP(title, TAG("SCL4"), g_shadow_colors, 4);
    SetOptionI(title, TAG("SX64"), 2 * 64);
    SetOptionI(title, TAG("SY64"), 2 * 64);
    SetPos(title, -184.0, -78.0, 0.0, 0);
    SetOptionI(title, TAG("FONT"), FontID(0));
    SetOptionI(title, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(title, TAG("FSIZ"), 17);
    SetOptionI(title, TAG("HALN"), ALIGN_CENTER);
    AttachScript(title, _, "title");
    {
        new url[256];
        get_tag_attr("IMG", g_group_images[0], "src", url, 256);
        SendMessageStr(window, TAG("IMAG"), url);
    }
    run_timezone_menu(GROUP_CLOCK, CLOCK_TIME_ZONE, scroll_bar, focus, title);
    SendMessage(scroll_bar, TAG("FADE"), 0, float(300), 0, 0);
    run_value_menu(GROUP_CLOCK, CLOCK_SUMMER_TIME, scroll_bar, focus, title);
    if (set_clock == 1)
    {
        run_clock_set(GROUP_CLOCK, CLOCK_SET, scroll_bar, focus, title);
    }
    scnfWrite();
    scnfSetSystemOption();
    wait_scnf();
    {
        new msg[64];
        get_tag_attr("TEXT", "finish_message", "value", msg, 64);
        show_message_dialog(focus, msg);
    }
    fade_background(0, 0, 600.0);
    {
        new url[64];
        get_tag_attr("LINK", "init_setup", "href", url, 64);
        xmlLoadStart(url, _, "mode=InitialSetup", XMLLOAD_MAIN);
        wait_xml_load();
    }
    Destroy(Self());
}

/* Runs the menu of the values of an item until the circle sets the value chosen (written at the end of the setup). */
run_value_menu(group, item, scroll_bar, focus, title)
{
    new scroll_pos = -1;
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
    if (count > 7)
    {
        new pos = GetPubVar(menu, "scroll_pos");
        SetPubVar(scroll_bar, "top", float(pos) / count);
        SetPubVar(scroll_bar, "length", float(7) / count);
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
        if (count > 7)
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
    Destroy(menu);
}

/* Same as run_value_menu for the time zone, with the list of the cities. */
run_timezone_menu(group, item, scroll_bar, focus, title)
{
    new scroll_pos = -1;
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
        SetPubVar(scroll_bar, "length", float(7) / count);
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
    Destroy(menu);
}

/*
 * Lets the user set the date and the time with clock_set, from 2000/01/01 00:00 if the clock is not within
 * 2000-2030, and writes them (2000/01/01 00:00 if clock_set gives none).
 */
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
    AttachScript(clock_set, _, "clock_set", "ps2init=1");
    SetPubVar(clock_set, "manager", Self());
    SetPubVar(clock_set, "focus", focus);
    date(year, month, day, 0);
    time(hour, minute, second, 0);
    if (year < 2000 || year > 2030)
    {
        year = 2000;
        month = 1;
        day = 1;
        hour = 0;
        minute = 0;
        second = 0;
    }
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
        scnfSetDateTime(year, month, day, hour, minute, second, 0);
    }
    else
    {
        scnfSetDateTime(2000, 1, 1, 0, 0, 0, 0);
    }
    scnfWrite();
    scnfSetSystemOption();
    wait_ms(300);
    wait_scnf();
    Destroy(clock_set);
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
    SetPubVar(menu, "cursor", get_sysconf_item(group, SUBGROUP_NONE, item));
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
    SetPubVar(menu, "cursor", get_sysconf_item(group, SUBGROUP_NONE, item));
    return menu;
}
