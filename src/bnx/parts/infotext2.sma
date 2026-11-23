/*
 * /opt0/bn/script/parts/infotext2.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A scrolling list of the lines of the TEXTAREA element with the id of its argument "id", one label per child
 * element; up and down move the cursor through the lines, the other buttons are reported to the manager.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "wait.inc"

#include "tag.inc"

#include <float>

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

public manager; /* the object told of focus moves and presses */
public focus; /* the focus object */
public padhelp; /* pad help buttons shown besides the circle while the list has the focus */
public active;  /* 1 while the list has the focus */
public cursor;  /* the line under the focus */
public Float:meta_alpha = 1.0; /* overall alpha, set by "FADE" */
public visible = 8;            /* lines shown at a time */
public Float:width = 400.0;    /* the lines are squeezed to this width, in pixels */
public scroll_pos;             /* the first line shown */
new g_first_line; /* the line of the first label of the list, which may be out of view */
new Float:g_line_height = 20.0;
new g_line_count;
new g_textarea;
new g_line_tag; /* the child element of the last line read, and its index */
new g_line_tag_index;
new g_id[32];
new g_first_name[1]; /* empty: any element */
new g_next_name[1];

/* Returns the number of child elements of tag. */
count_children(tag)
{
    new count;
    for (new child = domFirstChild(tag, g_first_name); child; child = domNext(child, g_next_name))
    {
        count++;
    }
    return count;
}

main()
{
    GetArg("id", g_id, 32);
    g_textarea = domFindTag(domRoot(), "TEXTAREA", g_id, 0);
    g_line_count = count_children(g_textarea);
    g_line_tag = domFirstChild(g_textarea, g_first_name);
    g_line_tag_index = 0;
    visible = clamp(visible, 0, g_line_count);
    scroll_pos = clamp(cursor - 4, 0, g_line_count - visible);
    g_first_line = scroll_pos;
    printf("visible = %d\n", visible);
    printf("scroll_pos = %d\n", scroll_pos);
    printf("exist_top = %d\n", g_first_line);
    for (new i; i < visible; i++)
    {
        new line = i + g_first_line;
        new text[64];
        read_line_text(line, text);
        new label = create_line(g_line_height * i, text);
        listAddLast(label);
        sleep;
    }
    update_color(100.0, 0.0);
    new pushed;
    for (; ; )
    {
        sleep;
        new msg;
        new a;
        new Float:b;
        /* "FOCS" (direction), "FADE" (alpha, duration), "PUSH": the manager has handled the last press */
        if (RecvMessage(msg, a, b))
        {
            if (msg == TAG("FOCS"))
            {
                receive_focus(a);
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
        }
        if (active && !pushed)
        {
            cursor = clamp(cursor, 0, g_line_count - 1);
            {
                new repeated;
                PadState(_, _, _, repeated, -1);
                if (repeated & PAD_UP)
                {
                    if (cursor > 0)
                    {
                        cursor--;
                        sndSePlay(SE_SCROLL);
                        update_color(100.0, 0.0);
                        scroll_to_cursor();
                        focus_cursor();
                    }
                    else
                    {
                        SendMessage(manager, TAG("FOCS"), TAG("UP"), 0, 0, 0);
                        lose_focus();
                        continue;
                    }
                }
                else if (repeated & PAD_DOWN)
                {
                    if (cursor < g_line_count - 1)
                    {
                        cursor++;
                        sndSePlay(SE_SCROLL);
                        update_color(100.0, 0.0);
                        scroll_to_cursor();
                        focus_cursor();
                    }
                    else
                    {
                        SendMessage(manager, TAG("FOCS"), TAG("DOWN"), 0, 0, 0);
                        lose_focus();
                        continue;
                    }
                }
                else if (repeated & PAD_LEFT)
                {
                    SendMessage(manager, TAG("FOCS"), TAG("LEFT"), 0, 0, 0);
                    lose_focus();
                    continue;
                }
                else if (repeated & PAD_RIGHT)
                {
                    SendMessage(manager, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
                    lose_focus();
                    continue;
                }
                if (repeated & PAD_CIRCLE)
                {
                    SendMessage(manager, TAG("PUSH"), cursor, TAG("RRGT"), 0, 0);
                    pushed = 1;
                }
                else if (repeated & PAD_CROSS)
                {
                    SendMessage(manager, TAG("PUSH"), cursor, TAG("RDWN"), 0, 0);
                    pushed = 1;
                }
                else if (repeated & PAD_TRIANGLE)
                {
                    SendMessage(manager, TAG("PUSH"), cursor, TAG("RUP"), 0, 0);
                    pushed = 1;
                }
                else if (repeated & PAD_SQUARE)
                {
                    SendMessage(manager, TAG("PUSH"), cursor, TAG("RLFT"), 0, 0);
                    pushed = 1;
                }
                else if (repeated & PAD_SELECT)
                {
                    SendMessage(manager, TAG("PUSH"), cursor, TAG("SLCT"), 0, 0);
                    pushed = 1;
                }
                else if (repeated & PAD_START)
                {
                    SendMessage(manager, TAG("PUSH"), cursor, TAG("STRT"), 0, 0);
                    pushed = 1;
                }
            }
            drop_hidden_lines();
        }
    }
    return;
}

focus_cursor()
{
    send_focus(focus, listGetAt(cursor - g_first_line), 0, 0.5, 60.0, 60.0, 150.0, 0, 1);
}

/* Fades the lines in view to meta_alpha and the others out. */
update_color(Float:duration, Float:delay)
{
    for (new i; i < listSize(); i++)
    {
        new line = i + g_first_line;
        {
            new Float:alpha = 1.0;
            if (active && cursor == line) /* redundant as written */
            {
                alpha = 1.0;
            }
            if (line < scroll_pos)
            {
                alpha = 0.0;
            }
            if (line >= scroll_pos + visible)
            {
                alpha = 0.0;
            }
            AnimColor(listGetAt(i), 0.7843, 0.7843, 0.7843, alpha * meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
        }
    }
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
        sndSePlay(SE_SCROLL);
    }
    else if (direction == TAG("RIGT"))
    {
        sndSePlay(SE_SCROLL);
    }
    else if (direction == TAG("UP"))
    {
        sndSePlay(SE_SCROLL);
        cursor = 0;
    }
    else if (direction == TAG("DOWN"))
    {
        sndSePlay(SE_SCROLL);
        cursor = g_line_count - 1;
    }
    else if (direction == TAG("FRST"))
    {
        printf("got initial focus.\n");
    }
    focus_cursor();
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
    SetSystemParam(SYS_PADHELP, padhelp | PADHELP_CIRCLE);
}

/* Creates the label of a line at y, squeezed to width by the labelsize script. */
create_line(Float:y, text[])
{
    new label = CreateLabel(Self());
    SetPos(label, 0.0, y, 0.0, 0);
    SetColor(label, 0.7843, 0.7843, 0.7843, 0.0);
    SetOptionI(label, TAG("ALPH"), BLEND_ADD);
    SetOptionI(label, TAG("HALN"), ALIGN_LEFT);
    SetOptionI(label, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(label, TAG("FSIZ"), 15);
    SetOptionS(label, TAG("UTF8"), text);
    AttachScript(label, _, "labelsize");
    SetPubVar(label, "max_width", width);
    return label;
}

new g_scroll_tick;
new g_last_scroll_tick;
new g_text_attr[1]; /* empty: the text of the element */
new g_prev_name[1];

/*
 * Scrolls the list by a line when the cursor leaves the view, adding the label that comes into view; the move
 * takes the time since the previous scroll, from 60 to 300 ms.
 */
scroll_to_cursor()
{
    new scrolled;
    if (cursor < scroll_pos)
    {
        scroll_pos--;
        if (g_first_line - 1 >= 0)
        {
            new text[64];
            read_line_text(g_first_line - 1, text);
            new label = create_line(-1.0 * g_line_height, text);
            listInsert(0, label);
            g_first_line--;
            sleep;
        }
        scrolled = 1;
    }
    else if (cursor >= scroll_pos + visible)
    {
        scroll_pos++;
        if (g_first_line + listSize() < g_line_count)
        {
            new text[64];
            read_line_text(g_first_line + listSize(), text);
            new label = create_line(g_line_height * visible, text);
            listAddLast(label);
            sleep;
        }
        scrolled = 1;
    }
    if (scrolled)
    {
        g_last_scroll_tick = g_scroll_tick;
        g_scroll_tick = tickcount();
        new duration = g_scroll_tick - g_last_scroll_tick;
        duration = clamp(duration, 60, 300);
        update_color(float(duration), 0.0);
        for (new i; i < listSize(); i++)
        {
            new line = i + g_first_line;
            new Float:y = g_line_height * (line - scroll_pos);
            AnimMoveLine(listGetAt(i), 0.0, y, 0.0, float(duration), 0.0, ANIM_FUNC_LINEAR);
        }
        wait_ms(60);
    }
}

/* Destroys the first label once it is above the view and the last once it is below. */
drop_hidden_lines()
{
    new Float:y;
    GetPos(listGetAt(0), _, y, _, 0);
    if (y < -1.0 * g_line_height)
    {
        Destroy(listGetAt(0));
        listRemoveAt(0);
        g_first_line++;
    }
    GetPos(listGetLast(), _, y, _, 0);
    if (y > g_line_height * visible)
    {
        Destroy(listGetLast());
        listRemoveLast();
    }
}

/* Reads the text of line index, walking from the last line read. */
read_line_text(index, text[])
{
    for (; ; )
    {
        if (index == g_line_tag_index)
        {
            domGetData(g_line_tag, g_text_attr, text, 64);
            return;
        }
        if (index > g_line_tag_index)
        {
            g_line_tag = domNext(g_line_tag, g_next_name);
            g_line_tag_index++;
        }
        else
        {
            g_line_tag = domPrev(g_line_tag, g_prev_name);
            g_line_tag_index--;
        }
    }
}
