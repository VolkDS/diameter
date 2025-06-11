#ifndef DIAMETER_MESSAGE_AVP_VALUE_DIAMETER_URI_H
#define DIAMETER_MESSAGE_AVP_VALUE_DIAMETER_URI_H

#include <cassert>
#include <optional>
#include <regex>
#include <string>

namespace diameter::message::avp::value {

/*
 *  The DiameterURI MUST follow the Uniform Resource Identifiers (RFC 3986)
 *  syntax [RFC3986] rules specified below:
 *
 *     "aaa://" FQDN [ port ] [ transport ] [ protocol ]
 *
 *                      ; No transport security
 *
 *      "aaas://" FQDN [ port ] [ transport ] [ protocol ]
 *
 *                      ; Transport security used
 *
 *      FQDN               = < Fully Qualified Domain Name >
 *
 *      port               = ":" 1*DIGIT
 *
 *                      ; One of the ports used to listen for
 *                      ; incoming connections.
 *                      ; If absent, the default Diameter port
 *                      ; (3868) is assumed if no transport
 *                      ; security is used and port 5658 when
 *                      ; transport security (TLS/TCP and DTLS/SCTP)
 *                      ; is used.
 *
 *      transport          = ";transport=" transport-protocol
 *
 *                      ; One of the transports used to listen
 *                      ; for incoming connections.  If absent,
 *                      ; the default protocol is assumed to be TCP.
 *                      ; UDP MUST NOT be used when the aaa-protocol
 *                      ; field is set to diameter.
 *
 *      transport-protocol = ( "tcp" / "sctp" / "udp" )
 *
 *      protocol           = ";protocol=" aaa-protocol
 *
 *                      ; If absent, the default AAA protocol
 *                      ; is Diameter.
 *
 *      aaa-protocol       = ( "diameter" / "radius" / "tacacs+" )
 *
 *      The following are examples of valid Diameter host identities:
 *      aaa://host.example.com;transport=tcp
 *      aaa://host.example.com:6666;transport=tcp
 *      aaa://host.example.com;protocol=diameter
 *      aaa://host.example.com:6666;protocol=diameter
 *      aaa://host.example.com:6666;transport=tcp;protocol=diameter
 *      aaa://host.example.com:1813;transport=udp;protocol=radius
 */
class DiameterURIBase
{
public:
    using value_type = std::string;
    using scheme_type = std::string;
    using fqdn_type = std::string;
    using port_type = std::optional<std::uint32_t>;
    using transport_type = std::optional<std::string>;
    using protocol_type = std::optional<std::string>;

    DiameterURIBase() = default;

    DiameterURIBase(DiameterURIBase const&) = default;
    DiameterURIBase& operator= (DiameterURIBase const&) = default;
    DiameterURIBase(DiameterURIBase&&) = default;
    DiameterURIBase& operator= (DiameterURIBase&&) = default;

    DiameterURIBase(const value_type& val)
        : m_value(val)
    {
    }

    value_type value() const noexcept
    {
        return m_value;
    }

    std::size_t size() const noexcept
    {
        return m_value.size();
    }

    bool validate()
    {
        static const std::regex r1(
            R"(^(aaa|aaas):\/\/([a-zA-Z0-9\-._~%!$&'()*+,=]+)(?::([0-9]+))?(?:;transport=(tcp|sctp|udp))?(?:;protocol=(diameter|radius|tacacs\+))?$)",
            std::regex_constants::icase);
        // group1: scheme
        // group2: FQDN
        // group3: port
        // group4: transport
        // group5: protocol

        std::smatch match;
        if (!std::regex_match(m_value, match, r1)) {
            return false;
        }
        for (std::size_t i = 0; i < match.size(); ++i) {
            if (!match[i].matched)
                continue;
            std::string piece = match[i].str();
            switch (i) {
                case 1:
                    m_scheme = piece;
                    break;
                case 2:
                    m_fqdn = piece;
                    break;
                case 3:
                    m_port = std::atoi(piece.c_str());
                    break;
                case 4:
                    m_transport = piece;
                    break;
                case 5:
                    m_protocol = piece;
                    break;
                default:
                    break;
            }
        }

        static const std::regex r2(
            R"((?=^.{4,253}$)(^((?!-)[a-zA-Z0-9-]{0,62}[a-zA-Z0-9]\.)+[a-zA-Z]{2,63}$))");
        m_is_validated = std::regex_match(m_fqdn, r2);
        return m_is_validated;
    }

    scheme_type scheme() const
    {
        // assert(m_is_validated);
        return m_scheme;
    }

    fqdn_type fqdn() const
    {
        // assert(m_is_validated);
        return m_fqdn;
    }

    port_type port() const
    {
        // assert(m_is_validated);
        return m_port;
    }

    transport_type transport() const
    {
        // assert(m_is_validated);
        return m_transport;
    }

    protocol_type protocol() const
    {
        // assert(m_is_validated);
        return m_protocol;
    }

private:
    std::vector<std::string> split(const value_type& val, char delim)
    {
        std::istringstream iss(val);
        std::string item;
        std::vector<std::string> result;
        while (std::getline(iss, item, delim)) {
            result.push_back(item);
        }
        return result;
    }

    value_type m_value;

    bool m_is_validated {false};
    scheme_type m_scheme;
    fqdn_type m_fqdn;
    port_type m_port;
    transport_type m_transport;
    protocol_type m_protocol;
};

} // namespace diameter::message::avp::value

#endif
