/*
 * /opt0/bn/script/utility/browser.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The top page of the file browser: radio buttons for the hard disk and the memory cards, and an album of the save
 * folders of the hard disk (pfs:/__common) or of the memory cards in the ports. Circle opens the thumb page of an
 * item, triangle an option menu, cross goes back. It declares its natives itself instead of including bn.inc.
 */

#pragma dynamic 2560

#include "browser.inc"

native FrameRate();
native swapchars(c);
native Float:float(value);
native Float:floatmul(Float:oper1, Float:oper2);
native floatcmp(Float:fOne, Float:fTwo);
native imgDecSync();
native FindMessage(msg, &a, &b, &c, &d=0);
native strlen(const string[]);
native min(value1, value2);
native xmlLoadSync();
native skbSync();
native domRoot();
native domFindTag(tag, const name[], const value[] = "", deep);
native domGetData(tag, const name[], const value[], maxlength);
native strcmp(const str1[], const str2[]);
native xmlMakeFullPath(const path[], maxlength);
native domFirstChild(tag, const name[]);
native domNext(tag, const name[]);
native domInsertTag(tag, const name[]);
native domAddTag(tag, const name[]);
native domSetData(tag, const name[], const value[]);
native imgDecStart(const url[], req_width, req_height, image_mode, priority, flags);
native datSync();
native Camera(obj);
native GetScreen(obj, &Float:screen_width, &Float:screen_height, &Float:screen_depth);
native Parent(obj);
native Self();
native Disp2D(obj);
native GetOffPos(obj, &Float:x, &Float:y, &Float:z, Float:off_x, Float:off_y, Float:off_z, ref);
native SetSystemParam(id, value);
native hisClear();
native Float:floatadd(Float:dividend, Float:divisor);
native Float:floatsub(Float:oper1, Float:oper2);
native wndGetName(window, const name[], maxlength);
native xmlLoadStart(const url[], const post[] = "", const arg[] = "", target);
native wndGetScene(window);
native SendMessage(obj, msg, {Float,_}:a, {Float,_}:b, {Float,_}:c, {Float,_}:d);
native Scene(obj);
native CreatePlane3x3(parent);
native imgGetResource(id);
native SetTexture(obj, texture, option, tex_mode);
native SetOptionI(obj, option, value);
native AttachScript(obj, const func[] = "", const file[] = "", const arg[] = "");
native GetArg(const name[], const value[], maxlength);
native CreateLabel(parent);
native SetPos(obj, Float:x, Float:y, Float:z, ref);
native SetColor(obj, Float:r, Float:g, Float:b, Float:a);
native AnimColor(obj, Float:r, Float:g, Float:b, Float:a, Float:duration, Float:delay, curve);
native FontID(font);
native SetOptionS(obj, option, const str[]);
native CreateNull(parent);
native SetPubVar(obj, const name[], {Float,_}:value);
native GetPubVar(obj, const name[]);
native RecvMessage(&msg, &a, &b, &c, &d=0);
native Float:floatdiv(Float:dividend, Float:divisor);
native ScriptState(obj);
native SendMessageStr(obj, msg, const str[]);
native Destroy(obj);
native CreatePlane(parent);
native SetLocalScale(obj, Float:x, Float:y, Float:z);
native sprintf(const dest[], const format[], ...);
native RecvMessageStr(&msg, const str[], maxlength);
native printf(const format[], ...);
native CreateLabelShadow(parent);
native SetOptionP(obj, option, const {Float,_}:data[], cells);
native SetRot(obj, Float:x, Float:y, Float:z, ref, order);
native SetScale(obj, Float:x, Float:y, Float:z);
native SetOptionF(obj, option, Float:value);
native CreateDefocus(parent);
native CreateEdit(parent, max_length);
native GetScale(obj, &Float:x, &Float:y, &Float:z);
native skbStart(obj, kind, input_mode, flags, length_unit, max_length, const chars[] = "", allow_only);
native getstring(const str[], maxlength);
native skbGetString(const str[], maxlength);
native imgTexID();
native GetSystemParam(id);
native GetSystemParamStr(id, const value[], maxlength);
native SetSystemParamStr(id, const value[]);
native svdatMCInfo(port, slot, &job);
native svdatSync(&percent, &status, job);
native svdatGetMCInfo(port, slot, &card_type, &free_blocks, &formatted, &status);
native datOpen(type, const name[], order);
native datGetNum();
native svdatCountData(const path[], const skip_folder[], &job);
native floatround(Float:value, method=0);
native avalGetValue(const name[], index, const value[], maxlength);
native atoi(const str[]);
native avalSet(const name[], const value[]);
native sndSePlay(se);
native AnimMoveHermite(obj, Float:x, Float:y, Float:z, Float:duration, Float:delay, curve, Float:velocity_scale);
native datGetData(index, field, const value[], maxlength);
native IsValidObject(obj);
native LookAt(obj, target, Float:at_x, Float:at_y, Float:at_z, up_obj, Float:up_x, Float:up_y, Float:up_z);
native datClose();
native svdatStrTrmSpc(const src[], const dest[], maxlength);
native svdatChkFolderName(const name[]);
native fileLstat(const url[], &st_mode, &file_size);
native svdatCopyFolder(const src[], const dest[], overwrite, &job);
native fileRename(const url[], const name[]);
native fileSync(&percent, &status);
native fileDelete(const url[]);
native svdatMkdir(const path[], &job);
native svdatStringCutByte(max_bytes, const src[], const dest[], maxlength);
native imgKeepTex(texture);

#pragma rational Float
native Float:operator*(Float:oper1, Float:oper2) = floatmul;
native Float:operator/(Float:oper1, Float:oper2) = floatdiv;
native Float:operator+(Float:oper1, Float:oper2) = floatadd;
native Float:operator-(Float:oper1, Float:oper2) = floatsub;

forward Float:get_node_x(node);
forward Float:get_node_y(node);
forward Float:get_node_z(node);

/* As in bn.inc. */
const DAT_FILE_LIST = 0;
const DAT_ORDER_CTCOUNT_DESC = 7;
const SKB_KEYBOARD_DEFAULT = 0;
const SKB_KEYBOARD_HIRAGANA = 1;
const SKB_KEYBOARD_KATAKANA = 2;
const SKB_KEYBOARD_QWERTY = 4;
const SKB_KEYBOARD_NUMBERS = 5;
const SKB_KEYBOARD_ALPHANUMERIC = 7;
const SKB_INPUT_DEFAULT = 0;
const SKB_INPUT_DIRECT = 1;
const SKB_INPUT_KATAKANA = 3;
const SKB_INPUT_HIRAGANA_FIXED = 7;
const SKB_INPUT_KATAKANA_FIXED = 8;
const SKB_INPUT_HALF_KATAKANA_FIXED = 9;
const SKB_LOCK_KEYBOARD = 0x01;
const SKB_LOCK_INPUT_MODE = 0x02;
const SKB_KANA_LOCK_FIXED = 0x10;
const SKB_FIXED_INPUT = 0x20;
const SKB_HALF_WIDTH = 0x40;
const SKB_FULL_WIDTH = 0x80;
const SKB_LENGTH_BYTES = 0;
const PAD_TRIANGLE = 0x1000;
const PAD_CIRCLE = 0x2000;
const PAD_CROSS = 0x4000;
const ANIM_FUNC_LINEAR = 1;
const ANIM_FUNC_EXPONENT = 6;
const ALIGN_LEFT = 0;
const ALIGN_CENTER = 1;
const WND_BG = 2;
const XMLLOAD_MAIN = 0;
const XMLLOAD_BG = 1;
const SYS_PADHELP = 500;
const SYS_ENABLE_NAVIMENU = 602;
const SYS_ALERT_ILLEGAL_DISC = 603;
const SYS_STR_PADHELP_RR = 500;
const SYS_STR_PADHELP_RD = 501;
const SYS_STR_PADHELP_RU = 502;
const SYS_STR_PADHELP_RL = 503;
const PADHELP_CIRCLE = 1;
const PADHELP_CROSS = 2;
const PADHELP_TRIANGLE = 4;
const BLEND_ADD = 2;
const RES_FOCUS = 10;
const SCRIPT_FINISHED = 2;
const SE_OPEN = 6;
const SE_DECIDE = 7;
const SE_CANCEL = 8;
const SE_WINDOW_OPEN = 15;
const SE_WINDOW_CLOSE = 16;
const ZSORT_NONE = 0;
const ZSORT_OVERLAY = 3;
const ZSORT_BACKGROUND = 4;
const IMG_BUSY = 0;
const IMG_DONE = 1;
const IMG_FAILED = -1;

/* As in stdmenu.inc. */
const MENU_REDRAW = 1;
const MENU_REOPEN = 2;
const MENU_SUBMENU = 4;
const MENU_BACK = 5;
const MENU_CLOSED = 7;
const MENU_ITEM = 8;
const MENU_RETURN = 3;
const MENU_CANCEL = 6;
const MENU_SIDE_LEFT = 0;
const MENU_SIDE_RIGHT = 1;
const MENU_BACKDROP_FADED = 0x200;
const MENU_MODE_GROUP = 0;

/* As in scroll.inc. */
const SCROLL_ALONG_CURVE = 0;
const SCROLL_LINE = 1;

/* As in genmenu.inc. */
const MENU_TYPE_PAGE = 0;
const MENU_TYPE_POPUP = 1;

/* As in devimage.inc. */
const DEVICE_DISC = 1;
const DEVICE_HDD = 2;
const DEVICE_MC = 3;
const DEVICE_USB = 4;

/* As in text.inc. */
const EDIT_MODE_USER_ID = 1;
const EDIT_MODE_PASSWORD = 2;
const EDIT_MODE_KATAKANA = 3;
const EDIT_MODE_NUMBER = 4;
const EDIT_MODE_KATAKANA_FIXED = 7;
const EDIT_MODE_QWERTY = 9;

/* As in string.inc. */
const MC_TYPE_PS1 = 1;
const MC_TYPE_PS2 = 2;
const MC_TYPE_PDA = 3;
const MC_CHANGED = -1;
const MC_CHANGED_UNFORMATTED = -2;

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

stock Float:operator*(Float:oper1, oper2) return floatmul(oper1, float(oper2));

stock bool:operator>(Float:oper1, Float:oper2) return floatcmp(oper1, oper2) > 0;

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

set_wnd_var_str(name[], value[])
{
    new vars = domFindTag(domRoot(), "__wndvar__", _, 0);
    if (!vars)
    {
        vars = domInsertTag(domFirstChild(domRoot(), ""), "__wndvar__");
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

/* Sets the size of the clouds of the background scene ("CLDR"). */
set_bg_cloud_size(size)
{
    new scene = wndGetScene(WND_BG);
    if (scene)
    {
        SendMessage(scene, TAG("CLDR"), size, 0, 0, 0);
    }
}

/* Makes the camera of the background window follow this camera, by these shares of its position and rotation. */
link_bg_camera(Float:pos_follow, Float:rot_follow)
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
    new scene = wndGetScene(WND_BG);
    if (scene)
    {
        SendMessage(scene, TAG("RELS"), animate, duration, 0, 0);
    }
}

/* Same as create_focus_plane2 of focus.inc. */
create_focus_plane2(script[])
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

public stdmenu_curr_group; /* the MENUGROUP element of find_menu_group */

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
 * general_menu of the items of the MENUGROUP group, and, when a push leads to another menu, sends "TITL", "GRUP",
 * "IDTY" and "FNSH" (result, mode, selected index) to the manager.
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
    new title_label;
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
    title_label = CreateLabel(self);
    {
        new Float:x;
        new Float:y;
        get_list_title_pos(0, item_count, 1, x, y);
        SetPos(title_label, 0.0 * screen_w, screen_h * y, 0.0, 0);
        SetColor(title_label, 0.902, 0.902, 0.215, 0.0);
        AnimColor(title_label, 0.902, 0.902, 0.215, 1.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
        SetOptionI(title_label, TAG("FONT"), FontID(0));
        SetOptionS(title_label, TAG("UTF8"), title);
        SetOptionI(title_label, TAG("FSIZ"), 17);
        SetOptionI(title_label, TAG("ZSRT"), ZSORT_OVERLAY);
        SetOptionI(title_label, TAG("HALN"), ALIGN_CENTER);
        SetOptionI(title_label, TAG("VALN"), ALIGN_CENTER);
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
                    new item = create_label_item(b, label);
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
    if (title_label)
    {
        AnimColor(title_label, 0.902, 0.902, 0.215, 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
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
    if (title_label)
    {
        Destroy(title_label);
        title_label = 0;
    }
    if (result == MENU_REDRAW)
    {
        goto rebuild;
    }
    return selected;
}

/*
 * Shows the option menu of the MENUGROUP group (prefix is not used) with item select selected, on the left of
 * the screen (flags & 0x3 is 0 or 2) or on the right (1), over a dark backdrop if flags & 0x300, and the menus
 * it leads to; returns the result, MENU_CLOSED or MENU_ITEM + the chosen index.
 */
pop_std_menu(prefix[], group[], flags, select)
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
    if (!get_item_attr(select, "id", cur_id, 64))
    {
        cur_id = "";
    }
    sleep;
    for (; ; )
    {
        new args[256];
        sprintf(args, "title = %s ; group = %s ; id = %s", cur_title, cur_group, cur_id);
        AttachScript(menu_obj, "std_menu_core", _, args);
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
        if (result >= MENU_CANCEL) /* MENU_CLOSED or an item */
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

/* Creates a shadowed label item of an option menu. */
create_label_item(parent, label[])
{
    new item;
    item = CreateLabelShadow(parent);
    SetOptionP(item, TAG("SCL4"), g_shadow_colors, 4);
    SetOptionI(item, TAG("SX64"), 2 * 64);
    SetOptionI(item, TAG("SY64"), 2 * 64);
    SetColor(item, 0.215, 0.708, 0.999, 0.0);
    SetOptionI(item, TAG("ALPH"), BLEND_ADD);
    SetOptionI(item, TAG("FONT"), FontID(0));
    SetOptionS(item, TAG("UTF8"), label);
    SetOptionI(item, TAG("FSIZ"), 15);
    SetOptionI(item, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(item, TAG("VALN"), ALIGN_CENTER);
    AttachScript(item, "label_item", "label_item");
    SetPubVar(item, "pad_enable", PAD_CIRCLE);
    return item;
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

new g_device_image;
new Float:g_device_x = 130.0;
new Float:g_device_y = 350.0;
new Float:g_device_w = 512.0;
new Float:g_device_h = 512.0;

set_box_title(text[])
{
    set_box_header(0, text);
}

/* Shows text, the position of the save in the album, in the footer of the box. */
set_box_counter(text[])
{
    set_box_footer(text);
}

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

/* Fades the device image to alpha over duration ms. */
fade_device_image(Float:alpha, Float:duration)
{
    if (g_device_image)
    {
        SendMessage(g_device_image, TAG("FADE"), alpha, duration, 0, 0);
    }
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
/* The kinds of the items of g_cards: the memory card of port 0 or 1; the virtual ones are never listed. */
const ENTRY_CARD1 = 1;
const ENTRY_CARD2 = 2;
const ENTRY_VMC1 = 3;
const ENTRY_VMC2 = 4;
new g_cards[4][67]; /* the items of the memory card album: ENTRY_*, card type, 1, location */
new g_card_types[4]; /* the type of the card in each port, as last seen */
public device_mode; /* DEVMODE_* */
public mc_port; /* unused, but part of the layout */
public mc_slot; /* unused, but part of the layout */

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
            new percent2;
            new status2;
            ret = svdatSync(percent2, status2, job);
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
            new percent3;
            new status3;
            ret = svdatSync(percent3, status3, job);
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

/*
 * Lists the items of the album of mode: the folders of the hard disk (DEVMODE_HDD, from database 0) or the memory
 * cards in the ports (DEVMODE_MC, into g_cards, refreshed first if refresh); returns their number, negative on an
 * error.
 */
query_album(mode, refresh)
{
    new count;
    new ret;
    count = 0;
    switch (mode)
    {
        case DEVMODE_HDD:
        {
            ret = datOpen(DAT_FILE_LIST, "pfs:/__common", DAT_ORDER_CTCOUNT_DESC);
            if (0 != ret)
            {
                return ret;
            }
            ret = wait_database();
            if (ret < 0)
            {
                return ret;
            }
            count = datGetNum();
        }
        case DEVMODE_MC:
        {
            count = 0;
            new unused; /* unused, but part of the layout */
            if (refresh)
            {
                update_card_info(unused, 0, 0, 0);
            }
            new type;
            new free_blocks;
            new formatted;
            new status;
            get_card_info(0, 0, type, free_blocks, formatted, status);
            g_card_types[0] = type;
            if (0 != type)
            {
                g_cards[count][0] = ENTRY_CARD1;
                g_cards[count][1] = type;
                g_cards[count][2] = 1;
                get_card_location(type, 0, g_cards[count][3], 0);
                count++;
            }
            get_card_info(1, 0, type, free_blocks, formatted, status);
            g_card_types[1] = type;
            if (0 != type)
            {
                g_cards[count][0] = ENTRY_CARD2;
                g_cards[count][1] = type;
                g_cards[count][2] = 1;
                get_card_location(type, 1, g_cards[count][3], 0);
                count++;
            }
        }
        default:
        {
            count = 0;
        }
    }
    return count;
}

/* Returns the number of folders in pfs://__common, negative on an error. */
count_hdd_folders()
{
    new job;
    new ret = svdatCountData("pfs://__common", "", job);
    if (ret < 0)
    {
        return ret;
    }
    new count;
    new percent;
    new waiting = 1;
    while (waiting)
    {
        ret = svdatSync(percent, count, job);
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
    return count;
}

/* Writes into text where a memory card of type (MC_TYPE_*) in port is. */
get_card_location(type, port, text[], maxlen)
{
    switch (type)
    {
        case MC_TYPE_PS1:
        {
            new card[64];
            new slot_name[64];
            get_tag_attr("TEXT", "msgutil_devinfo_location_mc", "value", card, 64);
            switch (port)
            {
                case 0:
                {
                    get_tag_attr("TEXT", "msgutil_devinfo_location1", "value", slot_name, 64);
                }
                case 1:
                {
                    get_tag_attr("TEXT", "msgutil_devinfo_location2", "value", slot_name, 64);
                }
            }
            sprintf(text, "%s%s", card, slot_name);
        }
        case MC_TYPE_PS2:
        {
            new card[64];
            new slot_name[64];
            get_tag_attr("TEXT", "msgutil_devinfo_location_mcps2", "value", card, 64);
            switch (port)
            {
                case 0:
                {
                    get_tag_attr("TEXT", "msgutil_devinfo_location1", "value", slot_name, 64);
                }
                case 1:
                {
                    get_tag_attr("TEXT", "msgutil_devinfo_location2", "value", slot_name, 64);
                }
            }
            sprintf(text, "%s%s", card, slot_name);
        }
        case MC_TYPE_PDA:
        {
            new card[64];
            new slot_name[64];
            get_tag_attr("TEXT", "msgutil_devinfo_location_pocket", "value", card, 64);
            switch (port)
            {
                case 0:
                {
                    get_tag_attr("TEXT", "msgutil_devinfo_location1", "value", slot_name, 64);
                }
                case 1:
                {
                    get_tag_attr("TEXT", "msgutil_devinfo_location2", "value", slot_name, 64);
                }
            }
            sprintf(text, "%s%s", card, slot_name);
        }
        default:
        {
            get_tag_attr("TEXT", "msgutil_dev_unknown", "value", text, maxlen);
        }
    }
}

public camera_focus; /* the point the camera looks at */
public camera_uptgt; /* unused, but part of the layout */
public focus_glowfly; /* the focus object */
public device_menu;   /* the radio buttons of the devices */
new g_message_shadow[4] = {0x5F000000, 0x5F000000, 0x5F000000, 0x5F000000}; /* the "SCL4" of the message labels */
new g_mode_devices[4] = {DEVICE_HDD, DEVICE_MC, DEVICE_MC}; /* the device image of each mode */
new g_ps1_card_icons[2];
new g_ps2_card_icons[2];
new g_pocket_icons[2];
new g_folder_icon;
new g_album_focus_pending; /* the focus moves to the album once it stops */

/* Sleeps for ms milliseconds, a Float. */
wait_ms_float(Float:ms)
{
    new frames = floatround(ms * float(FrameRate()) / 1000.0, 0);
    for (new i; i < frames; i++)
    {
        sleep;
    }
}

/*
 * Mode 0 is the hard disk, 1 the memory cards (album variable BrowserAlbumMode, kept from the thumb page with the
 * selected item, BrowserAlbumNo). Opening an item leaves its name, mode and number in the album variables for the
 * thumb page. The memory cards are checked every 30 frames, and the album is rebuilt when one changes.
 */
main()
{
    start_top_page(0);
    load_common_bg("bg", 1000.0);
    set_background_poor(600.0, 0);
    set_bg_cloud_size(0);
    new self = Self();
    new camera = Camera(0);
    new Float:screen_w;
    new Float:screen_h;
    get_screen_size(screen_w, screen_h);
    create_camera();
    link_bg_camera(0.4, 0.4);
    reset_bg_camera();
    focus_glowfly = create_focus_plane2("focus");
    new album_idle; /* the album has stopped moving */
    create_focus_box(0.7625 * screen_w, 0.5 * screen_h);
    fade_box(0.0, 100.0, 1.0, 1.0, 1.0);
    set_box_focus(focus_glowfly);
    set_box_title("");
    new name[256];
    g_ps1_card_icons[0] = load_kept_texture("file:/opt0/bn/data/tex/default_icon/PS1_MemoryCard.tm2");
    g_ps1_card_icons[1] = load_kept_texture("file:/opt0/bn/data/tex/default_icon/PS1_MemoryCard2.tm2");
    g_ps2_card_icons[0] = load_kept_texture("file:/opt0/bn/data/tex/default_icon/Memory_card1.tm2");
    g_ps2_card_icons[1] = load_kept_texture("file:/opt0/bn/data/tex/default_icon/Memory_card2.tm2");
    g_pocket_icons[0] = load_kept_texture("file:/opt0/bn/data/tex/default_icon/Pocket_Station1.tm2");
    g_pocket_icons[1] = load_kept_texture("file:/opt0/bn/data/tex/default_icon/Pocket_Station2.tm2");
    g_folder_icon = load_kept_texture("file:/opt0/bn/data/tex/default_icon/Folder.tm2");
    new mode = DEVMODE_MC;
    new album_no = -1;
    {
        new value[64];
        if (GetArg("mode", value, 64))
        {
            if (!strcmp(value, "ThumbReturn"))
            {
                if (0 == avalGetValue("BrowserJumpFrom", 0, value, 64))
                {
                    if (0 != strcmp(value, "top"))
                    {
                        if (0 == avalGetValue("BrowserAlbumMode", 0, value, 64))
                        {
                            mode = atoi(value);
                        }
                        if (0 == avalGetValue("BrowserAlbumNo", 0, value, 64))
                        {
                            album_no = atoi(value);
                        }
                    }
                }
            }
        }
        avalSet("BrowserJumpFrom", "top");
    }
    create_device_image();
    show_device_image(g_mode_devices[mode]);
    device_menu = create_device_buttons();
    SetPubVar(device_menu, "select", 1 - mode);
    SetPubVar(device_menu, "cursor", 1 - mode);
    new count = query_album(mode, 1);
    SendMessage(device_menu, TAG("DSEL"), count <= 0, 0, 0, 0);
    if (count < 0)
    {
        if (DEVMODE_HDD == mode)
        {
            if (count == -5)
            {
                show_disk_error();
            }
        }
        count = 0;
    }
    new album = create_album(count);
    new idle;
    new active = device_menu;
    if (album_no >= 0)
    {
        if (count > 0)
        {
            active = album;
        }
        if (mode == DEVMODE_MC)
        {
            album_no = find_card_entry(0, count, album_no);
        }
        if (album_no >= 0)
        {
            SendMessage(album, TAG("SLCT"), album_no, 0, 0, 0);
        }
    }
    show_active_menu(device_menu, album, active, count, 1);
    SendMessage(active, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
    new message_labels[3];
    create_message_labels(message_labels);
    if (count <= 0)
    {
        show_empty_message(message_labels, mode, 1.0, 500.0);
    }
    new frames;
    frames = 0;
    new card_changed; /* a memory card changed before an item was opened */
    for (; ; )
    {
        new reload; /* an option succeeded: the album is read again */
        new msg;
        new a;
        new b;
        new c;
        /*
         * From the device buttons: "FOCS", "CHNG" (button: 0 memory cards, 1 hard disk), "PUSH" (index, button, item);
         * from the album: "PUSH", "FOCS" and "ITEM" (menu, path, index).
         */
        while (RecvMessage(msg, a, b, c))
        {
            if (active == device_menu)
            {
                if (msg == TAG("FOCS"))
                {
                    SendMessage(active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                }
                if (msg == TAG("CHNG"))
                {
                    new button = a;
                    new changed;
                    SendMessage(device_menu, TAG("CHNG"), 0, 0, 0, 0);
                    if (0 == button)
                    {
                        show_device_image(DEVICE_MC);
                        if (DEVMODE_MC != mode)
                        {
                            mode = DEVMODE_MC;
                            count = load_album(album, mode, 1);
                            changed = 1;
                        }
                    }
                    else if (button == 1)
                    {
                        show_device_image(DEVICE_HDD);
                        if (DEVMODE_HDD != mode)
                        {
                            mode = DEVMODE_HDD;
                            count = load_album(album, mode, 1);
                            changed = 1;
                        }
                    }
                    SendMessage(device_menu, TAG("DSEL"), count <= 0, 0, 0, 0);
                    if (changed)
                    {
                        if (0 == count)
                        {
                            new text[64];
                            show_empty_message(message_labels, mode, 1.0, 500.0);
                            active = device_menu;
                            SendMessage(device_menu, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                        }
                        else if (count > 0)
                        {
                            show_empty_message(message_labels, -1, 0.0, 500.0);
                            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                            SendMessage(device_menu, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                            g_album_focus_pending = 1;
                        }
                    }
                    else if (count > 0)
                    {
                        if (album_idle)
                        {
                            SendMessage(device_menu, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                            SendMessage(album, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
                            focus_box_slot(0);
                            active = album;
                            show_active_menu(device_menu, album, active, count, 0);
                        }
                        else
                        {
                            g_album_focus_pending = 1;
                            SendMessage(device_menu, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                        }
                    }
                    printf(" device_menu pushed %d\n", a);
                }
                if (msg == TAG("PUSH"))
                {
                    if (b == TAG("RUP"))
                    {
                        sndSePlay(SE_OPEN);
                        sndSePlay(SE_WINDOW_OPEN);
                        fade_device_image(0.0, 400.0);
                        AnimColor(message_labels[0], 0.7843, 0.7843, 0.7843, 0.0, 400.0, 0.0, ANIM_FUNC_LINEAR);
                        AnimColor(message_labels[1], 0.7843, 0.7843, 0.7843, 0.0, 400.0, 0.0, ANIM_FUNC_LINEAR);
                        AnimColor(message_labels[2], 0.7843, 0.7843, 0.7843, 0.0, 400.0, 0.0, ANIM_FUNC_LINEAR);
                        SendMessage(active, TAG("PUSH"), 0, 0, 0, 0);
                        SendMessage(active, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                        unfocus_box();
                        fade_box(0.0, 400.0, 1.0, 1.0, 1.0);
                        SendMessage(device_menu, TAG("FADE"), 0, 400.0, 0, 0);
                        AnimMoveHermite(Camera(0), 300.0, 0.0, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
                        AnimMoveHermite(camera_focus, 300.0, 0.0, -100.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
                        wait_ms_float(750.0);
                        SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
                        new result = pop_std_menu("menutitle", "channel_menu", 1, 0);
                        SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
                        switch (result)
                        {
                            case MENU_ITEM:
                            {
                                reload = 1;
                            }
                        }
                        sndSePlay(SE_WINDOW_CLOSE);
                        fade_device_image(1.0, 400.0);
                        center_camera();
                        wait_ms_float(750.0);
                        SendMessage(device_menu, TAG("FADE"), 1.0, 400.0, 0, 0);
                        if (0 == count)
                        {
                            new text[64];
                            show_empty_message(message_labels, mode, 1.0, 500.0);
                        }
                        else if (count > 0)
                        {
                            show_empty_message(message_labels, -1, 0.0, 500.0);
                            fade_box(0.4, 400.0, 1.0, 1.0, 1.0);
                            focus_box_slot(0);
                        }
                        SendMessage(active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                    }
                    else if (b == TAG("RLFT"))
                    {
                        SendMessage(active, TAG("PUSH"), 0, 0, 0, 0);
                        SendMessage(active, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                    }
                    else if (b != TAG("RLFT"))
                    {
                        SendMessage(active, TAG("PUSH"), 0, 0, 0, 0);
                    }
                }
            }
            else if (active == album)
            {
                if (msg == TAG("PUSH"))
                {
                    if (b == TAG("RRGT"))
                    {
                        if (mode == DEVMODE_MC)
                        {
                            new unused; /* unused, but part of the layout */
                            new types[2];
                            new free_blocks;
                            new formatted;
                            new states[2];
                            update_card_info(unused, 0, 0, 0);
                            get_card_info(0, 0, types[0], free_blocks, formatted, states[0]);
                            get_card_info(1, 0, types[1], free_blocks, formatted, states[1]);
                            if (g_card_types[0] != types[0] || states[0] == MC_CHANGED
                                    || states[0] == MC_CHANGED_UNFORMATTED || g_card_types[1] != types[1]
                                    || states[1] == MC_CHANGED || states[1] == MC_CHANGED_UNFORMATTED)
                            {
                                card_changed = 1;
                            }
                        }
                        if (!card_changed)
                        {
                            sndSePlay(SE_DECIDE);
                            SendMessage(album, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                            SendMessage(device_menu, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                            {
                                new select = GetPubVar(album, "select");
                                switch (mode)
                                {
                                    case DEVMODE_HDD:
                                    {
                                        datGetData(select, SAVE_NAME, name, 256);
                                    }
                                    case DEVMODE_MC:
                                    {
                                        sprintf(name, "%s", g_cards[select][3]);
                                    }
                                }
                                avalSet("BrowserAlbumName", name);
                                new number[16];
                                sprintf(number, "%d", mode);
                                avalSet("BrowserAlbumMode", number);
                                if (mode == DEVMODE_MC)
                                {
                                    select = g_cards[select][0];
                                }
                                sprintf(number, "%d", select);
                                avalSet("BrowserAlbumNo", number);
                                avalSet("BrowserJumpFrom", "album");
                            }
                            fade_background(0, 0, 600.0);
                            new url[256];
                            release_bg_camera(1, 2000.0);
                            switch (mode)
                            {
                                case DEVMODE_HDD:
                                {
                                    new value[256];
                                    datGetData(a, SAVE_COMMENT, value, 256);
                                    avalSet("BrowserHDDURL", value);
                                    datGetData(a, SAVE_NAME, value, 256);
                                    avalSet("BrowserHDDNAME", value);
                                    get_tag_attr("LINK", "thumbnail", "href", url, 256);
                                    xmlLoadStart(url, _, "device = HDD;", XMLLOAD_MAIN);
                                    wait_xml_load();
                                }
                                case DEVMODE_MC:
                                {
                                    new virtual;
                                    new select = GetPubVar(album, "select");
                                    switch (g_cards[select][0])
                                    {
                                        case ENTRY_CARD1:
                                        {
                                            avalSet("BrowserMCPort", "0");
                                            avalSet("BrowserMCSlot", "0");
                                            virtual = 0;
                                        }
                                        case ENTRY_CARD2:
                                        {
                                            avalSet("BrowserMCPort", "1");
                                            avalSet("BrowserMCSlot", "0");
                                            virtual = 0;
                                        }
                                        case ENTRY_VMC1:
                                        {
                                            avalSet("BrowserMCPort", "0");
                                            avalSet("BrowserMCSlot", "0");
                                            virtual = 1;
                                        }
                                        case ENTRY_VMC2:
                                        {
                                            avalSet("BrowserMCPort", "1");
                                            avalSet("BrowserMCSlot", "0");
                                            virtual = 1;
                                        }
                                        default:
                                        {
                                            get_tag_attr("LINK", "Back", "href", url, 256);
                                            xmlLoadStart(url, _, _, XMLLOAD_MAIN);
                                            wait_xml_load();
                                        }
                                    }
                                    get_tag_attr("LINK", "thumbnail", "href", url, 256);
                                    if (0 == virtual)
                                    {
                                        xmlLoadStart(url, _, "device = MC;", XMLLOAD_MAIN);
                                    }
                                    else
                                    {
                                        xmlLoadStart(url, _, "device = VMC;", XMLLOAD_MAIN);
                                    }
                                    wait_xml_load();
                                }
                            }
                            goto leave;
                        }
                    }
                    else if (b == TAG("RUP"))
                    {
                        if (DEVMODE_HDD == mode)
                        {
                            sndSePlay(SE_OPEN);
                            sndSePlay(SE_WINDOW_OPEN);
                            fade_device_image(0.0, 400.0);
                            new value[256];
                            new select = GetPubVar(album, "select");
                            datGetData(select, SAVE_COMMENT, value, 256);
                            set_wnd_var_str("BRSRCURL", value);
                            datGetData(select, SAVE_NAME, value, 256);
                            set_wnd_var_str("BRSRCNAME", value);
                            SendMessage(active, TAG("PUSH"), 0, 0, 0, 0);
                            SendMessage(active, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                            unfocus_box();
                            SendMessage(device_menu, TAG("FADE"), 0, 400.0, 0, 0);
                            set_box_title(value);
                            fade_box(1.0, 400.0, 1.0, 1.0, 1.0);
                            SetPos(g_focus_box, 0.7625 * screen_w, 0.5 * screen_h, 0.0, 0);
                            AnimMoveHermite(g_focus_box, 0.7625 * screen_w - 300.0, 0.5 * screen_h, 0.0, 750.0, 0.0,
                                    ANIM_FUNC_EXPONENT, 1.0);
                            fade_box_footer(0.0, 400.0, 0.7843, 0.7843, 0.7843);
                            AnimMoveHermite(Camera(0), 300.0, 0.0, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
                            AnimMoveHermite(camera_focus, 300.0, 0.0, -100.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
                            wait_ms_float(750.0);
                            SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
                            new result = pop_std_menu("menutitle", "option_menu", 1, 0);
                            SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
                            switch (result)
                            {
                                case MENU_ITEM:
                                {
                                    reload = 1;
                                }
                            }
                            sndSePlay(SE_WINDOW_CLOSE);
                            fade_device_image(1.0, 400.0);
                            SetPos(g_focus_box, 0.7625 * screen_w - 300.0, 0.5 * screen_h, 0.0, 0);
                            AnimMoveHermite(g_focus_box, 0.7625 * screen_w, 0.5 * screen_h, 0.0, 750.0, 0.0,
                                    ANIM_FUNC_EXPONENT, 1.0);
                            fade_box_footer(1.0, 400.0, 0.7843, 0.7843, 0.7843);
                            center_camera();
                            wait_ms_float(750.0);
                            focus_box_slot(0);
                            SendMessage(device_menu, TAG("FADE"), 0.5, 400.0, 0, 0);
                            SendMessage(active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                        }
                        else
                        {
                            SendMessage(active, TAG("PUSH"), 0, 0, 0, 0);
                        }
                    }
                    else if (b != TAG("RLFT"))
                    {
                        SendMessage(active, TAG("PUSH"), 0, 0, 0, 0);
                    }
                }
                else if (msg == TAG("FOCS"))
                {
                    focus_box_slot(0);
                    SendMessage(active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                }
            }
            if (msg == TAG("ITEM"))
            {
                new item = create_album_item(b, c, count, mode);
                if (IsValidObject(a))
                {
                    SendMessage(a, TAG("ITEM"), item, b, 0, 0);
                }
            }
            if (msg == TAG("PUSH"))
            {
                if (b == TAG("RDWN"))
                {
                    if (active == album)
                    {
                        sndSePlay(SE_CANCEL);
                        SendMessage(album, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                        unfocus_box();
                        sleep;
                        sleep;
                        SendMessage(device_menu, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
                        active = device_menu;
                        show_active_menu(device_menu, album, active, count, !album_idle);
                    }
                    else
                    {
                        avalSet("BrowserAlbumMode", "1");
                        avalSet("BrowserAlbumNo", "-1");
                        sndSePlay(SE_CANCEL);
                        SendMessage(album, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                        SendMessage(device_menu, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                        fade_background(0, 0, 600.0);
                        release_bg_camera(1, 2000.0);
                        new url[256];
                        get_tag_attr("LINK", "Back", "href", url, 256);
                        xmlLoadStart(url, _, _, XMLLOAD_MAIN);
                        wait_xml_load();
                        goto leave;
                    }
                }
                else if (b == TAG("RLFT"))
                {
                    SendMessage(active, TAG("PUSH"), 0, 0, 0, 0);
                    SendMessage(active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                }
            }
        }
        if (reload)
        {
            new was_album;
            if (active == album)
            {
                was_album = 1;
            }
            SendMessage(active, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            count = load_album(album, mode, 1);
            SendMessage(device_menu, TAG("DSEL"), count <= 0, 0, 0, 0);
            if (0 == count)
            {
                new text[64];
                show_empty_message(message_labels, mode, 1.0, 500.0);
                if (was_album)
                {
                    active = device_menu;
                    SendMessage(active, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
                }
                else
                {
                    SendMessage(active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                }
            }
            else if (count > 0)
            {
                show_empty_message(message_labels, -1, 0.0, 500.0);
                if (was_album)
                {
                    send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                    SendMessage(device_menu, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                    g_album_focus_pending = 1;
                }
                else
                {
                    SendMessage(active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                    g_album_focus_pending = 1;
                }
            }
            show_active_menu(device_menu, album, active, count, 1);
            sleep;
            continue;
        }
        if (count > 0)
        {
            new was_idle = album_idle;
            new select = GetPubVar(album, "select");
            idle = GetPubVar(album, "idle");
            if (idle)
            {
                album_idle = 1;
            }
            else
            {
                album_idle = 0;
            }
            if (album_idle != was_idle)
            {
                new was_album;
                if (active == album)
                {
                    was_album = 1;
                }
                if (album_idle)
                {
                    if (g_album_focus_pending)
                    {
                        SendMessage(device_menu, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                        sleep;
                        sleep;
                        SendMessage(album, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
                        focus_box_slot(0);
                        active = album;
                        show_active_menu(device_menu, album, active, count, 0);
                        g_album_focus_pending = 0;
                    }
                    switch (mode)
                    {
                        case DEVMODE_HDD:
                        {
                            datGetData(select, SAVE_NAME, name, 256);
                        }
                        case DEVMODE_MC:
                        {
                            sprintf(name, "%s", g_cards[select][3]);
                        }
                    }
                    new counter[256];
                    sprintf(counter, "%d/%d", select + 1, count);
                    set_box_title(name);
                    set_box_counter(counter);
                    if (active == album)
                    {
                        fade_box(1.0, 1000.0, 1.0, 1.0, 1.0);
                    }
                    else
                    {
                        fade_box(0.4, 1000.0, 1.0, 1.0, 1.0);
                    }
                    if (was_album)
                    {
                        focus_box_slot(0);
                    }
                }
                else
                {
                    if (was_album)
                    {
                        unfocus_box();
                    }
                    fade_box(0.0, 200.0, 1.0, 1.0, 1.0);
                }
            }
        }
        if (mode == DEVMODE_MC)
        {
            if (frames > 30)
            {
                new unused; /* unused, but part of the layout */
                new types[2];
                new free_blocks;
                new formatted;
                new states[2];
                update_card_info(unused, 0, 0, 0);
                get_card_info(0, 0, types[0], free_blocks, formatted, states[0]);
                get_card_info(1, 0, types[1], free_blocks, formatted, states[1]);
                if ((g_card_types[0] != types[0] || states[0] == MC_CHANGED || states[0] == MC_CHANGED_UNFORMATTED
                        || g_card_types[1] != types[1] || states[1] == MC_CHANGED
                        || states[1] == MC_CHANGED_UNFORMATTED) || card_changed == 1)
                {
                    card_changed = 0;
                    mode = DEVMODE_MC;
                    new was_album;
                    if (active == album)
                    {
                        was_album = 1;
                    }
                    SendMessage(active, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                    count = load_album(album, mode, 0);
                    SendMessage(device_menu, TAG("DSEL"), count <= 0, 0, 0, 0);
                    if (0 == count)
                    {
                        new text[64];
                        show_empty_message(message_labels, mode, 1.0, 500.0);
                        if (was_album)
                        {
                            active = device_menu;
                            SendMessage(active, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
                        }
                        else
                        {
                            SendMessage(active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                        }
                    }
                    else if (count > 0)
                    {
                        show_empty_message(message_labels, -1, 0.0, 500.0);
                        if (was_album)
                        {
                            active = album;
                            g_album_focus_pending = 1;
                        }
                        else
                        {
                            SendMessage(active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                        }
                    }
                }
                show_active_menu(device_menu, album, active, count, 1);
                g_card_types[0] = types[0];
                g_card_types[1] = types[1];
                frames = 0;
            }
            else
            {
                frames++;
            }
        }
        sleep;
    }
    leave:
    if (album)
    {
        SendMessage(album, TAG("FADE"), 0, 500.0, 0, 0);
    }
    if (device_menu)
    {
        SendMessage(device_menu, TAG("FADE"), 0, 500.0, 0, 0);
    }
    unfocus_box();
    fade_box(0.0, 500.0, 1.0, 1.0, 1.0);
    wait_ms(500);
    Destroy(self);
    return;
}

/* Returns the index of the first item of g_cards from start whose kind is kind, -1 if there is none. */
find_card_entry(start, count, kind)
{
    for (new i = start; i < count; i++)
    {
        if (g_cards[i][0] == kind)
        {
            return i;
        }
    }
    return -1;
}

/* Highlights the active menu, the device buttons or the album, and dims the other; the box too unless keep_box. */
show_active_menu(buttons, album, active, count, keep_box)
{
    if (active == buttons)
    {
        SendMessage(buttons, TAG("FCOL"), 0.215, 0.708, 0.999, 0);
        SendMessage(buttons, TAG("FADE"), 1.0, 400.0, 0, 0);
        if (count > 0)
        {
            SendMessage(album, TAG("FADE"), 0.4, 400.0, 0, 0);
            if (!keep_box)
            {
                fade_box(0.4, 400.0, 1.0, 1.0, 1.0);
            }
            fade_box_header(0.4, 400.0, 0.7843, 0.7843, 0.7843);
        }
    }
    else
    {
        SendMessage(buttons, TAG("FCOL"), 0.7843, 0.7843, 0.7843, 0);
        SendMessage(buttons, TAG("FADE"), 0.5, 400.0, 0, 0);
        SendMessage(album, TAG("FADE"), 1.0, 400.0, 0, 0);
        if (!keep_box)
        {
            fade_box(1.0, 400.0, 1.0, 1.0, 1.0);
        }
        fade_box_header(1.0, 400.0, 0.215, 0.708, 0.999);
    }
}

/* Points the camera at a new camera_focus and attaches camera_main, which keeps it there. */
create_camera()
{
    new camera = Camera(0);
    camera_focus = CreateNull(Self());
    camera_uptgt = CreateNull(Self());
    SetPos(camera_focus, 0.0, 0.0, -100.0, 0);
    SetPos(camera_uptgt, 0.0, 0.0 + 1000.0, 0.0, 0);
    SetPos(camera, 0.0, 0.0, 0.0, 0);
    LookAt(camera, camera_focus, 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
    AttachScript(Camera(0), "camera_main");
    SetPubVar(Camera(0), "camera_focus", camera_focus);
}

/* Creates the radio buttons of the devices: the memory cards and the hard disk (TEXT browser_menu items 1 and 0). */
create_device_buttons()
{
    new Float:screen_w;
    new Float:screen_h;
    get_screen_size(screen_w, screen_h);
    new buttons;
    buttons = CreateNull(Disp2D(0));
    {
        new label0[64];
        new label1[64];
        new label2[64];
        new args[256];
        new count = 2;
        get_group_tag_attr("TEXT", "browser_menu", 1, "value", label0, 64);
        get_group_tag_attr("TEXT", "browser_menu", 0, "value", label1, 64);
        new Float:top = 0.5 - 0.08 * float(count - 1) / 2.0;
        SetPos(buttons, 0.14 * screen_w, screen_h * top, 0.0, 0);
        SetColor(buttons, 0.215, 0.708, 0.999, 0.0);
        SetOptionI(buttons, TAG("ZSRT"), ZSORT_OVERLAY);
        sprintf(args, "btn0 = %s ; btn1 = %s ; interval = %d", label0, label1, floatround(0.08 * screen_h, 0));
        AttachScript(buttons, _, "radio_button_v", args);
        SetPubVar(buttons, "manager", Self());
        SetPubVar(buttons, "focus", focus_glowfly);
        SetPubVar(buttons, "padhelp", PADHELP_CIRCLE | PADHELP_CROSS | PADHELP_TRIANGLE);
        SetPubVar(buttons, "disable_selected", 0);
        SetPubVar(buttons, "se_focusin", -1);
        SendMessage(buttons, TAG("MASK"), 0, PADHELP_CIRCLE | PADHELP_CROSS, 0, 0);
    }
    return buttons;
}

/*
 * Creates item index of the album of mode: the folder icon on the hard disk; on the memory cards the icon of the
 * card type, or the image of a virtual memory card.
 */
create_album_item(path, index, count, mode)
{
    if (!IsValidObject(path))
    {
        return 0;
    }
    new item = CreatePlane(path);
    AttachScript(item, "album_item", "album_item");
    SetOptionI(item, TAG("FDEG"), 1);
    SetColor(item, 1.0, 1.0, 1.0, 0.0);
    if (mode == DEVMODE_MC)
    {
        SetPubVar(item, "pad_enable", PAD_CIRCLE);
    }
    else
    {
        SetPubVar(item, "pad_enable", PAD_CIRCLE | PAD_TRIANGLE);
    }
    if (index >= 0 && index < count)
    {
        switch (mode)
        {
            case DEVMODE_HDD:
            {
                SendMessage(item, TAG("TEX"), g_folder_icon, 1.0, 0, 0);
            }
            case DEVMODE_MC:
            {
                switch (g_cards[index][0])
                {
                    case ENTRY_CARD1:
                    {
                        new type;
                        new free_blocks;
                        new formatted;
                        new status;
                        get_card_info(0, 0, type, free_blocks, formatted, status);
                        switch (type)
                        {
                            case MC_TYPE_PS1:
                            {
                                SendMessage(item, TAG("TEX"), g_ps1_card_icons[0], 1.0, 0, 0);
                            }
                            case MC_TYPE_PS2:
                            {
                                SendMessage(item, TAG("TEX"), g_ps2_card_icons[0], 1.0, 0, 0);
                            }
                            case MC_TYPE_PDA:
                            {
                                SendMessage(item, TAG("TEX"), g_pocket_icons[0], 1.0, 0, 0);
                            }
                        }
                    }
                    case ENTRY_CARD2:
                    {
                        new type;
                        new free_blocks;
                        new formatted;
                        new status;
                        get_card_info(1, 0, type, free_blocks, formatted, status);
                        switch (type)
                        {
                            case MC_TYPE_PS1:
                            {
                                SendMessage(item, TAG("TEX"), g_ps1_card_icons[1], 1.0, 0, 0);
                            }
                            case MC_TYPE_PS2:
                            {
                                SendMessage(item, TAG("TEX"), g_ps2_card_icons[1], 1.0, 0, 0);
                            }
                            case MC_TYPE_PDA:
                            {
                                SendMessage(item, TAG("TEX"), g_pocket_icons[1], 1.0, 0, 0);
                            }
                        }
                    }
                    case ENTRY_VMC1:
                    {
                        SendMessageStr(item, TAG("DCDE"), "file:/opt0/bn/data/tex/default_icon/Virtual_MemoryCard.tm2");
                    }
                    case ENTRY_VMC2:
                    {
                        SendMessageStr(item, TAG("DCDE"), "file:/opt0/bn/data/tex/default_icon/Virtual_MemoryCard.tm2");
                    }
                }
            }
        }
    }
    return item;
}

/* Replaces album with the one of mode; returns its number of items (0 on an error, after the message of a -5). */
load_album(&album, mode, refresh)
{
    unfocus_box();
    fade_box(0.0, 400.0, 1.0, 1.0, 1.0);
    SendMessage(album, TAG("FADE"), 0, 400, 0, 0);
    datClose();
    Destroy(album);
    album = 0;
    wait_database();
    new count = query_album(mode, refresh);
    if (count < 0)
    {
        if (DEVMODE_HDD == device_mode)
        {
            if (count == -5)
            {
                SendMessage(device_menu, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                show_disk_error();
                SendMessage(device_menu, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
            }
        }
        count = 0;
    }
    printf("new db opened %d\n", count);
    album = create_album(count);
    SendMessage(album, TAG("FADE"), 0.4, 1000, 0, 0);
    return count;
}

/* Creates the album: a general_menu of count items on the curve of get_node_x/y/z. */
create_album(count)
{
    new Float:screen_w;
    new Float:screen_h;
    get_screen_size(screen_w, screen_h);
    new album;
    album = CreateNull(Self());
    {
        new Float:x;
        new Float:y;
        new Float:z;
        map_screen_point(x, y, z, 0.7625 * screen_w, 0.5 * screen_h, Self());
        SetPos(album, x, y, z, 0);
        AttachScript(album, "general_menu", "general_menu");
        SetPubVar(album, "manager", Self());
        SetPubVar(album, "focus", focus_glowfly);
        SetPubVar(album, "pad_enable", PAD_CROSS);
        SetPubVar(album, "se_focusin", -1);
        SendMessage(album, TAG("INUM"), count, 0, 0, 0);
        SendMessage(album, TAG("CLRW"), 1, 1, 0, 0);
        SendMessage(album, TAG("PADL"), 0, 0, 1, -1);
        SendMessage(album, TAG("MGIN"), 1, 7, 0, 0);
        SendMessage(album, TAG("MODE"), MENU_TYPE_PAGE, SCROLL_ALONG_CURVE, 0, 0);
        SendMessage(album, TAG("ANIM"), 250.0, 0, ANIM_FUNC_EXPONENT, 0);
        SendMessage(album, TAG("REPT"), 50.01, 83.34, 0, 0);
        for (new node; node < 9; node++)
        {
            SendMessage(album, TAG("NPOS"), node, get_node_x(node - 1), get_node_y(node - 1), get_node_z(node - 1));
        }
    }
    return album;
}

/* Returns the x of node of the album curve, the selected item being node 0: -25 * node^2. */
Float:get_node_x(node)
{
    new n = node;
    return 25.0 * (-n * n);
}

/* Returns the y of node of the album curve: 0. */
Float:get_node_y(node)
{
    new n = node;
    return 0.0 * n;
}

/* Returns the z of node of the album curve: -350 per node. */
Float:get_node_z(node)
{
    new n = node;
    return 350.0 * -n;
}

/* Moves the camera back to the center. */
center_camera()
{
    AnimMoveHermite(Camera(0), 0.0, 0.0, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
    AnimMoveHermite(camera_focus, 0.0, 0.0, -100.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
}

/* The script of the camera of the page: it keeps looking at camera_focus. */
public camera_main()
{
    for (; ; )
    {
        LookAt(Self(), camera_focus, 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
        sleep;
    }
}

/* Asks msg with the buttons yes and no, select first; returns 0 for yes, 1 for no or cross. */
ask_yes_no(msg[], yes[], no[], select)
{
    new args[512];
    sprintf(args, "msg=%s;yes=%s;no=%s", msg, yes, no);
    SetOptionI(focus_glowfly, TAG("ZSRT"), ZSORT_OVERLAY);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    AttachScript(dialog, _, "confirm_yesno", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus_glowfly);
    SetPubVar(dialog, "select", select);
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
    SetOptionI(focus_glowfly, TAG("ZSRT"), ZSORT_NONE);
    return answer;
}

/* Shows msg in a confirm_ok dialog with ok on its button and waits until it is closed. */
show_message(msg[], ok[])
{
    new args[512];
    sprintf(args, "msg=%s;ok=%s", msg, ok);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    AttachScript(dialog, _, "confirm_ok", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus_glowfly);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    Destroy(dialog);
}

/*
 * Copies the hard disk folder BRSRCNAME to a name the user enters, offering to overwrite a folder that exists;
 * returns 0, or -1 when it fails or is cancelled.
 */
copy_folder()
{
    new msg[256];
    new yes[32];
    new no[32];
    new ok[32];
    new ret;
    new src_path[256];
    new src_name[256];
    new dest_path[256];
    new count = count_hdd_folders();
    if (count >= MAX_FOLDERS)
    {
        get_tag_attr("TEXT", "msgutil_vmccopy_morefile", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        show_message(msg, ok);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        return -1;
    }
    if (!get_wnd_var_str("BRSRCNAME", src_name, 256))
    {
        get_tag_attr("TEXT", "msgutil_vmccopy_fail", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        show_message(msg, ok);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        return -1;
    }
    sprintf(src_path, "pfs:/__common/%s", src_name);
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
    new name[256];
    while (1)
    {
        new title[32];
        get_tag_attr("TEXT", "msgutil_hdddir_foldername", "value", title, 32);
        get_tag_attr("TEXT", "msgutil_hddcopy_new", "value", msg, 256);
        make_copy_name(msg, src_name, name);
        new entered = run_edit_dialog(title, name, 0, 256, 251);
        if (0 == entered)
        {
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
            return -1;
        }
        svdatStrTrmSpc(name, name, 256);
        if (strlen(name) <= 0)
        {
            get_tag_attr("TEXT", "msgutil_hddrename_nodata", "value", msg, 256);
            get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
            show_message(msg, ok);
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            continue;
        }
        ret = svdatChkFolderName(name);
        if (ret == NAME_TOO_LONG)
        {
            get_tag_attr("TEXT", "msgutil_hddcopy_longname", "value", msg, 256);
            get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
            show_message(msg, ok);
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            continue;
        }
        else if (ret == NAME_BAD_CHAR)
        {
            get_tag_attr("TEXT", "msgutil_hddrename_illigaldata", "value", msg, 256);
            get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
            show_message(msg, ok);
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            continue;
        }
        else if (ret == NAME_DOTS)
        {
            get_tag_attr("TEXT", "msgutil_hddrename_samesysdata", "value", msg, 256);
            get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
            show_message(msg, ok);
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            continue;
        }
        if (0 == strcmp(src_name, name))
        {
            get_tag_attr("TEXT", "msgutil_hddcopy_rename", "value", msg, 256);
            get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
            show_message(msg, ok);
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            continue;
        }
        sprintf(dest_path, "pfs:/__common/%s", name);
        new st_mode;
        new file_size;
        if (0 == fileLstat(dest_path, st_mode, file_size))
        {
            get_tag_attr("TEXT", "msgutil_hddrename_samedata", "value", msg, 256);
            get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
            show_message(msg, ok);
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            continue;
        }
        break;
    }
    new job;
    ret = svdatCopyFolder(src_path, dest_path, 0, job);
    if (ret < 0)
    {
        get_tag_attr("TEXT", "msgutil_vmccopy_fail", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        show_message(msg, ok);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        return -1;
    }
    new status;
    new percent;
    new waiting = 1;
    while (waiting)
    {
        ret = svdatSync(percent, status, job);
        if (ret < 0)
        {
            status = -1;
            break;
        }
        if (ret == 1)
        {
            break;
        }
        if (percent > 0)
        {
            break;
        }
        sleep;
    }
    if (status == SVDAT_EXISTS)
    {
        get_tag_attr("TEXT", "msgutil_hddcopy_overwrite_ask", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_yes", "value", yes, 32);
        get_tag_attr("TEXT", "msgutil_common_no", "value", no, 32);
        new answer = ask_yes_no(msg, yes, no, 1);
        if (0 != answer)
        {
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
            return -1;
        }
        ret = svdatCopyFolder(src_path, dest_path, 1, job);
        if (ret < 0)
        {
            get_tag_attr("TEXT", "msgutil_vmccopy_fail", "value", msg, 256);
            get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
            show_message(msg, ok);
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
            return -1;
        }
        status = 0;
    }
    if (0 == status && 0 == ret)
    {
        new progress_win = CreateNull(Disp2D(0));
        get_tag_attr("TEXT", "msgutil_vmccopy", "value", msg, 256);
        {
            new args[128];
            sprintf(args, "msg=%s", msg);
            SetPos(progress_win, 320.0, 224.0, 0.0, 0);
            AttachScript(progress_win, _, "progress_window", args);
            SetPubVar(progress_win, "manager", Self());
            SetPubVar(progress_win, "focus", focus_glowfly);
            SetPubVar(progress_win, "progress", 0);
        }
        waiting = 1;
        while (waiting)
        {
            ret = svdatSync(percent, status, job);
            if (ret == 1)
            {
                break;
            }
            if (ret < 0)
            {
                break;
            }
            SetPubVar(progress_win, "progress", percent);
            sleep;
        }
        if (status < 0 || ret < 0)
        {
            get_tag_attr("TEXT", "msgutil_vmccopy_fail", "value", msg, 256);
            get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
            show_message(msg, ok);
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            Destroy(progress_win);
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
            return -1;
        }
        printf(" ------- end = %d\n", ret);
        SetPubVar(progress_win, "progress", 100);
        while (SCRIPT_FINISHED != ScriptState(progress_win))
        {
            sleep;
        }
        Destroy(progress_win);
    }
    else if ((0 == status || status == SVDAT_SKIPPED) && ret == 1)
    {
        new progress_win = CreateNull(Disp2D(0));
        get_tag_attr("TEXT", "msgutil_vmccopy", "value", msg, 256);
        {
            new args[128];
            sprintf(args, "msg=%s", msg);
            SetPos(progress_win, 320.0, 224.0, 0.0, 0);
            AttachScript(progress_win, _, "progress_window", args);
            SetPubVar(progress_win, "manager", Self());
            SetPubVar(progress_win, "focus", focus_glowfly);
            SetPubVar(progress_win, "progress", 0);
        }
        SetPubVar(progress_win, "progress", 100);
        while (SCRIPT_FINISHED != ScriptState(progress_win))
        {
            sleep;
        }
        Destroy(progress_win);
    }
    if (status < 0 || ret < 0)
    {
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        get_tag_attr("TEXT", "msgutil_vmccopy_fail", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        show_message(msg, ok);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        return -1;
    }
    if (status == SVDAT_NO_SPACE)
    {
        get_tag_attr("TEXT", "msgutil_vmccopy_nospace_delete", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        show_message(msg, ok);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        return 0;
    }
    if (status == DISK_ERROR)
    {
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        return 0;
    }
    send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    if (status == SVDAT_SKIPPED)
    {
        new complete[64];
        new prohibited[64];
        get_tag_attr("TEXT", "msgutil_vmccopy_complete", "value", complete, 64);
        get_tag_attr("TEXT", "msgutil_hddcopy_complete_prohibit", "value", prohibited, 64);
        sprintf(msg, "%s\n%s", complete, prohibited);
    }
    else
    {
        get_tag_attr("TEXT", "msgutil_vmccopy_complete", "value", msg, 256);
    }
    get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
    show_message(msg, ok);
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
    return 0;
}

/* Renames the hard disk folder BRSRCNAME to a name the user enters; returns 0, or -1 when it fails or is cancelled. */
rename_folder()
{
    new msg[256];
    new ok[32];
    new ret;
    new src_path[256];
    new src_name[256];
    new dest_path[256];
    if (!get_wnd_var_str("BRSRCNAME", src_name, 256))
    {
        get_tag_attr("TEXT", "msgutil_hddrename_fail", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        show_message(msg, ok);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        return -1;
    }
    sprintf(src_path, "pfs:/__common/%s", src_name);
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
    new name[256];
    while (1)
    {
        new title[32];
        get_tag_attr("TEXT", "msgutil_hdddir_foldername", "value", title, 32);
        sprintf(name, "%s", src_name);
        new entered = run_edit_dialog(title, name, 0, 256, 251);
        if (0 == entered)
        {
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
            return -1;
        }
        svdatStrTrmSpc(name, name, 256);
        if (strlen(name) <= 0)
        {
            get_tag_attr("TEXT", "msgutil_hddrename_nodata", "value", msg, 256);
            get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
            show_message(msg, ok);
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
            continue;
        }
        ret = svdatChkFolderName(name);
        if (ret == NAME_TOO_LONG)
        {
            get_tag_attr("TEXT", "msgutil_hddcopy_longname", "value", msg, 256);
            get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
            show_message(msg, ok);
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            continue;
        }
        else if (ret == NAME_BAD_CHAR)
        {
            get_tag_attr("TEXT", "msgutil_hddrename_illigaldata", "value", msg, 256);
            get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
            show_message(msg, ok);
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            continue;
        }
        else if (ret == NAME_DOTS)
        {
            get_tag_attr("TEXT", "msgutil_hddrename_samesysdata", "value", msg, 256);
            get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
            show_message(msg, ok);
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            continue;
        }
        if (0 == strcmp(src_name, name))
        {
            goto done;
        }
        sprintf(dest_path, "pfs:/__common/%s", name);
        new st_mode;
        new file_size;
        if (0 == fileLstat(dest_path, st_mode, file_size))
        {
            get_tag_attr("TEXT", "msgutil_hddrename_samedata", "value", msg, 256);
            get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
            show_message(msg, ok);
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            continue;
        }
        break;
    }
    ret = fileRename(src_path, name);
    if (ret < 0)
    {
        get_tag_attr("TEXT", "msgutil_hddrename_fail", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        show_message(msg, ok);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        return -1;
    }
    new status;
    new percent;
    new waiting = 1;
    while (waiting)
    {
        ret = fileSync(percent, status);
        if (ret < 0)
        {
            status = -1;
            break;
        }
        if (ret == 1)
        {
            break;
        }
        if (percent > 0)
        {
            break;
        }
        sleep;
    }
    if (status == SVDAT_EXISTS)
    {
        get_tag_attr("TEXT", "msgutil_hddrename_samedata", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        show_message(msg, ok);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        return -1;
    }
    if (0 == status && 0 == ret)
    {
        new progress_win = CreateNull(Disp2D(0));
        get_tag_attr("TEXT", "msgutil_hddrename_wait", "value", msg, 256);
        {
            new args[128];
            sprintf(args, "msg=%s", msg);
            SetPos(progress_win, 320.0, 224.0, 0.0, 0);
            AttachScript(progress_win, _, "progress_window", args);
            SetPubVar(progress_win, "manager", Self());
            SetPubVar(progress_win, "focus", focus_glowfly);
        }
        waiting = 1;
        while (waiting)
        {
            ret = fileSync(percent, status);
            if (ret == 1)
            {
                break;
            }
            if (ret < 0)
            {
                break;
            }
            SetPubVar(progress_win, "progress", percent);
            sleep;
        }
        printf(" ------- end = %d\n", ret);
        SetPubVar(progress_win, "progress", 100);
        while (SCRIPT_FINISHED != ScriptState(progress_win))
        {
            sleep;
        }
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        Destroy(progress_win);
    }
    else if (0 == status && ret == 1)
    {
        new progress_win = CreateNull(Disp2D(0));
        get_tag_attr("TEXT", "msgutil_hddrename_wait", "value", msg, 256);
        {
            new args[128];
            sprintf(args, "msg=%s", msg);
            SetPos(progress_win, 320.0, 224.0, 0.0, 0);
            AttachScript(progress_win, _, "progress_window", args);
            SetPubVar(progress_win, "manager", Self());
            SetPubVar(progress_win, "focus", focus_glowfly);
        }
        SetPubVar(progress_win, "progress", 100);
        while (SCRIPT_FINISHED != ScriptState(progress_win))
        {
            sleep;
        }
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        Destroy(progress_win);
    }
    if (status < 0 || ret < 0)
    {
        get_tag_attr("TEXT", "msgutil_hddrename_fail", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        show_message(msg, ok);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        return -1;
    }
    if (status == DISK_ERROR)
    {
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        return 0;
    }
    done:
    get_tag_attr("TEXT", "msgutil_hddrename_complete", "value", msg, 256);
    get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
    show_message(msg, ok);
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
    return 0;
}

/* Deletes the hard disk folder BRSRCURL after asking; returns 0, or -1 when it fails or is cancelled. */
delete_folder()
{
    new msg[256];
    new yes[32];
    new no[32];
    new ok[32];
    new line1[64];
    new line2[64];
    new line3[64];
    new url[256];
    if (!get_wnd_var_str("BRSRCURL", url, 256))
    {
        get_tag_attr("TEXT", "msgutil_vmcdel_fail", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        show_message(msg, ok);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        return -1;
    }
    get_tag_attr("TEXT", "msgutil_hdddelete1", "value", line1, 64);
    get_tag_attr("TEXT", "msgutil_hdddelete2", "value", line2, 64);
    get_tag_attr("TEXT", "msgutil_vmcdel_ask", "value", line3, 64);
    sprintf(msg, "%s\n%s\n%s", line1, line2, line3);
    get_tag_attr("TEXT", "msgutil_common_yes", "value", yes, 32);
    get_tag_attr("TEXT", "msgutil_common_no", "value", no, 32);
    new answer = ask_yes_no(msg, yes, no, 1);
    if (0 != answer)
    {
        return -1;
    }
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
    new ret = fileDelete(url);
    if (ret < 0)
    {
        get_tag_attr("TEXT", "msgutil_vmcdel_fail", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        show_message(msg, ok);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        return -1;
    }
    new status;
    new percent;
    new waiting = 1;
    while (waiting)
    {
        ret = fileSync(percent, status);
        if (ret < 0)
        {
            status = -1;
            break;
        }
        if (ret == 1)
        {
            break;
        }
        if (percent > 0)
        {
            break;
        }
        sleep;
    }
    if (0 == status && 0 == ret)
    {
        new progress_win = CreateNull(Disp2D(0));
        get_tag_attr("TEXT", "msgutil_vmcdel", "value", msg, 256);
        {
            new args[128];
            sprintf(args, "msg=%s", msg);
            SetPos(progress_win, 320.0, 224.0, 0.0, 0);
            AttachScript(progress_win, _, "progress_window", args);
            SetPubVar(progress_win, "manager", Self());
            SetPubVar(progress_win, "focus", focus_glowfly);
        }
        waiting = 1;
        while (waiting)
        {
            ret = fileSync(percent, status);
            if (ret == 1)
            {
                break;
            }
            if (ret < 0)
            {
                break;
            }
            SetPubVar(progress_win, "progress", percent);
            sleep;
        }
        printf(" ------- end = %d\n", ret);
        SetPubVar(progress_win, "progress", 100);
        while (SCRIPT_FINISHED != ScriptState(progress_win))
        {
            sleep;
        }
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        Destroy(progress_win);
    }
    else if (0 == status && ret == 1)
    {
        new progress_win = CreateNull(Disp2D(0));
        get_tag_attr("TEXT", "msgutil_vmcdel", "value", msg, 256);
        {
            new args[128];
            sprintf(args, "msg=%s", msg);
            SetPos(progress_win, 320.0, 224.0, 0.0, 0);
            AttachScript(progress_win, _, "progress_window", args);
            SetPubVar(progress_win, "manager", Self());
            SetPubVar(progress_win, "focus", focus_glowfly);
        }
        SetPubVar(progress_win, "progress", 100);
        while (SCRIPT_FINISHED != ScriptState(progress_win))
        {
            sleep;
        }
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        Destroy(progress_win);
    }
    if (status < 0 || ret < 0)
    {
        get_tag_attr("TEXT", "msgutil_vmcdel_fail", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        show_message(msg, ok);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        return -1;
    }
    if (status == DISK_ERROR)
    {
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        return 0;
    }
    printf(" ------- end = %d\n", ret);
    send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    get_tag_attr("TEXT", "msgutil_vmcdel_complete", "value", msg, 256);
    get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
    show_message(msg, ok);
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
    return 0;
}

/*
 * Creates a hard disk folder with a name the user enters (up to 256 folders); returns 0, or -1 when it fails or is
 * cancelled.
 */
create_folder()
{
    new name[256];
    new path[256];
    new msg[256];
    new yes[32];
    new no[32];
    new ok[32];
    new ret;
    new count = count_hdd_folders();
    if (count >= MAX_FOLDERS)
    {
        get_tag_attr("TEXT", "msgutil_hdddir_make_max", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        show_message(msg, ok);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        return -1;
    }
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
    while (1)
    {
        new title[32];
        get_tag_attr("TEXT", "msgutil_hdddir_foldername", "value", title, 32);
        get_tag_attr("TEXT", "msgutil_hdddir_new", "value", msg, 256);
        make_folder_name(msg, name);
        new entered = run_edit_dialog(title, name, 0, 256, 251);
        if (0 == entered)
        {
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
            return -1;
        }
        svdatStrTrmSpc(name, name, 256);
        if (strlen(name) <= 0)
        {
            get_tag_attr("TEXT", "msgutil_hddrename_nodata", "value", msg, 256);
            get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
            show_message(msg, ok);
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            continue;
        }
        ret = svdatChkFolderName(name);
        if (ret == NAME_TOO_LONG)
        {
            get_tag_attr("TEXT", "msgutil_hddcopy_longname", "value", msg, 256);
            get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
            show_message(msg, ok);
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            continue;
        }
        else if (ret == NAME_BAD_CHAR)
        {
            get_tag_attr("TEXT", "msgutil_hddrename_illigaldata", "value", msg, 256);
            get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
            show_message(msg, ok);
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            continue;
        }
        else if (ret == NAME_DOTS)
        {
            get_tag_attr("TEXT", "msgutil_hddrename_samesysdata", "value", msg, 256);
            get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
            show_message(msg, ok);
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            continue;
        }
        sprintf(path, "pfs:/__common/%s", name);
        new st_mode;
        new file_size;
        if (0 == fileLstat(path, st_mode, file_size))
        {
            get_tag_attr("TEXT", "msgutil_hddrename_samedata", "value", msg, 256);
            get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
            show_message(msg, ok);
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            continue;
        }
        break;
    }
    new job;
    ret = svdatMkdir(path, job);
    if (ret < 0)
    {
        get_tag_attr("TEXT", "msgutil_hdddir_make_fail", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        show_message(msg, ok);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        return -1;
    }
    new percent;
    new status;
    new waiting = 1;
    while (waiting)
    {
        ret = svdatSync(percent, status, job);
        if (ret < 0)
        {
            status = -1;
            break;
        }
        if (ret == 1)
        {
            break;
        }
        if (percent > 0)
        {
            break;
        }
        sleep;
    }
    if (0 == status && 0 == ret)
    {
        new progress_win = CreateNull(Disp2D(0));
        get_tag_attr("TEXT", "msgutil_hdddir_make_wait", "value", msg, 256);
        {
            new args[128];
            sprintf(args, "msg=%s", msg);
            SetPos(progress_win, 320.0, 224.0, 0.0, 0);
            AttachScript(progress_win, _, "progress_window", args);
            SetPubVar(progress_win, "manager", Self());
            SetPubVar(progress_win, "focus", focus_glowfly);
        }
        waiting = 1;
        while (waiting)
        {
            ret = svdatSync(percent, status, job);
            if (ret == 1)
            {
                break;
            }
            if (ret < 0)
            {
                break;
            }
            SetPubVar(progress_win, "progress", percent);
            sleep;
        }
        if (status < 0 || ret < 0)
        {
            get_tag_attr("TEXT", "msgutil_hdddir_make_fail", "value", msg, 256);
            get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
            show_message(msg, ok);
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            Destroy(progress_win);
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
            return -1;
        }
        printf(" ------- end = %d\n", ret);
        SetPubVar(progress_win, "progress", 100);
        while (SCRIPT_FINISHED != ScriptState(progress_win))
        {
            sleep;
        }
        Destroy(progress_win);
    }
    else if (0 == status && ret == 1)
    {
        new progress_win = CreateNull(Disp2D(0));
        get_tag_attr("TEXT", "msgutil_hdddir_make_wait", "value", msg, 256);
        {
            new args[128];
            sprintf(args, "msg=%s", msg);
            SetPos(progress_win, 320.0, 224.0, 0.0, 0);
            AttachScript(progress_win, _, "progress_window", args);
            SetPubVar(progress_win, "manager", Self());
            SetPubVar(progress_win, "focus", focus_glowfly);
        }
        SetPubVar(progress_win, "progress", 100);
        while (SCRIPT_FINISHED != ScriptState(progress_win))
        {
            sleep;
        }
        Destroy(progress_win);
    }
    if (status < 0 || ret < 0)
    {
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        get_tag_attr("TEXT", "msgutil_hdddir_make_fail", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        show_message(msg, ok);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        return -1;
    }
    if (status == SVDAT_NO_SPACE)
    {
        get_tag_attr("TEXT", "msgutil_hdddir_nospace", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        show_message(msg, ok);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        return 0;
    }
    if (status == DISK_ERROR)
    {
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        return 0;
    }
    send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    get_tag_attr("TEXT", "msgutil_hdddir_make_complete", "value", msg, 256);
    get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
    show_message(msg, ok);
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
    return 0;
}

/* Returns the number of items an option menu has besides those of its MENUGROUP: none. */
count_extra_items(side, result, mode, title[], group[], id[], &selected)
{
    new unused; /* takes the parameters the function does not use */
    new buffer[64];
    unused = side;
    unused = result;
    unused = mode;
    unused = selected;
    copy_string(buffer, title, 64);
    copy_string(buffer, group, 64);
    copy_string(buffer, id, 64);
    unused = 0;
    return unused;
}

/* Leaves the label of an option menu item as it is. */
fill_menu_item(side, mode, index, group[], label[])
{
    new unused; /* takes the parameters the function does not use */
    new buffer[64];
    unused = side;
    unused = mode;
    unused = index;
    unused = 64;
    copy_string(buffer, group, unused);
    copy_string(buffer, label, unused);
}

/*
 * Handles a push in an option menu: circle runs the item id (ccopy, crename, cdelete, gnewfolder: a new folder
 * switches to the hard disk), MENU_ITEM if it succeeded and 0 if not; cross goes back.
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
    new result = MENU_REOPEN;
    if (button == TAG("RRGT"))
    {
        printf("stdmenu btn RRGT\n");
        focus_glowfly = GetPubVar(manager, "focus_glowfly");
        switch (mode)
        {
            case 0:
            {
                if (!strcmp("ccopy", id))
                {
                    new ret = copy_folder();
                    mode = 0;
                    if (ret >= 0)
                    {
                        result = MENU_ITEM;
                    }
                    else
                    {
                        result = 0;
                    }
                }
                else if (!strcmp("crename", id))
                {
                    new ret = rename_folder();
                    mode = 0;
                    if (ret >= 0)
                    {
                        result = MENU_ITEM;
                    }
                    else
                    {
                        result = 0;
                    }
                }
                else if (!strcmp("cdelete", id))
                {
                    new ret = delete_folder();
                    mode = 0;
                    if (ret >= 0)
                    {
                        result = MENU_ITEM;
                    }
                    else
                    {
                        result = 0;
                    }
                }
                else if (!strcmp("gnewfolder", id))
                {
                    new ret = create_folder();
                    mode = 0;
                    if (ret >= 0)
                    {
                        result = MENU_ITEM;
                        new buttons = GetPubVar(manager, "device_menu");
                        SendMessage(buttons, TAG("SLCT"), 1, 0, 0, 0);
                        SendMessage(manager, TAG("CHNG"), 1, 0, 0, 0);
                        sleep;
                    }
                    else
                    {
                        result = 0;
                    }
                }
            }
        }
    }
    if (button == TAG("RDWN"))
    {
        switch (mode)
        {
            case 0:
            {
                printf("stdmenu btn RDWN\n");
                mode = 0;
                result = MENU_BACK;
            }
        }
    }
    return result;
}

/*
 * Writes into dest the first of prefix name, prefix 1 name, prefix 2 name, ... (cut to 250 bytes) that is not a
 * folder of the hard disk; returns 0, or -1 with dest empty if they are all taken.
 */
make_copy_name(prefix[], name[], dest[])
{
    new i;
    new path[255];
    sprintf(dest, "%s%s", prefix, name);
    svdatStringCutByte(250, dest, dest, 0);
    svdatStrTrmSpc(dest, dest, 0);
    sprintf(path, "pfs:/__common/%s", dest);
    new st_mode;
    new file_size;
    if (0 != fileLstat(path, st_mode, file_size))
    {
        return 0;
    }
    i = 1;
    for (; i <= 257; i++)
    {
        sprintf(dest, "%s%d%s", prefix, i, name);
        svdatStringCutByte(250, dest, dest, 0);
        svdatStrTrmSpc(dest, dest, 0);
        sprintf(path, "pfs:/__common/%s", dest);
        if (0 != fileLstat(path, st_mode, file_size))
        {
            return 0;
        }
    }
    sprintf(dest, "");
    return -1;
}

/*
 * Writes into dest the first of format with 1, 2, ... that is not a folder of the hard disk; -1 if they are all taken.
 */
make_folder_name(format[], dest[])
{
    new i;
    new path[256];
    new st_mode;
    new file_size;
    i = 1;
    for (; i <= 257; i++)
    {
        sprintf(dest, format, i);
        sprintf(path, "pfs:/__common/%s", dest);
        if (0 != fileLstat(path, st_mode, file_size))
        {
            return 0;
        }
    }
    sprintf(dest, "");
    return -1;
}

/* Tells that the file system of the hard disk is damaged (TEXT msgutil_error_fsck1 to 4). */
show_disk_error()
{
    new line1[32];
    new line2[32];
    new line3[32];
    new line4[32];
    new msg[128];
    new ok[32];
    get_tag_attr("TEXT", "msgutil_error_fsck1", "value", line1, 32);
    get_tag_attr("TEXT", "msgutil_error_fsck2", "value", line2, 32);
    get_tag_attr("TEXT", "msgutil_error_fsck3", "value", line3, 32);
    get_tag_attr("TEXT", "msgutil_error_fsck4", "value", line4, 32);
    sprintf(msg, "%s%s\n%s%s", line1, line2, line3, line4);
    get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
    new unused = show_message(msg, ok); /* unused, but part of the layout */
}

/* Decodes the image url and keeps its texture; returns it, 0 on failure. */
load_kept_texture(url[])
{
    start_image_decode(url);
    if (IMG_DONE != finish_image_decode())
    {
        return 0;
    }
    new texture = imgTexID();
    imgKeepTex(texture);
    return texture;
}

/* Creates the three labels of the message of an empty album. */
create_message_labels(labels[])
{
    new Float:screen_w;
    new Float:screen_h;
    get_screen_size(screen_w, screen_h);
    labels[0] = CreateLabelShadow(Disp2D(0));
    SetPos(labels[0], 348.0 * screen_w / 640.0, 0.5 * screen_h, 0.0, 0);
    SetColor(labels[0], 0.7843, 0.7843, 0.7843, 0.0);
    SetOptionP(labels[0], TAG("SCL4"), g_message_shadow, 4);
    SetOptionI(labels[0], TAG("SX64"), 2 * 64);
    SetOptionI(labels[0], TAG("SY64"), 2 * 64);
    labels[1] = CreateLabelShadow(Disp2D(0));
    SetPos(labels[1], 348.0 * screen_w / 640.0, 0.48 * screen_h, 0.0, 0);
    SetColor(labels[1], 0.7843, 0.7843, 0.7843, 0.0);
    SetOptionP(labels[1], TAG("SCL4"), g_message_shadow, 4);
    SetOptionI(labels[1], TAG("SX64"), 2 * 64);
    SetOptionI(labels[1], TAG("SY64"), 2 * 64);
    labels[2] = CreateLabelShadow(Disp2D(0));
    SetPos(labels[2], 348.0 * screen_w / 640.0, 0.52 * screen_h, 0.0, 0);
    SetColor(labels[2], 0.7843, 0.7843, 0.7843, 0.0);
    SetOptionP(labels[2], TAG("SCL4"), g_message_shadow, 4);
    SetOptionI(labels[2], TAG("SX64"), 2 * 64);
    SetOptionI(labels[2], TAG("SY64"), 2 * 64);
    return;
}

/*
 * Shows in labels the message of an empty album of mode (no folder on the hard disk, no memory card), or nothing
 * for another mode, and fades them to alpha.
 */
show_empty_message(labels[], mode, Float:alpha, Float:duration)
{
    new line1[64];
    new line2[64];
    new line3[64];
    switch (mode)
    {
        case DEVMODE_MC:
        {
            get_tag_attr("TEXT", "msgutil_devinfo_mc_none1", "value", line2, 64);
            get_tag_attr("TEXT", "msgutil_devinfo_mc_none2", "value", line3, 64);
            sprintf(line1, "");
        }
        case DEVMODE_HDD:
        {
            get_tag_attr("TEXT", "msgutil_devinfo_folder_none", "value", line1, 64);
            sprintf(line2, "");
            sprintf(line3, "");
        }
        default:
        {
            sprintf(line1, "");
            sprintf(line2, "");
            sprintf(line3, "");
        }
    }
    SetOptionI(labels[0], TAG("FONT"), FontID(0));
    SetOptionI(labels[0], TAG("VALN"), ALIGN_CENTER);
    SetOptionI(labels[0], TAG("HALN"), ALIGN_LEFT);
    SetOptionI(labels[0], TAG("FSIZ"), 17);
    SetOptionS(labels[0], TAG("UTF8"), line1);
    AnimColor(labels[0], 0.7843, 0.7843, 0.7843, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    SetOptionI(labels[1], TAG("FONT"), FontID(0));
    SetOptionI(labels[1], TAG("VALN"), ALIGN_CENTER);
    SetOptionI(labels[1], TAG("HALN"), ALIGN_LEFT);
    SetOptionI(labels[1], TAG("FSIZ"), 17);
    SetOptionS(labels[1], TAG("UTF8"), line2);
    AnimColor(labels[1], 0.7843, 0.7843, 0.7843, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    SetOptionI(labels[2], TAG("FONT"), FontID(0));
    SetOptionI(labels[2], TAG("VALN"), ALIGN_CENTER);
    SetOptionI(labels[2], TAG("HALN"), ALIGN_LEFT);
    SetOptionI(labels[2], TAG("FSIZ"), 17);
    SetOptionS(labels[2], TAG("UTF8"), line3);
    AnimColor(labels[2], 0.7843, 0.7843, 0.7843, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    return;
}
