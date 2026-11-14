/*
 * Small/amx natives "dom*" (table smallext_dom): the XML DOM of the scene
 * window as seen by scripts. A script refers to a node by an integer handle,
 * translated through the XmlTagTable returned by GetNodeList.
 */

#include "../../ref/small-fmt6/amx.h"
#include "natives/smallext.h"

#include <string>

#include "app/strutil.h"

#include "xml/xml.h"
#include "xml/xml_element.h"
#include "xml/xml_string.h"
#include "xml/xml_document.h"
#include "xml/xmltagtable.h"
#include "xml/xml_attr.h"
#include "xml/xml_text.h"
#include "bntrace.h"
#include "xml/xml_filedescriptor.h"

#include "script/script.h"

/* True when [p,q] lies entirely in the script heap or entirely in its stack. */
static inline int amx_addr_ok(AMX *amx, cell p, cell q)
{
	if (p >= 0 && p < amx->hea && q >= 0 && q < amx->hea)
		return 1;
	if (p >= amx->stk && p < amx->stp && q >= amx->stk && q < amx->stp)
		return 1;
	return 0;
}
/* domGetTagName(tag, buf, size): tag name of the element, truncated to size cells. */
static cell AMX_NATIVE_CALL n_domGetTagName(AMX *amx, cell *params)
{
	XmlTagTable *list = GetNodeList(amx);
	unsigned handle = params[1];
	cell *cptr;

	if (GetTagNode(list, handle) != 0) {
	} else {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}

	Element e = GetTagNode(list, handle);
	string v = (const char *)e.get_tagName().GetBufferPointer();

	if ((unsigned int)(params[3] * 4) < v.length() + 1)
		v = v.substr(0, params[3] * 4 - 1);
	if (!amx_addr_ok(amx, params[2], v.length() + params[2])) {
		amx_RaiseError(amx, 5);  /* AMX_ERR_MEMACCESS */
		return 0;
	}
	amx_GetAddr(amx, params[2], &cptr);
	amx_SetString(cptr, (char *)v.c_str(), 1);
	return 0;
}

/* Copies the value of attribute name into out; false (out untouched) when absent. */
static bool GetAttrValue(Element elem, const string &name, string &out)
{
	Attr attr = elem.getAttributeNode(XMLString(name.c_str(), false));

	if (attr == 0)
		return false;
	out = (const char *)attr.get_value().GetBufferPointer();
	return true;
}

/* insertBefore that does nothing when child is ref itself, returning child. */
static Node InsertBeforeUnlessSame(Node parent, Node child, Node ref)
{
	if (child != ref)
		return parent.insertBefore(child, ref);
	return child;
}

struct Str3 {
	char s[3];
};
struct Str2 {
	char s[2];
};

/*
 * domGetData(tag, name, buf, size): with a name, the value of that attribute;
 * with an empty name, the text content (consecutive TEXT nodes and the first
 * child of ENTITY_REFERENCE nodes), CRLF turned into LF and the leading and
 * trailing blank lines dropped. The result goes to buf, truncated to size
 * cells; returns 1 when there was data, else 0.
 */
static cell AMX_NATIVE_CALL n_domGetData(AMX *amx, cell *params)
{
	XmlTagTable *list = GetNodeList(amx);
	unsigned handle = params[1];
	cell *cptr;

	if (GetTagNode(list, handle) != 0) {
	} else {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}

	string name = CellToString(amx, params[2]);
	string val;
	int ok = 0;
	Element e = GetTagNode(list, handle);

	if (!name.empty()) {
		ok = GetAttrValue(e, name, val);
	} else {
		Node n;

		for (n = e.get_firstChild(); n != 0 && n.get_nodeType() != 3 && n.get_nodeType() != 5;
		    n = n.get_nextSibling())
			;
		while (n != 0) {
			if (n.get_nodeType() == 3)
				val += (const char *)n.get_nodeValue().GetBufferPointer();
			else if (n.get_nodeType() == 5) {
				Node c = n.get_firstChild();

				if (c != 0)
					val += (const char *)c.get_nodeValue().GetBufferPointer();
			} else
				break;
			n = n.get_nextSibling();
		}
		if (!val.empty() && val.find_first_not_of(" \t\r\n") != string::npos) {
			{
				StringUtil su(val);
				StringUtil &r = su;
				static const Str3 kCrLf = { { '\r', '\n', '\0' } };
				static const Str2 kLf = { { '\n', '\0' } };
				Str3 crlf = BN_TRACE(48, kCrLf);
				Str2 lf = BN_TRACE(48, kLf);

				r.Replace(crlf.s, lf.s);
				val = r;
			}
			unsigned int first = val.find_first_not_of(" \t\r\n");
			unsigned int head = val.find_first_of("\r\n");

			if (first != string::npos && head != string::npos && head < first)
				val = val.substr(head + 1);

			unsigned int last = val.find_last_not_of(" \t\r\n");
			unsigned int tail = val.find_last_of("\r\n");

			if (last != string::npos && tail != string::npos && last < tail)
				val = val.substr(0, tail);
			ok = 1;
		}
	}
	if (ok) {
		if ((unsigned int)(params[4] * 4) < val.length() + 1)
			val = val.substr(0, params[4] * 4 - 1);
		if (!amx_addr_ok(amx, params[3], val.length() + params[3])) {
			amx_RaiseError(amx, 5);  /* AMX_ERR_MEMACCESS */
			return 0;
		}
		amx_GetAddr(amx, params[3], &cptr);
		amx_SetString(cptr, (char *)val.c_str(), 1);
	}
	return ok;
}

/*
 * domSetData(tag, name, value): with a name, sets that attribute; with an
 * empty name, replaces the text content (removes every TEXT node and appends
 * a new text node).
 */
static cell AMX_NATIVE_CALL n_domSetData(AMX *amx, cell *params)
{
	Document *doc = GetDocument(amx);
	XmlTagTable *list = GetNodeList(amx);
	unsigned handle = params[1];

	if (GetTagNode(list, handle) != 0) {
	} else {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}

	string name = CellToString(amx, params[2]);
	string val = CellToString(amx, params[3]);
	Element e = GetTagNode(list, handle);

	if (!name.empty()) {
		e.setAttribute(XMLString(name.c_str(), false), XMLString(val.c_str(), false));
	} else {
		for (Text t = e.get_firstChild(); t != 0; t = t.get_nextSibling()) {
			if (t != 0) {
				if (t.get_nodeType() == 3)
					e.removeChild(t);
			}
		}
		e.appendChild(doc->createTextNode(XMLString(val.c_str(), false)));
	}
	return 0;
}

/*
 * domRemoveData(tag, name): with a name, removes that attribute; with an
 * empty name, removes every TEXT child.
 */
static cell AMX_NATIVE_CALL n_domRemoveData(AMX *amx, cell *params)
{
	XmlTagTable *list = GetNodeList(amx);
	unsigned handle = params[1];

	if (GetTagNode(list, handle) != 0) {
	} else {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}

	string s = CellToString(amx, params[2]);
	Element e = GetTagNode(list, handle);

	if (!s.empty()) {
		e.removeAttribute(XMLString(s.c_str(), false));
	} else {
		Text t = e.get_firstChild();

		while (t != 0) {
			if (t != 0) {
				if (t.get_nodeType() == 3)
					e.removeChild(t);
			}
			t = t.get_nextSibling();
		}
	}
	return 0;
}

/* domAddTag(tag, name): appends a new element to the node and returns its handle. */
static cell AMX_NATIVE_CALL n_domAddTag(AMX *amx, cell *params)
{
	Document *doc = GetDocument(amx);
	XmlTagTable *list = GetNodeList(amx);
	unsigned handle = params[1];

	if (GetTagNode(list, handle) != 0) {
	} else {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}

	string s = CellToString(amx, params[2]);

	if (s.empty()) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	Node n = GetTagNode(list, handle).appendChild(doc->createElement(XMLString(s.c_str(), false)));

	return TagList_Add(*list, n);
}

/* domInsertTag(tag, name): inserts a new element before the node and returns its handle. */
static cell AMX_NATIVE_CALL n_domInsertTag(AMX *amx, cell *params)
{
	Document *doc = GetDocument(amx);
	XmlTagTable *list = GetNodeList(amx);
	unsigned handle = params[1];

	if (GetTagNode(list, handle) != 0) {
	} else {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}

	string s = CellToString(amx, params[2]);

	if (s.empty()) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}

	Node parent = GetTagNode(list, handle).get_parentNode();

	if (parent.get_nodeType() != 1) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	Node n =
	    InsertBeforeUnlessSame(parent, doc->createElement(XMLString(s.c_str(), false)), GetTagNode(list, handle));

	return TagList_Add(*list, n);
}

/*
 * domRemoveTag(tag): detaches the node and drops its handle. A root node only
 * loses its handle; a parent that is not an element is an error.
 */
static cell AMX_NATIVE_CALL n_domRemoveTag(AMX *amx, cell *params)
{
	XmlTagTable *list = GetNodeList(amx);
	unsigned handle = params[1];

	if (GetTagNode(list, handle) != 0) {
	} else {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}

	Node parent = GetTagNode(list, handle).get_parentNode();

	if (parent != 0) {
		if (parent.get_nodeType() != 1) {
			amx_RaiseError(amx, AMX_ERR_NATIVE);
			return 0;
		}
		parent.removeChild(GetTagNode(list, handle));
		TagList_Remove(*list, handle);
	} else {
		TagList_Remove(*list, handle);
	}
	return 0;
}

/* True when a is n or one of its ancestors. */
static bool NodeContains(Node a, Node n)
{
	while (n != 0) {
		if (n == a)
			return true;
		n = n.get_parentNode();
	}
	return false;
}

/*
 * domMoveTag(tag, ref, mode): moves the node relative to ref; mode 1 = first
 * child, 2 = last child, 3 = after, 4 = before. A node cannot move into its
 * own subtree, and modes 3 and 4 reject the document element.
 */
static cell AMX_NATIVE_CALL n_domMoveTag(AMX *amx, cell *params)
{
	XmlTagTable *list = GetNodeList(amx);
	unsigned handle = params[1];

	if (GetTagNode(list, handle) != 0) {
	} else {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}

	unsigned target = params[2];

	if (GetTagNode(list, target) != 0) {
	} else {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	if (NodeContains(GetTagNode(list, handle), GetTagNode(list, target))) {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}

	switch (params[3]) {
	case 1:
		InsertBeforeUnlessSame(GetTagNode(list, target), GetTagNode(list, handle),
		    GetTagNode(list, target).get_firstChild());
		break;
	case 2:
		GetTagNode(list, target).appendChild(GetTagNode(list, handle));
		break;
	case 3: {
		Node root = GetDocument(amx)->get_documentElement();

		if (!(GetTagNode(list, target) != root)) {
			amx_RaiseError(amx, AMX_ERR_NATIVE);
			return 0;
		}

		Node parent = GetTagNode(list, target).get_parentNode();
		Element e;

		e = GetTagNode(list, target).get_nextSibling();
		while (e != 0 && e.get_nodeType() != 1)
			e = e.get_nextSibling();

		if (e != 0)
			InsertBeforeUnlessSame(parent, GetTagNode(list, handle), e);
		else
			parent.appendChild(GetTagNode(list, handle));
		break;
	}
	case 4: {
		Node root = GetDocument(amx)->get_documentElement();

		if (!(GetTagNode(list, target) != root)) {
			amx_RaiseError(amx, AMX_ERR_NATIVE);
			return 0;
		}

		Node parent = GetTagNode(list, target).get_parentNode();

		InsertBeforeUnlessSame(parent, GetTagNode(list, handle), GetTagNode(list, target));
		break;
	}
	default:
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	return 0;
}

static cell AMX_NATIVE_CALL n_domRoot(AMX *amx, cell *params)
{
	Document *doc = GetDocument(amx);
	XmlTagTable *list = GetNodeList(amx);
	Node node = doc->get_documentElement();

	return FindTagFromList(*list, node);
}

static cell AMX_NATIVE_CALL n_domParent(AMX *amx, cell *params)
{
	XmlTagTable *list = GetNodeList(amx);
	unsigned handle = params[1];

	if (GetTagNode(list, handle) != 0) {
	} else {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}
	Node node = GetTagNode(list, handle);
	Node parent = node.get_parentNode();

	return FindTagFromList(*list, parent);
}

/*
 * domFirstChild(tag, name): handle of the first element from get_firstChild()
 * on, through the next siblings, whose tag name is name (any element when
 * empty). domLastChild, domNext and domPrev do the same from get_lastChild(),
 * get_nextSibling() and get_previousSibling().
 */
static cell AMX_NATIVE_CALL n_domFirstChild(AMX *amx, cell *params)
{
	XmlTagTable *list = GetNodeList(amx);
	unsigned handle = params[1];

	if (GetTagNode(list, handle) != 0) {
	} else {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}

	string tag = CellToString(amx, params[2]);
	Node node = GetTagNode(list, handle);
	int anyTag = tag.length() == 0;
	Element e = node.get_firstChild();

	while (e != 0) {
		if (e.get_nodeType() == 1) {
			if (anyTag || tag == (const char *)e.get_tagName().GetBufferPointer())
				return FindTagFromList(*list, e);
		}
		e = e.get_nextSibling();
	}
	return 0;
}

static cell AMX_NATIVE_CALL n_domLastChild(AMX *amx, cell *params)
{
	XmlTagTable *list = GetNodeList(amx);
	unsigned handle = params[1];

	if (GetTagNode(list, handle) != 0) {
	} else {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}

	string tag = CellToString(amx, params[2]);
	Node node = GetTagNode(list, handle);
	int anyTag = tag.length() == 0;
	Element e = node.get_lastChild();

	while (e != 0) {
		if (e.get_nodeType() == 1) {
			if (anyTag || tag == (const char *)e.get_tagName().GetBufferPointer())
				return FindTagFromList(*list, e);
		}
		e = e.get_previousSibling();
	}
	return 0;
}

static cell AMX_NATIVE_CALL n_domNext(AMX *amx, cell *params)
{
	XmlTagTable *list = GetNodeList(amx);
	unsigned handle = params[1];

	if (GetTagNode(list, handle) != 0) {
	} else {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}

	string tag = CellToString(amx, params[2]);
	Node node = GetTagNode(list, handle);
	int anyTag = tag.length() == 0;
	Element e = node.get_nextSibling();

	while (e != 0) {
		if (e.get_nodeType() == 1) {
			if (anyTag || tag == (const char *)e.get_tagName().GetBufferPointer())
				return FindTagFromList(*list, e);
		}
		e = e.get_nextSibling();
	}
	return 0;
}

static cell AMX_NATIVE_CALL n_domPrev(AMX *amx, cell *params)
{
	XmlTagTable *list = GetNodeList(amx);
	unsigned handle = params[1];

	if (GetTagNode(list, handle) != 0) {
	} else {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}

	string tag = CellToString(amx, params[2]);
	Node node = GetTagNode(list, handle);
	int anyTag = tag.length() == 0;
	Element e = node.get_previousSibling();

	while (e != 0) {
		if (e.get_nodeType() == 1) {
			if (anyTag || tag == (const char *)e.get_tagName().GetBufferPointer())
				return FindTagFromList(*list, e);
		}
		e = e.get_previousSibling();
	}
	return 0;
}

/*
 * Search behind domFindTag: the first element child of node whose tag name is
 * tag and whose node value is value (an empty string means any); with
 * deep, it descends into the subtrees.
 */
static Element FindTagRecursive(Element node, const string &tag, const string &value, int deep)
{
	for (Element e = node.get_firstChild(); e != 0; e = e.get_nextSibling()) {
		if (e.get_nodeType() == 1
		    && (tag.length() == 0 || tag == (const char *)e.get_tagName().GetBufferPointer())
		    && (value.length() == 0 || value == (const char *)e.GetNodePtrValue().GetBufferPointer()))
			return e;

		if (deep) {
			Element found = FindTagRecursive(e, tag, value, deep);

			if (found != 0)
				return found;
		}
	}
	return Node(0);
}

/*
 * domFindTag(tag, name, value, deep): handle of the first element under the
 * node with that tag name and value (empty = any).
 */
static cell AMX_NATIVE_CALL n_domFindTag(AMX *amx, cell *params)
{
	XmlTagTable *list = GetNodeList(amx);
	unsigned handle = params[1];

	if (GetTagNode(list, handle) != 0) {
	} else {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}

	string tag = CellToString(amx, params[2]);
	string value = CellToString(amx, params[3]);
	Node node = GetTagNode(list, handle);
	Element found = FindTagRecursive(node, tag, value, params[4] != 0);

	if (found != 0)
		return FindTagFromList(*list, found);
	return 0;
}

/* domDump(tag, buf, size): the subtree serialized as XML, truncated to size cells. */
static cell AMX_NATIVE_CALL n_domDump(AMX *amx, cell *params)
{
	XmlTagTable *list = GetNodeList(amx);
	unsigned handle = params[1];
	cell *cptr;

	if (GetTagNode(list, handle) != 0) {
	} else {
		amx_RaiseError(amx, AMX_ERR_NATIVE);
		return 0;
	}

	XMLString buf;

	GetTagNode(list, handle).SerialiseToBuffer(&buf);

	string v = (const char *)buf.GetBufferPointer();

	if ((unsigned int)(params[3] * 4) < v.length() + 1)
		v = v.substr(0, params[3] * 4 - 1);
	if (!amx_addr_ok(amx, params[2], v.length() + params[2])) {
		amx_RaiseError(amx, 5);  /* AMX_ERR_MEMACCESS */
		return 0;
	}
	amx_GetAddr(amx, params[2], &cptr);
	amx_SetString(cptr, (char *)v.c_str(), 1);
	return 0;
}

AMX_NATIVE_INFO smallext_dom[] = {
	{ "domGetTagName", n_domGetTagName },
	{ "domGetData", n_domGetData },
	{ "domSetData", n_domSetData },
	{ "domRemoveData", n_domRemoveData },
	{ "domAddTag", n_domAddTag },
	{ "domInsertTag", n_domInsertTag },
	{ "domRemoveTag", n_domRemoveTag },
	{ "domMoveTag", n_domMoveTag },
	{ "domRoot", n_domRoot },
	{ "domParent", n_domParent },
	{ "domFirstChild", n_domFirstChild },
	{ "domLastChild", n_domLastChild },
	{ "domNext", n_domNext },
	{ "domPrev", n_domPrev },
	{ "domFindTag", n_domFindTag },
	{ "domDump", n_domDump },
	{ 0, 0 },
};
