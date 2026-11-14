/*
 * Handle tables of objecttable.cpp: integer keys mapped to Objects and to
 * ModelData.
 */

#ifndef OBJECTTABLE_H
#define OBJECTTABLE_H

class Object;
class ModelData;

namespace ObjectTable {
void Init();
void Exit();
int Regist(Object *obj);
Object *Get(int key);
int Remove(int key);
void Dump();
}

namespace ModelTable {
void Init();
void Exit();
int Regist(ModelData *data, bool flag);
ModelData *Get(int key);
int Remove(int key);
void Dump();
}

#endif
