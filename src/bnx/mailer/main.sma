/*
 * /opt0/bn/script/mailer/main.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The manager of the mail client: it loads the shared textures, opens one page script at a time (list, read, write,
 * address book, settings, ...) under the global menu (select) and the option menu (triangle), and runs the
 * dialogs, the sending and the receiving of the mail.
 */

#include <core>
#include <bn>

#pragma dynamic 50000

#include "mailer.inc"

#include "wait.inc"

#include "tag.inc"

#include <float>

/* The dialog open in g_objDialog (g_iDialogType). */
const DLG_NONE = 0;
const DLG_YESNO = 1;
const DLG_START = 3;   /* a message with a start button */
const DLG_MESSAGE = 4; /* a message without buttons */
const DLG_PROGRESS = 5;
const DLG_CANCEL = 7;  /* after the cancel button, asks whether to stop the transfer; no starts it again */

/* Where the page of a setting was opened from (g_iSetEditFrom). */
const SEDIT_FROM_MENU = 0;
const SEDIT_FROM_PART = 1; /* back from the editor of one item */
const SEDIT_FROM_BOOT = 2; /* the first setup, at the start */

/* Results of AskSaveDraft. */
const DRAFT_SAVED = 0;
const DRAFT_DROPPED = 1;
const DRAFT_STAY = 2; /* the user stays on the page */

/* Results of ConfirmTransfer and RunTransfer. */
const TRANSFER_NO = 0; /* the user answered no */
const TRANSFER_ENDED = 1;
const TRANSFER_CANCELLED = -1;

/* Result of ConfirmAction besides the ANSWER_* of the dialog: a deletion found the disk full. */
const CONFIRM_NOSPACE = -2;

/* Bits of GetBootFlags. */
const BOOT_NOSPACE = 0x02;
const BOOT_NOTSET = 0x04; /* the mailer has not been set up */
const BOOT_FATAL = 0x08;
const BOOT_NGPASS = 0x10; /* wrong password */

const MAX_ADDR = 999; /* entries of the address book */

/* Sleeps until a message msg arrives. */
wait_message(msg, &a=0, &b=0, &c=0, &d=0)
{
    while (!FindMessage(msg, a, b, c, d))
    {
        sleep;
    }
}

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

/* Decodes the URL-encoded src ("+" and "%XX") into the packed dest of maxlen cells; stops at a bad escape. */
DecodeUrl(src[], dest[], maxlen)
{
    new len;
    for (new i; src{i}; i++)
    {
        new c = src{i};
        new out;
        if (c == '+')
        {
            out = ' ';
        }
        else if (c == '%')
        {
            new hex[2];
            for (new j; j < 2; j++)
            {
                i++;
                c = src{i};
                if ('0' <= c <= '9')
                {
                    hex[j] = c - '0';
                }
                else if ('A' <= c <= 'F')
                {
                    hex[j] = c - 'A' + 10;
                }
                else if ('a' <= c <= 'f')
                {
                    hex[j] = c - 'a' + 10;
                }
                else
                {
                    return;
                }
            }
            out = (hex[0] << 4) + hex[1];
        }
        else if ('a' <= c <= 'z' || 'A' <= c <= 'Z' || '0' <= c <= '9' || c == '*' || c == '-' || c == '.' || c == '@'
                || c == '_') /* redundant as written */
        {
            out = c;
        }
        else
        {
            out = c;
        }
        if (len >= maxlen * 4 - 1)
        {
            return;
        }
        dest{len} = out;
        len++;
    }
}

#include "focus.inc"

#include "mailsync.inc"

/* The color of the dimmer behind the dialogs. */
new Float:g_flDimRed = 0.25;
new Float:g_flDimGreen = 0.25;
new Float:g_flDimBlue = 0.25;
new Float:g_flDimAlpha = 1.0;
new g_szAddrSep[3] = {',', ' '}; /* between the addresses of the To field */

/* Returns the bit of screen (SCREEN_*) in g_iScreenHist. */
ScreenBit(screen)
{
    return 1 << screen;
}

/* Returns the bit of item id of the option menu, for the flags argument of mailopt. */
OptBit(id[])
{
    new index = GetItemIndex("OPT_ITEM", id);
    return 1 << index;
}

/* Loads the texture of the IMG element id and its size. */
LoadImage(id[], &tex, &Float:tex_w=32.0, &Float:tex_h=32.0)
{
    new url[256];
    get_tag_attr("IMG", id, "src", url, 256);
    DecodeTexture(url, tex, tex_w, tex_h);
}

new g_iItemCount;
new g_szAnyId[1]; /* an empty id: the first element with the tag */
new g_szItemId[64];

/* Decodes the image at url into a kept texture; tex and its size stay as they are on an error. */
DecodeTexture(url[], &tex, &Float:tex_w, &Float:tex_h)
{
    imgDecStart(url, 256, 256, 0, 100, 0); /* asked at 256x256, in the default image mode */
    new status = imgDecSync();
    while (IMG_DONE != status)
    {
        status = imgDecSync();
        if (status == IMG_FAILED)
        {
            return;
        }
        sleep;
    }
    tex = imgTexID();
    imgKeepTex(tex);
    tex_w = float(imgTexOrgWidth());
    tex_h = float(imgTexOrgHeight());
}

/* Fills ids with the ids of the empty leaves of the first tag element; returns their number. */
GetItemIds(tag[], ids[][])
{
    g_iItemCount = 0;
    new node = domFindTag(domRoot(), tag, g_szAnyId, 0);
    CollectItemIds(node, ids);
    return g_iItemCount;
}

/* Appends to ids, from g_iItemCount on, the id of every element below node with no children and no text. */
CollectItemIds(node, ids[][])
{
    new tag_name[16];
    domGetTagName(node, tag_name, 16);
    g_szItemId[0] = 0;
    domGetData(node, "id", g_szItemId, 64);
    static text[256];
    text[0] = 0;
    domGetData(node, _, text, 256);
    if (0 == domFirstChild(node))
    {
        if (0 == strlen(text))
        {
            sprintf(ids[g_iItemCount++], "%s", g_szItemId);
        }
    }
    else
    {
        for (new child = domFirstChild(node); child; child = domNext(child))
        {
            CollectItemIds(child, ids);
        }
    }
}

new g_iItemIndex;
new g_szFindId[64];

/* Returns the index of item id among the empty leaves of the first tag element, 0 when it is not there. */
GetItemIndex(tag[], id[])
{
    g_iItemCount = 0;
    g_iItemIndex = 0;
    new node = domFindTag(domRoot(), tag, g_szAnyId, 0);
    FindItemIndex(node, id);
    return g_iItemIndex;
}

/* Counts the empty leaves below node like CollectItemIds; the one whose id is id sets g_iItemIndex. */
FindItemIndex(node, id[])
{
    new tag_name[16];
    domGetTagName(node, tag_name, 16);
    g_szFindId[0] = 0;
    domGetData(node, "id", g_szFindId, 64);
    static text[256];
    text[0] = 0;
    domGetData(node, _, text, 256);
    if (0 == domFirstChild(node))
    {
        if (0 == strlen(text))
        {
            if (!strcmp(g_szFindId, id))
            {
                g_iItemIndex = g_iItemCount;
            }
            g_iItemCount++;
        }
    }
    else
    {
        for (new child = domFirstChild(node); child; child = domNext(child))
        {
            FindItemIndex(child, id);
        }
    }
}

#include "mailline.inc"

/* The textures shared by the pages, which read them with GetPubVar. */
public g_txBg; /* the IMG "bg" of the page */
public Float:g_BgWidth; /* in pixels */
public Float:g_BgHeight; /* in pixels */
public g_txArrow; /* the IMG "arrow" of the page */
public Float:g_ArrowWidth; /* in pixels */
public Float:g_ArrowHeight; /* in pixels */
public g_txArrowGlow; /* the IMG "arrow_glow" of the page */
public Float:g_ArrowGlowWidth; /* in pixels */
public Float:g_ArrowGlowHeight; /* in pixels */
public g_txListFg; /* the IMG "list_focus" of the page */
public Float:g_ListFgWidth; /* in pixels */
public Float:g_ListFgHeight; /* in pixels */
public g_txListBg; /* the IMG "list_back" of the page */
public Float:g_ListBgWidth; /* in pixels */
public Float:g_ListBgHeight; /* in pixels */
public g_txAddrGrid; /* the IMG "addr_back" of the page */
public Float:g_AddrGridWidth; /* in pixels */
public Float:g_AddrGridHeight; /* in pixels */
public g_txAddrEdit; /* the IMG "addr_focus" of the page */
public Float:g_AddrEditWidth; /* in pixels */
public Float:g_AddrEditHeight; /* in pixels */
public g_txAddrEditGrid; /* the IMG "aedit_back" of the page */
public Float:g_AddrEdGridWidth; /* in pixels */
public Float:g_AddrEdGridHeight; /* in pixels */
public g_txAddrTop; /* the IMG "addr_top" of the page */
public Float:g_AddrTopWidth; /* in pixels */
public Float:g_AddrTopHeight; /* in pixels */
public g_txBgOpt; /* the IMG "opt_back" of the page */
public Float:g_BgOptWidth; /* in pixels */
public Float:g_BgOptHeight; /* in pixels */
public g_txSeparate; /* the IMG "separate" of the page */
public Float:g_SeparateWidth; /* in pixels */
public Float:g_SeparateHeight; /* in pixels */
public g_txBgGbl; /* the IMG "gbl_title" of the page */
public Float:g_BgGblWidth; /* in pixels */
public Float:g_BgGblHeight; /* in pixels */
public g_txFromBack; /* the IMG "from_back" of the page */
public Float:g_FromBackWidth; /* in pixels */
public Float:g_FromBackHeight; /* in pixels */
public g_txToBack; /* the IMG "to_back" of the page */
public Float:g_ToBackWidth; /* in pixels */
public Float:g_ToBackHeight; /* in pixels */
public g_txTitleBack; /* the IMG "title_back" of the page */
public Float:g_TitleBackWidth; /* in pixels */
public Float:g_TitleBackHeight; /* in pixels */
public g_txDateBack; /* the IMG "date_back" of the page */
public Float:g_DateBackWidth; /* in pixels */
public Float:g_DateBackHeight; /* in pixels */
public g_txA01; /* the icon of a mail: unread */
public g_txA03; /* the icon of a mail: unread, partial */
public g_txA05; /* the icon of a mail: unread, locked */
public g_txA07; /* the icon of a mail: unread, locked, partial */
public g_txA09; /* the icon of a mail: read */
public g_txA11; /* the icon of a mail: read, partial */
public g_txA13; /* the icon of a mail: read, locked */
public g_txA15; /* the icon of a mail: read, locked, partial */
public Float:g_AWidth; /* the size of every icon of a mail, in pixels */
public Float:g_AHeight; /* the size of every icon of a mail, in pixels */
public g_txBoxInbox; /* the IMG "box_inbox" of the page */
public Float:g_BoxInboxWidth; /* in pixels */
public Float:g_BoxInboxHeight; /* in pixels */
public g_txBoxOutbox; /* the IMG "box_outbox" of the page */
public Float:g_BoxOutboxWidth; /* in pixels */
public Float:g_BoxOutboxHeight; /* in pixels */
public g_txBoxDraft; /* the IMG "box_draft" of the page */
public Float:g_BoxDraftWidth; /* in pixels */
public Float:g_BoxDraftHeight; /* in pixels */
public g_txBoxQueue; /* the IMG "box_queue" of the page */
public Float:g_BoxQueueWidth; /* in pixels */
public Float:g_BoxQueueHeight; /* in pixels */
public g_OptAddr; /* enables the add-address item of the option menu */
public g_OptLock; /* enables the lock item of the option menu */
public g_OptDele; /* the selected mail is not locked and can be deleted */
public s_bSecondFlg; /* unused, but part of the layout */
public g_blink; /* unused, but part of the layout */
new g_objFocus;
new g_objPage; /* the page that has the focus */
new g_objOpt;
new g_bOptShown;
new g_iOptMenu; /* the item chosen in the option menu (mailopt) */
new g_objGbl;
new g_bGblShown;
new g_iGblMenu; /* the item chosen in the global menu (mailgbl), where it opens again */
new g_objList;
new g_bListShown;
new g_iFolder; /* the folder shown (FOLDER_*) */
new g_iLine; /* the selected mail is g_iLine + g_iTop */
new g_iTop;
new g_iInboxMsgNo; /* the number of the selected mail, kept over a refresh */
new g_iInboxLine;
new g_iInboxTop;
new g_iOutboxMsgNo; /* the number of the selected mail, kept over a refresh */
new g_iOutboxLine;
new g_iOutboxTop;
new g_iDraftLine;
new g_iDraftTop;
new g_iQueueMsgNo; /* the number of the selected mail, kept over a refresh */
new g_iQueueLine;
new g_iQueueTop;
new g_objRead;
new g_bReadShown;
new g_objWrite;
new g_bWriteShown;
new g_iWriteItem; /* the item selected in the write page, kept while the address book is open */
new g_iWriteMode; /* the WRITE_* mode the write page was opened in */
new g_bWriteEdited; /* g_revert of the write page, kept while the address book is open */
new g_szTo[246]; /* the mail being written, kept while the address book is open */
new g_szSubject[246];
new g_szBody[32768];
new g_objAddr;
new g_bAddrShown;
new g_iAddrLine;
new g_iAddrTop;
new g_iAddrMode; /* ADDR_MODE_* */
new g_objAddrEdit;
new g_bAddrEditShown;
new g_iAddrEditMode; /* AEDIT_* */
new g_objSet;
new g_bSetShown;
new g_iSetMenu; /* the item chosen in the settings menu (mailset) */
new g_objSetEdit;
new g_bSetEditShown;
new g_iSetEditFrom; /* SEDIT_FROM_* */
new g_iSetEditItem; /* the item chosen in the page of a setting */
new g_objPart;
new g_bPartShown;
new g_iPartSelect;
new g_objInterval;
new g_bIntervalShown;
new g_szPartArgs[64]; /* the arguments of the item editor, to open it again */
new g_objSort;
new g_bSortShown;
new g_iSortMenu; /* the item chosen in the sort menu (mailsort) */
new g_objSortEdit;
new g_bSortEditShown;
new g_iSortError; /* ERR_NOSPACE when the sort found no space */
new g_bSortApply; /* the sort page was closed with circle */
new g_objRefresh;
new g_bRefreshShown;
new g_bTransferred; /* the refresh moved mail */
new g_objInfo;
new g_bInfoShown;
new g_objBg;
new g_objDimmer;
new g_objDialog;
new g_iDialogType;

/* Receives messages until msg arrives, dropping the others. */
WaitRecvMessage(msg, &a, &b=0, &c=0)
{
    for (new recv_msg; ; )
    {
        while (RecvMessage(recv_msg, a, b, c))
        {
            if (recv_msg == msg)
            {
                return;
            }
        }
        sleep;
    }
}

/* Receives string messages until msg arrives, dropping the others. */
WaitRecvMessageStr(msg, text[], maxlen)
{
    for (new recv_msg; ; )
    {
        while (RecvMessageStr(recv_msg, text, maxlen))
        {
            if (recv_msg == msg)
            {
                return;
            }
        }
        sleep;
    }
}

/* Fades out and destroys dialog in fade_time ms. */
CloseDialog(dialog, fade_time)
{
    SendMessage(dialog, TAG("FOCS"), TAG("OUT"), 0, 0, 0);
    SendMessage(dialog, TAG("FADE"), TAG("OUT"), float(fade_time), 0, 0);
    wait_ms(fade_time);
    Destroy(dialog);
    g_iDialogType = DLG_NONE;
    sleep;
}

/*
 * Opens maildlgyesno. Like the dialogs below, it shows the text msg_id of the page (a "\n" in it breaks the
 * line) over the dimmer and keeps type in g_iDialogType.
 */
OpenYesNoDialog(msg_id[], focus, select, fade_time, type)
{
    CreateDimmer();
    new text[128];
    new msg[128];
    new yes[8];
    new no[8];
    new args[256];
    get_tag_attr("TEXT", msg_id, "value", text, 128);
    replace_all(msg, text, "\\n", "\n", 128);
    get_tag_attr("TEXT", "msgmailer_common_yes", "value", yes, 8);
    get_tag_attr("TEXT", "msgmailer_common_no", "value", no, 8);
    sprintf(args, "msg=%s;yes=%s;no=%s;", msg, yes, no);
    new dialog = CreateNull(g_objDimmer);
    AttachScript(dialog, _, "maildlgyesno", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus);
    SetPubVar(dialog, "select", select);
    SetPubVar(dialog, "dlg_active", 1);
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    wait_message(TAG("DGWT"));
    g_objDialog = dialog;
    g_iDialogType = type;
    SendMessage(dialog, TAG("FOCS"), TAG("IN"), 0, 0, 0);
    SendMessage(dialog, TAG("FADE"), TAG("IN"), float(fade_time), 0, 0);
    wait_ms(fade_time);
    return dialog;
}

/* Opens maildlgokstart, a message with a start button. */
OpenStartDialog(msg_id[], focus, fade_time, type)
{
    CreateDimmer();
    new text[128];
    new msg[128];
    new ok[8];
    new args[256];
    get_tag_attr("TEXT", msg_id, "value", text, 128);
    replace_all(msg, text, "\\n", "\n", 128);
    get_tag_attr("TEXT", "msgmailer_common_start", "value", ok, 8);
    sprintf(args, "msg=%s;ok=%s;", msg, ok);
    new dialog = CreateNull(g_objDimmer);
    AttachScript(dialog, _, "maildlgokstart", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus);
    SetPubVar(dialog, "dlg_active", 1);
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    wait_message(TAG("DGWT"));
    g_objDialog = dialog;
    g_iDialogType = type;
    SendMessage(dialog, TAG("FOCS"), TAG("IN"), 0, 0, 0);
    SendMessage(dialog, TAG("FADE"), TAG("IN"), float(fade_time), 0, 0);
    wait_ms(fade_time);
    return dialog;
}

/* Opens maildlgnobtn, a message without buttons. */
OpenMessageDialog(msg_id[], focus, fade_time, type)
{
    CreateDimmer();
    new text[128];
    new msg[128];
    new args[256];
    get_tag_attr("TEXT", msg_id, "value", text, 128);
    replace_all(msg, text, "\\n", "\n", 128);
    sprintf(args, "msg=%s;", msg);
    new dialog = CreateNull(g_objDimmer);
    AttachScript(dialog, _, "maildlgnobtn", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus);
    SetPubVar(dialog, "dlg_active", 1);
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    wait_message(TAG("DGWT"));
    g_objDialog = dialog;
    g_iDialogType = type;
    SendMessage(dialog, TAG("FADE"), TAG("IN"), float(fade_time), 0, 0);
    SendMessage(dialog, TAG("FOCS"), TAG("IN"), 0, 0, 0);
    wait_ms(fade_time);
    return dialog;
}

/* Opens a message without buttons where marker ("%ERRNO", "%MAILNO") is replaced by number. */
OpenNumberDialog(msg_id[], focus, number, marker[], fade_time, type)
{
    CreateDimmer();
    new text[128];
    new msg_fmt[128];
    new msg[128];
    new fmt[128];
    new args[256];
    get_tag_attr("TEXT", msg_id, "value", text, 128);
    replace_all(msg_fmt, text, "\\n", "\n", 128);
    replace_all(fmt, msg_fmt, marker, "%d", 128);
    sprintf(msg, fmt, number);
    sprintf(args, "msg=%s;", msg);
    new dialog = CreateNull(g_objDimmer);
    AttachScript(dialog, _, "maildlgnobtn", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus);
    SetPubVar(dialog, "dlg_active", 1);
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    wait_message(TAG("DGWT"));
    g_objDialog = dialog;
    g_iDialogType = type;
    SendMessage(dialog, TAG("FADE"), TAG("IN"), float(fade_time), 0, 0);
    SendMessage(dialog, TAG("FOCS"), TAG("IN"), 0, 0, 0);
    wait_ms(fade_time);
    return dialog;
}

/* Opens the end of the receiving: count mails received ("%MAILNO"), over_count left over the size limit. */
OpenSizeOverDialog(focus, count, over_count, fade_time, type)
{
    CreateDimmer();
    new text[128];
    new msg_fmt[128];
    new fmt1[128];
    new fmt[128];
    new msg[128];
    new args[128];
    get_tag_attr("TEXT", "msgmailer_dlg_recv_done_with_sizeover", "value", text, 128);
    replace_all(msg_fmt, text, "\\n", "\n", 128);
    replace_all(fmt1, msg_fmt, "%MAILNO", "%d", 128);
    replace_all(fmt, fmt1, "%BIGMAILNO", "%d", 128);
    sprintf(msg, fmt, count, over_count);
    sprintf(args, "msg=%s;", msg);
    new dialog = CreateNull(g_objDimmer);
    AttachScript(dialog, _, "maildlgnobtn", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus);
    SetPubVar(dialog, "dlg_active", 1);
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    wait_message(TAG("DGWT"));
    g_objDialog = dialog;
    g_iDialogType = type;
    SendMessage(dialog, TAG("FADE"), TAG("IN"), float(fade_time), 0, 0);
    SendMessage(dialog, TAG("FOCS"), TAG("IN"), 0, 0, 0);
    wait_ms(fade_time);
    return dialog;
}

/* Opens a message without buttons that takes no focus and hides the pad help, while a transfer stops. */
OpenBusyDialog(msg_id[], focus, fade_time, type)
{
    CreateDimmer();
    new text[128];
    new msg[128];
    new args[256];
    get_tag_attr("TEXT", msg_id, "value", text, 128);
    replace_all(msg, text, "\\n", "\n", 128);
    sprintf(args, "msg=%s;", msg);
    new dialog = CreateNull(g_objDimmer);
    AttachScript(dialog, _, "maildlgnobtn", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus);
    SetPubVar(dialog, "dlg_active", 1);
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    wait_message(TAG("DGWT"));
    g_objDialog = dialog;
    g_iDialogType = type;
    SendMessage(dialog, TAG("FADE"), TAG("IN2"), float(fade_time), 0, 0);
    SendMessage(dialog, TAG("FOCS"), TAG("OUT"), 0, 0, 0);
    SetSystemParam(SYS_PADHELP, 0);
    wait_ms(fade_time);
    return dialog;
}

/* Opens mailprogresswindow, the progress of a transfer with a cancel button. */
OpenProgressDialog(msg_id[], focus, fade_time, type)
{
    CreateDimmer();
    new text[128];
    new msg[128];
    new cancel[8];
    new args[256];
    get_tag_attr("TEXT", msg_id, "value", text, 128);
    replace_all(msg, text, "\\n", "\n", 128);
    get_tag_attr("TEXT", "msgmailer_common_cancel", "value", cancel, 8);
    sprintf(args, "msg=%s;cancel=%s;percent=%s;", msg, cancel, "%");
    new dialog = CreateNull(g_objDimmer);
    AttachScript(dialog, _, "mailprogresswindow", args);
    SetPubVar(dialog, "manager", Self());
    SetPubVar(dialog, "focus", focus);
    SetPubVar(dialog, "dlg_active", 1);
    SetPos(dialog, 320.0, 224.0, 0.0, 0);
    wait_message(TAG("DGWT"));
    g_objDialog = dialog;
    g_iDialogType = type;
    SendMessage(dialog, TAG("FADE"), TAG("IN"), float(fade_time), 0, 0);
    SendMessage(dialog, TAG("FOCS"), TAG("IN"), 0, 0, 0);
    wait_ms(fade_time);
    return dialog;
}

/* Creates the defocus layer behind the dialogs, unless it is there, and fades it in. */
CreateDimmer()
{
    if (!IsValidObject(g_objDimmer))
    {
        g_objDimmer = CreateDefocus(Disp2D(0));
        SetOptionI(g_objDimmer, TAG("LEVL"), 3);
        SetOptionI(g_objDimmer, TAG("ZSRT"), ZSORT_OVERLAY);
        AnimColor(g_objDimmer, g_flDimRed, g_flDimGreen, g_flDimBlue, g_flDimAlpha, float(400), 0.0, ANIM_FUNC_LINEAR);
    }
}

FadeOutDimmer()
{
    if (IsValidObject(g_objDimmer))
    {
        AnimColor(g_objDimmer, 0.35, 0.35, 0.35, 0.0, float(400), 0.0, ANIM_FUNC_LINEAR);
    }
}

/* Sets the progress of dialog from the SYNC_* state of a transfer and the bytes done of total. */
UpdateProgress(state, done, total, dialog)
{
    new percent;
    if (state == SYNC_CONNECT)
    {
        percent = 10;
    }
    else if (state == SYNC_TRANSFER)
    {
        percent = floatround(80 * (float(done) / float(total)), 0) + 10;
    }
    else if (state == SYNC_CLOSE)
    {
        percent = 90;
    }
    if (percent > 0)
    {
        SetPubVar(dialog, "progress", percent);
    }
}

/*
 * Sends the mail at queue_line of the queue (-1: the whole queue) showing the progress in dialog; count gets the
 * mail reached of mail_total, new_no the number of the last mail sent. Returns 0 when it was cancelled.
 */
RunSend(&count, &mail_total, dialog, focus, queue_line, &new_no=-1)
{
    SystemCommand(SYSCMD_BUSY, 1);
    new completed = 1;
    new can_cancel = 1;
    new state;
    new done;
    new total;
    mx_RefreshSend(queue_line);
    do
    {
        state = mx_SyncSend(count, mail_total, done, total, new_no);
        UpdateProgress(state, done, total, dialog);
        if (state == SYNC_CLOSE && can_cancel)
        {
            SendMessage(dialog, TAG("FOCS"), TAG("DIM"), 0, 0, 0);
            can_cancel = 0;
        }
        else if (!GetPubVar(dialog, "dlg_cancel"))
        {
            if (mx_SendCancel())
            {
                sndSePlay(SE_DECIDE);
                CloseDialog(dialog, 50);
                new busy = OpenBusyDialog("msgmailer_dlg_send_cancel_prog", focus, 50, DLG_PROGRESS);
                do
                {
                    sleep;
                }
                while (mx_SyncSend(count, mail_total, done, total, new_no));
                completed = 0;
                dialog = busy;
                break;
            }
        }
        sleep;
    }
    while (state);
    SendMessage(dialog, TAG("FOCS"), TAG("DIM"), 0, 0, 0);
    if (completed)
    {
        SetPubVar(dialog, "progress", 100);
    }
    SetPubVar(dialog, "dlg_active", 0);
    SystemCommand(SYSCMD_BUSY, 0);
    return completed;
}

/*
 * Receives the mail showing the progress in dialog; count gets the mail reached of mail_total, over_count the
 * mails left on the server over the size limit. Returns 0 when it was cancelled.
 */
RunRecv(&count, &mail_total, dialog, focus, &over_count)
{
    SystemCommand(SYSCMD_BUSY, 1);
    new completed = 1;
    new can_cancel = 1;
    new state;
    new done;
    new total;
    mx_RefreshRecv();
    do
    {
        state = mx_SyncRecv(count, mail_total, done, total, over_count);
        UpdateProgress(state, done, total, dialog);
        if (state == SYNC_CLOSE && can_cancel)
        {
            SendMessage(dialog, TAG("FOCS"), TAG("DIM"), 0, 0, 0);
            can_cancel = 0;
        }
        else if (!GetPubVar(dialog, "dlg_cancel"))
        {
            if (mx_RecvCancel())
            {
                sndSePlay(SE_DECIDE);
                CloseDialog(dialog, 50);
                new busy = OpenBusyDialog("msgmailer_dlg_recv_cancel_prog", focus, 50, DLG_PROGRESS);
                do
                {
                    sleep;
                }
                while (mx_SyncRecv(count, mail_total, done, total, over_count));
                completed = 0;
                dialog = busy;
                break;
            }
        }
        sleep;
    }
    while (state);
    if (completed)
    {
        SetPubVar(dialog, "progress", 100);
    }
    SetPubVar(dialog, "dlg_active", 0);
    SystemCommand(SYSCMD_BUSY, 0);
    return completed;
}

/*
 * Asks msg_id; on yes sends action to page ("ACT") and shows done_msg_id. Returns the ANSWER_* of the dialog
 * (ANSWER_YES when it was done), or CONFIRM_NOSPACE when a deletion found no space.
 */
ConfirmAction(action, msg_id[], done_msg_id[], select, focus, page)
{
    if (page != Self())
    {
        SetPubVar(page, "g_active", 0);
        SendMessage(page, TAG("FOCS"), TAG("OUT"), 0, 0, 0);
        SendMessage(page, TAG("BLNK"), TAG("OUT"), 0, 0, 0);
    }
    OpenYesNoDialog(msg_id, focus, select, 400, DLG_YESNO);
    new result;
    new running = 1;
    while (running)
    {
        if (!GetPubVar(g_objDialog, "dlg_active"))
        {
            if (g_iDialogType == DLG_YESNO)
            {
                if (ANSWER_YES == GetPubVar(g_objDialog, "select"))
                {
                    CloseDialog(g_objDialog, 50);
                    SetSystemParam(SYS_PADHELP, 0);
                    SendMessage(page, TAG("ACT"), action, 0, 0, 0);
                    wait_message(TAG("ACWT"));
                    if (action == TAG("DELE") && GetPubVar(page, "g_space") == ERR_NOSPACE)
                    {
                        OpenMessageDialog("msgmailer_dlg_delete_nospace", focus, 50, DLG_MESSAGE);
                        result = CONFIRM_NOSPACE;
                    }
                    else
                    {
                        OpenMessageDialog(done_msg_id, focus, 50, DLG_MESSAGE);
                        result = ANSWER_YES;
                    }
                }
                else if (GetPubVar(g_objDialog, "select") == ANSWER_CANCEL)
                {
                    FadeOutDimmer();
                    CloseDialog(g_objDialog, 400);
                    result = ANSWER_CANCEL;
                    running = 0;
                }
                else
                {
                    FadeOutDimmer();
                    CloseDialog(g_objDialog, 400);
                    result = ANSWER_NO;
                    running = 0;
                }
            }
            else if (g_iDialogType == DLG_MESSAGE)
            {
                FadeOutDimmer();
                CloseDialog(g_objDialog, 400);
                running = 0;
            }
        }
        sleep;
    }
    Destroy(g_objDimmer);
    if (page != Self())
    {
        SendMessage(page, TAG("BLNK"), TAG("IN"), 0, 0, 0);
        SendMessage(page, TAG("FOCS"), TAG("IN"), 0, 0, 0);
    }
    sleep;
    return result;
}

/* Asks msg_id and returns the select of the dialog. */
AskYesNo(msg_id[], select, focus, page)
{
    if (page != Self())
    {
        SetPubVar(page, "g_active", 0);
        SendMessage(page, TAG("FOCS"), TAG("OUT"), 0, 0, 0);
        SendMessage(page, TAG("BLNK"), TAG("OUT"), 0, 0, 0);
    }
    OpenYesNoDialog(msg_id, focus, select, 400, DLG_YESNO);
    new answer;
    new running = 1;
    while (running)
    {
        if (!GetPubVar(g_objDialog, "dlg_active"))
        {
            if (g_iDialogType == DLG_YESNO)
            {
                answer = GetPubVar(g_objDialog, "select");
                FadeOutDimmer();
                CloseDialog(g_objDialog, 400);
                running = 0;
            }
        }
        sleep;
    }
    Destroy(g_objDimmer);
    sleep;
    return answer;
}

/*
 * Asks whether to leave the mail being written, then whether to save it as a draft (through page when it is the
 * write page). Returns DRAFT_SAVED, DRAFT_DROPPED or DRAFT_STAY; error gets
 * ERR_NOSPACE, index the line of the draft.
 */
AskSaveDraft(select, focus, page, &error, &index)
{
    if (page != Self())
    {
        SetPubVar(page, "g_active", 0);
        SendMessage(page, TAG("FOCS"), TAG("OUT"), 0, 0, 0);
        SendMessage(page, TAG("BLNK"), TAG("OUT"), 0, 0, 0);
    }
    OpenYesNoDialog("msgmailer_dlg_write_draft_1", focus, select, 400, DLG_YESNO);
    new result;
    new first = 1;
    new running = 1;
    while (running)
    {
        if (!GetPubVar(g_objDialog, "dlg_active"))
        {
            if (g_iDialogType == DLG_YESNO)
            {
                if (ANSWER_YES == GetPubVar(g_objDialog, "select"))
                {
                    CloseDialog(g_objDialog, 50);
                    if (first)
                    {
                        select = ANSWER_YES;
                        OpenYesNoDialog("msgmailer_dlg_write_draft_2", focus, select, 50, DLG_YESNO);
                        first = 0;
                    }
                    else
                    {
                        new save_error;
                        if (page == g_objWrite)
                        {
                            SendMessage(page, TAG("ACT"), TAG("DRFT"), 0, 0, 0);
                            WaitRecvMessage(TAG("ACWT"), index, save_error);
                        }
                        else
                        {
                            if (g_iFolder == FOLDER_DRAFT)
                            {
                                remove_message(g_iFolder, g_iLine + g_iTop);
                            }
                            index = save_message(FOLDER_DRAFT, -1, g_szTo, "", "", g_szSubject, g_szBody, g_iWriteMode,
                                    g_iFolder, g_iLine + g_iTop, save_error);
                        }
                        if (save_error == ERR_NOSPACE)
                        {
                            OpenMessageDialog("msgmailer_dlg_write_nospace", focus, 50, DLG_MESSAGE);
                            error = ERR_NOSPACE;
                        }
                        else
                        {
                            OpenMessageDialog("msgmailer_dlg_write_draft_save_ok", focus, 50, DLG_MESSAGE);
                        }
                        result = DRAFT_SAVED;
                    }
                }
                else if (GetPubVar(g_objDialog, "select") == ANSWER_CANCEL)
                {
                    FadeOutDimmer();
                    CloseDialog(g_objDialog, 400);
                    result = DRAFT_STAY;
                    running = 0;
                }
                else
                {
                    FadeOutDimmer();
                    CloseDialog(g_objDialog, 400);
                    if (first)
                    {
                        result = DRAFT_STAY;
                        running = 0;
                    }
                    else
                    {
                        result = DRAFT_DROPPED;
                        running = 0;
                    }
                }
            }
            else if (g_iDialogType == DLG_MESSAGE)
            {
                FadeOutDimmer();
                CloseDialog(g_objDialog, 400);
                running = 0;
            }
        }
        sleep;
    }
    Destroy(g_objDimmer);
    sleep;
    return result;
}

/* Shows msg_id until it is closed; always returns 1. */
ShowMessage(msg_id[], focus, page)
{
    if (page != Self())
    {
        SetPubVar(page, "g_active", 0);
        SendMessage(page, TAG("FOCS"), TAG("OUT"), 0, 0, 0);
        SendMessage(page, TAG("BLNK"), TAG("OUT"), 0, 0, 0);
    }
    OpenMessageDialog(msg_id, focus, 400, DLG_MESSAGE);
    new running = 1;
    while (running)
    {
        if (!GetPubVar(g_objDialog, "dlg_active"))
        {
            if (g_iDialogType == DLG_MESSAGE)
            {
                FadeOutDimmer();
                CloseDialog(g_objDialog, 400);
                running = 0;
            }
        }
        sleep;
    }
    Destroy(g_objDimmer);
    if (page != Self())
    {
        SendMessage(page, TAG("BLNK"), TAG("IN"), 0, 0, 0);
        SendMessage(page, TAG("FOCS"), TAG("IN"), 0, 0, 0);
    }
    sleep;
    return 1;
}

/* Shows msg_id with a start button and returns the select of the dialog. */
AskStart(msg_id[], focus, page)
{
    if (page != Self())
    {
        SetPubVar(page, "g_active", 0);
        SendMessage(page, TAG("FOCS"), TAG("OUT"), 0, 0, 0);
        SendMessage(page, TAG("BLNK"), TAG("OUT"), 0, 0, 0);
    }
    OpenStartDialog(msg_id, focus, 400, DLG_START);
    new answer;
    new running = 1;
    while (running)
    {
        if (!GetPubVar(g_objDialog, "dlg_active"))
        {
            if (g_iDialogType == DLG_START)
            {
                FadeOutDimmer();
                answer = GetPubVar(g_objDialog, "select");
                CloseDialog(g_objDialog, 400);
                running = 0;
            }
        }
        sleep;
    }
    Destroy(g_objDimmer);
    if (page != Self())
    {
        SendMessage(page, TAG("BLNK"), TAG("IN"), 0, 0, 0);
        SendMessage(page, TAG("FOCS"), TAG("IN"), 0, 0, 0);
    }
    sleep;
    return answer;
}

/* Asks whether to quit the mailer; returns 1 on yes. */
AskQuit(select, focus, page)
{
    if (page != Self())
    {
        SetPubVar(page, "g_active", 0);
        SendMessage(page, TAG("FOCS"), TAG("OUT"), 0, 0, 0);
        SendMessage(page, TAG("BLNK"), TAG("OUT"), 0, 0, 0);
    }
    OpenYesNoDialog("msgmailer_dlg_quit", focus, select, 400, DLG_YESNO);
    new quit;
    new running = 1;
    while (running)
    {
        if (!GetPubVar(g_objDialog, "dlg_active"))
        {
            if (g_iDialogType == DLG_YESNO)
            {
                if (ANSWER_YES != GetPubVar(g_objDialog, "select"))
                {
                    quit = 0;
                }
                else
                {
                    quit = 1;
                }
                FadeOutDimmer();
                CloseDialog(g_objDialog, 400);
                running = 0;
            }
        }
        sleep;
    }
    Destroy(g_objDimmer);
    sleep;
    return quit;
}

/*
 * Asks whether to add the address of page to the address book and adds it ("ADDR", or "AEXT" over an entry
 * with the same address); returns 1 when it was added.
 */
AskAddAddress(select, focus, page)
{
    if (page != Self())
    {
        SetPubVar(page, "g_active", 0);
        SendMessage(page, TAG("FOCS"), TAG("OUT"), 0, 0, 0);
        SendMessage(page, TAG("BLNK"), TAG("OUT"), 0, 0, 0);
    }
    OpenYesNoDialog("msgmailer_dlg_addredit_add", focus, select, 400, DLG_YESNO);
    new added;
    new running = 1;
    new check = 1;
    while (running)
    {
        if (!GetPubVar(g_objDialog, "dlg_active"))
        {
            if (g_iDialogType == DLG_YESNO)
            {
                if (ANSWER_YES != GetPubVar(g_objDialog, "select"))
                {
                    FadeOutDimmer();
                    CloseDialog(g_objDialog, 400);
                    added = 0;
                    running = 0;
                }
                else
                {
                    CloseDialog(g_objDialog, 50);
                    if (check)
                    {
                        new found;
                        new too_long;
                        SendMessage(page, TAG("ACT"), TAG("ACHK"), 0, 0, 0);
                        WaitRecvMessage(TAG("ACWT"), found, too_long);
                        if (mx_CountAddr() >= MAX_ADDR)
                        {
                            OpenMessageDialog("msgmailer_dlg_addredit_add_full", focus, 50, DLG_MESSAGE);
                        }
                        else if (too_long)
                        {
                            OpenMessageDialog("msgmailer_dlg_addredit_add_toolong", focus, 50, DLG_MESSAGE);
                        }
                        else if (found >= 0)
                        {
                            OpenYesNoDialog("msgmailer_dlg_addredit_add_exist", focus, ANSWER_NO, 50, DLG_YESNO);
                            check = 0;
                        }
                        else
                        {
                            OpenMessageDialog("msgmailer_dlg_addredit_add_ok", focus, 50, DLG_MESSAGE);
                            SendMessage(page, TAG("ACT"), TAG("ADDR"), 0, 0, 0);
                            wait_message(TAG("ACWT"));
                            added = 1;
                        }
                    }
                    else
                    {
                        OpenMessageDialog("msgmailer_dlg_addredit_add_ok", focus, 50, DLG_MESSAGE);
                        SendMessage(page, TAG("ACT"), TAG("AEXT"), 0, 0, 0);
                        wait_message(TAG("ACWT"));
                        added = 1;
                    }
                }
            }
            else if (g_iDialogType == DLG_MESSAGE)
            {
                FadeOutDimmer();
                CloseDialog(g_objDialog, 400);
                running = 0;
            }
        }
        sleep;
    }
    Destroy(g_objDimmer);
    if (page != Self())
    {
        SendMessage(page, TAG("BLNK"), TAG("IN"), 0, 0, 0);
        SendMessage(page, TAG("FOCS"), TAG("IN"), 0, 0, 0);
    }
    sleep;
    return added;
}

/*
 * Asks ask_msg, then sends the mail of page (action "SEND") or receives ("RECV") with a progress dialog and
 * shows the result. Returns TRANSFER_ENDED (error; index: the line of the mail in the outbox or the queue),
 * TRANSFER_CANCELLED or TRANSFER_NO; count gets the mails transferred.
 */
ConfirmTransfer(action, ask_msg[], prog_msg[], ok_msg[], cancel_msg[], cancel_ok_msg[], select, focus, page, &error,
        &index, &count=0)
{
    if (page != Self())
    {
        SetPubVar(page, "g_active", 0);
        SendMessage(page, TAG("FOCS"), TAG("OUT"), 0, 0, 0);
        SendMessage(page, TAG("BLNK"), TAG("OUT"), 0, 0, 0);
    }
    new mail_total = -1;
    new result;
    new start = 1;
    new completed;
    new queue_line = -1;
    new new_no = -1;
    new over_count;
    new running = 1;
    OpenYesNoDialog(ask_msg, focus, select, 400, DLG_YESNO);
    while (running)
    {
        if (g_iDialogType == DLG_PROGRESS && start)
        {
            if (action == TAG("SEND"))
            {
                completed = RunSend(count, mail_total, g_objDialog, focus, queue_line, new_no);
            }
            else if (action == TAG("RECV"))
            {
                completed = RunRecv(count, mail_total, g_objDialog, focus, over_count);
            }
            start = 0;
        }
        if (!GetPubVar(g_objDialog, "dlg_active"))
        {
            if (g_iDialogType == DLG_YESNO)
            {
                if (ANSWER_YES != GetPubVar(g_objDialog, "select"))
                {
                    FadeOutDimmer();
                    CloseDialog(g_objDialog, 400);
                    result = TRANSFER_NO;
                    running = 0;
                }
                else
                {
                    CloseDialog(g_objDialog, 50);
                    new save_error;
                    SendMessage(page, TAG("ACT"), TAG("SEND"), 0, 0, 0);
                    WaitRecvMessage(TAG("ACWT"), index, save_error);
                    if (action == TAG("SEND") && 0 != mx_IsReadySend())
                    {
                        OpenMessageDialog("msgmailer_dlg_send_nodata", focus, 50, DLG_MESSAGE);
                        error = ERR_NODATA;
                        result = TRANSFER_ENDED;
                    }
                    else if (action == TAG("RECV") && 0 != mx_IsReadyRecv())
                    {
                        OpenMessageDialog("msgmailer_dlg_recv_nodata", focus, 50, DLG_MESSAGE);
                        error = ERR_NODATA;
                        result = TRANSFER_ENDED;
                    }
                    else if (!mx_GetNetworkUp())
                    {
                        OpenMessageDialog("msgmailer_dlg_network_noconn", focus, 50, DLG_MESSAGE);
                        error = ERR_NOCONN;
                        result = TRANSFER_ENDED;
                    }
                    else if (save_error == ERR_NOSPACE)
                    {
                        OpenMessageDialog("msgmailer_dlg_send_nospace", focus, 50, DLG_MESSAGE);
                        error = ERR_NOSPACE;
                        result = TRANSFER_ENDED;
                    }
                    else
                    {
                        OpenProgressDialog(prog_msg, focus, 50, DLG_PROGRESS);
                        queue_line = index;
                        start = 1;
                    }
                }
            }
            else if (g_iDialogType == DLG_PROGRESS)
            {
                if (!completed)
                {
                    CloseDialog(g_objDialog, 50);
                    OpenYesNoDialog(cancel_msg, focus, select, 50, DLG_CANCEL);
                    SetPubVar(g_objDialog, "dlg_active", 1);
                }
                else
                {
                    wait_ms(200);
                    CloseDialog(g_objDialog, 50);
                    if (action == TAG("SEND"))
                    {
                        error = mx_SyncSendError();
                        if (0 == error)
                        {
                            if (new_no < 0)
                            {
                                new_no = 0;
                            }
                            index = mx_MsgNum2Line(FOLDER_OUTBOX, new_no);
                            OpenMessageDialog(ok_msg, focus, 50, DLG_MESSAGE);
                        }
                        else if (error == ERR_NOSPACE)
                        {
                            OpenMessageDialog("msgmailer_dlg_send_nospace", focus, 50, DLG_MESSAGE);
                        }
                        else
                        {
                            if (new_no < 0)
                            {
                                new_no = 0;
                            }
                            index = mx_MsgNum2Line(FOLDER_QUEUE, new_no);
                            OpenNumberDialog("msgmailer_dlg_send_err", focus, -54099, "%ERRNO", 50, DLG_MESSAGE);
                        }
                    }
                    else if (action == TAG("RECV"))
                    {
                        error = mx_SyncRecvError();
                        if (0 == error)
                        {
                            SystemCommand(SYSCMD_CLEAR_BIFF);
                            OpenMessageDialog(ok_msg, focus, 50, DLG_MESSAGE);
                        }
                        else if (error == ERR_NOSPACE)
                        {
                            OpenMessageDialog("msgmailer_dlg_recv_nospace", focus, 50, DLG_MESSAGE);
                        }
                        else if (error == PS_SOCKET)
                        {
                            OpenNumberDialog("msgmailer_dlg_recv_err", focus, -54501, "%ERRNO", 50, DLG_MESSAGE);
                        }
                        else if (error == PS_AUTHFAIL)
                        {
                            OpenNumberDialog("msgmailer_dlg_recv_err", focus, -54502, "%ERRNO", 50, DLG_MESSAGE);
                        }
                        else if (error == PS_PROTOCOL)
                        {
                            OpenNumberDialog("msgmailer_dlg_recv_err", focus, -54503, "%ERRNO", 50, DLG_MESSAGE);
                        }
                        else if (error == PS_IOERR)
                        {
                            OpenNumberDialog("msgmailer_dlg_recv_err", focus, -54504, "%ERRNO", 50, DLG_MESSAGE);
                        }
                        else if (error == PS_LOCKBUSY)
                        {
                            OpenNumberDialog("msgmailer_dlg_recv_err", focus, -54505, "%ERRNO", 50, DLG_MESSAGE);
                        }
                        else
                        {
                            OpenNumberDialog("msgmailer_dlg_recv_err", focus, -54599, "%ERRNO", 50, DLG_MESSAGE);
                        }
                    }
                    result = TRANSFER_ENDED;
                }
            }
            else if (g_iDialogType == DLG_MESSAGE)
            {
                FadeOutDimmer();
                CloseDialog(g_objDialog, 400);
                running = 0;
            }
            else if (g_iDialogType == DLG_CANCEL)
            {
                if (ANSWER_YES != GetPubVar(g_objDialog, "select"))
                {
                    CloseDialog(g_objDialog, 50);
                    OpenProgressDialog(prog_msg, focus, 50, DLG_PROGRESS);
                    SetPubVar(g_objDialog, "dlg_active", 1);
                    start = 1;
                }
                else
                {
                    result = TRANSFER_CANCELLED;
                    CloseDialog(g_objDialog, 50);
                    OpenNumberDialog(cancel_ok_msg, focus, count, "%MAILNO", 50, DLG_MESSAGE);
                }
            }
        }
        sleep;
    }
    Destroy(g_objDimmer);
    if (page != Self())
    {
        SendMessage(page, TAG("BLNK"), TAG("IN"), 0, 0, 0);
        SendMessage(page, TAG("FOCS"), TAG("IN"), 0, 0, 0);
    }
    sleep;
    return result;
}

/*
 * Sends the whole queue (action "SEND") or receives the mail ("RECV") with a progress dialog and shows the
 * result. Returns TRANSFER_ENDED (error) or TRANSFER_CANCELLED; count gets the mails transferred.
 */
RunTransfer(action, prog_msg[], done_msg[], cancel_msg[], cancel_ok_msg[], select, focus, page, &error, &count)
{
    if (page != Self())
    {
        SetPubVar(page, "g_active", 0);
        SendMessage(page, TAG("FOCS"), TAG("OUT"), 0, 0, 0);
        SendMessage(page, TAG("BLNK"), TAG("OUT"), 0, 0, 0);
    }
    new mail_total = -1;
    new result;
    new start = 1;
    new completed;
    new queue_line = -1;
    new over_count;
    new running = 1;
    new total;
    if (action == TAG("SEND") && 0 != mx_IsReadySend())
    {
        OpenMessageDialog("msgmailer_dlg_send_nodata", focus, 400, DLG_MESSAGE);
        error = ERR_NODATA;
        result = TRANSFER_ENDED;
    }
    else if (action == TAG("RECV") && 0 != mx_IsReadyRecv())
    {
        OpenMessageDialog("msgmailer_dlg_recv_nodata", focus, 400, DLG_MESSAGE);
        error = ERR_NODATA;
        result = TRANSFER_ENDED;
    }
    else if (!mx_GetNetworkUp())
    {
        OpenMessageDialog("msgmailer_dlg_network_noconn", focus, 400, DLG_MESSAGE);
        error = ERR_NOCONN;
        result = TRANSFER_ENDED;
    }
    else
    {
        OpenProgressDialog(prog_msg, focus, 400, DLG_PROGRESS);
        start = 1;
    }
    while (running)
    {
        if (g_iDialogType == DLG_PROGRESS && start)
        {
            if (action == TAG("SEND"))
            {
                completed = RunSend(count, mail_total, g_objDialog, focus, queue_line);
            }
            else if (action == TAG("RECV"))
            {
                completed = RunRecv(count, mail_total, g_objDialog, focus, over_count);
            }
            start = 0;
        }
        if (!GetPubVar(g_objDialog, "dlg_active"))
        {
            if (g_iDialogType == DLG_PROGRESS)
            {
                if (!completed)
                {
                    CloseDialog(g_objDialog, 50);
                    OpenYesNoDialog(cancel_msg, focus, select, 50, DLG_YESNO);
                    SetPubVar(g_objDialog, "dlg_active", 1);
                }
                else
                {
                    wait_ms(200);
                    CloseDialog(g_objDialog, 50);
                    if (action == TAG("SEND"))
                    {
                        error = mx_SyncSendError();
                        if (0 == error)
                        {
                            count = total + count;
                            if (!count)
                            {
                                OpenMessageDialog("msgmailer_dlg_send_nomail", focus, 50, DLG_MESSAGE);
                            }
                            else
                            {
                                OpenNumberDialog(done_msg, focus, count, "%MAILNO", 50, DLG_MESSAGE);
                            }
                        }
                        else if (error == ERR_NOSPACE)
                        {
                            OpenMessageDialog("msgmailer_dlg_send_nospace", focus, 50, DLG_MESSAGE);
                        }
                        else
                        {
                            OpenNumberDialog("msgmailer_dlg_send_err", focus, -54099, "%ERRNO", 50, DLG_MESSAGE);
                        }
                    }
                    else if (action == TAG("RECV"))
                    {
                        error = mx_SyncRecvError();
                        if (0 == error)
                        {
                            count = total + count;
                            if (over_count <= 0)
                            {
                                SystemCommand(SYSCMD_CLEAR_BIFF);
                                if (!count)
                                {
                                    OpenMessageDialog("msgmailer_dlg_recv_done_nomail", focus, 50, DLG_MESSAGE);
                                }
                                else
                                {
                                    OpenNumberDialog(done_msg, focus, count, "%MAILNO", 50, DLG_MESSAGE);
                                }
                            }
                            else
                            {
                                OpenSizeOverDialog(focus, count, over_count, 50, DLG_MESSAGE);
                            }
                        }
                        else if (error == ERR_NOSPACE)
                        {
                            OpenMessageDialog("msgmailer_dlg_recv_nospace", focus, 50, DLG_MESSAGE);
                        }
                        else if (error == PS_SOCKET)
                        {
                            OpenNumberDialog("msgmailer_dlg_recv_err", focus, -54501, "%ERRNO", 50, DLG_MESSAGE);
                        }
                        else if (error == PS_AUTHFAIL)
                        {
                            OpenNumberDialog("msgmailer_dlg_recv_err", focus, -54502, "%ERRNO", 50, DLG_MESSAGE);
                        }
                        else if (error == PS_PROTOCOL)
                        {
                            OpenNumberDialog("msgmailer_dlg_recv_err", focus, -54503, "%ERRNO", 50, DLG_MESSAGE);
                        }
                        else if (error == PS_IOERR)
                        {
                            OpenNumberDialog("msgmailer_dlg_recv_err", focus, -54504, "%ERRNO", 50, DLG_MESSAGE);
                        }
                        else if (error == PS_LOCKBUSY)
                        {
                            OpenNumberDialog("msgmailer_dlg_recv_err", focus, -54505, "%ERRNO", 50, DLG_MESSAGE);
                        }
                        else
                        {
                            OpenNumberDialog("msgmailer_dlg_recv_err", focus, -54599, "%ERRNO", 50, DLG_MESSAGE);
                        }
                    }
                    result = TRANSFER_ENDED;
                }
            }
            else if (g_iDialogType == DLG_MESSAGE)
            {
                FadeOutDimmer();
                CloseDialog(g_objDialog, 400);
                running = 0;
            }
            else if (g_iDialogType == DLG_YESNO)
            {
                if (ANSWER_YES != GetPubVar(g_objDialog, "select"))
                {
                    total = total + count;
                    CloseDialog(g_objDialog, 50);
                    OpenProgressDialog(prog_msg, focus, 50, DLG_PROGRESS);
                    SetPubVar(g_objDialog, "dlg_active", 1);
                    start = 1;
                }
                else
                {
                    count = total + count;
                    result = TRANSFER_CANCELLED;
                    CloseDialog(g_objDialog, 50);
                    OpenNumberDialog(cancel_ok_msg, focus, count, "%MAILNO", 50, DLG_MESSAGE);
                }
            }
        }
        sleep;
    }
    Destroy(g_objDimmer);
    if (page != Self())
    {
        SendMessage(page, TAG("BLNK"), TAG("IN"), 0, 0, 0);
        SendMessage(page, TAG("FOCS"), TAG("IN"), 0, 0, 0);
    }
    sleep;
    return result;
}

new g_iScreenHist; /* the ScreenBit of the screens opened since the list */
new g_bQuit;

main()
{
    mx_Init();
    InitMailer();
    LoadImages();
    if (ReadArgs())
    {
        if (CheckBootWrite())
        {
            ShowWrite(WRITE_FROM_ARGS);
        }
    }
    else if (CheckBoot())
    {
        ShowList(0, 1);
    }
    while (!g_bQuit)
    {
        new pressed;
        if (g_bOptShown || g_bGblShown || GetPubVar(g_objPage, "g_active"))
        {
            PadState(_, pressed, _, _, -1);
        }
        if (!g_bGblShown)
        {
            if (pressed & PAD_SELECT)
            {
                sndSePlay(SE_DIALOG_OPEN);
                SetPubVar(g_objPage, "g_active", 0);
                if (g_bOptShown)
                {
                    HideOpt();
                    while (IsValidObject(g_objOpt))
                    {
                        sleep;
                    }
                }
                SendMessage(g_objPage, TAG("BLNK"), TAG("OUT"), 0, 0, 0);
                ShowGbl();
                pressed = 0;
            }
        }
        else
        {
            if (pressed & (PAD_CROSS | PAD_CIRCLE | PAD_SELECT))
            {
                if (pressed & PAD_CIRCLE)
                {
                    sndSePlay(SE_DECIDE);
                    RunGblMenu();
                }
                else
                {
                    sndSePlay(SE_CANCEL);
                    HideGbl();
                    SendMessage(g_objPage, TAG("FOCS"), TAG("IN"), 0, 0, 0);
                    SendMessage(g_objPage, TAG("BLNK"), TAG("IN"), 0, 0, 0);
                }
            }
            pressed = 0;
        }
        if (g_bOptShown)
        {
            if (pressed & PAD_CIRCLE)
            {
                sndSePlay(SE_DECIDE);
                HideOpt();
                RunOptMenu();
            }
            else if (pressed & PAD_CROSS)
            {
                sndSePlay(SE_CANCEL);
                HideOpt();
                SendMessage(g_objPage, TAG("FOCS"), TAG("IN"), 0, 0, 0);
            }
        }
        else if (pressed & PAD_CIRCLE)
        {
            OnCircle();
        }
        else if (pressed & PAD_CROSS)
        {
            OnCross();
        }
        else if (pressed & PAD_TRIANGLE)
        {
            OnTriangle();
        }
        new msg;
        new a;
        new b;
        new c;
        /*
         * From the pages: "FOCS", "FADE" and "BLNK" with a IN, OUT or DIM are ignored; "DLG" with a "DDEF" asks to
         * restore the default settings, with a "WEXT" the write page b asks whether to write the body ("ACWT" to b
         * with the answer in its g_bdwrite).
         */
        while (RecvMessage(msg, a, b, c))
        {
            if (msg == TAG("FOCS"))
            {
                if (a == TAG("IN"))
                {
                }
                else if (a == TAG("OUT"))
                {
                }
            }
            else if (msg == TAG("FADE"))
            {
                if (a == TAG("IN"))
                {
                }
                else if (a == TAG("OUT"))
                {
                }
                else if (a == TAG("DIM"))
                {
                }
            }
            else if (msg == TAG("BLNK"))
            {
                if (a == TAG("IN"))
                {
                }
                else if (a == TAG("OUT"))
                {
                }
            }
            else if (msg == TAG("DLG"))
            {
                if (a == TAG("DDEF"))
                {
                    if (GetPubVar(g_objPage, "g_active"))
                    {
                        ConfirmAction(TAG("DEFT"), "msgmailer_dlg_set_default", "msgmailer_dlg_set_default_ok",
                                ANSWER_NO, g_objFocus, g_objPage);
                    }
                }
                else if (a == TAG("WEXT"))
                {
                    new answer = AskYesNo("msgmailer_dlg_write_body", ANSWER_NO, g_objFocus, g_objPage);
                    SetPubVar(b, "g_bdwrite", answer);
                    SendMessage(b, TAG("ACWT"), 0, 0, 0, 0);
                }
            }
        }
        new text[32768];
        /* The mail being written, from the write page. */
        while (RecvMessageStr(msg, text, 32768))
        {
            if (msg == TAG("MTO"))
            {
                copy_string(g_szTo, text, 246);
            }
            else if (msg == TAG("MSUB"))
            {
                copy_string(g_szSubject, text, 246);
            }
            else if (msg == TAG("MBDY"))
            {
                copy_string(g_szBody, text, 32768);
            }
        }
        sleep;
    }
    Shutdown();
}

/*
 * Asks the list, read or sort page for its folder and the line of each folder ("LINE"); it answers "FOL",
 * "L0" to "L3" (top, line) and "ACWT".
 */
FetchLines()
{
    if (g_bListShown || g_bReadShown || g_bSortEditShown)
    {
        SendMessage(g_objPage, TAG("ACT"), TAG("LINE"), 0, 0, 0);
        new msg;
        new a;
        new b;
        for (; ; )
        {
            /* "FOL": a the folder; "L0" to "L3": a the top line and b the line of that folder. */
            while (RecvMessage(msg, a, b))
            {
                if (msg == TAG("FOL"))
                {
                    g_iFolder = a;
                }
                if (msg == TAG("L0"))
                {
                    g_iInboxTop = a;
                    g_iInboxLine = b;
                }
                if (msg == TAG("L1"))
                {
                    g_iOutboxTop = a;
                    g_iOutboxLine = b;
                }
                if (msg == TAG("L2"))
                {
                    g_iDraftTop = a;
                    g_iDraftLine = b;
                }
                if (msg == TAG("L3"))
                {
                    g_iQueueTop = a;
                    g_iQueueLine = b;
                }
                if (msg == TAG("ACWT"))
                {
                    goto got_lines;
                }
            }
            sleep;
        }
        got_lines:
        SelectFolderLine();
    }
}

/* Sets g_iLine and g_iTop to the line of the current folder. */
SelectFolderLine()
{
    if (FOLDER_INBOX == g_iFolder)
    {
        g_iTop = g_iInboxTop;
        g_iLine = g_iInboxLine;
    }
    else if (g_iFolder == FOLDER_OUTBOX)
    {
        g_iTop = g_iOutboxTop;
        g_iLine = g_iOutboxLine;
    }
    else if (g_iFolder == FOLDER_DRAFT)
    {
        g_iTop = g_iDraftTop;
        g_iLine = g_iDraftLine;
    }
    else if (g_iFolder == FOLDER_QUEUE)
    {
        g_iTop = g_iQueueTop;
        g_iLine = g_iQueueLine;
    }
}

/* Keeps the item and the edited flag of the write page. */
SaveWriteState()
{
    if (g_bWriteShown)
    {
        g_iWriteItem = GetPubVar(g_objWrite, "g_iIndex");
        g_bWriteEdited = GetPubVar(g_objWrite, "g_revert");
    }
}

/* Keeps the line of the address book. */
SaveAddrState()
{
    if (g_bAddrShown)
    {
        g_iAddrLine = GetPubVar(g_objAddr, "g_line");
        g_iAddrTop = GetPubVar(g_objAddr, "g_top");
    }
}

IsAddrPick()
{
    if (g_iAddrMode == ADDR_MODE_PICK)
    {
        return 1;
    }
    return 0;
}

/* Opens the option menu (mailopt) with the items (OptBit) of the page hist (SCREEN_*). */
ShowOpt(items, hist)
{
    SaveAddrState();
    HideOpt();
    if (!g_bOptShown)
    {
        SendMessage(g_objPage, TAG("FOCS"), TAG("OUT"), 0, 0, 0);
        new args[64];
        g_objOpt = CreateNull(Disp2D(0));
        SetPos(g_objOpt, 0.0, 0.0, 0.0, 0);
        SetOptionI(g_objOpt, TAG("ZSRT"), ZSORT_OVERLAY);
        sprintf(args, "flags=%d;hist=%d;", items, hist);
        AttachScript(g_objOpt, _, "mailopt", args);
        SetPubVar(g_objOpt, "g_manager", Self());
        SetPubVar(g_objOpt, "g_focus", g_objFocus);
        g_OptAddr = 1;
        g_OptLock = 1;
        mx_GetMsgLock(g_iFolder, g_iLine + g_iTop, args, 10);
        g_OptDele = atoi(args) ^ 1;
        wait_message(TAG("SHWT"));
        wait_ms(400);
        g_bOptShown = 1;
    }
}

HideOpt()
{
    if (g_bOptShown)
    {
        SendMessage(g_objOpt, TAG("FADE"), TAG("OUT"), 0, 0, 0);
        SendMessage(g_objOpt, TAG("FOCS"), TAG("OUT"), 0, 0, 0);
        g_iOptMenu = GetPubVar(g_objOpt, "g_iOptMenu");
        wait_ms(400);
        Destroy(g_objOpt);
        g_bOptShown = 0;
    }
}

/* Opens the global menu (mailgbl). */
ShowGbl()
{
    FetchLines();
    SaveAddrState();
    HideGbl();
    if (!g_bGblShown)
    {
        SendMessage(g_objPage, TAG("FOCS"), TAG("OUT"), 0, 0, 0);
        g_objGbl = CreateNull(Disp2D(0));
        SetPos(g_objGbl, 0.0, 0.0, 0.0, 0);
        SetOptionI(g_objGbl, TAG("ZSRT"), ZSORT_OVERLAY);
        AttachScript(g_objGbl, _, "mailgbl", "");
        SetPubVar(g_objGbl, "g_manager", Self());
        SetPubVar(g_objGbl, "g_focus", g_objFocus);
        SetPubVar(g_objGbl, "g_iGblMenu", g_iGblMenu);
        wait_message(TAG("SHWT"));
        wait_ms(500);
        g_bGblShown = 1;
    }
}

HideGbl()
{
    if (g_bGblShown)
    {
        SendMessage(g_objGbl, TAG("FADE"), TAG("OUT"), 0, 0, 0);
        SendMessage(g_objGbl, TAG("FOCS"), TAG("OUT"), 0, 0, 0);
        wait_ms(500);
        Destroy(g_objGbl);
        g_bGblShown = 0;
    }
}

/* Opens the list (maillist) on folder, 0 for the current one; with focus_in it takes the focus. */
ShowList(folder, focus_in)
{
    if (!g_bListShown)
    {
        if (0 != folder)
        {
            g_iFolder = folder;
            g_iLine = 0;
            g_iTop = 0;
        }
        new args[64];
        g_objList = CreateNull(Disp2D(0));
        SetPos(g_objList, 0.0, 0.0, 0.0, 0);
        SetOptionI(g_objList, TAG("ZSRT"), ZSORT_OVERLAY);
        sprintf(args, "folder=%d;line0=%d;top0=%d;line1=%d;top1=%d;line2=%d;top2=%d;line3=%d;top3=%d;", g_iFolder,
                g_iInboxLine, g_iInboxTop, g_iOutboxLine, g_iOutboxTop, g_iDraftLine, g_iDraftTop, g_iQueueLine,
                g_iQueueTop);
        AttachScript(g_objList, _, "maillist", args);
        SetPubVar(g_objList, "g_manager", Self());
        SetPubVar(g_objList, "g_focus", g_objFocus);
        if (focus_in)
        {
            SendMessage(g_objList, TAG("FOCS"), TAG("IN"), 0, 0, 0);
        }
        wait_message(TAG("SHWT"));
        wait_ms(400);
        g_iScreenHist = ScreenBit(SCREEN_LIST);
        g_bListShown = 1;
        SendMessage(g_objList, TAG("BLNK"), TAG("IN"), 0, 0, 0);
    }
    g_objPage = g_objList;
}

HideList()
{
    if (g_bListShown)
    {
        FetchLines();
        SendMessage(g_objList, TAG("FOCS"), TAG("OUT"), 0, 0, 0);
        SendMessage(g_objList, TAG("FADE"), TAG("OUT"), 0, 0, 0);
        SendMessage(g_objList, TAG("BLNK"), TAG("OUT"), 0, 0, 0);
        wait_ms(400);
        Destroy(g_objList);
        g_bListShown = 0;
        sleep;
    }
}

/* Opens the selected mail (mailread); with focus_in it takes the focus. */
ShowRead(focus_in)
{
    if (!g_bReadShown)
    {
        new args[64];
        g_objRead = CreateNull(Disp2D(0));
        SetPos(g_objRead, 0.0, 0.0, 0.0, 0);
        SetOptionI(g_objRead, TAG("ZSRT"), ZSORT_OVERLAY);
        sprintf(args, "folder=%d;line0=%d;top0=%d;line1=%d;top1=%d;line2=%d;top2=%d;line3=%d;top3=%d;", g_iFolder,
                g_iInboxLine, g_iInboxTop, g_iOutboxLine, g_iOutboxTop, g_iDraftLine, g_iDraftTop, g_iQueueLine,
                g_iQueueTop);
        AttachScript(g_objRead, _, "mailread", args);
        SetPubVar(g_objRead, "g_manager", Self());
        SetPubVar(g_objRead, "g_focus", g_objFocus);
        if (focus_in)
        {
            SendMessage(g_objRead, TAG("FOCS"), TAG("IN"), 0, 0, 0);
        }
        wait_message(TAG("SHWT"));
        wait_ms(400);
        g_objPage = g_objRead;
        g_iScreenHist = g_iScreenHist | ScreenBit(SCREEN_READ);
        g_bReadShown = 1;
    }
}

HideRead()
{
    if (g_bReadShown)
    {
        FetchLines();
        SendMessage(g_objRead, TAG("FOCS"), TAG("OUT"), 0, 0, 0);
        SendMessage(g_objRead, TAG("FADE"), TAG("OUT"), 0, 0, 0);
        wait_ms(400);
        Destroy(g_objRead);
        g_bReadShown = 0;
        sleep;
    }
}

/* Opens the write page (mailwrite) in mode (WRITE_*), with the recipients, subject and body kept here. */
ShowWrite(mode)
{
    if (!g_bWriteShown)
    {
        new args[64];
        new to_too_long;
        if (WRITE_ADD_ADDR != mode && WRITE_RETURN != mode)
        {
            g_iWriteItem = 0;
            g_iWriteMode = mode;
        }
        sprintf(args, "send=%d;old=%d;folder=%d;line=%d;", mode, g_iWriteMode, g_iFolder, g_iLine + g_iTop);
        g_objWrite = CreateNull(Disp2D(0));
        AttachScript(g_objWrite, _, "mailwrite", args);
        SetPubVar(g_objWrite, "g_manager", Self());
        SetPubVar(g_objWrite, "g_focus", g_objFocus);
        SetPubVar(g_objWrite, "g_iIndex", g_iWriteItem);
        g_objPage = g_objWrite;
        if (mode == WRITE_ADD_ADDR)
        {
            new addr[65];
            new to[246];
            mx_GetAddr2(g_iAddrLine + g_iAddrTop, addr, 259);
            if (strlen(g_szTo))
            {
                if (strlen(g_szTo) < 984 - strlen(addr) - strlen(g_szAddrSep))
                {
                    sprintf(to, "%s%s%s", g_szTo, g_szAddrSep, addr);
                }
                else
                {
                    sprintf(to, "%s", g_szTo);
                    to_too_long = 1;
                }
            }
            else
            {
                sprintf(to, "%s", addr);
            }
            SendMessageStr(g_objPage, TAG("FROM"), to);
            SendMessageStr(g_objPage, TAG("SUBJ"), g_szSubject);
            SendMessageStr(g_objPage, TAG("BODY"), g_szBody);
        }
        else if (mode == WRITE_RETURN)
        {
            SendMessageStr(g_objPage, TAG("FROM"), g_szTo);
            SendMessageStr(g_objPage, TAG("SUBJ"), g_szSubject);
            SendMessageStr(g_objPage, TAG("BODY"), g_szBody);
        }
        else if (mode == WRITE_FROM_ARGS)
        {
            SendMessageStr(g_objPage, TAG("FROM"), g_szTo);
            SendMessageStr(g_objPage, TAG("SUBJ"), g_szSubject);
            SendMessageStr(g_objPage, TAG("BYCR"), g_szBody);
        }
        wait_message(TAG("SHWT"));
        wait_ms(400);
        g_iScreenHist = g_iScreenHist | ScreenBit(SCREEN_WRITE);
        g_bWriteShown = 1;
        if (mode == WRITE_RETURN)
        {
            SetPubVar(g_objWrite, "g_revert", g_bWriteEdited);
        }
        if (to_too_long)
        {
            ShowMessage("msgmailer_dlg_write_to_toolong", g_objFocus, g_objPage);
        }
        SendMessage(g_objWrite, TAG("BLNK"), TAG("IN"), 0, 0, 0);
    }
}

HideWrite()
{
    if (g_bWriteShown)
    {
        SendMessage(g_objWrite, TAG("FOCS"), TAG("OUT"), 0, 0, 0);
        SendMessage(g_objWrite, TAG("FADE"), TAG("OUT"), 0, 0, 0);
        SendMessage(g_objWrite, TAG("BLNK"), TAG("OUT"), 0, 0, 0);
        wait_ms(400);
        Destroy(g_objWrite);
        g_bWriteShown = 0;
        sleep;
    }
}

/* Opens the address book (mailaddr) at top and line in mode (ADDR_MODE_*). */
ShowAddr(top, line, mode)
{
    if (!g_bAddrShown)
    {
        new args[64];
        if (g_iAddrLine != line)
        {
            g_iAddrLine = line;
        }
        if (g_iAddrTop != top)
        {
            g_iAddrTop = top;
        }
        g_iAddrMode = mode;
        g_objAddr = CreateNull(Disp2D(0));
        sprintf(args, "line=%d;top=%d;mode=%d;", g_iAddrLine, g_iAddrTop, mode);
        AttachScript(g_objAddr, _, "mailaddr", args);
        SetPubVar(g_objAddr, "g_manager", Self());
        SetPubVar(g_objAddr, "g_focus", g_objFocus);
        g_objPage = g_objAddr;
        wait_message(TAG("SHWT"));
        wait_ms(400);
        if (ADDR_MODE_BOOK == mode)
        {
            g_iScreenHist = g_iScreenHist | ScreenBit(SCREEN_ADDR);
        }
        else
        {
            g_iScreenHist = g_iScreenHist | ScreenBit(SCREEN_ADDR_PICK);
        }
        g_bAddrShown = 1;
        SendMessage(g_objAddr, TAG("BLNK"), TAG("IN"), 0, 0, 0);
    }
}

HideAddr()
{
    if (g_bAddrShown)
    {
        SaveAddrState();
        SendMessage(g_objAddr, TAG("FOCS"), TAG("OUT"), 0, 0, 0);
        SendMessage(g_objAddr, TAG("FADE"), TAG("OUT"), 0, 0, 0);
        SendMessage(g_objAddr, TAG("BLNK"), TAG("OUT"), 0, 0, 0);
        wait_ms(400);
        wait_ms(120);
        Destroy(g_objAddr);
        g_bAddrShown = 0;
        sleep;
    }
}

/* Opens the address editor (mailaedit) in mode (AEDIT_*). */
ShowAddrEdit(mode)
{
    if (!g_bAddrEditShown)
    {
        new args[64];
        g_iAddrEditMode = mode;
        g_objAddrEdit = CreateNull(Disp2D(0));
        sprintf(args, "mode=%d;", g_iAddrEditMode);
        AttachScript(g_objAddrEdit, _, "mailaedit", args);
        SetPubVar(g_objAddrEdit, "g_manager", Self());
        SetPubVar(g_objAddrEdit, "g_focus", g_objFocus);
        if (AEDIT_CHANGE == mode)
        {
            SetPubVar(g_objAddrEdit, "g_select", g_iAddrTop + g_iAddrLine);
        }
        else
        {
            SetPubVar(g_objAddrEdit, "g_select", -1);
        }
        g_objPage = g_objAddrEdit;
        wait_message(TAG("SHWT"));
        wait_ms(400);
        g_iScreenHist = g_iScreenHist | ScreenBit(SCREEN_ADDR_EDIT);
        g_bAddrEditShown = 1;
        SendMessage(g_objAddrEdit, TAG("BLNK"), TAG("IN"), 0, 0, 0);
    }
}

HideAddrEdit()
{
    if (g_bAddrEditShown)
    {
        SendMessage(g_objAddrEdit, TAG("FOCS"), TAG("OUT"), 0, 0, 0);
        SendMessage(g_objAddrEdit, TAG("FADE"), TAG("OUT"), 0, 0, 0);
        SendMessage(g_objAddrEdit, TAG("BLNK"), TAG("OUT"), 0, 0, 0);
        wait_ms(400);
        Destroy(g_objAddrEdit);
        g_bAddrEditShown = 0;
        sleep;
    }
}

/* Opens the settings menu (mailset); with reset on its first item. */
ShowSet(reset)
{
    if (!g_bSetShown)
    {
        if (reset)
        {
            g_iSetMenu = 0;
        }
        g_objSet = CreateNull(Disp2D(0));
        AttachScript(g_objSet, _, "mailset", "");
        SetPubVar(g_objSet, "g_manager", Self());
        SetPubVar(g_objSet, "g_focus", g_objFocus);
        SetPubVar(g_objSet, "g_iSetMenu", g_iSetMenu);
        g_objPage = g_objSet;
        wait_message(TAG("SHWT"));
        wait_ms(400);
        g_iScreenHist = g_iScreenHist | ScreenBit(SCREEN_SET);
        g_bSetShown = 1;
    }
}

HideSet()
{
    if (g_bSetShown)
    {
        SendMessage(g_objSet, TAG("FOCS"), TAG("OUT"), 0, 0, 0);
        SendMessage(g_objSet, TAG("FADE"), TAG("OUT"), 0, 0, 0);
        g_iSetMenu = GetPubVar(g_objSet, "g_iSetMenu");
        wait_ms(400);
        Destroy(g_objSet);
        g_bSetShown = 0;
        sleep;
    }
}

/*
 * Opens the page of setting set_item; coming back from the editor of an item (SEDIT_FROM_PART) it gets
 * edit_item and part_value, the value chosen there.
 */
ShowSetEdit(set_item, from, edit_item, part_value)
{
    if (!g_bSetEditShown)
    {
        new args[64];
        g_iSetEditFrom = from;
        g_objSetEdit = CreateNull(Disp2D(0));
        new items[4][16];
        GetItemIds("SET_ITEM", items);
        if (!strcmp(items[set_item], "MAILER_SET_FOUND"))
        {
            get_tag_attr("TEXT", "msgmailer_set_item_basic", "value", args, 64);
            sprintf(args, "top=%s;", args);
            AttachScript(g_objSetEdit, _, "mailseditfound", args);
        }
        else if (!strcmp(items[set_item], "MAILER_SET_RECEPT"))
        {
            get_tag_attr("TEXT", "msgmailer_set_item_recv", "value", args, 64);
            sprintf(args, "top=%s;", args);
            if (from == SEDIT_FROM_PART)
            {
                sprintf(args, "%sedit=%d;parts=%d;", args, edit_item, part_value);
            }
            AttachScript(g_objSetEdit, _, "mailseditrecept", args);
        }
        else if (!strcmp(items[set_item], "MAILER_SET_CREATE"))
        {
            get_tag_attr("TEXT", "msgmailer_set_item_write", "value", args, 64);
            sprintf(args, "top=%s;", args);
            if (from == SEDIT_FROM_PART)
            {
                sprintf(args, "%sedit=%d;parts=%d;", args, edit_item, part_value);
            }
            AttachScript(g_objSetEdit, _, "mailseditcreate", args);
        }
        else if (!strcmp(items[set_item], "MAILER_SET_DETAIL"))
        {
            get_tag_attr("TEXT", "msgmailer_set_item_detail", "value", args, 64);
            sprintf(args, "top=%s;", args);
            if (from == SEDIT_FROM_PART)
            {
                sprintf(args, "%sedit=%d;parts=%d;", args, edit_item, part_value);
            }
            AttachScript(g_objSetEdit, _, "mailseditdetail", args);
        }
        SetPubVar(g_objSetEdit, "g_manager", Self());
        SetPubVar(g_objSetEdit, "g_focus", g_objFocus);
        SetPubVar(g_objSetEdit, "g_iSetMenu", edit_item);
        g_objPage = g_objSetEdit;
        wait_message(TAG("SHWT"));
        wait_ms(400);
        g_iScreenHist = g_iScreenHist | ScreenBit(SCREEN_SET_EDIT);
        g_bSetEditShown = 1;
        SendMessage(g_objSetEdit, TAG("BLNK"), TAG("IN"), 0, 0, 0);
    }
}

HideSetEdit()
{
    if (g_bSetEditShown)
    {
        SendMessage(g_objSetEdit, TAG("FOCS"), TAG("OUT"), 0, 0, 0);
        SendMessage(g_objSetEdit, TAG("FADE"), TAG("OUT"), 0, 0, 0);
        SendMessage(g_objSetEdit, TAG("BLNK"), TAG("OUT"), 0, 0, 0);
        wait_ms(400);
        Destroy(g_objSetEdit);
        g_bSetEditShown = 0;
        sleep;
    }
}

/* Opens the editor of item edit_item of setting set_item, a radio button or a spin number, with args. */
ShowPart(args[], set_item, edit_item)
{
    if (!g_bPartShown)
    {
        g_objPart = CreateNull(Disp2D(0));
        new items[10][16];
        GetItemIds("SET_ITEM", items);
        if (!strcmp(items[set_item], "MAILER_SET_RECEPT"))
        {
            GetItemIds("SEDIT_RECEPT_ITEM", items);
            if (!strcmp(items[edit_item], "MAILER_SEDIT_RECEPT_CHECK"))
            {
                AttachScript(g_objPart, _, "mailradiobutton", args);
                sprintf(g_szPartArgs, args);
            }
            if (!strcmp(items[edit_item], "MAILER_SEDIT_RECEPT_SOUND"))
            {
                AttachScript(g_objPart, _, "mailradiobutton", args);
                sprintf(g_szPartArgs, args);
            }
            else if (!strcmp(items[edit_item], "MAILER_SEDIT_RECEPT_SIZE"))
            {
                AttachScript(g_objPart, _, "mailspinnumber", args);
            }
            else
            {
            }
        }
        else if (!strcmp(items[set_item], "MAILER_SET_CREATE"))
        {
            GetItemIds("SEDIT_CREATE_ITEM", items);
            if (!strcmp(items[edit_item], "MAILER_SEDIT_CREATE_SIGN"))
            {
                AttachScript(g_objPart, _, "mailradiobutton", args);
            }
            else if (!strcmp(items[edit_item], "MAILER_SEDIT_CREATE_MARK"))
            {
                AttachScript(g_objPart, _, "mailradiobutton", args);
            }
            else
            {
            }
        }
        else if (!strcmp(items[set_item], "MAILER_SET_DETAIL"))
        {
            GetItemIds("SEDIT_DETAIL_ITEM", items);
            if (!strcmp(items[edit_item], "MAILER_SEDIT_DETAIL_CERTIFY"))
            {
                AttachScript(g_objPart, _, "mailradiobutton", args);
            }
            else if (!strcmp(items[edit_item], "MAILER_SEDIT_DETAIL_MAIL"))
            {
                AttachScript(g_objPart, _, "mailradiobutton", args);
            }
            else if (!strcmp(items[edit_item], "MAILER_SEDIT_DETAIL_SMTP"))
            {
                AttachScript(g_objPart, _, "mailspinnumber", args);
            }
            else if (!strcmp(items[edit_item], "MAILER_SEDIT_DETAIL_POP3"))
            {
                AttachScript(g_objPart, _, "mailspinnumber", args);
            }
            else if (!strcmp(items[edit_item], "MAILER_SEDIT_DETAIL_POPSMTP"))
            {
                AttachScript(g_objPart, _, "mailradiobutton", args);
            }
            else if (!strcmp(items[edit_item], "MAILER_SEDIT_DETAIL_SMTPAUTH"))
            {
                AttachScript(g_objPart, _, "mailradiobutton", args);
            }
            else
            {
            }
        }
        SetPubVar(g_objPart, "g_manager", Self());
        SetPubVar(g_objPart, "g_focus", g_objFocus);
        g_objPage = g_objPart;
        wait_message(TAG("SHWT"));
        wait_ms(400);
        g_iScreenHist = g_iScreenHist | ScreenBit(SCREEN_SET_PART);
        g_bPartShown = 1;
    }
}

HidePart()
{
    if (g_bPartShown)
    {
        SendMessage(g_objPart, TAG("FOCS"), TAG("OUT"), 0, 0, 0);
        SendMessage(g_objPart, TAG("FADE"), TAG("OUT"), 0, 0, 0);
        wait_ms(400);
        Destroy(g_objPart);
        g_bPartShown = 0;
        sleep;
    }
}

/* Opens the spin number of the interval between two checks of the mail. */
ShowInterval()
{
    if (!g_bIntervalShown)
    {
        g_objInterval = CreateNull(Disp2D(0));
        new args[64];
        new end_fmt[8];
        new end_text[8];
        new label[8];
        new interval;
        mx_GetPref("INTERVAL", interval);
        get_tag_attr("TEXT", "msgmailer_set_item_recv_check_interval_2", "value", end_fmt, 8);
        get_tag_attr("TEXT", "msgmailer_set_item_recv_check_interval", "value", label, 8);
        replace_all(end_text, end_fmt, "%CHKINTV", "", 8);
        sprintf(args, "max=%d;min=%d;spd=%d;label=%s;end=%s;sel=%d;", 60, 3, 4, label, end_text, interval);
        AttachScript(g_objInterval, _, "mailspinnumber", args);
        SetPubVar(g_objInterval, "g_manager", Self());
        SetPubVar(g_objInterval, "g_focus", g_objFocus);
        g_objPage = g_objInterval;
        wait_message(TAG("SHWT"));
        wait_ms(400);
        g_bIntervalShown = 1;
    }
}

HideInterval()
{
    if (g_bIntervalShown)
    {
        SendMessage(g_objInterval, TAG("FOCS"), TAG("OUT"), 0, 0, 0);
        SendMessage(g_objInterval, TAG("FADE"), TAG("OUT"), 0, 0, 0);
        wait_ms(400);
        Destroy(g_objInterval);
        g_bIntervalShown = 0;
        sleep;
    }
}

/* Opens the sort menu (mailsort); without keep_item on its first item. */
ShowSort(keep_item)
{
    if (!g_bSortShown)
    {
        if (0 == keep_item)
        {
            g_iSortMenu = 0;
        }
        g_objSort = CreateNull(Disp2D(0));
        AttachScript(g_objSort, _, "mailsort", "");
        SetPubVar(g_objSort, "g_manager", Self());
        SetPubVar(g_objSort, "g_focus", g_objFocus);
        SetPubVar(g_objSort, "g_iSortMenu", g_iSortMenu);
        g_objPage = g_objSort;
        wait_message(TAG("SHWT"));
        wait_ms(400);
        g_iScreenHist = g_iScreenHist | ScreenBit(SCREEN_SORT);
        g_bSortShown = 1;
    }
}

HideSort()
{
    if (g_bSortShown)
    {
        SendMessage(g_objSort, TAG("FOCS"), TAG("OUT"), 0, 0, 0);
        SendMessage(g_objSort, TAG("FADE"), TAG("OUT"), 0, 0, 0);
        g_iSortMenu = GetPubVar(g_objSort, "g_iSortMenu");
        wait_ms(400);
        Destroy(g_objSort);
        g_bSortShown = 0;
        sleep;
    }
}

/* Opens the sort page (mailstedit) for item g_iSortMenu of the sort menu. */
ShowSortEdit()
{
    if (!g_bSortEditShown)
    {
        new args[64];
        g_objSortEdit = CreateNull(Disp2D(0));
        sprintf(args, "mline=%d;folder=%d;line0=%d;top0=%d;line1=%d;top1=%d;line2=%d;top2=%d;line3=%d;top3=%d;",
                g_iSortMenu, g_iFolder, g_iInboxLine, g_iInboxTop, g_iOutboxLine, g_iOutboxTop, g_iDraftLine,
                g_iDraftTop, g_iQueueLine, g_iQueueTop);
        AttachScript(g_objSortEdit, _, "mailstedit", args);
        SetPubVar(g_objSortEdit, "g_manager", Self());
        SetPubVar(g_objSortEdit, "g_focus", g_objFocus);
        g_objPage = g_objSortEdit;
        wait_message(TAG("SHWT"));
        wait_ms(400);
        g_iScreenHist = g_iScreenHist | ScreenBit(SCREEN_SORT_EDIT);
        g_bSortEditShown = 1;
    }
}

/* Closes the sort page, sorting the folders first when it was closed with circle. */
HideSortEdit()
{
    if (g_bSortEditShown)
    {
        SendMessage(g_objSortEdit, TAG("FOCS"), TAG("OUT"), 0, 0, 0);
        SendMessage(g_objSortEdit, TAG("FADE"), TAG("OUT"), 0, 0, 0);
        wait_ms(400);
        if (g_bSortApply)
        {
            SendMessage(g_objSortEdit, TAG("ACT"), TAG("SSET"), 0, 0, 0);
            wait_message(TAG("ACWT"));
            g_iSortError = GetPubVar(g_objSortEdit, "g_space");
            FetchLines();
        }
        Destroy(g_objSortEdit);
        g_bSortEditShown = 0;
        sleep;
    }
}

/* Opens the refresh page (mailrefresh): receive the mail or send the queue. */
ShowRefresh()
{
    if (!g_bRefreshShown)
    {
        g_objRefresh = CreateNull(Disp2D(0));
        AttachScript(g_objRefresh, _, "mailrefresh", "");
        SetPubVar(g_objRefresh, "g_manager", Self());
        SetPubVar(g_objRefresh, "g_focus", g_objFocus);
        g_objPage = g_objRefresh;
        wait_message(TAG("SHWT"));
        wait_ms(400);
        g_iScreenHist = g_iScreenHist | ScreenBit(SCREEN_REFRESH);
        g_bRefreshShown = 1;
        g_bTransferred = 0;
    }
}

HideRefresh()
{
    if (g_bRefreshShown)
    {
        SendMessage(g_objRefresh, TAG("FOCS"), TAG("OUT"), 0, 0, 0);
        SendMessage(g_objRefresh, TAG("FADE"), TAG("OUT"), 0, 0, 0);
        wait_ms(400);
        Destroy(g_objRefresh);
        g_bRefreshShown = 0;
        sleep;
    }
}

/* Opens the information on the selected mail (mailinfo). */
ShowInfo()
{
    if (!g_bInfoShown)
    {
        new args[64];
        g_objInfo = CreateNull(Disp2D(0));
        sprintf(args, "folder=%d;line=%d;", g_iFolder, g_iLine + g_iTop);
        AttachScript(g_objInfo, _, "mailinfo", args);
        SetPubVar(g_objInfo, "g_manager", Self());
        SetPubVar(g_objInfo, "g_focus", g_objFocus);
        g_objPage = g_objInfo;
        wait_message(TAG("SHWT"));
        wait_ms(400);
        g_iScreenHist = g_iScreenHist | ScreenBit(SCREEN_INFO);
        g_bInfoShown = 1;
    }
}

HideInfo()
{
    if (g_bInfoShown)
    {
        SendMessage(g_objInfo, TAG("FOCS"), TAG("OUT"), 0, 0, 0);
        SendMessage(g_objInfo, TAG("FADE"), TAG("OUT"), 0, 0, 0);
        wait_ms(400);
        Destroy(g_objInfo);
        g_bInfoShown = 0;
        sleep;
    }
}

/* Closes every page and menu. */
HideAll()
{
    HideOpt();
    HideGbl();
    HideList();
    HideRead();
    HideSort();
    HideSortEdit();
    HideSet();
    HideSetEdit();
    HidePart();
    HideInterval();
    HideRefresh();
    HideAddr();
    HideAddrEdit();
    HideWrite();
    HideInfo();
}

/* Goes back to the mail being read when it was open, otherwise to the list. */
ReturnToList(focus_in)
{
    if (g_iScreenHist & ScreenBit(SCREEN_READ))
    {
        ShowRead(focus_in);
    }
    else
    {
        ShowList(g_iFolder, focus_in);
    }
}

/* Reads the to, subject and body arguments of the script; returns whether there was one. */
ReadArgs()
{
    new value[32768];
    new found;
    if (GetArg("to", value, 32768))
    {
        /* NOTE: bug: maxlen is given in characters, but DecodeUrl takes cells: it may write past the buffer. */
        DecodeUrl(value, g_szTo, 984);
        found = 1;
    }
    if (GetArg("subject", value, 32768))
    {
        DecodeUrl(value, g_szSubject, 984);
        found = 1;
    }
    if (GetArg("body", value, 32768))
    {
        DecodeUrl(value, g_szBody, 131072);
        found = 1;
    }
    return found;
}

/* Returns the BOOT_* problems found by mx_Init. */
GetBootFlags()
{
    new flags;
    new error;
    new result;
    mx_GetInitResult(error, result);
    if (!error)
    {
        if (result & INIT_NOACCOUNT)
        {
            flags = flags | BOOT_FATAL;
        }
        else if (result & INIT_BADPASS_SMTP || result & INIT_BADPASS_RECV)
        {
            flags = flags | BOOT_NGPASS;
        }
    }
    else
    {
        flags = flags | BOOT_FATAL;
    }
    if (mx_GetFree() < 0)
    {
        flags = flags | BOOT_NOSPACE;
    }
    if (mx_IsReadyRecv() == 1)
    {
        flags = flags | BOOT_NOTSET;
    }
    return flags;
}

/* Shows the problems found at the start; returns 0 when the list is not to be opened. */
CheckBoot()
{
    new flags = GetBootFlags();
    if (flags & BOOT_FATAL)
    {
        ShowMessage("msgmailer_dlg_boot_fatal", g_objFocus, Self());
        g_bQuit = 1;
        return 0;
    }
    if (flags & BOOT_NOSPACE)
    {
        ShowMessage("msgmailer_dlg_boot_nospace", g_objFocus, Self());
        g_bQuit = 1;
        return 0;
    }
    if (flags & BOOT_NOTSET)
    {
        if (AskStart("msgmailer_dlg_set_start", g_objFocus, Self()))
        {
            ShowSetEdit(0, SEDIT_FROM_BOOT, 0, 0);
        }
        else
        {
            g_bQuit = 1;
        }
        return 0;
    }
    if (flags & BOOT_NGPASS)
    {
        if (AskStart("msgmailer_dlg_boot_ngpass_quit", g_objFocus, Self()))
        {
            ShowSetEdit(0, SEDIT_FROM_BOOT, 0, 0);
        }
        else
        {
            g_bQuit = 1;
        }
        return 0;
    }
    return 1;
}

/* The same for a start with arguments, where every problem quits; returns 1 when the mail can be written. */
CheckBootWrite()
{
    new flags = GetBootFlags();
    if (flags & BOOT_FATAL)
    {
        ShowMessage("msgmailer_dlg_boot_fatal", g_objFocus, Self());
        g_bQuit = 1;
        return 0;
    }
    if (flags & BOOT_NOSPACE)
    {
        ShowMessage("msgmailer_dlg_boot_nospace", g_objFocus, Self());
        g_bQuit = 1;
        return 0;
    }
    if (flags & BOOT_NOTSET)
    {
        ShowMessage("msgmailer_dlg_set_quit", g_objFocus, Self());
        g_bQuit = 1;
        return 0;
    }
    if (flags & BOOT_NGPASS)
    {
        ShowMessage("msgmailer_dlg_boot_ngpass_quit", g_objFocus, Self());
        g_bQuit = 1;
        return 0;
    }
    return 1;
}

/* Sets up the camera, the focus and the background, and clears the state. */
InitMailer()
{
    SetPos(Camera(0), 0.0, 0.0, 300.0, 0);
    LookAt(Camera(0), Scene(0), 0.0, 0.0, 0.0, 0, 0.0, 0.0, 0.0);
    g_objFocus = create_focus_plane2("focus");
    SetOptionI(g_objFocus, TAG("ZSRT"), ZSORT_OVERLAY);
    g_iInboxLine = g_iInboxTop = g_iOutboxLine = g_iOutboxTop = g_iDraftLine = g_iDraftTop = g_iQueueLine =
            g_iQueueTop = g_iFolder = g_iLine = g_iTop = g_iWriteItem = g_iWriteMode = g_iAddrLine = g_iAddrTop =
            g_iAddrMode = g_iAddrEditMode = g_iOptMenu = g_iGblMenu = g_iSetMenu = g_iSetEditItem = g_iSortMenu = 0;
    g_bListShown = g_bReadShown = g_bWriteShown = g_bAddrShown = g_bAddrEditShown = g_bSetShown = g_bSetEditShown =
            g_bPartShown = g_bIntervalShown = g_bSortShown = g_bSortEditShown = g_bRefreshShown = g_bInfoShown = 0;
    s_bSecondFlg = 1;
    g_iScreenHist = 0;
    g_bQuit = 0;
    copy_string(g_szTo, "", 984);
    copy_string(g_szSubject, "", 984);
    copy_string(g_szBody, "", 131072);
    LoadImage("bg", g_txBg, g_BgWidth, g_BgHeight);
    g_objBg = CreatePlane(Disp2D(0));
    SetPos(g_objBg, 320.0, 240.0, 0.0, 0);
    SetScale(g_objBg, g_BgWidth, g_BgHeight, 0.0);
    SetColor(g_objBg, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_objBg, TAG("ZSRT"), ZSORT_BACKGROUND);
    SetOptionI(g_objBg, TAG("ALPH"), BLEND_ADD);
    SetTexture(g_objBg, g_txBg, 0, -1);
    AnimColor(g_objBg, 1.0, 1.0, 1.0, 1.0, float(400), 0.0, ANIM_FUNC_LINEAR);
}

/* Loads the textures that the pages share. */
LoadImages()
{
    SystemCommand(SYSCMD_BUSY, 1);
    LoadImage("arrow", g_txArrow, g_ArrowWidth, g_ArrowHeight);
    LoadImage("arrow_glow", g_txArrowGlow, g_ArrowGlowWidth, g_ArrowGlowHeight);
    LoadImage("list_focus", g_txListFg, g_ListFgWidth, g_ListFgHeight);
    LoadImage("list_back", g_txListBg, g_ListBgWidth, g_ListBgHeight);
    LoadImage("addr_back", g_txAddrGrid, g_AddrGridWidth, g_AddrGridHeight);
    LoadImage("addr_focus", g_txAddrEdit, g_AddrEditWidth, g_AddrEditHeight);
    LoadImage("aedit_back", g_txAddrEditGrid, g_AddrEdGridWidth, g_AddrEdGridHeight);
    LoadImage("addr_top", g_txAddrTop, g_AddrTopWidth, g_AddrTopHeight);
    LoadImage("opt_back", g_txBgOpt, g_BgOptWidth, g_BgOptHeight);
    LoadImage("separate", g_txSeparate, g_SeparateWidth, g_SeparateHeight);
    LoadImage("gbl_title", g_txBgGbl, g_BgGblWidth, g_BgGblHeight);
    LoadImage("from_back", g_txFromBack, g_FromBackWidth, g_FromBackHeight);
    LoadImage("to_back", g_txToBack, g_ToBackWidth, g_ToBackHeight);
    LoadImage("title_back", g_txTitleBack, g_TitleBackWidth, g_TitleBackHeight);
    LoadImage("date_back", g_txDateBack, g_DateBackWidth, g_DateBackHeight);
    LoadImage("box_inbox", g_txBoxInbox, g_BoxInboxWidth, g_BoxInboxHeight);
    LoadImage("box_outbox", g_txBoxOutbox, g_BoxOutboxWidth, g_BoxOutboxHeight);
    LoadImage("box_draft", g_txBoxDraft, g_BoxDraftWidth, g_BoxDraftHeight);
    LoadImage("box_queue", g_txBoxQueue, g_BoxQueueWidth, g_BoxQueueHeight);
    LoadImage("A01", g_txA01, g_AWidth, g_AHeight);
    LoadImage("A03", g_txA03);
    LoadImage("A05", g_txA05);
    LoadImage("A07", g_txA07);
    LoadImage("A09", g_txA09);
    LoadImage("A11", g_txA11);
    LoadImage("A13", g_txA13);
    LoadImage("A15", g_txA15);
    SystemCommand(SYSCMD_BUSY, 0);
}

/* Fades out the background, waits for the page to go and destroys the scene. */
Shutdown()
{
    SetSystemParam(SYS_PADHELP, 0);
    AnimColor(g_objBg, 1.0, 1.0, 1.0, 0.0, float(400), 0.0, ANIM_FUNC_LINEAR);
    for (; ; )
    {
        sleep;
        if (ANIM_DONE == AnimGetState(g_objBg, ANIM_COLOR))
        {
            break;
        }
    }
    while (IsValidObject(g_objPage))
    {
        sleep;
    }
    Destroy(Scene(0));
}

/* Runs the item chosen in the global menu. */
RunGblMenu()
{
    g_iGblMenu = GetPubVar(g_objGbl, "g_iGblMenu");
    HideGbl();
    if (g_iGblMenu == GetItemIndex("GBL_ITEM", "MAILER_GBL_NEW"))
    {
        if (ConfirmLeave(0))
        {
            return;
        }
        HideAll();
        ShowWrite(WRITE_NEW);
    }
    else if (g_iGblMenu == GetItemIndex("GBL_ITEM", "MAILER_GBL_ADDR"))
    {
        if (ConfirmLeave(0))
        {
            return;
        }
        HideAll();
        ShowAddr(0, 0, ADDR_MODE_BOOK);
    }
    else if (g_iGblMenu == GetItemIndex("GBL_ITEM", "MAILER_GBL_REFRESH"))
    {
        if (ConfirmLeave(0))
        {
            return;
        }
        HideAll();
        ShowRefresh();
    }
    else if (g_iGblMenu == GetItemIndex("GBL_ITEM", "MAILER_GBL_SETTINGS"))
    {
        if (ConfirmLeave(0))
        {
            return;
        }
        HideAll();
        ShowSet(1);
    }
    else if (g_iGblMenu == GetItemIndex("GBL_ITEM", "MAILER_GBL_SORT"))
    {
        if (ConfirmLeave(0))
        {
            return;
        }
        HideAll();
        ShowSort(0);
    }
    else if (g_iGblMenu == GetItemIndex("GBL_ITEM", "MAILER_GBL_FINISH"))
    {
        if (ConfirmLeave(1))
        {
            return;
        }
        if (AskQuit(ANSWER_NO, g_objFocus, g_objPage))
        {
            HideAll();
            g_bQuit = 1;
        }
        else
        {
            SendMessage(g_objPage, TAG("FOCS"), TAG("IN"), 0, 0, 0);
            SendMessage(g_objPage, TAG("BLNK"), TAG("IN"), 0, 0, 0);
        }
    }
}

/*
 * Lets the current page be left: asks to save the mail being written, warns about missing basic settings.
 * Returns 1 to stay; with reopen it goes back to the list or the mail being read (the drafts when it saved).
 */
ConfirmLeave(reopen)
{
    if (g_objPage == g_objWrite)
    {
        if (GetPubVar(g_objPage, "g_revert"))
        {
            new error;
            new index;
            new answer = AskSaveDraft(ANSWER_NO, g_objFocus, g_objPage, error, index);
            if (DRAFT_STAY != answer)
            {
                SendMessage(g_objPage, TAG("ACT"), TAG("BSET"), 0, 0, 0);
                wait_message(TAG("ACWT"));
                if (error == ERR_NOSPACE)
                {
                    HideAll();
                    g_bQuit = 1;
                    return 1;
                }
                if (reopen)
                {
                    if (answer == DRAFT_DROPPED)
                    {
                        HideAll();
                        g_iScreenHist = g_iScreenHist ^ ScreenBit(SCREEN_WRITE);
                        ReturnToList(0);
                    }
                    else
                    {
                        HideAll();
                        get_message_line(g_iDraftTop, g_iDraftLine, FOLDER_DRAFT, index, 0, 0);
                        ShowList(FOLDER_DRAFT, 0);
                    }
                }
            }
            else
            {
                SendMessage(g_objPage, TAG("BLNK"), TAG("IN"), 0, 0, 0);
                SendMessage(g_objPage, TAG("FOCS"), TAG("IN"), 0, 0, 0);
                return 1;
            }
        }
    }
    else if (g_objPage == g_objAddr && IsAddrPick())
    {
        if (g_bWriteEdited)
        {
            new error;
            new index;
            new answer = AskSaveDraft(ANSWER_NO, g_objFocus, g_objPage, error, index);
            if (DRAFT_STAY != answer)
            {
                if (error == ERR_NOSPACE)
                {
                    HideAll();
                    g_bQuit = 1;
                    return 1;
                }
                if (reopen)
                {
                    if (answer == DRAFT_DROPPED)
                    {
                        HideAll();
                        g_iScreenHist = g_iScreenHist ^ ScreenBit(SCREEN_WRITE);
                        ReturnToList(0);
                    }
                    else
                    {
                        HideAll();
                        get_message_line(g_iDraftTop, g_iDraftLine, FOLDER_DRAFT, index, 0, 0);
                        ShowList(FOLDER_DRAFT, 0);
                    }
                }
            }
            else
            {
                SendMessage(g_objPage, TAG("BLNK"), TAG("IN"), 0, 0, 0);
                SendMessage(g_objPage, TAG("FOCS"), TAG("IN"), 0, 0, 0);
                return 1;
            }
        }
    }
    else if (g_objPage == g_objSetEdit)
    {
        if (g_iSetMenu == GetItemIndex("SET_ITEM", "MAILER_SET_FOUND"))
        {
            SendMessage(g_objPage, TAG("ACT"), TAG("SCHK"), 0, 0, 0);
            wait_message(TAG("ACWT"));
            if (GetPubVar(g_objPage, "g_setchk"))
            {
                ShowMessage("msgmailer_dlg_set_basic_warn", g_objFocus, g_objPage);
            }
        }
    }
    return 0;
}

/* Runs the item chosen in the option menu. */
RunOptMenu()
{
    new items[12][16];
    GetItemIds("OPT_ITEM", items);
    if (!strcmp("MAILER_OPT_ADDR_ADD", items[g_iOptMenu]))
    {
        AskAddAddress(ANSWER_YES, g_objFocus, g_objPage);
    }
    else if (!strcmp("MAILER_OPT_ADDR_REF", items[g_iOptMenu]))
    {
        if (g_objPage == g_objWrite)
        {
            SaveWriteState();
            SendMessage(g_objPage, TAG("ACT"), TAG("BSET"), 0, 0, 0);
            wait_message(TAG("ACWT"));
        }
        HideAll();
        ShowAddr(0, 0, ADDR_MODE_PICK);
    }
    else if (!strcmp("MAILER_OPT_INFOMATION", items[g_iOptMenu]))
    {
        HideAll();
        ShowInfo();
    }
    else if (!strcmp("MAILER_OPT_LOCK", items[g_iOptMenu]))
    {
        if (!g_OptDele)
        {
            SendMessage(g_objPage, TAG("ACT"), TAG("LOCK"), 0, 0, 0);
            wait_message(TAG("ACWT"));
            ShowMessage("msgmailer_dlg_unlock", g_objFocus, g_objPage);
        }
        else
        {
            SendMessage(g_objPage, TAG("ACT"), TAG("LOCK"), 0, 0, 0);
            wait_message(TAG("ACWT"));
            ShowMessage("msgmailer_dlg_lock", g_objFocus, g_objPage);
        }
    }
    else if (!strcmp("MAILER_OPT_LOUPE", items[g_iOptMenu]))
    {
        SendMessage(g_objPage, TAG("ACT"), TAG("LOUP"), 0, 0, 0);
        wait_message(TAG("ACWT"));
        SetPubVar(g_objPage, "g_active", 1);
    }
    else if (!strcmp("MAILER_OPT_DELE", items[g_iOptMenu]))
    {
        if (!g_OptDele)
        {
            ShowMessage("msgmailer_dlg_delete_lock", g_objFocus, g_objPage);
        }
        else
        {
            new result = ConfirmAction(TAG("DELE"), "msgmailer_dlg_delete", "msgmailer_dlg_delete_ok", ANSWER_NO,
                    g_objFocus, g_objPage);
            if (result == CONFIRM_NOSPACE)
            {
                HideAll();
                g_bQuit = 1;
                return;
            }
            if (ANSWER_YES == result)
            {
                if (g_objPage == g_objRead)
                {
                    HideAll();
                    ShowList(0, 1);
                }
            }
        }
    }
    else if (!strcmp("MAILER_OPT_REWRITE", items[g_iOptMenu]))
    {
        HideAll();
        ShowWrite(WRITE_REWRITE);
    }
    else if (!strcmp("MAILER_OPT_REPLY", items[g_iOptMenu]))
    {
        HideAll();
        ShowWrite(WRITE_REPLY);
    }
    else if (!strcmp("MAILER_OPT_FWD", items[g_iOptMenu]))
    {
        HideAll();
        ShowWrite(WRITE_FORWARD);
    }
    else if (!strcmp("MAILER_OPT_SEND", items[g_iOptMenu]))
    {
        SendMail();
    }
    else if (!strcmp("MAILER_OPT_ADDR_DELE", items[g_iOptMenu]))
    {
        ConfirmAction(TAG("ADEL"), "msgmailer_dlg_addredit_delete", "msgmailer_dlg_addredit_delete_ok", ANSWER_NO,
                g_objFocus, g_objPage);
    }
    else if (!strcmp("MAILER_OPT_ADDR_NEW", items[g_iOptMenu]))
    {
        if (mx_CountAddr() >= MAX_ADDR)
        {
            ShowMessage("msgmailer_dlg_addredit_add_full", g_objFocus, g_objPage);
        }
        else
        {
            HideAll();
            ShowAddrEdit(AEDIT_NEW);
        }
    }
}

/* Circle on the current page: opens what its selected item leads to. */
OnCircle()
{
    if (g_objPage == g_objList && GetPubVar(g_objPage, "g_count"))
    {
        sndSePlay(SE_DECIDE);
        HideAll();
        if (g_iFolder == FOLDER_DRAFT)
        {
            ShowWrite(WRITE_REWRITE);
        }
        else
        {
            ShowRead(1);
        }
    }
    else if (g_objPage == g_objAddr && GetPubVar(g_objPage, "g_count"))
    {
        sndSePlay(SE_DECIDE);
        HideAll();
        if (!IsAddrPick())
        {
            if (mx_CountAddr())
            {
                ShowAddrEdit(AEDIT_CHANGE);
            }
            else
            {
                ShowAddrEdit(AEDIT_NEW);
            }
        }
        else
        {
            g_iScreenHist = g_iScreenHist ^ ScreenBit(SCREEN_ADDR_PICK);
            ShowWrite(WRITE_ADD_ADDR);
        }
    }
    else if (g_objPage == g_objWrite && GetPubVar(g_objPage, "g_iIndex") == GetPubVar(g_objPage, "g_iSend"))
    {
        SendMail();
    }
    else if (g_objPage == g_objSet)
    {
        sndSePlay(SE_DECIDE);
        HideAll();
        ShowSetEdit(g_iSetMenu, SEDIT_FROM_MENU, 0, 0);
    }
    else if (g_objPage == g_objSetEdit && !GetPubVar(g_objPage, "g_chkskb"))
    {
        g_iSetEditItem = GetPubVar(g_objPage, "g_iSetMenu");
        new part_args[64];
        SendMessage(g_objPage, TAG("ACT"), TAG("PART"), 0, 0, 0);
        WaitRecvMessageStr(TAG("ACWT"), part_args, 64);
        sndSePlay(SE_DECIDE);
        HideAll();
        ShowPart(part_args, g_iSetMenu, g_iSetEditItem);
    }
    else if (g_objPage == g_objPart)
    {
        g_iPartSelect = GetPubVar(g_objPage, "g_select");
        sndSePlay(SE_DECIDE);
        HideAll();
        ShowSetEdit(g_iSetMenu, SEDIT_FROM_PART, g_iSetEditItem, g_iPartSelect);
    }
    else if (g_objPage == g_objInterval)
    {
        new part_args[64];
        {
            new label[8];
            new unit_text[8];
            new unit_fmt[8];
            new value_text[8];
            new new_text[8];
            new old_text[8];
            new old_interval;
            new new_interval = GetPubVar(g_objPage, "g_select");
            get_tag_attr("TEXT", "msgmailer_set_item_recv_check_interval", "value", label, 8);
            get_tag_attr("TEXT", "msgmailer_set_item_recv_check_interval_2", "value", unit_text, 8);
            replace_all(unit_fmt, unit_text, "%CHKINTV", "%d", 8);
            sprintf(value_text, unit_fmt, new_interval);
            sprintf(new_text, "%s %s", label, value_text);
            mx_GetPref("INTERVAL", old_interval);
            sprintf(value_text, unit_fmt, old_interval);
            sprintf(old_text, "%s %s", label, value_text);
            replace_all(part_args, g_szPartArgs, old_text, new_text, 64);
            mx_SetPref("INTERVAL", new_interval);
            mx_Restart();
        }
        sndSePlay(SE_DECIDE);
        HideAll();
        ShowPart(part_args, g_iSetMenu, g_iSetEditItem);
    }
    else if (g_objPage == g_objSort)
    {
        sndSePlay(SE_DECIDE);
        HideAll();
        ShowSortEdit();
    }
    else if (g_objPage == g_objSortEdit)
    {
        sndSePlay(SE_DECIDE);
        g_bSortApply = 1;
        HideAll();
        if (g_iSortError == ERR_NOSPACE)
        {
            ShowMessage("msgmailer_dlg_sort_nospace", g_objFocus, Self());
            g_bQuit = 1;
            return;
        }
        g_iScreenHist = g_iScreenHist ^ ScreenBit(SCREEN_SORT_EDIT);
        ShowSort(1);
    }
    else if (g_objPage == g_objRefresh)
    {
        if (RunRefresh())
        {
            return;
        }
    }
}

/* Cross on the current page: goes back. */
OnCross()
{
    if (g_objPage == g_objList)
    {
        if (AskQuit(ANSWER_NO, g_objFocus, g_objPage))
        {
            HideAll();
            g_bQuit = 1;
        }
        else
        {
            SendMessage(g_objPage, TAG("FOCS"), TAG("IN"), 0, 0, 0);
            SendMessage(g_objPage, TAG("BLNK"), TAG("IN"), 0, 0, 0);
        }
    }
    else if (g_objPage == g_objRead)
    {
        sndSePlay(SE_CANCEL);
        HideAll();
        ShowList(0, 1);
    }
    else if (g_objPage == g_objWrite && GetPubVar(g_objPage, "g_active"))
    {
        LeaveWrite();
    }
    else if (g_objPage == g_objInfo)
    {
        sndSePlay(SE_CANCEL);
        HideAll();
        g_iScreenHist = g_iScreenHist ^ ScreenBit(SCREEN_INFO);
        ReturnToList(1);
    }
    else if (g_objPage == g_objAddr)
    {
        sndSePlay(SE_CANCEL);
        HideAll();
        if (g_iAddrMode == ADDR_MODE_PICK)
        {
            g_iScreenHist = g_iScreenHist ^ ScreenBit(SCREEN_ADDR);
            ShowWrite(WRITE_RETURN);
        }
        else
        {
            ShowList(0, 1);
        }
    }
    else if (g_objPage == g_objAddrEdit)
    {
        LeaveAddrEdit();
    }
    else if (g_objPage == g_objSet)
    {
        sndSePlay(SE_CANCEL);
        HideAll();
        g_iScreenHist = g_iScreenHist ^ ScreenBit(SCREEN_SET);
        ReturnToList(1);
    }
    else if (g_objPage == g_objSetEdit)
    {
        sndSePlay(SE_CANCEL);
        if (g_iSetMenu == GetItemIndex("SET_ITEM", "MAILER_SET_FOUND"))
        {
            SendMessage(g_objPage, TAG("ACT"), TAG("SCHK"), 0, 0, 0);
            wait_message(TAG("ACWT"));
            if (GetPubVar(g_objPage, "g_setchk"))
            {
                ShowMessage("msgmailer_dlg_set_basic_warn", g_objFocus, g_objPage);
            }
        }
        HideAll();
        g_iScreenHist = g_iScreenHist ^ ScreenBit(SCREEN_SET_EDIT);
        if (g_iSetEditFrom == SEDIT_FROM_BOOT)
        {
            ShowList(0, 1);
        }
        else
        {
            ShowSet(0);
        }
    }
    else if (g_objPage == g_objPart)
    {
        sndSePlay(SE_CANCEL);
        HideAll();
        g_iScreenHist = g_iScreenHist ^ ScreenBit(SCREEN_SET_PART);
        ShowSetEdit(g_iSetMenu, SEDIT_FROM_MENU, g_iSetEditItem, 0);
    }
    else if (g_objPage == g_objInterval)
    {
        sndSePlay(SE_CANCEL);
        HideAll();
        ShowPart(g_szPartArgs, g_iSetMenu, g_iSetEditItem);
    }
    else if (g_objPage == g_objSort)
    {
        sndSePlay(SE_CANCEL);
        HideAll();
        g_iScreenHist = g_iScreenHist ^ ScreenBit(SCREEN_SORT);
        ReturnToList(1);
    }
    else if (g_objPage == g_objSortEdit)
    {
        sndSePlay(SE_CANCEL);
        g_bSortApply = 0;
        HideAll();
        g_iScreenHist = g_iScreenHist ^ ScreenBit(SCREEN_SORT_EDIT);
        ShowSort(1);
    }
    else if (g_objPage == g_objRefresh)
    {
        sndSePlay(SE_CANCEL);
        HideAll();
        g_iScreenHist = g_iScreenHist ^ ScreenBit(SCREEN_REFRESH);
        ReturnToList(1);
    }
}

/* Triangle on the current page: opens its option menu. */
OnTriangle()
{
    if (g_objPage == g_objAddr && !IsAddrPick())
    {
        sndSePlay(SE_DIALOG_OPEN);
        SetPubVar(g_objPage, "g_active", 0);
        ShowOpt(OptBit("MAILER_OPT_ADDR_DELE") | OptBit("MAILER_OPT_ADDR_NEW"), SCREEN_ADDR);
    }
    else if (g_objPage == g_objWrite)
    {
        SaveWriteState();
        if (g_iWriteItem == GetItemIndex("WRITE_ITEM", "MAILER_WRITE_TO"))
        {
            sndSePlay(SE_DIALOG_OPEN);
            SetPubVar(g_objPage, "g_active", 0);
            ShowOpt(OptBit("MAILER_OPT_ADDR_REF"), SCREEN_WRITE);
        }
    }
    else if (g_objPage == g_objList)
    {
        SetPubVar(g_objPage, "g_active", 0);
        FetchLines();
        if (GetPubVar(g_objPage, "g_count"))
        {
            sndSePlay(SE_DIALOG_OPEN);
            if (FOLDER_INBOX == g_iFolder)
            {
                ShowOpt(OptBit("MAILER_OPT_ADDR_ADD") | OptBit("MAILER_OPT_LOCK") | OptBit("MAILER_OPT_REPLY")
                        | OptBit("MAILER_OPT_FWD") | OptBit("MAILER_OPT_INFOMATION") | OptBit("MAILER_OPT_DELE"),
                        SCREEN_LIST);
            }
            else if (g_iFolder == FOLDER_OUTBOX)
            {
                ShowOpt(OptBit("MAILER_OPT_REWRITE") | OptBit("MAILER_OPT_LOCK") | OptBit("MAILER_OPT_FWD")
                        | OptBit("MAILER_OPT_INFOMATION") | OptBit("MAILER_OPT_DELE"),
                        SCREEN_LIST);
            }
            else if (g_iFolder == FOLDER_DRAFT)
            {
                ShowOpt(OptBit("MAILER_OPT_LOCK") | OptBit("MAILER_OPT_INFOMATION") | OptBit("MAILER_OPT_DELE"),
                        SCREEN_LIST);
            }
            else if (g_iFolder == FOLDER_QUEUE)
            {
                ShowOpt(OptBit("MAILER_OPT_SEND") | OptBit("MAILER_OPT_REWRITE") | OptBit("MAILER_OPT_LOCK")
                        | OptBit("MAILER_OPT_INFOMATION") | OptBit("MAILER_OPT_DELE"),
                        SCREEN_LIST);
            }
        }
        else
        {
            SetPubVar(g_objPage, "g_active", 1);
        }
    }
    else if (g_objPage == g_objRead)
    {
        SetPubVar(g_objPage, "g_active", 0);
        FetchLines();
        if (GetPubVar(g_objPage, "g_count"))
        {
            sndSePlay(SE_DIALOG_OPEN);
            if (FOLDER_INBOX == g_iFolder)
            {
                ShowOpt(OptBit("MAILER_OPT_ADDR_ADD") | OptBit("MAILER_OPT_LOCK") | OptBit("MAILER_OPT_LOUPE")
                        | OptBit("MAILER_OPT_REPLY") | OptBit("MAILER_OPT_FWD") | OptBit("MAILER_OPT_INFOMATION")
                        | OptBit("MAILER_OPT_DELE"),
                        SCREEN_LIST);
            }
            else if (g_iFolder == FOLDER_OUTBOX)
            {
                ShowOpt(OptBit("MAILER_OPT_REWRITE") | OptBit("MAILER_OPT_LOCK") | OptBit("MAILER_OPT_LOUPE")
                        | OptBit("MAILER_OPT_FWD") | OptBit("MAILER_OPT_INFOMATION") | OptBit("MAILER_OPT_DELE"),
                        SCREEN_LIST);
            }
            else if (g_iFolder == FOLDER_DRAFT)
            {
                ShowOpt(OptBit("MAILER_OPT_LOCK") | OptBit("MAILER_OPT_LOUPE") | OptBit("MAILER_OPT_INFOMATION")
                        | OptBit("MAILER_OPT_DELE"),
                        SCREEN_LIST);
            }
            else if (g_iFolder == FOLDER_QUEUE)
            {
                ShowOpt(OptBit("MAILER_OPT_SEND") | OptBit("MAILER_OPT_REWRITE") | OptBit("MAILER_OPT_LOCK")
                        | OptBit("MAILER_OPT_LOUPE") | OptBit("MAILER_OPT_INFOMATION") | OptBit("MAILER_OPT_DELE"),
                        SCREEN_LIST);
            }
        }
        else
        {
            SetPubVar(g_objPage, "g_active", 1);
        }
    }
    else if (g_objPage == g_objPart)
    {
        if (g_iSetMenu == GetItemIndex("SET_ITEM", "MAILER_SET_RECEPT"))
        {
            if (g_iSetEditItem == GetItemIndex("SEDIT_RECEPT_ITEM", "MAILER_SEDIT_RECEPT_CHECK"))
            {
                g_iPartSelect = GetPubVar(g_objPage, "g_select");
                if (g_iPartSelect == GetItemIndex("SEDIT_RECEPT_CHECK_ITEM", "MAILER_SEDIT_RECEPT_INTERVAL"))
                {
                    sndSePlay(SE_DECIDE);
                    HideAll();
                    ShowInterval();
                    return;
                }
            }
        }
    }
}

/* Cross on the address editor: checks the address ("ACHK"), saves it and goes back to the address book. */
LeaveAddrEdit()
{
    new result;
    SendMessage(g_objPage, TAG("ACT"), TAG("ACHK"), 0, 0, 0);
    WaitRecvMessage(TAG("ACWT"), result);
    if (result == AEDIT_CHECK_EMPTY)
    {
        HideAll();
        g_iScreenHist = g_iScreenHist ^ ScreenBit(SCREEN_ADDR_EDIT);
        ShowAddr(g_iAddrTop, g_iAddrLine, ADDR_MODE_BOOK);
        return;
    }
    if (result == AEDIT_CHECK_NOADDR)
    {
        ShowMessage("msgmailer_dlg_addredit_add_noadd", g_objFocus, g_objPage);
        return;
    }
    if (result == AEDIT_CHECK_EXISTS)
    {
        new answer = AskYesNo("msgmailer_dlg_addredit_add_exist", ANSWER_NO, g_objFocus, g_objPage);
        if (ANSWER_YES == answer)
        {
            g_iAddrEditMode = AEDIT_OVERWRITE;
        }
        else
        {
            if (answer == ANSWER_NO)
            {
                HideAll();
                g_iScreenHist = g_iScreenHist ^ ScreenBit(SCREEN_ADDR_EDIT);
                ShowAddr(g_iAddrTop, g_iAddrLine, ADDR_MODE_BOOK);
                return;
            }
            SendMessage(g_objPage, TAG("BLNK"), TAG("IN"), 0, 0, 0);
            SendMessage(g_objPage, TAG("FOCS"), TAG("IN"), 0, 0, 0);
            return;
        }
    }
    new line;
    new top;
    if (AEDIT_CHANGE == g_iAddrEditMode)
    {
        SendMessage(g_objPage, TAG("ACT"), TAG("AEDT"), 0, 0, 0);
        wait_message(TAG("ACWT"));
        line = g_iAddrLine;
        top = g_iAddrTop;
    }
    else if (g_iAddrEditMode == AEDIT_NEW)
    {
        SendMessage(g_objPage, TAG("ACT"), TAG("ANEW"), 0, 0, 0);
        wait_message(TAG("ACWT"));
        line = top = 0;
    }
    else if (g_iAddrEditMode == AEDIT_OVERWRITE)
    {
        new index;
        SendMessage(g_objPage, TAG("ACT"), TAG("AORW"), 0, 0, 0);
        WaitRecvMessage(TAG("ACWT"), index);
        get_address_line(top, line, index);
    }
    sndSePlay(SE_CANCEL);
    HideAll();
    g_iScreenHist = g_iScreenHist ^ ScreenBit(SCREEN_ADDR_EDIT);
    ShowAddr(top, line, ADDR_MODE_BOOK);
}

/* Cross on the write page: asks to save the mail when it was edited, then goes back. */
LeaveWrite()
{
    if (GetPubVar(g_objPage, "g_revert"))
    {
        new error;
        new index;
        new answer = AskSaveDraft(ANSWER_NO, g_objFocus, g_objPage, error, index);
        if (DRAFT_STAY != answer)
        {
            if (error == ERR_NOSPACE)
            {
                HideAll();
                g_bQuit = 1;
            }
            else if (answer == DRAFT_DROPPED)
            {
                HideAll();
                g_iScreenHist = g_iScreenHist ^ ScreenBit(SCREEN_WRITE);
                ReturnToList(1);
            }
            else
            {
                HideAll();
                get_message_line(g_iDraftTop, g_iDraftLine, FOLDER_DRAFT, index, 0, 0);
                ShowList(FOLDER_DRAFT, 1);
            }
        }
        else
        {
            SendMessage(g_objPage, TAG("BLNK"), TAG("IN"), 0, 0, 0);
            SendMessage(g_objPage, TAG("FOCS"), TAG("IN"), 0, 0, 0);
        }
    }
    else
    {
        sndSePlay(SE_CANCEL);
        HideAll();
        g_iScreenHist = g_iScreenHist ^ ScreenBit(SCREEN_WRITE);
        ReturnToList(1);
    }
}

/* Sends the mail of the page after asking, then opens the outbox, or the queue when it was not sent. */
SendMail()
{
    new ready;
    SendMessage(g_objPage, TAG("ACT"), TAG("SDCK"), 0, 0, 0);
    WaitRecvMessage(TAG("ACWT"), ready);
    if (ready)
    {
        new error;
        new index;
        new result;
        result = ConfirmTransfer(TAG("SEND"), "msgmailer_dlg_send", "msgmailer_dlg_send_prog", "msgmailer_dlg_send_ok",
                "msgmailer_dlg_send_cancel", "msgmailer_dlg_send_cancel_ok", ANSWER_NO, g_objFocus, g_objPage, error,
                index);
        if (result == TRANSFER_ENDED)
        {
            if (0 == error)
            {
                HideAll();
                get_message_line(g_iOutboxTop, g_iOutboxLine, FOLDER_OUTBOX, index, 0, 0);
                ShowList(FOLDER_OUTBOX, 1);
            }
            else if (error == ERR_NOSPACE)
            {
                HideAll();
                g_bQuit = 1;
            }
            else
            {
                HideAll();
                get_message_line(g_iQueueTop, g_iQueueLine, FOLDER_QUEUE, index, 0, 0);
                ShowList(FOLDER_QUEUE, 1);
            }
        }
        else if (result == TRANSFER_CANCELLED)
        {
            HideAll();
            get_message_line(g_iQueueTop, g_iQueueLine, FOLDER_QUEUE, index, 0, 0);
            ShowList(FOLDER_QUEUE, 1);
        }
    }
    else
    {
        ShowMessage("msgmailer_dlg_send_noto", g_objFocus, g_objPage);
    }
}

/*
 * Runs the item chosen on the refresh page and keeps the lines of the folders on the same mails. Returns 1
 * when the mailer quits for lack of space.
 */
RunRefresh()
{
    SendMessage(g_objPage, TAG("DLG"), TAG("IN"), 0, 0, 0);
    new items[2][16];
    GetItemIds("REFRESH_ITEM", items);
    new item = GetPubVar(g_objPage, "g_iLine");
    if (!strcmp(items[item], "MAILER_REFRESH_RECV"))
    {
        g_iInboxMsgNo = mx_Line2MsgNum(FOLDER_INBOX, g_iInboxLine + g_iInboxTop);
        {
            new error;
            new count;
            RunTransfer(TAG("RECV"), "msgmailer_dlg_recv_prog", "msgmailer_dlg_recv_done", "msgmailer_dlg_recv_cancel",
                    "msgmailer_dlg_recv_cancel_ok", ANSWER_NO, g_objFocus, g_objPage, error, count);
            if (count)
            {
                g_bTransferred = 1;
            }
            if (error == ERR_NOSPACE)
            {
                HideAll();
                g_bQuit = 1;
                return 1;
            }
        }
        if (g_bTransferred)
        {
            new index;
            {
                new old_line = g_iInboxLine;
                index = mx_MsgNum2Line(FOLDER_INBOX, g_iInboxMsgNo);
                get_message_line(g_iInboxTop, g_iInboxLine, FOLDER_INBOX, index, 1, old_line);
            }
            SelectFolderLine();
        }
    }
    else if (!strcmp(items[item], "MAILER_REFRESH_SEND"))
    {
        g_iOutboxMsgNo = mx_Line2MsgNum(FOLDER_OUTBOX, g_iOutboxLine + g_iOutboxTop);
        g_iQueueMsgNo = mx_Line2MsgNum(FOLDER_QUEUE, g_iQueueLine + g_iQueueTop);
        {
            new error;
            new count;
            RunTransfer(TAG("SEND"), "msgmailer_dlg_send_prog", "msgmailer_dlg_send_done", "msgmailer_dlg_send_cancel",
                    "msgmailer_dlg_send_cancel_ok", ANSWER_NO, g_objFocus, g_objPage, error, count);
            if (count)
            {
                g_bTransferred = 1;
            }
            if (error == ERR_NOSPACE)
            {
                HideAll();
                g_bQuit = 1;
                return 1;
            }
        }
        if (g_bTransferred)
        {
            new index;
            {
                new old_line = g_iOutboxLine;
                index = mx_MsgNum2Line(FOLDER_OUTBOX, g_iOutboxMsgNo);
                get_message_line(g_iOutboxTop, g_iOutboxLine, FOLDER_OUTBOX, index, 1, old_line);
            }
            {
                new old_line = g_iQueueLine;
                index = mx_MsgNum2Line(FOLDER_QUEUE, g_iQueueMsgNo);
                get_message_line(g_iQueueTop, g_iQueueLine, FOLDER_QUEUE, index, 1, old_line);
            }
            SelectFolderLine();
        }
    }
    SendMessage(g_objPage, TAG("DLG"), TAG("OUT"), 0, 0, 0);
    return 0;
}
