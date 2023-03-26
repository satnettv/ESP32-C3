#pragma once
#include <freertos/FreeRTOS.h>
#include "freertos/ringbuf.h"
#include "protocol/Stream_buffer.hpp"

class Ring_buffer {
	RingbufHandle_t _handle;

	Ring_buffer(Ring_buffer &&other) = delete;
	Ring_buffer(const Ring_buffer &other) = delete;
	void operator=(Ring_buffer &&other) = delete;
	void operator=(const Ring_buffer &other) = delete;
public:

	class Writer: public segment {
	    size_t pos = 0;
	public:
		template <class Data> void write(const Data &d) {
			auto dd = (uint8_t*)&d;
			for (size_t s = 0; s < sizeof(Data); s++) {
				data[pos++] = dd[s];
			}
		}
		template <class Size> void write_arr(const uint8_t *_data, Size s) {
			write(s);
			for (Size i = 0; i < s; i++) {
				data[pos++] = _data[i];
			}
		}
		template <class Size> void write_arr(const char *_data, Size s) {
			write_arr((const uint8_t *)_data, s);
		}

		Writer(uint8_t *buf, size_t S): segment(buf, S) {}
	};

	uint32_t count() {
		UBaseType_t count;
		vRingbufferGetInfo(_handle, nullptr, nullptr, nullptr, nullptr, &count);
		return count;
	}
	Ring_buffer(size_t S) {
		assert(S % 4 == 0);
		_handle = xRingbufferCreate(S, RINGBUF_TYPE_NOSPLIT);
		if (!_handle) {
			throw std::runtime_error("xRingbufferCreateStatic failed");
		}
	}
	~Ring_buffer() {
		vRingbufferDelete(_handle);
	}

	class entry: public segment {
		Ring_buffer &ring_buf;
		entry(Ring_buffer &buf): ring_buf(buf) {}
		friend Ring_buffer;
		size_t pos = 0;
	public:
        template <class Data> Data read() {
            Data d{};
            auto dd = (uint8_t*)&d;
            for (size_t s = 0; s < sizeof(Data); s++) {
                dd[s] = data[pos++];
            }
            return d;
        }
        template <class Size> segment read_arr() {
            segment out;
            out.size = read<Size>();
            out.data = &data[pos];
            pos += out.size;
            return out;
        }

		~entry() {
			if (data) {
				vRingbufferReturnItem(ring_buf._handle, data);
				data = nullptr;
			}
		}
	};
	friend entry;

	entry receive(TickType_t ticks = 0) {
		entry out(*this);
		out.size = 0;
		out.data = (uint8_t*)xRingbufferReceive(_handle, &out.size, ticks);
		return out;
	}

	bool send(segment seg, TickType_t ticks = 0) {
		return xRingbufferSend(_handle, seg.data, seg.size, ticks) == pdTRUE;
	}

};
