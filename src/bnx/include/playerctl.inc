/*
 * Commands to the music player.
 */

stock reset_player()
{
    playerExecCommand(PLAYER_RESET);
    return 1;
}

/* Stops the player and waits until it reports the stopped state. */
stock stop_player()
{
    printf("---------------- stopping \n");
    playerExecCommand(PLAYER_STOP, PLAYER_UNIT_CD, 0, 0);
    new state = PLAYER_STATE_STOPPED;
    new info;
    do
    {
        playerExecCommand(PLAYER_CD_GET_STATE, state, info, 0);
        sleep;
    }
    while (PLAYER_STATE_STOPPED != state);
    return;
}

