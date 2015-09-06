// -*- c-basic-offset: 4; related-file-name: "../lib/eid.hh" -*-
/*
 * eid.{cc,hh} -- EID class
 */

#include <click/config.h>
#include <click/glue.hh>
#include "eid.hh"
#include <click/confparse.hh>
#include <click/straccum.hh>
#include <click/integers.hh>
#include <click/args.hh>
CLICK_DECLS

EID::EID()
{
    memset(&_id, 0, sizeof(_id));
}

EID::EID(const String &str)
{
    parse(str);
}

EID&
EID::operator=(const EID& rhs)
{
    _id = rhs._id;
    return *this;
}

EID&
EID::operator=(const struct click_id& rhs)
{
    _id = rhs;
    return *this;
}

// parse string to EID
// EID representation
//	ver.mode.rsv.scope::Hex(4byte):Hex(4byte):Hex(4byte):Hex(4byte):Hex(4byte)
//	ver.mode.rsv.scope::Hex(4byte)
bool
EID::parse(const String& str)
{
	int start=0, end=0;
	String word;
	unsigned int ver, mode, rsv, scope;

	if ((end = str.find_left(".")) < 0) return false;
	cp_unsigned(str.substring(start, end-start), 16, &ver);
	_id.idver = ver & 0xF;

	start=end+1;
	if ((end = str.find_left(".", start+1)) < 0) return false;
	cp_unsigned(str.substring(start, end-start), 16, &mode);
	_id.idmode = mode & 0xF;

	start=end+1;
	if ((end = str.find_left(".", start+1)) < 0) return false;
	cp_unsigned(str.substring(start, end-start), 16, &rsv);
	_id.idrsv = rsv & 0xFF;

	start=end+1;
	if ((end = str.find_left(':', start+1)) < 0) return false;
	cp_unsigned(str.substring(start, end-start), 16, &scope);
	_id.idscope = scope & 0xFFFF;

	end = str.length();
	int index = 5;
	do {
		if ((start = str.find_right(':', end)) < 0) return false;
		uint32_t tempid;
		cp_unsigned(str.substring(start+1,end-start), 16, &tempid);
//		cp_unsigned(str.substring(start+1), 16, &tempid);
		_id.id32[index--] = htonl(tempid); // for byte order
		end = start - 1;
	} while(index && start && str.c_str()[start-1] != ':');

	while(index)
		_id.id32[index--] = 0;

//	click_chatter("EID <%s>", unparse().c_str());

	return true;
}

/** @brief Unparses this address into a String.

    Maintains the invariant that,
    for an EID @a a, EID(@a a.unparse()) == @a a. */
String
EID::unparse() const
{
	StringAccum sa;
	const uint32_t *p = _id.id32;

	sa.snprintf(3, "%1X.", _id.idver);
	sa.snprintf(3, "%1X.", _id.idmode);
	sa.snprintf(4, "%02X.", _id.idrsv);
	sa.snprintf(6, "%04X:", _id.idscope);

	bool brief = true;
    for (int i = 1; i < 5; i++) {
		if (p[i]) { brief = false; }
		if (!brief)
			sa.snprintf(12, ":%08X", ntohl(p[i]));
	}
	sa.snprintf(12, ":%08X", ntohl(p[5]));
	return sa.take_string();
}

String
EID::unparse_pretty(const Element* context) const
{
    String s;
// #ifndef CLICK_TOOL
//    if (context)
//       s = EIDInfo::revquery_id(&_id, context);
//#else
//    (void)context;
//#endif
    if (s.length() != 0)
        return s;
    else
        return unparse();
}

StringAccum &
operator<<(StringAccum &sa, const EID& id)
{
    return sa << id.unparse();
}

bool
EIDArg::parse(const String &str, EID &eid, Args &args /* args */)
{
    if (eid.parse(str)) return true;
    else return false;
}

bool
EIDArg::parse(const String &str, Vector<EID> &value, Args &args)
{
    String arg(str);
    Vector<EID> v;
    EID id_tmp;
    int nwords = 0;

    while (String word = cp_shift_spacevec(arg)) {
		++nwords;

		if (id_tmp.parse(word))
			v.push_back(id_tmp);
		else
			return false;
	}
    if (nwords == v.size()) {
		v.swap(value);
		return true;
    }
    args.error("out of memory");
    return false;
}

CLICK_ENDDECLS
ELEMENT_PROVIDES(EID)
