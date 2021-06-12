#include "reios.h"
#include "hw/sh4/sh4_mem.h"

#include <elfio/elfio.hpp>
#include <nowide/fstream.hpp>

bool reios_loadElf(const std::string& elf)
{
	nowide::ifstream ifs(elf, std::ios::in | std::ios::binary);
	if (!ifs) {
		return false;
	}

	ifs.seekg(0, std::ios::end);
	if (ifs.tellg() > 16 * 1024 * 1024) {
		return false;
	}

	ifs.seekg(0, std::ios::beg);

	ELFIO::elfio reader;
	reader.load(ifs);

	ELFIO::Elf_Half seg_num = reader.segments.size();
	for (int i = 0; i < seg_num; i++) {
		const ELFIO::segment* pseg = reader.segments[i];

		ELFIO::Elf64_Addr dest = pseg->get_virtual_address();
		ELFIO::Elf_Xword len = pseg->get_file_size();

		u8* ptr = GetMemPtr(dest, len);
		if (ptr == NULL) {
			WARN_LOG(REIOS, "Invalid load address for section %d: %08lx", i, dest);
			continue;
		}
		DEBUG_LOG(REIOS, "Loading section %d to %08lx - %08lx", i, dest, dest + len - 1);
		memcpy(ptr, pseg->get_data(), len);
		ptr += len;
		memset(ptr, 0, pseg->get_memory_size() - len);
	}

	return true;
}
