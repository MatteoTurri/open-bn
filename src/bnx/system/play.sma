/*
 * /opt0/bn/script/system/play.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The built-in play page (BUILDIN_PLAY): hands the RealPlayer URL of its "cref" argument to the player
 * application, with "nextref" to come back to.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

new g_cref[256];
new g_nextref[256];

main()
{
    SetSystemParam(SYS_ALERT_ILLEGAL_DISC, 0);
    SetSystemParam(SYS_ENABLE_NAVIMENU, 0);
    GetArg("cref", g_cref, 256);
    GetArg("nextref", g_nextref, 256);
    printf("cref=[%s] nextref=[%s] \n", g_cref, g_nextref);
    play(0, 1024); /* the addresses of g_cref and g_nextref */
}
