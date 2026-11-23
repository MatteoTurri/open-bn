/*
 * /opt0/bn/script/edclient/author.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The authentication page of the paid services: has the server check the game (GameID, GamePasswd) and shows
 * its information, takes the PSBB ID and password (typed, or read from a memory card or the hard disk), lists
 * the services of the member and opens the one chosen: a link, a download or a DNAS download.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "edpage.inc"

/* Values of g_state: the page shown. */
const STATE_INFORMATION = 0;    /* the information of the game */
const STATE_CERTIFICATE = 1;    /* the PSBB ID and password, typed */
const STATE_DEVICE = 2;         /* the device to read them from */
const STATE_SELECT_SERVICE = 3; /* the services of the member */
const STATE_SERVICE_DETAIL = 4; /* the inquiry ID and the expiry of a service */
const STATE_NONE = 5;           /* no page: the next one opens even if it is the same */

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

/* Opens the built-in download page, which runs the downloads queued with cdlQueAdd. */
open_download_page()
{
    return xmlLoadStart("$$BUILDIN_DOWNLOAD$$", _, _, XMLLOAD_MAIN);
}

#include "text.inc"

/* Copies src into dest (maxlen cells) with every occurrence of from replaced by to. */
replace_all(dest[], src[], from[], to[], maxlen)
{
    new count;
    new from_len = strlen(from);
    new to_len = strlen(to);
    new src_pos;
    new dest_pos;
    for (; ; )
    {
        new found = find_substring(src, from, src_pos);
        if (found < 0)
        {
            copy_substring(dest, dest_pos, src, src_pos, -1, maxlen);
            return count;
        }
        copy_substring(dest, dest_pos, src, src_pos, found - src_pos, maxlen);
        dest_pos = dest_pos + (found - src_pos);
        append_string(dest, to, maxlen);
        dest_pos = dest_pos + to_len;
        src_pos = found + from_len;
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

/*
 * Opens the built-in download page on the content of the element node (title, album, cref, thumbnail_image
 * and nextref, by default the current page); returns 0 without cref.
 */
open_download2_page(node)
{
    new args[512];
    new value[300];
    value[0] = 0;
    domGetData(node, "title", value, 300);
    append_string(args, "title=", 512);
    append_string(args, value, 512);
    value[0] = 0;
    domGetData(node, "album", value, 300);
    append_string(args, ";album=", 512);
    append_string(args, value, 512);
    value[0] = 0;
    domGetData(node, "cref", value, 300);
    if (0 == value[0])
    {
        printf("cref must be set.\n");
        return 0;
    }
    xmlMakeFullPath(value, 300);
    append_string(args, ";cref=", 512);
    append_string(args, value, 512);
    value[0] = 0;
    domGetData(node, "thumbnail_image", value, 300);
    xmlMakeFullPath(value, 300);
    append_string(args, ";thumbnail_image=", 512);
    append_string(args, value, 512);
    value[0] = 0;
    domGetData(node, "nextref", value, 300);
    if (0 == value[0])
    {
        xmlGetURI(value, 300);
    }
    xmlMakeFullPath(value, 300);
    append_string(args, ";nextref=", 512);
    append_string(args, value, 512);
    xmlLoadStart("$$BUILDIN_DOWNLOAD2$$", "", args, XMLLOAD_MAIN);
    wait_xml_load();
    return 1;
}

/*
 * Sets nextref, cref and thumbnail_image of node from the arguments ret, ref and image; returns 0 without ret
 * or ref.
 */
set_download_refs(node)
{
    new value[64];
    if (GetArg("ret", value, 64))
    {
        domSetData(node, "nextref", value);
        if (GetArg("ref", value, 64))
        {
            domSetData(node, "cref", value);
            if (GetArg("image", value, 64))
            {
                domSetData(node, "thumbnail_image", value);
            }
            return 1;
        }
    }
    return 0;
}

/* The attributes of a DNAS download, which come from the arguments of the same name. */
new g_dnas_attrs[13][] = {
    "ver", "type", "title", "title_id", "dldata_size", "inst_size", "content_type", "pat_name",
    "pat_size", "inst_alloc_type", "do_id", "application_name", "jump_flag"
};

/* Sets the attributes names of node (count of them) from the arguments of the same name. */
copy_args_to_tag(node, names[][], count)
{
    new value[64];
    while (--count >= 0)
    {
        if (GetArg(names[count], value, 64))
        {
            domSetData(node, names[count], value);
        }
    }
    return 1;
}

/*
 * Queues the DNAS download described by the arguments in a new element tag ("." in title_sub_id become ";")
 * and opens the download page.
 */
start_dnas_download(tag[])
{
    new node;
    if (node = domAddTag(domRoot(), tag))
    {
        if (set_download_refs(node))
        {
            if (copy_args_to_tag(node, g_dnas_attrs, 13))
            {
                new sub_id[64];
                if (GetArg("title_sub_id", sub_id, 64))
                {
                    new sub_ids[64];
                    replace_all(sub_ids, sub_id, ".", ";", 64);
                    domSetData(node, "title_sub_id", sub_ids);
                }
                cdlQueAdd(node, -1);
                open_download_page();
                wait_xml_load();
            }
        }
    }
}

/* The attributes of a download that come from the arguments. */
new g_download_attrs[2][2] = {"title", "album"};

/* Opens the download described by the arguments, through a new element tag. */
start_download(tag[])
{
    new node;
    if (node = domAddTag(domRoot(), tag))
    {
        if (set_download_refs(node))
        {
            if (copy_args_to_tag(node, g_download_attrs, 2))
            {
                open_download2_page(node);
            }
        }
    }
}

/*
 * Appends src to the packed string dest (maxlen cells) URL encoded: a space as "+", the characters that are
 * not letters, digits or "*-.@_" as "%xx". Returns the new length, -1 when dest is full.
 */
append_url_encoded(dest[], src[], maxlen)
{
    new c;
    new len = strlen(dest);
    for (new i; 0 != (c = src{i}); i++)
    {
        if (len >= maxlen << 2)
        {
            return -1;
        }
        if (c == ' ')
        {
            dest{len++} = '+';
        }
        else if ('a' <= c <= 'z' || 'A' <= c <= 'Z' || '0' <= c <= '9' || c == '*' || c == '-' || c == '.' || c == '@'
                || c == '_')
        {
            dest{len++} = c;
        }
        else if (len + 3 < maxlen << 2)
        {
            new hex[1];
            sprintf(hex, "%02x", c);
            dest{len++} = '%';
            dest{len++} = hex{0};
            dest{len++} = hex{1};
        }
        else
        {
            return -1;
        }
    }
    dest{len} = 0;
    return len;
}

append_post_field(post[], name[], value[], maxlen)
{
    if (strlen(post))
    {
        append_string(post, "&", maxlen);
    }
    append_string(post, name, maxlen);
    append_string(post, "=", maxlen);
    append_url_encoded(post, value, maxlen);
}

/*
 * Opens the page of the argument ref (with the arguments refarg) by a POST of the date, the inquiry ID, the
 * service, its end date and its ticket.
 */
post_service_link(now_date[], service_id[], end_date[], inquiry_id[], ticket[])
{
    new url[64];
    if (GetArg("ref", url, 64))
    {
        new post[128] = {0};
        append_post_field(post, "NowDate", now_date, 128);
        append_post_field(post, "CommonInqID", inquiry_id, 128);
        append_post_field(post, "ServiceID", service_id, 128);
        append_post_field(post, "EndDate", end_date, 128);
        append_post_field(post, "Ticket", ticket, 128);
        xmlLoadSetPost(1, post, strlen(post));
        if (GetArg("refarg", post, 128))
        {
            xmlLoadStart(url, _, post, XMLLOAD_MAIN);
        }
        else
        {
            xmlLoadStart(url, _, _, XMLLOAD_MAIN);
        }
        wait_xml_load();
    }
}

/*
 * Opens the service as the argument tag says: "LINK" a page, "Download" a download, "DNAS_Download_nokey" a
 * DNAS download; then ends the script.
 */
start_service(now_date[], service_id[], end_date[], inquiry_id[], ticket[])
{
    new tag[8];
    if (GetArg("tag", tag, 8))
    {
        if (!strcmp(tag, "LINK"))
        {
            post_service_link(now_date, service_id, end_date, inquiry_id, ticket);
        }
        else if (!strcmp(tag, "Download"))
        {
            start_download(tag);
        }
        else if (!strcmp(tag, "DNAS_Download_nokey"))
        {
            start_dnas_download(tag);
        }
    }
    Destroy(Self());
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

new g_any_child3[1]; /* empty: any element */
new g_any_next3[1]; /* empty: any element */

/* Same as find_child_tag of dom.inc. */
find_child_tag3(parent, tag[], attr[], value[])
{
    new name[64];
    new node;
    new len;
    node = domFirstChild(parent, g_any_child3);
    for (; node; node = domNext(node, g_any_next3))
    {
        domGetTagName(node, name, 64);
        if (0 != strcmp(tag, name))
        {
            continue;
        }
        if (0 == strlen(attr) && 0 == strlen(value))
        {
            return node;
        }
        len = domGetData(node, attr, name, 64);
        if (0 == len)
        {
            continue;
        }
        if (0 == strcmp(value, name))
        {
            return node;
        }
    }
    return 0;
}

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
    xmlLoadStart(url, _, page_args, XMLLOAD_MAIN);
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
    xmlLoadStart(buf, bg_name, _, XMLLOAD_BG);
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
    node = domFirstChild(domRoot(), g_any_child3);
    for (; node; node = domNext(node, g_any_next3))
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

/* Loads the sound effects of the SE element se_<index>; returns the result of the load. */
load_se(index)
{
    new id[32];
    sprintf(id, "se_%d", index);
    new bd[256];
    new hd[256];
    get_tag_attr("SE", id, "bd", bd, 256);
    get_tag_attr("SE", id, "hd", hd, 256);
    soundSeLoadStart(bd, hd);
    new ret = wait_se_load();
    return ret;
}

#include "background.inc"

/* Plays the BGM element bgm_<index> from the start, loading it unless the player has the same files. */
play_bgm(index)
{
    printf("MIDI No. = %d\n", index);
    new id[32];
    sprintf(id, "bgm_%d", index);
    new bd[256];
    new hd[256];
    new sq[256];
    get_tag_attr("BGM", id, "bd", bd, 256);
    get_tag_attr("BGM", id, "hd", hd, 256);
    get_tag_attr("BGM", id, "sq", sq, 256);
    xmlMakeFullPath(bd, 256);
    xmlMakeFullPath(hd, 256);
    xmlMakeFullPath(sq, 256);
    new info[256];
    soundGetInfo(5, info, 256);
    if (0 != info[0])
    {
        new changed;
        soundGetInfo(0, info, 256);
        if (strcmp(info, bd))
        {
            changed = 1;
        }
        soundGetInfo(1, info, 256);
        if (strcmp(info, hd))
        {
            changed = 1;
        }
        soundGetInfo(2, info, 256);
        if (strcmp(info, sq))
        {
            changed = 1;
        }
        if (!changed)
        {
            printf("same bgm files has already been loaded.\n");
            return;
        }
    }
    new ret;
    soundBgmLoadStart(bd, hd, sq);
    ret = wait_bgm_load();
    printf("soundBgmLoadWait::ret = %d\n", ret);
    soundBgmSeek(0);
    soundBgmPlay();
}

#include "sysconf.inc"

/* The server of the paid services and the name and version this client gives it. */
new g_server_host[] = "lib.feega.sonyfinance.co.jp";
new g_server_port = 14020;
new g_client_name[] = "BBnavi";
new g_client_version[] = "1.0.0";

/* Same as run_eds_request of edsreq.inc. */
run_eds_request(request, page_id[], focus_obj, arrows[], cancelable)
{
    new msg;
    new a;
    new b;
    new c;
    new start_tick;
    new ret;
    new page;
    new slide_in;
    new slide_dir;
    slide_in = SLIDE_IN_NONE; /* redundant as written */
    slide_in = 0; /* acts as SLIDE_IN_NONE */
    if (0 != arrows[PAGE_ARROW_NEXT])
    {
        update_page_arrows(arrows, 0, 0, 1, 1);
    }
    ret = edsClientRequest(request);
    if (ret < 0)
    {
        printf("@@@ failed edsClientRequest [%d]\n", ret);
        return ret;
    }
    SystemCommand(SYSCMD_BUSY, 1);
    if (cancelable == 1)
    {
        /* the label of the cross button: "Abort" */
        new label[16];
        label{0} = 0xE4;
        label{1} = 0xB8;
        label{2} = 0xAD;
        label{3} = 0xE6;
        label{4} = 0x96;
        label{5} = 0xAD;
        label{6} = 0;
        SetSystemParam(SYS_PADHELP, PADHELP_CROSS);
        SetSystemParamStr(SYS_STR_PADHELP_RD, label);
    }
    else
    {
        SetSystemParam(SYS_PADHELP, 0);
    }
    start_tick = tickcount();
    new busy = 1;
    for (; ; )
    {
        while (RecvMessage(msg, a, b, c))
        {
        }
        new pressed;
        PadState(_, pressed, _, _, -1);
        if (pressed & PAD_CROSS)
        {
            if (cancelable == 1)
            {
                if (0 == busy)
                {
                    if (0 != page)
                    {
                        close_ed_page(page, focus_obj, slide_dir);
                    }
                    ret = EDS_CANCELED;
                    break;
                }
                else
                {
                    edsClientCancel();
                    printf("@@@@ edsClientCancel()\n");
                }
            }
        }
        if (busy == 1)
        {
            busy = edsClientSync(ret);
        }
        if (busy == 1)
        {
            if (0 == page)
            {
                start_tick = tickcount() < start_tick ? tickcount() : start_tick;
                if (tickcount() - start_tick > 5000)
                {
                    page = open_ed_page(page_id, focus_obj, slide_in);
                    SetPubVar(page, "g_iOpenSE", ED_SE_WAIT_OPEN);
                    SetPubVar(page, "g_iCloseSE", ED_SE_WAIT_CLOSE);
                    SendMessage(page, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                    start_tick = tickcount();
                }
            }
            else
            {
            }
        }
        else if (0 != page)
        {
            start_tick = tickcount() < start_tick ? tickcount() : start_tick;
            if (tickcount() - start_tick > 3000)
            {
                close_ed_page(page, focus_obj, slide_dir);
                break;
            }
        }
        else
        {
            break;
        }
        sleep;
    }
    while (RecvMessage(msg, a, b, c))
    {
    }
    SystemCommand(SYSCMD_BUSY, 0);
    if (ret < 0)
    {
        printf("@@@ request failed [%d]\n", ret);
        if (ret == EDS_CANCELED)
        {
        }
        else if (ret <= EDS_NETWORK_ERROR && ret > -3000)
        {
            ret = EDS_NETWORK_ERROR;
        }
        return ret;
    }
    return 0;
}

#include "validate.inc"

#include "pfs.inc"

new g_no_inquiry_head[1]; /* empty: the halves of an inquiry ID too short */
new g_no_inquiry_tail[1];
new g_default_game_id[1]; /* empty: without the arguments GameID and GamePasswd */
new g_default_game_passwd[1];

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
get_game_id(id[], maxlen)
{
    new found;
    found = GetArg(g_arg_game_id, id, maxlen);
    if (0 == found)
    {
        copy_string(id, g_default_game_id, maxlen);
    }
}

/* Reads the argument GamePasswd, or the empty g_default_game_passwd without it. */
get_game_passwd(passwd[], maxlen)
{
    new found;
    found = GetArg(g_arg_game_passwd, passwd, maxlen);
    if (0 == found)
    {
        copy_string(passwd, g_default_game_passwd, maxlen);
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

/* Returns the argument unloadBGM if it is 0 or 1, else 0; with 1 the page does not play its BGM. */
get_unload_bgm_arg()
{
    new unload;
    new value[32];
    new found;
    found = GetArg(g_arg_unload_bgm, value, 32);
    if (0 == found)
    {
        return;
    }
    unload = atoi(value);
    return 0 == unload || unload == 1 ? unload : 0;
}

/* Same as create_title of title.inc. */
create_title2(text_id[])
{
    new font_size;
    new Float:x;
    new Float:y;
    new Float:tex_w;
    new Float:tex_h;
    new text[64];
    new plane;
    new tex;
    new node = domFindTag(domRoot(), "TEXT", text_id, 0);
    domGetData(node, "value", text, 64);
    font_size = 16;
    imgDecStringStart(text, font_size, 64, 64, 0);
    while (IMG_DONE != wait_image_decode())
    {
        imgDecStringStart(text, font_size, 64, 64, 0);
    }
    x = 64.0;
    y = 88.0 + 20.0;
    tex = imgTexID();
    tex_w = float(imgTexWidth());
    tex_h = float(imgTexHeight());
    x = x + 0.5 * tex_w;
    plane = CreatePlane(Disp2D(0));
    SetTexture(plane, tex, 0, -1);
    SetPos(plane, x, y, 0.0, 0);
    SetColor(plane, 1.0, 1.0, 1.0, 0.0);
    SetScale(plane, tex_w, tex_h, 1.0);
    AnimColor(plane, 1.0, 1.0, 1.0, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
    return plane;
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
    dialog_node = find_child_tag3(domRoot(), "dialog", "id", dialog_id);
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
    set_pad_help_labels2(buttons);
    node = domFirstChild(dialog_node, g_any_child3);
    for (; node; node = domNext(node, g_any_next3))
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
    node = domFirstChild(dialog_node, g_any_child3);
    for (; node; node = domNext(node, g_any_next3))
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
    node = domFirstChild(dialog_node, g_any_child3);
    for (; node; node = domNext(node, g_any_next3))
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

/* Same as get_item_device of bgmode.inc. */
get_item_device(menu, index)
{
    new device;
    new text[64];
    new node;
    sprintf(text, "%d", index);
    node = domFindTag(menu, "item", text, 0);
    domGetData(node, "device", text, 64);
    device = atoi(text);
    return device;
}

new g_dialog_tag2[] = "dialog";
new g_type_attr2[4] = {0x74797065, 0, 0x64657669, 0x63650000}; /* "type", then "device" */

/* Same as wait_eds_with_dialog of edsid.inc, with globals of its own. */
wait_eds_with_dialog2(focus_obj, dialog_id[])
{
    new ret;
    new buf[128];
    new args[512];
    new dialog_node;
    new node;
    new len;
    dialog_node = domFindTag(domRoot(), g_dialog_tag2, dialog_id, 0);
    len = domGetData(dialog_node, g_type_attr2, buf, 128);
    args = "";
    append_string(args, "msg=", 512);
    node = domFirstChild(dialog_node, g_any_child3);
    for (; node; node = domNext(node, g_any_next3))
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
        append_string(args, buf, 512);
        append_string(args, "\n", 512);
    }
    append_string(args, ";", 512);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 260.0, 0.0, 0);
    AttachScript(dialog, _, "dialog", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus_obj);
    SetPubVar(dialog, "se_open", ED_SE_WAIT_OPEN);
    SetPubVar(dialog, "se_ok", ED_SE_DECIDE);
    SetPubVar(dialog, "se_cancel", ED_SE_CANCEL);
    SetPubVar(dialog, "se_cursor", ED_SE_CURSOR);
    while (edsClientSync(ret))
    {
        sleep;
    }
    SetPubVar(dialog, "canceled", 1);
    for (; ; )
    {
        if (ScriptState(dialog) == SCRIPT_FINISHED)
        {
            break;
        }
        sleep;
    }
    Destroy(dialog);
    play_se(ED_SE_WAIT_CLOSE);
    return ret;
}

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
    status = wait_eds_with_dialog2(focus_obj, dialog_id);
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
new g_next_state = STATE_NONE; /* the page to show from the next frame */
new g_page;
new g_page_slide = SLIDE_IN_NONE; /* the slide of the next open_ed_page */
new g_arrows[4];
new g_psbb_id[16]; /* the PSBB ID and password of the member */
new g_password[8];
new g_now_date[64];   /* from the server, with the list of the services */
new g_inquiry_id[64]; /* the inquiry ID of the member */
new g_service_id[32]; /* the service chosen: its id, end date and ticket */
new g_end_date[32];
new g_ticket[64];
new g_service_cursor; /* the item chosen in g_service_menu */
new g_device; /* where the ID was read from (EDS_DEVICE_*) */
new g_text[12288]; /* the arguments of the pages; also the information of the game */
new g_service_menu; /* the "menu" element select_game: the services of the member */
new g_device_cursor; /* the item chosen in the menu of the devices */
new g_device_menu; /* the menu element of the devices */
new g_game_id[64]; /* the arguments GameID and GamePasswd */
new g_game_passwd[64];
new g_has_services;
new g_has_devices;
new g_input_valid; /* the PSBB ID and the password typed are well formed */
new g_ready_devices; /* get_ready_devices when the device page opened */

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
    if (0 == get_unload_bgm_arg())
    {
        play_bgm(0);
    }
    load_se(0);
    g_focus = create_focus_plane2("focus");
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
    get_game_id(g_game_id, 64);
    get_game_passwd(g_game_passwd, 64);
    pop_dialog(g_focus, "DIALOG_SSL");
    create_title2("dummy_msged_author_title");
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
        case STATE_DEVICE:
        {
            open_device_page();
        }
        case STATE_SELECT_SERVICE:
        {
            open_select_service_page();
        }
        case STATE_SERVICE_DETAIL:
        {
            open_service_detail_page();
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
        case STATE_DEVICE:
        {
            update_device_page();
        }
        case STATE_SELECT_SERVICE:
        {
            update_select_service_page();
        }
        case STATE_SERVICE_DETAIL:
        {
            update_service_detail_page();
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

/* Closes the client, fades the background, stops the BGM and leaves for the top page, or where "retflag" says. */
quit_page()
{
    edsClientExit();
    kill_object(g_focus);
    fade_ed_background(0.0, 600.0);
    stop_bgm();
    load_page("LINK_TOP", _, 0);
}

/*
 * Has the server check the game and the service, tells whether it is free or charged and puts the information
 * of the game in the TEXT element mess_infomation; on an error, leaves.
 */
fetch_information()
{
    new ret;
    new buf[64];
    edsClientSetStr(EDS_FIELD_GROUP_ID, g_game_id);
    edsClientSetStr(EDS_FIELD_GROUP_PASSWD, g_game_passwd);
    ret = run_eds_request(EDS_REQ_GET_GROUP_CHARGE_INFO, "PAGE_SERVER_CONNECT", g_focus, g_arrows, 1);
    if (ret < 0)
    {
        if (ret == EDS_CANCELED)
        {
        }
        else if (ret == EDS_NETWORK_ERROR)
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
    ret = edsClientGetInt(GROUP_CHARGE_GROUP_AUTH_STATUS);
    if (0 != ret)
    {
        pop_dialog(g_focus, "ERR_DIALOG_GROUP_AUTHNG");
        quit_page();
        return 0;
    }
    ret = edsClientGetInt(GROUP_CHARGE_SERVICE_STATUS);
    if (0 != ret)
    {
        pop_dialog(g_focus, "ERR_DIALOG_SERVICE_NG");
        quit_page();
        return 0;
    }
    ret = edsClientGetInt(GROUP_CHARGE_CHARGE_STATUS);
    switch (ret)
    {
        case CHARGE_STATUS_FREE:
        {
            pop_dialog(g_focus, "DIALOG_FREE");
        }
        case CHARGE_STATUS_CHARGED:
        {
            pop_dialog(g_focus, "DIALOG_CHARGE");
        }
        default:
        {
        }
    }
    ret = edsClientGetStr(GROUP_CHARGE_MSG_CODE, buf, 64);
    edsClientGetStr(GROUP_CHARGE_MESSAGE, g_text, 12288);
    set_text_value("mess_infomation", g_text);
    return 1;
}

/*
 * Logs in with g_psbb_id and g_password and gets the services of the member into g_service_menu; returns 1
 * when done. A wrong ID or password goes back to the certificate page, other errors leave.
 */
fetch_services()
{
    new ret;
    new buf[64];
    edsClientSetStr(EDS_FIELD_GROUP_ID, g_game_id);
    edsClientSetStr(EDS_FIELD_AUTH_SYSTEM_ID, "001");
    edsClientSetStr(EDS_FIELD_AUTH_ID, g_psbb_id);
    edsClientSetStr(EDS_FIELD_AUTH_PASSWD, g_password);
    ret = run_eds_request(EDS_REQ_AUTH_GROUP, "PAGE_SERVER_CONNECT", g_focus, g_arrows, 1);
    if (ret < 0)
    {
        if (ret == EDS_CANCELED)
        {
            set_next_state(g_state);
        }
        else if (ret == EDS_NETWORK_ERROR)
        {
            pop_dialog(g_focus, "ERR_DIALOG_NETWORK_DISCONNECT");
            quit_page();
        }
        else
        {
            pop_dialog(g_focus, "ERR_DIALOG_ED_SYSTEM");
            quit_page();
        }
        return 0;
    }
    ret = edsClientGetInt(AUTH_GROUP_GROUP_AUTH_STATUS);
    if (0 != ret)
    {
        pop_dialog(g_focus, "ERR_DIALOG_GROUP_AUTHNG");
        quit_page();
        return 0;
    }
    ret = edsClientGetInt(AUTH_GROUP_SERVICE_STATUS);
    if (0 != ret)
    {
        pop_dialog(g_focus, "ERR_DIALOG_SERVICE_NG");
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
        return 0;
    }
    ret = edsClientGetStr(EDS_SERVER_DATE, g_now_date, 64);
    ret = edsClientGetStr(AUTH_GROUP_COMMON_INQ_ID, g_inquiry_id, 64);
    if (0 != g_service_menu)
    {
        domRemoveTag(g_service_menu);
        g_service_menu = 0;
    }
    g_service_menu = domAddTag(domRoot(), "menu");
    domSetData(g_service_menu, "id", "select_game");
    new count = edsClientGetInt(AUTH_GROUP_COUNT);
    new n;
    for (new i; i < count; i++)
    {
        if (edsClientEnter(i) < 0)
        {
            continue;
        }
        new item;
        item = domAddTag(g_service_menu, "item");
        ret = edsClientGetStr(SERVICE_SERVICE_ID, buf, 64);
        domSetData(item, "id", buf);
        ret = edsClientGetStr(SERVICE_NAME, buf, 64);
        domSetData(item, "value", buf);
        ret = edsClientGetStr(SERVICE_PARENT_SERVICE_ID, buf, 64);
        domSetData(item, "parent", buf);
        ret = edsClientGetInt(SERVICE_SERVICE_STATUS);
        sprintf(buf, "%d", ret);
        domSetData(item, "status", buf);
        ret = edsClientGetStr(SERVICE_END_DATE, buf, 64);
        domSetData(item, "enddate", buf);
        ret = edsClientGetStr(SERVICE_TICKET, buf, 64);
        domSetData(item, "ticket", buf);
        edsClientLeave();
        n++;
    }
    g_has_services = n > 0 ? 1 : 0;
    return 1;
}

open_information_page()
{
    g_text = "";
    g_page = open_ed_page("PAGE_INFOMATION", g_focus, g_page_slide, g_text);
}

/*
 * The messages of the ed_page: circle asks whether to read a saved ID (the device page) or to type it (the
 * certificate page), cross leaves.
 */
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
            }
            else if (a == TAG("LEFT"))
            {
            }
        }
        else if (msg == TAG("PUSH"))
        {
            if (b == TAG("RRGT"))
            {
                play_se(ED_SE_DECIDE);
                close_ed_page(g_page, g_focus, SLIDE_OUT_NONE);
                new choice = pop_dialog(g_focus, "DIALOG_USE_IDPASSWORD");
                if (0 == choice)
                {
                    set_next_state(STATE_DEVICE);
                }
                else
                {
                    set_next_state(STATE_CERTIFICATE);
                }
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

open_certificate_page()
{
    sprintf(g_text, "psbbid=%s;password=%s;", g_psbb_id, g_password);
    g_page = open_ed_page("PAGE_CERTIFICATE", g_focus, g_page_slide, g_text);
    g_input_valid = is_id_input_valid();
    update_page_arrows(g_arrows, 0, 1, g_input_valid, 1);
}

/*
 * The messages of the ed_page: "PARM" "psbbid" or "password" and STR give the new value, right logs in when both
 * are valid, triangle opens the registration page, cross leaves.
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
                    close_ed_page(g_page, g_focus, SLIDE_OUT_LEFT);
                    new ok = fetch_services();
                    if (ok == 1)
                    {
                        set_next_state(STATE_SELECT_SERVICE);
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
                play_se(ED_SE_CANCEL);
                quit_page();
            }
            if (b == TAG("RUP"))
            {
                edsClientExit();
                kill_object(g_focus);
                g_text = "";
                append_member_args(g_text, 12288);
                sprintf(name, "%s=2;", g_arg_retflag); /* RETFLAG_AUTHOR */
                append_string(g_text, name, 12288);
                load_page("LINK_REGIST", g_text, 1);
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

open_device_page()
{
    new count;
    g_ready_devices = get_ready_devices();
    count = build_device_menu(g_device_menu);
    g_has_devices = count > 0 ? 1 : 0;
    g_text = "";
    g_page = open_ed_page("PAGE_SELECT_DEVICE", g_focus, g_page_slide, g_text);
    update_page_arrows(g_arrows, 0, 1, 1, 1);
}

/*
 * The messages of the ed_page: "PARM" "item0" and VAL give the item of the device chosen, circle reads the ID
 * from it, cross offers to leave. The page opens again when the devices ready change.
 */
update_device_page()
{
    new msg;
    new a;
    new b;
    new c;
    new name[64];
    new devices = get_ready_devices();
    if (g_ready_devices != devices)
    {
        set_next_state(STATE_DEVICE);
        close_ed_page(g_page, g_focus, SLIDE_OUT_NONE);
        while (RecvMessageStr(msg, name, 64))
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
                    load_saved_id(g_device_cursor);
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
            wait_message_str(TAG("ID"), name, 64);
            if (0 == strcmp("item0", name))
            {
                wait_message(TAG("VAL"), a);
                g_device_cursor = a;
            }
        }
    }
}

/* Reads the ID saved on the device of the menu item and logs in with it; the device page again on a failure. */
load_saved_id(item)
{
    new ok;
    close_ed_page(g_page, g_focus, SLIDE_OUT_LEFT);
    g_device = get_item_device(g_device_menu, item);
    switch (g_device)
    {
        case EDS_DEVICE_CARD1:
        {
            ok = load_card_id(EDS_DEVICE_CARD1);
        }
        case EDS_DEVICE_CARD2:
        {
            ok = load_card_id(EDS_DEVICE_CARD2);
        }
        case EDS_DEVICE_HDD:
        {
            ok = load_hdd_id(EDS_DEVICE_HDD);
        }
        default:
        {
        }
    }
    if (ok == 1)
    {
        ok = fetch_services();
        if (ok == 1)
        {
            set_next_state(STATE_SELECT_SERVICE);
            g_page_slide = SLIDE_IN_RIGHT;
        }
        else
        {
            g_page_slide = SLIDE_IN_LEFT;
        }
    }
    else
    {
        set_next_state(STATE_DEVICE);
        g_page_slide = SLIDE_IN_LEFT;
    }
}

/* Opens the page of the services with the focus on the service chosen before. */
open_select_service_page()
{
    {
        new node;
        node = find_child_tag3(domRoot(), "page", "id", "PAGE_SELECT_SERVICE");
        node = find_child_tag3(node, "item", "id", "item0");
        domRemoveData(node, "current");
        sprintf(g_text, "%d", g_service_cursor);
        domSetData(node, "current", g_text);
    }
    g_text = "";
    g_page = open_ed_page("PAGE_SELECT_SERVICE", g_focus, g_page_slide, g_text);
    update_page_arrows(g_arrows, 0, 0, 1, 1);
}

/*
 * The messages of the ed_page: "PARM" "item0" and VAL give the item of the service chosen, right opens the
 * service, circle its details, cross leaves.
 */
update_select_service_page()
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
                if (g_has_services == 1)
                {
                    close_ed_page(g_page, g_focus, SLIDE_OUT_NONE);
                    new item;
                    item = find_nth_child_tag2(g_service_menu, "item", g_service_cursor);
                    domGetData(item, "id", g_service_id, 32);
                    domGetData(item, "enddate", g_end_date, 32);
                    domGetData(item, "ticket", g_ticket, 64);
                    start_service(g_now_date, g_service_id, g_end_date, g_inquiry_id, g_ticket);
                }
            }
            else if (a == TAG("LEFT"))
            {
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
                if (g_has_services == 1)
                {
                    set_next_state(STATE_SERVICE_DETAIL);
                    close_ed_page(g_page, g_focus, SLIDE_OUT_NONE);
                }
            }
            else if (b == TAG("RDWN"))
            {
                play_se(ED_SE_CANCEL);
                quit_page();
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
                g_service_cursor = a;
            }
        }
    }
}

/* Opens the details of the service chosen: the inquiry ID of the member and the expiry. */
open_service_detail_page()
{
    new buf[64];
    new end_date[64];
    new item;
    item = find_nth_child_tag2(g_service_menu, "item", g_service_cursor);
    g_text = "";
    {
        new head[8];
        new tail[8];
        split_inquiry_id(g_inquiry_id, head, tail, 8, 8);
        get_tag_attr("TEXT", "msged_member_entry_tablelist_inquiry", "value", buf, 64);
        sprintf(buf, buf, head, tail);
        sprintf(buf, "inqid=%s;", buf);
        append_string(g_text, buf, 12288);
    }
    domGetData(item, "enddate", end_date, 64);
    get_tag_attr("TEXT", "msged_buy_contents_expire", "value", buf, 64);
    sprintf(buf, buf, end_date);
    sprintf(buf, "expire=%s;", buf);
    append_string(g_text, buf, 12288);
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

/*
 * Reads the ID and password saved on the memory card device into g_psbb_id and g_password; returns 1 when
 * read. Saved data that cannot be used can be deleted.
 */
load_card_id(device)
{
    new status;
    new ok;
    status = edsClientLoad(device);
    if (status < 0)
    {
        printf("@@@ Error edsClientLoad() : [%d]\n", status);
        pop_dialog(g_focus, "ERR_DIALOG_LOAD_FAIL");
        quit_page();
    }
    while (edsClientSync(status))
    {
        sleep;
    }
    switch (status)
    {
        case 0:
        {
            edsClientGetParam(EDS_FIELD_AUTH_ID, g_psbb_id, 16);
            edsClientGetParam(EDS_FIELD_AUTH_PASSWD, g_password, 8);
            ok = 1;
        }
        case EDS_INVALID_PARAM:
        {
            new choice = pop_dialog(g_focus, "DIALOG_MC_INVALID_PARAM");
            if (0 == choice)
            {
                ok = remove_bad_card_id(device);
            }
            else
            {
            }
        }
        case EDS_INVALID_ID:
        {
            new choice = pop_dialog(g_focus, "DIALOG_MC_INVALID_ID");
            if (0 == choice)
            {
                ok = remove_bad_card_id(device);
            }
            else
            {
            }
        }
        case EDS_NO_CARD:
        {
            pop_dialog(g_focus, "DIALOG_NO_CARD");
        }
        case EDS_UNFORMATTED:
        {
            pop_dialog(g_focus, "DIALOG_UNFORMAT");
        }
        case EDS_NO_SPACE:
        {
            pop_dialog(g_focus, "DIALOG_NO_SPACE");
        }
        case EDS_FILE_STATE:
        {
            pop_dialog(g_focus, "DIALOG_NO_EXIST_FILE");
            quit_page();
        }
        case EDS_BAD_VERSION:
        {
            new choice = pop_dialog(g_focus, "DIALOG_MC_NOSUPPORT_VERSION");
            if (0 == choice)
            {
                ok = remove_bad_card_id(device);
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
    return ok;
}

/*
 * Deletes the ID saved on the memory card device and leaves the page; returns 1 when deleted. When the card is missing
 * or was changed, it says so and the page stays.
 */
remove_bad_card_id(device)
{
    new status;
    new ok;
    ok = run_eds_remove(g_focus, device, "DIALOG_DELETE_MEMORY_CARD", status);
    if (ok == 1)
    {
        pop_dialog(g_focus, "DIALOG_DELETE_COMPLETE");
        quit_page();
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
    return ok;
}

/* Like load_card_id, on the hard disk. */
load_hdd_id(device)
{
    new status;
    new ok;
    status = edsClientLoad(device);
    if (status < 0)
    {
        printf("@@@ Error edsClientLoad() : [%d]\n", status);
        pop_dialog(g_focus, "ERR_DIALOG_LOAD_FAIL");
        quit_page();
    }
    while (edsClientSync(status))
    {
        sleep;
    }
    switch (status)
    {
        case 0:
        {
            edsClientGetParam(EDS_FIELD_AUTH_ID, g_psbb_id, 16);
            edsClientGetParam(EDS_FIELD_AUTH_PASSWD, g_password, 8);
            ok = 1;
        }
        case EDS_INVALID_PARAM:
        {
            new choice = pop_dialog(g_focus, "DIALOG_HDD_INVALID_PARAM");
            if (0 == choice)
            {
                ok = remove_bad_hdd_id(device);
            }
            else
            {
            }
        }
        case EDS_INVALID_ID:
        {
            new choice = pop_dialog(g_focus, "DIALOG_HDD_INVALID_ID");
            if (0 == choice)
            {
                ok = remove_bad_hdd_id(device);
            }
            else
            {
            }
        }
        case EDS_NO_SPACE:
        {
            pop_dialog(g_focus, "DIALOG_NO_SPACE");
        }
        case EDS_FILE_STATE:
        {
            pop_dialog(g_focus, "DIALOG_NO_EXIST_FILE");
            quit_page();
        }
        case EDS_BAD_VERSION:
        {
            new choice = pop_dialog(g_focus, "DIALOG_HDD_NOSUPPORT_VERSION");
            if (0 == choice)
            {
                ok = remove_bad_hdd_id(device);
            }
            else
            {
            }
        }
        default:
        {
            printf("@@@@ Error edsClientLoad() : Unknown status : [%d]\n", status);
        }
    }
    return ok;
}

/* Deletes the ID saved on the hard disk device and leaves the page; returns 1 when deleted. */
remove_bad_hdd_id(device)
{
    new status;
    new ok;
    ok = run_eds_remove(g_focus, device, "DIALOG_DELETE_HDD", status);
    if (ok == 1)
    {
        pop_dialog(g_focus, "DIALOG_DELETE_COMPLETE");
        quit_page();
    }
    else
    {
        pop_dialog(g_focus, "ERR_DIALOG_DELETE_FAIL");
        quit_page();
    }
    return ok;
}

#include "edsid.inc"
