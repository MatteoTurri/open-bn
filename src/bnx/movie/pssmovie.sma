/*
 * /opt0/bn/script/movie/pssmovie.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * Plays the movie file of the argument "file" with the movie player, then loads the page of the argument "bref".
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

#include "movie.inc"

/* Plays the first movie of the play list. */
play_list()
{
    if (0 == movieListNum())
    {
        printf("playlist is empty.\n");
        return;
    }
    moviePlay(0);
}

new g_file[256];
new g_back_url[256];

main()
{
    GetArg("file", g_file, 256);
    GetArg("bref", g_back_url, 256);
    if (g_file[0])
    {
        movieListClear();
        movieListAdd(g_file);
    }
    play_list();
    if (g_back_url[0])
    {
        xmlLoadStart(g_back_url, "", "", XMLLOAD_MAIN);
    }
    else
    {
    }
}
