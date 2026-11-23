/*
 * /opt0/bn/script/music/music_import.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * Copies the music downloaded to file://net/download into pfs://__contents/contents on the hard disk, after asking;
 * sql_process, on an object of its own, adds a row of the contents table for each file copied.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1500

/* The fields of datGetData for a file found by datFind. */
const FILE_COL_NAME = 1;
const FILE_COL_PATH = 2;

#include "player.inc"

#include "tag.inc"

#include "sound.inc"

/* Same as get_tag_attr2 of text.inc. */
get_tag_attr2(tag[], id[], attr[], value[], maxlen)
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

public focus_glowfly; /* the focus object */

/* Shows msg in a dialog with an ok button labelled ok and waits until it is closed. */
show_message(msg[], ok[])
{
    new args[128];
    sprintf(args, "msg=%s;ok=%s", msg, ok);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    AttachScript(dialog, _, "confirm_ok", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus_glowfly);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    Destroy(dialog);
    return;
}

/* Asks msg with a yes/no dialog; returns its select, 0 for yes (1 when it is closed without an answer). */
ask_yes_no(msg[], yes[], no[])
{
    new args[512];
    sprintf(args, "msg=%s;yes=%s;no=%s", msg, yes, no);
    new dialog = CreateNull(Disp2D(0));
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    AttachScript(dialog, _, "confirm_yesno", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus_glowfly);
    while (SCRIPT_FINISHED != ScriptState(dialog))
    {
        sleep;
    }
    new answer = GetPubVar(dialog, "select");
    Destroy(dialog);
    if (answer == -1)
    {
        answer = 1;
    }
    return answer;
}

/*
 * Waits for "TITL" (a name prefix, not used), then adds a row of the contents table for each "FILE" (name) and "URL"
 * (path) that import_process sends, until "FNSH".
 */
public sql_process()
{
    new msg;
    new a;
    new title[256];
    new status;
    status = datOpen(99, "", DAT_ORDER_MTIME_DESC); /* a database type of its own, with no name */
    if (0 != status)
    {
        return -1;
    }
    status = wait_database();
    if (status < 0)
    {
        return -1;
    }
    wait_message_str(TAG("TITL"), title, 256);
    for (; ; )
    {
        new file_name[128];
        new url[128];
        if (FindMessageStr(TAG("FILE"), file_name, 128))
        {
            if (FindMessageStr(TAG("URL"), url, 128))
            {
                new sql[256];
                sprintf(sql, "insert into contents values('%s','%s',257,0)", file_name, url);
                status = datSQLExec(sql);
                status = wait_database();
                if (status < 0)
                {
                    return -1;
                }
            }
        }
        if (RecvMessage(msg, a))
        {
            if (msg == TAG("FNSH"))
            {
                break;
            }
        }
        sleep;
    }
    status = wait_database();
    if (status < 0)
    {
        return -1;
    }
    datClose();
    return 0;
}

/*
 * Copies the files, showing a progress_window; returns 0, or a negative step: -1 to -5 the database (-5: nothing to
 * copy), -6 the copy refused, -7 the copies did not all complete, -8 a copy reported an error.
 */
public import_process()
{
    new status = datOpen(DAT_FILE_LIST, "", DAT_ORDER_MTIME_DESC);
    if (0 != status)
    {
        return -1;
    }
    status = wait_database();
    if (status < 0)
    {
        return -2;
    }
    status = datFind("music", "file://net/download");
    if (0 != status)
    {
        return -3;
    }
    status = wait_database();
    if (status < 0)
    {
        return -4;
    }
    new count = datGetNum();
    if (count <= 0)
    {
        datClose();
        return -5;
    }
    new text[256];
    {
        new yes[32];
        new no[32];
        get_tag_attr2("TEXT", "msgmusic_hddimport_copy_found1", "value", text, 256);
        get_tag_attr2("TEXT", "msgmusic_common_yes", "value", yes, 32);
        get_tag_attr2("TEXT", "msgmusic_common_no", "value", no, 32);
        new answer = ask_yes_no(text, yes, no);
        if (0 != answer)
        {
            return -6;
        }
    }
    new progress_window = CreateNull(Disp2D(0));
    {
        new cancel[8];
        get_tag_attr2("TEXT", "msgmusic_hddimport_copy", "value", text, 256);
        get_tag_attr2("TEXT", "msgmusic_common_cancel", "value", cancel, 8);
        new args[128];
        sprintf(args, "msg=%s;cancel=%s", text, cancel);
        SetPos(progress_window, 320.0, 224.0, 0.0, 0);
        AttachScript(progress_window, _, "progress_window", args);
        SetPubVar(progress_window, "manager", Self());
        SetPubVar(progress_window, "focus", focus_glowfly);
    }
    new sql_obj = CreateNull(Scene(0));
    AttachScript(sql_obj, "sql_process", "", "");
    new prefix[256];
    sprintf(prefix, "Download");
    SendMessageStr(sql_obj, TAG("TITL"), prefix);
    new copy_status;
    new percent;
    {
        new i;
        for (; i < count; i++)
        {
            new dest_path[128];
            new src_path[128];
            new file_name[128];
            datGetData(i, FILE_COL_PATH, src_path, 128);
            datGetData(i, FILE_COL_NAME, file_name, 128);
            sprintf(dest_path, "pfs://__contents/contents/%s_%s", prefix, file_name);
            status = fileCopy(src_path, dest_path);
            if (status < 0)
            {
                break;
            }
            if (ScriptState(sql_obj) == SCRIPT_FINISHED)
            {
                status = -1;
                break;
            }
            SendMessageStr(sql_obj, TAG("FILE"), file_name);
            SendMessageStr(sql_obj, TAG("URL"), dest_path);
            for (; ; )
            {
                status = fileSync(percent, copy_status);
                if (status == FILESYNC_DONE)
                {
                    break;
                }
                else if (0 != status)
                {
                    goto finish;
                }
                if (0 != copy_status)
                {
                    goto finish;
                }
                SetPubVar(progress_window, "progress", i * 100 / count + percent / 10);
                sleep;
            }
        }
    }
    SetPubVar(progress_window, "progress", 100);
    finish:
    new ok[8];
    get_tag_attr2("TEXT", "msgmusic_common_ok", "value", ok, 8);
    if (status < 0)
    {
        get_tag_attr2("TEXT", "msgmusic_hddimport_copy_fail", "value", text, 256);
    }
    else if (copy_status > 0)
    {
        sprintf(text, "WARNING:No data on DataBase!");
        get_tag_attr2("TEXT", "msgmusic_hddimport_copy_fail", "value", text, 256);
    }
    /* NOTE: bug: the else below overwrites the failure message of a failed copy with the completion one. */
    if (copy_status < 0)
    {
        sprintf(text, "WARNING:No data on DataBase!");
        get_tag_attr2("TEXT", "msgmusic_hddimport_copy_fail", "value", text, 256);
    }
    else
    {
        get_tag_attr2("TEXT", "msgmusic_hddimport_copy_complete", "value", text, 256);
    }
    show_message(text, ok);
    SendMessage(sql_obj, TAG("FNSH"), 0, 0, 0, 0);
    Destroy(sql_obj);
    Destroy(progress_window);
    if (FILESYNC_DONE != status)
    {
        return -7;
    }
    if (copy_status < 0)
    {
        return -8;
    }
    return 0;
}

#include "playerctl.inc"
