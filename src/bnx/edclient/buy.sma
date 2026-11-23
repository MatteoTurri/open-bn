/*
 * /opt0/bn/script/edclient/buy.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The page that buys a paid service for a game (arguments GameID and GamePasswd): the information, the PSBB ID
 * and password, the service and its charge from the lists of the server, the agreement, then the payment with
 * the registered card, a card typed or WebMoney scratch numbers, and the receipt.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "edpage.inc"

/* Values of g_state: the page shown. */
const STATE_BUY_INFORMATION = 0;
const STATE_SERVICE_INFORMATION = 1;
const STATE_CERTIFICATE = 2;
const STATE_EVERYTIME_EXPLAIN = 3;
const STATE_REGULAR_EXPLAIN = 4;
const STATE_SELECT_SERVICE = 5;
const STATE_SERVICE_DETAIL = 6;
const STATE_AGREEMENT = 7;
const STATE_SELECT_CHARGE = 8;
const STATE_SELECT_PAYMENT = 9;
const STATE_CONTENTS = 10;
const STATE_CONFIRM = 11;
const STATE_WM_CONTENTS = 12;
const STATE_WM_CONFIRM = 13;
const STATE_SCRATCH_NO = 14;
const STATE_ADD_SCRATCH_NO = 15;
const STATE_REGIST_NOTICE = 16;
const STATE_CARD = 17;
const STATE_CARD_REINPUT = 18;
const STATE_SECURITY_CODE = 19;
const STATE_CODE_REINPUT = 20;
const STATE_COMPLETE = 21;
const STATE_NONE = 22; /* before the first page */

/* Values of g_payment: the ids of the payments of a charge on the server. */
const PAYMENT_REGISTERED_CARD = 0; /* the card registered for the member */
const PAYMENT_CARD = 1;            /* a card typed for this purchase */
const PAYMENT_PSBB_ID = 2;         /* never offered: build_charge_menu leaves it out */
const PAYMENT_WEBMONEY = 3;        /* WebMoney scratch numbers */

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
new g_default_period[] = "200201"; /* the first period of the card, YYYYMM */

#include "edsreq.inc"

#include "validate.inc"

#include "pfs.inc"

#include "card.inc"

/* The halves of an inquiry ID too short to split. */
new g_no_inquiry_head[1];
new g_no_inquiry_tail[1];

/* Returns 1 for 1 to 16 letters, digits or @ $ # - *, without I, O and l. */
is_valid_scratch_no(scratch_no[])
{
    new i;
    new c;
    new len;
    len = strlen(scratch_no);
    if (!(len > 0 && len <= 16))
    {
        return 0;
    }
    i = 0;
    for (; scratch_no{i}; i++)
    {
        c = scratch_no{i};
        if (c == 'I' || c == 'O' || c == 'l')
        {
            return 0;
        }
        if (c == '@' || c == '$' || c == '#' || c == '-' || c == '*')
        {
            continue;
        }
        if (c >= '0' && c <= '9')
        {
            continue;
        }
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
        {
            continue;
        }
        return 0;
    }
    return 1;
}

#include "date.inc"

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

/* The game without the arguments. */
new g_default_game_id[1];
new g_default_game_passwd[1];

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

/* Reads the argument GamePasswd, or the empty g_default_game_passwd without it. */
get_game_passwd(game_passwd[], maxlen)
{
    new found;
    found = GetArg(g_arg_game_passwd, game_passwd, maxlen);
    if (0 == found)
    {
        copy_string(game_passwd, g_default_game_passwd, maxlen);
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
new g_card_number[5];
new g_card_period[2]; /* YYYYMM */
new g_scratch_no[64];
new g_security_code[2];
/* The service, charge and payment chosen. */
new g_service_id[64];
new g_service_name[64];
new g_charge_id[64];
new g_charge_kind = -1; /* 0: the card typed can be registered on request, 1: it is registered */
new g_charge_name[64]; /* the text of the charge in its menu */
new g_price; /* with the tax */
new g_payment = -1; /* PAYMENT_* */
/* From the server with the order: the receipt number and the end of the service. */
new g_order_id[64];
new g_expire_date[64]; /* YYYYMMDD, "9999..." without an end */
new g_wm_session_id[64]; /* the WebMoney session of the payment waiting for more scratch numbers */
/* From the server with the payment. */
new g_inquiry_id[64];
new g_ticket[64]; /* read, never shown */
new g_card_regist_status; /* 0: the member has a registered card, which the payment page offers */
new g_need_security_code; /* the card needs its security code */
/* The item chosen in each menu, whether the menu has items, and whether one was chosen with circle. */
new g_service_cursor;
new g_has_services;
new g_service_chosen;
new g_charge_cursor;
new g_has_charges;
new g_charge_chosen;
new g_payment_cursor;
new g_has_payments;
new g_payment_chosen;
/* The "menu" elements of the services, the charges and the payments. */
new g_service_menu;
new g_charge_menu;
new g_payment_menu;
new g_retry_count; /* the card payments refused: more than 5 leave the page */
new g_card_typed; /* the card of the payment was typed: offer_card_regist follows */
/* The input of the pages is well formed: the right arrow is lit. */
new g_input_valid;
new g_card_valid;
new g_security_code_valid;
new g_scratch_no_valid;
new g_game_id[64];
new g_game_passwd[64];
new g_info_error; /* the error of the information request, shown when the page is left */
new g_group_status; /* not 0: the GameID and GamePasswd were refused */
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

/*
 * Unless the argument unloadBGP is 1, loads the background; reads the game from the arguments and fetches the
 * information before the first page.
 */
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
        printf("@@@@ Error : client init fail.\n");
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
    get_game_passwd(g_game_passwd, 64);
    copy_string(g_card_period, g_default_period, 2);
    create_title("msged_buy_titlemenu_buy");
    fetch_information();
    g_next_state = STATE_BUY_INFORMATION;
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
        case STATE_BUY_INFORMATION:
        {
            open_buy_information_page();
        }
        case STATE_SERVICE_INFORMATION:
        {
            open_service_information_page();
        }
        case STATE_CERTIFICATE:
        {
            open_certificate_page();
        }
        case STATE_EVERYTIME_EXPLAIN:
        {
            open_everytime_explain_page();
        }
        case STATE_REGULAR_EXPLAIN:
        {
            open_regular_explain_page();
        }
        case STATE_SELECT_SERVICE:
        {
            open_select_service_page();
        }
        case STATE_SERVICE_DETAIL:
        {
            open_service_detail_page();
        }
        case STATE_AGREEMENT:
        {
            open_agreement_page();
        }
        case STATE_SELECT_CHARGE:
        {
            open_select_charge_page();
        }
        case STATE_SELECT_PAYMENT:
        {
            open_select_payment_page();
        }
        case STATE_CONTENTS:
        {
            open_contents_page();
        }
        case STATE_CONFIRM:
        {
            open_confirm_page();
        }
        case STATE_WM_CONTENTS:
        {
            open_wm_contents_page();
        }
        case STATE_WM_CONFIRM:
        {
            open_wm_confirm_page();
        }
        case STATE_SCRATCH_NO:
        {
            open_scratch_no_page();
        }
        case STATE_ADD_SCRATCH_NO:
        {
            open_add_scratch_no_page();
        }
        case STATE_REGIST_NOTICE:
        {
            open_regist_notice_page();
        }
        case STATE_CARD:
        {
            open_card_page();
        }
        case STATE_CARD_REINPUT:
        {
            open_card_reinput_page();
        }
        case STATE_SECURITY_CODE:
        {
            open_security_code_page();
        }
        case STATE_CODE_REINPUT:
        {
            open_code_reinput_page();
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
        case STATE_BUY_INFORMATION:
        {
            update_buy_information_page();
        }
        case STATE_SERVICE_INFORMATION:
        {
            update_service_information_page();
        }
        case STATE_CERTIFICATE:
        {
            update_certificate_page();
        }
        case STATE_EVERYTIME_EXPLAIN:
        {
            update_everytime_explain_page();
        }
        case STATE_REGULAR_EXPLAIN:
        {
            update_regular_explain_page();
        }
        case STATE_SELECT_SERVICE:
        {
            update_select_service_page();
        }
        case STATE_SERVICE_DETAIL:
        {
            update_service_detail_page();
        }
        case STATE_AGREEMENT:
        {
            update_agreement_page();
        }
        case STATE_SELECT_CHARGE:
        {
            update_select_charge_page();
        }
        case STATE_SELECT_PAYMENT:
        {
            update_select_payment_page();
        }
        case STATE_CONTENTS:
        {
            update_contents_page();
        }
        case STATE_CONFIRM:
        {
            update_confirm_page();
        }
        case STATE_WM_CONTENTS:
        {
            update_wm_contents_page();
        }
        case STATE_WM_CONFIRM:
        {
            update_wm_confirm_page();
        }
        case STATE_SCRATCH_NO:
        {
            update_scratch_no_page();
        }
        case STATE_ADD_SCRATCH_NO:
        {
            update_add_scratch_no_page();
        }
        case STATE_REGIST_NOTICE:
        {
            update_regist_notice_page();
        }
        case STATE_CARD:
        {
            update_card_page();
        }
        case STATE_CARD_REINPUT:
        {
            update_card_reinput_page();
        }
        case STATE_SECURITY_CODE:
        {
            update_security_code_page();
        }
        case STATE_CODE_REINPUT:
        {
            update_code_reinput_page();
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
        quit_page();
    }
    else
    {
    }
}

/* Writes into args the service, the charge, the payment, the price and the end of the service. */
build_contents_args(args[], maxlen)
{
    new line[64];
    new value[64];
    new year_text[8];
    new month_text[8];
    new day_text[8];
    copy_string(args, "", maxlen);
    get_tag_attr("TEXT", "msged_buy_contents_service_name", "value", line, 64);
    sprintf(line, line, g_service_name);
    sprintf(line, "name=%s;", line);
    append_string(args, line, maxlen);
    get_tag_attr("TEXT", "msged_buy_contents_service", "value", line, 64);
    sprintf(line, line, g_charge_name);
    sprintf(line, "service=%s;", line);
    append_string(args, line, maxlen);
    switch (g_payment)
    {
        case PAYMENT_REGISTERED_CARD:
        {
            get_tag_attr("TEXT", "msged_buy_payment_select_creditcard_registration", "value", value, 64);
        }
        case PAYMENT_CARD:
        {
            get_tag_attr("TEXT", "msged_buy_payment_select_creditcard", "value", value, 64);
        }
        case PAYMENT_PSBB_ID:
        {
        }
        case PAYMENT_WEBMONEY:
        {
            get_tag_attr("TEXT", "msged_buy_payment_select_webmoney", "value", value, 64);
        }
        default:
        {
        }
    }
    get_tag_attr("TEXT", "msged_buy_contents_payment", "value", line, 64);
    sprintf(line, line, value);
    sprintf(line, "payment=%s;", line);
    append_string(args, line, maxlen);
    get_tag_attr("TEXT", "msged_buy_contents_charge", "value", line, 64);
    sprintf(value, "%d", g_price);
    sprintf(line, line, value);
    sprintf(line, "price=%s;", line);
    append_string(args, line, maxlen);
    if (0 == strncmp(g_expire_date, "9999", 4))
    {
        get_tag_attr("TEXT", "msged_servicelist_endless_period", "value", value, 64);
    }
    else
    {
        get_tag_attr("TEXT", "msged_servicelist_number_data", "value", line, 64);
        split_date(g_expire_date, year_text, month_text, day_text, 8, 8, 8);
        sprintf(value, line, atoi(year_text), atoi(month_text), atoi(day_text));
    }
    get_tag_attr("TEXT", "msged_buy_contents_expire", "value", line, 64);
    sprintf(line, line, value);
    sprintf(line, "expire=%s;", line);
    append_string(args, line, maxlen);
}

/* Gets the information of the purchase and of the service into their TEXT elements; on an error, leaves. */
fetch_information()
{
    new ret;
    edsClientSetStr(EDS_FIELD_GROUP_ID, g_game_id);
    edsClientSetStr(EDS_FIELD_GROUP_PASSWD, g_game_passwd);
    ret = run_eds_request(EDS_REQ_GET_GROUP_INFO, "PAGE_SERVER_CONNECT", g_focus, g_arrows, 1);
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
    g_group_status = edsClientGetInt(GROUP_INFO_GROUP_AUTH_STATUS);
    g_service_status = edsClientGetInt(GROUP_INFO_SERVICE_STATUS);
    ret = edsClientGetStr(GROUP_INFO_MESSAGE, g_text, 12288);
    set_text_value("mess_buy_infomation", g_text);
    ret = edsClientGetStr(GROUP_INFO_SG_MESSAGE, g_text, 12288);
    set_text_value("mess_service_infomation", g_text);
    return 1;
}

/*
 * Logs the PSBB ID in and gets the services of the game into g_service_menu; returns 1, or 0 back on the certificate
 * page when the ID is refused or the request canceled; leaves on the other errors.
 */
fetch_services()
{
    new ret;
    edsClientSetStr(EDS_FIELD_GROUP_ID, g_game_id);
    edsClientSetStr(EDS_FIELD_GROUP_PASSWD, g_game_passwd);
    edsClientSetStr(EDS_FIELD_AUTH_SYSTEM_ID, "001");
    edsClientSetStr(EDS_FIELD_AUTH_ID, g_psbb_id);
    edsClientSetStr(EDS_FIELD_AUTH_PASSWD, g_password);
    ret = run_eds_request(EDS_REQ_AUTH_GROUP, "PAGE_SERVER_CONNECT", g_focus, g_arrows, 1);
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
    ret = edsClientGetInt(AUTH_GROUP_GROUP_AUTH_STATUS);
    if (0 != ret)
    {
        pop_dialog(g_focus, "ERR_DIALOG_GROUP_AUTHNG");
        quit_page();
        return 0;
    }
    ret = edsClientGetInt(AUTH_GROUP_PURCHASE_STATUS);
    if (0 != ret)
    {
        pop_dialog(g_focus, "ERR_DIALOG_BUY_NG");
        quit_page();
        return 0;
    }
    ret = edsClientGetInt(AUTH_GROUP_AUTH_STATUS);
    if (0 != ret)
    {
        pop_dialog(g_focus, "ERR_DIALOG_AUTHNG");
        set_next_state(STATE_CERTIFICATE);
        g_page_slide = SLIDE_IN_LEFT;
        return 0;
    }
    ret = edsClientGetInt(AUTH_GROUP_COUNT);
    if (ret <= 0)
    {
        pop_dialog(g_focus, "ERR_DIALOG_NO_SERVICE");
        quit_page();
        return 0;
    }
    new listed = build_service_menu();
    g_has_services = listed > 0 ? 1 : 0;
    if (0 == listed)
    {
        pop_dialog(g_focus, "ERR_DIALOG_NOLIST");
        quit_page();
        return 0;
    }
    return 1;
}

/*
 * Builds the menu element select_service from the services of the answer: a service bought is marked, one under another
 * needs that one bought, one bought without an end is shown disabled; returns their number.
 */
build_service_menu()
{
    new i;
    new listed;
    new count;
    new status;
    new id[32];
    new parent[32];
    new buf[64];
    new end_date[64];
    new item;
    new icon;
    new endless;
    new is_child;
    new len;
    count = edsClientGetInt(AUTH_GROUP_COUNT);
    if (0 != g_service_menu)
    {
        domRemoveTag(g_service_menu);
        g_service_menu = 0;
    }
    g_service_menu = domAddTag(domRoot(), "menu");
    domSetData(g_service_menu, "id", "select_service");
    i = 0;
    for (; i < count; i++)
    {
        if (edsClientEnter(i) < 0)
        {
            continue;
        }
        status = edsClientGetInt(SERVICE_SERVICE_STATUS);
        if (status < 0 || status > SERVICE_STATUS_BOUGHT)
        {
            printf("@@@@ [SCE_SERVICE_STATUS] Unknow status : [%d]\n", status);
            edsClientLeave();
            continue;
        }
        if (status == SERVICE_STATUS_UNLISTED)
        {
            edsClientLeave();
            continue;
        }
        edsClientGetStr(SERVICE_END_DATE, end_date, 64);
        endless = 0;
        if (0 == strncmp(end_date, "9999", 4))
        {
            endless = 1;
        }
        edsClientGetStr(SERVICE_SERVICE_ID, id, 32);
        edsClientGetStr(SERVICE_PARENT_SERVICE_ID, parent, 32);
        is_child = 0;
        if (0 == strcmp(id, parent))
        {
        }
        else
        {
            is_child = 1;
        }
        if (SERVICE_STATUS_NEW == status && is_child == 1)
        {
            item = domFirstChild(g_service_menu, g_any_child2);
            for (; item; item = domNext(item, g_any_next2))
            {
                domGetTagName(item, buf, 64);
                if (0 != strcmp("item", buf))
                {
                    continue;
                }
                len = domGetData(item, "id", buf, 64);
                if (0 == len)
                {
                    continue;
                }
                if (0 != strcmp(parent, buf))
                {
                    continue;
                }
                len = domGetData(item, "parent", buf, 64);
                if (0 == len)
                {
                    continue;
                }
                if (0 != strcmp(parent, buf))
                {
                    continue;
                }
                break;
            }
            domGetData(item, "status", buf, 64);
            if (SERVICE_STATUS_BOUGHT != atoi(buf))
            {
                edsClientLeave();
                continue;
            }
        }
        if (status == SERVICE_STATUS_BOUGHT && endless == 1 && is_child == 1)
        {
            edsClientLeave();
            continue;
        }
        item = domAddTag(g_service_menu, "item");
        edsClientGetStr(SERVICE_SERVICE_ID, buf, 64);
        domSetData(item, "id", buf);
        if (status == SERVICE_STATUS_BOUGHT && endless == 1 && 0 == is_child)
        {
            domSetData(item, "enable", "false");
        }
        else
        {
            domSetData(item, "enable", "true");
        }
        g_text = "";
        append_string(g_text, "            ", 12288);
        if (is_child == 1)
        {
            append_string(g_text, "  ", 12288);
            get_tag_attr("TEXT", "msged_buy_service_select_dott", "value", buf, 64);
            append_string(g_text, buf, 12288);
        }
        edsClientGetStr(SERVICE_NAME, buf, 64);
        append_string(g_text, buf, 12288);
        domSetData(item, "value", g_text);
        domSetData(item, "name", buf);
        sprintf(buf, "%d", status);
        domSetData(item, "status", buf);
        edsClientGetStr(SERVICE_END_DATE, buf, 64);
        domSetData(item, "enddate", buf);
        edsClientGetStr(SERVICE_PARENT_SERVICE_ID, buf, 64);
        domSetData(item, "parent", buf);
        edsClientGetStr(SERVICE_TICKET, buf, 64);
        domSetData(item, "ticket", buf);
        domSetData(item, "pad_ru", "enable");
        if (SERVICE_STATUS_NEW == status)
        {
            domSetData(item, "pad_ru", "disable");
            icon = domAddTag(item, "plane");
            domSetData(icon, "x", "16");
            domSetData(icon, "img_id", "IMG_ICON_NEW");
        }
        else if (status == SERVICE_STATUS_BOUGHT && endless == 1 && 0 == is_child)
        {
            icon = domAddTag(item, "plane");
            domSetData(icon, "x", "48");
            domSetData(icon, "img_id", "IMG_ICON_ALREADY");
        }
        else if (status == SERVICE_STATUS_BOUGHT && 0 == endless)
        {
            icon = domAddTag(item, "plane");
            domSetData(icon, "x", "16");
            domSetData(icon, "img_id", "IMG_ICON_ADD");
            icon = domAddTag(item, "plane");
            domSetData(icon, "x", "48");
            domSetData(icon, "img_id", "IMG_ICON_ALREADY");
        }
        edsClientLeave();
        listed++;
    }
    return listed;
}

/*
 * Gets the agreement of the service and the date of the server, which sets the default card period and its years; on an
 * error, leaves.
 */
fetch_agreement()
{
    new ret;
    edsClientSetStr(EDS_FIELD_SERVICE_ID, g_service_id);
    edsClientSetStr(EDS_FIELD_GROUP_ID, g_game_id);
    edsClientSetStr(EDS_FIELD_GROUP_PASSWD, g_game_passwd);
    ret = run_eds_request(EDS_REQ_GET_SERVICE_INFO, "PAGE_SERVER_CONNECT", g_focus, g_arrows, 1);
    if (ret < 0)
    {
        if (ret == EDS_CANCELED)
        {
            set_next_state(STATE_SELECT_CHARGE);
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
    ret = edsClientGetInt(SERVICE_INFO_GROUP_AUTH_STATUS);
    if (0 != ret)
    {
        pop_dialog(g_focus, "ERR_DIALOG_GROUP_AUTHNG");
        quit_page();
        return 0;
    }
    ret = edsClientGetInt(SERVICE_INFO_SERVICE_STATUS);
    if (0 != ret)
    {
        pop_dialog(g_focus, "ERR_DIALOG_SERVICE_NG");
        quit_page();
        return 0;
    }
    edsClientGetStr(SERVICE_INFO_AGREEMENT, g_text, 12288);
    set_text_value("mess_agreement", g_text);
    {
        new date_text[64];
        new node;
        ret = edsClientGetStr(EDS_SERVER_DATE, date_text, 64);
        new min_year[8];
        new max_year[8];
        get_card_period_range(date_text, g_card_period, min_year, max_year);
        node = find_child_tag2(domRoot(), "page", "id", "PAGE_CARD");
        node = find_child_tag2(node, "item", "id", "item1");
        node = find_child_tag2(node, "param", "type", "year");
        domRemoveData(node, "min");
        domRemoveData(node, "max");
        domSetData(node, "min", min_year);
        domSetData(node, "max", max_year);
        node = find_child_tag2(domRoot(), "page", "id", "PAGE_CARD_REINPUT");
        node = find_child_tag2(node, "item", "id", "item1");
        node = find_child_tag2(node, "param", "type", "year");
        domRemoveData(node, "min");
        domRemoveData(node, "max");
        domSetData(node, "min", min_year);
        domSetData(node, "max", max_year);
    }
    return 1;
}

/* Gets the charges of the service g_service_id into g_charge_menu; on an error, leaves. */
fetch_charges()
{
    new ret;
    new buf[64];
    edsClientSetStr(EDS_FIELD_SERVICE_ID, g_service_id);
    edsClientSetStr(EDS_FIELD_AUTH_SYSTEM_ID, "001");
    edsClientSetStr(EDS_FIELD_AUTH_ID, g_psbb_id);
    edsClientSetStr(EDS_FIELD_GROUP_ID, g_game_id);
    edsClientSetStr(EDS_FIELD_GROUP_PASSWD, g_game_passwd);
    ret = run_eds_request(EDS_REQ_GET_SERVICE_SETTLEMENT, "PAGE_SERVER_CONNECT", g_focus, g_arrows, 1);
    if (ret < 0)
    {
        if (ret == EDS_CANCELED)
        {
            set_next_state(STATE_SELECT_SERVICE);
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
    ret = edsClientGetInt(SETTLEMENT_GROUP_AUTH_STATUS);
    if (0 != ret)
    {
        pop_dialog(g_focus, "ERR_DIALOG_GROUP_AUTHNG");
        quit_page();
        return 0;
    }
    edsClientGetStr(SETTLEMENT_CARD_VALID_START_YM, buf, 64);
    edsClientGetStr(SETTLEMENT_CARD_VALID_END_YM, buf, 64);
    g_card_regist_status = edsClientGetInt(SETTLEMENT_CARD_REGIST_STATUS);
    new listed = build_charge_menu();
    g_has_charges = listed > 0 ? 1 : 0;
    return 1;
}

/*
 * Builds the menu element select_charge, one item per charge with its price and its payments; returns their number, 0
 * after leaving when there is none or a charge without a payment.
 */
build_charge_menu()
{
    new i;
    new j;
    new payment;
    new count;
    new listed;
    new value;
    new buf[64];
    if (0 != g_charge_menu)
    {
        domRemoveTag(g_charge_menu);
        g_charge_menu = 0;
    }
    g_charge_menu = domAddTag(domRoot(), "menu");
    domSetData(g_charge_menu, "id", "select_charge");
    domSetData(g_charge_menu, "w", "320");
    count = edsClientGetInt(SETTLEMENT_COUNT);
    if (0 == count)
    {
        edsClientLeave();
        pop_dialog(g_focus, "ERR_DIALOG_NOSETTLEMENT");
        quit_page();
        return 0;
    }
    i = 0;
    for (; i < count; i++)
    {
        if (edsClientEnter(i) < 0)
        {
            continue;
        }
        new item;
        new node;
        item = domAddTag(g_charge_menu, "item");
        edsClientGetStr(CHARGE_SERVICE_SETTLEMENT_ID, buf, 64);
        domSetData(item, "id", buf);
        edsClientGetStr(CHARGE_EXPLANATION, buf, 64);
        domSetData(item, "value", buf);
        value = edsClientGetInt(CHARGE_SETTLEMENT_KIND);
        sprintf(buf, "%d", value);
        domSetData(item, "kind", buf);
        value = edsClientGetInt(CHARGE_END_TERM_SETUP);
        edsClientGetStr(CHARGE_END_TERM_START_YM, buf, 64);
        edsClientGetStr(CHARGE_END_TERM_END_YM, buf, 64);
        value = edsClientGetInt(CHARGE_INITIAL_FREE_TERM);
        value = edsClientGetInt(CHARGE_TERM_VALIDITY);
        value = edsClientGetInt(CHARGE_PURCHASE_UNIT);
        value = edsClientGetInt(CHARGE_PRICE);
        value = edsClientGetInt(CHARGE_TAX_AMOUNT);
        value = edsClientGetInt(CHARGE_TAX_PRICE);
        sprintf(buf, "%d", value);
        domSetData(item, "tax_price", buf);
        node = domAddTag(item, "text");
        domSetData(node, "x", "-160");
        domSetData(node, "align_h", "left");
        edsClientGetStr(CHARGE_EXPLANATION, buf, 64);
        domSetData(node, "value", buf);
        node = domAddTag(item, "text");
        domSetData(node, "x", "160");
        domSetData(node, "align_h", "right");
        value = edsClientGetInt(CHARGE_TAX_PRICE);
        sprintf(buf, "%d", value);
        get_tag_attr("TEXT", "msged_buy_period_charge_yen", "value", g_text, 12288);
        sprintf(g_text, g_text, buf);
        domSetData(node, "value", g_text);
        new payment_count = edsClientGetInt(CHARGE_PAYMENT_COUNT);
        new payments;
        j = 0;
        for (; j < payment_count; j++)
        {
            if (edsClientEnter(j) < 0)
            {
                continue;
            }
            payment = edsClientGetInt(PAYMENT_PAYMENT_ID);
            if (payment < 0)
            {
                edsClientLeave();
                continue;
            }
            if (payment == PAYMENT_PSBB_ID)
            {
                edsClientLeave();
                continue;
            }
            node = domAddTag(item, "payment");
            sprintf(buf, "%d", payment);
            domSetData(node, "id", buf);
            edsClientGetStr(PAYMENT_NAME, buf, 64);
            domSetData(node, "name", buf);
            payments++;
            edsClientLeave();
        }
        if (0 == payments)
        {
            edsClientLeave();
            pop_dialog(g_focus, "ERR_DIALOG_NEED_CREDITCARD");
            quit_page();
            return 0;
        }
        sprintf(buf, "%d", payments);
        domSetData(item, "payment_num", buf);
        listed++;
        edsClientLeave();
    }
    return listed;
}

/*
 * Orders the charge chosen with payment, and reads the answer of the server for that payment; returns 1 when the order
 * is accepted.
 */
send_order(payment)
{
    new ret;
    new ok;
    set_order_fields();
    ret = run_eds_request(EDS_REQ_ORDER_SERVICE, "PAGE_SERVER_CONNECT", g_focus, g_arrows, 1);
    if (ret < 0)
    {
        if (ret == EDS_CANCELED)
        {
            set_next_state(g_state);
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
    ret = edsClientGetInt(ORDER_GROUP_AUTH_STATUS);
    if (0 != ret)
    {
        pop_dialog(g_focus, "ERR_DIALOG_GROUP_AUTHNG");
        quit_page();
    }
    switch (payment)
    {
        case PAYMENT_REGISTERED_CARD, PAYMENT_CARD:
        {
            ok = read_card_order();
        }
        case PAYMENT_WEBMONEY:
        {
            ok = read_wm_order();
        }
        default:
        {
        }
    }
    return ok;
}

/* Sets the fields of the order: the member, the game, the charge, the payment and its card. */
set_order_fields()
{
    new buf[64];
    edsClientSetStr(EDS_FIELD_AUTH_SYSTEM_ID, "001");
    edsClientSetStr(EDS_FIELD_AUTH_ID, g_psbb_id);
    edsClientSetStr(EDS_FIELD_AUTH_PASSWD, g_password);
    edsClientSetStr(EDS_FIELD_SERVICE_SETTLEMENT_ID, g_charge_id);
    edsClientSetStr(EDS_FIELD_SETTLEMENT_END_TERM, "");
    edsClientSetStr(EDS_FIELD_STORE_ID, "");
    sprintf(buf, "%d", g_payment);
    edsClientSetStr(EDS_FIELD_PAYMENT_ID, buf);
    edsClientSetStr(EDS_FIELD_GROUP_ID, g_game_id);
    edsClientSetStr(EDS_FIELD_GROUP_PASSWD, g_game_passwd);
    switch (g_payment)
    {
        case PAYMENT_REGISTERED_CARD:
        {
            edsClientSetStr(EDS_FIELD_CARD_NO, "");
            edsClientSetStr(EDS_FIELD_CARD_VALID_YM, "");
            edsClientSetStr(EDS_FIELD_ACCOUNT_ID, "");
            edsClientSetStr(EDS_FIELD_ACCOUNT_PASSWD, "");
        }
        case PAYMENT_CARD:
        {
            edsClientSetStr(EDS_FIELD_CARD_NO, g_card_number);
            edsClientSetStr(EDS_FIELD_CARD_VALID_YM, g_card_period);
            edsClientSetStr(EDS_FIELD_ACCOUNT_ID, "");
            edsClientSetStr(EDS_FIELD_ACCOUNT_PASSWD, "");
        }
        case PAYMENT_PSBB_ID:
        {
            edsClientSetStr(EDS_FIELD_CARD_NO, "");
            edsClientSetStr(EDS_FIELD_CARD_VALID_YM, "");
            edsClientSetStr(EDS_FIELD_ACCOUNT_ID, g_psbb_id);
            edsClientSetStr(EDS_FIELD_ACCOUNT_PASSWD, g_password);
        }
        case PAYMENT_WEBMONEY:
        {
            edsClientSetStr(EDS_FIELD_CARD_NO, "");
            edsClientSetStr(EDS_FIELD_CARD_VALID_YM, "");
            edsClientSetStr(EDS_FIELD_ACCOUNT_ID, "");
            edsClientSetStr(EDS_FIELD_ACCOUNT_PASSWD, "");
        }
        default:
        {
            printf("@@@ Unknow payment id : [%d]\n", g_payment);
        }
    }
}

/*
 * Reads the answer of an order paid by card: refused, leaves or goes back to the card or the certificate page; else
 * gets the receipt number and goes on to the security code when the card needs it, or to the contents.
 */
read_card_order()
{
    new ret;
    ret = edsClientGetInt(ORDER_ORDER_STATUS);
    switch (ret)
    {
        case ORDER_STATUS_OK:
        {
        }
        case ORDER_STATUS_UNPURCHASABLE:
        {
            pop_dialog(g_focus, "ERR_DIALOG_ORDER_UNPURCHASABLE");
            quit_page();
            return 0;
        }
        case ORDER_STATUS_INVALID_CARD:
        {
            pop_dialog(g_focus, "ERR_DIALOG_ORDER_INVALIDCARD");
            set_next_state(STATE_CARD);
            g_page_slide = SLIDE_IN_LEFT;
            return 0;
        }
        case ORDER_STATUS_AUTH_ERROR:
        {
            pop_dialog(g_focus, "ERR_DIALOG_ORDER_AUTHERROR");
            set_next_state(STATE_CERTIFICATE);
            g_page_slide = SLIDE_IN_LEFT;
            return 0;
        }
        case ORDER_STATUS_PERIOD_NG:
        {
            pop_dialog(g_focus, "ERR_DIALOG_ORDER_PERIODNG");
            quit_page();
            return 0;
        }
        default:
        {
            printf("@@@ [SCE_OS_STATUS] Unknow status : [%d]\n,", ret);
        }
    }
    ret = edsClientGetStr(ORDER_END_DATE, g_expire_date, 64);
    ret = edsClientGetStr(ORDER_ORDER_NO, g_order_id, 64);
    g_need_security_code = edsClientGetInt(ORDER_CARD_SECURITY_FLAG);
    ret = edsClientGetInt(ORDER_BIRTH_MD_FLAG);
    if (g_need_security_code == 1)
    {
        set_next_state(STATE_SECURITY_CODE);
    }
    else
    {
        set_next_state(STATE_CONTENTS);
    }
    return 1;
}

/* Reads the answer of an order paid with WebMoney: refused, leaves; else gets the receipt number. */
read_wm_order()
{
    new ret;
    ret = edsClientGetInt(ORDER_ORDER_STATUS);
    switch (ret)
    {
        case ORDER_STATUS_OK:
        {
        }
        case ORDER_STATUS_UNPURCHASABLE:
        {
            pop_dialog(g_focus, "ERR_DIALOG_ORDER_UNPURCHASABLE");
            quit_page();
            return 0;
        }
        case ORDER_STATUS_INVALID_CARD:
        {
            pop_dialog(g_focus, "ERR_DIALOG_ORDER_INVALIDCARD");
            quit_page();
            return 0;
        }
        case ORDER_STATUS_AUTH_ERROR:
        {
            pop_dialog(g_focus, "ERR_DIALOG_ORDER_AUTHERROR");
            quit_page();
            return 0;
        }
        case ORDER_STATUS_PERIOD_NG:
        {
            pop_dialog(g_focus, "ERR_DIALOG_ORDER_PERIODNG");
            quit_page();
            return 0;
        }
        default:
        {
            printf("@@@ [SCE_OS_STATUS] Unknow status : [%d]\n", ret);
        }
    }
    ret = edsClientGetStr(ORDER_END_DATE, g_expire_date, 64);
    ret = edsClientGetStr(ORDER_ORDER_NO, g_order_id, 64);
    ret = edsClientGetInt(ORDER_CARD_SECURITY_FLAG);
    ret = edsClientGetInt(ORDER_BIRTH_MD_FLAG);
    return 1;
}

/*
 * Pays the order with the scratch number typed; returns 1 when paid. Without enough balance, or for another scratch
 * number, goes to the page to add one; the other errors leave.
 */
send_wm_payment()
{
    new ret;
    edsClientSetStr(EDS_FIELD_ORDER_NO, g_order_id);
    edsClientSetStr(EDS_FIELD_SCRATCH_NO, g_scratch_no);
    edsClientSetStr(EDS_FIELD_WM_SESSION_ID, g_wm_session_id);
    edsClientSetStr(EDS_FIELD_AUTH_SYSTEM_ID, "001");
    edsClientSetStr(EDS_FIELD_AUTH_ID, g_psbb_id);
    edsClientSetStr(EDS_FIELD_AUTH_PASSWD, g_password);
    ret = run_eds_request(EDS_REQ_WM_PURCHASE_SERVICE, "PAGE_SERVER_CONNECT", g_focus, g_arrows, 0);
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
        quit_page();
        return 0;
    }
    ret = edsClientGetInt(WM_PURCHASE_AUTH_STATUS);
    switch (ret)
    {
        case AUTH_STATUS_OK:
        {
        }
        case AUTH_STATUS_NG:
        {
            pop_dialog(g_focus, "ERR_DIALOG_AUTH_AUTHNG");
            quit_page();
            return 0;
        }
        case AUTH_STATUS_INVALID:
        {
            pop_dialog(g_focus, "ERR_DIALOG_AUTH_INVALID");
            quit_page();
            return 0;
        }
        case AUTH_STATUS_QUITTED:
        {
            pop_dialog(g_focus, "ERR_DIALOG_AUTH_QUITTED");
            quit_page();
            return 0;
        }
        case AUTH_STATUS_ETC_NG:
        {
            pop_dialog(g_focus, "ERR_DIALOG_AUTH_ETCNG");
            quit_page();
            return 0;
        }
        default:
        {
            printf("@@@ [SCE_WMPS_AUTHSTATUS] Unknow status : [%d]\n", ret);
        }
    }
    ret = edsClientGetInt(WM_PURCHASE_PURCHASE_STATUS);
    get_wm_error_text(ret, g_text, 12288);
    switch (ret)
    {
        case PURCHASE_STATUS_OK:
        {
        }
        case PURCHASE_STATUS_WM_SERVER_NG:
        {
            pop_dialog(g_focus, "ERR_DIALOG_WM_PURCHASE_SERVERNG", g_text);
            cancel_wm_payment();
            quit_page();
            return 0;
        }
        case PURCHASE_STATUS_WM_NG:
        {
            pop_dialog(g_focus, "ERR_DIALOG_WM_PURCHASE_NG", g_text);
            cancel_wm_payment();
            quit_page();
            return 0;
        }
        case PURCHASE_STATUS_WM_REENTER:
        {
            pop_dialog(g_focus, "ERR_DIALOG_WM_PURCHASE_REENTER", g_text);
            set_next_state(STATE_SCRATCH_NO);
            return 0;
        }
        case PURCHASE_STATUS_WM_APPEND_CARD:
        {
            pop_dialog(g_focus, "ERR_DIALOG_WM_PURCHASE_APPENDCARD", g_text);
            set_next_state(STATE_ADD_SCRATCH_NO);
            return 0;
        }
        case PURCHASE_STATUS_WM_NO_BALANCE:
        {
            pop_dialog(g_focus, "ERR_DIALOG_WM_PURCHASE_NOBALANCE", g_text);
            ret = edsClientGetStr(WM_PURCHASE_WM_SESSION_ID, g_wm_session_id, 64);
            set_next_state(STATE_ADD_SCRATCH_NO);
            return 0;
        }
        case PURCHASE_STATUS_WM_PURCHASED_NG:
        {
            pop_dialog(g_focus, "ERR_DIALOG_WM_PURCHASE_PURCHASEDNG", g_text);
            cancel_wm_payment();
            quit_page();
            return 0;
        }
        case PURCHASE_STATUS_WM_SYSTEM_NG:
        {
            pop_dialog(g_focus, "ERR_DIALOG_WM_PURCHASE_SYSTEMNG", g_text);
            cancel_wm_payment();
            quit_page();
            return 0;
        }
        case PURCHASE_STATUS_WM_CARD_OVER_NG:
        {
            pop_dialog(g_focus, "ERR_DIALOG_WM_PURCHASE_CARDOVERNG", g_text);
            cancel_wm_payment();
            quit_page();
            return 0;
        }
        default:
        {
            printf("@@@ [SCE_WMPS_STATUS] Unknow status : [%d]\n", ret);
        }
    }
    new buf[64];
    ret = edsClientGetStr(WM_PURCHASE_WM_ERROR_CODE, buf, 64);
    ret = edsClientGetStr(WM_PURCHASE_WM_MESSAGE, buf, 64);
    ret = edsClientGetStr(WM_PURCHASE_COMMON_INQ_ID, g_inquiry_id, 64);
    ret = edsClientGetStr(WM_PURCHASE_TICKET, g_ticket, 64);
    return 1;
}

/* Cancels the WebMoney payment waiting for more scratch numbers, if any. */
cancel_wm_payment()
{
    new ret;
    if (0 == strcmp("", g_wm_session_id))
    {
        return;
    }
    edsClientSetStr(EDS_FIELD_ORDER_NO, g_order_id);
    edsClientSetStr(EDS_FIELD_WM_SESSION_ID, g_wm_session_id);
    ret = run_eds_request(EDS_REQ_STOP_WEB_MONEY, "PAGE_SERVER_CONNECT", g_focus, g_arrows, 1);
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
        quit_page();
    }
}

/*
 * Writes into text the message of the server for the WebMoney status, with its code, or else the TEXT element for that
 * status.
 */
get_wm_error_text(status, text[], maxlen)
{
    new message[64];
    new code[16];
    new code_text[16];
    edsClientGetStr(WM_PURCHASE_WM_MESSAGE, message, 64);
    if (strlen(message) > 0)
    {
        edsClientGetStr(WM_PURCHASE_WM_ERROR_CODE, code, 16);
        copy_string(code_text, code, 16);
        sprintf(text, "%s\n(%08d)", message, atoi(code_text));
        return;
    }
    switch (status)
    {
        case PURCHASE_STATUS_OK:
        {
            return;
        }
        case PURCHASE_STATUS_WM_SERVER_NG:
        {
            message = "msged_error_noservice_webmoney";
        }
        case PURCHASE_STATUS_WM_NG:
        {
            message = "msged_error_system_w31";
        }
        case PURCHASE_STATUS_WM_REENTER:
        {
            message = "msged_error_notuse_this_scratchno";
        }
        case PURCHASE_STATUS_WM_APPEND_CARD:
        {
            message = "msged_error_notuse_available_scratchno";
        }
        case PURCHASE_STATUS_WM_NO_BALANCE:
        {
            message = "msged_error_nobalance_scratch";
        }
        case PURCHASE_STATUS_WM_PURCHASED_NG:
        {
            message = "msged_error_already_buy_service_w35";
        }
        case PURCHASE_STATUS_WM_SYSTEM_NG:
        {
            message = "msged_error_system_w36";
        }
        case PURCHASE_STATUS_WM_CARD_OVER_NG:
        {
            message = "msged_error_addmax_w37";
        }
        default:
        {
            printf("@@@ [SCE_WMPS_STATUS] Unknow status : [%d]\n", status);
        }
    }
    get_tag_attr("TEXT", message, "value", text, maxlen);
}

/*
 * Checks the card typed again with the server: refused, leaves; else goes on to the security code when the card needs
 * it, or pays.
 */
check_card()
{
    new ret;
    edsClientSetStr(EDS_FIELD_ORDER_NO, g_order_id);
    edsClientSetStr(EDS_FIELD_CARD_NO, g_card_number);
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
        quit_page();
        return 0;
    }
    ret = edsClientGetInt(CHECK_CARD_CHECK_STATUS);
    if (0 != ret)
    {
        pop_dialog(g_focus, "ERR_DIALOG_CARD_NG");
        quit_page();
        return 0;
    }
    g_need_security_code = edsClientGetInt(CHECK_CARD_CARD_SECURITY_FLAG);
    ret = edsClientGetInt(CHECK_CARD_BIRTH_MD_FLAG);
    if (g_need_security_code == 1)
    {
        set_next_state(STATE_CODE_REINPUT);
    }
    else
    {
        send_card_payment();
    }
    return 1;
}

/*
 * Pays the order by card and goes on to the complete page, offering to register a card typed. A refused card or
 * security code goes back to be typed again; more than 5 refusals leave.
 */
send_card_payment()
{
    new ret;
    edsClientSetStr(EDS_FIELD_ORDER_NO, g_order_id);
    edsClientSetStr(EDS_FIELD_SECURITY_CODE, g_security_code);
    edsClientSetStr(EDS_FIELD_BIRTH_MD, "");
    edsClientSetStr(EDS_FIELD_AUTH_SYSTEM_ID, "001");
    edsClientSetStr(EDS_FIELD_AUTH_ID, g_psbb_id);
    edsClientSetStr(EDS_FIELD_AUTH_PASSWD, g_password);
    ret = run_eds_request(EDS_REQ_PURCHASE_SERVICE, "PAGE_SERVER_CONNECT", g_focus, g_arrows, 0);
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
        quit_page();
        return;
    }
    ret = edsClientGetInt(PURCHASE_AUTH_STATUS);
    switch (ret)
    {
        case AUTH_STATUS_OK:
        {
        }
        case AUTH_STATUS_NG:
        {
            pop_dialog(g_focus, "ERR_DIALOG_AUTH_AUTHNG");
            quit_page();
            return;
        }
        case AUTH_STATUS_INVALID:
        {
            pop_dialog(g_focus, "ERR_DIALOG_AUTH_INVALID");
            quit_page();
            return;
        }
        case AUTH_STATUS_QUITTED:
        {
            pop_dialog(g_focus, "ERR_DIALOG_AUTH_QUITTED");
            quit_page();
            return;
        }
        case AUTH_STATUS_ETC_NG:
        {
            pop_dialog(g_focus, "ERR_DIALOG_AUTH_ETCNG");
            quit_page();
            return;
        }
        default:
        {
            printf("@@@ [SCE_PS_AUTHSTATUS] Unknow status : [%d]\n", ret);
        }
    }
    ret = edsClientGetInt(PURCHASE_PURCHASE_STATUS);
    if (0 != ret)
    {
        g_retry_count++;
        if (g_retry_count > 5)
        {
            pop_dialog(g_focus, "ERR_DIALOG_CALL_OFFICE");
            quit_page();
            return;
        }
    }
    switch (ret)
    {
        case PURCHASE_STATUS_OK:
        {
        }
        case PURCHASE_STATUS_FAULT:
        {
            new choice = pop_dialog(g_focus, "ERR_DIALOG_PURCHASE_FAULT");
            if (0 == choice)
            {
                send_card_payment();
            }
            else
            {
                quit_page();
            }
            return;
        }
        case PURCHASE_STATUS_CARD_NO_NG:
        {
            switch (g_payment)
            {
                case PAYMENT_REGISTERED_CARD:
                {
                    pop_dialog(g_focus, "ERR_DIALOG_PURCHASE_REGIST_CARDNONG");
                }
                case PAYMENT_CARD:
                {
                    pop_dialog(g_focus, "ERR_DIALOG_PURCHASE_CARDNONG");
                }
                default:
                {
                    printf("@@@ Unknow payment id : [%d]\n", g_payment);
                }
            }
            set_next_state(STATE_CARD_REINPUT);
            g_page_slide = SLIDE_IN_LEFT;
            return;
        }
        case PURCHASE_STATUS_CARD_TERM_NG:
        {
            switch (g_payment)
            {
                case PAYMENT_REGISTERED_CARD:
                {
                    pop_dialog(g_focus, "ERR_DIALOG_PURCHASE_REGIST_CARDTERMNG");
                }
                case PAYMENT_CARD:
                {
                    pop_dialog(g_focus, "ERR_DIALOG_PURCHASE_CARDTERMNG");
                }
                default:
                {
                    printf("@@@ Unknow payment id : [%d]\n", g_payment);
                }
            }
            set_next_state(STATE_CARD_REINPUT);
            g_page_slide = SLIDE_IN_LEFT;
            return;
        }
        case PURCHASE_STATUS_SECURITY_NG:
        {
            pop_dialog(g_focus, "ERR_DIALOG_PURCHASE_SECURITYNG");
            set_next_state(STATE_CODE_REINPUT);
            g_page_slide = SLIDE_IN_LEFT;
            return;
        }
        case PURCHASE_STATUS_BIRTH_MD_NG:
        {
            pop_dialog(g_focus, "ERR_DIALOG_PURCHASE_BIRTHMDNG");
            quit_page();
            return;
        }
        case PURCHASE_STATUS_SERVER_NG:
        {
            pop_dialog(g_focus, "ERR_DIALOG_PURCHASE_SERVERNG");
            quit_page();
            return;
        }
        case PURCHASE_STATUS_PURCHASED_NG:
        {
            pop_dialog(g_focus, "ERR_DIALOG_PURCHASE_PURCHASEDNG");
        }
        case PURCHASE_STATUS_SYSTEM_NG:
        {
            pop_dialog(g_focus, "ERR_DIALOG_PURCHASE_SYSTEMNG");
            quit_page();
            return;
        }
        default:
        {
            printf("@@@ [SCE_PS_STATUS] Unknow status : [%d]\n", ret);
        }
    }
    ret = edsClientGetStr(PURCHASE_COMMON_INQ_ID, g_inquiry_id, 64);
    ret = edsClientGetStr(PURCHASE_TICKET, g_ticket, 64);
    if (is_card_typed() == 1)
    {
        offer_card_regist();
    }
    set_next_state(STATE_COMPLETE);
}

/* Returns 1 when the card of the payment was typed. */
is_card_typed()
{
    if (g_card_typed == 1)
    {
        return 1;
    }
    return 0;
}

/*
 * After a payment with a card typed: for a charge of kind 0 asks with DIALOG_REGIST_CARD_CONFIRM whether to register
 * the card and does it; of kind 1 only tells it is registered.
 */
offer_card_regist()
{
    new choice;
    new ret;
    if (0 == g_charge_kind)
    {
        choice = pop_dialog(g_focus, "DIALOG_REGIST_CARD_CONFIRM");
        if (0 == choice)
        {
            edsClientSetStr(EDS_FIELD_AUTH_SYSTEM_ID, "001");
            edsClientSetStr(EDS_FIELD_AUTH_ID, g_psbb_id);
            edsClientSetStr(EDS_FIELD_AUTH_PASSWD, g_password);
            edsClientSetStr(EDS_FIELD_ORDER_NO, g_order_id);
            ret = run_eds_request(EDS_REQ_REGIST_CARD_INFO, "PAGE_SERVER_CONNECT", g_focus, g_arrows, 0);
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
                quit_page();
            }
            ret = edsClientGetInt(REGIST_CARD_REGIST_STATUS);
            if (0 == ret)
            {
                pop_dialog(g_focus, "DIALOG_REGIST_CARD");
            }
            else
            {
                pop_dialog(g_focus, "ERR_DIALOG_REGIST_FAIL");
            }
        }
        else
        {
        }
    }
    else if (g_charge_kind == 1)
    {
        pop_dialog(g_focus, "DIALOG_REGIST_CARD");
    }
}

open_buy_information_page()
{
    g_text = "";
    g_page = open_ed_page("PAGE_BUY_INFOMATION", g_focus, g_page_slide, g_text);
    update_page_arrows(g_arrows, 0, 1, 1, 1);
}

/*
 * The messages of the ed_page: right shows the error of the information if any and leaves, else goes on; cross leaves.
 */
update_buy_information_page()
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
                if (0 != g_group_status)
                {
                    pop_dialog(g_focus, "ERR_DIALOG_GROUP_AUTHNG");
                    quit_page();
                }
                set_next_state(STATE_SERVICE_INFORMATION);
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
                quit_page();
            }
        }
        if (IsValidObject(g_page) == 1)
        {
            SendMessage(g_page, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        }
    }
}

open_service_information_page()
{
    g_text = "";
    g_page = open_ed_page("PAGE_SERVICE_INFOMATION", g_focus, g_page_slide, g_text);
    update_page_arrows(g_arrows, 1, 1, 1, 1);
}

/* The messages of the ed_page: right goes on unless the service cannot be used, left goes back, cross asks to leave. */
update_service_information_page()
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
                if (0 != g_service_status)
                {
                    pop_dialog(g_focus, "ERR_DIALOG_SERVICE_NG");
                    quit_page();
                }
                set_next_state(STATE_CERTIFICATE);
                g_page_slide = SLIDE_IN_RIGHT;
            }
            else if (a == TAG("LEFT"))
            {
                close_ed_page(g_page, g_focus, SLIDE_OUT_RIGHT);
                g_page_slide = SLIDE_IN_LEFT;
                set_next_state(STATE_BUY_INFORMATION);
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

open_certificate_page()
{
    sprintf(g_text, "psbbid=%s;password=%s;", g_psbb_id, g_password);
    g_page = open_ed_page("PAGE_CERTIFICATE", g_focus, g_page_slide, g_text);
    g_show_prev = 1;
    g_show_next = 1;
    update_page_arrows(g_arrows, g_show_prev, g_show_next, g_input_valid, 1);
}

/*
 * The messages of the ed_page: the PSBB ID and the password typed ("PARM" and "STR"), right fetches the services and
 * goes on, left goes back, cross asks to leave.
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
                    new fetched = fetch_services();
                    if (fetched == 1)
                    {
                        set_next_state(STATE_EVERYTIME_EXPLAIN);
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
                set_next_state(STATE_SERVICE_INFORMATION);
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

open_everytime_explain_page()
{
    g_text = "";
    g_page = open_ed_page("PAGE_EVERYTIME_EXPLAIN", g_focus, g_page_slide, g_text);
    update_page_arrows(g_arrows, 1, 1, 1, 1);
}

/* The messages of the ed_page: right goes on, left goes back, cross asks to leave. */
update_everytime_explain_page()
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
                set_next_state(STATE_REGULAR_EXPLAIN);
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

open_regular_explain_page()
{
    g_text = "";
    g_page = open_ed_page("PAGE_REGULAR_EXPLAIN", g_focus, g_page_slide, g_text);
    update_page_arrows(g_arrows, 1, 1, 1, 1);
}

/* The messages of the ed_page: right goes on, left goes back, cross asks to leave. */
update_regular_explain_page()
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
                set_next_state(STATE_SELECT_SERVICE);
                close_ed_page(g_page, g_focus, SLIDE_OUT_LEFT);
                g_page_slide = SLIDE_IN_RIGHT;
            }
            else if (a == TAG("LEFT"))
            {
                set_next_state(STATE_EVERYTIME_EXPLAIN);
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

/* Opens the menu of the services with the cursor on the service chosen last. */
open_select_service_page()
{
    {
        new node;
        node = find_child_tag2(domRoot(), "page", "id", "PAGE_SELECT_SERVICE");
        node = find_child_tag2(node, "item", "id", "item0");
        domRemoveData(node, "current");
        sprintf(g_text, "%d", g_service_cursor);
        domSetData(node, "current", g_text);
    }
    g_text = "";
    g_page = open_ed_page("PAGE_SELECT_SERVICE", g_focus, g_page_slide, g_text);
    g_service_chosen = 0;
    update_page_arrows(g_arrows, 1, g_has_services, g_service_chosen, 1);
}

/*
 * The messages of the ed_page: circle on a service chooses it ("PUSH" with its index), triangle shows its details,
 * right fetches the charges of the service chosen and goes on, left goes back, cross asks to leave.
 */
update_select_service_page()
{
    new msg;
    new a;
    new b;
    new c;
    /* "FOCS": a the direction; "PUSH": a the row, b the button ("RRGT" circle, "RDWN" cross). */
    while (RecvMessage(msg, a, b, c))
    {
        SendMessage(g_page, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
        if (msg == TAG("FOCS"))
        {
            if (a == TAG("RIGT"))
            {
                if (g_has_services == 1 && g_service_chosen == 1)
                {
                    get_selected_service();
                    close_ed_page(g_page, g_focus, SLIDE_OUT_LEFT);
                    new fetched = fetch_charges();
                    if (fetched == 1)
                    {
                        set_next_state(STATE_SELECT_CHARGE);
                        g_page_slide = SLIDE_IN_RIGHT;
                    }
                    else
                    {
                    }
                }
            }
            else if (a == TAG("LEFT"))
            {
                set_next_state(STATE_REGULAR_EXPLAIN);
                close_ed_page(g_page, g_focus, SLIDE_OUT_RIGHT);
                g_page_slide = SLIDE_IN_LEFT;
            }
        }
        else if (msg == TAG("PUSH"))
        {
            if (b == TAG("RRGT"))
            {
                if (g_has_services == 1)
                {
                    g_service_cursor = a;
                    g_service_chosen = 1;
                    update_page_arrows(g_arrows, 1, g_has_services, g_service_chosen, 1);
                }
            }
            else if (b == TAG("RDWN"))
            {
                ask_abort(1);
            }
            else if (b == TAG("RUP"))
            {
                if (g_has_services == 1)
                {
                    g_service_cursor = a;
                    set_next_state(STATE_SERVICE_DETAIL);
                    close_ed_page(g_page, g_focus, SLIDE_OUT_NONE);
                }
            }
        }
        if (IsValidObject(g_page) == 1)
        {
            SendMessage(g_page, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        }
    }
}

/* Reads the id and the name of the service at g_service_cursor. */
get_selected_service()
{
    new item;
    item = find_nth_child_tag(g_service_menu, "item", g_service_cursor);
    domGetData(item, "id", g_service_id, 64);
    domGetData(item, "name", g_service_name, 64);
}

/* Opens the page with the name, the state and the end of the service at g_service_cursor. */
open_service_detail_page()
{
    new line[64];
    new value[64];
    new item;
    new endless;
    g_text = "";
    item = find_nth_child_tag(g_service_menu, "item", g_service_cursor);
    domGetData(item, "enddate", value, 64);
    endless = 0;
    if (0 == strncmp(value, "9999", 4))
    {
        endless = 1;
    }
    get_tag_attr("TEXT", "msged_buy_contents_service_name", "value", line, 64);
    domGetData(item, "name", value, 64);
    sprintf(line, line, value);
    sprintf(line, "name=%s;", line);
    append_string(g_text, line, 12288);
    value = "";
    if (endless == 1)
    {
        get_tag_attr("TEXT", "msged_buy_service_impossible", "value", value, 64);
    }
    else
    {
        get_tag_attr("TEXT", "msged_buy_service_addbuy", "value", value, 64);
    }
    get_tag_attr("TEXT", "msged_buy_service_buyinfo_status", "value", line, 64);
    sprintf(line, line, value);
    sprintf(line, "status=%s;", line);
    append_string(g_text, line, 12288);
    value = "";
    get_tag_attr("TEXT", "msged_buy_service_alreadybuy", "value", value, 64);
    get_tag_attr("TEXT", "msged_buy_service_buyinfo_presence", "value", line, 64);
    sprintf(line, line, value);
    sprintf(line, "presence=%s;", line);
    append_string(g_text, line, 12288);
    value = "";
    if (endless == 1)
    {
        get_tag_attr("TEXT", "msged_servicelist_endless_period", "value", value, 64);
    }
    else
    {
        new year_text[8];
        new month_text[8];
        new day_text[8];
        domGetData(item, "enddate", value, 64);
        split_date(value, year_text, month_text, day_text, 8, 8, 8);
        get_tag_attr("TEXT", "msged_servicelist_number_data", "value", line, 64);
        sprintf(value, line, atoi(year_text), atoi(month_text), atoi(day_text));
    }
    get_tag_attr("TEXT", "msged_buy_contents_expire", "value", line, 64);
    sprintf(line, line, value);
    sprintf(line, "expire=%s;", line);
    append_string(g_text, line, 12288);
    g_page = open_ed_page("PAGE_SERVICE_DETAIL", g_focus, g_page_slide, g_text);
    update_page_arrows(g_arrows, 0, 0, 1, 1);
}

/* The messages of the ed_page: cross goes back to the page of the services. */
update_service_detail_page()
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
                play_se(ED_SE_CANCEL);
                set_next_state(STATE_SELECT_SERVICE);
                close_ed_page(g_page, g_focus, SLIDE_OUT_NONE);
            }
        }
        if (IsValidObject(g_page) == 1)
        {
            SendMessage(g_page, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        }
    }
}

open_agreement_page()
{
    g_text = "";
    g_page = open_ed_page("PAGE_AGREEMENT", g_focus, g_page_slide, g_text);
    update_page_arrows(g_arrows, 0, 1, 1, 1);
}

/* The messages of the ed_page: right goes on once the member agrees, cross asks to leave. */
update_agreement_page()
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
                new choice = pop_dialog(g_focus, "DIALOG_AGREEMENT");
                if (0 == choice)
                {
                    close_ed_page(g_page, g_focus, SLIDE_OUT_LEFT);
                    g_page_slide = SLIDE_IN_RIGHT;
                    set_next_state(STATE_SELECT_PAYMENT);
                }
                else
                {
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

/* Opens the menu of the charges with the cursor on the charge chosen last. */
open_select_charge_page()
{
    {
        new node;
        node = find_child_tag2(domRoot(), "page", "id", "PAGE_SELECT_CHARGE");
        node = find_child_tag2(node, "item", "id", "item0");
        domRemoveData(node, "current");
        sprintf(g_text, "%d", g_charge_cursor);
        domSetData(node, "current", g_text);
    }
    g_text = "";
    g_page = open_ed_page("PAGE_SELECT_CHARGE", g_focus, g_page_slide, g_text);
    g_charge_chosen = 0;
    update_page_arrows(g_arrows, 1, g_has_charges, g_charge_chosen, 1);
}

/*
 * The messages of the ed_page: circle on a charge chooses it, right fetches the agreement and goes on, left goes back,
 * cross asks to leave.
 */
update_select_charge_page()
{
    new msg;
    new a;
    new b;
    new c;
    /* "FOCS": a the direction; "PUSH": a the row, b the button ("RRGT" circle, "RDWN" cross). */
    while (RecvMessage(msg, a, b, c))
    {
        SendMessage(g_page, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
        if (msg == TAG("FOCS"))
        {
            if (a == TAG("RIGT"))
            {
                if (g_has_charges == 1 && g_charge_chosen == 1)
                {
                    build_payment_menu();
                    close_ed_page(g_page, g_focus, SLIDE_OUT_LEFT);
                    new fetched = fetch_agreement();
                    if (fetched == 1)
                    {
                        set_next_state(STATE_AGREEMENT);
                        g_page_slide = SLIDE_IN_RIGHT;
                    }
                }
            }
            else if (a == TAG("LEFT"))
            {
                close_ed_page(g_page, g_focus, SLIDE_OUT_RIGHT);
                set_next_state(STATE_SELECT_SERVICE);
                g_page_slide = SLIDE_IN_LEFT;
            }
        }
        else if (msg == TAG("PUSH"))
        {
            if (b == TAG("RRGT"))
            {
                if (g_has_charges == 1)
                {
                    g_charge_cursor = a;
                    g_charge_chosen = 1;
                    update_page_arrows(g_arrows, 1, g_has_charges, g_charge_chosen, 1);
                }
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
 * Builds the menu element select_payment from the payments of the charge at g_charge_cursor, and reads the id, kind,
 * name and price of the charge.
 */
build_payment_menu()
{
    new listed;
    new count;
    new buf[64];
    new charge;
    new payment;
    new item;
    if (0 != g_payment_menu)
    {
        domRemoveTag(g_payment_menu);
        g_payment_menu = 0;
    }
    g_payment_menu = domAddTag(domRoot(), "menu");
    domSetData(g_payment_menu, "id", "select_payment");
    charge = find_nth_child_tag(g_charge_menu, "item", g_charge_cursor);
    g_payment_cursor = 0;
    domGetData(charge, "payment_num", buf, 64);
    count = atoi(buf);
    for (new i; i < count; i++)
    {
        payment = find_nth_child_tag(charge, "payment", i);
        item = domAddTag(g_payment_menu, "item");
        domGetData(payment, "id", buf, 64);
        domSetData(item, "id", buf);
        domGetData(payment, "name", buf, 64);
        domSetData(item, "value", buf);
        listed++;
    }
    g_has_payments = listed > 0 ? 1 : 0;
    domGetData(charge, "id", g_charge_id, 64);
    domGetData(charge, "kind", buf, 64);
    g_charge_kind = atoi(buf);
    domGetData(charge, "value", g_charge_name, 64);
    domGetData(charge, "tax_price", buf, 64);
    g_price = atoi(buf);
}

/*
 * Opens the menu of the payments; when the member has a registered card, first asks with DIALOG_PAYMENT_REGIST_CARD
 * whether to pay with it.
 */
open_select_payment_page()
{
    {
        new node;
        node = find_child_tag2(domRoot(), "page", "id", "PAGE_SELECT_PAYMENT");
        node = find_child_tag2(node, "item", "id", "item0");
        domRemoveData(node, "current");
        sprintf(g_text, "%d", g_payment_cursor);
        domSetData(node, "current", g_text);
    }
    g_text = "";
    g_page = open_ed_page("PAGE_SELECT_PAYMENT", g_focus, g_page_slide, g_text);
    g_payment_chosen = 0;
    update_page_arrows(g_arrows, 1, g_has_payments, g_payment_chosen, 1);
    if (0 == g_card_regist_status)
    {
        SendMessage(g_page, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
        wait_ms(floatround(200.0 * 2, 0));
        new choice = pop_dialog(g_focus, "DIALOG_PAYMENT_REGIST_CARD");
        if (0 == choice)
        {
            g_payment = PAYMENT_REGISTERED_CARD;
            close_ed_page(g_page, g_focus, SLIDE_OUT_LEFT);
            new ok = send_order(g_payment);
            if (ok == 1)
            {
                g_page_slide = SLIDE_IN_RIGHT;
            }
            else
            {
            }
        }
        else
        {
        }
        if (IsValidObject(g_page) == 1)
        {
            SendMessage(g_page, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        }
    }
}

/*
 * The messages of the ed_page: circle on a payment chooses it, right goes on with it, left goes back, cross asks to
 * leave.
 */
update_select_payment_page()
{
    new msg;
    new a;
    new b;
    new c;
    /* "FOCS": a the direction; "PUSH": a the row, b the button ("RRGT" circle, "RDWN" cross). */
    while (RecvMessage(msg, a, b, c))
    {
        SendMessage(g_page, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
        if (msg == TAG("FOCS"))
        {
            if (a == TAG("RIGT"))
            {
                if (g_has_payments == 1 && g_payment_chosen == 1)
                {
                    select_payment();
                }
            }
            else if (a == TAG("LEFT"))
            {
                set_next_state(STATE_SELECT_CHARGE);
                close_ed_page(g_page, g_focus, SLIDE_OUT_RIGHT);
                g_page_slide = SLIDE_IN_LEFT;
            }
        }
        else if (msg == TAG("PUSH"))
        {
            if (b == TAG("RRGT"))
            {
                g_payment_cursor = a;
                g_payment_chosen = 1;
                update_page_arrows(g_arrows, 1, g_has_payments, g_payment_chosen, 1);
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
 * Goes on with the payment at g_payment_cursor: the registered card and WebMoney order at once, a new card is typed
 * first.
 */
select_payment()
{
    new item;
    new buf[64];
    item = find_nth_child_tag(g_payment_menu, "item", g_payment_cursor);
    domGetData(item, "id", buf, 64);
    g_payment = atoi(buf);
    g_card_typed = 0;
    switch (g_payment)
    {
        case PAYMENT_REGISTERED_CARD:
        {
            close_ed_page(g_page, g_focus, SLIDE_OUT_LEFT);
            new ok = send_order(g_payment);
            if (ok == 1)
            {
                g_page_slide = SLIDE_IN_RIGHT;
            }
            else
            {
            }
        }
        case PAYMENT_CARD:
        {
            close_ed_page(g_page, g_focus, SLIDE_OUT_LEFT);
            set_next_state(STATE_REGIST_NOTICE);
            g_page_slide = SLIDE_IN_RIGHT;
        }
        case PAYMENT_PSBB_ID:
        {
        }
        case PAYMENT_WEBMONEY:
        {
            close_ed_page(g_page, g_focus, SLIDE_OUT_LEFT);
            new ok = send_order(g_payment);
            if (ok == 1)
            {
                set_next_state(STATE_WM_CONTENTS);
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

/* Opens the page with the purchase to confirm, for a card. */
open_contents_page()
{
    build_contents_args(g_text, 12288);
    g_page = open_ed_page("PAGE_CONTENTS", g_focus, g_page_slide, g_text);
    update_page_arrows(g_arrows, 1, 1, 1, 1);
}

/*
 * The messages of the ed_page: right goes on, left goes back to the security code or to where the card was chosen,
 * cross asks to leave.
 */
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
                close_ed_page(g_page, g_focus, SLIDE_OUT_LEFT);
                g_page_slide = SLIDE_IN_RIGHT;
                set_next_state(STATE_CONFIRM);
            }
            else if (a == TAG("LEFT"))
            {
                switch (g_payment)
                {
                    case PAYMENT_REGISTERED_CARD:
                    {
                        if (0 == g_need_security_code)
                        {
                            set_next_state(STATE_SELECT_PAYMENT);
                        }
                        else
                        {
                            set_next_state(STATE_SECURITY_CODE);
                        }
                    }
                    case PAYMENT_CARD:
                    {
                        if (0 == g_need_security_code)
                        {
                            set_next_state(STATE_CARD);
                        }
                        else
                        {
                            set_next_state(STATE_SECURITY_CODE);
                        }
                    }
                    case PAYMENT_PSBB_ID:
                    {
                    }
                    case PAYMENT_WEBMONEY:
                    {
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

open_confirm_page()
{
    g_text = "";
    g_page = open_ed_page("PAGE_CONFIRM", g_focus, g_page_slide, g_text);
    update_page_arrows(g_arrows, 1, 0, 1, 1);
}

/*
 * The messages of the ed_page: the choice of item0 ("PARM", "ID" and "VAL"), circle with the first choice asks with
 * DIALOG_BUY_CONFIRM and pays by card, else asks to leave; left goes back, cross asks to leave.
 */
update_confirm_page()
{
    new msg;
    new a;
    new b;
    new c;
    new id[64];
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
                set_next_state(STATE_CONTENTS);
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
                    new answer = pop_dialog(g_focus, "DIALOG_BUY_CONFIRM");
                    if (0 == answer)
                    {
                        close_ed_page(g_page, g_focus, SLIDE_OUT_NONE);
                        send_card_payment();
                    }
                    else
                    {
                        ask_abort(0);
                    }
                }
                else
                {
                    ask_abort(0);
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
            wait_message_str(TAG("ID"), id, 64);
            if (0 == strcmp("item0", id))
            {
                wait_message(TAG("VAL"), a);
                choice = a;
            }
        }
    }
}

/* Opens the page with the purchase to confirm, for WebMoney. */
open_wm_contents_page()
{
    build_contents_args(g_text, 12288);
    g_page = open_ed_page("PAGE_CONTENTS", g_focus, g_page_slide, g_text);
    update_page_arrows(g_arrows, 0, 1, 1, 1);
}

/* The messages of the ed_page: right goes on, cross asks to leave. */
update_wm_contents_page()
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
                set_next_state(STATE_WM_CONFIRM);
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

open_wm_confirm_page()
{
    g_text = "";
    g_page = open_ed_page("PAGE_CONFIRM", g_focus, g_page_slide, g_text);
    update_page_arrows(g_arrows, 1, 0, 1, 1);
}

/*
 * The messages of the ed_page: the choice of item0, circle with the first choice asks with DIALOG_BUY_CONFIRM and goes
 * on to the scratch number, else asks to leave; left goes back, cross asks to leave.
 */
update_wm_confirm_page()
{
    new msg;
    new a;
    new b;
    new c;
    new id[64];
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
                set_next_state(STATE_WM_CONTENTS);
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
                    new answer = pop_dialog(g_focus, "DIALOG_BUY_CONFIRM");
                    if (0 == answer)
                    {
                        close_ed_page(g_page, g_focus, SLIDE_OUT_NONE);
                        set_next_state(STATE_SCRATCH_NO);
                    }
                    else
                    {
                        ask_abort(0);
                    }
                }
                else
                {
                    ask_abort(0);
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
            wait_message_str(TAG("ID"), id, 64);
            if (0 == strcmp("item0", id))
            {
                wait_message(TAG("VAL"), a);
                choice = a;
            }
        }
    }
}

open_scratch_no_page()
{
    g_text = "";
    g_wm_session_id = "";
    g_page = open_ed_page("PAGE_SCRACHNO", g_focus, g_page_slide, g_text);
    g_scratch_no_valid = 0;
    g_scratch_no = "";
    update_page_arrows(g_arrows, 0, 1, g_scratch_no_valid, 1);
}

/* The messages of the ed_page: the scratch number typed, right pays with it, cross asks to leave. */
update_scratch_no_page()
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
            if (0 == strcmp("scrachno", name))
            {
                wait_message_str(TAG("STR"), g_scratch_no, 64);
                g_scratch_no_valid = is_valid_scratch_no(g_scratch_no);
                update_page_arrows(g_arrows, 0, 1, g_scratch_no_valid, 1);
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
                if (g_scratch_no_valid == 1)
                {
                    close_ed_page(g_page, g_focus, SLIDE_OUT_LEFT);
                    new paid = send_wm_payment();
                    if (paid == 1)
                    {
                        set_next_state(STATE_COMPLETE);
                        g_page_slide = SLIDE_IN_RIGHT;
                    }
                    else
                    {
                        g_page_slide = SLIDE_IN_LEFT;
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

open_add_scratch_no_page()
{
    g_text = "";
    g_page = open_ed_page("PAGE_ADD_SCRACHNO", g_focus, g_page_slide, g_text);
    g_scratch_no_valid = 0;
    g_scratch_no = "";
    update_page_arrows(g_arrows, 0, 0, 1, 1);
}

/* The messages of the ed_page: another scratch number typed, right pays with it, cross asks to leave. */
update_add_scratch_no_page()
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
            if (0 == strcmp("scrachno", name))
            {
                wait_message_str(TAG("STR"), g_scratch_no, 64);
                g_scratch_no_valid = is_valid_scratch_no(g_scratch_no);
                update_page_arrows(g_arrows, 0, 1, g_scratch_no_valid, 1);
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
                if (g_scratch_no_valid == 1)
                {
                    close_ed_page(g_page, g_focus, SLIDE_OUT_NONE);
                    new paid = send_wm_payment();
                    if (paid == 1)
                    {
                        set_next_state(STATE_COMPLETE);
                    }
                    else
                    {
                        g_page_slide = SLIDE_IN_LEFT;
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

open_regist_notice_page()
{
    g_text = "";
    g_page = open_ed_page("PAGE_REGIST_NOTICE", g_focus, g_page_slide, g_text);
    update_page_arrows(g_arrows, 1, 1, 1, 1);
}

/* The messages of the ed_page: right goes on, left goes back, cross asks to leave. */
update_regist_notice_page()
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
                set_next_state(STATE_CARD);
                close_ed_page(g_page, g_focus, SLIDE_OUT_LEFT);
                g_page_slide = SLIDE_IN_RIGHT;
            }
            else if (a == TAG("LEFT"))
            {
                set_next_state(STATE_SELECT_PAYMENT);
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

/* Opens the page of a card typed for this purchase. */
open_card_page()
{
    g_payment = PAYMENT_CARD;
    g_card_typed = 1;
    sprintf(g_text, "cardno=%s;period=%s;", g_card_number, g_card_period);
    g_page = open_ed_page("PAGE_CARD", g_focus, g_page_slide, g_text);
    g_card_valid = is_valid_card_number(g_card_number);
    update_page_arrows(g_arrows, 1, 1, g_card_valid, 1);
}

/*
 * The messages of the ed_page: the card number and period typed, right orders with a valid number, left goes back,
 * cross asks to leave.
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
                wait_message_str(TAG("STR"), g_card_number, 5);
                g_card_valid = is_valid_card_number(g_card_number);
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
                    close_ed_page(g_page, g_focus, SLIDE_OUT_LEFT);
                    new ordered = send_order(g_payment);
                    if (ordered == 1)
                    {
                        g_page_slide = SLIDE_IN_RIGHT;
                    }
                    else
                    {
                    }
                }
            }
            else if (a == TAG("LEFT"))
            {
                set_next_state(STATE_REGIST_NOTICE);
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

/* Opens the card page again after the payment refused the card. */
open_card_reinput_page()
{
    g_payment = PAYMENT_CARD;
    g_card_typed = 1;
    sprintf(g_text, "cardno=%s;period=%s;", g_card_number, g_card_period);
    g_page = open_ed_page("PAGE_CARD_REINPUT", g_focus, g_page_slide, g_text);
    g_card_valid = is_valid_card_number(g_card_number);
    update_page_arrows(g_arrows, 0, 1, g_card_valid, 1);
}

/*
 * The messages of the ed_page: the card number and period typed, right checks a valid card with the server, cross asks
 * to leave.
 */
update_card_reinput_page()
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
                wait_message_str(TAG("STR"), g_card_number, 5);
                g_card_valid = is_valid_card_number(g_card_number);
                update_page_arrows(g_arrows, 0, 1, g_card_valid, 1);
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
                    close_ed_page(g_page, g_focus, SLIDE_OUT_LEFT);
                    g_page_slide = SLIDE_IN_RIGHT;
                    check_card();
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

open_security_code_page()
{
    sprintf(g_text, "securitycode=%s;", g_security_code);
    g_page = open_ed_page("PAGE_SECURITYCODE", g_focus, g_page_slide, g_text);
    update_page_arrows(g_arrows, 1, 1, g_security_code_valid, 1);
}

/*
 * The messages of the ed_page: the security code typed, right goes on with a valid code, left goes back to where the
 * card was chosen, cross asks to leave.
 */
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
                    set_next_state(STATE_CONTENTS);
                    close_ed_page(g_page, g_focus, SLIDE_OUT_LEFT);
                    g_page_slide = SLIDE_IN_RIGHT;
                }
            }
            else if (a == TAG("LEFT"))
            {
                switch (g_payment)
                {
                    case PAYMENT_REGISTERED_CARD:
                    {
                        set_next_state(STATE_SELECT_PAYMENT);
                    }
                    case PAYMENT_CARD:
                    {
                        set_next_state(STATE_CARD);
                    }
                    case PAYMENT_PSBB_ID:
                    {
                    }
                    case PAYMENT_WEBMONEY:
                    {
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

/* Opens the security code page again after the payment refused the code. */
open_code_reinput_page()
{
    sprintf(g_text, "securitycode=%s;", g_security_code);
    g_page = open_ed_page("PAGE_SECURITYCODE", g_focus, g_page_slide, g_text);
    update_page_arrows(g_arrows, 0, 1, g_security_code_valid, 1);
}

/* The messages of the ed_page: the security code typed, right pays with a valid code, cross asks to leave. */
update_code_reinput_page()
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
                update_page_arrows(g_arrows, 0, 1, g_security_code_valid, 1);
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
                    close_ed_page(g_page, g_focus, SLIDE_OUT_NONE);
                    send_card_payment();
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

/* Opens the page with the receipt number and the inquiry ID. */
open_complete_page()
{
    new line[64];
    g_text = "";
    get_tag_attr("TEXT", "msged_buy_receipt_no", "value", line, 64);
    sprintf(line, line, g_order_id);
    sprintf(line, "ticket=%s;", line);
    append_string(g_text, line, 12288);
    {
        new inquiry_head[8];
        new inquiry_tail[8];
        split_inquiry_id(g_inquiry_id, inquiry_head, inquiry_tail, 8, 8);
        get_tag_attr("TEXT", "msged_member_entry_tablelist_inquiry", "value", line, 64);
        sprintf(line, line, inquiry_head, inquiry_tail);
        sprintf(line, "inqid=%s;", line);
        append_string(g_text, line, 12288);
    }
    g_page = open_ed_page("PAGE_COMPLETE", g_focus, g_page_slide, g_text);
    update_page_arrows(g_arrows, 0, 0, 1, 1);
    play_se(ED_SE_COMPLETE);
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
                quit_page();
            }
        }
        if (IsValidObject(g_page) == 1)
        {
            SendMessage(g_page, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
        }
    }
}
