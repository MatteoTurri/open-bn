/*
 * /opt0/bn/script/utility/menu/camera.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The camera of the utility menu: moves in front of (or into) the cube it is sent to and back again.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

#include "tag.inc"

new g_depth;
/* The positions to go back to, one per move not yet undone (g_depth of them). */
new Float:g_back_x[10];
new Float:g_back_y[10];
new Float:g_back_z[10];

main()
{
    SetRot(Self(), 0.0, 3.141592, 0.0, 0, 0);
    for (; ; )
    {
        new msg;
        new a;
        new b;
        /* "INTO" (target, enter): moves to target; "AWAY": back to where it was; "QUIT": ends. */
        if (RecvMessage(msg, a, b))
        {
            if (msg == TAG("INTO"))
            {
                move_to(a, b);
            }
            else if (msg == TAG("AWAY"))
            {
                move_back();
            }
            else if (msg == TAG("QUIT"))
            {
                break;
            }
        }
        sleep;
    }
}

/* Moves into target when enter is set, else in front of it, remembering where to come back. */
move_to(target, enter)
{
    GetPos(Self(), g_back_x[g_depth], g_back_y[g_depth], g_back_z[g_depth], 0);
    new Float:x;
    new Float:y;
    new Float:z;
    GetPos(target, x, y, z, Scene(0));
    if (enter)
    {
        AnimMoveLine(Self(), x, y, 16.0 + z, 1.2 * float(1200), 0.0, ANIM_FUNC_FASTER);
    }
    else
    {
        AnimMoveLine(Self(), 96.0 + x, y - 4.0, 260.0 + z, float(1200), 0.0, ANIM_FUNC_EXPONENT);
    }
    g_depth++;
}

move_back()
{
    g_depth--;
    AnimMoveLine(Self(), g_back_x[g_depth], g_back_y[g_depth], g_back_z[g_depth], float(1200), 0.0, ANIM_FUNC_EXPONENT);
}
