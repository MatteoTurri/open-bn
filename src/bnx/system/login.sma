/*
 * /opt0/bn/script/system/login.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The authentication dialog of a page that asks for a user id and a password: OK sets them, and whether to
 * save them, as the authentication of the system; retry shows that the last attempt failed first.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

#include "wait.inc"

#include "tag.inc"

#include "sound.inc"

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

#include "focus.inc"

new g_focus;
new g_active;
new g_defocus;
new g_window;
new g_title;
new g_id_label;
new g_pass_label;
new g_id_box;
new g_pass_box;
new g_save_box;

/* Copies an argument into dest decoding its escapes: %% for %, %s for ;, %e for = and %b for a space. */
unescape_arg(src[], dest[], maxlen)
{
    new src_pos;
    new dest_pos;
    for (; ; )
    {
        new found = find_substring(src, "%", src_pos);
        if (found < 0)
        {
            copy_substring(dest, dest_pos, src, src_pos, -1, maxlen);
            return;
        }
        {
            new ch[1];
            switch (src{found + 1})
            {
                case '%':
                {
                    ch = "%";
                }
                case 's':
                {
                    ch = ";";
                }
                case 'e':
                {
                    ch = "=";
                }
                case 'b':
                {
                    ch = " ";
                }
            }
            copy_substring(dest, dest_pos, src, src_pos, found - src_pos, maxlen);
            dest_pos = dest_pos + (found - src_pos);
            append_string(dest, ch, maxlen);
            dest_pos = dest_pos + 1;
            src_pos = found + 2;
        }
    }
}

main()
{
    g_focus = create_focus_plane2("focus");
    SetOptionI(g_focus, TAG("ZSRT"), ZSORT_OVERLAY);
    new arg[128];
    new user_id[64];
    new pass[64];
    new save;
    new retry;
    GetArg("id", arg, 128);
    unescape_arg(arg, user_id, 64);
    printf("pass : [%s] -> [%s] \n", arg, user_id);
    GetArg("pass", arg, 128);
    unescape_arg(arg, pass, 64);
    printf("pass : [%s] -> [%s] \n", arg, pass);
    save = 0 != user_id[0] && 0 != pass[0];
    GetArg("retry", arg, 128);
    retry = atoi(arg);
    printf("retry : %d\n", retry);
    if (retry)
    {
        new text[256];
        new msg[256];
        get_tag_attr("TEXT", "msgcom_error_certification_fail", "value", text, 256);
        replace_all(msg, text, "\\n", "\n", 256);
        show_msgbox(0, msg, "", "", 0);
    }
    g_defocus = CreateDefocus(Disp2D(0));
    SetPos(g_defocus, 320.0, 240.0, 0.0, 0);
    SetColor(g_defocus, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_defocus, TAG("LEVL"), 3);
    SetOptionI(g_defocus, TAG("ZSRT"), ZSORT_BACKGROUND);
    SetOptionF(g_defocus, TAG("ZOFS"), -10.0);
    g_window = CreateNull(g_defocus);
    SetPos(g_window, 0.0, -16.0, 0.0, 0);
    SetColor(g_window, 1.0, 1.0, 1.0, 0.0);
    SetOptionF(g_window, TAG("ZOFS"), -2.0);
    AttachScript(g_window, _, "window");
    SetPubVar(g_window, "manager", Self());
    SetPubVar(g_window, "focus", g_focus);
    SetPubVar(g_window, "width", 450.0);
    SetPubVar(g_window, "height", 260.0);
    {
        new title[64];
        get_tag_attr("TEXT", "text_title", "value", title, 64);
        g_title = CreateLabel(g_defocus);
        SetPos(g_title, 0.0, -90.0, 0.0, 0);
        SetColor(g_title, 0.7843, 0.7843, 0.7843, 0.0);
        SetOptionS(g_title, TAG("UTF8"), title);
        SetOptionI(g_title, TAG("HALN"), ALIGN_CENTER);
        SetOptionI(g_title, TAG("VALN"), ALIGN_CENTER);
        SetOptionI(g_title, TAG("FSIZ"), 15);
    }
    {
        new label[64];
        get_tag_attr("TEXT", "text_id", "value", label, 64);
        g_id_label = CreateLabel(g_defocus);
        SetPos(g_id_label, -50.0, -50.0, 0.0, 0);
        SetColor(g_id_label, 0.7843, 0.7843, 0.7843, 1.0);
        SetOptionS(g_id_label, TAG("UTF8"), label);
        SetOptionI(g_id_label, TAG("HALN"), ALIGN_RIGHT);
        SetOptionI(g_id_label, TAG("VALN"), ALIGN_CENTER);
        SetOptionI(g_id_label, TAG("FSIZ"), 15);
    }
    {
        new label[64];
        get_tag_attr("TEXT", "text_pass", "value", label, 64);
        g_pass_label = CreateLabel(g_defocus);
        SetPos(g_pass_label, -50.0, -10.0, 0.0, 0);
        SetColor(g_pass_label, 0.7843, 0.7843, 0.7843, 1.0);
        SetOptionS(g_pass_label, TAG("UTF8"), label);
        SetOptionI(g_pass_label, TAG("HALN"), ALIGN_RIGHT);
        SetOptionI(g_pass_label, TAG("VALN"), ALIGN_CENTER);
        SetOptionI(g_pass_label, TAG("FSIZ"), 15);
    }
    g_id_box = CreateNull(g_defocus);
    SetPos(g_id_box, -20.0, -50.0, 0.0, 0);
    AttachScript(g_id_box, _, "edit_box");
    SetPubVar(g_id_box, "manager", Self());
    SetPubVar(g_id_box, "focus", g_focus);
    SetPubVar(g_id_box, "width", 240.0);
    SetPubVar(g_id_box, "mode", EDIT_MODE_USER_ID);
    SendMessageStr(g_id_box, TAG("TEXT"), user_id);
    g_pass_box = CreateNull(g_defocus);
    SetPos(g_pass_box, -20.0, -10.0, 0.0, 0);
    AttachScript(g_pass_box, _, "edit_box");
    SetPubVar(g_pass_box, "manager", Self());
    SetPubVar(g_pass_box, "focus", g_focus);
    SetPubVar(g_pass_box, "width", 240.0);
    SetPubVar(g_pass_box, "mode", EDIT_MODE_PASSWORD);
    SendMessageStr(g_pass_box, TAG("TEXT"), pass);
    g_save_box = CreateNull(g_defocus);
    SetPos(g_save_box, 5.0, 30.0, 0.0, 0);
    {
        new label[64];
        get_tag_attr("TEXT", "text_save", "value", label, 64);
        new args[75];
        sprintf(args, "text=%s", label);
        AttachScript(g_save_box, _, "check_box", args);
        SetPubVar(g_save_box, "manager", Self());
        SetPubVar(g_save_box, "focus", g_focus);
        SetPubVar(g_save_box, "checked", save);
    }
    new ok_button;
    {
        new label[64];
        get_tag_attr("TEXT", "megcom_common_ok", "value", label, 64);
        new args[75];
        sprintf(args, "text=%s", label);
        ok_button = CreateNull(g_defocus);
        SetPos(ok_button, -64.0, 65.0, 0.0, 0);
        AttachScript(ok_button, _, "button", args);
        SetPubVar(ok_button, "manager", Self());
        SetPubVar(ok_button, "focus", g_focus);
        SetPubVar(ok_button, "margin_x", 10.0);
        SetPubVar(ok_button, "margin_y", 10.0);
    }
    new cancel_button;
    {
        new label[64];
        get_tag_attr("TEXT", "megcom_common_cancel", "value", label, 64);
        new args[75];
        sprintf(args, "text=%s", label);
        cancel_button = CreateNull(g_defocus);
        SetPos(cancel_button, 64.0, 65.0, 0.0, 0);
        AttachScript(cancel_button, _, "button", args);
        SetPubVar(cancel_button, "manager", Self());
        SetPubVar(cancel_button, "focus", g_focus);
        SetPubVar(cancel_button, "margin_x", 10.0);
        SetPubVar(cancel_button, "margin_y", 10.0);
    }
    g_active = g_id_box;
    fade_dialog(1.0, 200.0);
    wait_ms(200);
    SendMessage(g_active, TAG("FOCS"), TAG("FRST"), 0, 0, 0);
    for (; ; )
    {
        new msg;
        new a;
        new b;
        new c;
        /* "FOCS" (direction): the cursor leaves the active part, which moves the focus; "PUSH" (index, button) */
        while (RecvMessage(msg, a, b, c))
        {
            if (msg == TAG("FOCS"))
            {
                if (g_active == g_id_box)
                {
                    if (a == TAG("LEFT"))
                    {
                        SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                    }
                    else if (a == TAG("RIGT"))
                    {
                        SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                    }
                    else if (a == TAG("UP"))
                    {
                        SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                    }
                    else if (a == TAG("DOWN"))
                    {
                        g_active = g_pass_box;
                        SendMessage(g_active, TAG("FOCS"), TAG("UP"), 0, 0, 0);
                    }
                }
                else if (g_active == g_pass_box)
                {
                    if (a == TAG("LEFT"))
                    {
                        SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                    }
                    else if (a == TAG("RIGT"))
                    {
                        SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                    }
                    else if (a == TAG("UP"))
                    {
                        g_active = g_id_box;
                        SendMessage(g_active, TAG("FOCS"), TAG("DOWN"), 0, 0, 0);
                    }
                    else if (a == TAG("DOWN"))
                    {
                        g_active = g_save_box;
                        SendMessage(g_active, TAG("FOCS"), TAG("UP"), 0, 0, 0);
                    }
                }
                else if (g_active == g_save_box)
                {
                    if (a == TAG("LEFT"))
                    {
                        SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                    }
                    else if (a == TAG("RIGT"))
                    {
                        SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                    }
                    else if (a == TAG("UP"))
                    {
                        g_active = g_pass_box;
                        SendMessage(g_active, TAG("FOCS"), TAG("DOWN"), 0, 0, 0);
                    }
                    else if (a == TAG("DOWN"))
                    {
                        g_active = ok_button;
                        SendMessage(g_active, TAG("FOCS"), TAG("UP"), 0, 0, 0);
                    }
                }
                else if (g_active == ok_button)
                {
                    if (a == TAG("LEFT"))
                    {
                        SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                    }
                    else if (a == TAG("RIGT"))
                    {
                        g_active = cancel_button;
                        SendMessage(g_active, TAG("FOCS"), TAG("LEFT"), 0, 0, 0);
                    }
                    else if (a == TAG("UP"))
                    {
                        g_active = g_save_box;
                        SendMessage(g_active, TAG("FOCS"), TAG("DOWN"), 0, 0, 0);
                    }
                    else if (a == TAG("DOWN"))
                    {
                        SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                    }
                }
                else if (g_active == cancel_button)
                {
                    if (a == TAG("LEFT"))
                    {
                        g_active = ok_button;
                        SendMessage(g_active, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
                    }
                    else if (a == TAG("RIGT"))
                    {
                        SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                    }
                    else if (a == TAG("UP"))
                    {
                        g_active = g_save_box;
                        SendMessage(g_active, TAG("FOCS"), TAG("DOWN"), 0, 0, 0);
                    }
                    else if (a == TAG("DOWN"))
                    {
                        SendMessage(g_active, TAG("FOCS"), TAG("RTRN"), 0, 0, 0);
                    }
                }
            }
            else if (msg == TAG("PUSH"))
            {
                if (b == TAG("RRGT"))
                {
                    if (g_active == ok_button)
                    {
                        switch (a)
                        {
                            case 0:
                            {
                                close_login(1);
                            }
                        }
                    }
                    else if (g_active == cancel_button)
                    {
                        switch (a)
                        {
                            case 0:
                            {
                                close_login(0);
                            }
                        }
                    }
                }
                SendMessage(g_active, TAG("PUSH"), 0, 0, 0, 0);
            }
            else if (msg == TAG("CHNG"))
            {
                SendMessage(g_active, TAG("CHNG"), 0, 0, 0, 0);
            }
        }
        sleep;
    }
    return;
}

/* Closes the dialog, setting the authentication from the boxes if ok. */
close_login(ok)
{
    SetSystemParam(SYS_AUTH_OK, ok);
    if (ok)
    {
        new user_id[256];
        SendMessage(g_id_box, TAG("GTXT"), 0, 0, 0, 0);
        wait_message_str(TAG("GTXT"), user_id, 256);
        new pass[256];
        SendMessage(g_pass_box, TAG("GTXT"), 0, 0, 0, 0);
        wait_message_str(TAG("GTXT"), pass, 256);
        new save = GetPubVar(g_save_box, "checked");
        printf("id : %s  pass: %s \n", user_id, pass);
        SetSystemParamStr(SYS_STR_AUTH_ID, user_id);
        SetSystemParamStr(SYS_STR_AUTH_PASS, pass);
        SetSystemParam(SYS_AUTH_SAVE, save);
    }
    fade_dialog(0.0, 200.0);
    wait_ms(200);
    Destroy(Scene(0));
}

fade_dialog(Float:alpha, Float:duration)
{
    AnimColor(g_defocus, 0.35, 0.35, 0.35, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_window, 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_title, 0.7843, 0.7843, 0.7843, 1.0 * alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_id_label, 0.7843, 0.7843, 0.7843, 1.0 * alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_pass_label, 0.7843, 0.7843, 0.7843, 1.0 * alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    SendMessage(g_id_box, TAG("FADE"), alpha, duration, 0, 0);
    SendMessage(g_pass_box, TAG("FADE"), alpha, duration, 0, 0);
}
