#ifndef DIAMETER_MESSAGE_AVP_VALUE_DIAMETER_IDENTITY_H
#define DIAMETER_MESSAGE_AVP_VALUE_DIAMETER_IDENTITY_H

#include <regex>
#include <string>

namespace diameter::message::avp::value {

/*
 * DiameterIdentity
 *
 * The DiameterIdentity format is derived from the OctetString Basic AVP Format.
 *
 * DiameterIdentity  = FQDN/Realm
 *
 * The DiameterIdentity value is used to uniquely identify either:
 *
 * - A Diameter node for purposes of duplicate connection and routing loop detection.
 * - A Realm to determine whether messages can be satisfied locally or whether they must be routed
 *   or redirected.
 *
 * When a DiameterIdentity value is used to identify a Diameter node, the contents of the string
 * MUST be the Fully Qualified Domain Name (FQDN) of the Diameter node. If multiple Diameter nodes
 * run on the same host, each Diameter node MUST be assigned a unique DiameterIdentity. If a
 * Diameter node can be identified by several FQDNs, a single FQDN should be picked at startup and
 * used as the only DiameterIdentity for that node, whatever the connection on which it is sent. In
 * this document, note that DiameterIdentity is in ASCII form in order to be compatible with
 * existing DNS infrastructure. See Appendix D for interactions between the Diameter protocol and
 * Internationalized Domain Names (IDNs).
 */
class DiameterIdentityBase
{
public:
    using value_type = std::string;

    DiameterIdentityBase() = default;

    DiameterIdentityBase(DiameterIdentityBase const&) = default;
    DiameterIdentityBase& operator= (DiameterIdentityBase const&) = default;
    DiameterIdentityBase(DiameterIdentityBase&&) = default;
    DiameterIdentityBase& operator= (DiameterIdentityBase&&) = default;

    DiameterIdentityBase(const value_type& val)
        : m_value(val)
    {
    }

    value_type value() const
    {
        return m_value;
    }

    std::size_t size() const noexcept
    {
        return m_value.size();
    }

    bool validate() const
    {
        // Validation for FQDN format
        // https://stackoverflow.com/questions/11809631/fully-qualified-domain-name-validation
        static const std::regex r(
            R"((?=^.{4,253}$)(^((?!-)[a-zA-Z0-9-]{0,62}[a-zA-Z0-9]\.)+[a-zA-Z]{2,63}$))",
            std::regex_constants::icase);
        return std::regex_match(m_value, r);
    }

private:
    value_type m_value;
};

}

#endif
