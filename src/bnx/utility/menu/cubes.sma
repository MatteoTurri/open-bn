/*
 * /opt0/bn/script/utility/menu/cubes.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The crystal cubes of the utility menu: they drift and spin around a point of the screen, the selected one lights
 * up, and the camera flies into it when it is opened while the others fade away.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

forward Float:sin_deg(Float:degrees);

new Float:g_pi = 3.141592;
new Float:g_screen_width;
new Float:g_screen_height;
new Float:g_screen_depth;
public cube_num = 8; /* at most 8 */
public Float:center_x = 0.5; /* the center of the cubes on the screen, as a fraction of its width */
public Float:center_y = 0.5; /* and of its height */
public texid_0; /* the textures of the faces of a cube; texid_0 is on two faces */
public texid_1; /* on one face */
public texid_2; /* on one face */
public texid_3; /* on one face */
new g_kept_cube = -1; /* the cube the fade leaves as it is, -1 for none */
/* The fade of the cubes goes from g_fade_from to g_fade_to with the timer animation of this object. */
new Float:g_fade_from;
new Float:g_fade_to;

/* Same as TAG of tag.inc. */
TAG(fourcc[])
{
    return swapchars(fourcc[0]);
}

#include <float>

/* Same as get_screen_size of text.inc. */
get_screen_size(&Float:screen_w, &Float:screen_h)
{
    GetScreen(Camera(0), g_screen_width, g_screen_height, g_screen_depth);
    screen_w = g_screen_width;
    screen_h = g_screen_height;
}

/*
 * Gets the position of the display point (screen_x, screen_y) in the space of rel (by default the parent, else this
 * object).
 */
map_screen_point(&Float:x, &Float:y, &Float:z, Float:screen_x, Float:screen_y, rel)
{
    assert 0 - screen_x;
    if (!rel)
    {
        rel = Parent(0);
    }
    if (!rel)
    {
        rel = Self();
    }
    GetOffPos(Disp2D(0), x, y, z, screen_x, screen_y, 0.0, rel);
}

main()
{
    new cubes[8];
    new selected = -1;
    new Float:move_time; /* in seconds, stopped while the camera is inside a cube */
    new Float:spin_time; /* in seconds */
    new inside; /* the camera has gone into the selected cube */
    new quitting;
    {
        new Float:screen_w;
        new Float:screen_h;
        get_screen_size(screen_w, screen_h);
        new Float:x;
        new Float:y;
        new Float:z;
        map_screen_point(x, y, z, screen_w * center_x, screen_h * center_y, 0);
        SetPos(Self(), x, y, z, Scene(0));
        SetRot(Self(), 0.0, 0.0, 0.0, Camera(0), 0);
    }
    for (new i; i < cube_num; i++)
    {
        cubes[i] = CreateCrystal(Self());
        SetLocalScale(cubes[i], 40.0, 40.0, 40.0);
        SetTexture(cubes[i], texid_0, 0, -1);
        SetTexture(cubes[i], texid_1, 1, -1);
        SetTexture(cubes[i], texid_0, 2, -1);
        SetTexture(cubes[i], texid_2, 3, -1);
        SetTexture(cubes[i], texid_3, 4, -1);
        SetColor(cubes[i], 1.0, 1.0, 1.0, 0.0);
        SetScale(cubes[i], 1.0, 1.0, 1.0);
        SetOptionF(cubes[i], TAG("FADE"), 0.0);
        SetOptionF(cubes[i], TAG("RFF0"), 0.2);
        SetOptionF(cubes[i], TAG("RFF1"), 0.4);
        SetOptionF(cubes[i], TAG("NEAR"), 16.0);
        new Float:x;
        new Float:y;
        new Float:z;
        new Float:rx;
        new Float:ry;
        new Float:rz;
        get_cube_pos(i, move_time, x, y, z);
        get_cube_rot(i, spin_time, rx, ry, rz);
        SetPos(cubes[i], x, y, z, 0);
        SetRot(cubes[i], rx, ry, rz, 0, 0);
    }
    start_fade(-1, 1.0, 500, 0);
    for (; ; )
    {
        sleep;
        new Float:fade;
        if (ANIM_DONE != AnimGetState(Self(), ANIM_TIMER))
        {
            fade = AnimGetCurrTime(Self(), 4);
            fade = g_fade_from + (g_fade_to - g_fade_from) * fade;
        }
        else
        {
            g_fade_from = g_fade_to;
            fade = g_fade_to;
        }
        for (new i; i < cube_num; i++)
        {
            new Float:x;
            new Float:y;
            new Float:z;
            new Float:rx;
            new Float:ry;
            new Float:rz;
            get_cube_pos(i, move_time, x, y, z);
            get_cube_rot(i, spin_time, rx, ry, rz);
            SetPos(cubes[i], x, y, z, 0);
            SetRot(cubes[i], rx, ry, rz, 0, 0);
            if (g_kept_cube != i)
            {
                if (0.0 == fade)
                {
                    Hide(cubes[i]);
                }
                else
                {
                    SetOptionF(cubes[i], TAG("FADE"), fade);
                    Show(cubes[i]);
                }
            }
        }
        new msg;
        new a;
        new Float:b;
        /*
         * "SLCT" (cube, alpha): lights up that cube instead of the selected one; "INTO" (enter) sends the camera to
         * the selected cube, "AWAY" back; "QUIT" fades every cube out and ends.
         */
        if (RecvMessage(msg, a, b))
        {
            if (msg == TAG("SLCT"))
            {
                if (selected >= 0 && selected < cube_num)
                {
                    AnimColor(cubes[selected], 1.0, 1.0, 1.0, 0.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
                }
                if (a >= 0 && a < cube_num)
                {
                    AnimColor(cubes[a], 1.0, 1.0, 1.0, b, 150.0, 0.0, ANIM_FUNC_LINEAR);
                }
                selected = a;
            }
            else if (msg == TAG("INTO"))
            {
                sndSePlay(SE_ZOOM_IN);
                SendMessage(Camera(0), TAG("INTO"), cubes[selected], a, 0, 0);
                start_fade(selected, 0.0, 1000, 0);
                inside = 1;
            }
            else if (msg == TAG("AWAY"))
            {
                sndSePlay(SE_ZOOM_OUT);
                SendMessage(Camera(0), TAG("AWAY"), 0, 0, 0, 0);
                start_fade(selected, 1.0, 1000, 1000);
                inside = 0;
            }
            else if (msg == TAG("QUIT"))
            {
                start_fade(-1, 0.0, 500, 0);
                inside = 0;
                quitting = 1;
            }
        }
        if (!inside)
        {
            move_time = move_time + 1.0 / FrameRateF();
        }
        spin_time = spin_time + 1.0 / FrameRateF();
        if (quitting && g_fade_from == g_fade_to)
        {
            break;
        }
    }
    Destroy(Self());
}

/* Starts fading every cube but kept_cube from the current fade to fade. */
start_fade(kept_cube, Float:fade, duration, delay)
{
    if (ANIM_DONE != AnimGetState(Self(), ANIM_TIMER))
    {
        new Float:t = AnimGetCurrTime(Self(), 4);
        g_fade_from = g_fade_from + (g_fade_to - g_fade_from) * t;
    }
    AnimTimerManager(Self(), float(duration), float(delay), ANIM_FUNC_LINEAR);
    g_kept_cube = kept_cube;
    g_fade_to = fade;
}

/* Gets the position of a cube after elapsed seconds of drifting. */
get_cube_pos(cube, Float:elapsed, &Float:x, &Float:y, &Float:z)
{
    elapsed = 30.0 * elapsed;
    switch (cube)
    {
        case 0:
        {
            x = 5.5;
            y = 3.5;
            z = 3.0 + (1.5 * sin_deg(30.0 + 0.1 * elapsed) - 8.4);
        }
        case 2:
        {
            x = 1.0 + 1.0 * sin_deg(-0.1 * elapsed);
            y = 3.0;
            z = 3.0 + (1.0 * sin_deg(90.0 + -0.1 * elapsed) - 1.0);
        }
        case 3:
        {
            x = 9.0 + 2.0 * sin_deg(0.1 * elapsed - 130.0);
            y = 1.0;
            z = 3.0 + (7.0 + (8.0 + 2.0 * sin_deg(30.0 + -0.08 * elapsed)));
        }
        case 1:
        {
            x = 5.5 + 1.0 * sin_deg(135.0 + 0.2 * elapsed);
            y = 5.0;
            z = 3.0 + (3.5 + 1.0 * sin_deg(60.0 + 0.2 * elapsed));
        }
        case 4:
        {
            x = 2.0 * sin_deg(0.1 * elapsed) - 1.0;
            y = -0.5 - 5.0;
            z = 3.0 + (10.0 + 3.0 * sin_deg(90 + 0.1 * elapsed));
        }
        case 5:
        {
            x = 2.0 + 9.0 + 2.0 * sin_deg(180 + 0.1 * elapsed);
            y = -1.0 - 1.0 * sin_deg(90 + (-0.1 * elapsed - 120.0)) - 6.0;
            z = 3.0 + (3.0 + 3.0 * sin_deg(180 + (90.0 + 0.1 * elapsed)));
        }
        case 6:
        {
            x = 4.0 + 1.0 * sin_deg(0.1 * elapsed);
            y = 0.0;
            z = 8.0 + 14.0 * sin_deg(0.08 * elapsed - 50.0);
        }
        case 7:
        {
            x = 0.0 + sin_deg(0.1 * elapsed - 30.0);
            y = 0.0;
            z = 8.0 + 10.0 * sin_deg(0.09 * elapsed);
        }
    }
    x = -25.0 * x;
    y = 25.0 * y;
    z = 25.0 * z;
}

/* Gets the rotation of a cube, in radians, after elapsed seconds of spinning. */
get_cube_rot(cube, Float:elapsed, &Float:rx, &Float:ry, &Float:rz)
{
    elapsed = 30.0 * elapsed;
    switch (cube)
    {
        case 0:
        {
            rx = 0.05 * elapsed;
            ry = -0.15 * elapsed;
            rz = 0.1 * elapsed;
        }
        case 2:
        {
            rx = 45.0 + -0.1 * elapsed;
            ry = 0.15 * elapsed;
            rz = 0.1 * elapsed;
        }
        case 3:
        {
            rx = 0.1 * elapsed;
            ry = 45 + 0.15 * elapsed;
            rz = -0.05 * elapsed;
        }
        case 1:
        {
            rx = 0.05 * elapsed;
            ry = -0.1 * elapsed;
            rz = 45.0 + 0.1 * elapsed;
        }
        case 4:
        {
            rx = 30.0 + 0.1 * elapsed;
            ry = -0.05 * elapsed;
            rz = 0.1 * elapsed;
        }
        case 5:
        {
            rx = 0.1 * elapsed;
            ry = 0.142 * elapsed;
            rz = 0.05 * elapsed;
        }
        case 6:
        {
            rx = 0.12 * elapsed;
            ry = 0.15 * elapsed;
            rz = 0.1 * elapsed;
        }
        case 7:
        {
            rx = 0.1 * elapsed;
            ry = 45.0 + 0.1 * elapsed;
            rz = 45.0 + 0.1 * elapsed;
        }
    }
    rx = rx * (g_pi * 2) / 360.0;
    ry = ry * (g_pi * 2) / 360.0;
    rz = rz * (g_pi * 2) / 360.0;
}

Float:sin_deg(Float:degrees)
{
    return sin(2 * degrees * g_pi / 360.0);
}
