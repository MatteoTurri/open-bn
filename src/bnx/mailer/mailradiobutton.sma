/*
 * /opt0/bn/script/mailer/mailradiobutton.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A choice of the mailer settings: a list of up to ten choices (arguments "btn0" to "btn9") under a
 * title ("label"), "sel" selected first; with "flg", the triangle button on the choice "next" edits the
 * signature text with mailsetskb.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "mailer.inc"

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

#include "misc.inc"

new Float:g_flTitleRed = 0.902;
new Float:g_flTitleGreen = 0.902;
new Float:g_flTitleBlue = 0.215;
new Float:g_flLabelRed = 0.215;
new Float:g_flLabelGreen = 0.708;
new Float:g_flLabelBlue = 0.999;
new Float:g_flDimAlpha = 0.2;
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

public g_manager; /* the settings page: gets "SHWT" when the list is ready */
public g_focus; /* the focus object, moved onto the selected choice */
public g_active; /* 1 while the list has the focus and takes the pad */
public g_select; /* the selected choice */
new g_objTitle;
new g_objLabels[10];
new g_objKeyboard; /* the mailsetskb object of the signature edit */
new g_iCount;
new g_bCanEdit; /* argument "flg": the choice "next" can be edited */

main()
{
    new value[16];
    new Float:pos[3];
    new Float:line_h;
    new edit_index = -1;
    {
        new arg_value[20];
        if (GetArg("next", arg_value, 20))
        {
            edit_index = atoi(arg_value);
        }
    }
    g_bCanEdit = 0;
    if (GetArg("flg", value, 16))
    {
        g_bCanEdit = 1;
    }
    get_group_tag_attr2("SEDIT", "HEAD", 0, "folder_x", value, 16);
    pos[0] = atof(value);
    get_group_tag_attr2("SEDIT", "HEAD", 0, "folder_y", value, 16);
    pos[1] = atof(value);
    get_group_tag_attr2("SEDIT", "HEAD", 0, "folder_z", value, 16);
    pos[2] = atof(value);
    g_objTitle = CreateLabel(Self());
    SetPos(g_objTitle, pos[0], pos[1], pos[2], Disp2D(0));
    SetColor(g_objTitle, g_flTitleRed, g_flTitleGreen, g_flTitleBlue, 0.0);
    SetOptionI(g_objTitle, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(g_objTitle, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_objTitle, TAG("FSIZ"), 17);
    SetOptionI(g_objTitle, TAG("ZSRT"), ZSORT_OVERLAY);
    {
        new label[64];
        if (GetArg("label", label, 64))
        {
            SetOptionS(g_objTitle, TAG("UTF8"), label);
        }
    }
    get_group_tag_attr2("SEDIT", "HEAD", 0, "radio_x", value, 16);
    pos[0] = atof(value);
    get_group_tag_attr2("SEDIT", "HEAD", 0, "radio_y", value, 16);
    pos[1] = atof(value);
    get_group_tag_attr2("SEDIT", "HEAD", 0, "radio_z", value, 16);
    pos[2] = atof(value);
    get_group_tag_attr2("SEDIT", "HEAD", 0, "line_height", value, 16);
    line_h = atof(value);
    for (new i; i < 10; i++)
    {
        new key[64];
        new label[64];
        sprintf(key, "btn%d", i);
        if (GetArg(key, label, 64))
        {
            g_objLabels[i] = CreateLabel(Self());
            SetPos(g_objLabels[i], pos[0], pos[1] + line_h * i, pos[2], 0);
            SetColor(g_objLabels[i], g_flLabelRed, g_flLabelGreen, g_flLabelBlue, 0.0);
            SetOptionS(g_objLabels[i], TAG("UTF8"), label);
            SetOptionI(g_objLabels[i], TAG("HALN"), ALIGN_CENTER);
            SetOptionI(g_objLabels[i], TAG("VALN"), ALIGN_CENTER);
            SetOptionI(g_objLabels[i], TAG("FSIZ"), 15);
            SetOptionI(g_objLabels[i], TAG("ZSRT"), ZSORT_OVERLAY);
            g_iCount++;
        }
        else
        {
            break;
        }
    }
    InitSelect();
    sleep;
    SendMessage(Self(), TAG("FADE"), TAG("IN"), 0, 0, 0);
    SendMessage(Self(), TAG("FOCS"), TAG("IN"), 0, 0, 0);
    SendMessage(Self(), TAG("BLNK"), TAG("IN"), 0, 0, 0);
    SendMessage(g_manager, TAG("SHWT"), 0, 0, 0, 0);
    for (; ; )
    {
        g_select = clamp(g_select, 0, g_iCount - 1);
        new held;
        new pressed;
        new released;
        new repeated;
        if (g_active)
        {
            PadState(held, pressed, released, repeated, -1);
        }
        /* the focus must move to the selected choice */
        static refocus;
        if (repeated & PAD_UP)
        {
            if (g_select - 1 >= 0)
            {
                g_select--;
                sndSePlay(SE_CURSOR);
                ShowPadHelp(edit_index);
                refocus = 1;
            }
        }
        else if (repeated & PAD_DOWN)
        {
            if (g_select + 1 < g_iCount)
            {
                g_select++;
                sndSePlay(SE_CURSOR);
                ShowPadHelp(edit_index);
                refocus = 1;
            }
        }
        else if (pressed & PAD_TRIANGLE)
        {
            if (g_bCanEdit && edit_index == g_select)
            {
                EditSignature();
            }
        }
        if (g_active && refocus)
        {
            send_label_focus2(g_focus, g_objLabels[g_select], 0, 0.5, 60.0, 60.0, 70.0, 1, 1);
            refocus = 0;
        }
        new msg;
        new what;
        new arg;
        /* "FOCS" IN/OUT, "FADE" IN/OUT/DIM; then "FNIS"/"FNIB" from mailsetskb with the edited signature. */
        while (RecvMessage(msg, what, arg))
        {
            if (msg == TAG("FOCS"))
            {
                if (what == TAG("IN"))
                {
                    ShowPadHelp(edit_index);
                    g_active = 1;
                    refocus = 1;
                }
                else if (what == TAG("OUT"))
                {
                    g_active = 0;
                    send_focus(g_focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                }
            }
            else if (msg == TAG("BLNK"))
            {
                if (what == TAG("IN"))
                {
                }
                else if (what == TAG("OUT"))
                {
                }
            }
            else if (msg == TAG("FADE"))
            {
                if (what == TAG("IN"))
                {
                    FadeIn();
                }
                else if (what == TAG("OUT"))
                {
                    FadeOut();
                }
                else if (what == TAG("DIM"))
                {
                    FadeDim();
                }
            }
        }
        new text[128];
        while (RecvMessageStr(msg, text, 128))
        {
            if (msg == TAG("FNIS"))
            {
                FinishEdit(text, 1);
            }
            else if (msg == TAG("FNIB"))
            {
                FinishEdit(text, 0);
            }
        }
        sleep;
    }
    return;
}

/* Fades the title and the choices to alpha. */
FadeItems(Float:alpha, Float:duration, Float:delay)
{
    new Float:color[4];
    for (new i; i < 10; i++)
    {
        if (IsValidObject(g_objLabels[i]))
        {
            GetColor(g_objLabels[i], color[0], color[1], color[2], color[3]);
            AnimColor(g_objLabels[i], color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
        }
    }
    GetColor(g_objTitle, color[0], color[1], color[2], color[3]);
    AnimColor(g_objTitle, color[0], color[1], color[2], alpha, duration, delay, ANIM_FUNC_LINEAR);
}

FadeIn()
{
    FadeItems(1.0, float(400), 0.0);
}

FadeOut()
{
    FadeItems(0.0, float(400), 0.0);
}

FadeDim()
{
    FadeItems(g_flDimAlpha, float(400), 0.0);
}

/* Shows the pad help; on the choice edit_index also the triangle, labelled to edit it. */
ShowPadHelp(edit_index)
{
    new label[16];
    if (edit_index >= 0 && edit_index == g_select)
    {
        get_tag_attr("TEXT", "msgmailer_btn_enter", "value", label, 16);
        SetSystemParamStr(SYS_STR_PADHELP_RR, label);
        get_tag_attr("TEXT", "msgmailer_btn_set", "value", label, 16);
        SetSystemParamStr(SYS_STR_PADHELP_RU, label);
        SetSystemParam(SYS_PADHELP, PADHELP_CIRCLE | PADHELP_CROSS | PADHELP_TRIANGLE);
    }
    else
    {
        SetSystemParam(SYS_PADHELP, PADHELP_CIRCLE | PADHELP_CROSS);
    }
}

/* Selects the choice of the argument "sel". */
InitSelect()
{
    new value[64];
    new sel;
    if (GetArg("sel", value, 64))
    {
        sel = atoi(value);
    }
    sel = clamp(sel, 0, g_iCount - 1);
    g_select = sel;
}

/* Opens mailsetskb on the signature text and takes the focus away from the list. */
EditSignature()
{
    new title[32];
    get_tag_attr("TEXT", "msgmailer_set_item_write_signature_text", "value", title, 32);
    new args[16];
    new signature[23];
    mx_GetPrefStr("SIG_TEXT", signature, 90);
    g_objKeyboard = CreateNull(Self());
    sprintf(args, "mode=%d;input=%d;opt=%d;type=%d;limit=%d;title=%s;", SKB_KEYBOARD_HIRAGANA, SKB_INPUT_DEFAULT, 0,
            SKB_LENGTH_BYTES, 90, title);
    AttachScript(g_objKeyboard, _, "mailsetskb", args);
    SetPubVar(g_objKeyboard, "g_manager", Self());
    SetOptionI(g_objKeyboard, TAG("ZSRT"), ZSORT_OVERLAY);
    SendMessageStr(g_objKeyboard, TAG("INIT"), signature);
    SendMessage(Self(), TAG("FOCS"), TAG("OUT"), 0, 0, 0);
}

/* Ends the signature edit: on ok shows the new text on its choice and stores it in the preferences. */
FinishEdit(text[], ok)
{
    if (ok)
    {
        new shown[64];
        if (strlen(text))
        {
            new trimmed[128];
            copy_string(trimmed, text, 128);
            mx_TrimString(trimmed, 10);
            sprintf(shown, "\"%s\"", trimmed);
        }
        else
        {
            get_tag_attr("TEXT", "msgmailer_set_item_write_signature_text", "value", shown, 64);
        }
        SetOptionS(g_objLabels[3], TAG("UTF8"), shown);
        mx_SetPrefStr("SIG_TEXT", text);
        mx_Restart();
    }
    Destroy(g_objKeyboard);
    SendMessage(Self(), TAG("FOCS"), TAG("IN"), 0, 0, 0);
}

#include "mailsync.inc"
