/*
 * /opt0/bn/script/game/game_dialog.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The dialogs of the game page, one per public function: delete a game ("url": its partition), show its title,
 * add it to the navigation menu.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

#include "gamedb.inc"

#include "tag.inc"

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

new g_db_name[] = "game";
public result; /* delete_main: 1 if the game was deleted; 0 otherwise */
public focus; /* the focus object */
new g_dialog_args[128];
new g_boot_link[128]; /* the href of the link navimenu_boot, the key of the shortcuts */

/* Asks to delete the game, then uninstalls it and removes its shortcut and its database row; not in album mode. */
public delete_main()
{
    new status;
    result = -1;
    new deleting_msg[32];
    new progress;
    new mode_arg[64];
    new album_mode;
    if (GetArg("album_mode", mode_arg, 64))
    {
        album_mode = atoi(mode_arg);
    }
    if (ALBUM_HDD != album_mode)
    {
        goto done;
    }
    new uri[64] = {0};
    GetArg("url", uri, 64);
    new message[64] = {0};
    new line1[16] = {0};
    new line2[16] = {0};
    new line3[16] = {0};
    new no_label[16] = {0};
    new title[64] = {0};
    GetArg("title", title, 64);
    if (strlen(title) <= 0)
    {
        get_tag_attr("TEXT", "msggame_del_ask_corrupted", "value", line1, 16);
    }
    else
    {
        err10030(uri, line1, line2, line3);
    }
    if (strlen(line1) > 0)
    {
        if (strlen(line2) > 0)
        {
            if (strlen(line3) > 0)
            {
                sprintf(message, "%s\n%s\n%s", line1, line2, line3);
            }
            else
            {
                sprintf(message, "%s\n%s", line1, line2);
            }
        }
        else
        {
            sprintf(message, "%s", line1);
        }
    }
    else
    {
        get_tag_attr("TEXT", "msggame_del_ask", "value", line1, 16);
        get_tag_attr("TEXT", "msggame_del_ask2", "value", line2, 16);
        get_tag_attr("TEXT", "msggame_del_ask3", "value", line3, 16);
        sprintf(message, "%s\n%s\n%s", line1, line2, line3);
    }
    /* line3 now holds the label of the yes button. */
    get_tag_attr("TEXT", "msggame_common_yes", "value", line3, 16);
    get_tag_attr("TEXT", "msggame_common_no", "value", no_label, 16);
    new answer = ask_yes_no(message, line3, no_label);
    if (0 != answer)
    {
        goto done;
    }
    SetSystemParam(SYS_PADHELP, 0);
    new progress_window = CreateNull(Disp2D(0));
    get_tag_attr("TEXT", "msggame_del", "value", deleting_msg, 32);
    sprintf(g_dialog_args, "msg=%s;cancel=%s", deleting_msg, "");
    SetPos(progress_window, 320.0, 224.0, 0.0, 0);
    AttachScript(progress_window, _, "progress_window", g_dialog_args);
    SetPubVar(progress_window, "manager", Self());
    SetPubVar(progress_window, "focus", focus);
    result = err10029(uri);
    progress = 0;
    for (; progress < 50; progress++)
    {
        SetPubVar(progress_window, "progress", progress);
        sleep;
    }
    if (0 == result)
    {
        get_tag_attr("LINK", "navimenu_boot", "href", g_boot_link, 128);
        sprintf(g_dialog_args, "uri=%s", uri);
        shortcutDel(0, g_boot_link, g_dialog_args);
        status = datOpen(DAT_DATABASE, g_db_name, DAT_ORDER_MTIME_DESC);
        if (0 != status)
        {
            goto db_done;
        }
        status = wait_database();
        if (status < 0)
        {
            goto db_done;
        }
        new sql[64];
        sprintf(sql, "delete from sce_game where uri = '%s'", uri);
        status = datSQLExec(sql);
        if (status < 0)
        {
            goto db_done;
        }
        status = wait_database();
        if (status < 0)
        {
            goto db_done;
        }
        status = datClose();
        status = 0;
        progress = 50;
        for (; progress <= 100; progress++)
        {
            SetPubVar(progress_window, "progress", progress);
            sleep;
        }
        db_done:
    }
    SetPubVar(progress_window, "progress", 100);
    while (SCRIPT_FINISHED != ScriptState(progress_window))
    {
        sleep;
    }
    if (progress_window)
    {
        Destroy(progress_window);
    }
    if (0 == result)
    {
        get_tag_attr("TEXT", "msggame_del_complete", "value", message, 64);
        show_message(message);
        goto done;
    }
    get_tag_attr("TEXT", "msggame_del_fail", "value", message, 64);
    show_message(message);
    done:
    if (0 == result)
    {
        result = 1;
    }
    else
    {
        result = 0;
    }
    SetPubVar(Self(), "result", result);
    return result;
}

/* Shows the title of the game in a window until the cross button is held. */
public information_main()
{
    result = 0;
    new title[64] = {0};
    GetArg("title", title, 64);
    new window = CreateNull(Disp2D(0));
    SetPos(window, 320.0, 224.0, 0.0, 0);
    SetOptionI(window, TAG("ZSRT"), 2); /* acts as ZSORT_NONE */
    AttachScript(window, _, "window", "");
    SetPubVar(window, "manager", Self());
    SetPubVar(window, "focus", focus);
    SetPubVar(window, "width", 500.0);
    SetPubVar(window, "height", 250.0);
    new label = CreateLabel(Disp2D(0));
    SetPos(label, 192.0, 160.0, 0.0, 0);
    SetColor(label, 1.0, 1.0, 1.0, 1.0);
    SetOptionI(label, TAG("FSIZ"), 20);
    SetOptionI(label, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(label, TAG("HALN"), ALIGN_LEFT);
    SetOptionS(label, TAG("UTF8"), title);
    new held;
    do
    {
        PadState(held, _, _, _, -1);
        sleep;
    }
    while (0 == held & PAD_CROSS);
    Destroy(label);
    Destroy(window);
    return result;
}

/* Adds a shortcut to the game to the navigation menu, which holds at most four; not in album mode. */
public add_navimenu_main()
{
    new status;
    new progress;
    new title[64];
    new uri[64];
    new text[64];
    new album_mode;
    text = "";
    if (GetArg("album_mode", text, 64))
    {
        album_mode = atoi(text);
    }
    if (ALBUM_HDD != album_mode)
    {
        goto done;
    }
    title = "";
    GetArg("title", title, 64);
    uri = "";
    GetArg("url", uri, 64);
    g_boot_link = "";
    get_tag_attr("LINK", "navimenu_boot", "href", g_boot_link, 128);
    sprintf(g_dialog_args, "uri=%s", uri);
    status = shortcutSet(0, title, g_boot_link, g_dialog_args);
    if (0 == status)
    {
        SetSystemParam(SYS_PADHELP, 0);
        new progress_window = CreateNull(Disp2D(0));
        text = "";
        get_tag_attr("TEXT", "msggame_navimenu_add", "value", text, 64);
        sprintf(g_dialog_args, "msg=%s;cancel=%s", text, "");
        SetPos(progress_window, 320.0, 224.0, 0.0, 0);
        AttachScript(progress_window, _, "progress_window", g_dialog_args);
        SetPubVar(progress_window, "manager", Self());
        SetPubVar(progress_window, "focus", focus);
        progress = 0;
        for (; progress <= 100; progress = progress + 3)
        {
            SetPubVar(progress_window, "progress", progress);
            sleep;
        }
        SetPubVar(progress_window, "progress", 100);
        while (SCRIPT_FINISHED != ScriptState(progress_window))
        {
            sleep;
        }
        if (progress_window)
        {
            Destroy(progress_window);
        }
        text = "";
        get_tag_attr("TEXT", "msggame_navimenu_add_complete", "value", text, 64);
    }
    else
    {
        text = "";
        get_tag_attr("TEXT", "msggame_navimenu_add_prohibit_cntover", "value", text, 64);
    }
    show_message(text);
    done:
    status = 0;
    SetPubVar(Self(), "result", status);
    return status;
}

/* Asks msg with the buttons yes and no, no selected first; returns 0 for yes, 1 for no or cross. */
ask_yes_no(msg[], yes[], no[])
{
    sprintf(g_dialog_args, "msg=%s;yes=%s;no=%s", msg, yes, no);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    AttachScript(dialog, _, "confirm_yesno", g_dialog_args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus);
    SetPubVar(dialog, "select", 1);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    new answer = GetPubVar(dialog, "select");
    if (answer == -1)
    {
        answer = 1;
    }
    Destroy(dialog);
    return answer;
}

/* Shows msg in a dialog and waits until it is closed. */
show_message(msg[])
{
    sprintf(g_dialog_args, "msg=%s", msg);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    AttachScript(dialog, _, "confirm_d", g_dialog_args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    Destroy(dialog);
    return;
}
