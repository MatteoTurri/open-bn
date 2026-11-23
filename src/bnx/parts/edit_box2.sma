/*
 * /opt0/bn/script/parts/edit_box2.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A text field (argument "text") edited with the software keyboard in a multi-line editor. While it has the focus
 * it passes the direction buttons to its manager as focus moves and the other buttons as presses.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "wait.inc"

#include "tag.inc"

#include <float>

/* As in text.inc, except mode 7, another keyboard here. */
const EDIT_MODE_USER_ID = 1;
const EDIT_MODE_PASSWORD = 2; /* shown as asterisks */
const EDIT_MODE_KATAKANA = 3;
const EDIT_MODE_NUMBER = 4;

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

/* Like run_edit_dialog, with a multi-line editor and without the title. */
run_multiline_edit(title[], text[], edit_mode, maxlen, max_chars)
{
    if (maxlen * 4 < max_chars)
    {
        max_chars = maxlen * 4;
    }
    new backdrop = CreateDefocus(Disp2D(0));
    SetOptionI(backdrop, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(backdrop, TAG("LEVL"), 3);
    SetColor(backdrop, 1.0, 1.0, 1.0, 0.0);
    AnimColor(backdrop, 0.3, 0.3, 0.3, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
    wait_ms(200);
    new edit = CreateEditML(backdrop);
    if (edit_mode == EDIT_MODE_PASSWORD)
    {
        SetOptionI(edit, TAG("PWMD"), '*');
    }
    SetPos(edit, 100.0, 70.0, 0.0, 0);
    SetColor(edit, 1.0, 1.0, 1.0, 1.0);
    SetOptionF(edit, TAG("ZOFS"), 2.0);
    SetOptionI(edit, TAG("HALN"), ALIGN_LEFT);
    SetOptionI(edit, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(edit, TAG("FSIZ"), 15);
    SetOptionS(edit, TAG("UTF8"), text);
    SetOptionI(edit, TAG("W64 "), 396 * 64);
    SetOptionI(edit, TAG("H64 "), 80 * 64);
    SetOptionI(edit, TAG("h64 "), 240 * 64);
    SetOptionI(edit, TAG("LM64"), 60 * 64);
    SetOptionI(edit, TAG("RM64"), 60 * 64);
    {
        new bg_colors[4];
        static bg_bytes[16];
        pack_color_bytes(bg_colors, bg_bytes);
        SetOptionP(edit, TAG("BGC4"), bg_colors, 4);
    }
    save_pad_help();
    new skb;
    switch (edit_mode)
    {
        case EDIT_MODE_USER_ID:
        {
            new ascii_chars[96] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q',
                    'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', ' ', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
                    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '1', '2', '3', '4',
                    '5', '6', '7', '8', '9', '0', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '-', '=', '\\', '`',
                    '_', '+', '|', '~', '[', ']', '{', '}', ';', ':', '\'', '"', ',', '.', '<', '>', '/', '?', 0};
            skb = skbStart(edit, SKB_KEYBOARD_QWERTY, SKB_INPUT_DIRECT,
                    SKB_LOCK_KEYBOARD | SKB_LOCK_INPUT_MODE | SKB_KANA_LOCK_FIXED, SKB_LENGTH_BYTES, max_chars,
                    ascii_chars, 1);
        }
        case EDIT_MODE_PASSWORD:
        {
            new password_chars[96] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
                    'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', ' ', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
                    'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '1', '2', '3',
                    '4', '5', '6', '7', '8', '9', '0', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '-', '=', '\\',
                    '`', '_', '+', '|', '~', '[', ']', '{', '}', ';', ':', '\'', '"', ',', '.', '<', '>', '/', '?', 0};
            skb = skbStart(edit, SKB_KEYBOARD_QWERTY, SKB_INPUT_DIRECT,
                    SKB_LOCK_KEYBOARD | SKB_LOCK_INPUT_MODE | SKB_KANA_LOCK_FIXED, SKB_LENGTH_BYTES, max_chars,
                    password_chars, 1);
        }
        case EDIT_MODE_KATAKANA:
        {
            skb = skbStart(edit, SKB_KEYBOARD_KATAKANA, SKB_INPUT_KATAKANA,
                    SKB_LOCK_KEYBOARD | SKB_LOCK_INPUT_MODE | SKB_KANA_LOCK_FIXED, SKB_LENGTH_BYTES, max_chars, _, 0);
        }
        case 5:
        {
            skb = skbStart(edit, SKB_KEYBOARD_KATAKANA, SKB_INPUT_KATAKANA_FIXED,
                    SKB_LOCK_KEYBOARD | SKB_LOCK_INPUT_MODE | SKB_KANA_LOCK_FIXED | SKB_FIXED_INPUT, SKB_LENGTH_BYTES,
                    max_chars, _, 0);
        }
        case 6:
        {
            skb = skbStart(edit, SKB_KEYBOARD_KATAKANA, SKB_INPUT_HALF_KATAKANA_FIXED,
                    SKB_LOCK_KEYBOARD | SKB_LOCK_INPUT_MODE | SKB_KANA_LOCK_FIXED | SKB_FIXED_INPUT | SKB_HALF_WIDTH,
                    SKB_LENGTH_BYTES, max_chars, _, 0);
        }
        case 7:
        {
            skb = skbStart(edit, SKB_KEYBOARD_HIRAGANA, SKB_INPUT_HIRAGANA_FIXED,
                    SKB_LOCK_KEYBOARD | SKB_LOCK_INPUT_MODE | SKB_KANA_LOCK_FIXED | SKB_FIXED_INPUT, SKB_LENGTH_BYTES,
                    max_chars, _, 0);
        }
        case EDIT_MODE_NUMBER:
        {
            new digits[11] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 0};
            skb = skbStart(edit, SKB_KEYBOARD_NUMBERS, SKB_INPUT_DIRECT,
                    SKB_LOCK_KEYBOARD | SKB_LOCK_INPUT_MODE | SKB_KANA_LOCK_FIXED, SKB_LENGTH_BYTES, max_chars, digits,
                    1);
        }
        default:
        {
            skb = skbStart(edit, SKB_KEYBOARD_DEFAULT, SKB_INPUT_DEFAULT, 0, SKB_LENGTH_BYTES, max_chars, _, 0);
        }
    }
    new ok;
    if (skb == -1)
    {
        printf("edit:");
        getstring(text, maxlen);
        sleep;
        ok = 1;
    }
    else if (wait_skb() == 1)
    {
        skbGetString(text, maxlen);
        ok = 1;
    }
    else
    {
        ok = 0;
    }
    Destroy(edit);
    AnimColor(backdrop, 0.3, 0.3, 0.3, 0.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
    wait_ms(200);
    Destroy(backdrop);
    restore_pad_help();
    return ok;
}

#include "pfs.inc"

#include "background.inc"

public manager; /* the object told of focus moves, changes and presses */
public focus; /* the focus object */
public padhelp; /* pad help buttons shown besides the circle while the box has the focus */
public active; /* 1 while the box has the focus */
public Float:meta_alpha = 1.0; /* overall alpha, set by "FADE" */
public Float:width = 380.0; /* of the field */
public mode; /* the edit mode of the keyboard (EDIT_MODE_PASSWORD hides the text) */
public max_byte = 2048; /* the longest text the keyboard takes, in bytes */
new g_label;
new g_frame; /* the field drawn around the text */
new g_text[256];

main()
{
    GetArg("text", g_text, 256);
    g_label = CreateLabel(Self());
    SetColor(g_label, 0.215, 0.708, 0.999, 0.0);
    set_label_text(g_label, g_text);
    SetOptionI(g_label, TAG("HALN"), ALIGN_LEFT);
    SetOptionI(g_label, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(g_label, TAG("FSIZ"), 15);
    SetOptionI(g_label, TAG("W64 "), floatround(width - 45.0, 0) * 64);
    g_frame = CreatePlane3x3(Self());
    SetPos(g_frame, 0.5 * width - 20.0, 0.0, 0.0, 0);
    SetScale(g_frame, width, 48.0, 1.0);
    SetColor(g_frame, 1.0, 1.0, 1.0, 0.0);
    SetTexture(g_frame, imgGetResource(RES_EDITBOX), 0, -1);
    SetOptionI(g_frame, TAG("ALPH"), BLEND_ADD);
    SetOptionI(g_frame, TAG("ZSRT"), ZSORT_BACKGROUND);
    {
        new Float:tex_margins[4] = {0.5, 0.5, 0.5, 0.5};
        SetOptionP(g_frame, TAG("MGNT"), tex_margins, 4);
        new Float:scale[3];
        GetScale(g_frame, scale[0], scale[1], scale[2]);
        new Float:margins[4];
        if (scale[0] > scale[1])
        {
            margins[0] = margins[1] = 0.5 * scale[1] / scale[0];
            margins[2] = margins[3] = 0.5;
        }
        else
        {
            margins[2] = margins[3] = 0.5 * scale[0] / scale[1];
            margins[0] = margins[1] = 0.5;
        }
        SetOptionP(g_frame, TAG("MGN"), margins, 4);
    }
    update_color(100.0, 0.0);
    wait_ms(100);
    new pushed;
    new changed;
    for (; ; )
    {
        new msg;
        new a;
        new Float:b;
        /*
         * "FOCS" (direction): the focus arrives or leaves ("FOUT"); "GTXT": the text is sent back to the manager;
         * "FADE" (alpha, duration); "PUSH" and "CHNG": the manager has handled the last press or change.
         */
        while (RecvMessage(msg, a, b))
        {
            if (msg == TAG("FOCS"))
            {
                receive_focus(a);
            }
            else if (msg == TAG("GTXT"))
            {
                SendMessageStr(manager, TAG("GTXT"), g_text);
            }
            else if (msg == TAG("FADE"))
            {
                meta_alpha = a;
                update_color(b, 0.0);
            }
            else if (msg == TAG("PUSH"))
            {
                pushed = 0;
            }
            else if (msg == TAG("CHNG"))
            {
                changed = 0;
            }
        }
        new str[256];
        /* "TEXT" (string): replaces the text. */
        while (RecvMessageStr(msg, str, 256))
        {
            if (msg == TAG("TEXT"))
            {
                copy_string(g_text, str, 256);
                set_label_text(g_label, str);
            }
        }
        /*
         * A direction asks the manager to move the focus ("FOCS"); the circle opens the keyboard (argument "title")
         * and an edit is a change ("CHNG"); another button is a press ("PUSH" 0, button).
         */
        if (active && !pushed && !changed)
        {
            new pressed;
            PadState(_, pressed, _, _, -1);
            if (pressed & PAD_LEFT)
            {
                SendMessage(manager, TAG("FOCS"), TAG("LEFT"), 0, 0, 0);
                lose_focus();
                goto next_frame;
            }
            if (pressed & PAD_RIGHT)
            {
                SendMessage(manager, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
                lose_focus();
                goto next_frame;
            }
            if (pressed & PAD_UP)
            {
                SendMessage(manager, TAG("FOCS"), TAG("UP"), 0, 0, 0);
                lose_focus();
                goto next_frame;
            }
            if (pressed & PAD_DOWN)
            {
                SendMessage(manager, TAG("FOCS"), TAG("DOWN"), 0, 0, 0);
                lose_focus();
                goto next_frame;
            }
            if (pressed & PAD_CIRCLE)
            {
                new title[256];
                GetArg("title", title, 256);
                if (run_multiline_edit(title, g_text, mode, 256, max_byte))
                {
                    set_label_text(g_label, g_text);
                    SendMessage(manager, TAG("CHNG"), 0, 0, 0, 0);
                    changed = 1;
                }
            }
            else if (pressed & PAD_CROSS)
            {
                SendMessage(manager, TAG("PUSH"), 0, TAG("RDWN"), 0, 0);
                pushed = 1;
            }
            else if (pressed & PAD_TRIANGLE)
            {
                SendMessage(manager, TAG("PUSH"), 0, TAG("RUP"), 0, 0);
                pushed = 1;
            }
            else if (pressed & PAD_SQUARE)
            {
                SendMessage(manager, TAG("PUSH"), 0, TAG("RLFT"), 0, 0);
                pushed = 1;
            }
            else if (pressed & PAD_SELECT)
            {
                SendMessage(manager, TAG("PUSH"), 0, TAG("SLCT"), 0, 0);
                pushed = 1;
            }
            else if (pressed & PAD_START)
            {
                SendMessage(manager, TAG("PUSH"), 0, TAG("STRT"), 0, 0);
                pushed = 1;
            }
        }
        next_frame:
        sleep;
    }
    return;
}

/* Fades the label and the field to their colors for the focus state and meta_alpha. */
update_color(Float:duration, Float:delay)
{
    {
        new Float:label_alpha = 0.5;
        if (active)
        {
            label_alpha = 1.0;
        }
        AnimColor(g_label, 0.215, 0.708, 0.999, label_alpha * meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
    }
    AnimColor(g_frame, 1.0, 1.0, 1.0, 0.5 * meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
}

/* Handles "FOCS": takes the focus, or gives it up if direction is "FOUT". */
receive_focus(direction)
{
    active = 1;
    if (direction == TAG("FOUT"))
    {
        lose_focus();
        return;
    }
    if (direction == TAG("LEFT"))
    {
        sndSePlay(SE_CURSOR);
    }
    else if (direction == TAG("RIGT"))
    {
        sndSePlay(SE_CURSOR);
    }
    else if (direction == TAG("UP"))
    {
        sndSePlay(SE_CURSOR);
    }
    else if (direction == TAG("DOWN"))
    {
        sndSePlay(SE_CURSOR);
    }
    else if (direction == TAG("FRST"))
    {
        printf("got initial focus.\n");
    }
    send_focus(focus, g_frame, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    update_color(100.0, 0.0);
    show_pad_help();
}

/* Leaves the focus and dims the colors. */
lose_focus()
{
    active = 0;
    update_color(100.0, 40.0);
}

/* Shows the pad help of padhelp and the circle. */
show_pad_help()
{
    new buttons = padhelp;
    buttons = buttons | PADHELP_CIRCLE;
    SetSystemParam(SYS_PADHELP, buttons);
}

/* Shows text on label, as asterisks in EDIT_MODE_PASSWORD. */
set_label_text(label, text[])
{
    if (mode == EDIT_MODE_PASSWORD)
    {
        new masked[256];
        copy_string(masked, text, 256);
        for (new i; text{i}; i++)
        {
            masked{i} = '*';
        }
        SetOptionS(label, TAG("UTF8"), masked);
    }
    else
    {
        SetOptionS(label, TAG("UTF8"), text);
    }
}
