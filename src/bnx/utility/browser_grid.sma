/*
 * /opt0/bn/script/utility/browser_grid.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The grid of the saves of an album of the file browser: their icons on tilted tiles, three to a row and
 * three rows in view, scrolled by the pad. 27 tiles are reused for the items around the view.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "tag.inc"

#include <float>

/* The dir of show_cell and hide_cell that only fades the tile. */
const MOVE_NONE = 0x8000;

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

public owner; /* gets "PUSH" (index, button: RUP, "RDWN" or "RLFT") and "SCRL" (yview, dir) */
public focus; /* the focus frame */
public padhelp; /* the pad help buttons (PADHELP_* bits) */
public size; /* number of items */
public cursor; /* index of the selected item */
public select; /* unused, but part of the layout */
public yview; /* the first row in view */
public idle; /* 1 once the pad has been left alone for a while */
public texture0; /* the front of a tile */
public texture1; /* the back of a tile */
new g_repeat_delay = 15; /* frames between two moves while a direction is held */
new g_buffered_dirs; /* directions pressed during the wait, done after it */
new g_has_focus = 1;
new g_pushed; /* a "PUSH" is sent: the pad waits for the "PUSH" of the owner */
new g_move_wait;
new g_max_yview;
new g_root;
new g_cells[27];
new g_backs[27];
new g_icons[27];
new g_busy_frames = 60; /* frames left before idle is set */
new g_cleanup_wait; /* frames after a scroll before the tiles below the view are hidden */
new g_icon_files[1024]; /* the number of the icon file (/opt0/cache/ictmpNNNN) of each item, -1 for none */
new g_shown_files[27]; /* the number of the icon file shown on each tile */

main()
{
    init_grid();
    for (; ; )
    {
        update();
        sleep;
    }
}

/* Computes the last row of the view, clears the icon files and creates the tiles. */
init_grid()
{
    if (size <= 0)
    {
        padhelp = padhelp & ~(PADHELP_CIRCLE | PADHELP_TRIANGLE);
    }
    g_max_yview = (size + 3 - 1) / 3 - 3;
    if (g_max_yview < 0)
    {
        g_max_yview = 0;
    }
    idle = 0;
    for (new i; i < 1024; i++)
    {
        g_icon_files[i] = -1;
    }
    for (new i; i < 27; i++)
    {
        g_shown_files[i] = -1;
    }
    create_cells();
    move_cursor(cursor);
}

/* Handles the messages and the pad, hides the tiles left below the view after a scroll, and keeps idle. */
update()
{
    new msg;
    new a;
    new b;
    /* "FOCS" (command), "FADE" (alpha, duration), "PUSH": the owner is done, "LOAD" (index, number of the icon file) */
    while (RecvMessage(msg, a, b))
    {
        if (msg == TAG("FOCS"))
        {
            handle_focus(a);
        }
        if (msg == TAG("FADE"))
        {
            fade_view(a, b);
        }
        if (msg == TAG("PUSH"))
        {
            end_push();
        }
        if (msg == TAG("LOAD"))
        {
            set_icon_file(a, b);
        }
    }
    if (g_has_focus && !g_pushed)
    {
        handle_pad();
    }
    if (g_cleanup_wait > 0)
    {
        g_cleanup_wait--;
        if (0 == g_cleanup_wait)
        {
            new index = yview * 3 + 9;
            for (new i; i < 18; i++)
            {
                remove_cell(index++);
            }
        }
    }
    if (g_busy_frames > 0)
    {
        g_busy_frames--;
        idle = 0;
    }
    else
    {
        idle = 1;
    }
}

end_push()
{
    g_pushed = 0;
}

set_icon_file(index, file)
{
    if (index < 0 || index >= 1024)
    {
        return;
    }
    g_icon_files[index] = file;
    update_icon(index, 1);
}

fade_view(Float:alpha, duration)
{
    new shown = alpha > 0.0;
    for (new i; i < 9; i++)
    {
        new index = yview * 3 + i;
        if (shown)
        {
            show_cell(index, MOVE_NONE, duration);
        }
        else
        {
            hide_cell(index, MOVE_NONE, duration);
        }
    }
}

/*
 * "FOCS": "LEFT", "RIGT", UP and "DOWN" (the cursor goes to the first or last item), "FRST", "FOUT" (the focus leaves).
 */
handle_focus(cmd)
{
    if (cmd == TAG("LEFT"))
    {
        sndSePlay(SE_CURSOR);
    }
    else if (cmd == TAG("RIGT"))
    {
        sndSePlay(SE_CURSOR);
    }
    else if (cmd == TAG("UP"))
    {
        cursor = 0;
        sndSePlay(SE_CURSOR);
    }
    else if (cmd == TAG("DOWN"))
    {
        cursor = size - 1;
        sndSePlay(SE_CURSOR);
    }
    else if (cmd == TAG("FRST"))
    {
        move_cursor(cursor);
    }
    else if (cmd == TAG("FOUT"))
    {
        leave_focus();
        return;
    }
    if (size > 0)
    {
        send_focus(focus, g_cells[cursor % 27], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    }
    show_pad_help();
    if (!g_has_focus)
    {
        g_has_focus = 1;
        move_cursor(cursor);
    }
}

/* Moves the cursor with the directions; triangle, cross and square send "PUSH" to the owner. */
handle_pad()
{
    new held;
    new pressed;
    PadState(held, pressed, _, _, -1);
    if (g_move_wait > 0)
    {
        g_move_wait--;
        if (0 == g_move_wait)
        {
            held = held | g_buffered_dirs;
            g_buffered_dirs = 0;
        }
    }
    if (0 == held)
    {
        return;
    }
    new dirs = held & (PAD_UP | PAD_RIGHT | PAD_DOWN | PAD_LEFT);
    if (g_move_wait > 0)
    {
        dirs = 0;
        g_buffered_dirs = g_buffered_dirs | pressed & (PAD_UP | PAD_RIGHT | PAD_DOWN | PAD_LEFT);
    }
    if (0 != dirs)
    {
        g_busy_frames = 30;
        if (dirs & PAD_UP)
        {
            g_move_wait = g_repeat_delay;
            if (move_cursor(cursor - 3))
            {
                sndSePlay(SE_CURSOR);
            }
            else
            {
            }
        }
        else if (dirs & PAD_DOWN)
        {
            g_move_wait = g_repeat_delay;
            if (move_cursor(cursor + 3))
            {
                sndSePlay(SE_CURSOR);
            }
            else
            {
            }
        }
        else if (dirs & PAD_LEFT)
        {
            g_move_wait = g_repeat_delay;
            if (0 != cursor % 3)
            {
                if (move_cursor(cursor - 1))
                {
                    sndSePlay(SE_CURSOR);
                }
            }
            else
            {
            }
        }
        else if (dirs & PAD_RIGHT)
        {
            g_move_wait = g_repeat_delay;
            if (2 != cursor % 3)
            {
                if (move_cursor(cursor + 1))
                {
                    sndSePlay(SE_CURSOR);
                }
            }
            else
            {
            }
        }
        sleep;
    }
    if (0 != pressed)
    {
        if (pressed & PAD_TRIANGLE)
        {
            if (size > 0)
            {
                SendMessage(owner, TAG("PUSH"), cursor, TAG("RUP"), 0, 0);
                g_pushed = 1;
            }
        }
        else if (pressed & PAD_CROSS)
        {
            SendMessage(owner, TAG("PUSH"), cursor, TAG("RDWN"), 0, 0);
            g_pushed = 1;
        }
        else if (pressed & PAD_SQUARE)
        {
            SendMessage(owner, TAG("PUSH"), cursor, TAG("RLFT"), 0, 0);
            g_pushed = 1;
        }
    }
}

/*
 * Moves the cursor to index: the old tile sinks and dims, the new one rises; an index in the row past the last
 * item goes to the last item. Scrolls the view when the cursor leaves it; returns whether the cursor moved.
 */
move_cursor(index)
{
    if (index < 0)
    {
        return 0;
    }
    if (index >= size)
    {
        if (index - (size - 1) / 3 * 3 >= 3)
        {
            return 0;
        }
        if (size <= 3)
        {
            return 0;
        }
        if (cursor != size - 1)
        {
            index = size - 1;
        }
        else
        {
            index = size - 3;
        }
    }
    new old = cursor;
    cursor = index;
    new slot = old % 27;
    new cell = g_cells[slot];
    new back = g_backs[slot];
    new icon = g_icons[slot];
    new view_pos = old - yview * 3;
    if (view_pos >= 0 && view_pos < 9)
    {
        new Float:x = 92.0 * (old % 3);
        new Float:y = 92.0 * (old / 3);
        AnimMoveLine(cell, x, -y, 0.0, 200.0, 0.0, ANIM_FUNC_EXPONENT);
        AnimColor(cell, 1.0, 1.0, 1.0, 0.8, 200.0, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(back, 1.0, 1.0, 1.0, 0.8, 200.0, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(icon, 0.8, 0.8, 0.8, 1.0, 1000.0, 0.0, ANIM_FUNC_LINEAR);
    }
    slot = index % 27;
    cell = g_cells[slot];
    back = g_backs[slot];
    icon = g_icons[slot];
    view_pos = index - yview * 3;
    if (view_pos >= 0 && view_pos < 9)
    {
        new Float:x = 92.0 * (index % 3);
        new Float:y = 92.0 * (index / 3);
        new Float:z = g_has_focus ? 35.0 : 0.0;
        AnimMoveLine(cell, x, -y, z, 200.0, 0.0, ANIM_FUNC_EXPONENT);
        AnimColor(cell, 1.0, 1.0, 1.0, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(back, 1.0, 1.0, 1.0, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(icon, 1.0, 1.0, 1.0, 1.0, 1000.0, 0.0, ANIM_FUNC_LINEAR);
    }
    if (g_has_focus)
    {
        send_focus(focus, cell, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    }
    if (index < yview * 3)
    {
        if (yview > 0)
        {
            scroll_view(-1);
        }
    }
    else if (index >= yview * 3 + 9)
    {
        if (yview < g_max_yview)
        {
            scroll_view(1);
        }
    }
    return index != old;
}

/* Gives the tile of item index its icon file, if the item is among the 27 around the view. */
update_icon(index, reload)
{
    if (index < 0 || index >= size)
    {
        return;
    }
    new first = yview * 3 - 9;
    if (first < 0)
    {
        first = 0;
    }
    if (first > size - 27)
    {
        first = size - 27;
    }
    new offset = index - first;
    if (offset >= 0 && offset < 27)
    {
        new slot = index % 27;
        new icon = g_icons[slot];
        new file = g_icon_files[index];
        if (file < 0)
        {
            if (g_shown_files[slot] >= 0)
            {
                g_shown_files[slot] = -1;
                SetOptionS(icon, TAG("FILE"), "");
            }
        }
        else
        {
            if (g_shown_files[slot] != file)
            {
                g_shown_files[slot] = file;
                new path[64];
                sprintf(path, "/opt0/cache/ictmp%04d", file);
                SetOptionS(icon, TAG("FILE"), path);
            }
            new Float:brightness = index == cursor ? 1.0 : 0.8;
            offset = index - yview * 3;
            if (offset >= 0 && offset < 9)
            {
                SetColor(icon, brightness, brightness, brightness, 0.0);
                AnimColor(icon, brightness, brightness, brightness, 1.0, 1000.0, 0.0, ANIM_FUNC_LINEAR);
                if (reload)
                {
                    SetOptionI(icon, TAG("FBUF"), 2);
                }
            }
            else
            {
                SetColor(icon, brightness, brightness, brightness, 0.0);
            }
        }
    }
}

/* The order in which the tiles of a new row appear, from the column of the cursor. */
new g_column_order[3][3] = {{2, 1}, {0, 2, 1}, {0, 1, 2}};

/* Scrolls the view by one row (dir -1 up, 1 down) and tells the owner. */
scroll_view(dir)
{
    g_move_wait = 7;
    yview = yview + dir;
    g_cleanup_wait = 30;
    SendMessage(owner, TAG("SCRL"), yview, dir, 0, 0);
    new Float:x = -92.0;
    new Float:y = 92.0 + 92.0 * float(yview);
    AnimMoveHermite(g_root, x, y, 0.0, 500.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
    new load_row;
    new show_row;
    new hide_row;
    new far_row; /* unused, but part of the layout */
    if (dir < 0)
    {
        load_row = yview - 1;
        show_row = yview;
        hide_row = yview + 3;
        far_row = hide_row + 1;
    }
    else
    {
        load_row = yview + 3;
        show_row = yview + 3 - 1;
        hide_row = yview - 1;
        far_row = hide_row - 1;
    }
    for (new i; i < 3; i++)
    {
        hide_cell(hide_row * 3 + i, dir, 500.0);
    }
    for (new i; i < 3; i++)
    {
        sleep;
        new column = g_column_order[cursor % 3][i];
        show_cell(show_row * 3 + column, dir, 500.0);
    }
    for (new i; i < 3; i++)
    {
        sleep;
        sleep;
        sleep;
        sleep;
        sleep;
        update_icon(load_row * 3 + i, 0);
    }
}

/* Creates the 27 tiles, front, back and icon; the first column faces the other way. */
create_cells()
{
    g_root = CreateNull(Self());
    SetPos(g_root, -92.0, 92.0, 0.0, 0);
    for (new i; i < 27; i++)
    {
        new mirrored = i % 3 < 1;
        new cell = CreatePlane(g_root);
        new back = CreatePlane(cell);
        g_cells[i] = cell;
        g_backs[i] = back;
        if (mirrored)
        {
            SetRot(cell, 0.0, 3.1416, 0.0, 0, 0);
        }
        SetLocalScale(cell, 92.0 * 1.3, 92.0 * 1.3, 1.0);
        SetOptionI(cell, TAG("ALPH"), BLEND_ADD);
        SetTexture(cell, texture0, 0, -1);
        SetOptionF(cell, TAG("ZOFS"), 92.0 * 0.5);
        SetLocalScale(back, 92.0 * 1.3, 92.0 * 1.3, 1.0);
        SetOptionI(back, TAG("ALPH"), BLEND_ADD);
        SetTexture(back, texture1, 0, -1);
        new icon = CreateIcon(g_cells[i]);
        g_icons[i] = icon;
        new Float:icon_size = 92.0 * 0.75;
        SetLocalScale(icon, icon_size, icon_size, icon_size);
        if (!mirrored)
        {
            SetPos(icon, 0.0, 0.0, 92.0 * 0.167, 0);
            SetRot(icon, 1.5708, 0.0, 0.0, 0, 0);
        }
        else
        {
            SetPos(icon, 0.0, 0.0, -92.0 * 0.167, 0);
            SetRot(icon, 1.5708, -3.1416, 0.0, 0, 0);
        }
        static Float:icon_rot[3] = {-1.5708, 0.0, 0.0};
        static Float:icon_pos[3] = {0.0, -0.167, 0.0};
        SetOptionP(icon, TAG("ROT1"), icon_rot, 3);
        SetOptionP(icon, TAG("POS1"), icon_pos, 3);
        if (i < 9)
        {
            show_cell(i, 0, 1000.0);
        }
        else
        {
            remove_cell(i);
        }
    }
}

/* Fades in the tile of item index, sliding it in from dir rows away (0: in place); hides it if there is no item. */
show_cell(index, dir, Float:duration)
{
    new slot = (index + 27) % 27;
    new cell = g_cells[slot];
    new back = g_backs[slot];
    if (index < 0 || index >= size)
    {
        Hide(cell);
        Hide(back);
        return;
    }
    Show(cell);
    Show(back);
    if (MOVE_NONE != dir)
    {
        new Float:x = 92.0 * (index % 3);
        new Float:y = 92.0 * (index / 3);
        new Float:z = index == cursor && g_has_focus ? 35.0 : 0.0;
        if (0 == dir)
        {
            SetPos(cell, x, -y, z, 0);
        }
        else
        {
            SetPos(cell, x, -y - 92.0 * dir, z, 0);
            AnimMoveHermite(cell, x, -y, z, duration, 0.0, ANIM_FUNC_EXPONENT, 1.0);
        }
    }
    new Float:alpha = 0.8;
    SetColor(cell, 1.0, 1.0, 1.0, 0.0);
    AnimColor(cell, 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    SetColor(back, 1.0, 1.0, 1.0, 0.0);
    AnimColor(back, 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    new icon = g_icons[slot];
    SetColor(icon, 0.8, 0.8, 0.8, 0.0);
    AnimColor(icon, 0.8, 0.8, 0.8, 1.0, duration, 200.0, ANIM_FUNC_LINEAR);
}

/* Fades out the tile of item index, sliding it dir rows away. */
hide_cell(index, dir, Float:duration)
{
    new slot = (index + 27) % 27;
    AnimColor(g_cells[slot], 1.0, 1.0, 1.0, 0.0, duration, 200.0, ANIM_FUNC_LINEAR);
    AnimColor(g_backs[slot], 1.0, 1.0, 1.0, 0.0, duration, 200.0, ANIM_FUNC_LINEAR);
    AnimColor(g_icons[slot], 0.8, 0.8, 0.8, 0.0, duration, 0.0, ANIM_FUNC_LINEAR);
    if (MOVE_NONE != dir)
    {
        new Float:x = 92.0 * (index % 3);
        new Float:y = 92.0 * (index / 3);
        new cell = g_cells[slot];
        AnimMoveHermite(cell, x, -y + 92.0 * dir, 0.0, duration, 0.0, ANIM_FUNC_EXPONENT, 1.0);
    }
}

remove_cell(index)
{
    new slot = (index + 27) % 27;
    Hide(g_cells[slot]);
    Hide(g_backs[slot]);
}

/* The focus leaves the grid: the selected tile goes back down. */
leave_focus()
{
    g_has_focus = 0;
    move_cursor(cursor);
}

show_pad_help()
{
    SetSystemParam(SYS_PADHELP, padhelp);
}
