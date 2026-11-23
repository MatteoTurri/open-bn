/*
 * /opt0/bn/script/system/navimenu/navimenu.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The navigation menu: the globe and a list of the home page, the mailer, the shortcuts and the about
 * page. Circle opens an item, triangle on a shortcut offers to delete it, cross or select closes the menu.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "wait.inc"

#include "tag.inc"

#include <float>

#include "imgsync.inc"

/* Returns whether str is a packed string. */
is_packed(str[])
{
    return str[0] <= 0 || str[0] > 255;
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

#include "misc.inc"

#include "focus.inc"

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

new Float:g_screen_w;
new Float:g_screen_h;

/* The items of the list: home, mailer, the shortcuts, about. */
get_first_shortcut()
{
    return 2;
}

get_last_shortcut()
{
    return shortcutNum(0) + 1;
}

get_about_item()
{
    return shortcutNum(0) + 1 + 1;
}

/* Fades the menu out and destroys the focus and the list. */
close_menu(focus, list, defocus, earth, title)
{
    send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    TimerDie(focus, ms_to_frames(300));
    SendMessage(list, TAG("FADE"), 0, float(300), 0, 0);
    TimerDie(list, ms_to_frames(300));
    AnimColor(defocus, 1.0, 1.0, 1.0, 0.0, float(300), 0.0, ANIM_FUNC_LINEAR);
    SendMessage(earth, TAG("FADE"), 0, float(300), 0, 0);
    AnimColor(title, 1.0, 1.0, 1.0, 0.0, float(300), 0.0, ANIM_FUNC_LINEAR);
    wait_ms(300);
}

fade_out_menu(list, earth, title)
{
    SendMessage(list, TAG("FADE"), 0, float(300), 0, 0);
    SendMessage(earth, TAG("FADE"), 0, float(300), 0, 0);
    AnimColor(title, 1.0, 1.0, 1.0, 0.0, float(300), 0.0, ANIM_FUNC_LINEAR);
}

fade_in_menu(list, earth, title)
{
    SendMessage(list, TAG("FADE"), 1.0, float(300), 0, 0);
    SendMessage(earth, TAG("FADE"), 1.0, float(300), 0, 0);
    AnimColor(title, 1.0, 1.0, 1.0, 1.0, float(300), 0.0, ANIM_FUNC_LINEAR);
}

main()
{
    SetSystemParam(SYS_FG_ALPHA, 255);
    SetSystemParam(SYS_FGBAR_ALPHA, 100);
    get_screen_size2(g_screen_w, g_screen_h);
    sndSePlay(SE_DIALOG_OPEN);
    new focus = create_focus_plane2("focus");
    SetOptionF(focus, TAG("ZOFS"), -1.0);
    new defocus;
    defocus = CreateDefocus(Disp2D(0));
    SetPos(defocus, 0.5 * g_screen_w, 0.5 * g_screen_h, 0.0, 0);
    SetColor(defocus, 1.0, 1.0, 1.0, 0.0);
    AnimColor(defocus, 0.5, 0.5, 0.5, 1.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    SetOptionI(defocus, TAG("ZSRT"), ZSORT_BACKGROUND);
    SetOptionI(defocus, TAG("LEVL"), 3);
    new earth;
    earth = CreateNull(Disp2D(0));
    SetPos(earth, 0.3 * g_screen_w, 0.5 * g_screen_h, 90.0, 0);
    SetScale(earth, 140.0, 140.0, 140.0);
    AttachScript(earth, _, "earth");
    new title;
    {
        new url[256];
        get_tag_attr("IMG", "title_tex", "src", url, 256);
        title = CreatePlane(Disp2D(0));
        SetPos(title, 0.3 * g_screen_w, 0.5 * g_screen_h, 90.0, 0);
        SetColor(title, 1.0, 1.0, 1.0, 0.0);
        SetTexture(title, decode_texture(url), 0, -1);
        AnimColor(title, 1.0, 1.0, 1.0, 1.0, float(300), 0.0, ANIM_FUNC_LINEAR);
        SetLocalScale(title, 0.8 * float(imgTexWidth()), 0.8 * float(imgTexHeight()), 1.0);
        SetOptionI(title, TAG("ALPH"), BLEND_ADD);
    }
    new list = create_menu_list(defocus, focus);
    SendMessage(list, TAG("FADE"), 1.0, float(300), 0, 0);
    new active = list;
    SendMessage(active, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
    for (; ; )
    {
        new msg;
        new a;
        new b;
        new c;
        /* "PUSH" (item, button) from the list; button: "RRGT" circle, "RUP" triangle, "RDWN" cross or "SLCT" */
        while (RecvMessage(msg, a, b, c))
        {
            if (active == list)
            {
                if (msg == TAG("FOCS"))
                {
                    SendMessage(active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                }
                else if (msg == TAG("CHNG"))
                {
                    SendMessage(active, TAG("CHNG"), 0, 0, 0, 0);
                }
                else if (msg == TAG("PUSH"))
                {
                    if (b == TAG("RRGT"))
                    {
                        if (0 == a)
                        {
                            SetSystemParam(SYS_PADHELP, 0);
                            close_menu(focus, list, defocus, earth, title);
                            hisClear();
                            SystemCommand(SYSCMD_FADEOUT_MAIN);
                            if (!is_common_bg())
                            {
                                SystemCommand(SYSCMD_FADEOUT_BG);
                            }
                            if (GetSystemParam(SYS_XML_BUSY))
                            {
                                xmlLoadAbortAll();
                            }
                            else
                            {
                                new url[256];
                                get_tag_attr("LINK", "home", "href", url, 256);
                                new args[64];
                                {
                                    new menu_id[32];
                                    get_tag_attr("TEXT", "top_menu_id", "value", menu_id, 32);
                                    sprintf(args, "menu_id=%s", menu_id);
                                }
                                xmlLoadStart(url, _, args, XMLLOAD_MAIN);
                                wait_xml_load();
                            }
                            Destroy(Scene(0));
                            return;
                        }
                        if (a == 1)
                        {
                            SetSystemParam(SYS_PADHELP, 0);
                            close_menu(focus, list, defocus, earth, title);
                            new url[256];
                            get_tag_attr("LINK", "mailer", "href", url, 256);
                            xmlLoadSetOptionI(10100, 1);
                            xmlLoadStart(url, _, _, XMLLOAD_MODAL);
                            wait_xml_load();
                            Destroy(Scene(0));
                            return;
                        }
                        if (get_first_shortcut() <= a <= get_last_shortcut())
                        {
                            fade_out_menu(list, earth, title);
                            SendMessage(active, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                            SetSystemParam(SYS_PADHELP, 0);
                            wait_ms(300);
                            hisClear();
                            {
                                new url[256];
                                shortcutGet(0, a - get_first_shortcut(), 1, url, 256);
                                new args[256];
                                shortcutGet(0, a - get_first_shortcut(), 2, args, 256);
                                static target[1] = {XMLLOAD_MODAL};
                                xmlLoadStart(url, _, args, target[0]);
                                wait_xml_load();
                            }
                            active = list;
                            SendMessage(active, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
                            fade_in_menu(list, earth, title);
                            wait_ms(300);
                        }
                        else if (a == get_about_item())
                        {
                            SetSystemParam(SYS_PADHELP, 0);
                            close_menu(focus, list, defocus, earth, title);
                            new url[256];
                            get_tag_attr("LINK", "about", "href", url, 256);
                            xmlLoadStart(url, _, _, XMLLOAD_MODAL);
                            wait_xml_load();
                            Destroy(Scene(0));
                            return;
                        }
                    }
                    else if (b == TAG("RUP"))
                    {
                        if (get_first_shortcut() <= a <= get_last_shortcut())
                        {
                            SendMessage(active, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
                            fade_out_menu(list, earth, title);
                            new label[256];
                            shortcutGet(0, a - get_first_shortcut(), 0, label, 256);
                            new alert = GetSystemParam(SYS_ALERT_ILLEGAL_DISC);
                            if (alert)
                            {
                                SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
                            }
                            if (0 == run_option_menu(defocus, focus, label))
                            {
                                new confirmed;
                                {
                                    new text[256];
                                    new question[256];
                                    new yes[10];
                                    new no[10];
                                    get_tag_attr("TEXT", "msgcom_navi_delete_ask", "value", question, 256);
                                    get_tag_attr("TEXT", "megcom_common_yes", "value", yes, 10);
                                    get_tag_attr("TEXT", "megcom_common_no", "value", no, 10);
                                    replace_all(text, question, "\\n", "\n", 256);
                                    question = text;
                                    confirmed = show_msgbox(2, question, yes, no, 0) == 1;
                                }
                                if (confirmed)
                                {
                                    {
                                        new key[256];
                                        new url[256];
                                        shortcutGet(0, a - get_first_shortcut(), 1, key, 256);
                                        shortcutGet(0, a - get_first_shortcut(), 2, url, 256);
                                        shortcutDel(0, key, url);
                                    }
                                    Destroy(list);
                                    list = 0;
                                    list = create_menu_list(defocus, focus);
                                }
                            }
                            if (alert)
                            {
                                SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 1);
                            }
                            active = list;
                            SendMessage(active, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
                            fade_in_menu(list, earth, title);
                        }
                    }
                    else if (b == TAG("RDWN") || b == TAG("SLCT"))
                    {
                        sndSePlay(SE_CANCEL);
                        send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                        TimerDie(focus, ms_to_frames(300));
                        SendMessage(list, TAG("FADE"), 0, float(300), 0, 0);
                        TimerDie(list, ms_to_frames(300));
                        AnimColor(defocus, 1.0, 1.0, 1.0, 0.0, float(300), 0.0, ANIM_FUNC_LINEAR);
                        SendMessage(earth, TAG("FADE"), 0, float(300), 0, 0);
                        AnimColor(title, 1.0, 1.0, 1.0, 0.0, float(300), 0.0, ANIM_FUNC_LINEAR);
                        wait_ms(300);
                        Destroy(Scene(0));
                        return;
                    }
                    SendMessage(active, TAG("PUSH"), 0, 0, 0, 0);
                }
            }
        }
        sleep;
    }
}

/* Creates the list of the menu; while a page is loading, only the home item is enabled. */
create_menu_list(defocus, focus)
{
    new list = CreateNull(defocus);
    new args[512];
    new count;
    {
        new i;
        new node = domFindTag(domRoot(), "LIST", "navi_menu", 0);
        node = domFirstChild(node);
        i = 0;
        for (; i < 2; i++)
        {
            new arg[64];
            new label[64];
            domGetData(node, "value", label, 64);
            sprintf(arg, "btn%d=%s;", i, label);
            append_string(args, arg, 512);
            node = domNext(node);
        }
        for (new sc; sc < shortcutNum(0); sc++, i++)
        {
            new arg[64];
            new label[64];
            shortcutGet(0, sc, 0, label, 64);
            sprintf(arg, "btn%d=%s;sc%d;", i, label, i);
            append_string(args, arg, 512);
        }
        {
            new arg[64];
            new label[64];
            domGetData(node, "value", label, 64);
            sprintf(arg, "btn%d=%s;", i, label);
            append_string(args, arg, 512);
            node = domNext(node);
        }
        count = i;
    }
    {
        new arg[64];
        sprintf(arg, "interval=%f;", float(30));
        append_string(args, arg, 512);
        sprintf(arg, "fontsize=%d;", 15);
        append_string(args, arg, 512);
        sprintf(arg, "visible=%d;", 9);
        append_string(args, arg, 512);
    }
    SetPos(list, 130.0, -60.0 - (count - 4) * 30 / 2.0, 0.0, 0);
    SetOptionI(list, TAG("ZSRT"), ZSORT_OVERLAY);
    AttachScript(list, _, "list", args);
    SetPubVar(list, "manager", Self());
    SetPubVar(list, "focus", focus);
    SetPubVar(list, "padhelp", PADHELP_CROSS);
    if (GetSystemParam(SYS_XML_BUSY))
    {
        SendMessage(list, TAG("ENBL"), 1, 0, 0, 0);
        SendMessage(list, TAG("ENBL"), get_about_item(), 0, 0, 0);
        for (new i = get_first_shortcut(); i <= get_last_shortcut(); i++)
        {
            SendMessage(list, TAG("ENBL"), i, 0, 0, 0);
        }
        SendMessage(list, TAG("ENBL"), get_about_item(), 0, 0, 0);
    }
    return list;
}

decode_texture(url[])
{
    imgDecStart(url, 0, 0, -1, 100, 0);
    if (IMG_DONE != wait_image_decode())
    {
        return 0;
    }
    return imgTexID();
}

/* Shows the option menu of the shortcut label; returns the chosen item, -1 on cross. */
run_option_menu(defocus, focus, label[])
{
    sndSePlay(SE_OPEN);
    sndSePlay(SE_WINDOW_OPEN);
    new title = CreateLabel(defocus);
    {
        new text[20];
        get_tag_attr("TEXT", "msgcom_navi_optionmenu_title", "value", text, 20);
        SetPos(title, 130.0, -80.0, 0.0, 0);
        SetColor(title, 0.902, 0.902, 0.215, 0.0);
        SetOptionI(title, TAG("ZSRT"), ZSORT_OVERLAY);
        SetOptionS(title, TAG("UTF8"), text);
        SetOptionI(title, TAG("FSIZ"), 17);
        SetOptionI(title, TAG("HALN"), ALIGN_CENTER);
        SetOptionI(title, TAG("VALN"), ALIGN_CENTER);
    }
    new name = CreateLabel(defocus);
    SetPos(name, -270.0, -15.0, 0.0, 0);
    SetColor(name, 0.7843, 0.7843, 0.7843, 0.0);
    SetOptionI(name, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionS(name, TAG("UTF8"), label);
    SetOptionI(name, TAG("FSIZ"), 15);
    SetOptionI(name, TAG("HALN"), ALIGN_LEFT);
    SetOptionI(name, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(name, TAG("W64 "), 330 * 64);
    new category = CreateLabel(defocus);
    SetPos(category, -240.0, 15.0, 0.0, 0);
    SetColor(category, 0.7843, 0.7843, 0.7843, 0.0);
    SetOptionI(category, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(category, TAG("FSIZ"), 15);
    SetOptionI(category, TAG("HALN"), ALIGN_LEFT);
    SetOptionI(category, TAG("VALN"), ALIGN_CENTER);
    {
        new node = domFindTag(domRoot(), "LIST", "category", 0);
        node = domFirstChild(node);
        new text[20];
        domGetData(node, "value", text, 20);
        SetOptionS(category, TAG("UTF8"), text);
    }
    new list = CreateNull(defocus);
    {
        new args[256];
        {
            new node = domFindTag(domRoot(), "LIST", "option_menu", 0);
            node = domFirstChild(node);
            for (new i; i < 1; i++)
            {
                new arg[64];
                new item_label[64];
                domGetData(node, "value", item_label, 64);
                sprintf(arg, "btn%d=%s;", i, item_label);
                append_string(args, arg, 256);
                node = domNext(node);
            }
        }
        {
            new arg[64];
            sprintf(arg, "interval=%f;", float(30));
            append_string(args, arg, 256);
            sprintf(arg, "fontsize=%d;", 15);
            append_string(args, arg, 256);
            sprintf(arg, "visible=%d;", 9);
            append_string(args, arg, 256);
        }
        SetPos(list, 130.0, -35.0, 0.0, 0);
        SetOptionI(list, TAG("ZSRT"), ZSORT_OVERLAY);
        AttachScript(list, _, "list", args);
        SetPubVar(list, "manager", Self());
        SetPubVar(list, "focus", focus);
        SetPubVar(list, "padhelp", PADHELP_CROSS);
    }
    SendMessage(list, TAG("FADE"), 1.0, float(300), 0, 0);
    AnimColor(title, 0.902, 0.902, 0.215, 1.0, float(300), 0.0, ANIM_FUNC_LINEAR);
    AnimColor(name, 0.7843, 0.7843, 0.7843, 1.0, float(300), 0.0, ANIM_FUNC_LINEAR);
    AnimColor(category, 0.7843, 0.7843, 0.7843, 1.0, float(300), 0.0, ANIM_FUNC_LINEAR);
    new active = list;
    SendMessage(active, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
    new result;
    for (; ; )
    {
        new msg;
        new a;
        new b;
        new c;
        /* "PUSH" (item, button) from the list; button: "RRGT" circle, "RUP" triangle, "RDWN" cross or "SLCT" */
        while (RecvMessage(msg, a, b, c))
        {
            if (active == list)
            {
                if (msg == TAG("FOCS"))
                {
                    SendMessage(active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                }
                else if (msg == TAG("CHNG"))
                {
                    SendMessage(active, TAG("CHNG"), 0, 0, 0, 0);
                }
                else if (msg == TAG("PUSH"))
                {
                    if (b == TAG("RRGT"))
                    {
                        result = c; /* always 0, as the list sends it: the menu has a single item */
                        goto done;
                    }
                    if (b == TAG("RDWN"))
                    {
                        sndSePlay(SE_CANCEL);
                        result = -1;
                        goto done;
                    }
                    SendMessage(active, TAG("PUSH"), 0, 0, 0, 0);
                }
            }
        }
        sleep;
    }
    done:
    SendMessage(active, TAG("FOCS"), TAG("FOUT"), 0, 0, 0);
    SendMessage(list, TAG("FADE"), 0, float(300), 0, 0);
    TimerDie(list, ms_to_frames(300));
    AnimColor(title, 0.902, 0.902, 0.215, 0.0, float(300), 0.0, ANIM_FUNC_LINEAR);
    TimerDie(title, ms_to_frames(300));
    AnimColor(name, 0.7843, 0.7843, 0.7843, 0.0, float(300), 0.0, ANIM_FUNC_LINEAR);
    TimerDie(name, ms_to_frames(300));
    AnimColor(category, 0.7843, 0.7843, 0.7843, 0.0, float(300), 0.0, ANIM_FUNC_LINEAR);
    wait_ms(300);
    return result;
}

/* Returns whether the background window shows the common background. */
is_common_bg()
{
    new bg_name[4] = "SCEI:CommonBG";
    new name[256];
    return wndGetName(WND_BG, name, 256) && 0 == strcmp(name, bg_name);
}
