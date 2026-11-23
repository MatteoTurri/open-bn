/*
 * /opt0/bn/script/system/error.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The error page, which bn opens with the arguments type and errno: a dialog with the message, the error
 * code and the suggested solution.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include <float>

/* Values of the argument type. */
const ERROR_LOAD = 1;   /* a page could not be loaded */
const ERROR_RELOAD = 2; /* a page of the history could not be loaded again */
const ERROR_SCRIPT = 3; /* a script failed */

/* The values of errno told apart: errors of the page loader (thxmlloader.cpp) and of httpfile.h. */
const LOAD_ABORTED = -90000;
const LOAD_BAD_DOCUMENT = -90001;   /* not a valid XML document */
const LOAD_BAD_ROOT = -90002;       /* the root is not RESOURCE, or it has no version */
const LOAD_BAD_SCRIPT = -90003;     /* a script could not be decrypted */
const LOAD_NEW_VERSION = -90004;    /* a version of the page bn does not know */
const LOAD_UPDATE_FAILED = -90007;  /* the last system update failed */
const LOAD_NO_UPDATE = -90008;      /* no system update has been done */
const LOAD_UPDATE_NETWORK = -90010; /* the last system update could not use the network */
const HTTP_TIMEOUT = -10006;        /* the request timed out */

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

#include "misc.inc"

main()
{
    new type;
    new error_no;
    {
        new value[8];
        if (GetArg("type", value, 8))
        {
            type = atoi(value);
        }
        if (GetArg("errno", value, 8))
        {
            error_no = atoi(value);
        }
    }
    /* HTTP 401 and 407 ask for authentication: the page closes at once */
    if (error_no == 401 || error_no == 407)
    {
        Destroy(Scene(0));
    }
    new Float:screen_w;
    new Float:screen_h;
    get_screen_size2(screen_w, screen_h);
    sndSePlay(SE_WINDOW_OPEN);
    new error_dialog; /* confirm_err rather than confirm_d */
    new show_code;
    new show_solution;
    new msg[256];
    new code_prefix[64]; /* the kind of error code: HTTP, system or internal */
    new solution[256];
    switch (type)
    {
        case ERROR_RELOAD:
        {
            error_dialog = 1;
            show_code = 1;
            show_solution = 1;
            get_tag_attr("TEXT", "msgcom_error_reload", "value", msg, 256);
        }
        case ERROR_SCRIPT:
        {
            error_dialog = 1;
            get_tag_attr("TEXT", "msgcom_error_runtime", "value", msg, 256);
        }
        case ERROR_LOAD:
        {
            if (error_no == LOAD_ABORTED)
            {
                error_dialog = 0;
                get_tag_attr("TEXT", "msgcom_error_read_cancel", "value", msg, 256);
            }
            else if (error_no == HTTP_TIMEOUT)
            {
                error_dialog = 0;
                get_tag_attr("TEXT", "msgcom_error_network_timeout", "value", msg, 256);
            }
            else if (error_no == LOAD_UPDATE_FAILED || error_no == LOAD_NEW_VERSION)
            {
                error_dialog = 0;
                get_tag_attr("TEXT", "msgcom_error_old_version", "value", msg, 256);
            }
            else if (error_no == LOAD_UPDATE_NETWORK)
            {
                error_dialog = 0;
                get_tag_attr("TEXT", "msgcom_error_network_connect_verify", "value", msg, 256);
            }
            else if (error_no == LOAD_NO_UPDATE)
            {
                error_dialog = 0;
                get_tag_attr("TEXT", "msgcom_error_network_update", "value", msg, 256);
            }
            else if (error_no == LOAD_BAD_DOCUMENT || error_no == LOAD_BAD_ROOT || error_no == LOAD_BAD_SCRIPT)
            {
                error_dialog = 1;
                get_tag_attr("TEXT", "msgcom_error_description", "value", msg, 256);
            }
            else
            {
                error_dialog = 1;
                show_code = 1;
                show_solution = 1;
                get_tag_attr("TEXT", "msgcom_error_read", "value", msg, 256);
            }
        }
    }
    if (show_code)
    {
        if (error_no >= 0)
        {
            get_tag_attr("TEXT", "msgcom_error_http", "value", code_prefix, 64);
        }
        else if (-1000 < error_no < 0)
        {
            get_tag_attr("TEXT", "msgcom_error_system", "value", code_prefix, 64);
        }
        else if (error_no <= -1000)
        {
            get_tag_attr("TEXT", "msgcom_error_internal", "value", code_prefix, 64);
        }
    }
    if (show_solution)
    {
        new id[16];
        sprintf(id, "text_error_solution%d", error_no);
        get_tag_attr("TEXT", id, "value", solution, 256);
    }
    {
        new text[256];
        replace_all(text, msg, "\\n", "\n", 256);
        msg = text;
        new code[8];
        sprintf(code, "%s%d", code_prefix, error_no);
        replace_all(text, msg, "%ERRORNO", code, 256);
        msg = text;
        if (solution[0])
        {
            append_string(msg, "\n\n", 256);
            append_string(msg, solution, 256);
        }
    }
    {
        new args[256];
        sprintf(args, "msg=%s", msg);
        new dialog = CreateNull(Disp2D(0));
        SetPos(dialog, screen_w / 2, screen_h / 2, 0.0, 0);
        if (error_dialog)
        {
            AttachScript(dialog, _, "confirm_err", args);
        }
        else
        {
            AttachScript(dialog, _, "confirm_d", args);
        }
        SetPubVar(dialog, "manager", Self());
        while (SCRIPT_FINISHED != ScriptState(dialog))
        {
            sleep;
        }
    }
    Destroy(Scene(0));
}
