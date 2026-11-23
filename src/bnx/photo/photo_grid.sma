/*
 * /opt0/bn/script/photo/photo_grid.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The grid of thumbnails of the photo page: the photos of the photo list, four by four, scrolled by rows. The
 * thumbnails are decoded one at a time in the background, and kept for the rows around the ones shown.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "photodb.inc"

const COLUMNS = 4;
const ROWS = 4;        /* the rows shown */
const CELLS = 16;      /* the cells shown */
const PLANES = 32;     /* the cells made, reused by index modulo PLANES, and the thumbnails kept */
const MAX_PHOTOS = 1024;
const NO_MOVE = 32768; /* rows of show_cell and hide_cell: only fade */

/* The states of the decoding of a thumbnail. */
const DEC_NONE = 0;
const DEC_IMAGE = 1; /* decoding the photo */
const DEC_TRIM = 3;  /* cutting the thumbnail out of it */

#include "tag.inc"

#include <float>

#pragma library "photo"
native photoListClear();
native photoListAdd(const path[]);
native photoListNum();
native photoListGet(index, const buf[], maxlength);
native photoDateGetFromDB(const date_str[], maxlength, date_notation, time_notation);
native photoSJIStoUTF8(const dest[], const src[]);
native photoUTF8toSJIS(const dest[], const src[]);

/* Same as send_focus, with a Float z_offset. */
send_focus_float_z(obj, target, Float:z_offset, Float:sharp, Float:extra_w, Float:extra_h, Float:move_time, smoothing,
        blinking)
{
    if (!obj)
    {
        return;
    }
    SendMessage(obj, TAG("FOC0"), target, TAG(""), z_offset, sharp);
    SendMessage(obj, TAG("FOC1"), extra_w, extra_h, move_time, smoothing);
    SendMessage(obj, TAG("FOC2"), blinking, 0, 0, 0);
}

public owner;    /* gets "PUSH" (index, button), "SCRL" (row, rows moved), "FOCS" (direction) as the cursor leaves */
public focus; /* the focus object */
public padhelp;  /* PADHELP_* of the pad help; circle and triangle are taken away from an empty grid */
public padexit;  /* the PAD_* directions that take the cursor out of the grid */
public size;     /* the number of photos */
public cursor;   /* the photo under the cursor */
public yview;    /* the top row shown */
public idle;     /* 1 once no direction has been pressed for a while */
public decstate; /* the thumbnail under the cursor, DECSTATE_* */
public decwait;  /* no thumbnail is decoded while it is above 0 */
new Float:g_alpha;
new g_focused;
new g_pushed;
new g_key_wait;
new g_last_yview;
new g_grid;
new g_frames[PLANES];
new g_thumbs[PLANES];
new g_idle_wait = 60;
new g_hide_wait;
new g_dec_state;
new g_dec_index;
new g_textures[MAX_PHOTOS];
new Float:g_aspects[MAX_PHOTOS]; /* height / width of each photo; 1.0 before it is decoded, -1.0 if broken */
new g_kept[PLANES]; /* the photos whose thumbnail texture is kept */
new g_kept_count;

main()
{
    init_grid();
    for (; ; )
    {
        update_grid();
        sleep;
    }
}

/* Clamps the cursor, scrolls it into sight and creates the cells. */
init_grid()
{
    if (size <= 0)
    {
        padhelp = padhelp & ~(PADHELP_CIRCLE | PADHELP_TRIANGLE);
    }
    g_last_yview = (size + COLUMNS - 1) / COLUMNS - ROWS;
    if (g_last_yview < 0)
    {
        g_last_yview = 0;
    }
    cursor = clamp(cursor, 0, size);
    new row = cursor / COLUMNS;
    if (row < yview)
    {
        yview = row;
    }
    if (row >= yview + ROWS)
    {
        yview = row - ROWS + 1;
    }
    idle = 0;
    g_dec_state = DEC_NONE;
    g_dec_index = -1;
    for (new i; i < MAX_PHOTOS; i++)
    {
        g_textures[i] = 0;
        g_aspects[i] = 1.0;
    }
    g_kept_count = 0;
    create_cells();
    scroll_to(yview, 0.0);
    move_cursor(cursor);
}

/* Handles "FOCS" (how), "FADE" (alpha, time), "PUSH" (the owner is done) and "CURS" (index), and decodes a step. */
update_grid()
{
    new msg;
    new a;
    new Float:b;
    while (RecvMessage(msg, a, b))
    {
        if (msg == TAG("FOCS"))
        {
            on_focus(a);
        }
        if (msg == TAG("FADE"))
        {
            fade_grid(a, b);
        }
        if (msg == TAG("PUSH"))
        {
            enable_keys();
        }
        if (msg == TAG("CURS"))
        {
            set_cursor(a);
        }
    }
    if (g_focused && !g_pushed)
    {
        read_pad();
    }
    if (g_dec_index < 0 && DEC_NONE == g_dec_state)
    {
        new index = find_thumb_to_load();
        if (index >= 0)
        {
            new path[256];
            photoListGet(index, path, 256);
            if (strlen(path) > 0)
            {
                imgDecStart(path, 64, 64, 0, 100, 1);
                g_dec_index = index;
                g_dec_state = DEC_IMAGE;
            }
            else
            {
                /* NOTE: bug: g_dec_index is still -1 here, not index: the empty entry is never marked broken */
                g_aspects[g_dec_index] = -1.0;
                g_textures[g_dec_index] = 0;
                g_dec_index = -1;
                g_dec_state = DEC_NONE;
            }
        }
    }
    else if (g_dec_state == DEC_IMAGE)
    {
        new ret = imgDecSync();
        if (ret == IMG_DONE)
        {
            new Float:aspect = 1.0 * imgOrgHeight() / imgOrgWidth();
            new trim_w;
            new trim_h;
            new trim_x;
            new trim_y;
            if (aspect < 1.0)
            {
                trim_w = floatround(64 * aspect, 0);
                trim_h = 64;
                trim_x = (64 - trim_w) / 2;
                trim_y = 0;
            }
            else
            {
                trim_w = 64;
                trim_h = floatround(64 / aspect, 0);
                trim_x = 0;
                trim_y = (64 - trim_h) / 2;
            }
            g_aspects[g_dec_index] = aspect;
            imgTrimStart(trim_x, trim_y, trim_w, trim_h);
            g_dec_state = DEC_TRIM;
        }
        else if (ret == IMG_FAILED)
        {
            g_aspects[g_dec_index] = -1.0;
            g_textures[g_dec_index] = 0;
            g_dec_index = -1;
            g_dec_state = DEC_NONE;
        }
    }
    else if (g_dec_state == DEC_TRIM)
    {
        new ret = imgTrimSync();
        if (ret == IMG_DONE)
        {
            new tex = imgTexID();
            imgKeepTex(tex);
            g_textures[g_dec_index] = tex;
            update_thumb(g_dec_index);
            while (g_kept_count >= PLANES)
            {
                release_far_texture();
            }
            g_kept[g_kept_count] = g_dec_index;
            g_kept_count++;
            g_dec_index = -1;
            g_dec_state = DEC_NONE;
        }
    }
    if (0 != g_textures[cursor] && g_aspects[cursor] > 0.0)
    {
        decstate = DECSTATE_SHOWN;
    }
    else if (g_aspects[cursor] < 0.0)
    {
        decstate = DECSTATE_BROKEN;
    }
    else if (0 == g_textures[cursor])
    {
        decstate = DECSTATE_PENDING;
    }
    if (g_hide_wait > 0)
    {
        g_hide_wait--;
        if (0 == g_hide_wait)
        {
            new index = yview * COLUMNS + CELLS;
            for (new i; i < CELLS; i++)
            {
                remove_cell(index++);
            }
        }
    }
    if (g_idle_wait > 0)
    {
        g_idle_wait--;
        idle = 0;
    }
    else
    {
        idle = 1;
    }
}

/* Returns the photo whose thumbnail to decode next: the one under the cursor, then the ones shown; -1 for none. */
find_thumb_to_load()
{
    if (decwait > 0)
    {
        return -1;
    }
    if (cursor >= 0 && 0 == g_textures[cursor] && g_aspects[cursor] > 0.0)
    {
        return cursor;
    }
    new first = yview * COLUMNS;
    new last = first + CELLS;
    if (last > size)
    {
        last = size;
    }
    for (new i = first; i < last; i++)
    {
        if (0 == g_textures[i] && g_aspects[i] > 0.0)
        {
            return i;
        }
    }
    return -1;
}

/* Releases the kept thumbnail texture farthest from the rows shown. */
release_far_texture()
{
    new farthest;
    new max_dist;
    for (new i; i < g_kept_count; i++)
    {
        new row = g_kept[i] / COLUMNS;
        new dist = yview - row;
        if (dist > max_dist)
        {
            farthest = i;
            max_dist = dist;
        }
        dist = row - (yview + ROWS - 1);
        if (dist > max_dist)
        {
            farthest = i;
            max_dist = dist;
        }
    }
    new index = g_kept[farthest];
    new tex = g_textures[index];
    imgReleaseTex(tex);
    g_textures[index] = 0;
    g_kept_count--;
    g_kept[farthest] = g_kept[g_kept_count];
}

/* Takes the pad again once the owner has handled a press. */
enable_keys()
{
    g_pushed = 0;
}

/* Moves the cursor to index, scrolling it into sight at once. */
set_cursor(index)
{
    index = clamp(index, 0, size);
    new row = index / COLUMNS;
    if (row < yview)
    {
        scroll_to(row, 0.0);
    }
    else if (row >= yview + ROWS)
    {
        scroll_to(row - ROWS + 1, 0.0);
    }
    move_cursor(index);
}

/* Fades the cells shown to alpha. */
fade_grid(Float:alpha, Float:duration)
{
    g_alpha = alpha;
    new visible = alpha > 0.0;
    for (new i; i < CELLS; i++)
    {
        new index = yview * COLUMNS + i;
        if (visible)
        {
            show_cell(index, NO_MOVE, duration);
        }
        else
        {
            hide_cell(index, NO_MOVE, duration);
        }
    }
}

/*
 * Handles "FOCS": takes the focus coming from how (from above the first photo, from below the last), or gives it up on
 * "FOUT".
 */
on_focus(how)
{
    if (how == TAG("LEFT"))
    {
        sndSePlay(SE_CURSOR);
    }
    else if (how == TAG("RIGT"))
    {
        sndSePlay(SE_CURSOR);
    }
    else if (how == TAG("UP"))
    {
        cursor = 0;
        sndSePlay(SE_CURSOR);
    }
    else if (how == TAG("DOWN"))
    {
        cursor = size - 1;
        sndSePlay(SE_CURSOR);
    }
    else if (how == TAG("FRST"))
    {
        move_cursor(cursor);
    }
    else if (how == TAG("FOUT"))
    {
        leave_focus();
        return;
    }
    if (size > 0)
    {
        send_focus_float_z(focus, g_frames[cursor % PLANES], -10.0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    }
    on_focus_moved();
    if (!g_focused)
    {
        g_focused = 1;
        move_cursor(cursor);
    }
}

/*
 * Moves the cursor with the directions, or out of the grid where padexit lets it; sends the buttons to the owner as
 * "PUSH".
 */
read_pad()
{
    new held;
    new pressed;
    PadState(held, pressed, _, _, -1);
    if (g_key_wait > 0)
    {
        g_key_wait--;
    }
    if (0 == held)
    {
        return;
    }
    new dir = held & (PAD_UP | PAD_RIGHT | PAD_DOWN | PAD_LEFT);
    if (g_key_wait > 0)
    {
        dir = 0;
    }
    if (0 != dir)
    {
        g_idle_wait = 30;
        if (dir & PAD_UP)
        {
            g_key_wait = 12;
            if (move_cursor(cursor - COLUMNS))
            {
                sndSePlay(SE_CURSOR);
            }
            else if (padexit & PAD_UP)
            {
                SendMessage(owner, TAG("FOCS"), TAG("UP"), 0, 0, 0);
                leave_focus();
                return;
            }
        }
        else if (dir & PAD_DOWN && cursor + COLUMNS < MAX_PHOTOS)
        {
            g_key_wait = 12;
            if (move_cursor(cursor + COLUMNS))
            {
                sndSePlay(SE_CURSOR);
            }
            else if (padexit & PAD_DOWN)
            {
                SendMessage(owner, TAG("FOCS"), TAG("DOWN"), 0, 0, 0);
                leave_focus();
                return;
            }
        }
        else if (dir & PAD_LEFT)
        {
            g_key_wait = 12;
            if (0 != cursor % COLUMNS)
            {
                if (move_cursor(cursor - 1))
                {
                    sndSePlay(SE_CURSOR);
                }
            }
            else if (padexit & PAD_LEFT)
            {
                SendMessage(owner, TAG("FOCS"), TAG("LEFT"), 0, 0, 0);
                leave_focus();
                return;
            }
        }
        else if (dir & PAD_RIGHT)
        {
            g_key_wait = 12;
            if (COLUMNS - 1 != cursor % COLUMNS && cursor != size - 1)
            {
                if (move_cursor(cursor + 1))
                {
                    sndSePlay(SE_CURSOR);
                }
            }
            else if (padexit & PAD_RIGHT)
            {
                SendMessage(owner, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
                leave_focus();
                return;
            }
        }
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
        else if (pressed & PAD_CIRCLE)
        {
            if (size > 0)
            {
                SendMessage(owner, TAG("PUSH"), cursor, TAG("RRGT"), 0, 0);
                g_pushed = 1;
            }
        }
        else if (pressed & PAD_CROSS)
        {
            SendMessage(owner, TAG("PUSH"), cursor, TAG("RDWN"), 0, 0);
            g_pushed = 1;
        }
        else if (pressed & PAD_START)
        {
            SendMessage(owner, TAG("PUSH"), cursor, TAG("STRT"), 0, 0);
            g_pushed = 1;
        }
    }
}

/* Moves the cursor to index (past the end: the last photo), scrolling a row if needed; returns whether it moved. */
move_cursor(index)
{
    if (index < 0)
    {
        return 0;
    }
    if (index >= size)
    {
        if (index - (size - 1) / COLUMNS * COLUMNS >= COLUMNS)
        {
            return 0;
        }
        if (size <= COLUMNS)
        {
            return 0;
        }
        if (cursor != size - 1)
        {
            index = size - 1;
        }
        else
        {
            index = size - COLUMNS;
        }
    }
    new old = cursor;
    cursor = index;
    new slot = old % PLANES;
    new frame = g_frames[slot];
    new thumb = g_thumbs[slot];
    new pos = old - yview * COLUMNS;
    if (pos >= 0 && pos < CELLS)
    {
        new Float:x = 70.0 * (old % COLUMNS);
        new Float:y = 70.0 * (old / COLUMNS);
        AnimMoveLine(frame, x, -y, 0.0, 200.0, 0.0, ANIM_FUNC_EXPONENT);
        AnimColor(frame, 1.0, 1.0, 1.0, 0.8 * g_alpha, 200.0, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(thumb, 0.8, 0.8, 0.8, g_alpha, 1000.0, 0.0, ANIM_FUNC_LINEAR);
    }
    slot = index % PLANES;
    frame = g_frames[slot];
    thumb = g_thumbs[slot];
    pos = index - yview * COLUMNS;
    if (pos >= 0 && pos < CELLS)
    {
        new Float:x = 70.0 * (index % COLUMNS);
        new Float:y = 70.0 * (index / COLUMNS);
        new Float:z = g_focused ? 50.0 : 0.0;
        AnimMoveLine(frame, x, -y, z, 200.0, 0.0, ANIM_FUNC_EXPONENT);
        AnimColor(frame, 1.0, 1.0, 1.0, 1.0 * g_alpha, 200.0, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(thumb, 1.0, 1.0, 1.0, g_alpha, 1000.0, 0.0, ANIM_FUNC_LINEAR);
    }
    if (g_focused)
    {
        send_focus_float_z(focus, frame, -10.0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    }
    if (index < yview * COLUMNS)
    {
        if (yview > 0)
        {
            scroll_to(yview - 1, 500.0);
        }
    }
    else if (index >= yview * COLUMNS + CELLS)
    {
        if (yview < g_last_yview)
        {
            scroll_to(yview + 1, 500.0);
        }
    }
    return index != old;
}

/* Makes row the top row shown, sliding by one row over duration if it is next; returns whether it changed. */
scroll_to(row, Float:duration)
{
    if (row < 0)
    {
        return 0;
    }
    if (row > g_last_yview)
    {
        return 0;
    }
    g_hide_wait = 60;
    g_key_wait = 16;
    new old_row = yview;
    SendMessage(owner, TAG("SCRL"), row, row - old_row, 0, 0);
    yview = row;
    new Float:x = -105.6;
    new Float:y = 105.6 + 70.0 * float(yview);
    if (duration > 0.0)
    {
        AnimMoveHermite(g_grid, x, y, 0.0, 500.0, 0.0, ANIM_FUNC_EXPONENT, 1.0);
    }
    else
    {
        SetPos(g_grid, x, y, 0.0, 0);
    }
    new delta = row - old_row;
    if (duration > 0.0 && (delta == -1 || delta == 1))
    {
        new show_from;
        new hide_from;
        new update_from;
        if (delta < 0)
        {
            show_from = row * COLUMNS;
            hide_from = show_from + CELLS;
            update_from = show_from;
        }
        else
        {
            hide_from = old_row * COLUMNS;
            show_from = hide_from + CELLS;
            update_from = show_from;
        }
        for (new i; i < COLUMNS; i++)
        {
            hide_cell(hide_from++, delta, duration);
            show_cell(show_from++, delta, duration);
            update_thumb(update_from++);
        }
    }
    else
    {
        new index = row * COLUMNS;
        for (new i; i < CELLS; i++)
        {
            show_cell(index, 0, 1000.0);
            update_thumb(index);
            index++;
        }
        for (new i; i < CELLS; i++)
        {
            remove_cell(index);
            index++;
        }
    }
    return row != old_row;
}

/* Creates the PLANES cells, each a grid frame with a thumbnail plane, hidden. */
create_cells()
{
    g_grid = CreateNull(Self());
    SetPos(g_grid, -105.6, 105.6, 0.0, 0);
    for (new i; i < PLANES; i++)
    {
        new frame = CreatePlane(g_grid);
        g_frames[i] = frame;
        SetColor(frame, 1.0, 1.0, 1.0, 0.0);
        SetLocalScale(frame, 92.0, 92.0, 1.0);
        SetOptionI(frame, TAG("ALPH"), BLEND_ADD);
        SetTexture(frame, imgGetResource(RES_GRID), 0, -1);
        SetOptionF(frame, TAG("ZOFS"), 10.0);
        Hide(frame);
        HideChildren(frame);
        new thumb = CreatePlane(frame);
        g_thumbs[i] = thumb;
        SetColor(thumb, 1.0, 1.0, 1.0, 0.0);
        SetLocalScale(thumb, 64.0, 64.0, 1.0);
        SetOptionI(thumb, TAG("FDEG"), 1);
        SetOptionF(thumb, TAG("ZOFS"), -10.0);
        Hide(thumb);
    }
}

/* Shows the cell of photo index, sliding it in from rows rows away. */
show_cell(index, rows, Float:duration)
{
    new slot = (index + PLANES) % PLANES;
    new frame = g_frames[slot];
    if (index < 0 || index >= size)
    {
        Hide(frame);
        HideChildren(frame);
        return;
    }
    Show(frame);
    ShowChildren(frame);
    if (NO_MOVE != rows)
    {
        new Float:x = 70.0 * (index % COLUMNS);
        new Float:y = 70.0 * (index / COLUMNS);
        new Float:z = index == cursor && g_focused ? 35.0 : 0.0;
        if (0 == rows)
        {
            SetPos(frame, x, -y, z, 0);
        }
        else
        {
            SetPos(frame, x, -y - 70.0 * rows, z, 0);
            AnimMoveHermite(frame, x, -y, z, duration, 0.0, ANIM_FUNC_EXPONENT, 1.0);
        }
    }
    SetColor(frame, 1.0, 1.0, 1.0, 0.0);
    AnimColor(frame, 1.0, 1.0, 1.0, 0.8 * g_alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    new thumb = g_thumbs[slot];
    SetColor(thumb, 0.8, 0.8, 0.8, 0.0);
    AnimColor(thumb, 0.8, 0.8, 0.8, g_alpha, duration, 200.0, ANIM_FUNC_LINEAR);
}

/* Fades out the cell of photo index, sliding it rows rows away. */
hide_cell(index, rows, Float:duration)
{
    new slot = (index + PLANES) % PLANES;
    AnimColor(g_frames[slot], 1.0, 1.0, 1.0, 0.0, duration, 200.0, ANIM_FUNC_LINEAR);
    AnimColor(g_thumbs[slot], 0.8, 0.8, 0.8, 0.0, duration, 0.0, ANIM_FUNC_LINEAR);
    if (NO_MOVE != rows)
    {
        new Float:x = 70.0 * (index % COLUMNS);
        new Float:y = 70.0 * (index / COLUMNS);
        new frame = g_frames[slot];
        AnimMoveHermite(frame, x, -y + 70.0 * rows, 0.0, duration, 0.0, ANIM_FUNC_EXPONENT, 1.0);
    }
}

/* Hides the cell of photo index and its thumbnail. */
remove_cell(index)
{
    new slot = (index + PLANES) % PLANES;
    new frame = g_frames[slot];
    Hide(frame);
    HideChildren(frame);
    clear_thumb(index);
}

/* Puts the kept texture of photo index on its cell, if the cell is around the rows shown. */
update_thumb(index)
{
    if (index < 0 || index >= size)
    {
        return;
    }
    new pos = index - yview * COLUMNS;
    if (pos >= -8 && pos < 24)
    {
        new thumb = g_thumbs[index % PLANES];
        new tex = g_textures[index];
        SetTexture(thumb, tex, 0, -1);
        if (0 == tex)
        {
            Hide(thumb);
        }
        else
        {
            Show(thumb);
            new Float:level = index == cursor ? 1.0 : 0.8;
            SetColor(thumb, level, level, level, 0.0);
            if (pos >= 0 && pos < CELLS)
            {
                AnimColor(thumb, level, level, level, g_alpha, 1000.0, 0.0, ANIM_FUNC_LINEAR);
            }
        }
    }
}

/* Hides the thumbnail of photo index and takes its texture away. */
clear_thumb(index)
{
    if (index < 0 || index >= size)
    {
        return;
    }
    new slot = (index + PLANES) % PLANES;
    new thumb = g_thumbs[slot];
    Hide(thumb);
    SetTexture(thumb, 0, 0, -1);
}

/* The focus leaves the grid: the selected tile goes back down. */
leave_focus()
{
    g_focused = 0;
    move_cursor(cursor);
}

/* Called by on_focus once the focus has moved; does nothing. */
on_focus_moved()
{
}

#include "string.inc"
#include "sysconf.inc"
#include "dom.inc"
