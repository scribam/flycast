#include "reios.h"
#include "hw/sh4/sh4_mem.h"

#include <fstream>

#include <elfio/elfio.hpp>

// True if a PT_LOAD segment already copied [addr, addr + size) in. Tested
// against file size, not memory size: the gap between them is only zero-filled.
static bool isLoadedBySegment(const ELFIO::elfio& reader, uint64_t addr, size_t size)
{
	for (const auto& segment : reader.segments)
	{
		if (segment->get_type() != ELFIO::PT_LOAD)
			continue;
		ELFIO::Elf64_Addr start = segment->get_virtual_address();
		ELFIO::Elf_Xword end = start + segment->get_file_size();
		if (addr >= start && addr + size <= end)
			return true;
	}
	return false;
}

bool reios_loadElf(const std::string& elf) {
	/**
	 * TODO: use nowide::fstream and fix the following errors
	 * nowide.lib(cstdio.obj) : error LNK2005: "struct _iobuf * __cdecl nowide::fopen(char const *,char const *)" (?fopen@nowide@@YAPEAU_iobuf@@PEBD0@Z) already defined in winmain.obj [D:\a\flycast\flycast\build\flycast.vcxproj]
     * nowide.lib(cstdio.obj) : error LNK2005: "int __cdecl nowide::remove(char const *)" (?remove@nowide@@YAHPEBD@Z) already defined in winmain.obj [D:\a\flycast\flycast\build\flycast.vcxproj]
     * D:\a\flycast\flycast\build\Release\flycast.exe : fatal error LNK1169: one or more multiply defined symbols found [D:\a\flycast\flycast\build\flycast.vcxproj]
	 */
	std::ifstream ifs(elf, std::ios::in | std::ios::binary);
	if (!ifs)
		return false;

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

	unsigned loaded = 0;

	for (const auto &segment : reader.segments)
	{
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
		loaded++;
	}

	// Sections added after linking (objcopy --add-section) have no program
	// header. Load those the loop above missed, after it so its zero-fill
	// doesn't clobber them.
	for (const auto& section : reader.sections)
	{
		if ((section->get_flags() & ELFIO::SHF_ALLOC) == 0
				|| section->get_type() == ELFIO::SHT_NOBITS)
			continue;

		ELFIO::Elf64_Addr dest = section->get_address();
		ELFIO::Elf_Xword len = section->get_size();
		if (dest == 0 || len == 0 || isLoadedBySegment(reader, dest, len))
			continue;
		ELFIO::Elf64_Off offset = section->get_offset();
		if (offset + len > size)
		{
			WARN_LOG(REIOS, "Section %s extends past end of file", section->get_name().c_str());
			continue;
		}
		u8* ptr = GetMemPtr(dest, len);
		if (ptr == nullptr)
		{
			WARN_LOG(REIOS, "Invalid load address or size for section %s: %08lx size %lx",
			         section->get_name().c_str(), (long)dest, (long)len);
			continue;
		}
		DEBUG_LOG(REIOS, "Loading section %s to %08lx - %08lx",
		          section->get_name().c_str(), (long)dest, (long)(dest + len - 1));
		memcpy(ptr, section->get_data(), len);
		loaded++;
	}

	if (loaded == 0) {
		WARN_LOG(REIOS, "Elf file has nothing to load");
		return false;
	}

	return true;
}
