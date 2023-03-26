#include "lwip/dns.h"
#include "lwip/sockets.h"
#include "DNS.hpp"
#include "../freertospp/timeout.hpp"

class TCP_sock {
	int s = -1;

	TCP_sock(const TCP_sock &other) = delete;
	TCP_sock(TCP_sock &&other) = delete;
	TCP_sock& operator=(const TCP_sock &other) = delete;

public:
	ssize_t write(const uint8_t *data, size_t size) {
		return lwip_write(s, data, size);
	}

	ssize_t write(segment seg) {
		return write(seg.data, seg.size);
	}

	ssize_t read(uint8_t *data, size_t size) {
		return lwip_read(s, data, size);
	}

	ssize_t read_all(uint8_t *data, size_t size, TickType_t ticks = portMAX_DELAY) {
		Timeout tm(ticks);
		ssize_t bytes_read = 0;
		while (bytes_read < size && !tm.done()) {
			set_read_timeout(tm);
			auto r = lwip_read(s, data + bytes_read, size - bytes_read);
			if (r < 0) {
				if (errno == EAGAIN) {
					return bytes_read;
				}
				throw std::runtime_error(strerror(errno));
			}
			bytes_read += r;
		}
		return bytes_read;
	}

	void set_read_timeout(TickType_t ticks) {
		auto ms = ticks == portMAX_DELAY ? 0 : pdTICKS_TO_MS(ticks);
		timeval tv = {
		    .tv_sec = ms / 1000,
		    .tv_usec = (suseconds_t)(ms % 1000) * 1000,
		};
		auto r = setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (r < 0) {
			throw std::runtime_error(strerror(errno));
		}
	}


	TCP_sock& operator=(TCP_sock &&other) {
		if (s > 0) {
			lwip_close(s);
		}
		s = other.s;
		other.s = -1;
		return *this;
	}

	TCP_sock() {}
	TCP_sock(const char *hostname, int port) {
		auto ai = dns_lookup(hostname);
		reinterpret_cast<sockaddr_in*>(ai->ai_addr)->sin_port = htons(port);

		s = lwip_socket(ai->ai_family, SOCK_STREAM, ai->ai_protocol);
		if (s < 0) {
			throw std::runtime_error(strerror(errno));
		}

        if(lwip_connect(s, ai->ai_addr, ai->ai_addrlen) != 0) {
    		lwip_close(s);
        	throw std::runtime_error("Failed to connect");
        }
	}

	~TCP_sock() {
		lwip_close(s);
		s = -1;
	}
};
