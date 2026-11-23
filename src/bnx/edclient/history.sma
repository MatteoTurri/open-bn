/*
 * /opt0/bn/script/edclient/history.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The page of the purchase history of the paid services, opened from the service menu: the information of the
 * server, the PSBB ID and password, then the list of the purchases, where triangle shows the details of one.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "edpage.inc"

/* Values of g_state: the page shown. */
const STATE_INFORMATION = 0;
const STATE_CERTIFICATE = 1;
const STATE_HISTORY = 2;
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

#include "text.inc"

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
new g_arg_service_focus[] = "service_focus";

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

/* Appends the argument service_focus to args, like append_member_args. */
append_focus_arg(args[], maxlen)
{
    new value[64];
    if (GetArg(g_arg_service_focus, value, 64) == 1)
    {
        sprintf(value, "%s=%s;", g_arg_service_focus, value);
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
open_ed_page(page_id[], focus_obj, &slide, args[] = "")
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

/* The server of the paid services and the name and version this client gives it. */
new g_server_host[] = "lib.feega.sonyfinance.co.jp";
new g_server_port = 14020;
new g_client_name[] = "BBnavi";
new g_client_version[] = "1.0.0";

#include "edsreq.inc"

#include "validate.inc"

#include "pfs.inc"

#include "date.inc"

new g_default_game_id[1];

/* Copies the name of the client, "BBnavi". */
get_client_name(name[], maxlen)
{
    copy_string(name, g_client_name, maxlen);
}

/* Reads the argument GameVer, or "1.0.0" without it. */
get_client_version(version[], maxlen)
{
    new found;
    found = GetArg(g_arg_game_ver, version, maxlen);
    if (0 == found)
    {
        copy_string(version, g_client_version, maxlen);
    }
}

/* Reads the argument GameID, or the empty g_default_game_id without it. */
get_game_id(game_id[], maxlen)
{
    new found;
    found = GetArg(g_arg_game_id, game_id, maxlen);
    if (0 == found)
    {
        copy_string(game_id, g_default_game_id, maxlen);
    }
}

/* Returns the argument unloadBGP if it is 0 or 1, else 0; with 1 the page does not load its background. */
get_unload_bgp_arg()
{
    new unload;
    new value[32];
    new found;
    found = GetArg(g_arg_unload_bgp, value, 32);
    if (0 == found)
    {
        return;
    }
    unload = atoi(value);
    return 0 == unload || unload == 1 ? unload : 0;
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

new g_focus;
new g_state = STATE_NONE;
new g_next_state = STATE_NONE;
new g_page;
new g_page_slide = SLIDE_IN_NONE; /* the slide of the next open_ed_page */
new g_arrows[4];
new g_show_prev;
new g_show_next;
new g_psbb_id[16];
new g_password[8];
new g_cursor; /* the purchase chosen in the list */
new g_history_menu; /* the menu element of the list */
new g_game_id[64];
new g_has_history;
new g_input_valid; /* the PSBB ID and the password typed are well formed */
new g_info_error; /* the error of the information request, shown when the page is left */
new g_service_status; /* not 0: the service cannot be used */
new g_text[12288];

main()
{
    init_page();
    for (; ; )
    {
        enter_state();
        update_state();
        sleep;
    }
}

init_page()
{
    new ret;
    SystemCommand(SYSCMD_WIPE_ON_FREE);
    start_top_page(0);
    if (0 == get_unload_bgp_arg())
    {
        load_background("bg");
    }
    SetPos(Camera(0), 0.0, 0.0, 300.0, 0);
    LookAt(Camera(0), Scene(0), 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
    g_focus = create_focus_plane2("focus");
    SetOptionI(g_focus, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_focus, TAG("ZOFS"), -1.0);
    load_image_resources2();
    create_page_arrows(g_arrows);
    ret = edsClientInit(g_server_host, g_server_port);
    if (ret < 0)
    {
        printf("Error : client init fail.\n");
        quit_page();
    }
    {
        new value[64];
        get_client_name(value, 64);
        edsClientSetStr(EDS_FIELD_CLIENT_NAME, value);
        get_client_version(value, 64);
        edsClientSetStr(EDS_FIELD_CLIENT_VERSION, value);
    }
    get_game_id(g_game_id, 64);
    create_title("msged_buy_titlemenu_uselist_check");
    fetch_information();
    g_next_state = STATE_INFORMATION;
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
        case STATE_INFORMATION:
        {
            open_information_page();
        }
        case STATE_CERTIFICATE:
        {
            open_certificate_page();
        }
        case STATE_HISTORY:
        {
            open_history_page();
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
        case STATE_INFORMATION:
        {
            update_information_page();
        }
        case STATE_CERTIFICATE:
        {
            update_certificate_page();
        }
        case STATE_HISTORY:
        {
            update_history_page();
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

/* Closes the client and leaves the page for the top page, or where "retflag" says. */
quit_page()
{
    edsClientExit();
    kill_object(g_focus);
    new args[64];
    args = "";
    append_focus_arg(args, 64);
    load_page("LINK_TOP", args, 0);
}

/* Gets the information of the server into the TEXT element mess_infomation; on an error, leaves. */
fetch_information()
{
    new ret;
    ret = run_eds_request(EDS_REQ_GET_HISTORY_INFO, "PAGE_SERVER_CONNECT", g_focus, g_arrows, 1);
    if (ret < 0)
    {
        if (ret == EDS_STATUS_ERROR)
        {
            g_info_error = ret;
        }
        else
        {
            if (ret == EDS_CANCELED)
            {
                quit_page();
                return 0;
            }
            if (ret == EDS_NETWORK_ERROR)
            {
                pop_dialog(g_focus, "ERR_DIALOG_NETWORK_DISCONNECT");
                quit_page();
                return 0;
            }
            pop_dialog(g_focus, "ERR_DIALOG_ED_SYSTEM");
            quit_page();
            return 0;
        }
    }
    g_service_status = edsClientGetInt(HISTORY_INFO_SERVICE_STATUS);
    ret = edsClientGetStr(HISTORY_INFO_MESSAGE, g_text, 12288);
    set_text_value("mess_infomation", g_text);
    return 1;
}

/*
 * Gets the purchases of the PSBB ID from the server into g_history_menu; returns 1, or 0 back on the certificate
 * page when the ID is refused or the request canceled.
 */
fetch_history()
{
    new ret;
    edsClientSetStr(EDS_FIELD_AUTH_SYSTEM_ID, "001");
    edsClientSetStr(EDS_FIELD_AUTH_ID, g_psbb_id);
    edsClientSetStr(EDS_FIELD_AUTH_PASSWD, g_password);
    edsClientSetStr(EDS_FIELD_GROUP_ID, g_game_id);
    ret = run_eds_request(EDS_REQ_GET_HISTORY, "PAGE_SERVER_CONNECT", g_focus, g_arrows, 1);
    if (ret < 0)
    {
        if (ret == EDS_CANCELED)
        {
            set_next_state(STATE_CERTIFICATE);
            return 0;
        }
        if (ret == EDS_NETWORK_ERROR)
        {
            pop_dialog(g_focus, "ERR_DIALOG_NETWORK_DISCONNECT");
        }
        else
        {
            pop_dialog(g_focus, "ERR_DIALOG_ED_SYSTEM");
        }
        quit_page();
        return 0;
    }
    ret = edsClientGetInt(HISTORY_AUTH_STATUS);
    if (0 != ret)
    {
        pop_dialog(g_focus, "ERR_DIALOG_AUTHNG");
        set_next_state(STATE_CERTIFICATE);
        g_page_slide = SLIDE_IN_LEFT;
        return 0;
    }
    new count = edsClientGetInt(HISTORY_REAL_SALES_NUMBER);
    if (0 == count)
    {
        pop_dialog(g_focus, "ERR_DIALOG_HISTORY_NOLIST");
        quit_page();
    }
    if (count > 50)
    {
        pop_dialog(g_focus, "ERR_DIALOG_HISTORY_MAX");
    }
    new listed = build_history_menu();
    g_has_history = listed > 0 ? 1 : 0;
    return 1;
}

/*
 * Builds the menu element menu_history from the purchases of the answer, at most 50: an item each, with date and
 * service as its text; returns their number.
 */
build_history_menu()
{
    new ret;
    new count = edsClientGetInt(HISTORY_COUNT);
    if (0 != g_history_menu)
    {
        domRemoveTag(g_history_menu);
        g_history_menu = 0;
    }
    g_history_menu = domAddTag(domRoot(), "menu");
    domSetData(g_history_menu, "id", "menu_history");
    new listed;
    for (new i; i < count; i++)
    {
        if (listed >= 50)
        {
            break;
        }
        if (edsClientEnter(i) < 0)
        {
            continue;
        }
        new item = domAddTag(g_history_menu, "item");
        ret = edsClientGetStr(SALES_SERVICE_ID, g_text, 12288);
        domSetData(item, "id", g_text);
        {
            new buf[32];
            new year[8];
            new month[8];
            new day[8];
            ret = edsClientGetStr(SALES_SALES_DATE, buf, 32);
            split_date(buf, year, month, day, 8, 8, 8);
            copy_substring(buf, 0, year, 2, 2, 32);
            copy_string(year, buf, 8);
            ret = edsClientGetStr(SALES_NAME, buf, 32);
            get_tag_attr("TEXT", "msged_history_string", "value", g_text, 12288);
            sprintf(g_text, g_text, atoi(year), atoi(month), atoi(day), buf);
            domSetData(item, "value", g_text);
        }
        ret = edsClientGetStr(SALES_ORDER_NO, g_text, 12288);
        domSetData(item, "order", g_text);
        ret = edsClientGetStr(SALES_NAME, g_text, 12288);
        domSetData(item, "name", g_text);
        ret = edsClientGetStr(SALES_SALES_DATE, g_text, 12288);
        domSetData(item, "date", g_text);
        ret = edsClientGetStr(SALES_PAYMENT_NAME, g_text, 12288);
        domSetData(item, "paymentname", g_text);
        ret = edsClientGetInt(SALES_PRICE);
        sprintf(g_text, "%d", ret);
        domSetData(item, "price", g_text);
        ret = edsClientGetInt(SALES_TAX_AMOUNT);
        sprintf(g_text, "%d", ret);
        domSetData(item, "tax", g_text);
        ret = edsClientGetInt(SALES_TAX_PRICE);
        sprintf(g_text, "%d", ret);
        domSetData(item, "taxprice", g_text);
        listed++;
        edsClientLeave();
    }
    return listed;
}

open_information_page()
{
    g_text = "";
    g_page = open_ed_page("PAGE_INFOMATION", g_focus, g_page_slide, g_text);
    update_page_arrows(g_arrows, 0, 1, 1, 1);
}

/* The messages of the ed_page: right goes on to the certificate page, cross leaves. */
update_information_page()
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
                close_ed_page(g_page, g_focus, SLIDE_OUT_LEFT);
                if (0 != g_info_error)
                {
                    pop_dialog(g_focus, "ERR_DIALOG_ED_SYSTEM");
                    quit_page();
                }
                if (0 != g_service_status)
                {
                    pop_dialog(g_focus, "ERR_DIALOG_SERVICE_NG");
                    quit_page();
                }
                set_next_state(STATE_CERTIFICATE);
                g_page_slide = SLIDE_IN_RIGHT;
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

open_certificate_page()
{
    sprintf(g_text, "psbbid=%s;password=%s;", g_psbb_id, g_password);
    g_page = open_ed_page("PAGE_CERTIFICATE", g_focus, g_page_slide, g_text);
    g_show_prev = 1;
    g_show_next = 1;
    update_page_arrows(g_arrows, g_show_prev, g_show_next, g_input_valid, 1);
}

/*
 * The messages of the ed_page: the PSBB ID and the password typed ("PARM" and "STR"), right fetches the history
 * and goes on, left goes back, cross asks to leave.
 */
update_certificate_page()
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
                update_page_arrows(g_arrows, g_show_prev, g_show_next, g_input_valid, 1);
            }
            else if (0 == strcmp("password", name))
            {
                wait_message_str(TAG("STR"), g_password, 8);
                g_input_valid = is_id_input_valid();
                update_page_arrows(g_arrows, g_show_prev, g_show_next, g_input_valid, 1);
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
                    close_ed_page(g_page, g_focus, SLIDE_OUT_LEFT);
                    new fetched = fetch_history();
                    if (fetched == 1)
                    {
                        set_next_state(STATE_HISTORY);
                        g_page_slide = SLIDE_IN_RIGHT;
                    }
                    else
                    {
                    }
                }
            }
            else if (a == TAG("LEFT"))
            {
                close_ed_page(g_page, g_focus, SLIDE_OUT_RIGHT);
                g_page_slide = SLIDE_IN_LEFT;
                set_next_state(STATE_INFORMATION);
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

/* Opens the list of the purchases with g_cursor chosen. */
open_history_page()
{
    {
        new node;
        node = find_child_tag2(domRoot(), "page", "id", "PAGE_HISTORY");
        node = find_child_tag2(node, "item", "id", "item0");
        domRemoveData(node, "current");
        sprintf(g_text, "%d", g_cursor);
        domSetData(node, "current", g_text);
    }
    g_text = "";
    g_page = open_ed_page("PAGE_HISTORY", g_focus, g_page_slide, g_text);
    update_page_arrows(g_arrows, 1, 0, 1, 1);
}

/*
 * The messages of the ed_page: the purchase chosen ("PARM", "ID" and "VAL"), triangle shows its details, left goes
 * back, cross leaves.
 */
update_history_page()
{
    new msg;
    new a;
    new b;
    new c;
    new id[64];
    /* "FOCS": a the direction; "PUSH": b the button ("RRGT" circle, "RDWN" cross); "PARM": "ID" and "VAL" follow. */
    while (RecvMessage(msg, a, b, c))
    {
        if (msg == TAG("FOCS"))
        {
            SendMessage(g_page, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
            if (a == TAG("LEFT"))
            {
                set_next_state(STATE_CERTIFICATE);
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
            }
            else if (b == TAG("RUP"))
            {
                if (g_has_history == 1)
                {
                    show_history_detail(g_cursor);
                }
            }
            else if (b == TAG("RDWN"))
            {
                quit_page();
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
                g_cursor = a;
            }
        }
    }
}

/* Shows the details of the purchase index of g_history_menu in a dialog. */
show_history_detail(index)
{
    new line[64];
    new value[64];
    new item;
    item = find_nth_child_tag(g_history_menu, "item", index);
    g_text = "";
    get_tag_attr("TEXT", "msged_history_orderno_string", "value", line, 64);
    domGetData(item, "order", value, 64);
    sprintf(line, line, value);
    append_string(g_text, line, 12288);
    append_string(g_text, "\n", 12288);
    get_tag_attr("TEXT", "msged_history_service_string", "value", line, 64);
    domGetData(item, "name", value, 64);
    sprintf(line, line, value);
    append_string(g_text, line, 12288);
    append_string(g_text, "\n", 12288);
    get_tag_attr("TEXT", "msged_history_payment_string", "value", line, 64);
    domGetData(item, "paymentname", value, 64);
    sprintf(line, line, value);
    append_string(g_text, line, 12288);
    append_string(g_text, "\n", 12288);
    {
        new year[8];
        new month[8];
        new day[8];
        domGetData(item, "date", value, 64);
        split_date(value, year, month, day, 8, 8, 8);
        get_tag_attr("TEXT", "msged_history_recive_string", "value", line, 64);
        sprintf(line, line, atoi(year), atoi(month), atoi(day));
        append_string(g_text, line, 12288);
        append_string(g_text, "\n", 12288);
    }
    get_tag_attr("TEXT", "msged_history_sum_string", "value", line, 64);
    domGetData(item, "price", value, 64);
    sprintf(line, line, value);
    append_string(g_text, line, 12288);
    append_string(g_text, "\n", 12288);
    get_tag_attr("TEXT", "msged_history_tax_string", "value", line, 64);
    domGetData(item, "tax", value, 64);
    sprintf(line, line, value);
    append_string(g_text, line, 12288);
    append_string(g_text, "\n", 12288);
    get_tag_attr("TEXT", "msged_history_sumintax_string", "value", line, 64);
    domGetData(item, "taxprice", value, 64);
    sprintf(line, line, value);
    append_string(g_text, line, 12288);
    append_string(g_text, "\n", 12288);
    pop_dialog(g_focus, "DIALOG_DETAIL", g_text);
}
