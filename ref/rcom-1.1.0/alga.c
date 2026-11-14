/*
 * The algae choice of BSAFE: AlgaChoiceChoose picks the implementation of an algorithm, and
 * ConvertAlgaeError translates its return codes into BE_* codes.
 */
#include "ah.h"

/* Walks the list of candidates: for each one that matches in (tag, mode) it fetches the key info,
 * if any, and tries slot->instantiate. On the first success it installs the methods of the entry in
 * the slot and returns 0. BE_ALLOC, BE_HARDWARE and BE_CANCEL are fatal; any other error is kept and the walk goes on.
 * BE_METHOD_NOT_IN_CHOOSER: no candidate was tried. */
int AlgaChoiceChoose(void *this_, void *methodsSlot, int mode, void *key, int initArg,
                     AlgaEntry **chooser, void *surrender)
{
    AlgaChoice *slot = methodsSlot;
    int status = BE_METHOD_NOT_IN_CHOOSER;

    for (; *chooser != 0; chooser++) {
        void *info;
        int rc;

        if ((*chooser)->tag == slot->tag && (*chooser)->mode == mode) {
            if ((*chooser)->infoType != 0) {
                rc = B_KeyGetInfo(key, &info, (*chooser)->infoType);
                if (rc != 0) {
                    if (rc == BE_ALLOC || rc == BE_HARDWARE || rc == BE_CANCEL)
                        return rc;
                    status = rc;
                    continue;
                }
            } else {
                info = 0;
            }
            rc = slot->instantiate(slot, info, initArg, *chooser,
                                   ((void **)this_)[1], surrender);
            if (rc != 0) {
                if (rc == BE_ALLOC || rc == BE_HARDWARE || rc == BE_CANCEL)
                    return rc;
                status = rc;
                continue;
            }
            slot->methods = (*chooser)->methods;
            return 0;
        }
    }
    return status;
}

/* Codes 8 and 14 fall into the default. */
int ConvertAlgaeError(int status)
{
    switch (status) {
    case 0:
        return 0;
    case 1:
        return BE_CANCEL;
    case 2:
        return BE_DATA;
    case 3:
        return BE_EXPONENT_EVEN;
    case 17:
        return BE_EXPONENT_ONE;
    case 4:
        return BE_EXPONENT_LEN;
    case 18:
        return BE_HARDWARE;
    case 5:
        return BE_INPUT_DATA;
    case 6:
        return BE_INPUT_LEN;
    case 13:
        return BE_KEY_INFO;
    case 12:
        return BE_KEY_LEN;
    case 7:
        return BE_MODULUS_LEN;
    case 11:
        return BE_NOT_INITIALIZED;
    case 9:
        return BE_NOT_SUPPORTED;
    case 10:
        return BE_OUTPUT_LEN;
    case 15:
        return BE_ALGORITHM_INFO;
    case 16:
        return BE_ALLOC;
    case 19:
        return 550;
    case 20:
        return 551;
    }
    return BE_DATA;
}
