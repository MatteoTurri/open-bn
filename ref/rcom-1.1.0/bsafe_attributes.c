#include "bsafe.h"
#include "bsafe_aitype.h"

int GetInternalAttributes(AIAttrEntry **out, AIAttrEntry **table, unsigned char *name)
{
    unsigned char lower[48];
    unsigned int i;

    i = 0;
    if (*name != 0) {
        do {
            lower[i] = (*name >= 'A' && *name <= 'Z') ? *name + 32 : *name;
            name++;
            i++;
        } while (*name != 0);
    }
    lower[i] = 0;

    for (i = 0; table[i] != 0; i++) {
        if (T_strcmp(table[i]->name, (char *)lower) == 0) {
            *out = table[i];
            return 0;
        }
    }
    return BE_NOT_SUPPORTED;
}
