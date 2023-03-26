#pragma once
#include <memory>
#include "lwip/netdb.h"

inline  std::shared_ptr<addrinfo> dns_lookup(const char *addr, const char *port = nullptr) {
	addrinfo *addr_list;

	if (lwip_getaddrinfo(addr, port, nullptr, &addr_list) != 0) {
		throw std::runtime_error("dns lookup failed");
	}

	return std::shared_ptr<addrinfo>(addr_list, lwip_freeaddrinfo);
}

inline std::string get_ip_str(const struct sockaddr *sa) {
	char buf[50];
    switch(sa->sa_family) {
        case AF_INET: {
        	auto sa_in = (struct sockaddr_in *)sa;
            inet_ntop(AF_INET, &(sa_in->sin_addr), buf, 50);
            return std::string(buf) + ':' + std::to_string(ntohs(sa_in->sin_port));
        }
        case AF_INET6: {
        	auto sa_in6 = (struct sockaddr_in6 *)sa;
            inet_ntop(AF_INET6, &(sa_in6->sin6_addr), buf, 50);
        	return std::string(buf) + ':' + std::to_string(ntohs(sa_in6->sin6_port));
        }
        default:
            return "Unknown AF";
    }
}

inline void sockaddr2ip_addr(ip_addr *ip, const std::shared_ptr<addrinfo> &sa) {
	switch(sa->ai_family) {
		case AF_INET: {
		    struct in_addr &addr4 = ((struct sockaddr_in *) (sa->ai_addr))->sin_addr;
		    inet_addr_to_ip4addr(ip_2_ip4(ip), &addr4);
		    IP_SET_TYPE_VAL(*ip, IPADDR_TYPE_V4);
			break;
		}
		case AF_INET6: {
		    struct in6_addr &addr6 = ((struct sockaddr_in6 *) (sa->ai_addr))->sin6_addr;
		    inet6_addr_to_ip6addr(ip_2_ip6(ip), &addr6);
		    IP_SET_TYPE_VAL(*ip, IPADDR_TYPE_V6);
			break;
		}
		default:
			throw std::runtime_error("Unknown AF");
	}
}
