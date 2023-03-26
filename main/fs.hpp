#pragma once
#include "esp_vfs_fat.h"
#include "stdio.h"
#include "dirent.h"
#include <memory>
#include <vector>

struct FILE_RAII {
	FILE *f;
	FILE_RAII(const char *name, const char *mode) {
		f = fopen(name, mode);
		if (!f) {
			perror("FILE_RAII fopen");
		}
	}
	~FILE_RAII() {
		if (f) {
			fclose(f);
		}
	}
	void write(const uint8_t *buf, size_t len) {
		if (fwrite(buf, 1, len, f) != len) {
			throw std::runtime_error((std::string)"file write failed; " + strerror(errno));
		}
	}
	template<class... Types>
	void printf(Types... args) {
		if (fprintf(f, args...) < 0) {
			throw std::runtime_error((std::string)"file write failed; " + strerror(errno));
		}
	}
	std::vector<uint8_t> read_vec() {
		std::vector<uint8_t> out;
		auto sz = size();
		out.resize(sz);
		fread(&out[0], 1, sz, f);
		return out;
	}
	std::string read_str() {
		std::string out;
		auto sz = size();
		out.resize(sz);
		fread((char*)out.data(), 1, sz, f);
		return out;
	}

	bool eof() {
		return feof(f) != 0;
	}

	size_t read(uint8_t *buf, size_t buf_size) {
		return fread(buf, 1, buf_size, f);
	}

	size_t read(char *buf, size_t buf_size) {
		return fread(buf, 1, buf_size, f);
	}

	int error() {
		return ferror(f);
	}

	int32_t tell() {
		return ftell(f);
	}

	void seek(int32_t pos) {
		if (fseek(f, pos, SEEK_SET) < 0) {
			perror("seek");
		}
	}

	operator bool() {
		return f != nullptr;
	}
	int32_t size() {
		auto pos = ftell(f);
		fseek(f, 0L, SEEK_END);
		auto sz = ftell(f);
		fseek(f, pos, SEEK_SET);
		return sz;
	}
	void rewind() {
		std::rewind(f);
	}
};

struct DIR_RAII {
	DIR *d;
	DIR_RAII(const char *path) {
		d = opendir(path);
		if (!d) {
			perror("DIR_RAII fopen");
		}
	}
	~DIR_RAII() {
		if (d) {
			closedir(d);
		}
	}
	dirent *read() {
		return readdir(d);
	}
	void rewind() {
		rewinddir(d);
	}
	operator bool() {
		return d != nullptr;
	}
};

class FS {
	wl_handle_t s_wl_handle = WL_INVALID_HANDLE;
public:
	void setup() {
	    const esp_vfs_fat_mount_config_t mount_config = {
	        .format_if_mount_failed = true,
	    	.max_files = 4,
	        .allocation_unit_size = 4096,
			.disk_status_check_enable = true
	    };
	    ESP_ERROR_CHECK(esp_vfs_fat_spiflash_mount_rw_wl("/fs", "fs", &mount_config, &s_wl_handle));
	}
};

