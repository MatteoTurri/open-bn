/*
 * /opt0/bn/script/edclient/change.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The page that changes the registration of a member: logs the member in, shows the registered data and lets
 * the user change the password, the mail address and its delivery, the zip code and the credit card, then
 * sends the changes. A new password can then be saved with the ID (id_change).
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "edpage.inc"

/* Values of g_state: the page shown. */
const STATE_INFORMATION = 0; /* the information of the service */
const STATE_CERTIFICATE = 1;
const STATE_CONTENTS = 2;    /* the registered data */
const STATE_CHANGE_START = 3;
const STATE_PASSWORD = 4;
const STATE_MAIL_ADDR = 5;
const STATE_MAIL_SEND = 6;   /* whether the member wants mail */
const STATE_ZIP = 7;
const STATE_CARD_MENU = 8;   /* keep, change or delete the card */
const STATE_CARD = 9;
const STATE_SECURITY_CODE = 10;
const STATE_CONFIRM = 11;    /* the data with the changes */
const STATE_SEND_CONFIRM = 12;
const STATE_COMPLETE = 13;
const STATE_NONE = 14;       /* no page: the next one opens even if it is the same */

/* Values of g_card_change: the items of the card menu. */
const CARD_KEEP = 0;
const CARD_NEW = 1;
const CARD_DELETE = 2;

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
new g_arg_member_focus[] = "member_focus";

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

#include "misc.inc"

/*
 * Creates a nine-slice plane at rect (x, y, z, width, height; with no size, the texture's), whose edges keep the size
 * in pixels given by border (left, right, top, bottom).
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
load_page(link_id[], args[] = "", direct)
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
    new node;
    node = domFindTag(domRoot(), "IMG_RESOURCE", "IMG_NEXT", 0);
    domGetData(node, "texture", buf, 64);
    texture = atoi(buf);
    domGetData(node, "w", buf, 64);
    tex_w = atof(buf);
    domGetData(node, "h", buf, 64);
    tex_h = atof(buf);
    rect[0] = 600.0;
    rect[1] = 260.0;
    rect[2] = 0;
    rect[3] = 0;
    rect[4] = 0;
    zsort = ZSORT_NONE;
    color = pack_color(255, 255, 255, 0);
    arrows[PAGE_ARROW_NEXT] = create_frame_plane(Disp2D(0), rect, zsort, color, texture, tex_w, tex_h);
    node = domFindTag(domRoot(), "IMG_RESOURCE", "IMG_NEXT_HI", 0);
    domGetData(node, "texture", buf, 64);
    texture = atoi(buf);
    domGetData(node, "w", buf, 64);
    tex_w = atof(buf);
    domGetData(node, "h", buf, 64);
    tex_h = atof(buf);
    rect[0] = 600.0;
    rect[1] = 260.0;
    rect[2] = 0;
    rect[3] = 0;
    rect[4] = 0;
    zsort = ZSORT_NONE;
    color = pack_color(255, 255, 255, 0);
    arrows[PAGE_ARROW_NEXT_LIT] = create_frame_plane(Disp2D(0), rect, zsort, color, texture, tex_w, tex_h);
    node = domFindTag(domRoot(), "IMG_RESOURCE", "IMG_PREV", 0);
    domGetData(node, "texture", buf, 64);
    texture = atoi(buf);
    domGetData(node, "w", buf, 64);
    tex_w = atof(buf);
    domGetData(node, "h", buf, 64);
    tex_h = atof(buf);
    rect[0] = 40.0;
    rect[1] = 260.0;
    rect[2] = 0;
    rect[3] = 0;
    rect[4] = 0;
    zsort = ZSORT_NONE;
    color = pack_color(255, 255, 255, 0);
    arrows[PAGE_ARROW_PREV] = create_frame_plane(Disp2D(0), rect, zsort, color, texture, tex_w, tex_h);
    node = domFindTag(domRoot(), "IMG_RESOURCE", "IMG_PREV_HI", 0);
    domGetData(node, "texture", buf, 64);
    texture = atoi(buf);
    domGetData(node, "w", buf, 64);
    tex_w = atof(buf);
    domGetData(node, "h", buf, 64);
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
new g_default_period[] = "200201"; /* the first expiry of the card, YYYYMM */

#include "edsreq.inc"

#include "validate.inc"

new g_no_inquiry_head[1]; /* empty: the halves of an inquiry ID too short */
new g_no_inquiry_tail[1];

/* Same as is_not_empty of validate.inc. */
is_not_empty2(text[])
{
    if (strlen(text) > 0)
    {
        return 1;
    }
    return 0;
}

#include "pfs.inc"

/* Same as is_not_empty of validate.inc. */
is_not_empty3(text[])
{
    if (strlen(text) > 0)
    {
        return 1;
    }
    return 0;
}

#include "card.inc"

#include "date.inc"

/* Splits the first 7 characters of zip into its first 3 and last 4 digits; both are empty for a shorter code. */
split_zip_code(zip[], head[], tail[], head_max, tail_max)
{
    if (strlen(zip) < 7)
    {
        head{0} = 0;
        tail{0} = 0;
        return;
    }
    copy_substring(head, 0, zip, 0, 3, head_max);
    copy_substring(tail, 0, zip, 3, 4, tail_max);
}

/* Splits an inquiry ID into its first and last 4 characters; both empty if it is shorter than 8. */
split_inquiry_id(inquiry_id[], head[], tail[], head_max, tail_max)
{
    if (strlen(inquiry_id) < 8)
    {
        copy_string(head, g_no_inquiry_head, head_max);
        copy_string(tail, g_no_inquiry_tail, tail_max);
        return;
    }
    copy_substring(head, 0, inquiry_id, 0, 4, head_max);
    copy_substring(tail, 0, inquiry_id, 4, 4, tail_max);
}

#include "fileop.inc"

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

#include "title.inc"

/*
 * Opens the dialog of the "dialog" element dialog_id (its type, title, texts and items), with extra after the texts,
 * and waits for it; returns the button chosen, 1 when it was canceled. The pad help is restored.
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
new g_next_state = STATE_NONE; /* the page to show from the next frame */
new g_page;
new g_page_slide = SLIDE_IN_NONE; /* the slide of the next open_ed_page */
new g_arrows[4];
new g_inquiry_id[64]; /* the registered data of the member, from the server */
new g_family_name[10];
new g_given_name[10];
new g_sex;
new g_birthday[3]; /* YYYYMMDD, split into the three below */
new g_birth_year[2];
new g_birth_month[1];
new g_birth_day[1];
new g_psbb_id[16]; /* the PSBB ID and password of the member */
new g_password[8];
new g_new_password[8]; /* the new values; the password and the address typed twice */
new g_new_password_again[8];
new g_mail_addr[17];
new g_new_mail_addr[17];
new g_new_mail_addr_again[17];
new g_sent_mail_addr[17]; /* g_new_mail_addr when the changes were last sent */
new g_mail_hope = 1;
new g_new_mail_hope = 1;
new g_zipcode[2];
new g_new_zipcode[2];
new g_card_number[5];
new g_new_card_number[5];
new g_card_period[2]; /* the expiry of the new card, YYYYMM */
new g_need_security_code; /* the card checked needs its security code */
new g_security_code[2];
new g_check_mail_domain = 1; /* the server checks the domain of the mail address */
new g_card_valid; /* the values typed are complete, for the arrow to the next page */
new g_input_valid;
new g_password_typed;
new g_mail_addr_typed;
new g_security_code_valid;
new g_card_change;
new g_card_menu;    /* the "menu" element select_change */
new g_retry_count;  /* the sendings refused: more than 5 leave the page */
new g_info_error;   /* EDS_STATUS_ERROR from the check of the service, told when the user goes on */
new g_service_status; /* nonzero: the service was refused */
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
    load_background("bg");
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
        printf("@@@@ Error : client init fail.\n");
        load_page("LINK_TOP", _, 0);
    }
    {
        new value[64];
        get_client_name(value, 64);
        edsClientSetStr(EDS_FIELD_CLIENT_NAME, value);
        get_client_version(value, 64);
        edsClientSetStr(EDS_FIELD_CLIENT_VERSION, value);
    }
    copy_string(g_card_period, g_default_period, 2);
    create_title("msged_feega_title_confirm_registration");
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
        case STATE_CONTENTS:
        {
            open_contents_page();
        }
        case STATE_CHANGE_START:
        {
            open_change_start_page();
        }
        case STATE_PASSWORD:
        {
            open_password_page();
        }
        case STATE_MAIL_ADDR:
        {
            open_mail_addr_page();
        }
        case STATE_MAIL_SEND:
        {
            open_mail_send_page();
        }
        case STATE_ZIP:
        {
            open_zip_page();
        }
        case STATE_CARD_MENU:
        {
            open_card_menu_page();
        }
        case STATE_CARD:
        {
            open_card_page();
        }
        case STATE_SECURITY_CODE:
        {
            open_security_code_page();
        }
        case STATE_CONFIRM:
        {
            open_confirm_page();
        }
        case STATE_SEND_CONFIRM:
        {
            open_send_confirm_page();
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
        case STATE_INFORMATION:
        {
            update_information_page();
        }
        case STATE_CERTIFICATE:
        {
            update_certificate_page();
        }
        case STATE_CONTENTS:
        {
            update_contents_page();
        }
        case STATE_CHANGE_START:
        {
            update_change_start_page();
        }
        case STATE_PASSWORD:
        {
            update_password_page();
        }
        case STATE_MAIL_ADDR:
        {
            update_mail_addr_page();
        }
        case STATE_MAIL_SEND:
        {
            update_mail_send_page();
        }
        case STATE_ZIP:
        {
            update_zip_page();
        }
        case STATE_CARD_MENU:
        {
            update_card_menu_page();
        }
        case STATE_CARD:
        {
            update_card_page();
        }
        case STATE_SECURITY_CODE:
        {
            update_security_code_page();
        }
        case STATE_CONFIRM:
        {
            update_confirm_page();
        }
        case STATE_SEND_CONFIRM:
        {
            update_send_confirm_page();
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
 * Closes the client and leaves for the page of the LINK link_id with args and member_focus; unless direct, where
 * "retflag" says.
 */
quit_page(link_id[] = "LINK_TOP", args[] = "", direct)
{
    edsClientExit();
    kill_object(g_focus);
    new page_args[128];
    append_string(page_args, args, 128);
    append_focus_arg(page_args, 128);
    load_page(link_id, page_args, direct);
}

/* Asks with DIALOG_ABORT whether to leave the page, and leaves on yes. */
ask_abort(play_cancel_se)
{
    new choice;
    if (play_cancel_se == 1)
    {
        play_se(ED_SE_CANCEL);
    }
    choice = pop_dialog(g_focus, "DIALOG_ABORT");
    if (0 == choice)
    {
        quit_page(_, _, 0);
    }
    else
    {
    }
}

/*
 * Has the server check the service and puts its information in the TEXT element mess_infomation; the errors other than
 * EDS_STATUS_ERROR, kept in g_info_error, leave.
 */
fetch_information()
{
    new ret;
    edsClientSetInt(EDS_FIELD_MEMBER_STATUS, 1); /* the information for a change of the registered data */
    ret = run_eds_request(EDS_REQ_GET_MEMBER_ADMIN_INFO, "PAGE_SERVER_CONNECT", g_focus, g_arrows, 1);
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
                quit_page(_, _, 0);
                return 0;
            }
            if (ret == EDS_NETWORK_ERROR)
            {
                pop_dialog(g_focus, "ERR_DIALOG_NETWORK_DISCONNECT");
                quit_page(_, _, 0);
                return 0;
            }
            pop_dialog(g_focus, "ERR_DIALOG_ED_SYSTEM");
            quit_page(_, _, 0);
            return 0;
        }
    }
    g_service_status = edsClientGetInt(MEMBER_INFO_SERVICE_STATUS);
    ret = edsClientGetStr(MEMBER_INFO_MESSAGE, g_text, 12288);
    set_text_value("mess_infomation", g_text);
    return 1;
}

/*
 * Logs in with g_psbb_id and g_password and reads the registered data, where the new values start from; the card page
 * gets the years of the expiry from the date of the server. Returns 1 when done.
 */
fetch_member_data()
{
    new ret;
    edsClientSetStr(EDS_FIELD_AUTH_SYSTEM_ID, "001");
    edsClientSetStr(EDS_FIELD_AUTH_ID, g_psbb_id);
    edsClientSetStr(EDS_FIELD_AUTH_PASSWD, g_password);
    ret = run_eds_request(EDS_REQ_GET_MEMBER, "PAGE_SERVER_CONNECT", g_focus, g_arrows, 1);
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
        quit_page(_, _, 0);
    }
    ret = edsClientGetInt(MEMBER_GET_STATUS);
    if (0 != ret)
    {
        pop_dialog(g_focus, "ERR_DIALOG_AUTHNG");
        set_next_state(STATE_CERTIFICATE);
        g_page_slide = SLIDE_IN_LEFT;
        return 0;
    }
    ret = edsClientGetStr(MEMBER_COMMON_INQ_ID, g_inquiry_id, 64);
    ret = edsClientGetStr(MEMBER_FAMILY_NAME_KANA, g_family_name, 10);
    ret = edsClientGetStr(MEMBER_GIVEN_NAME_KANA, g_given_name, 10);
    ret = edsClientGetStr(MEMBER_ZIP_CODE, g_zipcode, 2);
    ret = edsClientGetStr(MEMBER_MAIL_ADDRESS, g_mail_addr, 17);
    g_mail_hope = edsClientGetInt(MEMBER_MAIL_REQUEST_FLAG);
    ret = edsClientGetStr(MEMBER_BIRTH_YMD, g_birthday, 3);
    g_sex = edsClientGetInt(MEMBER_SEX);
    ret = edsClientGetStr(MEMBER_CARD_NO, g_card_number, 5);
    copy_string(g_new_mail_addr, g_mail_addr, 17);
    copy_string(g_new_mail_addr_again, g_mail_addr, 17);
    copy_string(g_new_password, g_password, 8);
    copy_string(g_new_password_again, g_password, 8);
    copy_string(g_new_zipcode, g_zipcode, 2);
    copy_string(g_new_card_number, g_card_number, 5);
    copy_string(g_security_code, "", 2);
    split_date(g_birthday, g_birth_year, g_birth_month, g_birth_day, 2, 1, 1);
    {
        new now_date[64];
        new node;
        ret = edsClientGetStr(EDS_SERVER_DATE, now_date, 64);
        new min_year[8];
        new max_year[8];
        get_card_period_range(now_date, g_card_period, min_year, max_year);
        node = find_child_tag2(domRoot(), "page", "id", "PAGE_CARD");
        node = find_child_tag2(node, "item", "id", "item1");
        node = find_child_tag2(node, "param", "type", "year");
        domRemoveData(node, "min");
        domRemoveData(node, "max");
        domSetData(node, "min", min_year);
        domSetData(node, "max", max_year);
    }
    return 1;
}

/* Has the server check the new card; then the security code page if the card needs one, else the confirm page. */
check_card()
{
    new ret;
    close_ed_page(g_page, g_focus, SLIDE_OUT_LEFT);
    g_page_slide = SLIDE_IN_RIGHT;
    edsClientSetStr(EDS_FIELD_ORDER_NO, "");
    edsClientSetStr(EDS_FIELD_CARD_NO, g_new_card_number);
    edsClientSetStr(EDS_FIELD_CARD_VALID_YM, g_card_period);
    ret = run_eds_request(EDS_REQ_CHECK_CARD, "PAGE_SERVER_CONNECT", g_focus, g_arrows, 0);
    if (ret < 0)
    {
        if (ret == EDS_NETWORK_ERROR)
        {
            pop_dialog(g_focus, "ERR_DIALOG_NETWORK_DISCONNECT");
        }
        else
        {
            pop_dialog(g_focus, "ERR_DIALOG_ED_SYSTEM");
        }
        quit_page(_, _, 0);
    }
    ret = edsClientGetInt(CHECK_CARD_CHECK_STATUS);
    switch (ret)
    {
        case CHECK_STATUS_OK:
        {
        }
        case CHECK_STATUS_CARD_NG:
        {
            pop_dialog(g_focus, "ERR_DIALOG_CARD_NG");
            set_next_state(STATE_CARD);
            g_page_slide = SLIDE_IN_LEFT;
            return;
        }
        default:
        {
            printf("@@@@ Error : [SCE_CC_STATUS] Unknown status : [%d]\n", ret);
        }
    }
    g_need_security_code = edsClientGetInt(CHECK_CARD_CARD_SECURITY_FLAG);
    ret = edsClientGetInt(CHECK_CARD_BIRTH_MD_FLAG);
    if (g_need_security_code == 1)
    {
        set_next_state(STATE_SECURITY_CODE);
    }
    else
    {
        set_next_state(STATE_CONFIRM);
    }
}

/*
 * Sends the changes; then the complete page, or the page of the value refused. A mail address refused by the check of
 * its domain can be sent again without the check.
 */
send_changes()
{
    new ret;
    new card_number[5];
    new period[2];
    card_number = "";
    period = "";
    switch (g_card_change)
    {
        case CARD_KEEP:
        {
        }
        case CARD_NEW:
        {
            copy_string(card_number, g_new_card_number, 5);
            copy_string(period, g_card_period, 2);
        }
        case CARD_DELETE:
        {
            card_number = "0000000000000000";
        }
        default:
        {
        }
    }
    copy_string(g_sent_mail_addr, g_new_mail_addr, 17);
    edsClientSetStr(EDS_FIELD_AUTH_SYSTEM_ID, "001");
    edsClientSetStr(EDS_FIELD_AUTH_ID, g_psbb_id);
    edsClientSetStr(EDS_FIELD_AUTH_PASSWD, g_password);
    edsClientSetStr(EDS_FIELD_NEW_AUTH_PASSWD, g_new_password);
    edsClientSetStr(EDS_FIELD_NEW_ZIP_CODE, g_new_zipcode);
    edsClientSetStr(EDS_FIELD_NEW_MAIL_ADDRESS, g_new_mail_addr);
    edsClientSetInt(EDS_FIELD_NEW_MAIL_REQUEST_FLAG, g_new_mail_hope);
    edsClientSetStr(EDS_FIELD_NEW_CARD_NO, card_number);
    edsClientSetStr(EDS_FIELD_NEW_CARD_VALID_YM, period);
    edsClientSetStr(EDS_FIELD_SECURITY_CODE, g_security_code);
    edsClientSetInt(EDS_FIELD_DOMAIN_CHECK_FLAG, g_check_mail_domain);
    ret = run_eds_request(EDS_REQ_CHANGE_MEMBER, "PAGE_SERVER_CONNECT", g_focus, g_arrows, 0);
    if (ret < 0)
    {
        if (ret == EDS_NETWORK_ERROR)
        {
            pop_dialog(g_focus, "ERR_DIALOG_NETWORK_DISCONNECT");
        }
        else
        {
            pop_dialog(g_focus, "ERR_DIALOG_ED_SYSTEM");
        }
        quit_page(_, _, 0);
    }
    ret = edsClientGetInt(CHANGE_CHANGE_STATUS);
    if (0 != ret)
    {
        g_retry_count++;
        if (g_retry_count > 5)
        {
            pop_dialog(g_focus, "ERR_DIALOG_RETRY_OVER");
            quit_page(_, _, 0);
        }
    }
    switch (ret)
    {
        case CHANGE_STATUS_OK:
        {
            set_next_state(STATE_COMPLETE);
        }
        case CHANGE_STATUS_DOMAIN_CHECK:
        {
            g_text = "";
            append_string(g_text, "\n", 12288);
            append_string(g_text, g_new_mail_addr, 12288);
            new choice = pop_dialog(g_focus, "ERR_DIALOG_DOMAINCHECK", g_text);
            if (0 == choice)
            {
                g_check_mail_domain = 0;
                send_changes();
            }
            else
            {
                set_next_state(STATE_MAIL_ADDR);
                g_page_slide = SLIDE_IN_LEFT;
            }
        }
        case CHANGE_STATUS_AUTH_PASSWORD:
        {
            pop_dialog(g_focus, "ERR_DIALOG_AUTHPASSWORD");
            set_next_state(STATE_PASSWORD);
            g_page_slide = SLIDE_IN_LEFT;
        }
        case CHANGE_STATUS_MAIL_ADDRESS:
        {
            pop_dialog(g_focus, "ERR_DIALOG_MAILADDRESS");
            set_next_state(STATE_MAIL_ADDR);
            g_page_slide = SLIDE_IN_LEFT;
        }
        case CHANGE_STATUS_NAME_KANA:
        {
            pop_dialog(g_focus, "ERR_DIALOG_NAMEKANA");
            quit_page(_, _, 0);
        }
        case CHANGE_STATUS_BIRTH_YMD:
        {
            pop_dialog(g_focus, "ERR_DIALOG_BIRTHYMD");
            quit_page(_, _, 0);
        }
        case CHANGE_STATUS_CARD_NO:
        {
            pop_dialog(g_focus, "ERR_DIALOG_CARDNO");
            set_next_state(STATE_CARD);
            g_page_slide = SLIDE_IN_LEFT;
        }
        case CHANGE_STATUS_CARD_VALID_YM:
        {
            pop_dialog(g_focus, "ERR_DIALOG_CARDVALIDYM");
            set_next_state(STATE_CARD);
            g_page_slide = SLIDE_IN_LEFT;
        }
        case CHANGE_STATUS_SYSTEM_NG:
        {
            pop_dialog(g_focus, "ERR_DIALOG_SYSTEMNG");
            quit_page(_, _, 0);
        }
        case CHANGE_STATUS_CARD_INFO:
        {
            pop_dialog(g_focus, "ERR_DIALOG_CARDINFO");
            set_next_state(STATE_CARD);
            g_page_slide = SLIDE_IN_LEFT;
        }
        case CHANGE_STATUS_ZIP_NG:
        {
            pop_dialog(g_focus, "ERR_DIALOG_ZIPNG");
            set_next_state(STATE_ZIP);
            g_page_slide = SLIDE_IN_LEFT;
        }
        case CHANGE_STATUS_SECURITY_NG:
        {
            pop_dialog(g_focus, "ERR_DIALOG_SECURITYNG");
            set_next_state(STATE_SECURITY_CODE);
            g_page_slide = SLIDE_IN_LEFT;
        }
        case CHANGE_STATUS_DELETE_CARD_FAIL:
        {
            pop_dialog(g_focus, "ERR_DIALOG_DELETE_CARD_FAIL");
            set_next_state(STATE_CARD_MENU);
            g_page_slide = SLIDE_IN_LEFT;
        }
        default:
        {
            printf("@@@@ Error : [SCE_CM_STATUS] Unknown status : [%d]\n", ret);
        }
    }
}

open_information_page()
{
    g_text = "";
    g_page = open_ed_page("PAGE_INFOMATION", g_focus, g_page_slide, g_text);
    update_page_arrows(g_arrows, 0, 1, 1, 1);
}

/* The messages of the ed_page: right goes on to the login unless the service was refused, cross leaves. */
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
                    quit_page(_, _, 0);
                }
                if (0 != g_service_status)
                {
                    pop_dialog(g_focus, "ERR_DIALOG_SERVICE_NG");
                    quit_page(_, _, 0);
                }
                set_next_state(STATE_CERTIFICATE);
                g_page_slide = SLIDE_IN_RIGHT;
            }
            else if (a == TAG("LEFT"))
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
                play_se(ED_SE_CANCEL);
                quit_page(_, _, 0);
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
    update_page_arrows(g_arrows, 1, 1, g_input_valid, 1);
}

/*
 * The messages of the ed_page: "PARM" "psbbid" or "password" and STR give the new value, right logs in when both are
 * valid, left goes back, cross offers to leave.
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
                update_page_arrows(g_arrows, 1, 1, g_input_valid, 1);
            }
            else if (0 == strcmp("password", name))
            {
                wait_message_str(TAG("STR"), g_password, 8);
                copy_string(g_new_password, g_password, 8);
                copy_string(g_new_password_again, g_password, 8);
                g_input_valid = is_id_input_valid();
                update_page_arrows(g_arrows, 1, 1, g_input_valid, 1);
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
                    new ok = fetch_member_data();
                    if (ok == 1)
                    {
                        set_next_state(STATE_CONTENTS);
                        g_page_slide = SLIDE_IN_RIGHT;
                    }
                    else
                    {
                    }
                }
            }
            else if (a == TAG("LEFT"))
            {
                set_next_state(STATE_INFORMATION);
                close_ed_page(g_page, g_focus, SLIDE_OUT_RIGHT);
                g_page_slide = SLIDE_IN_LEFT;
            }
        }
        else if (msg == TAG("PUSH"))
        {
            if (b == TAG("RRGT"))
            {
            }
            else if (b == TAG("RDWN"))
            {
                ask_abort(1);
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

/* Opens the registered data of the member, the password as asterisks. */
open_contents_page()
{
    new i;
    new buf[64];
    new text[64];
    g_text = "";
    get_tag_attr("TEXT", "msged_member_entry_tablelist_pdbbid", "value", buf, 64);
    sprintf(buf, buf, g_psbb_id);
    sprintf(buf, "psbbid=%s;", buf);
    append_string(g_text, buf, 12288);
    {
        new head[8];
        new tail[8];
        split_inquiry_id(g_inquiry_id, head, tail, 8, 8);
        get_tag_attr("TEXT", "msged_member_entry_tablelist_inquiry", "value", buf, 64);
        sprintf(buf, buf, head, tail);
        sprintf(buf, "inqid=%s;", buf);
        append_string(g_text, buf, 12288);
    }
    get_tag_attr("TEXT", "msged_member_entry_tablelist_username", "value", buf, 64);
    sprintf(buf, buf, g_family_name, g_given_name);
    sprintf(buf, "name=%s;", buf);
    append_string(g_text, buf, 12288);
    get_tag_attr("TEXT", "msged_member_entry_tablelist_birthday", "value", buf, 64);
    sprintf(buf, buf, atoi(g_birth_year), atoi(g_birth_month), atoi(g_birth_day));
    sprintf(buf, "birthday=%s;", buf);
    append_string(g_text, buf, 12288);
    if (0 == g_sex)
    {
        get_tag_attr("TEXT", "msged_member_entry_detail_male", "value", text, 64);
    }
    else
    {
        get_tag_attr("TEXT", "msged_member_entry_detail_female", "value", text, 64);
    }
    get_tag_attr("TEXT", "msged_member_entry_tablelist_sex", "value", buf, 64);
    sprintf(buf, buf, text);
    sprintf(buf, "sex=%s;", buf);
    append_string(g_text, buf, 12288);
    get_tag_attr("TEXT", "msged_member_entry_tablelist_password", "value", buf, 64);
    i = 0;
    for (; g_password{i}; i++)
    {
        text{i} = '*';
    }
    text{i} = 0;
    sprintf(buf, buf, text);
    sprintf(buf, "password=%s;", buf);
    append_string(g_text, buf, 12288);
    get_tag_attr("TEXT", "msged_member_entry_tablelist_mailaddress", "value", buf, 64);
    sprintf(buf, buf, g_mail_addr);
    sprintf(buf, "mailaddr=%s;", buf);
    append_string(g_text, buf, 12288);
    get_tag_attr("TEXT", "msged_member_entry_tablelist_maildelivery", "value", buf, 64);
    if (0 == g_mail_hope)
    {
        get_tag_attr("TEXT", "msged_member_entry_maildelivery_norequest", "value", text, 64);
    }
    else
    {
        get_tag_attr("TEXT", "msged_member_entry_maildelivery_request", "value", text, 64);
    }
    sprintf(buf, buf, text);
    sprintf(buf, "mailhope=%s;", buf);
    append_string(g_text, buf, 12288);
    get_tag_attr("TEXT", "msged_member_entry_tablelist_postalcode", "value", buf, 64);
    {
        new head[8];
        new tail[8];
        split_zip_code(g_zipcode, head, tail, 8, 8);
        sprintf(buf, buf, head, tail);
    }
    sprintf(buf, "zip=%s;", buf);
    append_string(g_text, buf, 12288);
    if (is_not_empty3(g_card_number) == 1)
    {
        get_tag_attr("TEXT", "msged_member_entry_tablelist_creditcardno", "value", buf, 64);
        sprintf(buf, buf, g_card_number);
        sprintf(buf, "cardno=%s;", buf);
        append_string(g_text, buf, 12288);
    }
    g_page = open_ed_page("PAGE_CONTENTS", g_focus, g_page_slide, g_text);
    update_page_arrows(g_arrows, 1, 1, 1, 1);
}

update_contents_page()
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
                set_next_state(STATE_CHANGE_START);
                close_ed_page(g_page, g_focus, SLIDE_OUT_LEFT);
                g_page_slide = SLIDE_IN_RIGHT;
            }
            else if (a == TAG("LEFT"))
            {
                set_next_state(STATE_CERTIFICATE);
                close_ed_page(g_page, g_focus, SLIDE_OUT_RIGHT);
                g_page_slide = SLIDE_IN_LEFT;
            }
        }
        else if (msg == TAG("PUSH"))
        {
            if (b == TAG("RRGT"))
            {
            }
            else if (b == TAG("RDWN"))
            {
                ask_abort(1);
            }
        }
        if (IsValidObject(g_page) == 1)
        {
            SendMessage(g_page, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        }
    }
}

open_change_start_page()
{
    g_text = "";
    g_page = open_ed_page("PAGE_CHANGE_START", g_focus, g_page_slide, g_text);
    update_page_arrows(g_arrows, 1, 0, 1, 1);
}

/*
 * The messages of the ed_page: "PARM" "item0" and VAL give the item chosen; circle on the first one starts the changes.
 */
update_change_start_page()
{
    new msg;
    new a;
    new b;
    new c;
    new name[64];
    new choice = -1;
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
                set_next_state(STATE_CONTENTS);
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
                if (0 == choice)
                {
                    set_next_state(STATE_PASSWORD);
                    close_ed_page(g_page, g_focus, SLIDE_OUT_NONE);
                }
            }
            else if (b == TAG("RDWN"))
            {
                ask_abort(1);
            }
            if (IsValidObject(g_page) == 1)
            {
                SendMessage(g_page, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
            }
        }
        else if (msg == TAG("PARM"))
        {
            wait_message_str(TAG("ID"), name, 64);
            if (0 == strcmp("item0", name))
            {
                wait_message(TAG("VAL"), a);
                choice = a;
            }
        }
    }
}

open_password_page()
{
    sprintf(g_text, "password=%s;repassword=%s;", g_new_password, g_new_password_again);
    g_page = open_ed_page("PAGE_PASSWORD", g_focus, g_page_slide, g_text);
    g_password_typed = is_password_typed();
    update_page_arrows(g_arrows, 0, 1, g_password_typed, 1);
}

/*
 * The messages of the ed_page: "PARM" "password" or "repassword" and STR give the new password twice; right goes on
 * when they agree and are valid.
 */
update_password_page()
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
            if (0 == strcmp("password", name))
            {
                wait_message_str(TAG("STR"), g_new_password, 8);
                g_password_typed = is_password_typed();
                update_page_arrows(g_arrows, 0, 1, g_password_typed, 1);
            }
            else if (0 == strcmp("repassword", name))
            {
                wait_message_str(TAG("STR"), g_new_password_again, 8);
                g_password_typed = is_password_typed();
                update_page_arrows(g_arrows, 0, 1, g_password_typed, 1);
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
                if (g_password_typed == 1)
                {
                    if (0 != strcmp(g_new_password, g_new_password_again) || 0 == is_valid_password(g_new_password)
                            || 0 == is_valid_password(g_new_password_again))
                    {
                        pop_dialog(g_focus, "ERR_DIALOG_PASSWORD_DISAGREE");
                    }
                    else
                    {
                        set_next_state(STATE_MAIL_ADDR);
                        close_ed_page(g_page, g_focus, SLIDE_OUT_LEFT);
                        g_page_slide = SLIDE_IN_RIGHT;
                    }
                }
            }
            else if (a == TAG("LEFT"))
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
                ask_abort(1);
            }
        }
        if (IsValidObject(g_page) == 1)
        {
            SendMessage(g_page, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        }
    }
}

/* Returns 1 when the new password is typed twice. */
is_password_typed()
{
    if (0 == is_not_empty2(g_new_password))
    {
        return 0;
    }
    if (0 == is_not_empty2(g_new_password_again))
    {
        return 0;
    }
    return 1;
}

open_mail_addr_page()
{
    sprintf(g_text, "mailaddr=%s;remailaddr=%s;", g_new_mail_addr, g_new_mail_addr_again);
    g_page = open_ed_page("PAGE_MAILADDR", g_focus, g_page_slide, g_text);
    g_mail_addr_typed = is_mail_addr_typed();
    update_page_arrows(g_arrows, 1, 1, g_mail_addr_typed, 1);
}

/*
 * The messages of the ed_page: "PARM" "mailaddr" or "remailaddr" and STR give the new address twice; right goes on when
 * they agree and are valid.
 */
update_mail_addr_page()
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
            if (0 == strcmp("mailaddr", name))
            {
                wait_message_str(TAG("STR"), g_new_mail_addr, 17);
                g_mail_addr_typed = is_mail_addr_typed();
                update_page_arrows(g_arrows, 1, 1, g_mail_addr_typed, 1);
            }
            else if (0 == strcmp("remailaddr", name))
            {
                wait_message_str(TAG("STR"), g_new_mail_addr_again, 17);
                g_mail_addr_typed = is_mail_addr_typed();
                update_page_arrows(g_arrows, 1, 1, g_mail_addr_typed, 1);
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
                if (g_mail_addr_typed == 1)
                {
                    if (0 != strcmp(g_new_mail_addr, g_new_mail_addr_again)
                            || 0 == is_valid_mail_address(g_new_mail_addr)
                            || 0 == is_valid_mail_address(g_new_mail_addr_again))
                    {
                        pop_dialog(g_focus, "ERR_DIALOG_MAILADDR_DISAGREE");
                    }
                    else
                    {
                        set_next_state(STATE_MAIL_SEND);
                        close_ed_page(g_page, g_focus, SLIDE_OUT_LEFT);
                        g_page_slide = SLIDE_IN_RIGHT;
                    }
                }
            }
            else if (a == TAG("LEFT"))
            {
                set_next_state(STATE_PASSWORD);
                close_ed_page(g_page, g_focus, SLIDE_OUT_RIGHT);
                g_page_slide = SLIDE_IN_LEFT;
            }
        }
        else if (msg == TAG("PUSH"))
        {
            if (b == TAG("RRGT"))
            {
            }
            else if (b == TAG("RDWN"))
            {
                ask_abort(1);
            }
        }
        if (IsValidObject(g_page) == 1)
        {
            SendMessage(g_page, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        }
    }
}

/* Returns 1 when the new mail address is typed twice. */
is_mail_addr_typed()
{
    if (0 == is_not_empty(g_new_mail_addr))
    {
        return 0;
    }
    if (0 == is_not_empty(g_new_mail_addr_again))
    {
        return 0;
    }
    return 1;
}

open_mail_send_page()
{
    g_new_mail_hope = g_mail_hope;
    sprintf(g_text, "mailhope=%d", g_new_mail_hope);
    g_page = open_ed_page("PAGE_MAILSEND", g_focus, g_page_slide, g_text);
    update_page_arrows(g_arrows, 1, 1, 1, 1);
}

/* The messages of the ed_page: "PARM" "mailhope" and VAL give the choice. */
update_mail_send_page()
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
            if (0 == strcmp("mailhope", name))
            {
                wait_message(TAG("VAL"), a);
                g_new_mail_hope = a;
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
                set_next_state(STATE_ZIP);
                close_ed_page(g_page, g_focus, SLIDE_OUT_LEFT);
                g_page_slide = SLIDE_IN_RIGHT;
            }
            else if (a == TAG("LEFT"))
            {
                set_next_state(STATE_MAIL_ADDR);
                close_ed_page(g_page, g_focus, SLIDE_OUT_RIGHT);
                g_page_slide = SLIDE_IN_LEFT;
            }
        }
        else if (msg == TAG("PUSH"))
        {
            if (b == TAG("RRGT"))
            {
            }
            else if (b == TAG("RDWN"))
            {
                ask_abort(1);
            }
        }
        if (IsValidObject(g_page) == 1)
        {
            SendMessage(g_page, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        }
    }
}

open_zip_page()
{
    sprintf(g_text, "zip=%s;", g_new_zipcode);
    g_page = open_ed_page("PAGE_ZIP", g_focus, g_page_slide, g_text);
    update_page_arrows(g_arrows, 1, 1, 1, 1);
}

/* The messages of the ed_page: "PARM" "zip" and STR give the new zip code. */
update_zip_page()
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
            if (0 == strcmp("zip", name))
            {
                wait_message_str(TAG("STR"), g_new_zipcode, 2);
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
                set_next_state(STATE_CARD_MENU);
                close_ed_page(g_page, g_focus, SLIDE_OUT_LEFT);
                g_page_slide = SLIDE_IN_RIGHT;
            }
            else if (a == TAG("LEFT"))
            {
                set_next_state(STATE_MAIL_SEND);
                close_ed_page(g_page, g_focus, SLIDE_OUT_RIGHT);
                g_page_slide = SLIDE_IN_LEFT;
            }
        }
        else if (msg == TAG("PUSH"))
        {
            if (b == TAG("RRGT"))
            {
            }
            else if (b == TAG("RDWN"))
            {
                ask_abort(1);
            }
        }
        if (IsValidObject(g_page) == 1)
        {
            SendMessage(g_page, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        }
    }
}

/*
 * Builds the card menu (the "menu" element select_change) with the focus on the choice made before and opens its page.
 */
open_card_menu_page()
{
    /* In change.xml the text of ..._change is "do not change", that of ..._nochange "change (register a new one)". */
    new text_ids[3][] = {"msged_info_change_usrinformation_creditcardinfo_change",
            "msged_info_change_usrinformation_creditcardinfo_nochange",
            "msged_info_change_usrinformation_creditcardinfo_delete"};
    if (0 != g_card_menu)
    {
        domRemoveTag(g_card_menu);
        g_card_menu = 0;
    }
    g_card_menu = domAddTag(domRoot(), "menu");
    domSetData(g_card_menu, "id", "select_change");
    for (new i; i < 3; i++)
    {
        new item = domAddTag(g_card_menu, "item");
        sprintf(g_text, "%d", i);
        domSetData(item, "id", g_text);
        get_tag_attr("TEXT", text_ids[i], "value", g_text, 12288);
        domSetData(item, "value", g_text);
    }
    new node;
    node = find_child_tag2(domRoot(), "page", "id", "PAGE_CHANGE_CARD");
    node = find_child_tag2(node, "item", "id", "item0");
    domRemoveData(node, "current");
    sprintf(g_text, "%d", g_card_change);
    domSetData(node, "current", g_text);
    g_text = "";
    g_page = open_ed_page("PAGE_CHANGE_CARD", g_focus, g_page_slide, g_text);
    update_page_arrows(g_arrows, 1, 1, 1, 1);
}

/* The messages of the ed_page: "PARM" "item0" and VAL give the choice, right or circle applies it. */
update_card_menu_page()
{
    new msg;
    new a;
    new b;
    new c;
    new name[64];
    /* "FOCS": a the direction; "PUSH": b the button ("RRGT" circle, "RDWN" cross); "PARM": "ID" and "VAL" follow. */
    while (RecvMessage(msg, a, b, c))
    {
        if (msg == TAG("FOCS"))
        {
            SendMessage(g_page, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
            if (a == TAG("RIGT"))
            {
                play_se(ED_SE_DECIDE);
                select_card_change(g_card_change);
            }
            else if (a == TAG("LEFT"))
            {
                set_next_state(STATE_ZIP);
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
                select_card_change(g_card_change);
            }
            else if (b == TAG("RDWN"))
            {
                ask_abort(1);
            }
            if (IsValidObject(g_page) == 1)
            {
                SendMessage(g_page, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
            }
        }
        else if (msg == TAG("PARM"))
        {
            wait_message_str(TAG("ID"), name, 64);
            if (0 == strcmp("item0", name))
            {
                wait_message(TAG("VAL"), a);
                g_card_change = a;
            }
        }
    }
}

/*
 * Applies the choice of the card menu: keep the card (the confirm page), type a new one (the card page) or, after a
 * confirmation, delete it (the confirm page).
 */
select_card_change(choice)
{
    switch (choice)
    {
        case CARD_KEEP:
        {
            copy_string(g_new_card_number, g_card_number, 5);
            set_next_state(STATE_CONFIRM);
            close_ed_page(g_page, g_focus, SLIDE_OUT_LEFT);
            g_page_slide = SLIDE_IN_RIGHT;
        }
        case CARD_NEW:
        {
            copy_string(g_new_card_number, "", 5);
            set_next_state(STATE_CARD);
            close_ed_page(g_page, g_focus, SLIDE_OUT_LEFT);
            g_page_slide = SLIDE_IN_RIGHT;
        }
        case CARD_DELETE:
        {
            new answer = pop_dialog(g_focus, "DIALOG_DELETE_CARD_INFO_CONFIRM");
            if (0 == answer)
            {
                copy_string(g_new_card_number, "", 5);
                set_next_state(STATE_CONFIRM);
                close_ed_page(g_page, g_focus, SLIDE_OUT_LEFT);
                g_page_slide = SLIDE_IN_RIGHT;
            }
            else
            {
            }
        }
        default:
        {
        }
    }
}

open_card_page()
{
    sprintf(g_text, "cardno=%s;period=%s;", g_new_card_number, g_card_period);
    g_page = open_ed_page("PAGE_CARD", g_focus, g_page_slide, g_text);
    g_card_valid = is_valid_card_number(g_new_card_number);
    update_page_arrows(g_arrows, 1, 1, g_card_valid, 1);
}

/*
 * The messages of the ed_page: "PARM" "cardno" or "period" and STR give the new card, right checks it when the number
 * is valid.
 */
update_card_page()
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
            if (0 == strcmp("cardno", name))
            {
                wait_message_str(TAG("STR"), g_new_card_number, 5);
                g_card_valid = is_valid_card_number(g_new_card_number);
                update_page_arrows(g_arrows, 1, 1, g_card_valid, 1);
            }
            else if (0 == strcmp("period", name))
            {
                wait_message_str(TAG("STR"), g_card_period, 2);
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
                if (g_card_valid == 1)
                {
                    check_card();
                }
            }
            else if (a == TAG("LEFT"))
            {
                set_next_state(STATE_ZIP);
                close_ed_page(g_page, g_focus, SLIDE_OUT_RIGHT);
                g_page_slide = SLIDE_IN_LEFT;
            }
        }
        else if (msg == TAG("PUSH"))
        {
            if (b == TAG("RRGT"))
            {
            }
            else if (b == TAG("RDWN"))
            {
                ask_abort(1);
            }
        }
        if (IsValidObject(g_page) == 1)
        {
            SendMessage(g_page, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        }
    }
}

open_security_code_page()
{
    sprintf(g_text, "securitycode=%s;", g_security_code);
    g_page = open_ed_page("PAGE_SECURITYCODE", g_focus, g_page_slide, g_text);
    update_page_arrows(g_arrows, 1, 1, g_security_code_valid, 1);
}

/* The messages of the ed_page: "PARM" "securitycode" and STR give the code, right goes on when it is valid. */
update_security_code_page()
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
            if (0 == strcmp("securitycode", name))
            {
                wait_message_str(TAG("STR"), g_security_code, 2);
                g_security_code_valid = is_valid_security_code(g_security_code);
                update_page_arrows(g_arrows, 1, 1, g_security_code_valid, 1);
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
                if (g_security_code_valid == 1)
                {
                    set_next_state(STATE_CONFIRM);
                    close_ed_page(g_page, g_focus, SLIDE_OUT_LEFT);
                    g_page_slide = SLIDE_IN_RIGHT;
                }
            }
            else if (a == TAG("LEFT"))
            {
                set_next_state(STATE_CARD);
                close_ed_page(g_page, g_focus, SLIDE_OUT_RIGHT);
                g_page_slide = SLIDE_IN_LEFT;
            }
        }
        else if (msg == TAG("PUSH"))
        {
            if (b == TAG("RRGT"))
            {
            }
            else if (b == TAG("RDWN"))
            {
                ask_abort(1);
            }
        }
        if (IsValidObject(g_page) == 1)
        {
            SendMessage(g_page, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        }
    }
}

/* Opens the registered data with the changes; a new password is shown as it is, an old one as asterisks. */
open_confirm_page()
{
    new i;
    new buf[64];
    new text[64];
    get_tag_attr("TEXT", "msged_member_entry_tablelist_pdbbid", "value", buf, 64);
    sprintf(buf, buf, g_psbb_id);
    sprintf(buf, "psbbid=%s;", buf);
    append_string(g_text, buf, 12288);
    {
        new head[8];
        new tail[8];
        split_inquiry_id(g_inquiry_id, head, tail, 8, 8);
        get_tag_attr("TEXT", "msged_member_entry_tablelist_inquiry", "value", buf, 64);
        sprintf(buf, buf, head, tail);
        sprintf(buf, "inqid=%s;", buf);
        append_string(g_text, buf, 12288);
    }
    get_tag_attr("TEXT", "msged_member_entry_tablelist_username", "value", buf, 64);
    sprintf(buf, buf, g_family_name, g_given_name);
    sprintf(buf, "name=%s;", buf);
    append_string(g_text, buf, 12288);
    get_tag_attr("TEXT", "msged_member_entry_tablelist_birthday", "value", buf, 64);
    sprintf(buf, buf, atoi(g_birth_year), atoi(g_birth_month), atoi(g_birth_day));
    sprintf(buf, "birthday=%s;", buf);
    append_string(g_text, buf, 12288);
    if (0 == g_sex)
    {
        get_tag_attr("TEXT", "msged_member_entry_detail_male", "value", text, 64);
    }
    else
    {
        get_tag_attr("TEXT", "msged_member_entry_detail_female", "value", text, 64);
    }
    get_tag_attr("TEXT", "msged_member_entry_tablelist_sex", "value", buf, 64);
    sprintf(buf, buf, text);
    sprintf(buf, "sex=%s;", buf);
    append_string(g_text, buf, 12288);
    get_tag_attr("TEXT", "msged_member_entry_tablelist_password", "value", buf, 64);
    if (0 == strcmp(g_password, g_new_password))
    {
        i = 0;
        for (; g_new_password{i}; i++)
        {
            text{i} = '*';
        }
        text{i} = 0;
    }
    else
    {
        copy_string(text, g_new_password, 64);
    }
    sprintf(buf, buf, text);
    sprintf(buf, "password=%s;", buf);
    append_string(g_text, buf, 12288);
    get_tag_attr("TEXT", "msged_member_entry_tablelist_mailaddress", "value", buf, 64);
    sprintf(buf, buf, g_new_mail_addr);
    sprintf(buf, "mailaddr=%s;", buf);
    append_string(g_text, buf, 12288);
    get_tag_attr("TEXT", "msged_member_entry_tablelist_maildelivery", "value", buf, 64);
    if (0 == g_new_mail_hope)
    {
        get_tag_attr("TEXT", "msged_member_entry_maildelivery_norequest", "value", text, 64);
    }
    else
    {
        get_tag_attr("TEXT", "msged_member_entry_maildelivery_request", "value", text, 64);
    }
    sprintf(buf, buf, text);
    sprintf(buf, "mailhope=%s;", buf);
    append_string(g_text, buf, 12288);
    {
        new head[8];
        new tail[8];
        split_zip_code(g_new_zipcode, head, tail, 8, 8);
        get_tag_attr("TEXT", "msged_member_entry_tablelist_postalcode", "value", buf, 64);
        sprintf(buf, buf, head, tail);
        sprintf(buf, "zip=%s;", buf);
        append_string(g_text, buf, 12288);
    }
    if (is_not_empty3(g_new_card_number) == 1 && CARD_DELETE != g_card_change)
    {
        get_tag_attr("TEXT", "msged_member_entry_tablelist_creditcardno", "value", buf, 64);
        sprintf(buf, buf, g_new_card_number);
        sprintf(buf, "cardno=%s;", buf);
        append_string(g_text, buf, 12288);
        if (g_card_change == CARD_NEW)
        {
            new year[2];
            new month[1];
            split_short_year_month(g_card_period, year, month, 2, 1);
            get_tag_attr("TEXT", "msged_member_entry_tablelist_creditcardno_period", "value", buf, 64);
            sprintf(buf, buf, atoi(month), atoi(year));
            sprintf(buf, "period=%s;", buf);
            append_string(g_text, buf, 12288);
        }
    }
    g_page = open_ed_page("PAGE_CONFIRM", g_focus, g_page_slide, g_text);
    update_page_arrows(g_arrows, 1, 1, 1, 1);
}

/* The messages of the ed_page: right goes on, left goes back to the last page of the card. */
update_confirm_page()
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
                g_page_slide = SLIDE_IN_RIGHT;
                set_next_state(STATE_SEND_CONFIRM);
            }
            else if (a == TAG("LEFT"))
            {
                switch (g_card_change)
                {
                    case CARD_KEEP:
                    {
                        set_next_state(STATE_CARD_MENU);
                    }
                    case CARD_NEW:
                    {
                        if (is_not_empty3(g_new_card_number) == 1 && g_need_security_code == 1)
                        {
                            set_next_state(STATE_SECURITY_CODE);
                        }
                        else
                        {
                            set_next_state(STATE_CARD);
                        }
                    }
                    case CARD_DELETE:
                    {
                        set_next_state(STATE_CARD_MENU);
                    }
                    default:
                    {
                    }
                }
                close_ed_page(g_page, g_focus, SLIDE_OUT_RIGHT);
                g_page_slide = SLIDE_IN_LEFT;
            }
        }
        else if (msg == TAG("PUSH"))
        {
            if (b == TAG("RRGT"))
            {
            }
            else if (b == TAG("RDWN"))
            {
                ask_abort(1);
            }
        }
        if (IsValidObject(g_page) == 1)
        {
            SendMessage(g_page, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        }
    }
}

open_send_confirm_page()
{
    g_text = "";
    g_page = open_ed_page("PAGE_SEND_CONFIRM", g_focus, g_page_slide, g_text);
    update_page_arrows(g_arrows, 1, 0, 1, 1);
}

/*
 * The messages of the ed_page: "PARM" "item0" and VAL give the item chosen; circle on the first one sends the changes
 * after a confirmation.
 */
update_send_confirm_page()
{
    new msg;
    new a;
    new b;
    new c;
    new name[64];
    new choice = -1;
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
                close_ed_page(g_page, g_focus, SLIDE_OUT_RIGHT);
                g_page_slide = SLIDE_IN_LEFT;
                set_next_state(STATE_CONFIRM);
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
                if (0 == choice)
                {
                    new answer = pop_dialog(g_focus, "DIALOG_SEND_CONFIRM");
                    if (0 == answer)
                    {
                        close_ed_page(g_page, g_focus, SLIDE_OUT_NONE);
                        if (0 == g_check_mail_domain && 0 != strcmp(g_new_mail_addr, g_sent_mail_addr))
                        {
                            g_check_mail_domain = 1;
                        }
                        send_changes();
                    }
                    else
                    {
                        ask_abort(0);
                    }
                }
            }
            else if (b == TAG("RDWN"))
            {
                ask_abort(1);
            }
            if (IsValidObject(g_page) == 1)
            {
                SendMessage(g_page, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
            }
        }
        else if (msg == TAG("PARM"))
        {
            wait_message_str(TAG("ID"), name, 64);
            if (0 == strcmp("item0", name))
            {
                wait_message(TAG("VAL"), a);
                choice = a;
            }
        }
    }
}

open_complete_page()
{
    g_text = "";
    g_page = open_ed_page("PAGE_COMPLETE", g_focus, g_page_slide, g_text);
    update_page_arrows(g_arrows, 0, 0, 1, 1);
    play_se(ED_SE_COMPLETE);
}

/* The messages of the ed_page: cross finishes. */
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
            else if (a == TAG("LEFT"))
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
                finish_change();
            }
        }
        if (IsValidObject(g_page) == 1)
        {
            SendMessage(g_page, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        }
    }
}

/* Leaves; after a change of the password, offers first to save the new one with the ID (the page of LINK_ID_CHANGE). */
finish_change()
{
    new choice;
    new args[64];
    if (0 == strcmp(g_password, g_new_password))
    {
        quit_page(_, _, 0);
    }
    else
    {
        choice = pop_dialog(g_focus, "DIALOG_SAVE_CONFIRM");
        if (0 == choice)
        {
            append_member_args(args, 64);
            append_string(args, "psbbid=", 64);
            append_string(args, g_psbb_id, 64);
            append_string(args, ";", 64);
            append_string(args, "password=", 64);
            append_string(args, g_new_password, 64);
            append_string(args, ";", 64);
            quit_page("LINK_ID_CHANGE", args, 1);
        }
        else
        {
            ask_abort(0);
        }
    }
}
