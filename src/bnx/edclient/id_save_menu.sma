/*
 * /opt0/bn/script/edclient/id_save_menu.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The menu of the saved ID and password: save a new one, change it or delete it. Each opens its page with the
 * member arguments; cross goes back to the top page.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

#include "tag.inc"

#include "edpage.inc"

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

#include "pfs.inc"

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

#include "background.inc"

new g_focus;
new g_focused;
new g_item_count;
new g_items[3];
new g_cursor; /* the item with the focus; argument "id_save_focus" first */
new g_title_ids[3][7] = {"msged_savecom_newsave_title", "msged_savecom_change_title", "msged_savecom_delete_title"};
new g_link_ids[3][] = {"LINK_ID_NEW", "LINK_ID_CHANGE", "LINK_ID_DELETE"};
new g_args[128];
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
    g_focus = create_focus_plane2("focus");
    color[0] = 0.902;
    color[1] = 0.902;
    color[2] = 0.215;
    color[3] = 1.0;
    get_tag_attr("TEXT", "msged_feega_title_idpasswd", "value", text, 64);
    create_text_label(Disp2D(0), 320.0, 128.0, color, 15, text, 0, ALIGN_CENTER, ALIGN_CENTER, BLEND_ADD);
    x = 320.0;
    y = 224.0;
    color[0] = 0.215;
    color[1] = 0.708;
    color[2] = 0.999;
    color[3] = 1.0;
    i = 0;
    for (; i < 3; i++)
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
    if (GetArg("id_save_focus", text, 64) == 1)
    {
        g_cursor = atoi(text);
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
        append_member_args(g_args, 128);
        sprintf(arg, "%s=1;", g_arg_retflag); /* RETFLAG_LINK */
        append_string(g_args, arg, 128);
        append_focus_arg(g_args, 128);
        sprintf(arg, "id_save_focus=%d;", g_cursor);
        append_string(g_args, arg, 128);
        load_page(g_link_ids[g_cursor], g_args, 1);
    }
    else if (pressed & PAD_CROSS)
    {
        play_se(ED_SE_CANCEL);
        g_args = "";
        append_member_args(g_args, 128);
        append_focus_arg(g_args, 128);
        load_page("LINK_TOP", g_args, 1);
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
