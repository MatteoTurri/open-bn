/*
 * /opt0/bn/script/edclient/member_menu.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The member menu of the paid services: registration, change, leave, agreement, saved ID and guide. Each opens
 * its page with the member arguments; cross goes back to the top page.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

#include "wait.inc"

#include "tag.inc"

#include "edpage.inc"

#include "sound.inc"

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

#include "misc.inc"

/* The arguments of the member pages, which the pages pass on from page to page. */
new g_arg_ret[] = "ret";
new g_arg_retarg[] = "retarg";
new g_arg_retflag[] = "retflag";

/* Appends to args (maxlen cells) the member arguments the page was opened with, as "name=value;". */
append_member_args(args[], maxlen)
{
    new value[64];
    new found;
    found = GetArg(g_arg_ret, value, 64);
    static arg_game_ver[2] = "GameVer";
    static arg_game_id[2] = "GameID";
    static arg_game_passwd[3] = "GamePasswd";
    static arg_given_name[3] = "givenname";
    static arg_family_name[3] = "familyname";
    static arg_birthday[3] = "birthday";
    static arg_zipcode[2] = "zipcode";
    static arg_sex[1] = "sex";
    static arg_mail_addr[3] = "mailaddr";
    static arg_mail_hope[3] = "mailhope";
    static arg_unload_bgp[3] = "unloadBGP";
    static arg_unload_bgm[3] = "unloadBGM";
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
    found = GetArg(arg_game_ver, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", arg_game_ver, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(arg_game_id, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", arg_game_id, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(arg_game_passwd, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", arg_game_passwd, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(arg_given_name, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", arg_given_name, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(arg_family_name, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", arg_family_name, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(arg_birthday, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", arg_birthday, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(arg_zipcode, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", arg_zipcode, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(arg_sex, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", arg_sex, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(arg_mail_addr, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", arg_mail_addr, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(arg_mail_hope, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", arg_mail_hope, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(arg_unload_bgp, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", arg_unload_bgp, value);
        append_string(args, value, maxlen);
    }
    found = GetArg(arg_unload_bgm, value, 64);
    if (found == 1)
    {
        sprintf(value, "%s=%s;", arg_unload_bgm, value);
        append_string(args, value, maxlen);
    }
}

#include "pfs.inc"

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
    soundGetInfo(5, info, 256); /* empty when no BGM is loaded; 0 to 2 are its bd, hd and sq files */
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

#include "title.inc"

/*
 * Opens the dialog of the "dialog" element dialog_id (its type, title, texts and items), with extra after the
 * texts, and waits for it; returns the button chosen, 1 when it was canceled. The pad help is restored.
 */
pop_dialog(focus_obj, dialog_id[], extra[])
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
    set_pad_help_labels2(buttons);
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
new g_focused;
new g_item_count;
new g_items[6];
new g_cursor; /* the item with the focus; argument "member_focus" first */
new g_title_ids[6][] = {
    "msged_feega_title_enter", "msged_feega_title_confirm_registration", "msged_feega_title_leave",
    "msged_feega_title_agreement", "msged_feega_title_idpasswd", "msged_feega_title_guide"
};
new g_link_ids[6][] = {
    "LINK_REGIST", "LINK_CHANGE", "LINK_QUIT", "LINK_AGREEMENT", "LINK_ID_SAVE",
    "LINK_EDCLIENT_GUIDE"
};
new g_args[256];
new g_move_tick; /* the tickcount of the last move: the focus moves at most every 70 ms */

main()
{
    init_menu();
    for (; ; )
    {
        update_menu();
        sleep;
    }
}

init_menu()
{
    new i;
    new Float:color[4];
    new text[64];
    new len;
    new Float:x;
    new Float:y;
    start_top_page(0);
    load_background("bg");
    play_bgm(0);
    load_se(0);
    g_focus = create_focus_plane2("focus");
    color[0] = 0.902;
    color[1] = 0.902;
    color[2] = 0.215;
    color[3] = 1.0;
    get_tag_attr("TEXT", "dummy_title", "value", text, 64);
    create_text_label(Disp2D(0), 320.0, 128.0, color, 15, text, 0, ALIGN_CENTER, ALIGN_CENTER, BLEND_ADD);
    x = 320.0;
    y = 224.0 - (16.0 + float(15));
    color[0] = 0.215;
    color[1] = 0.708;
    color[2] = 0.999;
    color[3] = 1.0;
    i = 0;
    for (; i < 6; i++)
    {
        len = get_tag_attr("TEXT", g_title_ids[i], "value", text, 64);
        if (0 == len)
        {
            break;
        }
        g_items[i] = create_text_label(Disp2D(0), x, y, color, 15, text, 0, ALIGN_CENTER, ALIGN_CENTER, BLEND_ADD);
        y = y + (16.0 + float(15));
        g_item_count++;
    }
    if (GetArg("member_focus", text, 64) == 1)
    {
        g_cursor = atoi(text);
    }
    else
    {
        pop_dialog(g_focus, "DIALOG_SSL", ""); /* entering the menu, not coming back to it */
    }
    g_focused = g_items[g_cursor];
    send_label_focus2(g_focus, g_focused, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    SetSystemParam(SYS_PADHELP, PADHELP_CIRCLE | PADHELP_CROSS);
    g_move_tick = tickcount();
}

update_menu()
{
    new msg;
    new a;
    new b;
    new c;
    new arg[64];
    while (RecvMessage(msg, a, b, c))
    {
    }
    new held;
    new pressed;
    new repeated;
    PadState(held, pressed, _, repeated, -1);
    if (pressed & PAD_CIRCLE)
    {
        play_se(ED_SE_DECIDE);
        g_args = "";
        append_member_args(g_args, 256);
        sprintf(arg, "%s=1;", g_arg_retflag); /* RETFLAG_LINK */
        append_string(g_args, arg, 256);
        sprintf(arg, "member_focus=%d;", g_cursor);
        append_string(g_args, arg, 256);
        load_page(g_link_ids[g_cursor], g_args, 1);
    }
    else if (pressed & PAD_CROSS)
    {
        play_se(ED_SE_CANCEL);
        stop_bgm();
        fade_ed_background(0.0, 600.0);
        load_page("LINK_TOP", _, 0);
    }
    else if (repeated & PAD_UP && tickcount() - g_move_tick > 70)
    {
        g_move_tick = tickcount();
        if (g_cursor > 0)
        {
            play_se(ED_SE_CURSOR);
            g_cursor = g_cursor > 0 ? --g_cursor : g_cursor; /* redundant as written */
            g_focused = g_items[g_cursor];
            send_label_focus2(g_focus, g_focused, 0, 0.5, 60.0, 60.0, float(70), 1, 1);
        }
    }
    else if (repeated & PAD_DOWN && tickcount() - g_move_tick > 70)
    {
        g_move_tick = tickcount();
        if (g_cursor < g_item_count - 1)
        {
            play_se(ED_SE_CURSOR);
            g_cursor = g_cursor < g_item_count - 1 ? ++g_cursor : g_cursor; /* redundant as written */
            g_focused = g_items[g_cursor];
            send_label_focus2(g_focus, g_focused, 0, 0.5, 60.0, 60.0, float(70), 1, 1);
        }
    }
}
