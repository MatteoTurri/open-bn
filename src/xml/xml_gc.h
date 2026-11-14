/*
 * GCObject: reference-counted base of the DOM implementation objects.
 */

#ifndef GUARD_XML_GC_H
#define GUARD_XML_GC_H
/*
 * The *Impl classes derive from it; the handle classes (Node...) hold a pointer to
 * their Impl and call AddRef/RemoveRef on construction and destruction.
 * fRefCount is at 0x00 and the vtable pointer at 0x04.
 */
class GCObject {
public:
	GCObject();
	virtual void firstReference();
	virtual void lastReference();
	virtual ~GCObject();

	static void AddRef(GCObject *obj);
	static void RemoveRef(GCObject *obj);
	int Referenced();

protected:
	int fRefCount;
};

#endif // GUARD_XML_GC_H
