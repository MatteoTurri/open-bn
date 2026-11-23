/*
 * /opt0/bn/script/mailer/mailsetskb.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The software keyboard of the mailer settings: it edits one setting under a title (argument "title"),
 * with the keyboard settings in "mode", "input", "opt", "type" and "limit"; "pass" hides the text and
 * "ucs4" forbids angle brackets.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "tag.inc"

#include <float>

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

#include "xmlsync.inc"

#include "text.inc"

new Float:g_flBackdropRed = 0.25;
new Float:g_flBackdropGreen = 0.25;
new Float:g_flBackdropBlue = 0.25;
new Float:g_flBackdropAlpha = 1.0;
new Float:g_flTextRed = 0.7843;
new Float:g_flTextGreen = 0.7843;
new Float:g_flTextBlue = 0.7843;
new Float:g_flFadeTime = 300.0;
new g_szAddrForbidden[3] = {'<', '>'}; /* the characters the keyboard refuses in an address */
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
new g_szFindId[64];

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
new g_szText[64];

main()
{
    new arg_value[32];
    new title[16];
    new kind;
    new input_mode;
    new flags;
    new length_unit;
    new max_length;
    new password;
    new ucs4;
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
    if (GetArg("title", arg_value, 32))
    {
        copy_string(title, arg_value, 16);
    }
    if (GetArg("pass", arg_value, 32))
    {
        password = atoi(arg_value);
    }
    if (GetArg("ucs4", arg_value, 32))
    {
        ucs4 = atoi(arg_value);
    }
    new ok = EditOnInit(kind, input_mode, flags, length_unit, max_length, title, password, ucs4);
    if (ok)
    {
        skbGetString(g_szText, 64);
        SendMessageStr(g_manager, TAG("FNIS"), g_szText);
    }
    else
    {
        SendMessageStr(g_manager, TAG("FNIB"), g_szText);
    }
    return;
}

/* Waits for "INIT" with the text to edit, then edits it; returns 1 if the keyboard was closed with OK. */
EditOnInit(kind, input_mode, flags, length_unit, max_length, title[], password, ucs4)
{
    new ok;
    for (; ; )
    {
        new msg;
        while (RecvMessageStr(msg, g_szText, 64))
        {
            if (msg == TAG("INIT"))
            {
                ok = RunKeyboard(g_szText, kind, input_mode, flags, length_unit, max_length, title, password, ucs4);
                return ok;
            }
        }
        sleep;
    }
    return ok;
}

/* Edits text with the software keyboard in a frame under title; returns 1 and the new text on OK. */
RunKeyboard(text[], kind, input_mode, flags, length_unit, max_length, title[], password, ucs4)
{
    new x[10];
    new y[10];
    new frame_h[10];
    new frame_w[10];
    new backdrop;
    backdrop = CreateDefocus(Self());
    SetOptionI(backdrop, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(backdrop, TAG("LEVL"), 3);
    AnimColor(backdrop, g_flBackdropRed, g_flBackdropGreen, g_flBackdropBlue, g_flBackdropAlpha, g_flFadeTime, 0.0,
            ANIM_FUNC_LINEAR);
    new caption;
    get_group_tag_attr2("SEDIT", "EDIT", 0, "title_x", x, 10);
    get_group_tag_attr2("SEDIT", "EDIT", 0, "title_y", y, 10);
    caption = CreateLabel(backdrop);
    SetPos(caption, atof(x), atof(y), 0.0, 0);
    SetOptionS(caption, TAG("UTF8"), title);
    SetOptionI(caption, TAG("HALN"), ALIGN_LEFT);
    SetOptionI(caption, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(caption, TAG("FSIZ"), 15);
    SetOptionI(caption, TAG("ZSRT"), ZSORT_OVERLAY);
    SetColor(caption, g_flTextRed, g_flTextGreen, g_flTextBlue, 0.0);
    AnimColor(caption, g_flTextRed, g_flTextGreen, g_flTextBlue, 1.0, g_flFadeTime, 0.0, ANIM_FUNC_LINEAR);
    new frame;
    get_group_tag_attr2("SEDIT", "EDIT", 0, "frame_x", x, 10);
    get_group_tag_attr2("SEDIT", "EDIT", 0, "frame_y", y, 10);
    get_group_tag_attr2("SEDIT", "EDIT", 0, "frame_w", frame_w, 10);
    get_group_tag_attr2("SEDIT", "EDIT", 0, "frame_h", frame_h, 10);
    frame = CreatePlane3x3(backdrop);
    SetPos(frame, atof(x), atof(y), 0.0, 0);
    SetScale(frame, atof(frame_w), atof(frame_h), 1.0);
    SetTexture(frame, imgGetResource(RES_EDITBOX), 0, -1);
    SetOptionI(frame, TAG("ALPH"), BLEND_ADD);
    SetOptionF(frame, TAG("ZOFS"), 1.0);
    {
        new Float:tex_margins[4] = {0.5, 0.5, 0.5, 0.5};
        new Float:frame_scale[3];
        new Float:margins[4];
        SetOptionP(frame, TAG("MGNT"), tex_margins, 4);
        GetScale(frame, frame_scale[0], frame_scale[1], frame_scale[2]);
        if (frame_scale[0] > frame_scale[1])
        {
            margins[0] = margins[1] = 0.5 * frame_scale[1] / frame_scale[0];
            margins[2] = margins[3] = 0.5;
        }
        else
        {
            margins[2] = margins[3] = 0.5 * frame_scale[0] / frame_scale[1];
            margins[0] = margins[1] = 0.5;
        }
        SetOptionP(frame, TAG("MGN"), margins, 4);
        SetColor(frame, 1.0, 1.0, 1.0, 0.0);
        AnimColor(frame, 1.0, 1.0, 1.0, 1.0, g_flFadeTime, 0.0, ANIM_FUNC_LINEAR);
    }
    new edit;
    edit = CreateEdit(backdrop, 1024);
    if (password)
    {
        SetOptionI(edit, TAG("PWMD"), '*');
    }
    SetPos(edit, 16.0 + (atof(x) - atof(frame_w) / 2), atof(y) - 10.0, 0.0, 0);
    SetColor(edit, g_flTextRed, g_flTextGreen, g_flTextBlue, 1.0);
    SetOptionF(edit, TAG("ZOFS"), 2.0);
    SetOptionI(edit, TAG("HALN"), ALIGN_LEFT);
    SetOptionI(edit, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(edit, TAG("FSIZ"), 15);
    SetOptionS(edit, TAG("UTF8"), text);
    SetOptionI(edit, TAG("W64 "), (atoi(frame_w) - 32) * 64);
    SetOptionI(edit, TAG("H64 "), 20 * 64);
    SetOptionI(edit, TAG("LM64"), 60 * 64);
    SetOptionI(edit, TAG("RM64"), 60 * 64);
    {
        new bg_colors[4];
        static bg_bytes[16];
        pack_color_bytes(bg_colors, bg_bytes);
        SetOptionP(edit, TAG("BGC4"), bg_colors, 4);
    }
    sndSePlay(SE_KEYBOARD_OPEN);
    while (ANIM_DONE != AnimGetState(backdrop, ANIM_COLOR))
    {
        sleep;
    }
    new skb;
    if (ucs4)
    {
        skb = skbStart(edit, kind, input_mode, flags, length_unit, max_length, g_szAddrForbidden, 0);
    }
    else
    {
        skb = skbStart(edit, kind, input_mode, flags, length_unit, max_length, "", 0);
    }
    if (skb == -1)
    {
        return 0;
    }
    new ok;
    if (wait_skb() == 1)
    {
        skbGetString(text, 128);
        ok = 1;
    }
    new Float:color[4];
    GetColor(caption, color[0], color[1], color[2], color[3]);
    AnimColor(caption, color[0], color[1], color[2], 0.0, g_flFadeTime, 0.0, ANIM_FUNC_LINEAR);
    GetColor(frame, color[0], color[1], color[2], color[3]);
    AnimColor(frame, color[0], color[1], color[2], 0.0, g_flFadeTime, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(backdrop, g_flBackdropRed, g_flBackdropGreen, g_flBackdropBlue, 0.0, g_flFadeTime, 0.0, ANIM_FUNC_LINEAR);
    Destroy(edit);
    while (ANIM_DONE != AnimGetState(backdrop, ANIM_COLOR))
    {
        sleep;
    }
    Destroy(backdrop);
    return ok;
}
