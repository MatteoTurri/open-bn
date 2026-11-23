/*
 * /opt0/bn/script/edclient/agreement.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The page of the membership agreement: fetches its text from the server and shows it in an ed_page; cross
 * leaves it. It declares the natives of the library itself and has its own copies of the helpers.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "edpage.inc"

/* Values of g_state: the page shown. */
const STATE_AGREEMENT = 0;
const STATE_NONE = 1; /* before the first page */

#pragma library "edclient"
/* Opens the client of the server host:port (https); returns 0, or a negative error. */
native edsClientInit(const host[], port);
native edsClientExit();
/* Copies field (EDS_FIELD_*) into buf; maxlength is not used. */
native edsClientGetParam(field, buf[], maxlength);
native edsClientSetStr(field, const value[]);
native edsClientSetInt(field, value);
/*
 * Starts request (EDS_REQ_*), or returns EDS_INVALID_PARAM or EDS_BUSY; edsClientSync gives its result, then the
 * answer is read with edsClientGetInt and edsClientGetStr.
 */
native edsClientRequest(request);
/*
 * device: 0 the hard disk, 1 and 2 the memory cards (also for edsClientRemove, edsClientLoad and edsClientSave);
 * edsClientFormat formats only a memory card. These start the operation, which edsClientSync waits for.
 */
native edsClientFormat(device);
native edsClientRemove(device);
/*
 * Reads the state of the memory cards; edsClientSync gives it: four bits for each card (bits 4-7 card 1, bits 0-3
 * card 2), the type of the card in the low two (2: a PS2 memory card) and bit 2 set when it is formatted.
 */
native edsClientPoll();
/* Reads the saved ID: EDS_FIELD_AUTH_ID and EDS_FIELD_AUTH_PASSWD. */
native edsClientLoad(device);
/* Saves EDS_FIELD_AUTH_ID and EDS_FIELD_AUTH_PASSWD; with bit 0x8000 in device it replaces an ID saved already. */
native edsClientSave(device);
/* Returns nonzero while the last operation runs; then eds_result is its result. */
native edsClientSync(&eds_result);
/* Cancels the request running. */
native edsClientCancel();
/* Opens entry index of a list of the answer, for edsClientGetStr and edsClientGetInt; edsClientLeave closes it. */
native edsClientEnter(index);
native edsClientLeave();
/* Copies the string field of the answer into buf (field -1: the date of the server, YYYYMMDD); returns 0, or -1. */
native edsClientGetStr(field, const buf[], maxlength);
/* Returns the number field of the answer, or -1. */
native edsClientGetInt(field);
#pragma library

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
wait_message(msg, &a=0, &b=0, &c=0, &d=0)
{
    while (!FindMessage(msg, a, b, c, d))
    {
        sleep;
    }
}

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

/* Same as start_top_page of disc.inc. */
start_top_page(keep_history)
{
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
    if (!keep_history)
    {
        hisClear();
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

new g_any_child[1]; /* empty: any element */
new g_any_next[1]; /* empty: any element */

/* Same as find_child_tag of dom.inc. */
find_child_tag(parent, tag[], attr[], value[])
{
    new name[64];
    new node;
    new len;
    node = domFirstChild(parent, g_any_child);
    for (; node; node = domNext(node, g_any_next))
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

/* Same as wait_xml_idle of dom.inc. */
wait_xml_idle()
{
    for (new status; ; )
    {
        sleep;
        status = xmlLoadSync();
        if (0 != status)
        {
            break;
        }
    }
}

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

#include <float>

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
    send_focus(focus_obj, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
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

/* Same as update_page_arrows of layout.inc. */
update_page_arrows(arrows[], show_prev, show_next, next_lit, prev_lit)
{
    if (show_next == 1)
    {
        if (next_lit == 1)
        {
            AnimColor(arrows[PAGE_ARROW_NEXT], 1.0, 1.0, 1.0, 0.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
            AnimColor(arrows[PAGE_ARROW_NEXT_LIT], 1.0, 1.0, 1.0, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
        }
        else
        {
            AnimColor(arrows[PAGE_ARROW_NEXT], 1.0, 1.0, 1.0, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
            AnimColor(arrows[PAGE_ARROW_NEXT_LIT], 1.0, 1.0, 1.0, 0.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
        }
    }
    else
    {
        AnimColor(arrows[PAGE_ARROW_NEXT], 1.0, 1.0, 1.0, 0.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(arrows[PAGE_ARROW_NEXT_LIT], 1.0, 1.0, 1.0, 0.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
    }
    if (show_prev == 1)
    {
        if (prev_lit == 1)
        {
            AnimColor(arrows[PAGE_ARROW_PREV], 1.0, 1.0, 1.0, 0.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
            AnimColor(arrows[PAGE_ARROW_PREV_LIT], 1.0, 1.0, 1.0, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
        }
        else
        {
            AnimColor(arrows[PAGE_ARROW_PREV], 1.0, 1.0, 1.0, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
            AnimColor(arrows[PAGE_ARROW_PREV_LIT], 1.0, 1.0, 1.0, 0.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
        }
    }
    else
    {
        AnimColor(arrows[PAGE_ARROW_PREV], 1.0, 1.0, 1.0, 0.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(arrows[PAGE_ARROW_PREV_LIT], 1.0, 1.0, 1.0, 0.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
    }
}

/* Same as fade_ed_background of layout.inc. */
fade_ed_background(Float:target_a, Float:duration)
{
    new scene = wndGetScene(WND_BG);
    if (0 != scene)
    {
        SendMessage(scene, TAG("FADE"), target_a, duration, 0, 0);
    }
}

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
load_image_resources()
{
    new id[64];
    new buf[64];
    new node;
    new len;
    node = domFirstChild(domRoot(), g_any_child);
    for (; node; node = domNext(node, g_any_next))
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

/* The server of the paid services and the name and version this client gives it. */
new g_server_host[] = "lib.feega.sonyfinance.co.jp";
new g_server_port = 14020;
new g_client_name[] = "BBnavi";
new g_client_version[] = "1.0.0";

/* Same as play_se of background.inc. */
play_se(se)
{
    soundSePlay(se, 127);
}

/* Same as run_eds_request of edsreq.inc, with arrows optional. */
run_eds_request(request, page_id[], focus_obj, arrows[] = {0, 0, 0, 0}, cancelable)
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

/* Same as create_title of title.inc. */
create_title(text_id[])
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

/* Same as set_text_value of title.inc. */
set_text_value(id[], value[])
{
    new node;
    node = domFindTag(domRoot(), "TEXT", id, 0);
    if (0 == node)
    {
        node = domAddTag(domRoot(), "TEXT");
        domSetData(node, "id", id);
    }
    domSetData(node, "", value);
}

/* Same as set_pad_help_labels of title.inc. */
set_pad_help_labels(mask)
{
    new label[16];
    if (mask & PADHELP_CIRCLE)
    {
        label{0} = 0xE6;
        label{1} = 0xB1;
        label{2} = 0xBA;
        label{3} = 0xE5;
        label{4} = 0xAE;
        label{5} = 0x9A;
        label{6} = 0;
        SetSystemParamStr(SYS_STR_PADHELP_RR, label);
    }
    if (mask & PADHELP_CROSS)
    {
        label{0} = 0xE6;
        label{1} = 0x88;
        label{2} = 0xBB;
        label{3} = 0xE3;
        label{4} = 0x82;
        label{5} = 0x8B;
        label{6} = 0;
        SetSystemParamStr(SYS_STR_PADHELP_RD, label);
    }
    if (mask & PADHELP_TRIANGLE)
    {
        label{0} = 0xE3;
        label{1} = 0x82;
        label{2} = 0xAA;
        label{3} = 0xE3;
        label{4} = 0x83;
        label{5} = 0x97;
        label{6} = 0xE3;
        label{7} = 0x82;
        label{8} = 0xB7;
        label{9} = 0xE3;
        label{10} = 0x83;
        label{11} = 0xA7;
        label{12} = 0xE3;
        label{13} = 0x83;
        label{14} = 0xB3;
        label{15} = 0;
        SetSystemParamStr(SYS_STR_PADHELP_RU, label);
    }
    if (mask & PADHELP_SQUARE)
    {
        label{0} = 0xE3;
        label{1} = 0x83;
        label{2} = 0x81;
        label{3} = 0xE3;
        label{4} = 0x83;
        label{5} = 0xA3;
        label{6} = 0xE3;
        label{7} = 0x83;
        label{8} = 0xB3;
        label{9} = 0xE3;
        label{10} = 0x83;
        label{11} = 0x8D;
        label{12} = 0xE3;
        label{13} = 0x83;
        label{14} = 0xAB;
        label{15} = 0;
        SetSystemParamStr(SYS_STR_PADHELP_RL, label);
    }
}

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
    dialog_node = find_child_tag(domRoot(), "dialog", "id", dialog_id);
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
    node = domFirstChild(dialog_node, g_any_child);
    for (; node; node = domNext(node, g_any_next))
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
    node = domFirstChild(dialog_node, g_any_child);
    for (; node; node = domNext(node, g_any_next))
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
    node = domFirstChild(dialog_node, g_any_child);
    for (; node; node = domNext(node, g_any_next))
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
new g_page_slide = SLIDE_IN_NONE;
new g_text[12288]; /* the text of the agreement */

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
    load_image_resources();
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
    create_title("msged_feega_title_agreement");
    fetch_agreement();
    g_next_state = STATE_AGREEMENT;
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
        case STATE_AGREEMENT:
        {
            open_agreement_page();
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
        case STATE_AGREEMENT:
        {
            update_agreement_page();
        }
        default:
        {
        }
    }
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

/* Gets the text of the agreement from the server into the TEXT element mess_agreement; on an error, leaves. */
fetch_agreement()
{
    new ret;
    ret = run_eds_request(EDS_REQ_GET_MEMBER_AGREEMENT, "PAGE_SERVER_CONNECT", g_focus, _, 1);
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
    }
    g_text = "";
    ret = edsClientGetStr(AGREEMENT_AGREEMENT, g_text, 12288);
    set_text_value("mess_agreement", g_text);
    return 1;
}

open_agreement_page()
{
    g_text = "";
    g_page = open_ed_page("PAGE_AGREEMENT", g_focus, g_page_slide, g_text);
}

/* The messages of the ed_page: cross leaves. */
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
