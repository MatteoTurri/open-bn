/*
 * /opt0/bn/script/system/bn_exit.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The exit page: shows the reboot message and reboots into the browser.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

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

#include "focus.inc"

#include "string.inc"

/* Opens the dialog with the text of the TEXT element msg_id and waits until it is ready. */
open_message_dialog(focus, msg_id[], pad_help)
{
    new args[128];
    {
        new msg[125];
        get_tag_attr("TEXT", msg_id, "value", msg, 125);
        sprintf(args, "msg=%s", msg);
    }
    new dialog = open_dialog(args);
    SetPubVar(dialog, "focus", focus);
    SetPubVar(dialog, "padhelp", pad_help);
    while (!GetPubVar(dialog, "ready"))
    {
        sleep;
    }
    return dialog;
}

#include "misc.inc"

#include "dom.inc"

new g_focus;

/*
 * Reboots into the browser: download slot 0 gets the new boot DES keys, slot 1 the encrypted
 * "BootBrowser" (both in Base64, the second also in ROT13).
 */
reboot_to_browser()
{
    new plain[2][32];
    new encoded[2][64];
    new len;
    new cipher[256];
    new i;
    err10019(plain[0]);
    encode_base64(plain[0], 16, encoded[0], 64);
    err10020();
    new count = 2;
    plain[1] = "BootBrowser";
    i = 1;
    for (; i < count; i++)
    {
        len = err10021(plain[i], cipher);
        encode_base64(cipher, len, encoded[i], 64);
        apply_rot13(encoded[i]);
    }
    err10022(0, encoded[0]);
    err10022(1, encoded[1]);
    err10023(count);
    return;
}

main()
{
    g_focus = create_focus_plane2("focus");
    SetSystemParam(SYS_PADHELP, 0);
    open_message_dialog(g_focus, "msgsystem_bnexit_rebootwait", 0);
    reboot_to_browser();
    Destroy(Scene(0));
}
