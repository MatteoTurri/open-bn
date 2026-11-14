/*
 * The hardware chooser of BSAFE: an array of algorithm methods where a device (a chooser object) can
 * install its own implementations, closed by an AM_FAUX_AIT entry that points to the device.
 */

#include "bsafe.h"
#include "bsafe_amtype.h"

/* The hardware algorithm method that a template entry describes. */
typedef struct BsafeAmMeth {
	B_AlgorithmInfoType *id;        /* 0x00: the algorithm info type of the new entry */
	int (*getLen)(unsigned int *);  /* 0x04: size of the method table to copy */
} BsafeAmMeth;

typedef struct BsafeAmDesc {
	void *buf;          /* 0x00: the method table to copy */
	BsafeAmMeth *meth;  /* 0x04 */
} BsafeAmDesc;

struct HwChooserObj;

/* An entry of a hardware chooser: an algorithm method and its device. In a template entry
 * am.algorithmInfoType is the chooser object and am.alga a BsafeAmDesc; the entry with
 * AM_FAUX_AIT closes the entries of the chooser and points to the device. */
typedef struct HwAlgorithmMethod {
	B_AlgorithmMethod am;  /* 0x00 */
	union {
		struct HwDevice *device;           /* in the AM_FAUX_AIT entry */
		struct HwAlgorithmMethod *faux;    /* in an entry it installed: the AM_FAUX_AIT entry */
	} link;                /* 0x10 */
	int unk_14;            /* 0x14 */
} HwAlgorithmMethod; /* size = 0x18 */

/* The device of a hardware chooser, allocated with the chooser array. */
typedef struct HwDevice {
	BsafeMemoryPool pool;          /* 0x00: holds the array and the new entries */
	HwAlgorithmMethod **chooser;   /* 0x0C */
	int *addedFlags;               /* 0x10: one per entry, 1 = installed by the device */
	ITEM name;                     /* 0x14 */
	struct HwChooserObj *obj;      /* 0x1C */
	void *handle;                  /* 0x20 */
} HwDevice; /* size = 0x24 */

int InstallNewAM(amList, index, outIndex, src, nextStatus, searchList, chooserFn, chooserArg, chooserObj, pAdded)
	HwAlgorithmMethod **amList;
	int index;
	int outIndex;
	HwAlgorithmMethod *src;
	int nextStatus;
	HwAlgorithmMethod **searchList;
	int chooserFn;
	int chooserArg;
	int chooserObj;
	int *pAdded;
{
	HwAlgorithmMethod *node;
	unsigned int len;
	HwAlgorithmMethod *parent;
	HwDevice *device;
	BsafeAmMeth *meth;
	B_AlgorithmInfoType *id;
	int status;

	parent = amList[index - 2];
	meth = ((BsafeAmDesc *)src->am.alga)->meth;
	device = parent->link.device;
	id = meth->id;

	if (nextStatus != 0) {
		int i;

		*pAdded = 0;
		if (searchList == 0)
			return nextStatus;
		for (i = 0; searchList[i] != 0; i++) {
			if (searchList[i]->am.algorithmInfoType == id) {
				amList[outIndex] = searchList[i];
				return 0;
			}
		}
		return nextStatus;
	}

	if ((status = B_MemoryPoolAlloc(&device->pool, (void **)&node, 24)) != 0)
		return status;
	node->am.encryptFlag = src->am.encryptFlag;
	node->unk_14 = src->unk_14;
	node->am.keyInfoType = src->am.keyInfoType;
	node->am.algorithmInfoType = id;
	if ((status = (*meth->getLen)(&len)) != 0)
		return status;
	if ((status = B_MemoryPoolAlloc(&device->pool, &node->am.alga, len)) != 0)
		return status;
	T_memcpy(node->am.alga, ((BsafeAmDesc *)src->am.alga)->buf, len);
	node->link.faux = parent;
	amList[outIndex] = node;
	*pAdded = 1;
	return 0;
}

/* B_CreateHardwareChooser and its helpers. AM_FAUX_AIT tags the closing entries of the arrays. */

typedef struct HwChooserOps {
	int (*init)(struct HwChooserObj *, int, void *);   /* +0 */
} HwChooserOps;

typedef struct HwChooserObj {
	HwChooserOps *ops;                    /* +0 */
	char pad_04[8];  /* unused, but part of the layout */
	int (*open)(void **, void *);          /* +12 */
	int (*next)(void *, ITEM **);           /* +16 */
	int (*close)(void *);                    /* +20 */
} HwChooserObj;

typedef int (*HwChooserFn)(void **);

/* AM_FAUX_AIT tags the closing entry of an array; the device name "software" is what B_GetDevice
 * returns when no device chose; DEVICE_NAME_FLAG is the cache key of the device name. */
B_AlgorithmInfoType AM_FAUX_AIT = { 0, { 0, 0 } };
static char deviceNameSoftware[] = "software";
int DEVICE_NAME_FLAG = 0;


static int CountAmList(HwAlgorithmMethod **templateList, HwAlgorithmMethod **softwareList, HwChooserObj *obj,
		       int *pCount);
static int BuildAmArray(HwAlgorithmMethod ***pOut, int n, HwChooserObj *obj, void *handle,
			ITEM *deviceName);
static int PrepareAmEntry(HwAlgorithmMethod *entry, int n, HwAlgorithmMethod **templateList,
			  HwChooserObj *obj);

/* Prepares the flag table of the closing entry just built (entry->link.device->addedFlags, n
 * entries): the last two -1, then the 1s of the closing entry already in templateList, keeping one more
 * slot for each entry of templateList that belongs to obj. Nothing when templateList has no closing entry.
 * Returns 0. */
static int PrepareAmEntry(entry, n, templateList, obj)
	HwAlgorithmMethod *entry;
	int n;
	HwAlgorithmMethod **templateList;
	HwChooserObj *obj;
{
	int *flags;
	int *src;
	int i, j, k;

	flags = entry->link.device->addedFlags;
	flags[n - 2] = -1;
	flags[n - 1] = -1;
	for (i = 0; ; i++) {
		HwAlgorithmMethod *e = templateList[i];
		if (e == 0)
			return 0;
		if (e->am.algorithmInfoType == &AM_FAUX_AIT)
			goto found;
	}
found:
	src = templateList[i]->link.device->addedFlags;
	j = 0;
	k = 0;
	while (src[j] != -1) {
		if (src[j] == 1)
			flags[k] = src[j];
		j++;
		templateList++;
		k++;
		if ((*templateList)->am.algorithmInfoType == (B_AlgorithmInfoType *)obj)
			k++;
	}
	return 0;
}

/* Allocates the pool (36 bytes, B_MemoryPoolConstructor) and the closing entry (24 bytes plus the
 * flag table, aligned to 8), and with the pool the array of n pointers in *pOut, filled with 0xFF,
 * the closing entry and the final 0. BE_ALLOC when memory runs out. */
static int BuildAmArray(pOut, n, obj, handle, deviceName)
	HwAlgorithmMethod ***pOut;
	int n;
	HwChooserObj *obj;
	void *handle;
	ITEM *deviceName;
{
	HwDevice *device;
	char *mem;
	HwAlgorithmMethod *ent;
	unsigned int size;

	device = T_malloc(36);
	if (device == 0)
		return BE_ALLOC;
	T_memset(device, 0, 36);
	B_MemoryPoolConstructor(&device->pool);
	size = n * 4;
	mem = T_malloc(size + 32);
	if (mem == 0) {
		T_free(device);
		return BE_ALLOC;
	}
	T_memset(mem, 0, size + 32);
	ent = (HwAlgorithmMethod *)mem;
	mem += 24;
	mem += 8 - ((unsigned int)mem & 7);
	ent->link.device = device;
	ent->am.algorithmInfoType = &AM_FAUX_AIT;
	ent->unk_14 = 0;
	device->handle = handle;
	device->obj = obj;
	device->addedFlags = (int *)mem;
	if (deviceName != 0) {
		device->name.data = deviceName->data;
		device->name.len = deviceName->len;
	}
	if (B_MemoryPoolAlloc(&device->pool, (void **)pOut, size) != 0) {
		T_free(ent->link.device);
		T_free(ent);
		return BE_ALLOC;
	}
	T_memset(*pOut, 255, size);
	device->chooser = *pOut;
	(*pOut)[n - 2] = ent;
	(*pOut)[n - 1] = 0;
	return 0;
}

static int CountAmList(templateList, softwareList, obj, pCount)
	HwAlgorithmMethod **templateList;
	HwAlgorithmMethod **softwareList;
	HwChooserObj *obj;
	int *pCount;
{
	int n;
	int i;
	HwAlgorithmMethod *e;

	i = 0;
	n = 0;
	while ((e = templateList[i]) != 0) {
		if (e->am.algorithmInfoType == (B_AlgorithmInfoType *)obj)
			n++;
		if (e->am.algorithmInfoType == &AM_FAUX_AIT)
			break;
		i++;
	}
	if (n == 0)
		return 0;
	n += i + 2;

	i = 0;
	if (softwareList != 0) {
		while (softwareList[i] != 0)
			i++;
		e = softwareList[i - 1];
		if (e->am.algorithmInfoType == &AM_FAUX_AIT)
			return 0;
	}
	*pCount = i;
	return n;
}

/* BE_INPUT_DATA when pOut is 0 or *pOut is already set, BE_ALG_OPERATION_UNKNOWN when the chooser does not answer 2. Then init,
 * open and next on the chooser object (next may return BE_HARDWARE, empty, which is not an error), counts
 * its methods, builds the array, copies the entries of templateList up to the AM_FAUX_AIT one (installing
 * those that belong to the chooser) and appends the softwareCount entries of softwareList. On error the chooser
 * is destroyed and *pOut set to 0. */
int B_CreateHardwareChooser(templateList, pOut, softwareList, chooser, chooserArg)
	HwAlgorithmMethod **templateList;
	HwAlgorithmMethod ***pOut;
	HwAlgorithmMethod **softwareList;
	HwChooserFn chooser;
	void *chooserArg;
{
	HwChooserObj *obj;
	void *handle;
	ITEM *deviceName;
	int softwareCount;
	int templateSlots;
	int nextStatus;
	int status;
	int i, j;
	HwAlgorithmMethod *entry;
	HwAlgorithmMethod *e;
	int *flags;

	templateSlots = 0;
	nextStatus = 0;
	softwareCount = 0;
	handle = 0;
	obj = 0;
	deviceName = 0;
	if (pOut == 0)
		return BE_INPUT_DATA;
	if (*pOut != 0)
		return BE_INPUT_DATA;
	if ((*chooser)((void **)&obj) != 2)
		return BE_ALG_OPERATION_UNKNOWN;

	do {
		status = (*obj->ops->init)(obj, 0, chooserArg);
		if (status != 0)
			break;
		status = (*obj->open)(&handle, chooserArg);
		if (status != 0)
			break;
		nextStatus = (*obj->next)(handle, &deviceName);
		if (nextStatus != 0) {
			status = nextStatus;
			if (status != BE_HARDWARE)
				goto opened;
		}
		status = BE_METHOD_NOT_IN_CHOOSER;
		templateSlots = CountAmList(templateList, softwareList, obj, &softwareCount);
		if (templateSlots != 0)
			status = 0;
	} while (0);
opened:
	if (status != 0) {
		(*obj->close)(handle);
		return status;
	}

	do {
		status = BuildAmArray(pOut, templateSlots + softwareCount, obj, handle, deviceName);
		if (status != 0)
			break;
		entry = (*pOut)[templateSlots + softwareCount - 2];
		status = PrepareAmEntry(entry, templateSlots + softwareCount, templateList, obj);
		if (status != 0)
			goto destroy;
		j = 0;
		flags = entry->link.device->addedFlags;
		for (i = 0; templateList[i] != 0 && (e = templateList[i])->am.algorithmInfoType != &AM_FAUX_AIT; i++) {
			(*pOut)[j] = templateList[i];
			j++;
			if (e->am.algorithmInfoType == (B_AlgorithmInfoType *)obj) {
				status = InstallNewAM(*pOut, templateSlots + softwareCount, j,
						      e, nextStatus, softwareList, chooser,
						      chooserArg, obj,
						      &flags[j]);
				if (status != 0)
					break;
				j++;
			}
		}
		if (status == 0) {
			for (i = 0; i < softwareCount; i++, j++)
				(*pOut)[j] = softwareList[i];
		}
	} while (0);
	if (status == 0)
		return 0;
destroy:
	B_DestroyHardwareChooser(pOut);
	*pOut = 0;
	return status;
}


/* *pObj is an array of pointers closed by 0. Finds the last entry, checks that it is the
 * AM_FAUX_AIT one and destroys its device: the pool, the close method of the chooser object, the
 * device and the entry. */
void B_DestroyHardwareChooser(HwAlgorithmMethod ***pObj)
{
    HwAlgorithmMethod **obj = *pObj;
    if (obj != 0) {
        unsigned int idx = 0;
        HwAlgorithmMethod **p = obj;
        HwAlgorithmMethod *entry = *p;
        if (entry != 0) {
            do {
                p++;
                idx++;
            } while (*p != 0);
        }
        {
            entry = obj[idx - 1];
            if (entry->am.algorithmInfoType == &AM_FAUX_AIT) {
                HwDevice *device = entry->link.device;
                HwChooserObj *chooserObj = device->obj;
                int (*fn)(void *);
                B_MemoryPoolDestructor(&device->pool);
                fn = chooserObj->close;
                fn(device->handle);
                T_free(entry->link.device);
                T_free(entry);
                *pObj = 0;
            }
        }
    }
}

/* Finds the first entry of `array` with the method (infoType, encryptFlag, alga) before the
 * AM_FAUX_AIT entry and, if the device installed it, records the device name in the cache under
 * DEVICE_NAME_FLAG. */
int SetDeviceName(BsafeAlgorithmObj *algorithm, B_AlgorithmInfoType *infoType, int encryptFlag, void *alga,
                  HwAlgorithmMethod **array)
{
    int found = -1;
    unsigned int i;
    int *flags;
    HwDevice *device;

    for (i = 0; ; i++) {
        if (array[i] == 0)
            return 0;
        if (found == -1 && array[i]->am.algorithmInfoType == infoType && array[i]->am.encryptFlag == encryptFlag
                && array[i]->am.alga == alga)
            found = i;
        if (array[i]->am.algorithmInfoType == &AM_FAUX_AIT)
            break;
    }
    if (found == -1)
        return 0;

    flags = array[i]->link.device->addedFlags;
    if (flags[found] == 0)
        return 0;

    device = array[found]->link.faux->link.device;
    return B_InfoCacheAddInfo(&algorithm->cache, &DEVICE_NAME_FLAG, &device->name);
}

int B_GetDevice(ITEM *outInfo, BsafeInfoCache *cache)
{
    ITEM *found;
    int status;
    outInfo->data = (unsigned char *)deviceNameSoftware;
    outInfo->len = 9;
    status = B_InfoCacheFindInfo(cache, &found, &DEVICE_NAME_FLAG);
    if (status != 0)
        return 0;
    outInfo->data = found->data;
    outInfo->len = found->len;
    return 0;
}
