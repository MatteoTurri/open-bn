/*
 * ObjectTable and ModelTable: handle tables mapping integer keys to Objects
 * and ModelData. Flagged entries (objects of network scenes) are limited to
 * 256.
 */

#include "script/scenewindow.h"
#include "xml/xml_filedescriptor.h"
#include "script/script.h"
#include "hashtable.h"
#include "objecttable.h"

class Object;
class ModelData;

static HashTable<Object *> *s_objectTable;
static HashTable<ModelData *> *s_modelTable;
static int s_networkObjCount;
static int s_modelCount;

void ObjectTable::Init()
{
	s_objectTable = new HashTable<Object *>(1024, 512);
	s_networkObjCount = 0;
}

void ObjectTable::Exit()
{
	if (s_objectTable)
		delete s_objectTable;
}

int ObjectTable::Regist(Object *obj)
{
	if (!obj)
		return 0;
	int extra = GetSceneWindow(obj)->IsFromNetwork();
	if (extra != 0) {
		int newCount = s_networkObjCount + 1;
		if (newCount >= 257)
			return 0;
		s_networkObjCount = newCount;
	}
	return s_objectTable->Regist(obj, extra);
}

Object *ObjectTable::Get(int key)
{
	if (!key)
		return 0;
	return s_objectTable->Get(key, 0);
}

int ObjectTable::Remove(int key)
{
	if (key) {
		int extra;
		if (s_objectTable->Remove(key, &extra)) {
			if (extra & 1)
				s_networkObjCount--;
			return 1;
		}
	}
	return 0;
}

void ObjectTable::Dump()
{
	s_objectTable->Dump();
}

void ModelTable::Init()
{
	s_modelTable = new HashTable<ModelData *>(1024, 512);
	s_modelCount = 0;
}

void ModelTable::Exit()
{
	if (s_modelTable)
		delete s_modelTable;
}

int ModelTable::Regist(ModelData *data, bool flag)
{
	if (!data)
		return 0;
	if (flag) {
		int newCount = s_modelCount + 1;
		if (newCount >= 257)
			return 0;
		s_modelCount = newCount;
	}
	return s_modelTable->Regist(data, flag);
}

ModelData *ModelTable::Get(int key)
{
	if (!key)
		return 0;
	return s_modelTable->Get(key, 0);
}

int ModelTable::Remove(int key)
{
	if (key) {
		int extra;
		if (s_modelTable->Remove(key, &extra)) {
			if (extra & 1)
				s_modelCount--;
			return 1;
		}
	}
	return 0;
}

void ModelTable::Dump()
{
	s_modelTable->Dump();
}
