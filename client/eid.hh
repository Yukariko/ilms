// -*- c-basic-offset: 4; related-file-name: "./id.cc" -*-
#ifndef CLICK_EID_HH
#define CLICK_EID_HH
#include <click/string.hh>
#include <click/vector.hh>
#include <click/glue.hh>
#include <click/hashcode.hh>
CLICK_DECLS
class StringAccum;
class Element;

class EID { public:
    EID();

    inline EID(const struct click_id& id)
	: _id(id) {
    }

	explicit inline EID(const unsigned char *x) {
		memcpy(&_id, x, sizeof(_id));
    }

    EID(const String& str);

    inline const struct click_id& id() const;
    inline struct click_id& id() { return _id;}

    inline unsigned char* data();
    inline const unsigned char* data() const;


    inline operator struct click_id() const;

    inline hashcode_t hashcode() const;

    inline bool operator==(const EID&) const;
    inline bool operator!=(const EID&) const;

    EID& operator=(const EID&);
    EID& operator=(const struct click_id&);

    bool parse(const String& str);
    String unparse() const;
    String unparse_pretty(const Element* context = (Element *)NULL) const;

	bool valid();

  private:
    click_id_t _id;
};


/** @brief Return a struct click_id corresponding to the EID. */
inline const struct click_id&
EID::id() const
{
    return _id;
}

/** @brief Return a pointer to the EID data. */
inline const unsigned char*
EID::data() const
{
    return reinterpret_cast<const unsigned char*>(&_id);
}

/** @brief Return a pointer to the EID data. */
inline unsigned char*
EID::data()
{
    return reinterpret_cast<unsigned char*>(&_id);
}


/** @brief Return a struct click_xia_eid corresponding to the address. */
inline
EID::operator struct click_id() const
{
    return id();
}

StringAccum& operator<<(StringAccum&, const EID&);


/** @brief Hash function.
 * @return The hash value of this EID.
 *
 * returns the last 32 bit of EID.
 * This hash function use hashcode provided at string.cc
 */
inline hashcode_t
EID::hashcode() const
{
    return String::hashcode(id().id8, (id().id8 + SIZE_OF_HASH + 3));
}

/** @brief Return if the addresses are the same. */
inline bool
EID::operator==(const EID& rhs) const
{
    return reinterpret_cast<const uint32_t*>(&_id)[0] == reinterpret_cast<const uint32_t*>(&rhs._id)[0] &&
           reinterpret_cast<const uint32_t*>(&_id)[1] == reinterpret_cast<const uint32_t*>(&rhs._id)[1] &&
           reinterpret_cast<const uint32_t*>(&_id)[2] == reinterpret_cast<const uint32_t*>(&rhs._id)[2] &&
           reinterpret_cast<const uint32_t*>(&_id)[3] == reinterpret_cast<const uint32_t*>(&rhs._id)[3] &&
           reinterpret_cast<const uint32_t*>(&_id)[4] == reinterpret_cast<const uint32_t*>(&rhs._id)[4] &&
           reinterpret_cast<const uint32_t*>(&_id)[5] == reinterpret_cast<const uint32_t*>(&rhs._id)[5];
}

/** @brief Return if the addresses are different. */
inline bool
EID::operator!=(const EID& rhs) const
{
    return reinterpret_cast<const uint32_t*>(&_id)[0] != reinterpret_cast<const uint32_t*>(&rhs._id)[0] ||
           reinterpret_cast<const uint32_t*>(&_id)[1] != reinterpret_cast<const uint32_t*>(&rhs._id)[1] ||
           reinterpret_cast<const uint32_t*>(&_id)[2] != reinterpret_cast<const uint32_t*>(&rhs._id)[2] ||
           reinterpret_cast<const uint32_t*>(&_id)[3] != reinterpret_cast<const uint32_t*>(&rhs._id)[3] ||
           reinterpret_cast<const uint32_t*>(&_id)[4] != reinterpret_cast<const uint32_t*>(&rhs._id)[4] ||
           reinterpret_cast<const uint32_t*>(&_id)[5] != reinterpret_cast<const uint32_t*>(&rhs._id)[5];
}

class ArgContext;
class Args;
extern const ArgContext blank_args;

/** @class EIDArg
  @brief Parser class for EID. */

struct EIDArg {
    static bool parse(const String&, EID&, Args&);
    static bool parse(const String&, Vector<EID>&, Args&);
};

template<> struct DefaultArg<EID> : public EIDArg {};


CLICK_ENDDECLS
#endif
