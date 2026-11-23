/*
 * /opt0/bn/script/utility/browser_thumb.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The thumb page of the file browser: the saves of the album opened in the browser as a grid of icons, with the
 * icon of the selected save turning in 3D and an option menu to copy, move or delete it. It declares its natives
 * itself instead of including bn.inc.
 */

#pragma dynamic 2560

native FrameRate();
native swapchars(c);
native Float:float(value);
native Float:floatmul(Float:oper1, Float:oper2);
native Float:floatdiv(Float:dividend, Float:divisor);
native Float:floatadd(Float:dividend, Float:divisor);
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
native domFirstChild(tag, const name[] = "");
native domInsertTag(tag, const name[]);
native domAddTag(tag, const name[]);
native domSetData(tag, const name[], const value[]);
native sprintf(const dest[], const format[], ...);
native atoi(const str[]);
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
native Float:floatsub(Float:oper1, Float:oper2);
native svdatMCInfo(port, slot, &job);
native svdatSync(&percent, &status, job);
native svdatGetMCInfo(port, slot, &card_type, &free_blocks, &formatted, &status);
native strncmp(const str1[], const str2[], count);
native printf(const format[], ...);
native svdatFolderClear();
native datOpen(type, const name[], order);
native datGetNum();
native avalGetNum(const name[]);
native avalGetValue(const name[], index, const value[], maxlength);
native datGetData(index, field, const value[], maxlength);
native svdatAddFolder(const name[], const comment[]);
native datClose();
native svdatCountData(const path[], const skip_folder[], &job);
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
native CreateNull(parent);
native SetPos(obj, Float:x, Float:y, Float:z, ref);
native SendMessageStr(obj, msg, const str[]);
native CreatePlane(parent);
native SetRot(obj, Float:x, Float:y, Float:z, ref, order);
native SetColor(obj, Float:r, Float:g, Float:b, Float:a);
native SetScale(obj, Float:x, Float:y, Float:z);
native SetOptionF(obj, option, Float:value);
native CreateDefocus(parent);
native AnimColor(obj, Float:r, Float:g, Float:b, Float:a, Float:duration, Float:delay, curve);
native CreateLabel(parent);
native SetOptionS(obj, option, const str[]);
native CreateEdit(parent, max_length);
native SetOptionP(obj, option, const {Float,_}:data[], cells);
native GetScale(obj, &Float:x, &Float:y, &Float:z);
native skbStart(obj, kind, input_mode, flags, length_unit, max_length, const chars[] = "", allow_only);
native getstring(const str[], maxlength);
native skbGetString(const str[], maxlength);
native Destroy(obj);
native imgTexID();
native GetSystemParam(id);
native GetSystemParamStr(id, const value[], maxlength);
native SetSystemParamStr(id, const value[]);
native svdatGetFreeSize(dev_mode, port, slot, const path[], &free_kb);
native floatround(Float:value, method=0);
native domNext(tag, const name[]);
native GetArg(const name[], const value[], maxlength);
native GetPubVar(obj, const name[]);
native CreateLabelShadow(parent);
native FontID(font);
native SetPubVar(obj, const name[], {Float,_}:value);
native RecvMessage(&msg, &{Float,_}:a, &b, &c, &d=0);
native ScriptState(obj);
native SetLocalScale(obj, Float:x, Float:y, Float:z);
native RecvMessageStr(&msg, const str[], maxlength);
native avalSet(const name[], const value[]);
native SetClipRange(obj, Float:near, Float:far);
native SetAmbient(obj, Float:r, Float:g, Float:b, Float:a);
native SetLightColor(obj, light, Float:r, Float:g, Float:b, a);
native SetLightVector(obj, light, Float:x, Float:y, Float:z);
native sndSePlay(se);
native AnimMoveHermite(obj, Float:x, Float:y, Float:z, Float:duration, Float:delay, curve, Float:velocity_scale);
native HideChildren(obj);
native datUpdateData(index, field, const value[], maxlength);
native ShowChildren(obj);
native iconCancel(handle);
native iconLoad(index, const url[], &handle, load_normal, load_copy, load_delete);
native iconSync(handle, &status, &damaged, const title1[], const title2[], size1, size2);
native LookAt(obj, target, Float:at_x, Float:at_y, Float:at_z, up_obj, Float:up_x, Float:up_y, Float:up_z);
native Hide(obj);
native CreateIcon(parent);
native AnimScale(obj, Float:x, Float:y, Float:z, Float:duration, Float:delay, curve);
native Show(obj);
native GetOptionP(obj, option, const data[], cells);
native GetOptionI(obj, option, &value);
native AnimMoveLine(obj, Float:x, Float:y, Float:z, Float:duration, Float:delay, curve);
native GetOptionF(obj, option, &Float:value);
native Float:cos(Float:angle);
native imgKeepTex(texture);
native svdatGetName(const path[], const name[], maxlength);
native svdatCopy(const src[], const dest[], overwrite, &job);
native svdatMove(const src[], const dest[], overwrite, &job);
native svdatDelete(const path[], &job);
native svdatChkFolderName(const name[]);
native fileLstat(const url[], &st_mode, &file_size);
native svdatMkdir(const path[], job);
native svdatMCFormat(port, slot, &job);
native svdatGetFolder(index, const name[], const comment[], name_size, comment_size);
native PadState(&held, &pressed, &released=0, &repeated=0, port);

#pragma rational Float
native Float:operator*(Float:oper1, Float:oper2) = floatmul;
native Float:operator/(Float:oper1, Float:oper2) = floatdiv;
native Float:operator+(Float:oper1, Float:oper2) = floatadd;
native Float:operator-(Float:oper1, Float:oper2) = floatsub;

/* As in bn.inc. */
const DAT_FILE_LIST = 0;
const DAT_ORDER_MTIME_DESC = 3;
const DAT_ORDER_CTCOUNT_DESC = 7;
const DAT_ORDER_MTCOUNT_DESC = 9;
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
const SYS_FGBAR_ALPHA = 504;
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
const MENU_SIDE_LEFT = 0;
const MENU_SIDE_RIGHT = 1;
const MENU_BACKDROP_FADED = 0x200;
const MENU_MODE_GROUP = 0;

/* As in scroll.inc. */
const SCROLL_LINE = 1;

/* As in genmenu.inc. */
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
const SIZE_MB_THOUSANDS = 1;
const SIZE_MB = 2;
const SIZE_KB_THOUSANDS = 3;
const SIZE_KB = 4;

#include "browser.inc"

/* More results of handle_menu_push: pop_std_menu stops at MENU_EXIT and above. */
const MENU_EXIT = 6;
const MENU_GO_TOP = MENU_ITEM + 1; /* greturn: back to the top page */

/* The answers of ask_yes_no and show_message when a memory card they watch is taken out. */
const CARD_EJECTED = -99; /* the card browsed */
const DEST_CARD_EJECTED = -98; /* the destination card */

/* The results of check_cards. */
const CHECK_CARD_GONE = 1; /* the card browsed is no longer there */
const CHECK_DEST_GONE = 2;
const CHECK_DEST_UNFORMATTED = 3;

/* Why format_card asks. */
const FORMAT_ON_OPEN = 0; /* the card browsed is not formatted */
const FORMAT_FOR_COPY = 1;
const FORMAT_FOR_MOVE = 2;

/* The mode of the option menu besides MENU_MODE_GROUP: the destinations of a copy or a move. */
const MENU_MODE_DEST = 1;

/* The modes of the icon view (message "MODE" to photo_focused). */
const VIEW_NORMAL = 0;
const VIEW_COPY = 1;
const VIEW_MOVE = 2;
const VIEW_DELETE = 3;
const VIEW_INFO = 4;

new g_top_url[] = "/opt0/bn/script/top.xml";

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

stock Float:operator/(Float:oper1, oper2) return floatdiv(oper1, float(oper2));

stock Float:operator+(Float:oper1, oper2) return floatadd(oper1, float(oper2));

stock bool:operator==(Float:oper1, Float:oper2) return floatcmp(oper1, oper2) == 0;

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

new Float:g_screen_width;
new Float:g_screen_height;
new Float:g_screen_depth;
new g_card_types[4]; /* the type of the card in each port, as last seen */
new g_folder_count; /* the hard disk folders in the destination list of svdat */
public device_mode; /* DEVMODE_* */
public mc_port; /* the port of the memory card shown (window variable BRMCPORT) */
public mc_slot; /* its slot (window variable BRMCSLOT) */

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

/* Tells whether path is on a memory card (mc00: or mc10:) and which; returns -1 for a longer path elsewhere. */
parse_card_path(path[], &is_card, &port, &slot)
{
    is_card = 0;
    port = 0;
    slot = 0;
    if (strlen(path) < 5)
    {
        return 0;
    }
    if (0 == strncmp(path, "mc00:", 5))
    {
        port = 0;
        slot = 0;
    }
    else if (0 == strncmp(path, "mc10:", 5))
    {
        port = 1;
        slot = 0;
    }
    else
    {
        return -1;
    }
    is_card = 1;
    return 0;
}

/*
 * Checks the memory card browsed and, with check_dest, the destination card in (port, slot); returns 0 or a
 * CHECK_* result.
 */
check_cards(check_dest, port, slot)
{
    new unused; /* unused, but part of the layout */
    new type;
    new free_blocks;
    new formatted;
    new status;
    if (device_mode == DEVMODE_MC || check_dest == 1)
    {
        update_card_info(unused, 0, 0, 0);
        printf("!!!!!!!!!!!! mc_check !!!!!!!!!!\n");
    }
    if (device_mode == DEVMODE_MC)
    {
        get_card_info(mc_port, mc_slot, type, free_blocks, formatted, status);
        printf("!!!!!!!!!!!! mc_check2 %d, %d !!!!!!!!!!\n", type, status);
        if (0 == type || status == MC_CHANGED || status == MC_CHANGED_UNFORMATTED)
        {
            return CHECK_CARD_GONE;
        }
        g_card_types[mc_port] = type;
    }
    if (check_dest)
    {
        get_card_info(port, slot, type, free_blocks, formatted, status);
        g_card_types[port] = type;
        if (0 == type || status == MC_CHANGED || status == MC_CHANGED_UNFORMATTED)
        {
            return CHECK_DEST_GONE;
        }
        if (0 != type && 0 == formatted)
        {
            return CHECK_DEST_UNFORMATTED;
        }
    }
    return 0;
}

/*
 * Puts the folders of the hard disk (at most 256) into the destination list of svdat, less the album browsed when it
 * is one of them; returns 0, or -1 on an error.
 */
list_hdd_folders(mode)
{
    new ret;
    svdatFolderClear();
    ret = datOpen(DAT_FILE_LIST, "pfs:/__common", DAT_ORDER_CTCOUNT_DESC);
    if (0 != ret)
    {
        return -1;
    }
    ret = wait_database();
    if (ret < 0)
    {
        return -1;
    }
    g_folder_count = datGetNum();
    if (g_folder_count > MAX_FOLDERS)
    {
        g_folder_count = MAX_FOLDERS;
    }
    new album[256];
    if (DEVMODE_HDD == mode)
    {
        if (avalGetNum("BrowserHDDNAME") <= 0)
        {
            return -1;
        }
        avalGetValue("BrowserHDDNAME", 0, album, 256);
    }
    new count;
    for (new i; i < g_folder_count; i++)
    {
        new name[256];
        new comment[384];
        datGetData(i, SAVE_NAME, name, 256);
        if (DEVMODE_HDD == mode)
        {
            if (0 == strcmp(album, name))
            {
                continue;
            }
        }
        datGetData(i, SAVE_COMMENT, comment, 384);
        svdatAddFolder(name, comment);
        count++;
    }
    datClose();
    g_folder_count = count;
    return 0;
}

/*
 * Opens the database of the saves of the album: the memory card of BrowserMCPort, the hard disk folder of
 * BrowserHDDURL or the virtual memory card; returns the number of saves, -1 on an error. The columns used: 0 the
 * state of the icon (0 not read, 1 read, 2 damaged), 1 the name, then the two title lines joined by "(>_<)", 2 the
 * url, 4 the kind (3, 4 and 5 have an icon), 7 flags (8: no copy).
 */
open_file_database(mode)
{
    new count;
    new port;
    new slot;
    new ret;
    new album[256];
    count = 0;
    switch (mode)
    {
        case DEVMODE_MC:
        {
            new value[256];
            if (avalGetNum("BrowserMCPort") <= 0)
            {
                return -1;
            }
            avalGetValue("BrowserMCPort", 0, value, 256);
            port = atoi(value);
            if (avalGetNum("BrowserMCSlot") <= 0)
            {
                return -1;
            }
            avalGetValue("BrowserMCSlot", 0, value, 256);
            slot = atoi(value);
            if (0 != slot)
            {
                return -1;
            }
            mc_port = port;
            mc_slot = slot;
            switch (port)
            {
                case 0:
                {
                    ret = datOpen(DAT_FILE_LIST, "mc00:/", DAT_ORDER_MTCOUNT_DESC);
                    if (0 != ret)
                    {
                        return -1;
                    }
                    sprintf(album, "Memory Card/1");
                }
                case 1:
                {
                    ret = datOpen(DAT_FILE_LIST, "mc10:/", DAT_ORDER_MTCOUNT_DESC);
                    if (0 != ret)
                    {
                        return -1;
                    }
                    sprintf(album, "Memory Card/2");
                }
                default:
                {
                    ret = -1;
                }
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
            count = datGetNum();
        }
        case DEVMODE_HDD:
        {
            if (avalGetNum("BrowserHDDNAME") <= 0)
            {
                return -1;
            }
            avalGetValue("BrowserHDDNAME", 0, album, 256);
            new url[256];
            if (avalGetNum("BrowserHDDURL") <= 0)
            {
                return -1;
            }
            avalGetValue("BrowserHDDURL", 0, url, 256);
            ret = datOpen(DAT_FILE_LIST, url, DAT_ORDER_MTCOUNT_DESC);
            if (0 != ret)
            {
                return -1;
            }
            ret = wait_database();
            if (ret < 0)
            {
                return -1;
            }
            count = datGetNum();
        }
        case DEVMODE_VMC:
        {
            new value[256];
            if (avalGetNum("BrowserMCPort") <= 0)
            {
                return -1;
            }
            avalGetValue("BrowserMCPort", 0, value, 256);
            port = atoi(value);
            if (avalGetNum("BrowserMCSlot") <= 0)
            {
                return -1;
            }
            avalGetValue("BrowserMCSlot", 0, value, 256);
            slot = atoi(value);
            if (0 != slot)
            {
                return -1;
            }
            mc_port = port;
            mc_slot = slot;
            switch (port)
            {
                case 0:
                {
                    ret = datOpen(DAT_FILE_LIST, "ps1vmc00:/", DAT_ORDER_MTIME_DESC);
                    sprintf(album, "Virtual Memory Card/1");
                }
                case 1:
                {
                    ret = datOpen(DAT_FILE_LIST, "ps1vmc10:/", DAT_ORDER_MTIME_DESC);
                    sprintf(album, "Virtual Memory Card/2");
                }
                default:
                {
                    ret = -1;
                }
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
            count = datGetNum();
        }
        default:
        {
            count = -1;
        }
    }
    return count;
}

/* Same as get_size_units of string.inc, for a size in KB. */
get_size_units(size_kb, &unit, &whole, &fraction)
{
    new count;
    unit = 0;
    whole = 0;
    fraction = 0;
    count = size_kb / 1024000;
    if (count > 0)
    {
        unit = SIZE_MB_THOUSANDS;
        whole = count;
        fraction = (size_kb - count * 1024000) / 1024;
        return;
    }
    count = size_kb / 1000;
    if (count > 0)
    {
        unit = SIZE_KB_THOUSANDS;
        whole = count;
        fraction = size_kb - count * 1000;
        return;
    }
    unit = SIZE_KB;
    whole = size_kb;
    return;
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

/* Returns the number of data in path (as svdatCountData counts them), negative on an error. */
count_folder_data(path[], skip_folder[])
{
    if (strlen(path) <= 0)
    {
        return -1;
    }
    new job;
    new ret = svdatCountData(path, skip_folder, job);
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

new g_common_bg_name[] = "SCEI:CommonBG";

/* Copies into dest the index-th field of the packed string src, whose fields are separated by sep. */
get_field(dest[], src[], index, sep[])
{
    new pos;
    new len;
    new k;
    new field;
    new src_len = strlen(src);
    new sep_len = strlen(sep);
    new sep_first = sep{0};
    while (pos < src_len)
    {
        new c = src{pos++};
        new at_sep;
        if (c == sep_first)
        {
            at_sep = 1;
            if (sep_len > 1)
            {
                k = 1;
                for (; k < sep_len; k++)
                {
                    if (src{pos - 1 + k} != sep{k})
                    {
                        break;
                    }
                }
                if (k == sep_len)
                {
                    pos = pos - 1 + sep_len;
                }
                else
                {
                    at_sep = 0;
                }
            }
        }
        if (at_sep)
        {
            if (field >= index)
            {
                break;
            }
            field++;
        }
        else if (field == index)
        {
            dest{len++} = c;
        }
    }
    dest{len} = 0;
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

/* Moves header line 0 to 2 of the box by (dx, dy, dz) over duration ms. */
move_box_header(line, dx, Float:dy, dz, duration)
{
    if (g_focus_box)
    {
        switch (line)
        {
            case 0:
            {
                SendMessage(g_focus_box, TAG("OFS0"), dx, dy, dz, duration);
            }
            case 1:
            {
                SendMessage(g_focus_box, TAG("OFS1"), dx, dy, dz, duration);
            }
            case 2:
            {
                SendMessage(g_focus_box, TAG("OFS2"), dx, dy, dz, duration);
            }
        }
    }
}

/* Sets the font size of header line 0 to 2 of the box. */
set_box_header_size(line, font_size)
{
    if (g_focus_box)
    {
        switch (line)
        {
            case 0:
            {
                SendMessage(g_focus_box, TAG("FSZ0"), font_size, 0, 0, 0);
            }
            case 1:
            {
                SendMessage(g_focus_box, TAG("FSZ1"), font_size, 0, 0, 0);
            }
            case 2:
            {
                SendMessage(g_focus_box, TAG("FSZ2"), font_size, 0, 0, 0);
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
fade_box_header(Float:alpha, duration, Float:red, Float:green, Float:blue)
{
    if (g_focus_box)
    {
        SendMessage(g_focus_box, TAG("FAD1"), alpha, duration, 0, 0);
        SendMessage(g_focus_box, TAG("FCL1"), red, green, blue, duration);
    }
}

new g_device_image;
new Float:g_device_x = 130.0;
new Float:g_device_y = 350.0;
new Float:g_device_w = 512.0;
new Float:g_device_h = 512.0;

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
public camera_focus; /* the point the camera looks at */
public camera_uptgt; /* unused, but part of the layout */
public focus_glowfly; /* the focus object */
public select = -1; /* the selected save */
public old_select = -1; /* the save whose icon and titles are shown */
public dst_mctype0; /* the type of the memory card in port 0 */
public dst_mctype1; /* the type of the memory card in port 1 */
new g_grid; /* the grid of the saves (browser_grid) */
new g_no_copy; /* the selected save cannot be copied */
new g_unformatted; /* the memory card browsed is not formatted */
new g_tile_front;
new g_tile_back;
new g_album_name[256];
/* The two lines at the bottom left: the album and the free space of a memory card, or only the album. */
new g_info_line1;
new g_info_line2;
public file_info; /* the information on the selected save (browser_fileinfo) */
new g_message_shadow[4] = {0x5F000000, 0x5F000000, 0x5F000000, 0x5F000000};
new g_mode_devices[4] = {DEVICE_HDD, DEVICE_MC, DEVICE_MC}; /* the device image of each DEVMODE_* */
public free_size; /* in KB, of the device of the album */

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

/* Reads the free space of the device of the album into free_size; returns 0 or -1. */
update_free_size()
{
    new ret = svdatGetFreeSize(device_mode, mc_port, mc_slot, "pfs://__common", free_size);
    return ret;
}

new g_file_count; /* the saves of the album */
new g_loaded_count; /* the saves whose icon is loaded */
new g_loading_index; /* the save whose icon is loading, -1 for none */
new g_icon_job; /* of iconLoad */
new g_icon_files[1024]; /* the number of the icon file (/opt0/cache/ictmpNNNN) of each save, -1 before it is loaded */
new g_load_delay; /* frames before the next icon is loaded in the background */
public photo_focused; /* the icon view, running photo_focused_main */
public stdmenu_curr_group; /* the MENUGROUP element of find_menu_group */

/* Sleeps for ms milliseconds, a Float. */
wait_ms_float(Float:ms)
{
    new frames = floatround(ms * float(FrameRate()) / 1000.0, 0);
    for (new i; i < frames; i++)
    {
        sleep;
    }
}

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

new g_title_shadow[4] = {0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000};
/* The parts of the option menu of std_menu_core, for fade_menu. */
new g_menu_backdrop;
new g_menu_title;
new g_menu_list;
new g_menu_scroll_bar;

/* Fades the option menu of std_menu_core to alpha (its backdrop to 0.3 of it). */
fade_menu(Float:alpha, Float:duration)
{
    if (g_menu_backdrop)
    {
        AnimColor(g_menu_backdrop, 0.0, 0.0, 0.0, 0.3 * alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    }
    if (g_menu_title)
    {
        AnimColor(g_menu_title, 0.902, 0.902, 0.215, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    }
    if (g_menu_list)
    {
        SendMessage(g_menu_list, TAG("FADE"), alpha, duration, 0, 0);
    }
    if (g_menu_scroll_bar)
    {
        SendMessage(g_menu_scroll_bar, TAG("FADE"), alpha, duration, 0, 0);
    }
}

/*
 * Runs on the object of pop_std_menu: waits for "INIT" (manager, side, result, mode), shows the title and a
 * general_menu of the items of the MENUGROUP group (mode 0) or of the destinations of a copy or a move (mode 1),
 * and, when a push leads to another menu, sends "TITL", "GRUP", "IDTY" and "FNSH" (result, mode, selected index)
 * to the manager. The memory cards are checked every 30 frames: a change closes the menu (MENU_EXIT) or brings
 * back the main one.
 */
public std_menu_core()
{
    new title[64];
    GetArg("title", title, 64);
    new group[64];
    GetArg("group", group, 64);
    new id[64];
    GetArg("id", id, 64);
    new border[64];
    GetArg("border", border, 64);
    g_menu_backdrop = atoi(border);
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
    new icon_view = GetPubVar(manager, "photo_focused");
    sleep;
    new title_label;
    new menu;
    new scroll_bar;
    new check_dest;
    new check_value;
    get_wnd_var_int("BRCHKDSTMC", check_value);
    if (check_value == 1)
    {
        check_dest = 1;
        goto rebuild;
    }
    check_dest = 0;
    rebuild:
    get_wnd_var_int("BRMCPORT", mc_port);
    get_wnd_var_int("BRDEVMODE", device_mode);
    new type;
    new free_blocks;
    new formatted;
    new status;
    if (device_mode == DEVMODE_MC)
    {
        get_card_info(mc_port, mc_slot, type, free_blocks, formatted, status);
        g_card_types[mc_port] = type;
        if (0 == type || status == MC_CHANGED || status == MC_CHANGED_UNFORMATTED)
        {
            new ejected = 1;
            set_wnd_var_int("BRMCEJECT", ejected);
            SendMessage(manager, TAG("FNSH"), MENU_EXIT, mode, 0, 0);
            return 0;
        }
    }
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
    new extra_count = count_extra_items(side, result, mode, title, group, id, selected, manager);
    if (extra_count == -1)
    {
        extra_count = 0;
        new ejected = 1;
        set_wnd_var_int("BRMCEJECT", ejected);
        result = MENU_EXIT;
        goto finish;
    }
    item_count = item_count + extra_count;
    if (item_count <= 0 || selected < 0)
    {
        mode = MENU_MODE_GROUP;
        result = MENU_RETURN;
        goto finish;
    }
    new shown = item_count >= 5 ? 5 : item_count;
    title_label = CreateLabelShadow(self);
    {
        new Float:x;
        new Float:y;
        get_list_title_pos(0, shown, 1, x, y);
        SetPos(title_label, 0.0 * screen_w, screen_h * y, 0.0, 0);
        SetColor(title_label, 0.902, 0.902, 0.215, 0.0);
        AnimColor(title_label, 0.902, 0.902, 0.215, 1.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
        SetOptionI(title_label, TAG("FONT"), FontID(0));
        SetOptionS(title_label, TAG("UTF8"), title);
        SetOptionI(title_label, TAG("FSIZ"), 17);
        SetOptionI(title_label, TAG("ZSRT"), ZSORT_OVERLAY);
        SetOptionI(title_label, TAG("HALN"), ALIGN_CENTER);
        SetOptionI(title_label, TAG("VALN"), ALIGN_CENTER);
        SetOptionP(title_label, TAG("SCL4"), g_title_shadow, 4);
        SetOptionI(title_label, TAG("SX64"), 2 * 64);
        SetOptionI(title_label, TAG("SY64"), 2 * 64);
    }
    sleep;
    menu = CreateNull(self);
    new columns = 1;
    new rows = 5;
    {
        new Float:y;
        get_list_top(shown, 1, y);
        SetPos(menu, 0.0 * screen_w, screen_h * y, 0.0, 0);
        SetOptionI(menu, TAG("ZSRT"), ZSORT_OVERLAY);
        AttachScript(menu, "general_menu", "general_menu");
        SetPubVar(menu, "manager", Self());
        SetPubVar(menu, "focus", GetPubVar(manager, "focus_glowfly"));
        SetPubVar(menu, "pad_enable", PAD_CROSS);
        rows = shown >= 8 ? 8 : shown;
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
    g_menu_title = title_label;
    g_menu_list = menu;
    g_menu_scroll_bar = scroll_bar;
    sleep;
    new frames;
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
                new value;
                get_wnd_var_int("BRCHKDSTMC", value);
                if (value == 1)
                {
                    check_dest = 1;
                }
                else
                {
                    check_dest = 0;
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
        if (frames > 30)
        {
            if (check_dest == 1)
            {
                new unused; /* unused, but part of the layout */
                update_card_info(unused, 0, 0, 0);
                get_card_info(0, 0, type, free_blocks, formatted, status);
                if (g_card_types[0] != type)
                {
                    if (device_mode == DEVMODE_MC && 0 == mc_port && 0 == mc_slot)
                    {
                        result = MENU_EXIT;
                        new ejected = 1;
                        set_wnd_var_int("BRMCEJECT", ejected);
                        SendMessage(menu, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                    }
                    else
                    {
                        mode = MENU_MODE_GROUP;
                        result = MENU_RETURN;
                        set_wnd_var_int("BRCHKDSTMC", 0);
                        SendMessage(icon_view, TAG("MODE"), VIEW_NORMAL, 1, 0, 0);
                        wait_ms(250);
                    }
                    g_card_types[0] = type;
                    break;
                }
                g_card_types[0] = type;
                get_card_info(1, 0, type, free_blocks, formatted, status);
                if (g_card_types[1] != type)
                {
                    if (device_mode == DEVMODE_MC && mc_port == 1 && 0 == mc_slot)
                    {
                        result = MENU_EXIT;
                        new ejected = 1;
                        set_wnd_var_int("BRMCEJECT", ejected);
                        SendMessage(menu, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                    }
                    else
                    {
                        mode = MENU_MODE_GROUP;
                        result = MENU_RETURN;
                        set_wnd_var_int("BRCHKDSTMC", 0);
                        SendMessage(icon_view, TAG("MODE"), VIEW_NORMAL, 1, 0, 0);
                        wait_ms(250);
                    }
                    g_card_types[1] = type;
                    break;
                }
                g_card_types[1] = type;
            }
            if (device_mode == DEVMODE_MC)
            {
                new unused; /* unused, but part of the layout */
                if (0 == check_dest)
                {
                    update_card_info(unused, 0, 0, 0);
                }
                get_card_info(mc_port, mc_slot, type, free_blocks, formatted, status);
                if (g_card_types[mc_port] != type || 0 == type || status == MC_CHANGED
                        || status == MC_CHANGED_UNFORMATTED)
                {
                    result = MENU_EXIT;
                    new ejected = 1;
                    set_wnd_var_int("BRMCEJECT", ejected);
                    SendMessage(menu, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                    break;
                }
                g_card_types[mc_port] = type;
            }
            else if (DEVMODE_HDD == device_mode)
            {
                new pfs_error;
                get_wnd_var_int("BRPFSERR", pfs_error);
                if (pfs_error == 1)
                {
                    result = MENU_EXIT;
                    SendMessage(menu, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                    show_disk_error();
                    break;
                }
            }
            frames = 0;
        }
        else
        {
            frames++;
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
    if (scroll_bar)
    {
        SendMessage(scroll_bar, TAG("FADE"), 0, 300.0, 0, 0);
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
 * Shows the option menu of the MENUGROUP group (prefix is not used) on the side of flags & 3, over a dark backdrop
 * if flags & 0x300, and the menus it leads to, until a result of MENU_EXIT or more, which it returns.
 */
public pop_std_menu(prefix[], group[], flags)
{
    new result;
    new mode;
    new side = flags & 3;
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
    copy_string(cur_group, group, 64);
    sleep;
    for (; ; )
    {
        new args[256];
        sprintf(args, "title = %s ; group = %s ; id = %s ; border = %d", cur_title, cur_group, cur_id, backdrop);
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
        if (result >= MENU_EXIT)
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

/*
 * The saves come from the album of the browser (argument device: HDD, MC or VMC; album variables BrowserAlbumName,
 * BrowserHDDNAME, BrowserHDDURL, BrowserMCPort, BrowserMCSlot). Triangle on a save opens the option menu, cross
 * goes back to the browser. The icons are loaded in the background, those around the selected save first, and the
 * memory card browsed is checked every 30 frames.
 */
main()
{
    avalSet("BrowserJumpFrom", "top");
    start_top_page(0);
    load_common_bg("bg", 1000.0);
    set_background_poor(600.0, 0);
    set_bg_cloud_size(128);
    new broken_title[32];
    get_tag_attr("TEXT", "msgutil_info_datatype_broken", "value", broken_title, 32);
    new self = Self();
    new Float:screen_w;
    new Float:screen_h;
    get_screen_size(screen_w, screen_h);
    create_camera();
    SetClipRange(Camera(0), 100.0, 32767.0);
    SetAmbient(Scene(0), 0.5, 0.5, 0.5, 1.0);
    SetLightColor(Scene(0), 0, 0.7, 0.7, 0.7, 0);
    SetLightVector(Scene(0), 0, 1.0, -1.0, -1.0);
    SetLightColor(Scene(0), 1, 0.3, 0.3, 0.3, 0);
    SetLightVector(Scene(0), 1, -1.0, -1.0, -1.0);
    SetLightColor(Scene(0), 2, 0.5, 0.5, 0.5, 0);
    SetLightVector(Scene(0), 2, 1.0, 1.0, 1.0);
    SetLightColor(Scene(0), 3, 0.0, 0.0, 0.0, 0);
    SetLightVector(Scene(0), 3, 0.0, 0.0, 1.0);
    g_tile_front = load_kept_texture("file:/opt0/bn/script/utility/texture/grid_a.tm2");
    g_tile_back = load_kept_texture("file:/opt0/bn/script/utility/texture/grid_b.tm2");
    device_mode = DEVMODE_HDD;
    {
        new device[64];
        GetArg("device", device, 64);
        printf("device %s\n", device);
        if (!strcmp(device, "HDD"))
        {
            device_mode = DEVMODE_HDD;
        }
        else if (!strcmp(device, "MC"))
        {
            device_mode = DEVMODE_MC;
        }
        else if (!strcmp(device, "VMC"))
        {
            device_mode = DEVMODE_VMC;
        }
    }
    list_hdd_folders(device_mode);
    focus_glowfly = create_focus_plane2("focus");
    new unused1; /* unused, but part of the layout */
    create_device_image();
    show_device_image(g_mode_devices[device_mode]);
    avalGetValue("BrowserAlbumName", 0, g_album_name, 256);
    create_info_lines();
    create_focus_box(488.0, 240.0);
    fade_box(0.0, 0.0, 1.0, 1.0, 1.0);
    set_box_header_size(0, 18);
    set_box_header_size(1, 18);
    fade_box_header(1.0, 0, 0.902, 0.902, 0.215);
    photo_focused = create_icon_view();
    file_info = create_file_info();
    new check_frames;
    if (device_mode == DEVMODE_MC)
    {
        new ejected;
        set_wnd_var_int("BRMCEJECT", ejected);
        new value[256];
        if (avalGetNum("BrowserMCPort") <= 0)
        {
            goto leave;
        }
        avalGetValue("BrowserMCPort", 0, value, 256);
        mc_port = atoi(value);
        if (avalGetNum("BrowserMCSlot") <= 0)
        {
            goto leave;
        }
        avalGetValue("BrowserMCSlot", 0, value, 256);
        mc_slot = atoi(value);
        if (0 != mc_slot)
        {
            goto leave;
        }
        check_frames = 0;
        new unused; /* unused, but part of the layout */
        new type;
        new free_blocks;
        new formatted;
        new status;
        update_card_info(unused, 0, 0, 0);
        get_card_info(mc_port, mc_slot, type, free_blocks, formatted, status);
        g_card_types[mc_port] = type;
        if (0 == type || status == MC_CHANGED || status == MC_CHANGED_UNFORMATTED)
        {
            goto leave;
        }
        if (0 != type && 0 == formatted)
        {
            g_unformatted = 1;
            new format_result = format_card(mc_port, mc_slot, FORMAT_ON_OPEN); /* unused, but part of the layout */
            get_wnd_var_int("BRMCEJECT", ejected);
            if (ejected == 1)
            {
                goto leave;
            }
            update_card_info(unused, 0, 0, 0);
            get_card_info(mc_port, mc_slot, type, free_blocks, formatted, status);
            g_card_types[mc_port] = type;
            if (0 == formatted)
            {
                g_unformatted = 1;
            }
            else
            {
                g_unformatted = 0;
            }
        }
        else
        {
            g_unformatted = 0;
        }
        if (0 == type)
        {
            goto leave;
        }
    }
    {
        new pfs_error;
        set_wnd_var_int("BRPFSERR", pfs_error);
    }
    update_free_size();
    printf("........... FREE SIZE = %d\n", free_size);
    show_info_lines(device_mode);
    new count = open_file_database(device_mode);
    if (count < 0)
    {
        fade_background(0, 0, 600.0);
        avalSet("BrowserJumpFrom", "player");
        new url[256];
        get_tag_attr("LINK", "Back", "href", url, 256);
        xmlLoadStart(url, _, _, XMLLOAD_MAIN);
        wait_xml_load();
        return -1;
    }
    g_file_count = count;
    g_loaded_count = 0;
    g_loading_index = -1;
    g_icon_job = 0;
    {
        new i;
        for (; i < 1024; i++)
        {
            g_icon_files[i] = -1;
        }
    }
    g_load_delay = 45;
    new unused2; /* unused, but part of the layout */
    g_grid = create_grid(count);
    SendMessage(g_grid, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
    new active = g_grid;
    new message_label = CreateLabelShadow(Disp2D(0));
    SetPos(message_label, 0.5 * screen_w, 0.5 * screen_h, 0.0, 0);
    SetColor(message_label, 0.7843, 0.7843, 0.7843, 0.0);
    SetOptionP(message_label, TAG("SCL4"), g_message_shadow, 4);
    SetOptionI(message_label, TAG("SX64"), 2 * 64);
    SetOptionI(message_label, TAG("SY64"), 2 * 64);
    if (device_mode == DEVMODE_MC && g_unformatted)
    {
        new text[64];
        get_tag_attr("TEXT", "msgutil_devinfo_unformat", "value", text, 64);
        show_message_label(message_label, text, 348.0, 240.0);
    }
    else if (count <= 0)
    {
        new text[64];
        get_tag_attr("TEXT", "msgutil_devinfo_nodata", "value", text, 64);
        show_message_label(message_label, text, 348.0, 240.0);
    }
    new cleared;
    new reload;
    for (; ; )
    {
        new msg;
        new a;
        new b;
        new c;
        /* From the grid: "PUSH" (index, button: RUP, "RDWN" or "RLFT"), "FOCS", "SCRL" (yview, dir). */
        while (RecvMessage(msg, a, b, c))
        {
            if (active == g_grid)
            {
                if (msg == TAG("PUSH"))
                {
                    if (b == TAG("RUP") && 1 != reload)
                    {
                        if (count > 0)
                        {
                            SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
                            sndSePlay(SE_OPEN);
                            sndSePlay(SE_WINDOW_OPEN);
                            fade_device_image(0.0, 400.0);
                            fade_info_lines(0.0, 400.0);
                            select = GetPubVar(g_grid, "cursor");
                            if (g_icon_files[select] < 0)
                            {
                                if (!load_icon_now(select))
                                {
                                    goto close;
                                }
                                SendMessage(g_grid, TAG("LOAD"), select, select, 0, 0);
                                old_select = -1;
                            }
                            if (select != old_select)
                            {
                                SendMessage(photo_focused, TAG("LOAD"), select, select, 1, 0);
                                sleep;
                            }
                            if (DEVMODE_HDD == device_mode)
                            {
                                new pfs_error;
                                get_wnd_var_int("BRPFSERR", pfs_error);
                                if (pfs_error == 1)
                                {
                                    goto close;
                                }
                            }
                            if (device_mode == DEVMODE_MC)
                            {
                                new unused; /* unused, but part of the layout */
                                update_card_info(unused, 0, 0, 0);
                                new type;
                                new free_blocks;
                                new formatted;
                                new status;
                                get_card_info(mc_port, mc_slot, type, free_blocks, formatted, status);
                                if (g_card_types[mc_port] != type || 0 == type || status == MC_CHANGED
                                        || status == MC_CHANGED_UNFORMATTED)
                                {
                                    goto close;
                                }
                                g_card_types[mc_port] = type;
                                check_frames = 0;
                            }
                            SendMessage(g_grid, TAG("FADE"), 0, 400.0, 0, 0);
                            SendMessage(photo_focused, TAG("FADS"), 0, 200.0, 0, 0);
                            SetSystemParam(SYS_FGBAR_ALPHA, 255);
                            show_message_label(message_label, "", 348.0, 240.0);
                            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                            fade_box(0.0, 400.0, 1.0, 1.0, 1.0);
                            AnimMoveHermite(Camera(0), 300.0 + 75.0, 0.0, 0.0, 750.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
                            AnimMoveHermite(camera_focus, 300.0 + 75.0, 0.0, -100.0, 750.0, 0.0, ANIM_FUNC_EXPONENT,
                                    1.0);
                            wait_ms_float(750.0 * 1.1);
                            new value[256];
                            new field[1];
                            datGetData(select, SAVE_NAME, value, 256);
                            set_wnd_var_str("BRSRCNAME", value);
                            datGetData(select, SAVE_COMMENT, value, 256);
                            set_wnd_var_str("BRSRCURL", value);
                            avalSet("BRSRCURL", value);
                            g_no_copy = get_no_copy(select);
                            set_wnd_var_int("BRSRCNOCOPY", g_no_copy);
                            set_wnd_var_int("BRSRCICONCACHE", g_icon_files[select]);
                            datGetData(select, SAVE_TYPE, field, 1);
                            set_wnd_var_int("BRSRCCONTTYPE", field[0]);
                            datGetData(select, SAVE_STATE, field, 1);
                            set_wnd_var_int("BRDAMAGED", field[0]);
                            set_wnd_var_int("BRDEVMODE", device_mode);
                            sprintf(value, "%d", device_mode);
                            avalSet("BRDEVMODE", value);
                            set_wnd_var_int("BRMCPORT", mc_port);
                            sprintf(value, "%d", mc_port);
                            avalSet("BRMCPORT", value);
                            set_wnd_var_int("BRMCSLOT", mc_slot);
                            sprintf(value, "%d", mc_slot);
                            avalSet("BRMCSLOT", value);
                            new card_type;
                            new card_free;
                            new card_formatted;
                            new card_status;
                            if (DEVMODE_MC != device_mode)
                            {
                                new unused; /* unused, but part of the layout */
                                update_card_info(unused, 0, 0, 0);
                            }
                            get_card_info(0, 0, card_type, card_free, card_formatted, card_status);
                            dst_mctype0 = card_type;
                            sprintf(value, "%d", dst_mctype0);
                            avalSet("BRMCTYPE0", value);
                            get_card_info(1, 0, card_type, card_free, card_formatted, card_status);
                            dst_mctype1 = card_type;
                            sprintf(value, "%d", dst_mctype1);
                            avalSet("BRMCTYPE1", value);
                            if (device_mode == DEVMODE_MC)
                            {
                                new type;
                                new free_blocks;
                                new formatted;
                                new status;
                                get_card_info(mc_port, mc_slot, type, free_blocks, formatted, status);
                                set_wnd_var_int("BRMCTYPE", type);
                            }
                            SendMessage(file_info, TAG("LOAD"), 0, 0, 0, 0);
                            SendMessage(file_info, TAG("FADE"), 1.0, 400.0, 0, 0);
                            HideChildren(g_grid);
                            wait_ms(500);
                            if (g_loaded_count < g_file_count)
                            {
                                if (g_loading_index >= 0)
                                {
                                    new status;
                                    new title[256];
                                    new title1[64];
                                    new title2[64];
                                    new damaged;
                                    new index = g_loading_index;
                                    new loading = poll_icon_load(status, index, damaged, title1, title2, 64, 64);
                                    sprintf(title, "%s(>_<)%s", title1, title2);
                                    new state[1];
                                    if (damaged == 1)
                                    {
                                        state[0] = STATE_DAMAGED;
                                    }
                                    else
                                    {
                                        new kind[1];
                                        datGetData(index, SAVE_TYPE, kind, 1);
                                        if (kind[0] == TYPE_PS2_SAVE || kind[0] == TYPE_PS1_SAVE
                                                || kind[0] == TYPE_PDA_SAVE)
                                        {
                                            state[0] = STATE_SOUND;
                                        }
                                        else
                                        {
                                            state[0] = STATE_DAMAGED;
                                        }
                                    }
                                    datUpdateData(index, SAVE_STATE, state, 1);
                                    datUpdateData(index, SAVE_NAME, title, 256);
                                    if (!loading)
                                    {
                                        if (status == ICON_CARD_ERROR)
                                        {
                                            goto close;
                                        }
                                        g_icon_files[index] = index;
                                        g_loading_index = -1;
                                        g_loaded_count++;
                                        SendMessage(g_grid, TAG("LOAD"), index, index, 0, 0);
                                        SendMessage(g_grid, TAG("FADE"), 0, 100.0, 0, 0);
                                        if (index == select)
                                        {
                                            old_select = -1;
                                        }
                                    }
                                }
                            }
                            new result = pop_std_menu("menu_title", "option_menu", 1);
                            ShowChildren(g_grid);
                            SendMessage(photo_focused, TAG("MODE"), VIEW_NORMAL, 1, 0, 0);
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
                            if (result == MENU_GO_TOP)
                            {
                                xmlLoadStart(g_top_url, _, _, XMLLOAD_MAIN);
                                wait_xml_load();
                                break;
                            }
                            if (!reload)
                            {
                                SendMessage(g_grid, TAG("FADE"), 1.0, 400.0, 0, 0);
                            }
                            else
                            {
                                SendMessage(photo_focused, TAG("FADE"), 0, 200.0, 0, 0);
                            }
                            SendMessage(file_info, TAG("FADE"), 0, 200.0, 0, 0);
                            center_camera();
                            wait_ms_float(750.0 * 0.5);
                            if (!reload)
                            {
                                SendMessage(photo_focused, TAG("FADS"), 1.0, 200.0, 0, 0);
                            }
                            wait_ms_float(750.0 * 0.5);
                            SetSystemParam(SYS_FGBAR_ALPHA, 100);
                            fade_info_lines(0.8, 400.0);
                            fade_box(1.0, 400.0, 1.0, 1.0, 1.0);
                        }
                        SendMessage(active, TAG("PUSH"), TAG("RLFT"), 0, 0, 0);
                        SendMessage(active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                    }
                    else if (b != TAG("RLFT"))
                    {
                        SendMessage(active, TAG("PUSH"), 0, 0, 0, 0);
                    }
                }
                else if (msg == TAG("FOCS"))
                {
                    SendMessage(active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                }
            }
            if (msg == TAG("PUSH"))
            {
                if (b == TAG("RDWN"))
                {
                    sndSePlay(SE_CANCEL);
                    goto close;
                }
                if (b == TAG("RLFT"))
                {
                    SendMessage(active, TAG("PUSH"), 0, 0, 0, 0);
                    SendMessage(active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                }
            }
            if (msg == TAG("SCRL"))
            {
                g_load_delay = 30;
            }
        }
        if (reload)
        {
            old_select = -1;
            if (device_mode == DEVMODE_MC)
            {
                new unused; /* unused, but part of the layout */
                update_card_info(unused, 0, 0, 0);
                new type;
                new free_blocks;
                new formatted;
                new status;
                get_card_info(mc_port, mc_slot, type, free_blocks, formatted, status);
                if (g_card_types[mc_port] != type || 0 == type || status == MC_CHANGED
                        || status == MC_CHANGED_UNFORMATTED)
                {
                    break;
                }
                g_card_types[mc_port] = type;
                check_frames = 0;
            }
            update_free_size();
            printf("........... FREE SIZE = %d\n", free_size);
            show_info_lines(device_mode);
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            count = reload_album(g_grid, device_mode);
            if (0 == count)
            {
                new text[64];
                get_tag_attr("TEXT", "msgutil_devinfo_nodata", "value", text, 64);
                show_message_label(message_label, text, 348.0, 240.0);
                active = g_grid;
            }
            else if (count > 0)
            {
                show_message_label(message_label, "", 348.0, 240.0);
                active = g_grid;
            }
            SendMessage(active, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
            reload = 0;
            sleep;
            continue;
        }
        if (count > 0)
        {
            select = GetPubVar(g_grid, "cursor");
            cleared = 0;
            if (select != old_select)
            {
                old_select = select;
                new icon_file = g_icon_files[select];
                SendMessage(photo_focused, TAG("LOAD"), select, icon_file, 0, 0);
                sleep;
                new title[128];
                new unused[1]; /* unused, but part of the layout */
                new state[1];
                datGetData(select, SAVE_STATE, state, 1);
                switch (state[0])
                {
                    case STATE_UNCHECKED:
                    {
                        sprintf(title, "");
                    }
                    case STATE_SOUND:
                    {
                        datGetData(select, SAVE_NAME, title, 128);
                    }
                    case STATE_DAMAGED:
                    {
                        copy_string(title, broken_title, 128);
                    }
                }
                new text[128];
                get_field(text, title, 0, "(>_<)");
                set_box_header(0, text);
                get_field(text, title, 1, "(>_<)");
                set_box_header(1, text);
                new Float:title_dy = 0 != strlen(text) ? -8.0 : 0.0;
                move_box_header(0, 0, title_dy, 0, 0);
                move_box_header(1, 0, -8.0, 0, 0);
                sprintf(text, "%d/%d", select + 1, count);
                set_box_counter(text);
                fade_box(1.0, 200.0, 1.0, 1.0, 1.0);
                if (icon_file < 0)
                {
                    get_tag_attr("TEXT", "msgmusic_album_getinfo", "value", text, 128);
                    show_message_label(message_label, text, 400.0, 240.0);
                }
                else
                {
                    show_message_label(message_label, "", 348.0, 240.0);
                }
            }
        }
        else if (0 == cleared)
        {
            SendMessage(photo_focused, TAG("LOAD"), select, -1, 0, 0);
            cleared = 1;
        }
        if (device_mode == DEVMODE_MC)
        {
            if (check_frames > 30)
            {
                new unused; /* unused, but part of the layout */
                update_card_info(unused, 0, mc_port, mc_slot);
                new type;
                new free_blocks;
                new formatted;
                new status;
                get_card_info(mc_port, mc_slot, type, free_blocks, formatted, status);
                if (g_card_types[mc_port] != type || 0 == type || status == MC_CHANGED
                        || status == MC_CHANGED_UNFORMATTED)
                {
                    printf("-------------- NORMAL MCEJECT !!!!\n");
                    break;
                }
                g_card_types[mc_port] = type;
                check_frames = 0;
            }
            else
            {
                check_frames++;
            }
            new ejected;
            get_wnd_var_int("BRMCEJECT", ejected);
            if (ejected == 1)
            {
                printf("-------------- BRMCEJECT !!!!\n");
                break;
            }
        }
        if (DEVMODE_HDD == device_mode)
        {
            new pfs_error;
            get_wnd_var_int("BRPFSERR", pfs_error);
            if (pfs_error == 1)
            {
                printf("-------------- BRPFSERR !!!!\n");
                break;
            }
        }
        if (g_loaded_count < g_file_count)
        {
            if (g_loading_index < 0)
            {
                if (g_load_delay > 0)
                {
                    g_load_delay--;
                }
                else
                {
                    new first = GetPubVar(g_grid, "yview") * 3;
                    new next = find_icon_to_load(first, 9);
                    if (next >= 0)
                    {
                        if (start_icon_load(next))
                        {
                            g_loading_index = next;
                        }
                        printf("load start %d %d\n", next, g_loading_index);
                    }
                }
            }
            else
            {
                new status;
                new title[256];
                new title1[64];
                new title2[64];
                new damaged;
                new index = g_loading_index;
                new loading = poll_icon_load(status, index, damaged, title1, title2, 64, 64);
                sprintf(title, "%s(>_<)%s", title1, title2);
                new state[1];
                if (damaged == 1)
                {
                    state[0] = STATE_DAMAGED;
                }
                else
                {
                    new kind[1];
                    datGetData(index, SAVE_TYPE, kind, 1);
                    if (kind[0] == TYPE_PS2_SAVE || kind[0] == TYPE_PS1_SAVE || kind[0] == TYPE_PDA_SAVE)
                    {
                        state[0] = STATE_SOUND;
                    }
                    else
                    {
                        state[0] = STATE_DAMAGED;
                    }
                }
                datUpdateData(index, SAVE_STATE, state, 1);
                datUpdateData(index, SAVE_NAME, title, 256);
                if (!loading)
                {
                    if (status == ICON_CARD_ERROR)
                    {
                        break;
                    }
                    g_icon_files[index] = index;
                    g_loading_index = -1;
                    g_loaded_count++;
                    SendMessage(g_grid, TAG("LOAD"), index, index, 0, 0);
                    if (index == select)
                    {
                        old_select = -1;
                    }
                }
            }
        }
        sleep;
    }
    close:
    SendMessage(g_grid, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
    send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    if (0 != g_icon_job)
    {
        iconCancel(g_icon_job);
    }
    if (count > 0)
    {
        SendMessage(photo_focused, TAG("FADE"), 0, 500.0, 0, 0);
        if (g_grid)
        {
            SendMessage(g_grid, TAG("FADE"), 0, 500.0, 0, 0);
        }
        fade_box(0.0, 500.0, 1.0, 1.0, 1.0);
        fade_info_lines(0.0, 500.0);
        show_message_label(message_label, "", 348.0, 240.0);
    }
    leave:
    fade_background(0, 0, 600.0);
    wait_ms(750);
    if (g_grid)
    {
        Destroy(g_grid);
    }
    if (focus_glowfly)
    {
        Destroy(focus_glowfly);
    }
    avalSet("BrowserJumpFrom", "player");
    {
        new url[256];
        get_tag_attr("LINK", "Back", "href", url, 256);
        xmlLoadStart(url, _, "mode=ThumbReturn", XMLLOAD_MAIN);
        wait_xml_load();
    }
    Destroy(self);
    return 0;
}

/* The order in which the icons around the selected save are loaded: columns, and rows times 3, from its own. */
new g_load_columns[9] = {0, 1, 2, 0, 1, 2, 2, 1};
new g_load_rows[9] = {0, 3, 6, 3, 6, 0, 6, 3};

/*
 * Returns the next save whose icon is not loaded: the selected one, then those of the count from first, in the
 * order of g_load_columns and g_load_rows; -1 if they are all loaded.
 */
find_icon_to_load(first, count)
{
    if (select >= 0 && g_icon_files[select] < 0)
    {
        return select;
    }
    new end = first + count;
    if (end > g_file_count)
    {
        end = g_file_count;
    }
    new pos = (select - first) % 9;
    if (pos < 0)
    {
        pos = pos + 9;
    }
    new column = pos % 3 * 3;
    new row = pos / 3 * 3;
    for (new i; i < 3; i++)
    {
        for (new j; j < 3; j++)
        {
            new index = first + g_load_columns[column + j] + g_load_rows[row + i];
            if (index < end && g_icon_files[index] < 0)
            {
                return index;
            }
        }
    }
    return -1;
}

/*
 * Loads the icon of save index and waits for it, after the load in progress; returns 0 if the memory card or the
 * hard disk failed.
 */
load_icon_now(index)
{
    if (g_icon_files[index] >= 0)
    {
        return 1;
    }
    new status;
    new damaged;
    new title[256];
    new title1[64];
    new title2[64];
    if (g_loading_index >= 0)
    {
        while (poll_icon_load(status, g_loading_index, damaged, title1, title2, 64, 64))
        {
            sleep;
        }
        g_loading_index = -1;
        if (status == ICON_CARD_ERROR)
        {
            return 0;
        }
        if (status == DISK_ERROR)
        {
            return 0;
        }
    }
    if (start_icon_load(index))
    {
        g_loading_index = index;
        while (poll_icon_load(status, index, damaged, title1, title2, 64, 64))
        {
            sleep;
        }
        sprintf(title, "%s(>_<)%s", title1, title2);
        new state[1];
        if (damaged == 1)
        {
            state[0] = STATE_DAMAGED;
        }
        else
        {
            new kind[1];
            datGetData(index, SAVE_TYPE, kind, 1);
            if (kind[0] == TYPE_PS2_SAVE || kind[0] == TYPE_PS1_SAVE || kind[0] == TYPE_PDA_SAVE)
            {
                state[0] = STATE_SOUND;
            }
            else
            {
                state[0] = STATE_DAMAGED;
            }
        }
        datUpdateData(index, SAVE_STATE, state, 1);
        datUpdateData(index, SAVE_NAME, title, 256);
        g_icon_files[index] = index;
        g_loaded_count++;
        g_loading_index = -1;
        if (status == DISK_ERROR)
        {
            return 0;
        }
        return ICON_CARD_ERROR != status;
    }
    return 0;
}

/*
 * Starts loading the icon of save index into /opt0/cache/ictmp<index> (the damaged icon for a damaged save or one
 * of a kind without an icon); returns 0 for an index out of the album.
 */
start_icon_load(index)
{
    if (index < 0 || index >= g_file_count)
    {
        return 0;
    }
    new url[256];
    new state[1];
    new kind_field[1];
    new damaged;
    datGetData(index, SAVE_STATE, state, 1);
    switch (state[0])
    {
        case STATE_UNCHECKED:
        {
            datGetData(index, SAVE_TYPE, kind_field, 1);
            new kind = kind_field[0];
            if (kind == TYPE_PS2_SAVE || kind == TYPE_PS1_SAVE || kind == TYPE_PDA_SAVE)
            {
                damaged = 0;
            }
            else
            {
                damaged = 1;
                state[0] = STATE_DAMAGED;
                datUpdateData(index, SAVE_STATE, state, 1);
            }
        }
        case STATE_SOUND:
        {
            damaged = 0;
        }
        case STATE_DAMAGED:
        {
            damaged = 1;
        }
        default:
        {
            damaged = 1;
        }
    }
    if (!damaged)
    {
        datGetData(index, SAVE_COMMENT, url, 256);
    }
    else
    {
        sprintf(url, "/opt0/bn/data/icon/damaged");
    }
    iconLoad(index, url, g_icon_job, 1, 1, 1);
    return 1;
}

/*
 * Returns 1 while the icon is loading; then status (0, ICON_CARD_ERROR, DISK_ERROR), damaged and the two
 * title lines of the save are set. index is not used.
 */
poll_icon_load(&status, index, &damaged, title1[], title2[], size1, size2)
{
    new ret = iconSync(g_icon_job, status, damaged, title1, title2, size1, size2);
    if (status == DISK_ERROR)
    {
        SendMessage(g_grid, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
        show_disk_error();
        SendMessage(g_grid, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        g_icon_job = 0;
        return 0;
    }
    if (status == ICON_CARD_ERROR)
    {
        g_icon_job = 0;
        return 0;
    }
    if (ret == 1)
    {
        g_icon_job = 0;
        return 0;
    }
    if (ret == 1) /* redundant as written */
    {
        g_icon_job = 0;
        return 0;
    }
    return 1;
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

/* Shows text on label at (x, y), fading it in; an empty text fades the label out. */
show_message_label(label, text[], Float:x, Float:y)
{
    SetOptionI(label, TAG("FONT"), FontID(0));
    SetOptionI(label, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(label, TAG("HALN"), ALIGN_LEFT);
    SetOptionI(label, TAG("FSIZ"), 17);
    new Float:alpha;
    if (0 != strlen(text))
    {
        SetOptionS(label, TAG("UTF8"), text);
        SetPos(label, x, y, 0.0, 0);
        SetColor(label, 0.7843, 0.7843, 0.7843, 0.0);
        alpha = 1.0;
    }
    AnimColor(label, 0.7843, 0.7843, 0.7843, alpha, 500.0, 0.0, ANIM_FUNC_LINEAR);
}

create_info_lines()
{
    g_info_line1 = CreateLabelShadow(Disp2D(0));
    SetPos(g_info_line1, 40.0, 390.0, 0.0, 0);
    SetColor(g_info_line1, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_info_line1, TAG("FONT"), FontID(0));
    SetOptionI(g_info_line1, TAG("FSIZ"), 14);
    SetOptionI(g_info_line1, TAG("W64 "), 304 * 64);
    SetOptionP(g_info_line1, TAG("SCL4"), g_message_shadow, 4);
    SetOptionI(g_info_line1, TAG("SX64"), 2 * 64);
    SetOptionI(g_info_line1, TAG("SY64"), 2 * 64);
    g_info_line2 = CreateLabelShadow(Disp2D(0));
    SetPos(g_info_line2, 40.0, 408.0, 0.0, 0);
    SetColor(g_info_line2, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_info_line2, TAG("FONT"), FontID(0));
    SetOptionI(g_info_line2, TAG("FSIZ"), 14);
    SetOptionI(g_info_line2, TAG("W64 "), 304 * 64);
    SetOptionP(g_info_line2, TAG("SCL4"), g_message_shadow, 4);
    SetOptionI(g_info_line2, TAG("SX64"), 2 * 64);
    SetOptionI(g_info_line2, TAG("SY64"), 2 * 64);
}

/* The TEXT of the free space, by the unit of get_size_units. */
new g_free_space_texts[5][32] = {
    "", "msgutil_info_freespace_mbyte2", "msgutil_info_freespace_mbyte1",
    "msgutil_info_freespace_kbyte2", "msgutil_info_freespace_kbyte1"
};

/* Shows the name of the album and, for a memory card, its free space (hidden while it is not formatted). */
show_info_lines(mode)
{
    if (DEVMODE_HDD == mode)
    {
        SetOptionS(g_info_line1, TAG("UTF8"), "");
        SetOptionS(g_info_line2, TAG("UTF8"), g_album_name);
    }
    else
    {
        new unused[256]; /* unused, but part of the layout */
        new text[128];
        new format[64];
        new unit;
        new whole;
        new fraction;
        get_size_units(free_size, unit, whole, fraction);
        get_tag_attr("TEXT", g_free_space_texts[unit], "value", format, 64);
        sprintf(text, format, whole, fraction);
        SetOptionS(g_info_line1, TAG("UTF8"), g_album_name);
        SetOptionS(g_info_line2, TAG("UTF8"), text);
    }
    AnimColor(g_info_line1, 1.0, 1.0, 1.0, 0.8, 1000.0, 0.0, ANIM_FUNC_LINEAR);
    if (g_unformatted == 1)
    {
        AnimColor(g_info_line2, 0.0, 0.0, 0.0, 0.0, 1000.0, 0.0, ANIM_FUNC_LINEAR);
    }
    else
    {
        AnimColor(g_info_line2, 1.0, 1.0, 1.0, 0.8, 1000.0, 0.0, ANIM_FUNC_LINEAR);
    }
}

fade_info_lines(Float:alpha, Float:duration)
{
    AnimColor(g_info_line1, 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_info_line2, 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
}

/* Creates the information on the selected save (browser_fileinfo), right of the grid. */
create_file_info()
{
    new info = CreateNull(Self());
    new Float:x;
    new Float:y;
    new Float:z;
    map_screen_point(x, y, z, 0.0, 0.0, 0);
    SetPos(info, 300.0 + 50.0, 0.0, z, 0);
    SetOptionI(info, TAG("ZSRT"), ZSORT_OVERLAY);
    AttachScript(info, _, "fileinfo");
    return info;
}

/* Creates the plane that runs photo_focused_main, right of the grid. */
create_icon_view()
{
    new Float:screen_w;
    new Float:screen_h;
    get_screen_size(screen_w, screen_h);
    new view;
    view = CreatePlane(Scene(0));
    {
        new Float:x;
        new Float:y;
        new Float:z;
        map_screen_point(x, y, z, 0.8 * screen_w, 0.55 * screen_h, 0);
        SetPos(view, x, y, z - 75.0, 0);
        SetColor(view, 1.0, 1.0, 1.0, 0.0);
        SetOptionI(view, TAG("FDEG"), 1);
        AttachScript(view, "photo_focused_main");
    }
    return view;
}

/*
 * Shows the icon of the selected save in 3D, turning, and plays the steps of the operations on it. The labels of
 * TEXT box take the colors of the icon little by little; behind the menus a plane of those colors covers the
 * screen.
 */
public photo_focused_main()
{
    new self = Self();
    new box_text[8];
    get_tag_attr("TEXT", "box", "value", box_text, 8);
    new box_label = CreateLabel(Disp2D(0));
    SetOptionI(box_label, TAG("ZSRT"), ZSORT_BACKGROUND);
    SetColor(box_label, 1.0, 1.0, 1.0, 0.0);
    SetPos(box_label, 488.0 + 45.0, 240.0 + 24.0, -125.0, 0);
    SetOptionS(box_label, TAG("UTF8"), box_text);
    SetLocalScale(box_label, 15.0, 15.0, 1.0);
    SetOptionI(box_label, TAG("FSIZ"), 20);
    SetOptionI(box_label, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(box_label, TAG("HALN"), ALIGN_CENTER);
    new cover = CreatePlane(Disp2D(0));
    SetOptionI(cover, TAG("ZSRT"), ZSORT_BACKGROUND);
    SetColor(cover, 1.0, 1.0, 1.0, 0.0);
    SetPos(cover, 320.0, 240.0, 0.0, 0);
    Hide(cover);
    SetLocalScale(cover, 640.0, 480.0, 1.0);
    static label_colors[4];
    static icon_colors[4];
    SetOptionP(box_label, TAG("COL4"), label_colors, 4);
    SetOptionP(cover, TAG("COL4"), label_colors, 4);
    new holder = CreateNull(self);
    new icon = CreateIcon(holder);
    new other_icon = CreateIcon(holder);
    SetLocalScale(icon, 217.0, 217.0, 217.0);
    SetLocalScale(other_icon, 217.0, 217.0, 217.0);
    SetOptionI(icon, TAG("AMBS"), 1);
    SetOptionI(other_icon, TAG("AMBS"), 1);
    SetOptionI(icon, TAG("LITS"), 1);
    SetOptionI(other_icon, TAG("LITS"), 1);
    SetOptionI(icon, TAG("SYNC"), 1);
    SetOptionI(other_icon, TAG("SYNC"), 1);
    SetOptionI(icon, TAG("FBUF"), 1);
    SetOptionI(other_icon, TAG("FBUF"), 1);
    SetOptionF(icon, TAG("PERS"), 2.0);
    SetOptionF(other_icon, TAG("PERS"), 2.0);
    new Float:light_rot[3];
    new shown_index = -100;
    new shown_file = -100;
    new path[256];
    new icon_type;
    new mode;
    new step;
    new unused; /* unused, but part of the layout */
    new color_frames;
    new cover_frames;
    new Float:angle;
    new Float:other_angle;
    new Float:holder_x;
    new Float:holder_y;
    new Float:icon_x;
    new Float:icon_y;
    for (; ; )
    {
        new msg;
        new Float:a;
        new b;
        new c;
        /*
         * "FADE" (alpha); "FADS" (0: the colored plane comes in front for a menu, else it goes, duration); "LOAD"
         * (index, number of the icon file or -1, force): slides to the icon of the save; "MODE" (VIEW_*, step), the
         * steps of an operation: 0 choosing the destination, 1 chosen, 2 working, 3 done, 4 failed, 5 the save is gone.
         */
        while (RecvMessage(msg, a, b, c))
        {
            if (msg == TAG("FADE"))
            {
                AnimColor(icon, 1.0, 1.0, 1.0, a, 300.0, 0.0, ANIM_FUNC_LINEAR);
                AnimColor(box_label, 1.0, 1.0, 1.0, a, 300.0, 0.0, ANIM_FUNC_LINEAR);
                AnimColor(cover, 1.0, 1.0, 1.0, a, 300.0, 0.0, ANIM_FUNC_LINEAR);
            }
            if (msg == TAG("FADS"))
            {
                new Float:alpha = a;
                new Float:duration = b;
                if (0.0 == alpha)
                {
                    SetOptionI(icon, TAG("ZSRT"), ZSORT_OVERLAY);
                    SetOptionI(other_icon, TAG("ZSRT"), ZSORT_OVERLAY);
                    AnimColor(box_label, 1.0, 1.0, 1.0, 0.0, duration, 0.0, ANIM_FUNC_LINEAR);
                    AnimScale(box_label, 2.0, 2.0, 2.0, duration, 0.0, ANIM_FUNC_EXPONENT);
                    AnimColor(cover, 1.0, 1.0, 1.0, 1.0, duration, duration, ANIM_FUNC_LINEAR);
                    Show(cover);
                    cover_frames = 0;
                }
                else
                {
                    SetOptionI(icon, TAG("ZSRT"), ZSORT_NONE);
                    SetOptionI(other_icon, TAG("ZSRT"), ZSORT_NONE);
                    AnimColor(box_label, 1.0, 1.0, 1.0, 1.0, duration, 0.0, ANIM_FUNC_LINEAR);
                    AnimScale(box_label, 1.0, 1.0, 1.0, duration, 0.0, ANIM_FUNC_EXPONENT);
                    AnimColor(cover, 1.0, 1.0, 1.0, 0.0, duration, 0.0, ANIM_FUNC_LINEAR);
                    cover_frames = 60;
                }
            }
            if (msg == TAG("LOAD"))
            {
                if (_:a == shown_index && b == shown_file && 0 == c)
                {
                    continue;
                }
                new delta = _:a - shown_index;
                shown_index = a;
                shown_file = b;
                new Float:dx;
                new Float:dy;
                switch (delta)
                {
                    case -3:
                    {
                        dy = 225.0;
                    }
                    case -2:
                    {
                        dx = 225.0;
                        dy = 225.0;
                    }
                    case -1:
                    {
                        dx = -225.0;
                    }
                    case 1:
                    {
                        if (0 != _:a % 3)
                        {
                            dx = 225.0;
                        }
                        else
                        {
                            dx = -225.0 * 2;
                            dy = -225.0;
                        }
                    }
                    case 2:
                    {
                        dx = -225.0;
                        dy = -225.0;
                    }
                    case 3:
                    {
                        dy = -225.0;
                    }
                }
                holder_x = holder_x - dx;
                holder_y = holder_y - dy;
                AnimMoveHermite(holder, holder_x, holder_y, 0.0, 300.0, 0.0, ANIM_FUNC_EXPONENT, 0.25);
                new swap = other_icon;
                other_icon = icon;
                icon = swap;
                icon_x = icon_x + dx;
                icon_y = icon_y + dy;
                SetPos(icon, icon_x, icon_y, 0.0, 0);
                SetScale(icon, 1.0, 1.0, 1.0);
                AnimColor(icon, 1.0, 1.0, 1.0, 1.0, 200.0, 0.0, ANIM_FUNC_EXPONENT);
                AnimColor(other_icon, 1.0, 1.0, 1.0, 0.0, 200.0, 0.0, ANIM_FUNC_EXPONENT);
                AnimScale(box_label, 1.0, 1.0, 1.0, 200.0, 0.0, ANIM_FUNC_EXPONENT);
                if (shown_file < 0)
                {
                    SetOptionS(icon, TAG("FILE"), "");
                    AnimColor(box_label, 1.0, 1.0, 1.0, 0.0, 400.0, 0.0, ANIM_FUNC_LINEAR);
                }
                else
                {
                    sprintf(path, "/opt0/cache/ictmp%04d", shown_file);
                    SetOptionS(icon, TAG("FILE"), path);
                    AnimColor(box_label, 1.0, 1.0, 1.0, 1.0, 400.0, 0.0, ANIM_FUNC_LINEAR);
                    GetOptionP(icon, TAG("COL4"), icon_colors, 4);
                    SetOptionP(cover, TAG("COL4"), icon_colors, 4);
                }
                GetOptionI(icon, TAG("TYPE"), icon_type);
                angle = -0.3;
                color_frames = 30;
                light_rot[1] = 0.0;
            }
            if (msg == TAG("MODE"))
            {
                mode = a;
                step = b;
                switch (mode)
                {
                    case VIEW_NORMAL:
                    {
                        AnimMoveLine(icon, icon_x, icon_y, 0.0, 200.0, 0.0, ANIM_FUNC_EXPONENT);
                        AnimScale(icon, 1.0, 1.0, 1.0, 200.0, 0.0, ANIM_FUNC_EXPONENT);
                        AnimScale(other_icon, 1.0, 1.0, 1.0, 200.0, 0.0, ANIM_FUNC_EXPONENT);
                        AnimColor(other_icon, 1.0, 1.0, 1.0, 0.0, 150.0, 0.0, ANIM_FUNC_LINEAR);
                        SetOptionS(icon, TAG("FILE"), path);
                        color_frames = 30;
                        AnimMoveLine(other_icon, icon_x, icon_y - 50.0, -50.0, 200.0, 0.0, ANIM_FUNC_EXPONENT);
                    }
                    case VIEW_COPY:
                    {
                        switch (step)
                        {
                            case 0:
                            {
                                AnimMoveLine(icon, icon_x, 120.0 + icon_y, 0.0, 200.0, 0.0, ANIM_FUNC_EXPONENT);
                                AnimScale(icon, 0.7, 0.7, 0.7, 200.0, 0.0, ANIM_FUNC_EXPONENT);
                                SetOptionS(icon, TAG("FILE"), path);
                            }
                            case 1:
                            {
                                new path_a[256];
                                sprintf(path_a, "%sa", path);
                                SetOptionS(icon, TAG("FILE"), path_a);
                            }
                            case 2:
                            {
                                SetOptionS(other_icon, TAG("FILE"), path);
                                SetPos(other_icon, icon_x, icon_y - 50.0 - 36.4, 0.0, 0);
                                if (icon_type == ICON_TYPE_PS2)
                                {
                                    SetScale(other_icon, 0.7, 0.35, 0.7);
                                    SetOptionF(other_icon, TAG("SPED"), 3.0);
                                }
                                else
                                {
                                    SetScale(other_icon, 0.35, 0.35, 0.35);
                                }
                                SetColor(other_icon, 1.0, 1.0, 1.0, 0.0);
                                AnimColor(other_icon, 1.0, 1.0, 1.0, 1.0, 150.0, 0.0, ANIM_FUNC_LINEAR);
                                other_angle = angle;
                            }
                            case 3:
                            {
                                AnimMoveLine(other_icon, icon_x, icon_y - 50.0, 0.0, 500.0, 0.0, ANIM_FUNC_EXPONENT);
                                AnimScale(other_icon, 0.7, 0.7, 0.7, 500.0, 0.0, ANIM_FUNC_EXPONENT);
                                SetOptionF(other_icon, TAG("SPED"), 1.0);
                                new Float:frame;
                                GetOptionF(icon, TAG("FRAM"), frame);
                                SetOptionF(other_icon, TAG("FRAM"), frame);
                                SetOptionS(icon, TAG("FILE"), path);
                            }
                            case 4:
                            {
                                AnimMoveLine(other_icon, icon_x, icon_y - 50.0 - 72.8, 0.0, 500.0, 0.0,
                                        ANIM_FUNC_EXPONENT);
                                AnimScale(other_icon, 0.0, 0.0, 0.0, 500.0, 0.0, ANIM_FUNC_EXPONENT);
                                AnimColor(other_icon, 1.0, 1.0, 1.0, 0.0, 150.0, 350.0, ANIM_FUNC_LINEAR);
                                SetOptionF(other_icon, TAG("SPED"), 1.0);
                                SetOptionS(icon, TAG("FILE"), path);
                            }
                        }
                    }
                    case VIEW_MOVE:
                    {
                        switch (step)
                        {
                            case 0:
                            {
                                AnimMoveLine(icon, icon_x, 120.0 + icon_y, 0.0, 200.0, 0.0, ANIM_FUNC_EXPONENT);
                                AnimScale(icon, 0.7, 0.7, 0.7, 200.0, 0.0, ANIM_FUNC_EXPONENT);
                                SetOptionS(icon, TAG("FILE"), path);
                            }
                            case 1:
                            {
                                new path_a[256];
                                sprintf(path_a, "%sa", path);
                                SetOptionS(icon, TAG("FILE"), path_a);
                            }
                            case 2:
                            {
                                SetOptionS(other_icon, TAG("FILE"), path);
                                SetPos(other_icon, icon_x, icon_y - 50.0 - 36.4, 0.0, 0);
                                if (icon_type == ICON_TYPE_PS2)
                                {
                                    SetScale(other_icon, 0.7, 0.35, 0.7);
                                    SetOptionF(other_icon, TAG("SPED"), 3.0);
                                }
                                else
                                {
                                    SetScale(other_icon, 0.35, 0.35, 0.35);
                                }
                                SetColor(other_icon, 1.0, 1.0, 1.0, 0.0);
                                AnimColor(other_icon, 1.0, 1.0, 1.0, 1.0, 150.0, 0.0, ANIM_FUNC_LINEAR);
                                other_angle = angle;
                            }
                            case 3:
                            {
                                AnimMoveLine(icon, icon_x, 130 + icon_y - 83.2, 0.0, 500.0, 0.0, ANIM_FUNC_EXPONENT);
                                AnimScale(icon, 0.0, 0.0, 0.0, 500.0, 0.0, ANIM_FUNC_EXPONENT);
                                AnimMoveLine(other_icon, icon_x, icon_y - 50.0, 0.0, 500.0, 0.0, ANIM_FUNC_EXPONENT);
                                AnimScale(other_icon, 0.7, 0.7, 0.7, 500.0, 0.0, ANIM_FUNC_EXPONENT);
                                SetOptionF(other_icon, TAG("SPED"), 1.0);
                            }
                            case 4:
                            {
                                AnimMoveLine(other_icon, icon_x, icon_y - 50.0 - 72.8, 0.0, 500.0, 0.0,
                                        ANIM_FUNC_EXPONENT);
                                AnimScale(other_icon, 0.0, 0.0, 0.0, 500.0, 0.0, ANIM_FUNC_EXPONENT);
                                AnimColor(other_icon, 1.0, 1.0, 1.0, 0.0, 150.0, 350.0, ANIM_FUNC_LINEAR);
                                SetOptionF(other_icon, TAG("SPED"), 1.0);
                                SetOptionS(icon, TAG("FILE"), path);
                            }
                            case 5:
                            {
                                path = "";
                            }
                        }
                    }
                    case VIEW_DELETE:
                    {
                        switch (step)
                        {
                            case 1:
                            {
                                AnimMoveLine(icon, icon_x, icon_y, 0.0, 200.0, 0.0, ANIM_FUNC_EXPONENT);
                                new path_b[256];
                                sprintf(path_b, "%sb", path);
                                SetOptionS(icon, TAG("FILE"), path_b);
                            }
                            case 3:
                            {
                                AnimMoveLine(icon, icon_x, icon_y - 104.0, 0.0, 500.0, 0.0, ANIM_FUNC_EXPONENT);
                                AnimScale(icon, 0.0, 0.0, 0.0, 500.0, 0.0, ANIM_FUNC_EXPONENT);
                            }
                            case 5:
                            {
                                path = "";
                            }
                        }
                    }
                    case VIEW_INFO:
                    {
                        AnimMoveLine(icon, 30.0 + icon_x, 145.0 + icon_y, 0.0, 300.0, 0.0, ANIM_FUNC_EXPONENT);
                        AnimScale(icon, 0.45, 0.45, 0.45, 300.0, 0.0, ANIM_FUNC_EXPONENT);
                    }
                }
            }
        }
        if (color_frames > 0)
        {
            color_frames--;
            if (0 == color_frames)
            {
                SetOptionS(other_icon, TAG("FILE"), "");
            }
            new weight = color_frames;
            for (new i; i < 4; i++)
            {
                new color;
                for (new shift; shift < 32; shift = shift + 8)
                {
                    new from = label_colors[i] >> shift & 0xFF;
                    new to = icon_colors[i] >> shift & 0xFF;
                    color = color | (from * weight + to) / (weight + 1) << shift;
                }
                label_colors[i] = color;
            }
            SetOptionS(box_label, TAG("UTF8"), box_text);
            SetOptionP(box_label, TAG("COL4"), label_colors, 4);
        }
        if (cover_frames > 0)
        {
            cover_frames--;
            if (0 == cover_frames)
            {
                Hide(cover);
            }
        }
        if (0 == icon_type)
        {
            GetOptionI(icon, TAG("TYPE"), icon_type);
        }
        else if (icon_type == ICON_TYPE_PS1)
        {
            new Float:rot_y = 0.2 * cos(angle) - 0.1;
            new Float:rot_x = 0.2 * cos(2.0 * angle);
            SetRot(icon, rot_x, rot_y, 0.0, 0, 0);
            angle = angle - 6.28 / 480;
            if (VIEW_NORMAL != mode)
            {
                if (VIEW_DELETE != mode)
                {
                    rot_y = 0.2 * cos(other_angle) - 0.1;
                    rot_x = 0.2 * cos(2.0 * other_angle);
                    SetRot(other_icon, rot_x, rot_y, 0.0, 0, 0);
                    other_angle = other_angle - 6.28 / 240;
                    if (step == 3)
                    {
                        other_angle = angle;
                    }
                }
            }
        }
        else
        {
            SetRot(icon, 0.0, angle, 0.0, 0, 0);
            light_rot[1] = 2.0 * angle;
            SetOptionP(icon, TAG("ROTL"), light_rot, 3);
            angle = angle - 6.28 / 720;
            if (VIEW_NORMAL != mode)
            {
                if (VIEW_DELETE != mode)
                {
                    SetRot(other_icon, 0.0, other_angle, 0.0, 0, 0);
                    SetOptionP(other_icon, TAG("ROTL"), light_rot, 3);
                    other_angle = other_angle - 6.28 / 720;
                }
                else if (step >= 2)
                {
                    angle = angle - 6.28 / 30;
                }
            }
        }
        sleep;
    }
    return;
}

/* Closes the database and the grid and opens them again; returns the number of saves, -1 on an error. */
reload_album(&grid, mode)
{
    fade_box(0.0, 400.0, 1.0, 1.0, 1.0);
    SendMessage(grid, TAG("FADE"), 0, 400, 0, 0);
    datClose();
    Destroy(grid);
    grid = 0;
    wait_database();
    new count = open_file_database(mode);
    if (count < 0)
    {
        return -1;
    }
    printf("new db opened %d\n", count);
    grid = create_grid(count);
    SendMessage(grid, TAG("FADE"), 1.0, 1000, 0, 0);
    if (g_loading_index >= 0)
    {
        new status;
        new damaged;
        new title1[64];
        new title2[64];
        while (poll_icon_load(status, g_loading_index, damaged, title1, title2, 64, 64))
        {
            sleep;
        }
        g_loading_index = -1;
    }
    g_file_count = count;
    g_loaded_count = 0;
    for (new i; i < 1024; i++)
    {
        g_icon_files[i] = -1;
    }
    return count;
}

/* Creates the grid of count saves (browser_grid), tilted on the left of the screen. */
create_grid(count)
{
    new Float:screen_w;
    new Float:screen_h;
    get_screen_size(screen_w, screen_h);
    new grid = CreateNull(Self());
    AttachScript(grid, _, "grid");
    SetPubVar(grid, "owner", Self());
    SetPubVar(grid, "focus", focus_glowfly);
    SetPubVar(grid, "size", count);
    SetPubVar(grid, "texture0", g_tile_front);
    SetPubVar(grid, "texture1", g_tile_back);
    SetPubVar(grid, "padhelp", PADHELP_CROSS | PADHELP_TRIANGLE);
    new Float:x;
    new Float:y;
    new Float:z;
    map_screen_point(x, y, z, 0.25 * screen_w, 0.5 * screen_h, Self());
    SetPos(grid, 1.5 * (40.0 + x), 1.5 * (10.0 + y), 1.5 * z, 0);
    SetScale(grid, 1.5, 1.5, 1.5);
    SetRot(grid, -0.75, 0.35, 0.0, 0, 0);
    return grid;
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

/*
 * Copies the selected save (window variable BRSRCURL) into dest, after the checks (no-copy flag, at most 1022 saves
 * in a hard disk folder, a PlayStation 2 save only to a PlayStation 2 memory card, an unformatted card), asking
 * before overwriting. Returns 0 when it is done or a memory card was taken out (then BRMCEJECT is set), -1 when it
 * fails or is cancelled, 1 when the card could not be formatted.
 */
copy_save(dest[])
{
    new msg[256];
    new yes[32];
    new no[32];
    new ok[32];
    new check;
    new content_type;
    if (strlen(dest) <= 0)
    {
        return -1;
    }
    new src_url[256];
    if (!get_wnd_var_str("BRSRCURL", src_url, 256))
    {
        get_tag_attr("TEXT", "msgutil_vmccopy_fail", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        new closed = show_message(msg, ok, 1, 1, 0, 0, 0);
        if (closed == CARD_EJECTED)
        {
            new ejected = 1;
            set_wnd_var_int("BRMCEJECT", ejected);
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            return 0;
        }
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        return -1;
    }
    get_wnd_var_int("BRSRCNOCOPY", g_no_copy);
    if (g_no_copy)
    {
        get_tag_attr("TEXT", "msgutil_mccopy_prohibit_", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        new closed = show_message(msg, ok, 1, 1, 0, 0, 0);
        if (closed == CARD_EJECTED)
        {
            new ejected = 1;
            set_wnd_var_int("BRMCEJECT", ejected);
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            return 0;
        }
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        return -1;
    }
    if (0 == strncmp(dest, "pfs:", 4))
    {
        new dest_path[256];
        new src_name[256];
        svdatGetName(src_url, src_name, 256);
        sprintf(dest_path, "%s/%s", dest, src_name);
        new count = count_folder_data(dest, dest_path);
        printf("..............current folder num = %d\n", count);
        if (count >= MAX_FOLDER_SAVES)
        {
            get_tag_attr("TEXT", "msgutil_hddcopy_datamany", "value", msg, 256);
            get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
            new closed = show_message(msg, ok, 1, 1, 0, 0, 0);
            if (closed == CARD_EJECTED)
            {
                new ejected = 1;
                set_wnd_var_int("BRMCEJECT", ejected);
                send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                return 0;
            }
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            return -1;
        }
    }
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
    new is_card;
    new port;
    new slot;
    parse_card_path(dest, is_card, port, slot);
    check = check_cards(is_card, port, slot);
    if (check == CHECK_CARD_GONE)
    {
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        new ejected = 1;
        set_wnd_var_int("BRMCEJECT", ejected);
        return 0;
    }
    if (is_card && check == CHECK_DEST_GONE)
    {
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        return 0;
    }
    get_wnd_var_int("BRSRCCONTTYPE", content_type);
    if (is_card)
    {
        if (content_type == TYPE_PS2_SAVE)
        {
            if (MC_TYPE_PS2 != g_card_types[port])
            {
                get_tag_attr("TEXT", "msgutil_hddcopy_fail_media", "value", msg, 256);
                get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
                new closed = show_message(msg, ok, 1, 1, 0, 0, 0);
                send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
                if (closed == CARD_EJECTED)
                {
                    new ejected = 1;
                    set_wnd_var_int("BRMCEJECT", ejected);
                    return 0;
                }
                return -1;
            }
        }
    }
    if (check == CHECK_DEST_UNFORMATTED)
    {
        new format_result = format_card(port, slot, FORMAT_FOR_COPY);
        if (format_result == 1)
        {
            return -1;
        }
        if (format_result < 0)
        {
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
            return 1;
        }
        check = check_cards(is_card, port, slot);
        if (check == CHECK_DEST_GONE || check == CHECK_CARD_GONE)
        {
            format_result = 0;
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
            if (format_result == CARD_EJECTED || check == CHECK_CARD_GONE) /* redundant as written */
            {
                new ejected = 1;
                set_wnd_var_int("BRMCEJECT", ejected);
            }
            return 0;
        }
    }
    check = check_cards(is_card, port, slot);
    if (check == CHECK_CARD_GONE)
    {
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        new ejected = 1;
        set_wnd_var_int("BRMCEJECT", ejected);
        return 0;
    }
    if (is_card && check == CHECK_DEST_GONE)
    {
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        return 0;
    }
    new job;
    new ret = svdatCopy(src_url, dest, 0, job);
    if (ret < 0)
    {
        get_tag_attr("TEXT", "msgutil_vmccopy_fail", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        ret = show_message(msg, ok, 1, 0, 0, 0, 0);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        if (ret == CARD_EJECTED)
        {
            new ejected = 1;
            set_wnd_var_int("BRMCEJECT", ejected);
            return 0;
        }
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
    if (status == SVDAT_EXISTS)
    {
        get_tag_attr("TEXT", "msgutil_hddcopy_overwrite_ask", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_yes", "value", yes, 32);
        get_tag_attr("TEXT", "msgutil_common_no", "value", no, 32);
        new answer = ask_yes_no(msg, yes, no, 1, 1, 1, is_card, port, slot);
        if (0 != answer)
        {
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
            if (answer == CARD_EJECTED)
            {
                new ejected = 1;
                set_wnd_var_int("BRMCEJECT", ejected);
                return 0;
            }
            if (answer == DEST_CARD_EJECTED)
            {
                return 0;
            }
            return -1;
        }
        ret = svdatCopy(src_url, dest, 1, job);
        if (ret < 0)
        {
            get_tag_attr("TEXT", "msgutil_vmccopy_fail", "value", msg, 256);
            get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
            ret = show_message(msg, ok, 1, 0, 0, 0, 0);
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
            if (ret == CARD_EJECTED)
            {
                new ejected = 1;
                set_wnd_var_int("BRMCEJECT", ejected);
                return 0;
            }
            return -1;
        }
        status = 0;
    }
    if (0 == status && 0 == ret)
    {
        new progress = CreateNull(Disp2D(0));
        get_tag_attr("TEXT", "msgutil_vmccopy", "value", msg, 256);
        {
            new args[128];
            if (device_mode == DEVMODE_MC || is_card == 1)
            {
                new wait_msg[64];
                get_tag_attr("TEXT", "msgutil_mcdel_wait", "value", wait_msg, 64);
                sprintf(args, "msg=%s;msg2=%s", msg, wait_msg);
            }
            else
            {
                sprintf(args, "msg=%s", msg);
            }
            AttachScript(progress, _, "progress_window", args);
            SetPubVar(progress, "manager", Self());
            SetPubVar(progress, "focus", focus_glowfly);
            SetPos(progress, 432.0, 224.0, 0.0, 0);
            SetPubVar(progress, "width", 340.0);
            SetPubVar(progress, "option_mode", 1);
        }
        SendMessage(photo_focused, TAG("MODE"), VIEW_COPY, 2, 0, 0);
        wait_ms(250);
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
            SetPubVar(progress, "progress", percent);
            sleep;
        }
        SetPubVar(progress, "progress", 99);
        if (status < 0 || ret < 0)
        {
            check = check_cards(is_card, port, slot);
        }
        else
        {
            check = 0;
        }
        new step = 0 == status ? 3 : 4;
        SendMessage(photo_focused, TAG("MODE"), VIEW_COPY, step, 0, 0);
        wait_ms(750);
        printf(" ------- end = %d\n", ret);
        SetPubVar(progress, "progress", 100);
        while (SCRIPT_FINISHED != ScriptState(progress))
        {
            sleep;
        }
        Destroy(progress);
    }
    else if (0 == status && ret == 1)
    {
        new progress = CreateNull(Disp2D(0));
        get_tag_attr("TEXT", "msgutil_vmccopy", "value", msg, 256);
        {
            new args[128];
            if (device_mode == DEVMODE_MC || is_card == 1)
            {
                new wait_msg[64];
                get_tag_attr("TEXT", "msgutil_mcdel_wait", "value", wait_msg, 64);
                sprintf(args, "msg=%s;msg2=%s", msg, wait_msg);
            }
            else
            {
                sprintf(args, "msg=%s", msg);
            }
            AttachScript(progress, _, "progress_window", args);
            SetPubVar(progress, "manager", Self());
            SetPubVar(progress, "focus", focus_glowfly);
            SetPos(progress, 432.0, 224.0, 0.0, 0);
            SetPubVar(progress, "width", 340.0);
            SetPubVar(progress, "option_mode", 1);
        }
        SendMessage(photo_focused, TAG("MODE"), VIEW_COPY, 2, 0, 0);
        wait_ms(250);
        SetPubVar(progress, "progress", 99);
        check = 0;
        new step = 3;
        SendMessage(photo_focused, TAG("MODE"), VIEW_COPY, step, 0, 0);
        wait_ms(750);
        printf(" ------- end = %d\n", ret);
        SetPubVar(progress, "progress", 100);
        while (SCRIPT_FINISHED != ScriptState(progress))
        {
            sleep;
        }
        Destroy(progress);
    }
    else
    {
        check = check_cards(is_card, port, slot);
    }
    if (status < 0 || ret < 0 || check == CHECK_DEST_GONE || check == CHECK_CARD_GONE)
    {
        get_tag_attr("TEXT", "msgutil_vmccopy_fail", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        ret = show_message(msg, ok, 1, 0, 0, 0, 0);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        if (ret == CARD_EJECTED || check == CHECK_CARD_GONE)
        {
            new ejected = 1;
            set_wnd_var_int("BRMCEJECT", ejected);
            return 0;
        }
        return -1;
    }
    if (status == SVDAT_NO_SPACE)
    {
        get_tag_attr("TEXT", "msgutil_vmccopy_nospace", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        ret = show_message(msg, ok, 1, 1, 0, 0, 0);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        if (ret == CARD_EJECTED)
        {
            new ejected = 1;
            set_wnd_var_int("BRMCEJECT", ejected);
        }
        return -1;
    }
    if (status == DISK_ERROR)
    {
        ret = show_disk_error();
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        if (ret == CARD_EJECTED)
        {
            new ejected = 1;
            set_wnd_var_int("BRMCEJECT", ejected);
        }
        return 0;
    }
    get_tag_attr("TEXT", "msgutil_vmccopy_complete", "value", msg, 256);
    get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
    ret = show_message(msg, ok, 1, 1, 0, 0, 0);
    send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
    if (ret == CARD_EJECTED)
    {
        new ejected = 1;
        set_wnd_var_int("BRMCEJECT", ejected);
    }
    return 0;
}

/* Same as copy_save, moving the save. */
move_save(dest[])
{
    new msg[256];
    new yes[32];
    new no[32];
    new ok[32];
    new check;
    new content_type;
    if (strlen(dest) <= 0)
    {
        return -1;
    }
    new src_url[256];
    if (!get_wnd_var_str("BRSRCURL", src_url, 256))
    {
        get_tag_attr("TEXT", "msgutil_vmcmove_fail", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        new closed = show_message(msg, ok, 1, 1, 0, 0, 0);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        if (closed == CARD_EJECTED)
        {
            new ejected = 1;
            set_wnd_var_int("BRMCEJECT", ejected);
            return 0;
        }
        return -1;
    }
    get_wnd_var_int("BRSRCNOCOPY", g_no_copy);
    if (g_no_copy)
    {
        get_tag_attr("TEXT", "msgutil_hddmove_prohibit", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        new closed = show_message(msg, ok, 1, 1, 0, 0, 0);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        if (closed == CARD_EJECTED)
        {
            new ejected = 1;
            set_wnd_var_int("BRMCEJECT", ejected);
            return 0;
        }
        return -1;
    }
    if (0 == strncmp(dest, "pfs:", 4))
    {
        new dest_path[256];
        new src_name[256];
        svdatGetName(src_url, src_name, 256);
        sprintf(dest_path, "%s/%s", dest, src_name);
        new count = count_folder_data(dest, dest_path);
        if (count >= MAX_FOLDER_SAVES)
        {
            get_tag_attr("TEXT", "msgutil_hddmove_datamany", "value", msg, 256);
            get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
            new closed = show_message(msg, ok, 1, 1, 0, 0, 0);
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            if (closed == CARD_EJECTED)
            {
                new ejected = 1;
                set_wnd_var_int("BRMCEJECT", ejected);
                return 0;
            }
            return -1;
        }
    }
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
    new is_card;
    new port;
    new slot;
    parse_card_path(dest, is_card, port, slot);
    check = check_cards(is_card, port, slot);
    if (check == CHECK_CARD_GONE)
    {
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        new ejected = 1;
        set_wnd_var_int("BRMCEJECT", ejected);
        return 0;
    }
    get_wnd_var_int("BRSRCCONTTYPE", content_type);
    if (is_card)
    {
        if (content_type == TYPE_PS2_SAVE)
        {
            if (MC_TYPE_PS2 != g_card_types[port])
            {
                get_tag_attr("TEXT", "msgutil_hddmove_prohibit_media", "value", msg, 256);
                get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
                new closed = show_message(msg, ok, 1, 1, 0, 0, 0);
                send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
                if (closed == CARD_EJECTED)
                {
                    new ejected = 1;
                    set_wnd_var_int("BRMCEJECT", ejected);
                    return 0;
                }
                return -1;
            }
        }
    }
    if (check == CHECK_DEST_UNFORMATTED)
    {
        new format_result = format_card(port, slot, FORMAT_FOR_MOVE);
        if (format_result == 1)
        {
            return -1;
        }
        if (format_result < 0)
        {
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
            return 1;
        }
        check = check_cards(is_card, port, slot);
        if (check == CHECK_DEST_GONE || check == CHECK_CARD_GONE)
        {
            format_result = 0;
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
            if (format_result == CARD_EJECTED || check == CHECK_CARD_GONE) /* redundant as written */
            {
                new ejected = 1;
                set_wnd_var_int("BRMCEJECT", ejected);
            }
            return 0;
        }
    }
    new job;
    new ret = svdatMove(src_url, dest, 0, job);
    if (ret < 0)
    {
        get_tag_attr("TEXT", "msgutil_vmcmove_fail", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        ret = show_message(msg, ok, 1, 0, 0, 0, 0);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        if (ret == CARD_EJECTED)
        {
            new ejected = 1;
            set_wnd_var_int("BRMCEJECT", ejected);
            return 0;
        }
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
    if (status == SVDAT_EXISTS)
    {
        get_tag_attr("TEXT", "msgutil_hddmove_overwrite_ask", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_yes", "value", yes, 32);
        get_tag_attr("TEXT", "msgutil_common_no", "value", no, 32);
        new answer = ask_yes_no(msg, yes, no, 1, 1, 1, is_card, port, slot);
        if (0 != answer)
        {
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
            if (answer == CARD_EJECTED)
            {
                new ejected = 1;
                set_wnd_var_int("BRMCEJECT", ejected);
                return 0;
            }
            if (answer == DEST_CARD_EJECTED)
            {
                return 0;
            }
            return -1;
        }
        ret = svdatMove(src_url, dest, 1, job);
        if (ret < 0)
        {
            get_tag_attr("TEXT", "msgutil_vmcmove_fail", "value", msg, 256);
            get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
            ret = show_message(msg, ok, 1, 0, 0, 0, 0);
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
            if (ret == CARD_EJECTED)
            {
                new ejected = 1;
                set_wnd_var_int("BRMCEJECT", ejected);
                return 0;
            }
            return -1;
        }
        status = 0;
    }
    if (0 == status && 0 == ret)
    {
        new progress = CreateNull(Disp2D(0));
        get_tag_attr("TEXT", "msgutil_vmcmove", "value", msg, 256);
        {
            new args[128];
            if (device_mode == DEVMODE_MC || is_card == 1)
            {
                new wait_msg[64];
                get_tag_attr("TEXT", "msgutil_mcdel_wait", "value", wait_msg, 64);
                sprintf(args, "msg=%s;msg2=%s", msg, wait_msg);
            }
            else
            {
                sprintf(args, "msg=%s", msg);
            }
            AttachScript(progress, _, "progress_window", args);
            SetPubVar(progress, "manager", Self());
            SetPubVar(progress, "focus", focus_glowfly);
            SetPos(progress, 432.0, 224.0, 0.0, 0);
            SetPubVar(progress, "width", 340.0);
            SetPubVar(progress, "option_mode", 1);
        }
        SendMessage(photo_focused, TAG("MODE"), VIEW_MOVE, 2, 0, 0);
        wait_ms(250);
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
            SetPubVar(progress, "progress", percent);
            sleep;
        }
        SetPubVar(progress, "progress", 99);
        if (status < 0 || ret < 0)
        {
            check = check_cards(is_card, port, slot);
        }
        else
        {
            check = 0;
        }
        if (0 == status)
        {
            SendMessage(photo_focused, TAG("MODE"), VIEW_MOVE, 3, 0, 0);
            wait_ms(1500);
            SendMessage(photo_focused, TAG("MODE"), VIEW_MOVE, 5, 0, 0);
        }
        else
        {
            SendMessage(photo_focused, TAG("MODE"), VIEW_MOVE, 4, 0, 0);
        }
        printf(" ------- end = %d\n", ret);
        SetPubVar(progress, "progress", 100);
        while (SCRIPT_FINISHED != ScriptState(progress))
        {
            sleep;
        }
        Destroy(progress);
    }
    else if (0 == status && ret == 1)
    {
        new progress = CreateNull(Disp2D(0));
        get_tag_attr("TEXT", "msgutil_vmcmove", "value", msg, 256);
        {
            new args[128];
            if (device_mode == DEVMODE_MC || is_card == 1)
            {
                new wait_msg[64];
                get_tag_attr("TEXT", "msgutil_mcdel_wait", "value", wait_msg, 64);
                sprintf(args, "msg=%s;msg2=%s", msg, wait_msg);
            }
            else
            {
                sprintf(args, "msg=%s", msg);
            }
            AttachScript(progress, _, "progress_window", args);
            SetPubVar(progress, "manager", Self());
            SetPubVar(progress, "focus", focus_glowfly);
            SetPos(progress, 432.0, 224.0, 0.0, 0);
            SetPubVar(progress, "width", 340.0);
            SetPubVar(progress, "option_mode", 1);
        }
        SendMessage(photo_focused, TAG("MODE"), VIEW_MOVE, 2, 0, 0);
        wait_ms(250);
        SetPubVar(progress, "progress", 99);
        check = 0;
        SendMessage(photo_focused, TAG("MODE"), VIEW_MOVE, 3, 0, 0);
        wait_ms(1500);
        SendMessage(photo_focused, TAG("MODE"), VIEW_MOVE, 5, 0, 0);
        printf(" ------- end = %d\n", ret);
        SetPubVar(progress, "progress", 100);
        while (SCRIPT_FINISHED != ScriptState(progress))
        {
            sleep;
        }
        Destroy(progress);
    }
    else
    {
        check = check_cards(is_card, port, slot);
    }
    if (status < 0 || ret < 0 || check == CHECK_DEST_GONE || check == CHECK_CARD_GONE)
    {
        get_tag_attr("TEXT", "msgutil_vmcmove_fail", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        ret = show_message(msg, ok, 1, 0, 0, 0, 0);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        if (ret == CARD_EJECTED || check == CHECK_CARD_GONE)
        {
            new ejected = 1;
            set_wnd_var_int("BRMCEJECT", ejected);
            return 0;
        }
        return -1;
    }
    if (status == SVDAT_NO_SPACE)
    {
        get_tag_attr("TEXT", "msgutil_vmcmove_nospace", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        ret = show_message(msg, ok, 1, 1, 0, 0, 0);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        if (ret == CARD_EJECTED)
        {
            new ejected = 1;
            set_wnd_var_int("BRMCEJECT", ejected);
        }
        return -1;
    }
    if (status == DISK_ERROR)
    {
        ret = show_disk_error();
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        if (ret == CARD_EJECTED)
        {
            new ejected = 1;
            set_wnd_var_int("BRMCEJECT", ejected);
        }
        return 0;
    }
    get_tag_attr("TEXT", "msgutil_vmcmove_complete", "value", msg, 256);
    get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
    ret = show_message(msg, ok, 1, 1, 0, 0, 0);
    send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
    if (ret == CARD_EJECTED)
    {
        new ejected = 1;
        set_wnd_var_int("BRMCEJECT", ejected);
    }
    return 0;
}

/* Deletes the selected save after asking; returns 0 when it is done or a memory card was taken out, -1 otherwise. */
delete_save()
{
    new msg[256];
    new yes[32];
    new no[32];
    new ok[32];
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
    new src_url[256];
    if (!get_wnd_var_str("BRSRCURL", src_url, 256))
    {
        get_tag_attr("TEXT", "msgutil_vmcdel_fail", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        new closed = show_message(msg, ok, 1, 1, 0, 0, 0);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        if (closed == CARD_EJECTED)
        {
            new ejected = 1;
            set_wnd_var_int("BRMCEJECT", ejected);
            return 0;
        }
        return -1;
    }
    get_tag_attr("TEXT", "msgutil_vmcdel_ask", "value", msg, 256);
    get_tag_attr("TEXT", "msgutil_common_yes", "value", yes, 32);
    get_tag_attr("TEXT", "msgutil_common_no", "value", no, 32);
    new answer = ask_yes_no(msg, yes, no, 1, 1, 1, 0, 0, 0);
    if (0 != answer)
    {
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        if (answer == CARD_EJECTED)
        {
            new ejected = 1;
            set_wnd_var_int("BRMCEJECT", ejected);
            return 0;
        }
        return -1;
    }
    new job;
    new ret = svdatDelete(src_url, job);
    if (ret < 0)
    {
        get_tag_attr("TEXT", "msgutil_vmcdel_fail", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        ret = show_message(msg, ok, 1, 0, 0, 0, 0);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        if (ret == CARD_EJECTED)
        {
            new ejected = 1;
            set_wnd_var_int("BRMCEJECT", ejected);
            return 0;
        }
        return -1;
    }
    new progress = CreateNull(Disp2D(0));
    get_tag_attr("TEXT", "msgutil_vmcdel", "value", msg, 256);
    {
        new args[128];
        if (device_mode == DEVMODE_MC)
        {
            new wait_msg[64];
            get_tag_attr("TEXT", "msgutil_mcdel_wait", "value", wait_msg, 64);
            sprintf(args, "msg=%s;msg2=%s", msg, wait_msg);
        }
        else
        {
            sprintf(args, "msg=%s", msg);
        }
        AttachScript(progress, _, "progress_window", args);
        SetPubVar(progress, "manager", Self());
        SetPubVar(progress, "focus", focus_glowfly);
        SetPos(progress, 432.0, 224.0, 0.0, 0);
        SetPubVar(progress, "width", 340.0);
        SetPubVar(progress, "option_mode", 1);
    }
    SendMessage(photo_focused, TAG("MODE"), VIEW_DELETE, 2, 0, 0);
    wait_ms(250);
    new percent;
    new status;
    new waiting = 1;
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
        SetPubVar(progress, "progress", percent);
        sleep;
    }
    SetPubVar(progress, "progress", 99);
    if (0 == status)
    {
        SendMessage(photo_focused, TAG("MODE"), VIEW_DELETE, 3, 0, 0);
        wait_ms(750);
        SendMessage(photo_focused, TAG("MODE"), VIEW_DELETE, 5, 0, 0);
    }
    else
    {
        SendMessage(photo_focused, TAG("MODE"), VIEW_DELETE, 4, 0, 0);
    }
    printf(" ------- end = %d\n", ret);
    SetPubVar(progress, "progress", 100);
    while (SCRIPT_FINISHED != ScriptState(progress))
    {
        sleep;
    }
    Destroy(progress);
    if (status < 0 || ret < 0)
    {
        new check;
        if (device_mode == DEVMODE_MC)
        {
            check = check_cards(0, 0, 0);
        }
        get_tag_attr("TEXT", "msgutil_vmcdel_fail", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        ret = show_message(msg, ok, 1, 0, 0, 0, 0);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        if (ret == CARD_EJECTED || check == CHECK_CARD_GONE)
        {
            new ejected = 1;
            set_wnd_var_int("BRMCEJECT", ejected);
            return 0;
        }
        return -1;
    }
    if (status == DISK_ERROR)
    {
        ret = show_disk_error();
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        if (ret == CARD_EJECTED)
        {
            new ejected = 1;
            set_wnd_var_int("BRMCEJECT", ejected);
        }
        return 0;
    }
    get_tag_attr("TEXT", "msgutil_vmcdel_complete", "value", msg, 256);
    get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
    ret = show_message(msg, ok, 1, 1, 0, 0, 0);
    send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
    if (ret == CARD_EJECTED)
    {
        new ejected = 1;
        set_wnd_var_int("BRMCEJECT", ejected);
    }
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
    new count = count_hdd_folders();
    if (count >= MAX_FOLDERS)
    {
        get_tag_attr("TEXT", "msgutil_hdddir_make_max", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        new closed = show_message(msg, ok, 0, 1, 0, 0, 0);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        if (closed == CARD_EJECTED)
        {
            new ejected = 1;
            set_wnd_var_int("BRMCEJECT", ejected);
            return 0;
        }
        return -1;
    }
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
    while (1)
    {
        get_tag_attr("TEXT", "msgutil_hdddir_new", "value", msg, 256);
        make_folder_name(msg, name);
        new entered = run_edit_dialog("Folder Name", name, 0, 256, 256);
        if (0 == entered)
        {
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
            return -1;
        }
        if (strlen(name) <= 0)
        {
            get_tag_attr("TEXT", "msgutil_hddrename_nodata", "value", msg, 256);
            get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
            new closed = show_message(msg, ok, 0, 1, 0, 0, 0);
            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            if (closed == CARD_EJECTED)
            {
                new ejected = 1;
                set_wnd_var_int("BRMCEJECT", ejected);
                SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
                return 0;
            }
            continue;
        }
        else
        {
            break;
        }
    }
    new ret = svdatChkFolderName(name);
    if (ret == NAME_TOO_LONG)
    {
        get_tag_attr("TEXT", "msgutil_hddcopy_longname", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        ret = show_message(msg, ok, 0, 1, 0, 0, 0);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        if (ret == CARD_EJECTED)
        {
            new ejected = 1;
            set_wnd_var_int("BRMCEJECT", ejected);
            return 0;
        }
        return -1;
    }
    if (ret == NAME_BAD_CHAR)
    {
        get_tag_attr("TEXT", "msgutil_hddrename_illigaldata", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        ret = show_message(msg, ok, 0, 1, 0, 0, 0);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        if (ret == CARD_EJECTED)
        {
            new ejected = 1;
            set_wnd_var_int("BRMCEJECT", ejected);
            return 0;
        }
        return -1;
    }
    if (ret == NAME_DOTS)
    {
        get_tag_attr("TEXT", "msgutil_hddrename_samesysdata", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        ret = show_message(msg, ok, 0, 1, 0, 0, 0);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        if (ret == CARD_EJECTED)
        {
            new ejected = 1;
            set_wnd_var_int("BRMCEJECT", ejected);
            return 0;
        }
        return -1;
    }
    sprintf(path, "pfs:/__common/%s", name);
    new st_mode;
    new file_size;
    if (0 == fileLstat(path, st_mode, file_size))
    {
        get_tag_attr("TEXT", "msgutil_hddrename_samedata", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        ret = show_message(msg, ok, 0, 1, 0, 0, 0);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        if (ret == CARD_EJECTED)
        {
            new ejected = 1;
            set_wnd_var_int("BRMCEJECT", ejected);
            return 0;
        }
        return -1;
    }
    get_tag_attr("TEXT", "msgutil_hdddir_make_ask", "value", msg, 256);
    get_tag_attr("TEXT", "msgutil_common_yes", "value", yes, 32);
    get_tag_attr("TEXT", "msgutil_common_no", "value", no, 32);
    new answer = ask_yes_no(msg, yes, no, 0, 0, 1, 0, 0, 0);
    if (0 != answer)
    {
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        if (answer == CARD_EJECTED)
        {
            new ejected = 1;
            set_wnd_var_int("BRMCEJECT", ejected);
            return 0;
        }
        return -1;
    }
    new progress = CreateNull(Disp2D(0));
    get_tag_attr("TEXT", "msgutil_hdddir_make_wait", "value", msg, 256);
    {
        new args[128];
        sprintf(args, "msg=%s", msg);
        SetPos(progress, 320.0, 224.0, 0.0, 0);
        AttachScript(progress, _, "progress_window", args);
        SetPubVar(progress, "manager", Self());
        SetPubVar(progress, "focus", focus_glowfly);
    }
    ret = svdatMkdir(path);
    if (ret < 0)
    {
        get_tag_attr("TEXT", "msgutil_hdddir_make_fail", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        ret = show_message(msg, ok, 0, 1, 0, 0, 0);
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        Destroy(progress);
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        if (ret == CARD_EJECTED)
        {
            new ejected = 1;
            set_wnd_var_int("BRMCEJECT", ejected);
            return 0;
        }
        return -1;
    }
    printf(" ------- end = %d\n", ret);
    SetPubVar(progress, "progress", 100);
    while (SCRIPT_FINISHED != ScriptState(progress))
    {
        sleep;
    }
    send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    Destroy(progress);
    get_tag_attr("TEXT", "msgutil_hdddir_make_complete", "value", msg, 256);
    get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
    ret = show_message(msg, ok, 0, 1, 0, 0, 0);
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
    if (ret == CARD_EJECTED)
    {
        new ejected = 1;
        set_wnd_var_int("BRMCEJECT", ejected);
    }
    return 0;
}

/*
 * Asks to format the unformatted memory card in (port, slot) and formats it; returns 0 when it is formatted or the
 * card browsed was taken out, 1 when the answer is no, -1 when it fails.
 */
format_card(port, slot, reason)
{
    new line1[64];
    new line2[64];
    new msg[256];
    new yes[32];
    new no[32];
    new ok[32];
    new other_card;
    if (mc_port == port && mc_slot == slot)
    {
        other_card = 0;
    }
    else
    {
        other_card = 1;
    }
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
    SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
    switch (reason)
    {
        case FORMAT_ON_OPEN:
        {
            get_tag_attr("TEXT", "msgutil_mcform_unform", "value", line1, 64);
            get_tag_attr("TEXT", "msgutil_mcform_unform_ask", "value", line2, 64);
            sprintf(msg, "%s\n%s", line1, line2);
        }
        case FORMAT_FOR_COPY:
        {
            get_tag_attr("TEXT", "msgutil_mcform_unform", "value", line1, 64);
            get_tag_attr("TEXT", "msgutil_mcform_ask", "value", line2, 64);
            sprintf(msg, "%s\n%s", line1, line2);
        }
        case FORMAT_FOR_MOVE:
        {
            get_tag_attr("TEXT", "msgutil_mcform_unform", "value", line1, 64);
            get_tag_attr("TEXT", "msgutil_hddmove_overwrite_form_ask", "value", line2, 64);
            sprintf(msg, "%s\n%s", line1, line2);
        }
    }
    get_tag_attr("TEXT", "msgutil_common_yes", "value", yes, 32);
    get_tag_attr("TEXT", "msgutil_common_no", "value", no, 32);
    new answer;
    if (other_card)
    {
        switch (reason)
        {
            case FORMAT_ON_OPEN:
            {
                answer = ask_yes_no(msg, yes, no, 0, 1, 1, 1, port, slot);
            }
            case FORMAT_FOR_COPY:
            {
                answer = ask_yes_no(msg, yes, no, 1, 1, 1, 1, port, slot);
            }
            case FORMAT_FOR_MOVE:
            {
                answer = ask_yes_no(msg, yes, no, 1, 1, 1, 1, port, slot);
            }
            default:
            {
                answer = -1;
            }
        }
    }
    else
    {
        switch (reason)
        {
            case FORMAT_ON_OPEN:
            {
                answer = ask_yes_no(msg, yes, no, 0, 1, 1, 0, 0, 0);
            }
            case FORMAT_FOR_COPY:
            {
                answer = ask_yes_no(msg, yes, no, 1, 1, 1, 0, 0, 0);
            }
            case FORMAT_FOR_MOVE:
            {
                answer = ask_yes_no(msg, yes, no, 1, 1, 1, 0, 0, 0);
            }
            default:
            {
                answer = -1;
            }
        }
    }
    if (0 != answer)
    {
        if (FORMAT_ON_OPEN == reason)
        {
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
            SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
        }
        if (answer == CARD_EJECTED)
        {
            new ejected = 1;
            set_wnd_var_int("BRMCEJECT", ejected);
            return 0;
        }
        if (answer == DEST_CARD_EJECTED)
        {
            return -1;
        }
        return 1;
    }
    new check;
    check = check_cards(other_card, port, slot);
    if (check == CHECK_CARD_GONE)
    {
        if (FORMAT_ON_OPEN == reason)
        {
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
            SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
        }
        new ejected = 1;
        set_wnd_var_int("BRMCEJECT", ejected);
        return -1;
    }
    if (other_card == 1 && check == CHECK_DEST_GONE)
    {
        if (FORMAT_ON_OPEN == reason)
        {
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
            SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
        }
        return -1;
    }
    new progress = CreateNull(Disp2D(0));
    switch (reason)
    {
        case FORMAT_ON_OPEN:
        {
            get_tag_attr("TEXT", "msgutil_mcform_unform_wait", "value", msg, 256);
        }
        case FORMAT_FOR_COPY:
        {
            get_tag_attr("TEXT", "msgutil_mcform_wait", "value", msg, 256);
        }
        case FORMAT_FOR_MOVE:
        {
            get_tag_attr("TEXT", "msgutil_hddmove_overwrite_form_wait", "value", msg, 256);
        }
    }
    {
        new args[128];
        sprintf(args, "msg=%s", msg);
        AttachScript(progress, _, "progress_window", args);
        SetPubVar(progress, "manager", Self());
        SetPubVar(progress, "focus", focus_glowfly);
        switch (reason)
        {
            case FORMAT_ON_OPEN:
            {
                SetPos(progress, 320.0, 224.0, 0.0, 0);
            }
            case FORMAT_FOR_COPY:
            {
                SetPos(progress, 432.0, 224.0, 0.0, 0);
                SetPubVar(progress, "width", 340.0);
                SetPubVar(progress, "option_mode", 1);
            }
            case FORMAT_FOR_MOVE:
            {
                SetPos(progress, 432.0, 224.0, 0.0, 0);
                SetPubVar(progress, "width", 340.0);
                SetPubVar(progress, "option_mode", 1);
            }
            default:
            {
                SetPos(progress, 320.0, 224.0, 0.0, 0);
            }
        }
    }
    check = check_cards(other_card, port, slot);
    if (check == CHECK_CARD_GONE)
    {
        if (FORMAT_ON_OPEN == reason)
        {
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
            SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
        }
        new ejected = 1;
        set_wnd_var_int("BRMCEJECT", ejected);
        return -1;
    }
    if (other_card == 1 && check == CHECK_DEST_GONE)
    {
        if (FORMAT_ON_OPEN == reason)
        {
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
            SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
        }
        return -1;
    }
    new job;
    new ret = svdatMCFormat(port, slot, job);
    if (ret < 0)
    {
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        Destroy(progress);
        if (FORMAT_ON_OPEN == reason)
        {
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
            SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
        }
        return -1;
    }
    new percent;
    new status;
    new waiting = 1;
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
        SetPubVar(progress, "progress", percent);
        sleep;
    }
    if (ret < 0 || status < 0)
    {
        if (device_mode == DEVMODE_MC)
        {
            check = check_cards(0, 0, 0);
        }
        send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        Destroy(progress);
        get_tag_attr("TEXT", "msgutil_mcform_fail", "value", msg, 256);
        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
        switch (reason)
        {
            case FORMAT_ON_OPEN:
            {
                ret = show_message(msg, ok, 0, 0, 0, 0, 0);
            }
            case FORMAT_FOR_COPY:
            {
                ret = 0;
            }
            case FORMAT_FOR_MOVE:
            {
                ret = 0;
            }
            default:
            {
                ret = show_message(msg, ok, 0, 0, 0, 0, 0);
            }
        }
        if (FORMAT_ON_OPEN == reason)
        {
            SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
            SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
        }
        if (ret == CARD_EJECTED || check == CHECK_CARD_GONE)
        {
            new ejected = 1;
            set_wnd_var_int("BRMCEJECT", ejected);
            return 0;
        }
        if (other_card == 1 && check == CHECK_DEST_GONE)
        {
            if (FORMAT_ON_OPEN == reason)
            {
                SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
                SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
            }
            return -1;
        }
        return -1;
    }
    SetPubVar(progress, "progress", 100);
    while (SCRIPT_FINISHED != ScriptState(progress))
    {
        sleep;
    }
    printf(" ------- end = %d\n", ret);
    send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    Destroy(progress);
    get_tag_attr("TEXT", "msgutil_mcform_unform_complete", "value", msg, 256);
    get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
    if (other_card)
    {
        switch (reason)
        {
            case FORMAT_ON_OPEN:
            {
                ret = show_message(msg, ok, 0, 1, 1, port, slot);
            }
            case FORMAT_FOR_COPY:
            {
                ret = show_message(msg, ok, 1, 1, 1, port, slot);
            }
            case FORMAT_FOR_MOVE:
            {
                ret = show_message(msg, ok, 1, 1, 1, port, slot);
            }
            default:
            {
                ret = show_message(msg, ok, 0, 1, 1, port, slot);
            }
        }
    }
    else
    {
        switch (reason)
        {
            case FORMAT_ON_OPEN:
            {
                ret = show_message(msg, ok, 0, 1, 0, 0, 0);
            }
            case FORMAT_FOR_COPY:
            {
                ret = show_message(msg, ok, 1, 1, 0, 0, 0);
            }
            case FORMAT_FOR_MOVE:
            {
                ret = show_message(msg, ok, 1, 1, 0, 0, 0);
            }
            default:
            {
                ret = show_message(msg, ok, 0, 1, 0, 0, 0);
            }
        }
    }
    if (FORMAT_ON_OPEN == reason)
    {
        SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
        SetSystemParam(SYS_ENABLE_NAVIMENU, 1);
    }
    if (ret == CARD_EJECTED)
    {
        new ejected = 1;
        set_wnd_var_int("BRMCEJECT", ejected);
        return -1;
    }
    if (ret == DEST_CARD_EJECTED)
    {
        return -1;
    }
    return 0;
}

new g_dest_card_count;
/* The kinds of the items of g_dest_cards: the memory card of port 0 or 1; the PS1 virtual ones are never listed. */
const DEST_CARD1 = 1;
const DEST_CARD2 = 2;
const DEST_PS1_VMC1 = 5;
const DEST_PS1_VMC2 = 6;
new g_dest_cards[4][67]; /* the memory cards of the destination menu: DEST_*, card type, 1, location */

/*
 * Lists the destinations of a copy or a move: the memory cards but the one browsed, into g_dest_cards, then the
 * hard disk folders; returns their number, -1 if the card browsed has changed.
 */
list_destinations()
{
    g_dest_card_count = 0;
    {
        new unused; /* unused, but part of the layout */
        new type;
        new free_blocks;
        new formatted;
        new status;
        update_card_info(unused, 0, 0, 0);
        get_card_info(0, 0, type, free_blocks, formatted, status);
        if (device_mode == DEVMODE_MC)
        {
            if (0 == mc_port && 0 == mc_slot)
            {
                if (g_card_types[0] != type || 0 == type || status == MC_CHANGED || status == MC_CHANGED_UNFORMATTED)
                {
                    g_card_types[0] = type;
                    return -1;
                }
            }
        }
        dst_mctype0 = type;
        g_card_types[0] = type;
        get_card_info(1, 0, type, free_blocks, formatted, status);
        if (device_mode == DEVMODE_MC)
        {
            if (mc_port == 1 && 0 == mc_slot)
            {
                if (g_card_types[1] != type || 0 == type || status == MC_CHANGED || status == MC_CHANGED_UNFORMATTED)
                {
                    g_card_types[1] = type;
                    return -1;
                }
            }
        }
        dst_mctype1 = type;
        g_card_types[1] = type;
    }
    if (DEVMODE_MC != device_mode || 0 != mc_port || 0 != mc_slot)
    {
        if (0 != dst_mctype0)
        {
            g_dest_cards[g_dest_card_count][0] = DEST_CARD1;
            g_dest_cards[g_dest_card_count][1] = dst_mctype0;
            g_dest_cards[g_dest_card_count][2] = 1;
            get_card_location(dst_mctype0, 0, g_dest_cards[g_dest_card_count][3], 64);
            g_dest_card_count++;
        }
    }
    if (DEVMODE_MC != device_mode || 1 != mc_port || 0 != mc_slot)
    {
        if (0 != dst_mctype1)
        {
            g_dest_cards[g_dest_card_count][0] = DEST_CARD2;
            g_dest_cards[g_dest_card_count][1] = dst_mctype1;
            g_dest_cards[g_dest_card_count][2] = 1;
            get_card_location(dst_mctype1, 1, g_dest_cards[g_dest_card_count][3], 64);
            g_dest_card_count++;
        }
    }
    list_hdd_folders(device_mode);
    printf("::::dst_device_num = %d\n", g_dest_card_count);
    return g_dest_card_count + g_folder_count;
}

/* Gets the name and the url of destination index of list_destinations. */
get_destination(index, name[], url[])
{
    if (index < 0)
    {
        return;
    }
    if (index < g_dest_card_count)
    {
        switch (g_dest_cards[index][0])
        {
            case DEST_CARD1:
            {
                sprintf(name, g_dest_cards[index][3]);
                sprintf(url, "mc00:/");
            }
            case DEST_CARD2:
            {
                sprintf(name, g_dest_cards[index][3]);
                sprintf(url, "mc10:/");
            }
            case DEST_PS1_VMC1:
            {
                sprintf(name, g_dest_cards[index][3]);
                sprintf(url, "ps1vmc00:/");
            }
            case DEST_PS1_VMC2:
            {
                sprintf(name, g_dest_cards[index][3]);
                sprintf(url, "ps1vmc10:/");
            }
        }
    }
    else if (index - g_dest_card_count < g_folder_count)
    {
        printf("............. index = %d,  dst_device_num = %d\n", index, g_dest_card_count);
        new folder[256];
        new comment[384];
        svdatGetFolder(index - g_dest_card_count, folder, comment, 256, 384);
        sprintf(name, "%s", folder);
        sprintf(url, "%s", comment);
    }
}

/*
 * Asks msg with the buttons yes and no (selected first), beside the option menu if beside_menu; the dialog watches
 * the memory card browsed and, with check_dest, the one in (port2, slot2). Returns 0 for yes, 1 for no or cross,
 * CARD_EJECTED or DEST_CARD_EJECTED.
 */
ask_yes_no(msg[], yes[], no[], beside_menu, selected, check_mode, check_dest, port2, slot2)
{
    new args[512];
    SetOptionI(focus_glowfly, TAG("ZSRT"), ZSORT_OVERLAY);
    if (device_mode == DEVMODE_MC)
    {
        new check = 1;
        if (0 == check_dest)
        {
            sprintf(args, "msg=%s;yes=%s;no=%s;mcchk=%d;mcport=%d;mcslot=%d;mctype=%d;mcchkmode=%d", msg, yes, no,
                    check, mc_port, mc_slot, g_card_types[mc_port], check_mode);
        }
        else
        {
            new check2 = 1;
            sprintf(args,
                    "msg=%s;yes=%s;no=%s;mcchk=%d;mcport=%d;mcslot=%d;mctype=%d;mcchkmode=%d;mcchk2=%d;mcport2=%d;mcslot2=%d;mctype2=%d",
                    msg, yes, no, check, mc_port, mc_slot, g_card_types[mc_port], check_mode, check2, port2, slot2,
                    g_card_types[port2]);
        }
    }
    else
    {
        new check;
        if (0 == check_dest)
        {
            sprintf(args, "msg=%s;yes=%s;no=%s;mcchk=%d", msg, yes, no, check);
        }
        else
        {
            new check2 = 1;
            sprintf(args, "msg=%s;yes=%s;no=%s;mcchk=%d;mcchkmode=%d;mcchk2=%d;mcport2=%d;mcslot2=%d;mctype2=%d", msg,
                    yes, no, check, check_mode, check2, port2, slot2, g_card_types[port2]);
        }
    }
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    SetOptionI(dialog, TAG("ZSRT"), ZSORT_OVERLAY);
    AttachScript(dialog, _, "confirm_yesno", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus_glowfly);
    SetPubVar(dialog, "select", selected);
    if (beside_menu)
    {
        SetPos(dialog, 432.0, 224.0, 0.0, 0);
        SetPubVar(dialog, "width", 340.0);
        SetPubVar(dialog, "option_mode", 1);
    }
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    new answer = GetPubVar(dialog, "select");
    new ejected = GetPubVar(dialog, "mc_eject_flag");
    new dest_ejected = GetPubVar(dialog, "mc_eject_flag2");
    Destroy(dialog);
    if (answer == -1)
    {
        answer = 1;
    }
    if (dest_ejected)
    {
        answer = DEST_CARD_EJECTED;
    }
    if (ejected)
    {
        answer = CARD_EJECTED;
    }
    SetOptionI(focus_glowfly, TAG("ZSRT"), ZSORT_NONE);
    return answer;
}

/* Shows msg in confirm_ok, watching the memory cards as ask_yes_no; returns 0, CARD_EJECTED or DEST_CARD_EJECTED. */
show_message(msg[], ok[], beside_menu, check_mode, check_dest, port2, slot2)
{
    new args[512];
    if (device_mode == DEVMODE_MC)
    {
        new check = 1;
        if (0 == check_dest)
        {
            sprintf(args, "msg=%s;ok=%s;mcchk=%d;mcport=%d;mcslot=%d;mctype=%d;mcchkmode=%d", msg, ok, check, mc_port,
                    mc_slot, g_card_types[mc_port], check_mode);
        }
        else
        {
            new check2 = 1;
            sprintf(args,
                    "msg=%s;ok=%s;mcchk=%d;mcport=%d;mcslot=%d;mctype=%d;mcchkmode=%d;mcchk2=%d;mcport2=%d;mcslot2=%d;mctype2=%d",
                    msg, ok, check, mc_port, mc_slot, g_card_types[mc_port], check_mode, check2, port2, slot2,
                    g_card_types[port2]);
        }
    }
    else
    {
        new check;
        if (0 == check_dest)
        {
            sprintf(args, "msg=%s;ok=%s;mcchk=%d", msg, ok, check);
        }
        else
        {
            new check2 = 1;
            sprintf(args, "msg=%s;ok=%s;mcchk=%d;mcchkmode=%d;mcchk2=%d;mcport2=%d;mcslot2=%d;mctype2=%d", msg, ok,
                    check, check_mode, check2, port2, slot2, g_card_types[port2]);
        }
    }
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    SetOptionI(dialog, TAG("ZSRT"), ZSORT_OVERLAY);
    AttachScript(dialog, _, "confirm_ok", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus_glowfly);
    if (beside_menu)
    {
        SetPos(dialog, 432.0, 224.0, 0.0, 0);
        SetPubVar(dialog, "width", 340.0);
        SetPubVar(dialog, "option_mode", 1);
    }
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    new ejected = GetPubVar(dialog, "mc_eject_flag");
    new dest_ejected = GetPubVar(dialog, "mc_eject_flag2");
    Destroy(dialog);
    if (ejected)
    {
        return CARD_EJECTED;
    }
    if (dest_ejected)
    {
        return DEST_CARD_EJECTED;
    }
    return 0;
}

/*
 * Returns the number of items an option menu has besides those of its MENUGROUP: in MENU_MODE_DEST the
 * destinations of a copy or a move (-1 if the memory card browsed has changed).
 */
count_extra_items(side, result, mode, title[], group[], id[], &selected, manager)
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
    get_wnd_var_int("BRMCPORT", mc_port);
    get_wnd_var_int("BRMCSLOT", mc_slot);
    get_wnd_var_int("BRDEVMODE", device_mode);
    get_wnd_var_int("BRMCTYPE0", dst_mctype0);
    get_wnd_var_int("BRMCTYPE1", dst_mctype1);
    count = 0;
    switch (mode)
    {
        case MENU_MODE_DEST:
        {
            count = list_destinations();
            printf("list DB0 opened ------ item_num = %d !!!!\n", count);
            if (count == -1)
            {
                return -1;
            }
            if (count <= 0)
            {
                new msg[256];
                new ok[32];
                if (!strcmp("ccopy", id))
                {
                    get_tag_attr("TEXT", "msgutil_hddcopy_nodevice", "value", msg, 256);
                }
                else
                {
                    get_tag_attr("TEXT", "msgutil_hddmove_nodevice", "value", msg, 256);
                }
                get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
                new closed = show_message(msg, ok, 1, 1, 0, 0, 0); /* unused, but part of the layout */
                photo_focused = GetPubVar(manager, "photo_focused");
                SendMessage(photo_focused, TAG("MODE"), VIEW_NORMAL, 1, 0, 0);
            }
        }
    }
    return count;
}

/* Gives an item of the destination menu (MENU_MODE_DEST) the name of its destination. */
fill_menu_item(side, mode, index, group[], label[])
{
    new unused; /* takes the parameters the function does not use */
    new buffer[64];
    unused = side;
    unused = mode;
    unused = index;
    unused = 64;
    copy_string(buffer, group, unused);
    get_wnd_var_int("BRMCPORT", mc_port);
    get_wnd_var_int("BRMCSLOT", mc_slot);
    get_wnd_var_int("BRDEVMODE", device_mode);
    get_wnd_var_int("BRMCTYPE0", dst_mctype0);
    get_wnd_var_int("BRMCTYPE1", dst_mctype1);
    switch (mode)
    {
        case MENU_MODE_DEST:
        {
            new name[256];
            new url[256];
            get_destination(index, name, url);
            copy_string(label, name, 256);
        }
    }
}

/*
 * Handles a push in an option menu. Circle runs the item id of the main menu (MENU_MODE_GROUP): ccopy and cmove open
 * the menu of the destinations (MENU_MODE_DEST), cdelete, cinfomation, gnewfolder, greturn; in MENU_MODE_DEST it copies
 * or moves the save there. Returns MENU_ITEM when the album has changed, MENU_EXIT to close the menu, 0 to stay; cross
 * goes back.
 */
handle_menu_push(manager, side, &mode, index, button, title[], group[], id[])
{
    photo_focused = GetPubVar(manager, "photo_focused");
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
        mc_port = GetPubVar(manager, "mc_port");
        mc_slot = GetPubVar(manager, "mc_slot");
        device_mode = GetPubVar(manager, "device_mode");
        dst_mctype0 = GetPubVar(manager, "dst_mctype0");
        dst_mctype1 = GetPubVar(manager, "dst_mctype1");
        if (avalGetNum("BRMCPORT") >= 0)
        {
            new value[8];
            avalGetValue("BRMCPORT", 0, value, 8);
            mc_port = atoi(value);
        }
        if (avalGetNum("BRMCSLOT") >= 0)
        {
            new value[8];
            avalGetValue("BRMCSLOT", 0, value, 8);
            mc_slot = atoi(value);
        }
        if (avalGetNum("BRDEVMODE") >= 0)
        {
            new value[8];
            avalGetValue("BRDEVMODE", 0, value, 8);
            device_mode = atoi(value);
        }
        if (avalGetNum("BRMCTYPE0") >= 0)
        {
            new value[8];
            avalGetValue("BRMCTYPE0", 0, value, 8);
            dst_mctype0 = atoi(value);
        }
        if (avalGetNum("BRMCTYPE1") >= 0)
        {
            new value[8];
            avalGetValue("BRMCTYPE1", 0, value, 8);
            dst_mctype1 = atoi(value);
        }
        new icon_file; /* unused, but part of the layout */
        get_wnd_var_int("BRSRCICONCACHE", icon_file);
        new damaged; /* unused, but part of the layout */
        get_wnd_var_int("BRDAMAGED", damaged);
        set_wnd_var_int("BRMCPORT", mc_port);
        set_wnd_var_int("BRMCSLOT", mc_slot);
        set_wnd_var_int("BRDEVMODE", device_mode);
        set_wnd_var_int("BRMCTYPE0", dst_mctype0);
        set_wnd_var_int("BRMCTYPE1", dst_mctype1);
        switch (mode)
        {
            case MENU_MODE_GROUP:
            {
                if (!strcmp("ccopy", id))
                {
                    new no_copy;
                    get_wnd_var_int("BRSRCNOCOPY", no_copy);
                    if (no_copy)
                    {
                        fade_menu(0.0, 250.0);
                        new msg[256];
                        new ok[32];
                        get_tag_attr("TEXT", "msgutil_mccopy_prohibit_", "value", msg, 256);
                        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
                        new closed = show_message(msg, ok, 1, 1, 0, 0, 0);
                        if (closed == CARD_EJECTED)
                        {
                            new ejected = 1;
                            set_wnd_var_int("BRMCEJECT", ejected);
                            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                            result = MENU_EXIT;
                        }
                        else
                        {
                            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                            result = 0;
                        }
                        fade_menu(1.0, 250.0);
                    }
                    else
                    {
                        SendMessage(photo_focused, TAG("MODE"), VIEW_COPY, 0, 0, 0);
                        wait_ms(250);
                        get_tag_attr("TEXT", "msgutil_vmccopy_device_select", "value", title, 64);
                        mode = MENU_MODE_DEST;
                        result = MENU_SUBMENU;
                        set_wnd_var_int("BRCHKDSTMC", 1);
                    }
                }
                else if (!strcmp("cmove", id))
                {
                    new no_copy;
                    get_wnd_var_int("BRSRCNOCOPY", no_copy);
                    if (no_copy)
                    {
                        fade_menu(0.0, 250.0);
                        new msg[256];
                        new ok[32];
                        get_tag_attr("TEXT", "msgutil_hddmove_prohibit", "value", msg, 256);
                        get_tag_attr("TEXT", "msgutil_common_ok", "value", ok, 32);
                        new closed = show_message(msg, ok, 1, 1, 0, 0, 0);
                        if (closed == CARD_EJECTED)
                        {
                            new ejected = 1;
                            set_wnd_var_int("BRMCEJECT", ejected);
                            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                            result = MENU_EXIT;
                        }
                        else
                        {
                            send_focus(focus_glowfly, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                            result = 0;
                        }
                        fade_menu(1.0, 250.0);
                    }
                    else
                    {
                        SendMessage(photo_focused, TAG("MODE"), VIEW_MOVE, 0, 0, 0);
                        wait_ms(250);
                        get_tag_attr("TEXT", "msgutil_vmcmove_device_select", "value", title, 64);
                        mode = MENU_MODE_DEST;
                        result = MENU_SUBMENU;
                        set_wnd_var_int("BRCHKDSTMC", 1);
                    }
                }
                else if (!strcmp("cdelete", id))
                {
                    fade_menu(0.0, 250.0);
                    SendMessage(photo_focused, TAG("MODE"), VIEW_DELETE, 1, 0, 0);
                    wait_ms(250);
                    new ret = delete_save();
                    mode = MENU_MODE_GROUP;
                    if (ret >= 0)
                    {
                        result = MENU_ITEM;
                    }
                    else
                    {
                        fade_menu(1.0, 250.0);
                        result = 0;
                    }
                    SendMessage(photo_focused, TAG("MODE"), VIEW_NORMAL, 1, 0, 0);
                    wait_ms(250);
                }
                else if (!strcmp("cinfomation", id))
                {
                    new info = GetPubVar(manager, "file_info");
                    SendMessage(photo_focused, TAG("MODE"), VIEW_INFO, 1, 0, 0);
                    SendMessage(info, TAG("FADE"), 0, 300.0, 0, 0);
                    fade_menu(0.0, 300.0);
                    wait_ms(300);
                    SendMessage(info, TAG("MODE"), 1, 0, 0, 0);
                    SendMessage(info, TAG("FADE"), 1.0, 200.0, 0, 0);
                    result = 0;
                    SetSystemParam(SYS_PADHELP, PADHELP_CROSS);
                    new frames;
                    for (; ; )
                    {
                        new held;
                        new pressed;
                        PadState(held, pressed, _, _, -1);
                        if (pressed & PAD_CROSS)
                        {
                            sndSePlay(SE_CANCEL);
                            break;
                        }
                        if (device_mode == DEVMODE_MC)
                        {
                            if (0 == frames % 30)
                            {
                                new ignored;
                                update_card_info(ignored, 0, 0, 0);
                                new type;
                                new free_blocks;
                                new formatted;
                                new status;
                                get_card_info(mc_port, mc_slot, type, free_blocks, formatted, status);
                                if (0 == type || status == MC_CHANGED || status == MC_CHANGED_UNFORMATTED)
                                {
                                    new ejected = 1;
                                    set_wnd_var_int("BRMCEJECT", ejected);
                                    result = MENU_EXIT;
                                    break;
                                }
                            }
                            frames++;
                        }
                        sleep;
                    }
                    SetSystemParam(SYS_PADHELP, PADHELP_CIRCLE | PADHELP_CROSS);
                    SendMessage(info, TAG("FADE"), 0, 300.0, 0, 0);
                    SendMessage(photo_focused, TAG("MODE"), VIEW_NORMAL, 1, 0, 0);
                    wait_ms(300);
                    SendMessage(info, TAG("MODE"), 0, 0, 0, 0);
                    SendMessage(info, TAG("FADE"), 1.0, 300.0, 0, 0);
                    fade_menu(1.0, 300.0);
                    wait_ms(300);
                    mode = MENU_MODE_GROUP;
                }
                else if (!strcmp("gnewfolder", id))
                {
                    new ret = create_folder(); /* unused, but part of the layout */
                    mode = MENU_MODE_GROUP;
                    result = 0;
                }
                else if (!strcmp("greturn", id))
                {
                    mode = MENU_MODE_DEST;
                    result = MENU_GO_TOP;
                }
            }
            case MENU_MODE_DEST:
            {
                new dest_name[256];
                new dest_url[256];
                get_destination(index, dest_name, dest_url);
                set_wnd_var_int("BRCHKDSTMC", 0);
                if (!strcmp("ccopy", id))
                {
                    fade_menu(0.0, 250.0);
                    SendMessage(photo_focused, TAG("MODE"), VIEW_COPY, 1, 0, 0);
                    wait_ms(250);
                    new ret = copy_save(dest_url);
                    if (ret >= 0)
                    {
                        result = MENU_EXIT;
                        SendMessage(photo_focused, TAG("MODE"), VIEW_NORMAL, 1, 0, 0);
                    }
                    else
                    {
                        fade_menu(1.0, 250.0);
                        result = 0;
                        SendMessage(photo_focused, TAG("MODE"), VIEW_COPY, 0, 0, 0);
                        set_wnd_var_int("BRCHKDSTMC", 1);
                    }
                    wait_ms(250);
                }
                else if (!strcmp("cmove", id))
                {
                    fade_menu(0.0, 250.0);
                    SendMessage(photo_focused, TAG("MODE"), VIEW_MOVE, 1, 0, 0);
                    wait_ms(250);
                    new ret = move_save(dest_url);
                    if (0 == ret)
                    {
                        result = MENU_ITEM;
                        SendMessage(photo_focused, TAG("MODE"), VIEW_NORMAL, 1, 0, 0);
                    }
                    else if (ret > 0)
                    {
                        result = MENU_EXIT;
                        SendMessage(photo_focused, TAG("MODE"), VIEW_NORMAL, 1, 0, 0);
                    }
                    else
                    {
                        fade_menu(1.0, 250.0);
                        result = 0;
                        SendMessage(photo_focused, TAG("MODE"), VIEW_MOVE, 0, 0, 0);
                        set_wnd_var_int("BRCHKDSTMC", 1);
                    }
                    wait_ms(250);
                }
            }
        }
    }
    if (button == TAG("RDWN"))
    {
        SendMessage(photo_focused, TAG("MODE"), VIEW_NORMAL, 1, 0, 0);
        wait_ms(250);
        switch (mode)
        {
            case MENU_MODE_GROUP:
            {
                printf("stdmenu btn RDWN\n");
                mode = MENU_MODE_GROUP;
                result = MENU_BACK;
            }
            case MENU_MODE_DEST:
            {
                printf("stdmenu btn RDWN\n");
                mode = MENU_MODE_GROUP;
                result = MENU_RETURN;
                set_wnd_var_int("BRCHKDSTMC", 0);
            }
        }
    }
    return result;
}

/* Returns 1 if save index cannot be copied: damaged, flagged so, or of a kind without an icon. */
get_no_copy(index)
{
    new state[1];
    new flags[1];
    new unused[1]; /* unused, but part of the layout */
    new kind_field[1];
    datGetData(index, SAVE_STATE, state, 1);
    if (state[0] == STATE_DAMAGED)
    {
        return 1;
    }
    datGetData(index, SAVE_ATTR, flags, 1);
    if (0 != flags[0] & ATTR_NO_COPY)
    {
        return 1;
    }
    datGetData(index, SAVE_TYPE, kind_field, 1);
    new kind = kind_field[0];
    if (TYPE_PS2_SAVE != kind && TYPE_PS1_SAVE != kind && TYPE_PDA_SAVE != kind)
    {
        return 1;
    }
    return 0;
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

/*
 * Tells that the file system is damaged (TEXT msgutil_error_fsck1 to 4), or, on the hard disk, sets the window
 * variable BRPFSERR instead; returns the answer of the message.
 */
show_disk_error()
{
    new closed;
    if (DEVMODE_HDD != device_mode)
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
        closed = show_message(msg, ok, 1, 1, 0, 0, 0);
    }
    if (DEVMODE_HDD == device_mode)
    {
        new pfs_error = 1;
        set_wnd_var_int("BRPFSERR", pfs_error);
    }
    return closed;
}
