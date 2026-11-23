/*
 * /opt0/bn/script/mailer/mailwriteskb.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The software keyboard of the mail editor: it edits the line "line" of the message (to or subject on one
 * line, the body in a multi-line editor), with the keyboard settings in "mode", "input", "opt", "type"
 * and "limit".
 */

#include <core>
#include <bn>

#pragma dynamic 42000

#include "tag.inc"

#include <float>

/* Sleeps until a message msg arrives. */
wait_message(msg, &a=0, &b=0, &c=0, &d=0)
{
    while (!FindMessage(msg, a, b, c, d))
    {
        sleep;
    }
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

#include "text.inc"

new Float:g_flBackdropRed = 0.25;
new Float:g_flBackdropGreen = 0.25;
new Float:g_flBackdropBlue = 0.25;
new Float:g_flBackdropAlpha = 1.0;
new Float:g_flTextRed = 0.7843;
new Float:g_flTextGreen = 0.7843;
new Float:g_flTextBlue = 0.7843;
new Float:g_flFadeTime = 300.0;
new g_iItemCount;
new g_szItemId[64];

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
new g_szAnyId[1]; /* an empty id: the first element with the tag */
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

public g_manager; /* gets "FNIS" with the edited text, or "FNIB" when the keyboard is cancelled */
public g_manager2; /* the mailer page, asked with "DLG" "WEXT" whether to go on writing the body */

main()
{
    new arg_value[32];
    new line;
    new kind;
    new input_mode;
    new flags;
    new length_unit;
    new max_length;
    if (GetArg("mode", arg_value, 32))
    {
        kind = atoi(arg_value);
    }
    if (GetArg("input", arg_value, 32))
    {
        input_mode = atoi(arg_value);
    }
    if (GetArg("opt", arg_value, 32))
    {
        flags = atoi(arg_value);
    }
    if (GetArg("type", arg_value, 32))
    {
        length_unit = atoi(arg_value);
    }
    if (GetArg("limit", arg_value, 32))
    {
        max_length = atoi(arg_value);
    }
    if (GetArg("line", arg_value, 32))
    {
        line = atoi(arg_value);
    }
    if (line == GetItemIndex("WRITE_ITEM", "MAILER_WRITE_BODY"))
    {
        EditBody(kind, input_mode, flags, length_unit, max_length);
    }
    else
    {
        EditLine(kind, input_mode, flags, length_unit, max_length, line);
    }
}

/* Edits the to or subject line and sends the result to g_manager. */
EditLine(kind, input_mode, flags, length_unit, max_length, line)
{
    new ok = EditLineOnSkb(kind, input_mode, flags, length_unit, max_length, line);
    new text[246];
    if (ok)
    {
        skbGetString(text, 246);
        SendMessageStr(g_manager, TAG("FNIS"), text);
    }
    else
    {
        SendMessageStr(g_manager, TAG("FNIB"), text);
    }
}

/* Waits for "SKB" with the text to edit, then edits it; returns 1 if the keyboard was closed with OK. */
EditLineOnSkb(kind, input_mode, flags, length_unit, max_length, line)
{
    new ok;
    for (; ; )
    {
        new msg;
        new text[246];
        while (RecvMessageStr(msg, text, 246))
        {
            if (msg == TAG("SKB"))
            {
                ok = RunLineKeyboard(text, kind, input_mode, flags, length_unit, max_length, line);
                return ok;
            }
        }
        sleep;
    }
    return ok;
}

/* Edits text with the software keyboard in the one-line editor of the mail; returns 1 on OK. */
RunLineKeyboard(text[], kind, input_mode, flags, length_unit, max_length, line)
{
    new backdrop = CreateDefocus(Self());
    SetOptionI(backdrop, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(backdrop, TAG("LEVL"), 3);
    AnimColor(backdrop, g_flBackdropRed, g_flBackdropGreen, g_flBackdropBlue, g_flBackdropAlpha, g_flFadeTime, 0.0,
            ANIM_FUNC_LINEAR);
    new frames[2];
    for (new i; i < 2; i++)
    {
        new x[10];
        new y[10];
        new h[10];
        new w[10];
        new r[10];
        new g[10];
        new b[10];
        new a[10];
        get_group_tag_attr2("WRITE", "WRITE_EDIT", i, "x", x, 10);
        get_group_tag_attr2("WRITE", "WRITE_EDIT", i, "y", y, 10);
        get_group_tag_attr2("WRITE", "WRITE_EDIT", i, "w", w, 10);
        get_group_tag_attr2("WRITE", "WRITE_EDIT", i, "h", h, 10);
        get_group_tag_attr2("WRITE", "WRITE_EDIT", i, "r", r, 10);
        get_group_tag_attr2("WRITE", "WRITE_EDIT", i, "g", g, 10);
        get_group_tag_attr2("WRITE", "WRITE_EDIT", i, "b", b, 10);
        get_group_tag_attr2("WRITE", "WRITE_EDIT", i, "a", a, 10);
        frames[i] = CreatePlane(backdrop);
        SetPos(frames[i], atof(x) + atof(w) / 2, atof(y) + atof(h) / 2, 0.0, 0);
        SetScale(frames[i], atof(w), atof(h), 0.0);
        SetColor(frames[i], atof(r), atof(g), atof(b), 0.0);
        AnimColor(frames[i], atof(r), atof(g), atof(b), atof(a), g_flFadeTime, 0.0, ANIM_FUNC_LINEAR);
    }
    new caption;
    new caption_text[64];
    {
        new x[10];
        new y[10];
        new h[10];
        new w[10];
        get_group_tag_attr2("WRITE", "WRITE_EDIT", 0, "x", x, 10);
        get_group_tag_attr2("WRITE", "WRITE_EDIT", 0, "y", y, 10);
        get_group_tag_attr2("WRITE", "WRITE_EDIT", 0, "w", w, 10);
        get_group_tag_attr2("WRITE", "WRITE_EDIT", 0, "h", h, 10);
        caption = CreateLabel(backdrop);
        SetPos(caption, atof(x) + atof(w) / 2, 2.0 + (atof(y) + atof(h) / 2), 0.0, 0);
        SetOptionI(caption, TAG("HALN"), ALIGN_CENTER);
        SetOptionI(caption, TAG("VALN"), ALIGN_CENTER);
        SetOptionI(caption, TAG("FSIZ"), 15);
        SetColor(caption, g_flTextRed, g_flTextGreen, g_flTextBlue, 0.0);
        AnimColor(caption, g_flTextRed, g_flTextGreen, g_flTextBlue, 1.0, g_flFadeTime, 0.0, ANIM_FUNC_LINEAR);
    }
    if (line == GetItemIndex("WRITE_ITEM", "MAILER_WRITE_TO"))
    {
        get_tag_attr("TEXT", "msgmailer_write_item_to", "value", caption_text, 64);
    }
    else if (line == GetItemIndex("WRITE_ITEM", "MAILER_WRITE_SUBJECT"))
    {
        get_tag_attr("TEXT", "msgmailer_write_item_subject", "value", caption_text, 64);
    }
    SetOptionS(caption, TAG("UTF8"), caption_text);
    new edit;
    {
        new x[10];
        new y[10];
        new h[10];
        new w[10];
        get_group_tag_attr2("WRITE", "WRITE_EDIT", 1, "x", x, 10);
        get_group_tag_attr2("WRITE", "WRITE_EDIT", 1, "y", y, 10);
        get_group_tag_attr2("WRITE", "WRITE_EDIT", 1, "w", w, 10);
        get_group_tag_attr2("WRITE", "WRITE_EDIT", 1, "h", h, 10);
        edit = CreateEdit(backdrop, 1024);
        SetPos(edit, 12.0 + atof(x), 8.0 + atof(y), 0.0, 0);
        SetColor(edit, g_flTextRed, g_flTextGreen, g_flTextBlue, 1.0);
        SetOptionF(edit, TAG("ZOFS"), 2.0);
        SetOptionI(edit, TAG("HALN"), ALIGN_LEFT);
        SetOptionI(edit, TAG("VALN"), ALIGN_CENTER);
        SetOptionI(edit, TAG("FSIZ"), 15);
        SetOptionS(edit, TAG("UTF8"), text);
        SetOptionI(edit, TAG("W64 "), (atoi(w) - 16) * 64);
        SetOptionI(edit, TAG("H64 "), 20 * 64);
        SetOptionI(edit, TAG("LM64"), 60 * 64);
        SetOptionI(edit, TAG("RM64"), 60 * 64);
        {
            new bg_colors[4];
            static bg_bytes[16];
            pack_color_bytes(bg_colors, bg_bytes);
            SetOptionP(edit, TAG("BGC4"), bg_colors, 4);
        }
    }
    sndSePlay(SE_KEYBOARD_OPEN);
    while (ANIM_DONE != AnimGetState(backdrop, ANIM_COLOR))
    {
        sleep;
    }
    new skb = skbStart(edit, kind, input_mode, flags, length_unit, max_length, _, 0);
    if (skb == -1)
    {
        return 0;
    }
    new ok;
    skbGetString(text, 131072);
    if (wait_skb() == 1)
    {
        ok = 1;
    }
    for (new i; i < 2; i++)
    {
        new Float:color[4];
        GetColor(frames[i], color[0], color[1], color[2], color[3]);
        AnimColor(frames[i], color[0], color[1], color[2], 0.0, g_flFadeTime, 0.0, ANIM_FUNC_LINEAR);
    }
    AnimColor(backdrop, g_flBackdropRed, g_flBackdropGreen, g_flBackdropBlue, 0.0, g_flFadeTime, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(caption, g_flTextRed, g_flTextGreen, g_flTextBlue, 0.0, g_flFadeTime, 0.0, ANIM_FUNC_LINEAR);
    Destroy(edit);
    while (ANIM_DONE != AnimGetState(backdrop, ANIM_COLOR))
    {
        sleep;
    }
    Destroy(backdrop);
    return ok;
}

/* Waits for "INIT" with the body, edits it and sends the result to g_manager. */
EditBody(kind, input_mode, flags, length_unit, max_length)
{
    new text[32768];
    for (; ; )
    {
        new msg;
        while (RecvMessageStr(msg, text, 32768))
        {
            if (msg == TAG("INIT"))
            {
                break;
            }
        }
        if (msg == TAG("INIT"))
        {
            break;
        }
        sleep;
    }
    new ok = RunBodyKeyboard(text, kind, input_mode, flags, length_unit, max_length);
    if (ok)
    {
        skbGetString(text, 32768);
        SendMessageStr(g_manager, TAG("FNIS"), text);
    }
    else
    {
        SendMessageStr(g_manager, TAG("FNIB"), text);
    }
}

public g_bdwrite; /* the ANSWER_* of the page to "DLG" "WEXT": other than ANSWER_YES goes back to the keyboard */

/*
 * Edits the body with the software keyboard in the multi-line editor; when it is cancelled the page asks whether to
 * go on writing. Returns 1 on OK.
 */
RunBodyKeyboard(text[], kind, input_mode, flags, length_unit, max_length)
{
    new backdrop = CreateDefocus(Self());
    SetOptionI(backdrop, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(backdrop, TAG("LEVL"), 3);
    AnimColor(backdrop, g_flBackdropRed, g_flBackdropGreen, g_flBackdropBlue, g_flBackdropAlpha, g_flFadeTime, 0.0,
            ANIM_FUNC_LINEAR);
    new edit;
    {
        new x[10];
        new y[10];
        new h[10];
        new w[10];
        get_group_tag_attr2("WRITE", "WRITE_EDIT2", 0, "x", x, 10);
        get_group_tag_attr2("WRITE", "WRITE_EDIT2", 0, "y", y, 10);
        get_group_tag_attr2("WRITE", "WRITE_EDIT2", 0, "w", w, 10);
        get_group_tag_attr2("WRITE", "WRITE_EDIT2", 0, "h", h, 10);
        edit = CreateEditML_P(backdrop);
        SetPos(edit, atof(x), atof(y), 0.0, 0);
        SetColor(edit, g_flTextRed, g_flTextGreen, g_flTextBlue, 1.0);
        SetOptionF(edit, TAG("ZOFS"), 2.0);
        SetOptionI(edit, TAG("FSIZ"), 15);
        SetOptionS(edit, TAG("UTF8"), text);
        SetOptionI(edit, TAG("W64 "), 548 * 64);
        SetOptionI(edit, TAG("H64 "), 114 * 64);
        SetOptionI(edit, TAG("h64 "), 294 * 64);
        SetOptionI(edit, TAG("LSPC"), 3);
        {
            new colors[4];
            static bg_bytes[16] = {5, 14, 133, 127, 5, 14, 133, 127, 5, 14, 133, 127, 5, 14, 133, 127};
            pack_color_bytes(colors, bg_bytes);
            SetOptionP(edit, TAG("BGC4"), colors, 4);
            static fg_bytes[16] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
            pack_color_bytes(colors, fg_bytes);
            SetOptionP(edit, TAG("COLR"), colors, 4);
        }
    }
    new guide;
    {
        new x[10];
        new y[10];
        new guide_text[16];
        get_group_tag_attr2("WRITE", "HEAD", 0, "info_x", x, 10);
        get_group_tag_attr2("WRITE", "HEAD", 0, "info_y", y, 10);
        get_tag_attr("TEXT", "msgmailer_write_item_body_guide", "value", guide_text, 16);
        guide = CreateLabel(Self());
        SetPos(guide, atof(x), atof(y), 0.0, 0);
        SetOptionI(guide, TAG("HALN"), ALIGN_RIGHT);
        SetOptionI(guide, TAG("VALN"), ALIGN_CENTER);
        SetOptionI(guide, TAG("FSIZ"), 13);
        SetOptionI(guide, TAG("ZSRT"), ZSORT_OVERLAY);
        SetOptionS(guide, TAG("UTF8"), guide_text);
        SetColor(guide, g_flTextRed, g_flTextGreen, g_flTextBlue, 0.0);
        AnimColor(guide, g_flTextRed, g_flTextGreen, g_flTextBlue, 1.0, g_flFadeTime, 0.0, ANIM_FUNC_LINEAR);
    }
    sndSePlay(SE_KEYBOARD_OPEN);
    while (ANIM_DONE != AnimGetState(backdrop, ANIM_COLOR))
    {
        sleep;
    }
    edit_again:
    Show(edit);
    new skb = skbStart(edit, kind, input_mode, flags, length_unit, max_length, _, 0);
    if (skb == -1)
    {
        return 0;
    }
    new ok;
    skbGetString(text, 131072);
    if (wait_skb() == 1)
    {
        ok = 1;
    }
    else
    {
        Hide(edit);
        SendMessage(g_manager2, TAG("DLG"), TAG("WEXT"), Self(), 0, 0);
        wait_message(TAG("ACWT"));
        if (0 != g_bdwrite)
        {
            goto edit_again;
        }
    }
    Destroy(edit);
    sleep;
    AnimColor(backdrop, g_flBackdropRed, g_flBackdropGreen, g_flBackdropBlue, 0.0, g_flFadeTime, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(guide, g_flTextRed, g_flTextGreen, g_flTextBlue, 0.0, g_flFadeTime, 0.0, ANIM_FUNC_LINEAR);
    while (ANIM_DONE != AnimGetState(backdrop, ANIM_COLOR))
    {
        sleep;
    }
    Destroy(backdrop);
    return ok;
}
