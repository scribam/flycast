#include "reios.h"
#include "hw/sh4/sh4_mem.h"

#include <fstream>

#include <elfio/elfio.hpp>

bool reios_loadElf(const std::string& elf)
{
	/**
	 * TODO: use nowide::fstream and fix the following errors
	 * nowide.lib(cstdio.obj) : error LNK2005: "struct _iobuf * __cdecl nowide::fopen(char const *,char const *)" (?fopen@nowide@@YAPEAU_iobuf@@PEBD0@Z) already defined in winmain.obj [D:\a\flycast\flycast\build\flycast.vcxproj]
     * nowide.lib(cstdio.obj) : error LNK2005: "int __cdecl nowide::remove(char const *)" (?remove@nowide@@YAHPEBD@Z) already defined in winmain.obj [D:\a\flycast\flycast\build\flycast.vcxproj]
     * D:\a\flycast\flycast\build\Release\flycast.exe : fatal error LNK1169: one or more multiply defined symbols found [D:\a\flycast\flycast\build\flycast.vcxproj]
	 */
	std::ifstream ifs(elf, std::ios::in | std::ios::binary);
	if (!ifs) {
		return false;
	}

	ifs.seekg(0, std::ios::end);
	std::size_t size = ifs.tellg();

	if (size == 0 || size > 16_MB) {
		return false;
	}

	ifs.seekg(0, std::ios::beg);

	ELFIO::elfio reader;
	reader.load(ifs);
	if (reader.get_machine() != ELFIO::EM_SH)
		WARN_LOG(REIOS, "Elf file is not for Hitachi SH: machine %d", reader.get_machine());

	for (const auto &segment : reader.segments) {
		if (segment->get_type() != ELFIO::PT_LOAD) {
			DEBUG_LOG(REIOS, "Ignoring section %d type %d", segment->get_index(), segment->get_type());
			continue;
		}
		// Load/initialize that section
		ELFIO::Elf64_Addr dest = segment->get_virtual_address();
		ELFIO::Elf_Xword len = segment->get_file_size();
		ELFIO::Elf_Xword memsize = segment->get_memory_size();
		if (memsize < len) {
			WARN_LOG(REIOS, "Invalid memory size for section %d: %lx", segment->get_index(), (long)memsize);
			continue;
		}
		if (memsize == 0)
			continue;
		u8* ptr = GetMemPtr(dest, memsize);
		if (ptr == nullptr)
		{
			WARN_LOG(REIOS, "Invalid load address or size for section %d: %08lx size %lx", (int)segment->get_index(), (long)dest, (long)memsize);
			continue;
		}
		DEBUG_LOG(REIOS, "Loading section %d to %08lx - %08lx", segment->get_index(), (long)dest, (long)(dest + memsize - 1));
		memcpy(ptr, segment->get_data(), len);
		memset(ptr + len, 0, memsize - len);
	}

	return true;
}
