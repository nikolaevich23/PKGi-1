#include "Pkg.h"

#include <fcntl.h>
#include <unistd.h>
#include <orbis/libkernel.h>

#include "Utility.h"
#include "Common.h"

Pkg::Pkg() {
	memset(&header, 0, sizeof(pkg_header));
	entries_nbr = 0;
	entries_table = NULL;
}

Pkg::~Pkg() {
	if (entries_table) {
		free(entries_table);
		entries_table = NULL;
	}
}

bool Pkg::LoadHeader(void* data, size_t size) {
	if (!data) {
		printf("LoadHeader: Data pointer is empty (%p)\n", data);
		return false;
	}

	if (size != sizeof(struct pkg_header)) {
		printf("LoadHeader: Size mismatch (%lu bytes)\n", size);
		return false;
	}
	
	memcpy(&header, data, sizeof(struct pkg_header));
	return isValidMagic();
}

bool Pkg::LoadEntries(void* data, size_t count) {
	if (!data || count <= 0) {
		printf("LoadHeader: Data pointer is empty (%p) or count is bad. (%lu bytes)\n", data, count);
		return false;
	}

	size_t size = count * sizeof(struct pkg_table_entry);
	void* entry_m = malloc(size);
	memcpy(entry_m, data, size);
	entries_table = (struct pkg_table_entry*)entry_m;
	entries_nbr = (int)count;

	return true;
}

bool Pkg::FindEntries(pkg_entry_id id, uint32_t* offset, size_t* size) {
	if (!offset || !size) {
		printf("FindEntries: Bad argument !\n");
		return false;
	}

	if (!entries_table || entries_nbr == 0)  {
		printf("FindEntries: Entry table not initialized or empty ! (%p => %i)\n", entries_table, entries_nbr);
		return false;
	}

	for (int i = 0; i < entries_nbr; ++i) {
		if (BE32(entries_table[i].id) == id) {
			*offset = BE32(entries_table[i].offset);
			*size = BE32(entries_table[i].size);
			return true;
			break;
		}
	}

	printf("FindEntries: Not found (%i)\n", id);
	*offset = 0;
	*size = 0;
	return false;
}

void Pkg::getEntriesPosition(uint32_t* entries_offset, size_t* entries_count) {
	if (!entries_offset || !entries_count)
		return;

	size_t entry_count = BE32(header.entry_count);
	uint32_t entry_table_offset = BE32(header.entry_table_offset);

	*entries_offset = entry_table_offset;
	*entries_count = entry_count;
}

bool Pkg::isValidMagic() {
	static const uint8_t magic[] = { '\x7F', 'C', 'N', 'T' };
	if (memcmp(header.magic, magic, sizeof(magic)) != 0) {
		return false;
	}

	return true;
}

bool Pkg::isPatch() {
	unsigned int flags;

	flags = BE32(header.content_flags);

	if (flags & PKG_CONTENT_FLAGS_FIRST_PATCH) {
		return true;
	}
	if (flags & PKG_CONTENT_FLAGS_SUBSEQUENT_PATCH) {
		return true;
	}
	if (flags & PKG_CONTENT_FLAGS_DELTA_PATCH) {
		return true;
	}
	if (flags & PKG_CONTENT_FLAGS_CUMULATIVE_PATCH) {
		return true;
	}

	return false;
}

size_t Pkg::getPackageSize() {
	return BE64(header.package_size);
}

uint32_t Pkg::getContentType() {
	return BE32(header.content_type);
}
bool Pkg::getPackageDigest(char* buf, size_t buf_size) {
	return Utility::ByteToHex(buf, buf_size, header.digest, sizeof(header.digest));
}

bool Pkg::getPieceDigest(char* buf, size_t buf_size) {
	static uint8_t s_zero_mini_digest[PKG_MINI_DIGEST_SIZE] = { 0 };
	return Utility::ByteToHex(buf, buf_size, s_zero_mini_digest, sizeof(s_zero_mini_digest));
}