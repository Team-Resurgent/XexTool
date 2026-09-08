// 
// XexIdcCreator.cpp
// 
// dumps idc info for basefile to a file
// 

#include "XexIdcCreator.h"
#include "Xex.h"
#include "XexEndian.h"
#include "PEParser.h"

XexIdcCreator::XexIdcCreator()
{
}
XexIdcCreator::~XexIdcCreator()
{
}


// dumps the basefile info to a file
bool XexIdcCreator::dump(const Xex& xex, const char* filename)
{
	m_pXex = &xex;
	
	if(filename == NULL) return false;
	if( !m_pXex->isBasefilePE() ) return false;
	
	FILE* fd = fopen(filename, "w+t");
	if(fd == NULL)
		return false;
	bool result = dump(xex, fd);
	fclose(fd);
	return result;
}

bool XexIdcCreator::dump(const Xex& xex, FILE* fd)
{
	m_pXex = &xex;
	
	if(fd == NULL) return false;
	if( !m_pXex->isBasefilePE() ) return false;
	
	if( fseek(fd, 0, SEEK_SET) != 0 ) return false;
	DataBlock basefile;
	m_pXex->getBasefile(basefile);
	u32 load_addr = m_pXex->getLoadAddress();
	s32 imports_per_func = 0x100;
	PEParser pe_parser(basefile);
	
	// header
	fprintf(fd,
		"//\n"
		"// Xbox360 Basefile Info - Created by XexTool\n"
		"//\n"
		"\n"
		"#include <idc.idc>\n"
		"#include <x360_imports.idc>\n"
		"\n"
		"\n"
	);
	
	// make name (forced to work by adding an extension)
	fprintf(fd,
		"static MakeNameForce(addr, name)\n"
		"{\n"
		"    auto num, name_fixed;\n"

		"    if( MakeNameEx(addr, name, SN_NOWARN) )\n"
		"        return;\n"
		"    for(num=0; num<999; num++)\n"
		"    {\n"
		"        name_fixed = form(\"%%s_%%d\", name, num);\n"
		"        if( MakeNameEx(addr, name_fixed, SN_NOWARN) )\n"
		"            return;\n"
		"    }\n"
		"}\n"
		"\n"
	);

	// get section address from name
	fprintf(fd,
		"static GetSectionAddr(sectName)\n"
		"{\n"
		"	auto seg_addr, seg_base;\n"
		"	seg_base = SegByName(sectName);\n"
		"	return SegByBase(seg_base);\n"
		"}\n"
		"\n"
	);

	// set up sections
	fprintf(fd,
		"static SetupSection(startAddr, endAddr, segClass, perms, name, base)\n"
		"{\n"
		"    SetSelector(base, 0);\n"
		"    SegCreate(startAddr, endAddr, base, 1, 3, 2);\n"
		"    SegClass(startAddr, segClass);\n"
		"    SegRename(startAddr, name);\n"
		"    SetSegmentAttr(startAddr, SEGATTR_PERM, perms); // 4=read, 2=write, 1=execute\n"
		"    SetSegmentAttr(startAddr, SEGATTR_FLAGS, 0x10); // SFL_LOADER\n"
		"    SegDefReg(startAddr, \"%%r26\", 0);\n"
		"    SegDefReg(startAddr, \"%%r27\", 0);\n"
		"    SegDefReg(startAddr, \"%%r28\", 0);\n"
		"    SegDefReg(startAddr, \"%%r29\", 0);\n"
		"    SegDefReg(startAddr, \"%%r30\", 0);\n"
		"    SegDefReg(startAddr, \"%%r31\", 0);\n"
		"}\n"
		"\n"
		"static SetupSections()\n"
		"{\n"
		"    auto addr;\n\n"
	);
	int section_base = 1;
	for(s32 sec_num=0; sec_num<pe_parser.getNumSections(); sec_num++)
	{
		char name[12];
		u32 addr, size, flags;
		pe_parser.getSectionName(sec_num, name);
		pe_parser.getSectionAddr(sec_num, addr);
		pe_parser.getSectionSize(sec_num, size);
		pe_parser.getSectionFlags(sec_num, flags);
		addr += load_addr;
		
		// make sure section isnt inside a resource
		bool create_section = true;
		for(s32 res_idx=0; res_idx<m_pXex->numResources(); res_idx++)
		{
			u32 res_addr;
			s32 res_size;
			char res_name[32];
			m_pXex->getResource(res_idx, res_addr, res_size, res_name);
			
			if(addr>=res_addr && addr<res_addr+res_size)
			{
				create_section = false;
				break;
			}
		}
		if(create_section)
		{
			int perms = 0;
			if(flags & 0x20000000) perms |= 1;	// executable
			if(flags & 0x80000000) perms |= 2;	// writeable
			if(flags & 0x40000000) perms |= 4;	// readable
			fprintf(fd,
				"    SetupSection(0x%08X, 0x%08X, \"%s\", %d, \"%s\", %d);\n",
				addr, addr+size, ((flags & 0x20000000)?"CODE":"DATA"), perms, name, section_base
			);
			section_base++;
		}
	}
	fprintf(fd,
		"\n"
		"    // remove unused \"leftovers\" of the original binary segment\n"
		"    while( (addr = SegByBase(0)) != BADADDR )\n"
		"        DelSeg(addr, SEGMOD_KILL|SEGMOD_SILENT);\n"
		"}\n"
		"\n"
	);
	
	// set up resources
	fprintf(fd,
		"static SetupResources()\n"
		"{\n"
	);
	for(s32 res_idx=0; res_idx<m_pXex->numResources(); res_idx++)
	{
		u32 addr;
		s32 size;
		char name[32];
		m_pXex->getResource(res_idx, addr, size, name);
		fprintf(fd,
			"    SetupSection(0x%08X, 0x%08X, \"DATA\", 4, \"%s\", %d);\n",
			addr, addr+size, name, section_base
		);
		section_base++;
	}
	fprintf(fd,
		"}\n"
		"\n"
	);
	
	// remove empty sections
	fprintf(fd,
		"static RemoveEmptySections()\n"
		"{\n"
		"    auto seg_addr, seg_num;\n"
		"    for(seg_num=0; seg_num<500; seg_num=seg_num+1)\n"
		"    {\n"
		"        seg_addr = GetSectionAddr(form( \"seg%%03d\", seg_num) );\n"
		"        if(seg_addr != -1)\n"
		"            SegDelete(seg_addr, 1);\n"
		"    }\n"
		"}\n"
		"\n"
		"\n"
	);
	

	// setup import funcs
	fprintf(fd,
		"static SetupImportFunc(importAddr, funcAddr, importNum, name)\n"
		"{\n"
		"    auto func_name;\n"
		"    func_name = DoNameGen(name, 0, importNum);\n"
		"\n"
		"    MakeNameForce(importAddr, \"__imp__\" + func_name);\n"
		"    MakeDword(importAddr);\n"
		"\n"
		"    PatchWord(funcAddr, 0x3860);\n"
		"    PatchWord(funcAddr + 4, 0x3880);\n"
		"    MakeUnknown(funcAddr, 0x10, 0); // DOUNK_SIMPLE\n"
		"    MakeCode(funcAddr);\n"
		"    MakeNameForce(funcAddr, func_name);\n"
		"    MakeFunction(funcAddr, funcAddr + 0x10);\n"
		"    SetFunctionFlags(funcAddr, FUNC_LIB);\n"
		"}\n"
		"\n"
	);
	fprintf(fd,
		"static SetupImportData(importAddr, importNum, name)\n"
		"{\n"
		"    auto data_name;\n"
		"    data_name = DoNameGen(name, 0, importNum);\n"
		"\n"
		"    MakeNameForce(importAddr, data_name);\n"
		"    MakeDword(importAddr);\n"
		"}\n"
		"\n"
	);
	// setup imports for each library
	for(s32 lib_num=0; lib_num<m_pXex->numImportLibraries(); lib_num++)
	{
		char lib_name[64];
		XexVersion32 version;
		XexVersion32 min_version;
		DataBlock addresses;
		u32 module_number;
		u8  module_index;
		if( m_pXex->getImportLibrary(lib_num, lib_name, version, min_version, addresses, module_number, module_index) )
		{
			// get number of imports and fix lib name
			s32 num_imports = addresses.size() / 4;
			char fixed_lib_name[64];
			strcpy(fixed_lib_name, lib_name);
			for(char* str_ptr=fixed_lib_name; *str_ptr; str_ptr++)
			{
				if(*str_ptr == '.')
					*str_ptr = '_';
			}
			
			// setup each import
			// these are split into 0x100 per function since
			// idc scripts cant handle functions that are too big
			for(s32 import_num=0; import_num<num_imports; import_num+=imports_per_func)
			{
				fprintf(fd,	
					"static setupImports_%d_%s_%d()\n"
					"{\n",
					lib_num, fixed_lib_name, import_num/imports_per_func
				);
				
				// handle imports inside one idc function
				for(s32 func_import_num=0; func_import_num<imports_per_func; func_import_num++)
				{
					if(import_num + func_import_num >= num_imports)
						break;
					
					u32 this_import_addr = addresses.get32((import_num + func_import_num) * 4);
					s32 this_import_type = basefile.get8(   0 + this_import_addr - load_addr);
					s32 this_import_num  = basefile.get16be(2 + this_import_addr - load_addr);
					
					u32 next_import_addr = 0;
					s32 next_import_type = 0;
					s32 next_import_num  = 0;
					if( func_import_num+1 < imports_per_func )
					{
						next_import_addr = addresses.get32((import_num + func_import_num+1) * 4);
						next_import_type = basefile.get8(   0 + next_import_addr - load_addr);
						next_import_num  = basefile.get16be(2 + next_import_addr - load_addr);
					}
					
					if(next_import_type != 0)
					{
						// function
						fprintf(fd, 
							"    SetupImportFunc(0x%08X, 0x%08X, 0x%03X, \"%s\");\n",
							this_import_addr, next_import_addr, this_import_num, lib_name
						);
						func_import_num++;
					}
					else
					{
						// some kind of reference (eg data import)
						fprintf(fd, 
							"    SetupImportData(0x%08X,             0x%03X, \"%s\");\n",
							this_import_addr, this_import_num, lib_name
						);
					}
				}
				
				fprintf(fd,	
					"}\n"
					"\n"
				);
			}
		}
	}
	
	// main import setup function
	fprintf(fd,
		"static SetupImports()\n"
		"{\n"
	);
	for(s32 lib_num=0; lib_num<m_pXex->numImportLibraries(); lib_num++)
	{
		char lib_name[64];
		XexVersion32 version;
		XexVersion32 min_version;
		DataBlock addresses;
		u32 module_number;
		u8  module_index;
		if( m_pXex->getImportLibrary(lib_num, lib_name, version, min_version, addresses, module_number, module_index) )
		{
			s32 num_imports = addresses.size() / 4;
			for(char* str_ptr=lib_name; *str_ptr; str_ptr++)
			{
				if(*str_ptr == '.')
					*str_ptr = '_';
			}
			for(s32 import_num=0; import_num<num_imports; import_num+=imports_per_func)
			{
				fprintf(fd,
					"    setupImports_%d_%s_%d();\n",
					lib_num, lib_name, import_num/imports_per_func
				);
			}
		}
	}
	fprintf(fd,	
		"}\n"
		"\n"
		"\n"
	);
	
	
	// setup export funcs
	fprintf(fd,
		"static SetupExportFunc(funcAddr, exportNum, funcName)\n"
		"{\n"
		"    MakeUnkn(funcAddr, 0);\n"
		"    MakeCode(funcAddr); \n"
		"    MakeNameForce(funcAddr, funcName);\n"
		"    MakeFunction(funcAddr, BADADDR);\n"
		"    AddEntryPoint(exportNum, funcAddr, funcName, 1);\n"
		"}\n"
		"\n"
	);
	fprintf(fd,
		"static SetupExportData(dataAddr, exportNum, name)\n"
		"{\n"
		"    auto data_name;\n"
		"    data_name = DoNameGen(name, 0, exportNum);\n"
		"\n"
		"    AddEntryPoint(exportNum, dataAddr, data_name, 0);\n"
		"    MakeNameForce(dataAddr, data_name);\n"
		"    MakeDword(dataAddr);\n"
		"}\n"
		"\n"
	);
	// main export setup function
	fprintf(fd,
		"static SetupExports()\n"
		"{\n"
		"    auto name;\n"
		"    name = GetInputFile();\n"
		"\n"
	);
	u32 export_table_addr = m_pXex->getExportTableAddress();
	if(export_table_addr)
	{
		u32 export_table_offset = export_table_addr - load_addr;
		s32 num_exports = basefile.get32be(export_table_offset+offsetof(XexHvExportTable, count));
		DataBlock export_block;
		export_block.set(basefile, export_table_offset, 0, sizeof(XexHvExportTable) + (num_exports-1)*4);
		u32 image_base_addr = export_block.get32be(offsetof(XexHvExportTable, imageBaseAddress)) << 16;
		u32 ordinal_base_num = export_block.get32be(offsetof(XexHvExportTable, base));
		for(s32 exp_num=0; exp_num<num_exports; exp_num++)
		{
			u32 func_addr = export_block.get32be(offsetof(XexHvExportTable, funcAddrs) + exp_num*4) + image_base_addr;
			u32 func_num = ordinal_base_num + exp_num;
//			fprintf(fd, 
//				"    MakeCode(0x%08X); MakeNameForce(0x%08X, \"export_%d\"); MakeFunction(0x%08X, BADADDR);\n",
//				func_addr, func_addr, func_num, func_addr);
			if(func_addr != image_base_addr)
			{
				fprintf(fd,
					"    SetupExportFunc(0x%08X, 0x%03X, DoNameGen(name, 0, 0x%03X));\n",
					func_addr, func_num, func_num);
			}
		}
	}
	if( m_pXex->hasEntryPoint() )
	{
		fprintf(fd,	
			"\n"
			"    // set start entry point\n"
			"    SetupExportFunc(0x%08X, 0x%08X, \"start\");\n"
			, m_pXex->getEntryPoint(), m_pXex->getEntryPoint()
		);
	}
	fprintf(fd,	
		"}\n"
		"\n"
	);
	
	// main export by name setup function
	fprintf(fd,
		"static SetupExportsByName()\n"
		"{\n"
	);
	if(m_pXex->hasExportsByName())
	{
		ExportsByName exports_info;
		m_pXex->getExportsByName(exports_info);
		u32 exports_offset = exports_info.exportTableOffset;
		u32 exports_size = exports_info.exportTableSize;
		DataBlock exports_by_name;
		exports_by_name.set(basefile, exports_offset, 0, exports_size);
		u32 ordinal_base_num = exports_by_name.get32(offsetof(ExportByNameTable, base));
		s32 num_exports = exports_by_name.get32(offsetof(ExportByNameTable, numberOfFunctions));
		s32 num_export_names = exports_by_name.get32(offsetof(ExportByNameTable, numberOfNames));
		u32 func_addr_offset = exports_by_name.get32(offsetof(ExportByNameTable, addressOfFunctions));
		u32 func_ord_offset = exports_by_name.get32(offsetof(ExportByNameTable, addressOfNameOrdinals));
		u32 func_name_offset = exports_by_name.get32(offsetof(ExportByNameTable, addressOfNames));
		u32 module_name_offset = exports_by_name.get32(offsetof(ExportByNameTable, name)) - exports_offset;
		for(s32 exp_num=0; exp_num<num_export_names; exp_num++)
		{
			u32 func_addr = exports_by_name.get32(func_addr_offset + exp_num*4) + load_addr;
			u32 func_num = exports_by_name.get16(func_ord_offset + exp_num*2) + ordinal_base_num;
			char func_name[256];
			sprintf(func_name, "export_by_name_%d", func_num);
			if(exp_num < num_export_names)
			{
				u32 func_name_ptr = exports_by_name.get32(func_name_offset + exp_num*4);
				for(int func_name_offset=0; func_name_offset<256; func_name_offset++)
				{
					exports_by_name.get(func_name+func_name_offset, func_name_ptr+func_name_offset, 1);
					if(*(func_name+func_name_offset) == 0)
						break;
				}
			}
//			fprintf(fd, 
//				"    MakeCode(0x%08X); MakeNameForce(0x%08X, \"%s\"); MakeFunction(0x%08X, BADADDR);\n",
//				func_addr, func_addr, func_name, func_addr);
			fprintf(fd,
				"    SetupExportFunc(0x%08X, 0x%03X, \"%s\");\n",
				func_addr, func_num, func_name);
		}
	}
	fprintf(fd,	
		"}\n"
		"\n"
	);
	
	// setup register save/load functions
	fprintf(fd,
		"static SetupRegSaves()\n"
		"{\n"
		"	auto currAddr, i;\n"
		"	\n"
		"	// find all saves of gp regs\n"
		"	for(currAddr=0; currAddr != BADADDR; currAddr=currAddr+4)\n"
		"	{\n"
		"		// find \"std %%r14, -0x98(%%sp)\" followed by \"std %%r15, -0x90(%%sp)\"\n"
		"		currAddr = FindBinary(currAddr, SEARCH_DOWN, \"F9 C1 FF 68 F9 E1 FF 70\");\n"
		"		if(currAddr == BADADDR)\n"
		"			break;\n"
		"		for(i=14; i<=31; i++)\n"
		"		{\n"
		"			MakeUnknown(currAddr, 4, 0); // DOUNK_SIMPLE\n"
		"			MakeCode(currAddr);\n"
		"			if(i != 31)\n"
		"				MakeFunction(currAddr, currAddr + 4);\n"
		"			else\n"
		"				MakeFunction(currAddr, currAddr + 0x0C);\n"
		"			MakeNameForce(currAddr, form(\"__savegprlr_%%d\", i));\n"
		"			currAddr = currAddr + 4;\n"
		"		}\n"
		"	}\n"
		"	\n"
		"	// find all loads of gp regs\n"
		"	for(currAddr=0; currAddr != BADADDR; currAddr=currAddr+4)\n"
		"	{\n"
		"		// find \"ld  %%r14, var_98(%%sp)\" followed by \"ld  %%r15, var_90(%%sp)\"\n"
		"		currAddr = FindBinary(currAddr, SEARCH_DOWN, \"E9 C1 FF 68 E9 E1 FF 70\");\n"
		"		if(currAddr == BADADDR)\n"
		"			break;\n"
		"		for(i=14; i<=31; i++)\n"
		"		{\n"
		"			MakeUnknown(currAddr, 4, 0); // DOUNK_SIMPLE\n"
		"			MakeCode(currAddr);\n"
		"			if(i != 31)\n"
		"				MakeFunction(currAddr, currAddr + 4);\n"
		"			else\n"
		"				MakeFunction(currAddr, currAddr + 0x10);\n"
		"			MakeNameForce(currAddr, form(\"__restgprlr_%%d\", i));\n"
		"			currAddr = currAddr + 4;\n"
		"		}\n"
		"	}\n"
		"}\n"
		"\n"
	);
	
	// convert to code
	fprintf(fd,
		"static ConvertToCode(startAddr, endAddr)\n"
		"{\n"
		"    auto addr;\n"
		"    if(startAddr == BADADDR || endAddr == BADADDR || startAddr>endAddr)\n"
		"        return;\n"
		"    \n"
		"    MakeUnknown(startAddr, endAddr-startAddr, 0); // DOUNK_SIMPLE\n"
		"    for(addr=startAddr&0xFFFFFFFC; addr<endAddr; addr=addr+4)\n"
		"    {\n"
		"        MakeCode(addr);\n"
		"    }\n"
		"    AnalyzeArea(startAddr, endAddr);\n"
		"}\n"
		"\n"
	);
	
	// main
	fprintf(fd,
		"static main()\n"
		"{\n"
		"    // ensure file was loaded in as binary\n"
		"    // if it was loaded in as PE then addresses will be incorrect\n"
		"    if( GetShortPrm(INF_FILETYPE) != FT_BIN )\n"
		"    {\n"
		"        Warning(\"The file must be loaded as a BINARY file to use this script.\\n\"\n"
		"                \"Close this database and create a new one, ensuring you\\n\"\n"
		"                \"select \\\"Binary File\\\" on IDAs \\\"Load a new file\\\" dialog.\");\n"
		"        return;\n"
		"    }\n"
		"    \n"
		"    // ensure file was loaded in as PPC\n"
		"    if( GetCharPrm(INF_PROCNAME+0) != 'P' ||\n"
		"        GetCharPrm(INF_PROCNAME+1) != 'P' ||\n"
		"        GetCharPrm(INF_PROCNAME+2) != 'C' ||\n"
		"        GetCharPrm(INF_PROCNAME+3) != '\\0' )\n"
		"    {\n"
		"        Warning(\"The file must be loaded for the PPC processor.\\n\"\n"
		"                \"Close this database and create a new one, ensuring you\\n\"\n"
		"                \"select \\\"PowerPC: ppc\\\" on IDAs \\\"Load a new file\\\" dialog.\");\n"
		"        return;\n"
		"    }\n"
		"\n"
		"    // set up resources\n"
		"    if( 1 == AskYN(0, \"Would you like to load reources as segments?\") )\n"
		"        SetupResources();\n"
		"\n"
		"    // set up sections\n"
		"    SetupSections();\n"
		"\n"
		"    // remove empty sections\n"
		"    RemoveEmptySections();\n"
		"\n"
		"    // analyse code\n"
		"    if( 1 == AskYN(1, \"Would you like to analyse the file as code?\") )\n"
		"        ConvertToCode( GetSectionAddr(\".text\"), SegEnd(GetSectionAddr(\".text\")) );\n"
		"\n"
		"    // set up imports\n"
		"    SetupImports();\n"
		"\n"
		"    // set up exports\n"
		"    SetupExports();\n"
		"\n"
		"    // set up exports by name\n"
		"    SetupExportsByName();\n"
		"\n"
		"    // setup all reg loads/stores\n"
		"    SetupRegSaves();\n"
		"\n"
		"    // done\n"
		"    Message(\"done\\n\\n\");\n"
		"}\n"
		"\n"
	);
	
	return true;
}

