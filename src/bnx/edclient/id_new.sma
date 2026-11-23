/*
 * /opt0/bn/script/edclient/id_new.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The page that saves a PSBB ID and its password, from the menu of the saved IDs: the ID to type (or given by the
 * arguments psbbid and password), the device, a memory card or the hard disk, then the save.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "edpage.inc"

/* Values of g_state: the page shown. */
const STATE_INPUT = 0;
const STATE_SELECT = 1;
const STATE_COMPLETE = 2;
const STATE_NONE = 3; /* before the first page */

#include "edclient.inc"

#include "wait.inc"

#include "tag.inc"

#include <float>

#include "imgsync.inc"

/* Sleeps until a message msg arrives. */
wait_message(msg, &a=0, &b=0, &c=0, &d=0)
{
    while (!FindMessage(msg, a, b, c, d))
    {
        sleep;
    }
}

#include "sound.inc"

/* Returns whether str is a packed string. */
is_packed(const str[])
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
append_string(dest[], const src[], maxlen)
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

#include "disc.inc"

#include "focus.inc"

/* Same as send_focus of dom.inc. */
send_focus2(obj, target, z_offset, Float:sharp, Float:extra_w, Float:extra_h, Float:move_time, smoothing, blinking)
{
    if (!obj)
    {
        return;
    }
    SendMessage(obj, TAG("FOC0"), target, TAG(""), z_offset, sharp);
    SendMessage(obj, TAG("FOC1"), extra_w, extra_h, move_time, smoothing);
    SendMessage(obj, TAG("FOC2"), blinking, 0, 0, 0);
}

/* The arguments of the member pages, which the pages pass on from page to page. */
new g_arg_ret[] = "ret";
new g_arg_retarg[] = "retarg";
new g_arg_retflag[] = "retflag";
new g_arg_game_ver[] = "GameVer";
new g_arg_game_id[] = "GameID";
new g_arg_game_passwd[] = "GamePasswd";
new g_arg_given_name[] = "givenname";
new g_arg_family_name[] = "familyname";
new g_arg_birthday[] = "birthday";
new g_arg_zipcode[] = "zipcode";
new g_arg_sex[] = "sex";
new g_arg_mail_addr[] = "mailaddr";
new g_arg_mail_hope[] = "mailhope";
new g_arg_unload_bgp[] = "unloadBGP";
new g_arg_unload_bgm[] = "unloadBGM";
new g_arg_member_focus[] = "member_focus";
new g_arg_id_save_focus[] = "id_save_focus";

/* Appends to args (maxlen cells) the member arguments the page was opened with, as "name=value;". */
append_member_args(args[], maxlen)
{
    new value[64];
    new found;
    found = GetArg(g_arg_ret, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", g_arg_ret, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(g_arg_retarg, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", g_arg_retarg, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(g_arg_game_ver, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", g_arg_game_ver, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(g_arg_game_id, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", g_arg_game_id, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(g_arg_game_passwd, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", g_arg_game_passwd, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(g_arg_given_name, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", g_arg_given_name, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(g_arg_family_name, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", g_arg_family_name, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(g_arg_birthday, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", g_arg_birthday, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(g_arg_zipcode, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", g_arg_zipcode, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(g_arg_sex, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", g_arg_sex, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(g_arg_mail_addr, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", g_arg_mail_addr, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(g_arg_mail_hope, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", g_arg_mail_hope, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(g_arg_unload_bgp, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", g_arg_unload_bgp, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(g_arg_unload_bgm, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", g_arg_unload_bgm, value);
        append_string(args, value, maxlen);
    }
}

/* Appends the argument member_focus to args, like append_member_args. */
append_focus_arg(args[], maxlen)
{
    new value[64];
    if (GetArg(g_arg_member_focus, value, 64) == 1)
    {
        sprintf(value, "%s=%s;", g_arg_member_focus, value);
        append_string(args, value, maxlen);
    }
}

/* Appends the argument id_save_focus to args, like append_member_args. */
append_id_save_focus_arg(args[], maxlen)
{
    new value[64];
    if (GetArg(g_arg_id_save_focus, value, 64) == 1)
    {
        sprintf(value, "%s=%s;", g_arg_id_save_focus, value);
        append_string(args, value, maxlen);
    }
}

#include "misc.inc"

/*
 * Creates a nine-slice plane at rect (x, y, z, width, height; with no size, the texture's), whose edges keep the
 * size in pixels given by border (left, right, top, bottom).
 */
create_frame_plane(parent, rect[], zsort, color, texture, Float:tex_w, Float:tex_h, Float:border[] = {0, 0, 0, 0})
{
    new plane;
    new Float:x;
    new Float:y;
    new Float:z;
    new Float:plane_w;
    new Float:plane_h;
    new Float:margins[4];
    new Float:red;
    new Float:green;
    new Float:blue;
    new Float:alpha;
    unpack_color(color, red, green, blue, alpha);
    plane = CreatePlane3x3(parent);
    x = rect[0];
    y = rect[1];
    z = rect[2];
    plane_w = rect[3];
    plane_h = rect[4];
    if (0.0 == plane_w && 0.0 == plane_h)
    {
        plane_w = tex_w;
        plane_h = tex_h;
    }
    SetPos(plane, x, y, z, 0);
    SetOptionI(plane, TAG("ZSRT"), zsort);
    SetColor(plane, red, green, blue, 0.0);
    if (0 != texture)
    {
        SetTexture(plane, texture, 0, -1);
    }
    SetLocalScale(plane, plane_w, plane_h, 0.0);
    margins[0] = border[0] / plane_w;
    margins[1] = border[1] / plane_w;
    margins[2] = border[2] / plane_h;
    margins[3] = border[3] / plane_h;
    SetOptionP(plane, TAG("MGN"), margins, 4);
    margins[0] = border[0] / tex_w;
    margins[1] = border[1] / tex_w;
    margins[2] = border[2] / tex_h;
    margins[3] = border[3] / tex_h;
    SetOptionP(plane, TAG("MGNT"), margins, 4);
    return plane;
}

new g_any_child2[1]; /* empty: any element */
new g_any_next2[1]; /* empty: any element */

#include "dom.inc"

/*
 * Opens the page of the LINK link_id with args and ends the script; unless direct, the page goes where the argument
 * "retflag" says.
 */
load_page(link_id[], args[], direct)
{
    new value[64];
    new url[64];
    new page_args[128];
    new found;
    new retflag;
    copy_string(page_args, args, 128);
    if (direct == 1)
    {
        get_tag_attr("LINK", link_id, "href", url, 64);
    }
    else
    {
        found = GetArg(g_arg_retflag, value, 64);
        if (found == 1)
        {
            retflag = atoi(value);
        }
        switch (retflag)
        {
            case RETFLAG_RET:
            {
                found = GetArg(g_arg_ret, url, 64);
                if (found == 1)
                {
                }
                else
                {
                    get_tag_attr("LINK", link_id, "href", url, 64);
                }
            }
            case RETFLAG_LINK:
            {
                found = GetArg(g_arg_ret, value, 64);
                if (found == 1)
                {
                    append_member_args(page_args, 128);
                }
                get_tag_attr("LINK", link_id, "href", url, 64);
            }
            case RETFLAG_AUTHOR:
            {
                found = GetArg(g_arg_ret, value, 64);
                if (found == 1)
                {
                    append_member_args(page_args, 128);
                }
                copy_string(url, "author.xml", 64);
            }
            default:
            {
            }
        }
    }
    wait_xml_idle();
    xmlLoadStart(url, "", page_args, XMLLOAD_MAIN);
    wait_xml_load();
    Destroy(Self());
}

/*
 * Opens the ed_page of the PAGE element page_id with args and gives it the focus; it slides in as slide says
 * (SLIDE_IN_*), which is then SLIDE_IN_NONE.
 */
open_ed_page(page_id[], focus_obj, &slide, args[])
{
    new page_args[256];
    new page;
    page = CreateNull(Disp2D(0));
    sprintf(page_args, "page_id=%s;", page_id);
    if (strlen(args) > 0)
    {
        append_string(page_args, args, 256);
    }
    AttachScript(page, _, "ed_page", page_args);
    SetPubVar(page, "g_objManager", Self());
    SetPubVar(page, "g_objFocus", focus_obj);
    if (slide == SLIDE_IN_LEFT)
    {
        SetPos(page, -320.0, 0.0, 0.0, 0);
    }
    else if (slide == SLIDE_IN_RIGHT)
    {
        SetPos(page, 320.0, 0.0, 0.0, 0);
    }
    AnimMoveLine(page, 0.0, 0.0, 0.0, 200.0, 0.0, ANIM_FUNC_EXPONENT);
    SendMessage(page, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
    slide = SLIDE_IN_NONE;
    return page;
}

/*
 * Fades out an ed_page, sliding it out as slide_dir says (SLIDE_OUT_*), and waits for its "END"; the focus goes back
 * to focus_obj.
 */
close_ed_page(page, focus_obj, slide_dir)
{
    new Float:x;
    new Float:y;
    SendMessage(page, TAG("FADE"), floatround(0.0, 0), floatround(200.0, 0), 0, 0);
    send_focus2(focus_obj, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    x = 0.0;
    y = 0.0;
    if (SLIDE_OUT_NONE == slide_dir)
    {
    }
    else if (slide_dir == SLIDE_OUT_LEFT)
    {
        x = -160.0;
    }
    else if (slide_dir == SLIDE_OUT_RIGHT)
    {
        x = 160.0;
    }
    AnimMoveLine(page, x, y, 0.0, 200.0, 0.0, ANIM_FUNC_EXPONENT);
    wait_message(TAG("END"));
}

/* Creates the planes of the page arrows (IMG_NEXT, IMG_NEXT_HI, IMG_PREV, IMG_PREV_HI) for update_page_arrows. */
create_page_arrows(arrows[])
{
    new buf[64];
    new texture;
    new Float:tex_w;
    new Float:tex_h;
    new rect[5];
    new zsort;
    new color;
    new res;
    res = domFindTag(domRoot(), "IMG_RESOURCE", "IMG_NEXT", 0);
    domGetData(res, "texture", buf, 64);
    texture = atoi(buf);
    domGetData(res, "w", buf, 64);
    tex_w = atof(buf);
    domGetData(res, "h", buf, 64);
    tex_h = atof(buf);
    rect[0] = 600.0;
    rect[1] = 260.0;
    rect[2] = 0;
    rect[3] = 0;
    rect[4] = 0;
    zsort = ZSORT_NONE;
    color = pack_color(255, 255, 255, 0);
    arrows[PAGE_ARROW_NEXT] = create_frame_plane(Disp2D(0), rect, zsort, color, texture, tex_w, tex_h);
    res = domFindTag(domRoot(), "IMG_RESOURCE", "IMG_NEXT_HI", 0);
    domGetData(res, "texture", buf, 64);
    texture = atoi(buf);
    domGetData(res, "w", buf, 64);
    tex_w = atof(buf);
    domGetData(res, "h", buf, 64);
    tex_h = atof(buf);
    rect[0] = 600.0;
    rect[1] = 260.0;
    rect[2] = 0;
    rect[3] = 0;
    rect[4] = 0;
    zsort = ZSORT_NONE;
    color = pack_color(255, 255, 255, 0);
    arrows[PAGE_ARROW_NEXT_LIT] = create_frame_plane(Disp2D(0), rect, zsort, color, texture, tex_w, tex_h);
    res = domFindTag(domRoot(), "IMG_RESOURCE", "IMG_PREV", 0);
    domGetData(res, "texture", buf, 64);
    texture = atoi(buf);
    domGetData(res, "w", buf, 64);
    tex_w = atof(buf);
    domGetData(res, "h", buf, 64);
    tex_h = atof(buf);
    rect[0] = 40.0;
    rect[1] = 260.0;
    rect[2] = 0;
    rect[3] = 0;
    rect[4] = 0;
    zsort = ZSORT_NONE;
    color = pack_color(255, 255, 255, 0);
    arrows[PAGE_ARROW_PREV] = create_frame_plane(Disp2D(0), rect, zsort, color, texture, tex_w, tex_h);
    res = domFindTag(domRoot(), "IMG_RESOURCE", "IMG_PREV_HI", 0);
    domGetData(res, "texture", buf, 64);
    texture = atoi(buf);
    domGetData(res, "w", buf, 64);
    tex_w = atof(buf);
    domGetData(res, "h", buf, 64);
    tex_h = atof(buf);
    rect[0] = 40.0;
    rect[1] = 260.0;
    rect[2] = 0;
    rect[3] = 0;
    rect[4] = 0;
    zsort = ZSORT_NONE;
    color = pack_color(255, 255, 255, 0);
    arrows[PAGE_ARROW_PREV_LIT] = create_frame_plane(Disp2D(0), rect, zsort, color, texture, tex_w, tex_h);
}

#include "layout.inc"

/* Loads the page of the LINK link_id into the background window, unless the edclient background is there. */
load_background(link_id[])
{
    new buf[64];
    new bg_name[2] = "EDBG";
    if (wndGetName(WND_BG, buf, 64) && 0 == strcmp(buf, bg_name))
    {
        fade_ed_background(0.5, 600.0);
        return;
    }
    get_tag_attr("LINK", link_id, "href", buf, 64);
    xmlLoadStart(buf, bg_name, "", XMLLOAD_BG);
    wait_xml_load();
    fade_ed_background(0.5, 600.0);
}

/* Same as load_image_resources of layout.inc. */
load_image_resources2()
{
    new id[64];
    new buf[64];
    new node;
    new len;
    node = domFirstChild(domRoot(), g_any_child2);
    for (; node; node = domNext(node, g_any_next2))
    {
        domGetTagName(node, buf, 64);
        if (0 != strcmp("IMG", buf))
        {
            continue;
        }
        len = domGetData(node, "id", id, 64);
        if (0 == len)
        {
            continue;
        }
        len = domGetData(node, "src", buf, 64);
        if (0 == len)
        {
            continue;
        }
        imgDecStart(buf, 0, 0, -1, 100, 0);
        for (; ; )
        {
            new status = imgDecSync();
            switch (status)
            {
                case IMG_BUSY:
                {
                }
                case IMG_DONE:
                {
                    new tex;
                    new tex_w;
                    new tex_h;
                    new res;
                    tex = imgTexID();
                    imgKeepTex(tex);
                    tex_w = imgTexOrgWidth();
                    tex_h = imgTexOrgHeight();
                    res = domAddTag(domRoot(), "IMG_RESOURCE");
                    domSetData(res, "id", id);
                    sprintf(buf, "%d", tex);
                    domSetData(res, "texture", buf);
                    sprintf(buf, "%d", tex_w);
                    domSetData(res, "w", buf);
                    sprintf(buf, "%d", tex_h);
                    domSetData(res, "h", buf);
                    break;
                }
                case IMG_FAILED:
                {
                    printf("@@@ Error : image decode fail. [%s]\n", buf);
                    break;
                }
                default:
                {
                }
            }
            sleep;
        }
    }
}

#include "background.inc"

#include "validate.inc"

#include "pfs.inc"

/* The server of the paid services and the name and version this client gives it. */
new g_server_host[] = "lib.feega.sonyfinance.co.jp";
new g_server_port = 14020;

/* Copies the name of the client, "BBnavi". */
get_client_name(name[], maxlen)
{
    copy_string(name, "BBnavi", maxlen);
}

/* Reads the argument GameVer, or "1.0.0" without it. */
get_client_version(version[], maxlen)
{
    new found;
    found = GetArg(g_arg_game_ver, version, maxlen);
    if (0 == found)
    {
        copy_string(version, "1.0.0", maxlen);
    }
}

#include "title.inc"

/*
 * Opens the dialog of the "dialog" element dialog_id (its type, title, texts and items), with extra after the
 * texts, and waits for it; returns the button chosen, 1 when it was canceled. The pad help is restored.
 */
pop_dialog(focus_obj, dialog_id[], const extra[] = "")
{
    new n;
    new choice;
    new buf[128];
    new args[256];
    new dialog_node;
    new node;
    new len;
    new buttons;
    new old_pad_help;
    new wait_time;
    new se_open = ED_SE_DIALOG_OPEN;
    new se_close = ED_SE_DIALOG_CLOSE;
    new Float:red;
    new Float:green = 0.2353;
    new Float:blue = 0.3922;
    old_pad_help = GetSystemParam(SYS_PADHELP);
    new old_circle[8];
    new old_cross[8];
    new old_triangle[8];
    new old_square[8];
    GetSystemParamStr(SYS_STR_PADHELP_RR, old_circle, 8);
    GetSystemParamStr(SYS_STR_PADHELP_RD, old_cross, 8);
    GetSystemParamStr(SYS_STR_PADHELP_RU, old_triangle, 8);
    GetSystemParamStr(SYS_STR_PADHELP_RL, old_square, 8);
    dialog_node = find_child_tag2(domRoot(), "dialog", "id", dialog_id);
    len = domGetData(dialog_node, "type", buf, 128);
    if (0 == strcmp("ok", buf))
    {
        copy_string(args, "type=confirm;", 256);
        buttons = PADHELP_CIRCLE;
    }
    else if (0 == strcmp("yesno", buf))
    {
        copy_string(args, "type=confirm;", 256);
        buttons = PADHELP_CIRCLE | PADHELP_CROSS;
        choice = 1;
    }
    else if (0 == strcmp("decide", buf))
    {
        buttons = PADHELP_CIRCLE;
    }
    else if (0 == strcmp("cancel", buf))
    {
        buttons = PADHELP_CROSS;
    }
    else if (0 == strcmp("error", buf))
    {
        buttons = PADHELP_CROSS;
        red = 0.549;
        green = 0.0784;
        blue = 0.1961;
        se_open = ED_SE_ERROR_OPEN;
        se_close = ED_SE_ERROR_CLOSE;
    }
    else if (0 == strcmp("moment", buf))
    {
        wait_time = 3000;
    }
    else
    {
        printf("@@@@@ Unknow PopDialog type : [%s]\n", buf);
    }
    set_pad_help_labels(buttons);
    node = domFirstChild(dialog_node, g_any_child2);
    for (; node; node = domNext(node, g_any_next2))
    {
        domGetTagName(node, buf, 128);
        if (0 != strcmp("title", buf))
        {
            continue;
        }
        len = domGetData(node, "text_id", buf, 128);
        if (0 == len)
        {
            continue;
        }
        len = get_tag_attr("TEXT", buf, "value", buf, 128);
        if (0 == len)
        {
            continue;
        }
        append_string(args, "title=", 256);
        append_string(args, buf, 256);
        append_string(args, ";", 256);
        break;
    }
    append_string(args, "msg=", 256);
    node = domFirstChild(dialog_node, g_any_child2);
    for (; node; node = domNext(node, g_any_next2))
    {
        domGetTagName(node, buf, 128);
        if (0 != strcmp("text", buf))
        {
            continue;
        }
        len = domGetData(node, "text_id", buf, 128);
        if (0 == len)
        {
            continue;
        }
        len = get_tag_attr("TEXT", buf, "value", buf, 128);
        if (0 == len)
        {
            continue;
        }
        append_string(args, buf, 256);
        append_string(args, "\n", 256);
    }
    append_string(args, extra, 256);
    append_string(args, ";", 256);
    n = 0;
    node = domFirstChild(dialog_node, g_any_child2);
    for (; node; node = domNext(node, g_any_next2))
    {
        domGetTagName(node, buf, 128);
        if (0 != strcmp("item", buf))
        {
            continue;
        }
        len = domGetData(node, "text_id", buf, 128);
        if (0 == len)
        {
            continue;
        }
        len = get_tag_attr("TEXT", buf, "value", buf, 128);
        if (0 == len)
        {
            continue;
        }
        sprintf(buf, "btn%d=%s;", n, buf);
        append_string(args, buf, 256);
        n++;
    }
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 120.0 + 260.0 * 0.5, 0.0, 0);
    AttachScript(dialog, _, "dialog", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus_obj);
    SetPubVar(dialog, "select", choice);
    SetPubVar(dialog, "padhelp", buttons);
    SetPubVar(dialog, "width", 500.0);
    SetPubVar(dialog, "height", 260.0);
    SetPubVar(dialog, "red", red);
    SetPubVar(dialog, "green", green);
    SetPubVar(dialog, "blue", blue);
    SetPubVar(dialog, "bgcolor", 0);
    SetPubVar(dialog, "se_open", se_open);
    SetPubVar(dialog, "se_close", se_close);
    SetPubVar(dialog, "se_ok", ED_SE_DECIDE);
    SetPubVar(dialog, "se_cancel", ED_SE_CANCEL);
    SetPubVar(dialog, "se_cursor", ED_SE_CURSOR);
    if (wait_time > 0)
    {
        wait_ms(wait_time);
        SetPubVar(dialog, "canceled", 1);
    }
    for (; ; )
    {
        if (ScriptState(dialog) == SCRIPT_FINISHED)
        {
            break;
        }
        sleep;
    }
    choice = GetPubVar(dialog, "select");
    new canceled = GetPubVar(dialog, "canceled");
    if (canceled == 1)
    {
        choice = 1;
    }
    choice = choice == -1 ? 1 : choice;
    Destroy(dialog);
    SetSystemParam(SYS_PADHELP, old_pad_help);
    SetSystemParamStr(SYS_STR_PADHELP_RR, old_circle);
    SetSystemParamStr(SYS_STR_PADHELP_RD, old_cross);
    SetSystemParamStr(SYS_STR_PADHELP_RU, old_triangle);
    SetSystemParamStr(SYS_STR_PADHELP_RL, old_square);
    return choice;
}

/*
 * Builds the menu element menu_device (a new one in menu) with an item for each device ready (memory card 1, memory
 * card 2, hard disk), whose attribute device is the device; returns the number of items.
 */
build_device_menu(&menu)
{
    new i;
    new status;
    new count;
    new buf[64];
    new devices[3] = {EDS_DEVICE_CARD1, EDS_DEVICE_CARD2, EDS_DEVICE_HDD};
    new text_ids[3][6] = {"msged_savemc_device_mc1", "msged_savemc_device_mc2", "msged_savemc_device_hdd"};
    edsClientPoll();
    while (edsClientSync(status))
    {
        sleep;
    }
    if (0 != menu)
    {
        domRemoveTag(menu);
        menu = 0;
    }
    menu = domAddTag(domRoot(), "menu");
    domSetData(menu, "id", "menu_device");
    i = 0;
    for (; i < 3; i++)
    {
        if (0 == is_device_ready(devices[i], status))
        {
            continue;
        }
        new item = domAddTag(menu, "item");
        sprintf(buf, "%d", count);
        domSetData(item, "id", buf);
        get_tag_attr("TEXT", text_ids[i], "value", buf, 64);
        domSetData(item, "value", buf);
        sprintf(buf, "%d", devices[i]);
        domSetData(item, "device", buf);
        count++;
    }
    return count;
}

#include "bgmode.inc"

#include "edsid.inc"

#include "card.inc"

/* Returns the devices ready as a mask: bit 0 memory card 1, bit 1 memory card 2, bit 2 the hard disk. */
get_ready_devices()
{
    new i;
    new status;
    new mask;
    new devices[3] = {EDS_DEVICE_CARD1, EDS_DEVICE_CARD2, EDS_DEVICE_HDD};
    edsClientPoll();
    while (edsClientSync(status))
    {
        sleep;
    }
    i = 0;
    for (; i < 3; i++)
    {
        if (0 == is_device_ready(devices[i], status))
        {
            continue;
        }
        mask = mask | 1 << i;
    }
    return mask;
}

#include "edsid2.inc"

/*
 * Removes the saved ID of device and waits behind the dialog dialog_id; returns 1 when done, with the result in status.
 */
run_eds_remove(focus_obj, device, dialog_id[], &status)
{
    new ok;
    status = edsClientRemove(device);
    if (status < 0)
    {
        printf("@@@ Error edsClientRemove() : [%d]\n", status);
        return 0;
    }
    status = wait_eds_with_dialog(focus_obj, dialog_id);
    switch (status)
    {
        case 0:
        {
            ok = 1;
        }
        case EDS_INVALID_PARAM:
        {
        }
        case EDS_NO_CARD:
        {
        }
        case EDS_FILE_STATE:
        {
        }
        case EDS_CARD_CHANGED:
        {
        }
        default:
        {
            printf("@@@@ Error : Unknown delete status. : [%d]\n", status);
        }
    }
    return ok;
}

new g_focus;
new g_state = STATE_NONE;
new g_next_state = STATE_NONE;
new g_page;
new g_page_slide = SLIDE_IN_NONE; /* the slide of the next open_ed_page */
new g_arrows[4];
new g_psbb_id[16];
new g_password[8];
new g_device; /* the device chosen (EDS_DEVICE_*) */
new g_device_cursor; /* the item chosen in the menu of the devices */
new g_device_menu; /* the menu element of the devices */
new g_text[12288];
new g_has_devices;
new g_input_valid; /* the PSBB ID and the password typed are well formed */
new g_no_id_args = 1; /* opened without the arguments psbbid and password: it goes back to the top page */
new g_ready_devices; /* get_ready_devices when the menu of the devices was built */

main()
{
    init_page();
    for (; ; )
    {
        run_state();
        sleep;
    }
}

init_page()
{
    SystemCommand(SYSCMD_WIPE_ON_FREE);
    start_top_page(0);
    load_background("bg");
    SetPos(Camera(0), 0.0, 0.0, 300.0, 0);
    LookAt(Camera(0), Scene(0), 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
    g_focus = create_focus_plane2("focus");
    SetOptionI(g_focus, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_focus, TAG("ZOFS"), -1.0);
    load_image_resources2();
    create_page_arrows(g_arrows);
    new ret;
    ret = edsClientInit(g_server_host, g_server_port);
    if (ret < 0)
    {
        printf("@@@@ Error : client init fild.\n");
        quit_page();
    }
    {
        new value[64];
        get_client_name(value, 64);
        edsClientSetStr(EDS_FIELD_CLIENT_NAME, value);
        get_client_version(value, 64);
        edsClientSetStr(EDS_FIELD_CLIENT_VERSION, value);
    }
    create_title("msged_savecom_newsave_title");
    {
        new found;
        found = GetArg("psbbid", g_psbb_id, 16);
        if (found == 1)
        {
            g_no_id_args = 0;
        }
        found = GetArg("password", g_password, 8);
        if (found == 1)
        {
            g_no_id_args = 0;
        }
    }
    g_next_state = STATE_INPUT;
}

/* Opens the page of g_next_state when it changes and runs the page shown. */
run_state()
{
    enter_state();
    update_state();
}

/* Opens the page of g_next_state when it changes. */
enter_state()
{
    if (g_state == g_next_state)
    {
        return;
    }
    g_state = g_next_state;
    switch (g_state)
    {
        case STATE_INPUT:
        {
            open_input_page();
        }
        case STATE_SELECT:
        {
            open_select_page();
        }
        case STATE_COMPLETE:
        {
            open_complete_page();
        }
        default:
        {
        }
    }
}

update_state()
{
    switch (g_state)
    {
        case STATE_INPUT:
        {
            update_input_page();
        }
        case STATE_SELECT:
        {
            update_select_page();
        }
        case STATE_COMPLETE:
        {
            update_complete_page();
        }
        default:
        {
        }
    }
}

/* Opens the page of state from the next frame, again if it is already the next one. */
set_next_state(state)
{
    if (g_next_state == state)
    {
        g_state = STATE_NONE;
    }
    g_next_state = state;
}

/*
 * Closes the client and leaves the page: for the top page, or where "retflag" says, or for the member menu when the
 * page was opened with an ID.
 */
quit_page()
{
    edsClientExit();
    kill_object(g_focus);
    new link_id[64];
    new direct;
    g_text = "";
    if (g_no_id_args == 1)
    {
        link_id = "LINK_TOP";
    }
    else
    {
        link_id = "LINK_MEMBER_MENU";
        direct = 1;
        append_member_args(g_text, 12288);
    }
    append_focus_arg(g_text, 12288);
    append_id_save_focus_arg(g_text, 12288);
    load_page(link_id, g_text, direct);
}

open_input_page()
{
    sprintf(g_text, "psbbid=%s;password=%s;", g_psbb_id, g_password);
    g_page = open_ed_page("PAGE_INPUT", g_focus, g_page_slide, g_text);
    g_input_valid = is_id_input_valid();
    update_page_arrows(g_arrows, 0, 1, g_input_valid, 1);
}

/*
 * The messages of the ed_page: the PSBB ID and the password typed ("PARM" and "STR"), right goes on to the devices,
 * cross asks to leave.
 */
update_input_page()
{
    new msg;
    new a;
    new b;
    new c;
    new name[64];
    while (RecvMessageStr(msg, name, 64))
    {
        if (msg == TAG("PARM"))
        {
            if (0 == strcmp("psbbid", name))
            {
                wait_message_str(TAG("STR"), g_psbb_id, 16);
                g_input_valid = is_id_input_valid();
                update_page_arrows(g_arrows, 0, 1, g_input_valid, 1);
            }
            else if (0 == strcmp("password", name))
            {
                wait_message_str(TAG("STR"), g_password, 8);
                g_input_valid = is_id_input_valid();
                update_page_arrows(g_arrows, 0, 1, g_input_valid, 1);
            }
        }
    }
    /* "FOCS": a the direction; "PUSH": b the button ("RRGT" circle, "RDWN" cross). */
    while (RecvMessage(msg, a, b, c))
    {
        SendMessage(g_page, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
        if (msg == TAG("FOCS"))
        {
            if (a == TAG("RIGT"))
            {
                if (g_input_valid == 1)
                {
                    set_next_state(STATE_SELECT);
                    close_ed_page(g_page, g_focus, SLIDE_OUT_LEFT);
                    g_page_slide = SLIDE_IN_RIGHT;
                }
            }
        }
        else if (msg == TAG("PUSH"))
        {
            if (b == TAG("RRGT"))
            {
            }
            else if (b == TAG("RDWN"))
            {
                play_se(ED_SE_CANCEL);
                new choice = pop_dialog(g_focus, "DIALOG_ABORT");
                if (0 == choice)
                {
                    quit_page();
                }
                else
                {
                }
            }
        }
        if (IsValidObject(g_page) == 1)
        {
            SendMessage(g_page, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        }
    }
}

/* Returns 1 when the PSBB ID and the password typed are well formed. */
is_id_input_valid()
{
    if (0 == is_valid_psbb_id(g_psbb_id))
    {
        return 0;
    }
    if (0 == is_valid_password(g_password))
    {
        return 0;
    }
    return 1;
}

open_select_page()
{
    new count;
    g_ready_devices = get_ready_devices();
    count = build_device_menu(g_device_menu);
    g_has_devices = count > 0 ? 1 : 0;
    g_text = "";
    g_page = open_ed_page("PAGE_SELECT", g_focus, g_page_slide, g_text);
    update_page_arrows(g_arrows, 1, 0, 1, 1);
}

/*
 * The messages of the ed_page, after building the page again if the devices that can be used changed: the device
 * chosen ("PARM", "ID" and "VAL"), circle saves on it, left goes back, cross asks to leave.
 */
update_select_page()
{
    new msg;
    new a;
    new b;
    new c;
    new id[64];
    new devices = get_ready_devices();
    if (g_ready_devices != devices)
    {
        set_next_state(STATE_SELECT);
        close_ed_page(g_page, g_focus, SLIDE_OUT_NONE);
        while (RecvMessageStr(msg, id, 64))
        {
        }
        while (RecvMessage(msg, a, b, c))
        {
        }
        return;
    }
    /* "FOCS": a the direction; "PUSH": b the button ("RRGT" circle, "RDWN" cross); "PARM": "ID" and "VAL" follow. */
    while (RecvMessage(msg, a, b, c))
    {
        if (msg == TAG("FOCS"))
        {
            SendMessage(g_page, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
            if (a == TAG("RIGT"))
            {
            }
            else if (a == TAG("LEFT"))
            {
                set_next_state(STATE_INPUT);
                close_ed_page(g_page, g_focus, SLIDE_OUT_RIGHT);
                g_page_slide = SLIDE_IN_LEFT;
            }
            if (IsValidObject(g_page) == 1)
            {
                SendMessage(g_page, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
            }
        }
        else if (msg == TAG("PUSH"))
        {
            SendMessage(g_page, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
            if (b == TAG("RRGT"))
            {
                if (g_has_devices == 1)
                {
                    save_to_device(g_device_cursor);
                }
            }
            else if (b == TAG("RDWN"))
            {
                play_se(ED_SE_CANCEL);
                new choice = pop_dialog(g_focus, "DIALOG_ABORT");
                if (0 == choice)
                {
                    quit_page();
                }
                else
                {
                }
            }
            if (IsValidObject(g_page) == 1)
            {
                SendMessage(g_page, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
            }
        }
        else if (msg == TAG("PARM"))
        {
            wait_message_str(TAG("ID"), id, 64);
            if (0 == strcmp("item0", id))
            {
                wait_message(TAG("VAL"), a);
                g_device_cursor = a;
            }
        }
    }
}

/* Saves the ID on the device of item of the menu, then goes on to the complete page, or back to the devices. */
save_to_device(item)
{
    new saved;
    update_page_arrows(g_arrows, 0, 0, 1, 1);
    close_ed_page(g_page, g_focus, SLIDE_OUT_NONE);
    g_device = get_item_device(g_device_menu, item);
    switch (g_device)
    {
        case EDS_DEVICE_CARD1:
        {
            saved = save_card_id(EDS_DEVICE_CARD1);
        }
        case EDS_DEVICE_CARD2:
        {
            saved = save_card_id(EDS_DEVICE_CARD2);
        }
        case EDS_DEVICE_HDD:
        {
            saved = save_hdd_id(EDS_DEVICE_HDD);
        }
        default:
        {
        }
    }
    if (saved == 1)
    {
        set_next_state(STATE_COMPLETE);
    }
    else
    {
        set_next_state(STATE_SELECT);
    }
}

open_complete_page()
{
    g_text = "";
    g_page = open_ed_page("PAGE_COMPLETE", g_focus, g_page_slide, g_text);
    update_page_arrows(g_arrows, 0, 0, 1, 1);
}

/* The messages of the ed_page: cross leaves. */
update_complete_page()
{
    new msg;
    new a;
    new b;
    new c;
    /* "FOCS": a the direction; "PUSH": b the button ("RRGT" circle, "RDWN" cross). */
    while (RecvMessage(msg, a, b, c))
    {
        SendMessage(g_page, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
        if (msg == TAG("FOCS"))
        {
            if (a == TAG("RIGT"))
            {
            }
        }
        else if (msg == TAG("PUSH"))
        {
            if (b == TAG("RRGT"))
            {
            }
            else if (b == TAG("RDWN"))
            {
                quit_page();
            }
        }
        if (IsValidObject(g_page) == 1)
        {
            SendMessage(g_page, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        }
    }
}

/*
 * Saves the ID on the memory card device, answering its errors with dialogs: it may format the card, or remove a
 * saved ID that cannot be used and save again. Returns 1 when saved.
 */
save_card_id(device)
{
    new ret;
    new saved;
    edsClientSetStr(EDS_FIELD_AUTH_ID, g_psbb_id);
    edsClientSetStr(EDS_FIELD_AUTH_PASSWD, g_password);
    ret = edsClientSave(device);
    if (ret < 0)
    {
        printf("@@@ Error edsClientSave() : [%d]\n", ret);
        pop_dialog(g_focus, "ERR_DIALOG_SAVE_FAIL");
        quit_page();
    }
    ret = wait_eds_with_dialog(g_focus, "DIALOG_SAVE_MEMORY_CARD");
    switch (ret)
    {
        case 0:
        {
            saved = 1;
        }
        case EDS_INVALID_PARAM:
        {
            new choice = pop_dialog(g_focus, "DIALOG_MC_INVALID_PARAM");
            if (0 == choice)
            {
                saved = resave_card_id(device);
            }
            else
            {
            }
        }
        case EDS_INVALID_ID:
        {
            pop_dialog(g_focus, "ERR_DIALOG_SAVE_FAIL");
            quit_page();
        }
        case EDS_NO_CARD:
        {
            pop_dialog(g_focus, "DIALOG_NO_CARD");
        }
        case EDS_UNFORMATTED:
        {
            new status = pop_dialog(g_focus, "DIALOG_FORMAT_CONFIRM");
            if (0 == status)
            {
                saved = run_eds_format(g_focus, device, "DIALOG_FORMAT_MEMORY_CARD", status);
                if (saved == 1)
                {
                    saved = save_card_id(device);
                }
                else if (status == EDS_NO_CARD)
                {
                    pop_dialog(g_focus, "DIALOG_NO_CARD");
                }
                else if (status == EDS_CARD_CHANGED)
                {
                    pop_dialog(g_focus, "DIALOG_CHANGE_CARD");
                }
                else
                {
                    pop_dialog(g_focus, "ERR_DIALOG_FORMAT_FAIL");
                    quit_page();
                }
            }
            else
            {
            }
        }
        case EDS_NO_SPACE:
        {
            pop_dialog(g_focus, "DIALOG_MC_NO_SPACE");
        }
        case EDS_FILE_STATE:
        {
            ret = edsClientLoad(device);
            if (ret < 0)
            {
                printf("@@@ Error edsClientLoad() : [%d]\n", ret);
                quit_page();
            }
            while (edsClientSync(ret))
            {
                sleep;
            }
            if (ret == EDS_INVALID_ID)
            {
                new choice = pop_dialog(g_focus, "DIALOG_MC_INVALID_ID");
                if (0 == choice)
                {
                    saved = resave_card_id(device);
                }
                else
                {
                }
            }
            else
            {
                pop_dialog(g_focus, "DIALOG_EXIST_FILE");
            }
        }
        case EDS_BAD_VERSION:
        {
            new choice = pop_dialog(g_focus, "DIALOG_MC_NOSUPPORT_VERSION");
            if (0 == choice)
            {
                saved = resave_card_id(device);
            }
            else
            {
            }
        }
        case EDS_CARD_CHANGED:
        {
            pop_dialog(g_focus, "DIALOG_CHANGE_CARD");
        }
        default:
        {
        }
    }
    return saved;
}

/* Removes the saved ID of the memory card device and saves again; returns 1 when saved. */
resave_card_id(device)
{
    new status;
    new saved;
    saved = run_eds_remove(g_focus, device, "DIALOG_DELETE_MEMORY_CARD", status);
    if (saved == 1)
    {
        saved = save_card_id(device);
    }
    else if (status == EDS_NO_CARD)
    {
        pop_dialog(g_focus, "DIALOG_NO_CARD");
    }
    else if (status == EDS_CARD_CHANGED)
    {
        pop_dialog(g_focus, "DIALOG_CHANGE_CARD");
    }
    else
    {
        pop_dialog(g_focus, "ERR_DIALOG_DELETE_FAIL");
        quit_page();
    }
    return saved;
}

/* Like save_card_id, on the hard disk. */
save_hdd_id(device)
{
    new ret;
    new saved;
    edsClientSetStr(EDS_FIELD_AUTH_ID, g_psbb_id);
    edsClientSetStr(EDS_FIELD_AUTH_PASSWD, g_password);
    ret = edsClientSave(device);
    if (ret < 0)
    {
        printf("@@@ Error edsClientSave() : [%d]\n", ret);
        pop_dialog(g_focus, "ERR_DIALOG_SAVE_FAIL");
        quit_page();
    }
    ret = wait_eds_with_dialog(g_focus, "DIALOG_SAVE_HDD");
    switch (ret)
    {
        case 0:
        {
            saved = 1;
        }
        case EDS_INVALID_PARAM:
        {
            new choice = pop_dialog(g_focus, "DIALOG_HDD_INVALID_PARAM");
            if (0 == choice)
            {
                saved = resave_hdd_id(device);
            }
            else
            {
            }
        }
        case EDS_INVALID_ID:
        {
            pop_dialog(g_focus, "ERR_DIALOG_SAVE_FAIL");
            quit_page();
        }
        case EDS_NO_SPACE:
        {
            pop_dialog(g_focus, "DIALOG_HDD_NO_SPACE");
        }
        case EDS_FILE_STATE:
        {
            ret = edsClientLoad(device);
            if (ret < 0)
            {
                printf("@@@ Error edsClientLoad() : [%d]\n", ret);
                quit_page();
            }
            while (edsClientSync(ret))
            {
                sleep;
            }
            if (ret == EDS_INVALID_ID)
            {
                new choice = pop_dialog(g_focus, "DIALOG_HDD_INVALID_ID");
                if (0 == choice)
                {
                    saved = resave_hdd_id(device);
                }
                else
                {
                }
            }
            else
            {
                pop_dialog(g_focus, "DIALOG_EXIST_FILE");
            }
        }
        case EDS_BAD_VERSION:
        {
            new choice = pop_dialog(g_focus, "DIALOG_HDD_NOSUPPORT_VERSION");
            if (0 == choice)
            {
                saved = resave_hdd_id(device);
            }
            else
            {
            }
        }
        default:
        {
        }
    }
    return saved;
}

/* Like resave_card_id, on the hard disk. */
resave_hdd_id(device)
{
    new status;
    new saved;
    saved = run_eds_remove(g_focus, device, "DIALOG_DELETE_HDD", status);
    if (saved == 1)
    {
        saved = save_hdd_id(device);
    }
    else
    {
        pop_dialog(g_focus, "ERR_DIALOG_DELETE_FAIL");
        quit_page();
    }
    return saved;
}

#include "edsreq.inc"
