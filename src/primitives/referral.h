// Copyright (c) 2017 The Merit Foundation developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MERIT_PRIMITIVES_REFERRAL_H
#define MERIT_PRIMITIVES_REFERRAL_H

#include "pubkey.h"
#include "script/script.h"
#include "serialize.h"
#include "uint256.h"

#include <stdint.h>
#include <vector>

typedef std::vector<unsigned char> valtype;

namespace referral
{

using Address = uint160;

struct MutableReferral;

static const int SERIALIZE_REFERRAL = 0x40000000;

/**
 * Basic referral serialization format:
 *
 * Extended referral serialization format:
 */
template<typename Stream, typename TxType>
inline void UnserializeReferral(TxType& ref, Stream& s) {
    s >> ref.parentAddress;
    s >> ref.addressType;
    s >> ref.address;
    // s >> ref.signature;
}

template<typename Stream, typename TxType>
inline void SerializeReferral(const TxType& ref, Stream& s) {
    s << ref.parentAddress;
    s << ref.addressType;
    s << ref.address;
    // s >> ref.signature;
}

/** The basic referral that is broadcast on the network and contained in
 * blocks. A referral references a previous referral which helps construct the
 * referral tree.
 */
class Referral
{
public:
    // Default referral version.
    static const int32_t CURRENT_VERSION=0;

    // Changing the default referral version requires a two step process: first
    // adapting relay policy by bumping MAX_STANDARD_VERSION, and then later date
    // bumping the default CURRENT_VERSION at which point both CURRENT_VERSION and
    // MAX_STANDARD_VERSION will be equal.
    static const int32_t MAX_STANDARD_VERSION=0;

    const int32_t version;

    // address of previous referral
    Address parentAddress;

    // Type of address. 0 == Key ID, 1 = Script ID, 2 = Parameterized Script ID
    const char addressType;

    // address that this referral is related to
    Address address;

    // signature of parentAddress + address
    valtype signature;

private:
    /** Memory only. */
    const uint256 hash;

    uint256 ComputeHash() const;

public:

    Referral(
            char addressTypeIn,
            const Address& addressIn,
            const Address& parentAddressIn);

    /** Convert a MutableReferral into a Referral. */
    Referral(const MutableReferral &ref);
    Referral(MutableReferral &&ref);

    template <typename Stream>
    inline void Serialize(Stream& s) const {
        SerializeReferral(*this, s);
    }

    /** This deserializing constructor is provided instead of an Unserialize method.
     *  Unserialize is not possible, since it would require overwriting const fields. */
    template <typename Stream>
    Referral(deserialize_type, Stream& s) : Referral(MutableReferral(deserialize, s)) {}

    const uint256& GetHash() const {
        return hash;
    }

    /**
     * Get the total transaction size in bytes, including witness data.
     * "Total Size" defined in BIP141 and BIP144.
     * @return Total transaction size in bytes
     */
    unsigned int GetTotalSize() const;

    friend bool operator==(const Referral& a, const Referral& b)
    {
        return a.hash == b.hash;
    }

    friend bool operator!=(const Referral& a, const Referral& b)
    {
        return a.hash != b.hash;
    }

    std::string ToString() const;
};

/** A mutable version of Referral. */
struct MutableReferral
{
    int32_t version;
    Address parentAddress;
    char addressType;
    Address address;
    valtype signature;

    MutableReferral() : version(Referral::CURRENT_VERSION), addressType{0} { }
    MutableReferral(char addressTypeIn, const Address& addressIn, const Address& parentAddressIn);
    MutableReferral(const Referral& ref);

    template <typename Stream>
    inline void Serialize(Stream& s) const
    {
        SerializeReferral(*this, s);
    }


    template <typename Stream>
    inline void Unserialize(Stream& s)
    {
        UnserializeReferral(*this, s);
    }

    template <typename Stream>
    MutableReferral(deserialize_type, Stream& s) {
        Unserialize(s);
    }

    /**
     * Compute the hash of this MutableReferral. This is computed on the
     * fly, as opposed to GetHash() in Referral, which uses a cached result.
     */
    uint256 GetHash() const;

    friend bool operator==(const MutableReferral& a, const MutableReferral& b)
    {
        return a.GetHash() == b.GetHash();
    }
};

using ReferralRef =  std::shared_ptr<const Referral>;
using ReferralRefs = std::vector<ReferralRef>;

static inline ReferralRef MakeReferralRef(
        char addressTypeIn,
        Address& addressIn,
        Address parentAddressIn)
{
    return std::make_shared<const Referral>(
            addressTypeIn,
            addressIn,
            parentAddressIn);
}

template <typename Ref> static inline ReferralRef MakeReferralRef(Ref&& referralIn)
{
     return std::make_shared<const Referral>(std::forward<Ref>(referralIn));
}

} //namespace referral

#endif // MERIT_PRIMITIVES_REFERRAL_H
