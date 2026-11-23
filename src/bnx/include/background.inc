/*
 * Messages to the background scene, the restore of the pad help and sound effects.
 */

#if !defined g_pad_help_on
new g_pad_help_on;
#endif
#if !defined g_pad_help_cross
new g_pad_help_cross[32];
#endif
#if !defined g_pad_help_triangle
new g_pad_help_triangle[32];
#endif
#if !defined g_pad_help_square
new g_pad_help_square[32];
#endif
#if !defined g_pad_help_circle
new g_pad_help_circle[32];
#endif
/* Restores the pad help saved by save_pad_help. */
stock restore_pad_help()
{
    SetSystemParam(SYS_PADHELP, g_pad_help_on);
    SetSystemParamStr(SYS_STR_PADHELP_RD, g_pad_help_cross);
    SetSystemParamStr(SYS_STR_PADHELP_RU, g_pad_help_triangle);
    SetSystemParamStr(SYS_STR_PADHELP_RL, g_pad_help_square);
    SetSystemParamStr(SYS_STR_PADHELP_RR, g_pad_help_circle);
}

/* Fades out the black cover of the background scene over fade_time ms (0: 2000 ms). */
stock fade_in_background(Float:fade_time)
{
    new scene = wndGetScene(WND_BG);
    if (scene)
    {
        SendMessage(scene, TAG("STRT"), fade_time, 0, 0, 0);
    }
}

/* Fades the two layers of the background scene to alpha0 and alpha1 over fade_time ms. */
stock fade_background(alpha0, alpha1, Float:fade_time)
{
    new scene = wndGetScene(WND_BG);
    if (scene)
    {
        SendMessage(scene, TAG("FADE"), alpha0, alpha1, fade_time, 0);
    }
}

/* Plays a sound effect at full volume. */
stock play_se(se)
{
    soundSePlay(se, 127);
}

