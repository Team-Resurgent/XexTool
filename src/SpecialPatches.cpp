// 
// special xex patches for particular games and xexs
// 

#include "SpecialPatches.h"
#include "SpecialPatchesRetailData.h"
#include "SpecialPatchesXefu.h"
#include "XeCryptCompat.h"
#include "PEParser.h"
#include "XexEndian.h"
#include <ctype.h>


bool DanceCryptPatch(Xex& xex, u32 patchFlags, FILE* printStream);
bool RockbandCryptPatch(Xex& xex, u32 patchFlags, FILE* printStream);
bool XamPatches( Xex& xex, u32 patchFlags, FILE* printStream);
bool XbdmPatches(Xex& xex, u32 patchFlags, FILE* printStream);
bool XefuPatches(Xex& xex, u32 patchFlags, FILE* printStream);
bool XboxPatches(Xex& xex, u32 patchFlags, FILE* printStream);
bool DashPatches(Xex& xex, u32 patchFlags, FILE* printStream);


// perform special patches on the xex
// a patchFlags of 0 means to print all patches
// a patchFlags of -1 means all patches
bool DoSpecialPatches(Xex& xex, u32 patchFlags, FILE* printStream)
{
	// work out what xex is being dealt with from its title id
	ExecutionId exec_id;
	if( !xex.getExecutionId(exec_id) )
		return false;
	
	// get extra values to use for dlls which do not have title-ids
	char bound_path[1024] = {0};
	char original_name[1024] = {0};
	xex.getBoundingPath(bound_path, sizeof(bound_path));
	xex.getOriginalPEName(original_name, sizeof(original_name));
	
	switch(exec_id.titleId)
	{
	// Rockband 1		(EA-2089)
	case 0x45410829:	return RockbandCryptPatch(xex, patchFlags, printStream);
	
	// Rockband 2		(EA-2153)
	case 0x45410869:	return RockbandCryptPatch(xex, patchFlags, printStream);
	
	// Rockband 3		(EA-2324)
	case 0x45410914:	return RockbandCryptPatch(xex, patchFlags, printStream);
	
	// Rockband AC/DC	(EA-2185)
	case 0x45410889:	return RockbandCryptPatch(xex, patchFlags, printStream);
	
	// Rockband Beatles	(EA-2225)
	case 0x454108B1:	return RockbandCryptPatch(xex, patchFlags, printStream);
	
	// Rockband Lego	(WR-2032)
	case 0x575207F0:	return RockbandCryptPatch(xex, patchFlags, printStream);
	
	// Dance Central Beta	(TV-34775)
	case 0x545687D7:	return DanceCryptPatch(xex, patchFlags, printStream);
	
	// Dance Central Final	(TV-2003)
	case 0x545607D3:	return DanceCryptPatch(xex, patchFlags, printStream);
	
	// Dance Central 2 Beta	(73-34772)
	case 0x373387D4:	return DanceCryptPatch(xex, patchFlags, printStream);
	
	// Dance Central 2 Final(73-2002)
	case 0x373307D2:	return DanceCryptPatch(xex, patchFlags, printStream);
	
	// Dance Central 3 Final(73-2009)
	case 0x373307D9:	return DanceCryptPatch(xex, patchFlags, printStream);
	
	// Dashboard - dash.xex
	case 0xFFFE07D1:	return DashPatches(xex, patchFlags, printStream);
	
	// Xbox1 Emu - xbox.xex or xefu.xex
	case 0xFFFE07D2:
		if(		strstr(bound_path,		"xefutitle") ||
				strstr(original_name,	"xefutitle") )
		{
			// not xefutitle??.xex
			break;
		}
		else if(strstr(bound_path,		"xbox") ||
				strstr(original_name,	"xbox") )
		{
			return XboxPatches(xex, patchFlags, printStream);
		}
		else if(strstr(bound_path,		"xefu") ||
				strstr(original_name,	"xefu") )
		{
			return XefuPatches(xex, patchFlags, printStream);
		}
		else
		{
			// files such as xefu1_1.xex and xefu2.xex do not have
			// xefu as their original or bounding name!
			// so here we force them to still be patched.
			return XefuPatches(xex, patchFlags, printStream);
		}
		break;
	
	// a dll such as xbdm.xex, xam.xex, etc
	case 0x00000000:
		if(		strstr(bound_path,		"xam") ||
				strstr(original_name,	"xam") )
		{
			return XamPatches(xex, patchFlags, printStream);
		}
		else if(strstr(bound_path,		"xbdm") ||
				strstr(original_name,	"xbdm") )
		{
			return XbdmPatches(xex, patchFlags, printStream);
		}
		break;
	}
	
	// done
	if(printStream)
		fprintf(printStream, "  no special patch found for this xex\n");
	return false;
}


// perform a fix upon an update-patched xex
bool DoUpdatePatchFix(Xex& xex, FILE* printStream)
{
	// get PE access to basefile
	DataBlock basefile;
	xex.getBasefile(basefile);
	PEParser pe_parser(basefile);

	// find "UPDATE:" and convert to "D:\0\0\0"
	// (this is found in section ".rdata")
	bool result = false;
	for(int s_idx=0; s_idx<pe_parser.getNumSections(); s_idx++)
	{
		char name[100];
		pe_parser.getSectionName(s_idx, name);
		if( !strcmp(name, ".rdata") )
		{
			u32 offset, sect_size;
			pe_parser.getSectionAddr(s_idx, offset);
			pe_parser.getSectionSize(s_idx, sect_size);
			u8* buff = new u8[sect_size];
			basefile.get(buff, offset, sect_size);
			
			for(int i=4-(offset&3); i<(int)sect_size-20; i+=4)
			{
				// the ORing of 0x20 convert it to lowercase
				// (which then remove the case specific compare)
				if( (buff[i]|0x20) == 'u' &&
					STRNICMP((char*)buff+i, "update:", strlen("update:")) == 0 )
				{
					// need to patch "update:xyz" to "D:xyz"
					int len = (int)strlen((char*)buff+i);
					if(len >= 1024)
						continue;
					int x;
					for(x=0; x<len; x++)
					{
						if( !isprint(buff[i+x]) )
							break;
					}
					if(x != len)
						continue;
					
					char fixname[1024] = {0};
					strcpy(fixname, "D:");
					strcat(fixname, (char*)buff+i+7);
					basefile.set(fixname, offset+i, len);
					xex.setBasefile(basefile);
					result = true;
				}
				// unicode "\0u\0p\0d\0a\0t\0e\0:\0"
				if(	buff[i+0x00] == 0 && (buff[i+0x01]|0x20)=='u' &&
					buff[i+0x02] == 0 && (buff[i+0x03]|0x20)=='p' &&
					buff[i+0x04] == 0 && (buff[i+0x05]|0x20)=='d' &&
					buff[i+0x06] == 0 && (buff[i+0x07]|0x20)=='a' &&
					buff[i+0x08] == 0 && (buff[i+0x09]|0x20)=='t' &&
					buff[i+0x0a] == 0 && (buff[i+0x0b]|0x20)=='e' &&
					buff[i+0x0c] == 0 && (buff[i+0x0d]|0x20)==':' )
				{
					// need to patch "\0u\0p\0d\0a\0t\0e\0:\0x\0y\0z\0\0" to "\0D\0:\0x\0y\0z\0\0"
					int len = 0;
					for(len=0; (char*)buff[i+1+len]!=0; len+=2)
					{} // just counting here folks...
					if(len > 1024*2)
						continue;
					int x;
					for(x=0; x<len; x+=2)
					{
						if( !isprint(buff[i+1+x]) )
							break;
					}
					if(x != len)
						continue;
					
					char fixname[1024*2] = {0};
					fixname[0] = '\0';
					fixname[1] = 'D';
					fixname[2] = '\0';
					fixname[3] = ':';
					for(x=7*2; x<len; x++)
						fixname[x-(7*2)+4] = buff[i+x];
					basefile.set(fixname, offset+i, len);
					xex.setBasefile(basefile);
					result = true;
				}
			}
			delete[] buff;
			break;
		}
	}
	
	if(printStream && result)
		fprintf(printStream, "  patched update fix\n");
	return result;
}


// remove discswap checks
bool DoDiscSwapChecksFix(Xex& xex, FILE* printStream)
{
	// get PE access to basefile
	DataBlock basefile;
	xex.getBasefile(basefile);
	PEParser pe_parser(basefile);
	
	// get ".text" section since that is needed for patching
	u8* text_sect_data = NULL;
	u32 text_sect_addr = 0;
	u32 text_sect_off  = 0;
	u32 text_sect_size = 0;
	bool result = false;
	for(int s_idx=0; s_idx<pe_parser.getNumSections(); s_idx++)
	{
		char name[100];
		pe_parser.getSectionName(s_idx, name);
		if( strcmp(name, ".text")==0 )
		{
			u32 offset, sect_size;
			pe_parser.getSectionAddr(s_idx, offset);
			pe_parser.getSectionSize(s_idx, sect_size);
			// buffer is only used for searching
			text_sect_addr = offset + xex.getLoadAddress();
			text_sect_off  = offset;
			text_sect_size = sect_size;
			text_sect_data = new u8[text_sect_size];
			basefile.get(text_sect_data, offset, text_sect_size);
			break;
		}
	}
	if( text_sect_data == NULL )
		return false;
	
	// search for imports of XamSwapDisc and XamSwapCancel from xam.xex
	// search for imports of KeSetEvent from xboxkrnl.exe
	u32 swapdisc_func_addr = 0;
	u32 swapcancel_func_addr = 0;
	u32 setevent_func_addr = 0;
	for(int lib_idx=0; lib_idx<xex.numImportLibraries(); lib_idx++)
	{
		char lib_name[64] = "";
		XexVersion32 lib_ver, lib_min_ver;
		DataBlock lib_addresses;
		u32 lib_module_number = 0;
		u8  lib_module_index = 0;
		if( xex.getImportLibrary(lib_idx, lib_name, lib_ver, lib_min_ver, lib_addresses, lib_module_number, lib_module_index) )
		{
			if( strcmp(lib_name, "xam.xex")==0 )
			{
				for(int a_idx=0; a_idx<lib_addresses.size()/4; a_idx++)
				{
					u32 func_addr = lib_addresses.get32(a_idx*4);
					u32 func_offset = (func_addr - text_sect_addr);
					if(func_offset+4 > text_sect_size)
						continue;
					u32 func_num1 = GET32BE(text_sect_data + func_offset + 0);
					u32 func_num2 = GET32BE(text_sect_data + func_offset + 4);
					if(	((func_num1>>24)&0xFF) != 1 ||
						((func_num2>>24)&0xFF) != 2 ||
						((func_num1>>16)&0xFF) != lib_module_index ||
						((func_num2>>16)&0xFF) != lib_module_index ||
						((func_num1>> 0)&0xFFFF) != ((func_num2>> 0)&0xFFFF) )
					{
						continue;
					}
					u32 func_num = func_num1 & 0xFFFF;
					
					// A28: XamSwapDisc
					if(func_num == 0xA28)
						swapdisc_func_addr = func_addr;
					// A2A: XamSwapCancel
					if(func_num == 0xA2A)
						swapcancel_func_addr = func_addr;
					if(swapdisc_func_addr && swapcancel_func_addr)
						break;
				}
			}
			if( strcmp(lib_name, "xboxkrnl.exe")==0 )
			{
				for(int a_idx=0; a_idx<lib_addresses.size()/4; a_idx++)
				{
					u32 func_addr = lib_addresses.get32(a_idx*4);
					u32 func_offset = (func_addr - text_sect_addr);
					if(func_offset+4 > text_sect_size)
						continue;
					u32 func_num1 = GET32BE(text_sect_data + func_offset + 0);
					u32 func_num2 = GET32BE(text_sect_data + func_offset + 4);
					if(	((func_num1>>24)&0xFF) != 1 ||
						((func_num2>>24)&0xFF) != 2 ||
						((func_num1>>16)&0xFF) != lib_module_index ||
						((func_num2>>16)&0xFF) != lib_module_index ||
						((func_num1>> 0)&0xFFFF) != ((func_num2>> 0)&0xFFFF) )
					{
						continue;
					}
					u32 func_num = func_num1 & 0xFFFF;
					
					// 9D: KeSetEvent
					if(func_num == 0x9D)
						setevent_func_addr = func_addr;
					if(setevent_func_addr)
						break;
				}
			}
		}
	}
	
//	printf("XamSwapDisc   = %08X\n", swapdisc_func_addr);
//	printf("XamSwapCancel = %08X\n", swapcancel_func_addr);
//	printf("KeSetEvent    = %08X\n", setevent_func_addr);
	
	int patch_count = 0;
	
	// Patch XSwapDisc call to XamSwapDisc
	// Patch XSwapCancel call to XamSwapCancel
	for(u32 offset=0; offset<text_sect_size; offset+=4)
	{
		u32 data = GET32BE(text_sect_data+offset);
		u32 addr = text_sect_addr + offset;
		u32 bl_XamSwapDisc_opcode	= 0x48000001 | ((swapdisc_func_addr - addr)& 0x3FFFFFE);
		u32 bl_XamSwapCancel_opcode	= 0x48000001 | ((swapcancel_func_addr - addr)& 0x3FFFFFE);
		
		if(		data == bl_XamSwapDisc_opcode )
		{
			u32 bl_KeSetEvent = 0x48000001 | ((setevent_func_addr - (addr-4))& 0x3FFFFFE);
			SET32BE(text_sect_data+offset - 0xC, 0x38800001);	// li  r4, 1
			SET32BE(text_sect_data+offset - 0x8, 0x38A00000);	// li  r5, 0
			SET32BE(text_sect_data+offset - 0x4, bl_KeSetEvent);// bl  KeSetEvent
			SET32BE(text_sect_data+offset - 0x0, 0x38600000);	// li  r3, 0
			patch_count++;
		}
		else if(data == bl_XamSwapCancel_opcode )
		{
			SET32BE(text_sect_data+offset, 0x38600000);			// li  r3, 0
			patch_count++;
		}
		
		if(patch_count == 2)
			break;
	}
	
	// done
	basefile.set(text_sect_data, text_sect_off, text_sect_size);
	xex.setBasefile(basefile);
	delete[] text_sect_data;
	return patch_count == 2;
}


// returns:	offset if found
//			<0 if not found
int SearchDataMask(const u32* buff, int buffsize, const u32* data, const u32* mask, int datasize)
{
	for(int i=0; i<buffsize/4 - datasize/4; i++)
	{
		int s;
		for(s=0; s<datasize/4; s++)
		{
			if((GET32BE(&buff[i+s]) & mask[s]) != (data[s] & mask[s]))
				break;
		}
		if(s == datasize/4)
			return i*4;
	}

	return -1;
}
int SearchDataMask(const u8* buff, int buffsize, const u8* data, u8* mask, int datasize)
{
	for(int offset=0; offset<buffsize-datasize; offset++)
	{
		if((buff[offset+0] & mask[0]) == (data[0] & mask[0]))
		{
			int i;
			for(i=0; i<datasize; i++)
			{
				if((buff[offset+i] & mask[i]) != (data[i] & mask[i]))
					break;
			}
			if(i == datasize)
				return offset;
		}
	}
	
	// not found
	return -1;
}


// patches xam for:
// 1 = force all arcade games to be licensed/unlocked/full
// 2 = allow unauthenticated kinect hw usage
// 4 = loads load.xex instead of dash.xex
bool XamPatches(Xex& xex, u32 patchFlags, FILE* printStream)
{
	if(patchFlags == 0)
	{
		if(printStream) fprintf(printStream, "  1 = force all arcade games to be licensed/unlocked/full\n");
		if(printStream) fprintf(printStream, "  2 = allow unauthenticated kinect hw usage\n");
		if(printStream) fprintf(printStream, "  4 = loads load.xex instead of dash.xex\n");
		return true;
	}
	if((patchFlags & (1|2|4)) == 0)
	{
		if(printStream) fprintf(printStream, "  unsupported patch flags\n");
		return false;
	}
	
	bool do_license_patch = (patchFlags & 1) != 0;
	bool do_kinect_patch = (patchFlags & 2) != 0;
	bool do_dash_patch = (patchFlags & 4) != 0;
	
	bool done_license_patch = false;
	bool done_kinect_patch = false;
	bool done_dash_patch = false;
	
	ExecutionId exec_id = {0};
	if( !xex.getExecutionId(exec_id) )
	{
		memset(&exec_id, 0, sizeof(exec_id));
	}
	
	// get PE access to basefile
	DataBlock basefile;
	xex.getBasefile(basefile);
	PEParser pe_parser(basefile);
	
	bool result = false;
	for(int s_idx=0; s_idx<pe_parser.getNumSections(); s_idx++)
	{
		char name[256];
		pe_parser.getSectionName(s_idx, name);
		
		// forced license and unauthorised kinect hw enable patches
		if( !strcmp(name, ".text") &&
			((do_license_patch && !done_license_patch) ||
			 (do_kinect_patch  && !done_kinect_patch)) )
		{
			// get ".text" section data
			u32 offset, sect_size;
			pe_parser.getSectionAddr(s_idx, offset);
			pe_parser.getSectionSize(s_idx, sect_size);
			u8* buff = new u8[sect_size];
			basefile.get(buff, offset, sect_size);
			
			
			// allow unauthorised kinect hw patch
			// this is only in v13549 onwards
			// 
			// find the following and make it always true
			const u32 KINECT_SEARCH_DATA1[] = {
				0x38600008,		// li        r3, 8
				0x4BFFFCB0,		// b         set_state_common
				0x4BFFEAC9,		// bl        ?XamNatalDevicePriCheckAuthentication@@YAJXZ # XamNatalDevicePriCheckAuthentication(void)
				0x7C7D1B79,		// mr.       r29, r3
				0x40800044, };	// bge       auth_successful
			const u32 KINECT_SEARCH_DATA2[] = {
				0x38600008,		// li        r3, 8
				0x4BFFFCB0,		// b         set_state_common
				0x4BFFEAC9,		// bl        ?XamNatalDevicePriCheckAuthentication@@YAJXZ # XamNatalDevicePriCheckAuthentication(void)
				0x2C030000,		// cmpwi     r3, 0
				0x40800044, };	// bge       auth_successful
			const u32 KINECT_SEARCH_MASK[] = {
				0xFFFFFFFE,0xFFFFF001,0xFC000001,0xFFE0FFFF,0xFFFFF000,};

			// this patch is probably wrong
/*
			const u32 KINECT_SEARCH_DATA[] = {
				0x38E00000,		// li        r7, 0
				0x38C00000,		// li        r6, 0
				0x38A00000,		// li        r5, 0
				0x38800035,		// li        r4, 0x35
				0x387F005D,		// addi      r3, r31, 0x5D
				0x4BFFF28D,		// bl        xam_device_auth
				0x2C030000,		// cmpwi     r3, 0
				0x40820080, };	// bne       finish_up_ret			<--- make this always jump
			const u32 KINECT_SEARCH_MASK[] = {
				0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFC000001,0xFFFFFFFF,0x000, };
*/			
/*			const u32 KINECT_SEARCH_DATA[] = {
				0x7FC3F378,		// mr        r3, result_val
				0x4BFFF411,		// bl        dev_auth_done
				0x7C7E1B79,		// mr.       result_val, r3
				0x40800034,		// bge       end_C
				0x3960000C,		// li        r11, 0xC
				0x917F0010, };	// stw       r11, 0x10(r31)
			const u32 KINECT_SEARCH_MASK[] = {
				0xFFFFFFFF,0xFC000001,0xFFFFFFFF,0xFFFFF000,0xFFFFFFFF,0xFFFFFFFF, };
*/
			if(	do_kinect_patch &&
				!done_kinect_patch )
			{
				int data_offset = SearchDataMask((u32*)buff, sect_size, KINECT_SEARCH_DATA1, KINECT_SEARCH_MASK, sizeof(KINECT_SEARCH_MASK));
				if( data_offset < 0 )
					data_offset = SearchDataMask((u32*)buff, sect_size, KINECT_SEARCH_DATA2, KINECT_SEARCH_MASK, sizeof(KINECT_SEARCH_MASK));
				if(data_offset >= 0)
				{
					const u8 BGE_INST[] = { 0x40,0x80, };
					for(u32 i=0; i<4*8; i+=4)
					{
						if( data_offset+i > sect_size )
							break;
						if( !memcmp(buff+data_offset+i, BGE_INST, sizeof(BGE_INST)) )
						{
							if( exec_id.version.build >= 15574 )
							{
								// this patch should be used on later xam.xex files
								// li        rXX, 0
								// nop
								u8 reg = basefile.get16be(offset+data_offset+i-4) & 0x1F;
								u8 patch[8] = {0x38,0x00,0x00,0x00, 0x60,0x00,0x00,0x00};
								patch[0] |= reg>>3;	// top 1 bits of regnum
								patch[1] |= reg<<5;	// bottom 3 bits of regnum
								basefile.set(patch, offset+data_offset+i-4, sizeof(patch));
								done_kinect_patch = true;
								break;
							}
							else
							{
								// this patch should be used on earlier xam.xex files
								// b         auth_successful
								basefile.set((void*)"\x48\x00", offset+data_offset+i, 2);
								done_kinect_patch = true;
								break;
							}
						}
					}
				}
			}
			
			
			// forced license patch
			// finds the XamContentGetLicenseMask() function
			// and patches it to return a license mask of '1' and return success (0)
			// this seems to work on all versions of xam.
			const u8 LICENSE_SEARCH_DATA[]	= {
				0x39,0x60,0x00,0x57,	// li        r11, 0x57
				0x7D,0x63,0x5B,0x78		// mr        r3, r11
			};
			const u8 LICENSE_PATCH_DATA[]	= {
				0x3D,0x60,0x00,0x00,	// lis %r11, 0x0000
				0x61,0x6B,0x00,0x01,	// ori %r11, %r11, 0x0001
				0x91,0x63,0x00,0x00,	// stw %r11, 0(%r3)
				0x38,0x60,0x00,0x00,	// li r3, 0
				0x4E,0x80,0x00,0x20,	// blr
			};
			
			
			// find and apply patches
			int sect_end = (int)sect_size-0x100;
			for(int i=0; i<sect_end; i++)
			{
				if(	do_license_patch &&
					!done_license_patch &&
					buff[i] == LICENSE_SEARCH_DATA[0] &&
					!memcmp(buff+i, LICENSE_SEARCH_DATA, sizeof(LICENSE_SEARCH_DATA)) )
				{
					const u8 FUNC_START[] = { 0x7D,0x88,0x02,0xA6 };	// mflr      r12
					while(i > 0)
					{
						if( !memcmp(buff+i, FUNC_START, sizeof(FUNC_START)) )
							break;
						i--;
					}
					basefile.set((void*)LICENSE_PATCH_DATA, offset+i, sizeof(LICENSE_PATCH_DATA));
					done_license_patch = true;
				}
				
				// only keep searching if there are still patches to be found
				if( do_license_patch && !done_license_patch )
					continue;
				break;
			}
			delete[] buff;
		}
		// dash replacement patch
		// this is a very simple patch, so it can't really go wrong
		else if( !strcmp(name, ".rdata") )
		{
			if( do_dash_patch &&
				!done_dash_patch )
			{
				const char DASH_SEARCH_DATA[]	= "dash.xex";
				const char DASH_PATCH_DATA[]	= "load.xex";
				
				u32 offset, sect_size;
				pe_parser.getSectionAddr(s_idx, offset);
				pe_parser.getSectionSize(s_idx, sect_size);
				u8* buff = new u8[sect_size];
				basefile.get(buff, offset, sect_size);
				
				for(int i=0; i<(int)sect_size-(int)sizeof(DASH_SEARCH_DATA); i++)
				{
					if(	buff[i] == DASH_SEARCH_DATA[0] &&
						!memcmp(buff+i, DASH_SEARCH_DATA, sizeof(DASH_SEARCH_DATA)) )
					{
						// string to patch has been found
						basefile.set((void*)DASH_PATCH_DATA, offset+i, sizeof(DASH_PATCH_DATA));
						done_dash_patch = true;
					}
					
					// only keep searching if there are still patches to be found
					if( do_dash_patch && !done_dash_patch )
						continue;
					break;
				}
				
				delete[] buff;
			}
		}
	}
	
	
	// print the results of the patching
	if( do_license_patch )
	{
		if( done_license_patch )
		{
			if(printStream) fprintf(printStream, "  patched force all arcade games to be licensed/unlocked/full\n");
			result = true;
		}
		else
			if(printStream) fprintf(printStream, "  error patching force all arcade games to be licensed/unlocked/full\n");
	}
	if( do_kinect_patch )
	{
		if( done_kinect_patch )
		{
			if(printStream) fprintf(printStream, "  patched allow unauthenticated kinect hw usage\n");
			result = true;
		}
		else
			if(printStream) fprintf(printStream, "  error patching allow unauthenticated kinect hw usage\n");
	}
	if( do_dash_patch )
	{
		if( done_dash_patch )
		{
			if(printStream) fprintf(printStream, "  patched loads load.xex instead of dash.xex\n");
			result = true;
		}
		else
			if(printStream) fprintf(printStream, "  error patching loads load.xex instead of dash.xex\n");
	}
	
	// if at least one patch was successful, then write the patches to the file
	if( result )
		xex.setBasefile(basefile);
	return result;
}


// patches xbdm for:
// 1 = allow access to extra drives in network neighbourhood
// 2 = disable flagging of slim devkits as testkits
bool XbdmPatches(Xex& xex, u32 patchFlags, FILE* printStream)
{
	if(patchFlags == 0)
	{
		if(printStream) fprintf(printStream, "  1 = allow access to extra drives in network neighbourhood\n");
		if(printStream) fprintf(printStream, "  2 = disable flagging of slim devkits as testkits\n");
		return true;
	}
	if((patchFlags & (1|2)) == 0)
	{
		if(printStream) fprintf(printStream, "  unsupported patch flags\n");
		return false;
	}
	
	bool do_map_patch = (patchFlags & 1) != 0;
	bool do_testkit_patch = (patchFlags & 2) != 0;
	
	int internal_addr_reg = 0;
	u32 internal_addr = 0;
	bool got_internal_addr_lo = false;
	bool got_internal_addr = false;
	bool patched_internal = false;
	bool patched_map_flags = false;
	
	// get PE access to basefile
	DataBlock basefile;
	xex.getBasefile(basefile);
	PEParser pe_parser(basefile);
	
	// find MapDebugDrive(?, ? 0)
	// and patch to MapDebugDrive(?, ? 1)
	// (this is found in section ".text")
	bool result = false;
	for(int s_idx=0; s_idx<pe_parser.getNumSections(); s_idx++)
	{
		char name[100];
		pe_parser.getSectionName(s_idx, name);
		if( !strcmp(name, ".text") )
		{
			u32 offset, sect_size;
			pe_parser.getSectionAddr(s_idx, offset);
			pe_parser.getSectionSize(s_idx, sect_size);
			// buffer is only used for searching?
			u8* buff = new u8[sect_size];
			basefile.get(buff, offset, sect_size);
			
			// new map search data
			const u32 MAP_NEW_SEARCH_DATA[]	= {
				0x38A00000,		// li        r5, 0
				0x4BFFC5D9,		// bl        MapDebugDrive
				0x4BFFECF5 };	// bl        LoadIniFile
			const u32 MAP_NEW_SEARCH_MASK[]	= {
				0xFFFFFFFF, 0xFC000001, 0xFC000001 };
			
			// old map search data
			const u32 MAP_OLD_SEARCH_DATA[]	= {
				0x816B8084,		// lwz       r11, -0x6318(r11)
				0x2F0B0000,		// cmpwi     cr6, r11, 0
				0x419A0008,		// beq       cr6, $+8
				0x4BFFD989, 	// bl        MapInternalDrives
				0x39610058, };	// addi      r11, r1, 0x58
			const u32 MAP_OLD_SEARCH_MASK[]	= {
				0xFE000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFC000001, 0xFF000000 };
			
			int data_offset = SearchDataMask((u32*)buff, sect_size, MAP_NEW_SEARCH_DATA, MAP_NEW_SEARCH_MASK, sizeof(MAP_NEW_SEARCH_DATA));
			if( data_offset < 0 )
				data_offset = SearchDataMask((u32*)buff, sect_size, MAP_OLD_SEARCH_DATA, MAP_OLD_SEARCH_MASK, sizeof(MAP_OLD_SEARCH_DATA));
			if(data_offset >= 0)
			{
				// get the low part of the address of the "g_fInternal" variable
				// and patch the flags inside MapInternalDrives()
				int i;
				for(i=0; i<20*4; i+=4)
				{
					// find the address of the "g_fInternal" variable
					if( (do_map_patch || do_testkit_patch) &&
						!got_internal_addr_lo &&
						(basefile.get32be(offset+data_offset+i) & 0xFFE00000) == 0x81600000 )
					{
						internal_addr = basefile.get32be(offset+data_offset+i) & 0xFFFF;
						if(internal_addr & 0x8000)
							internal_addr |= 0xFFFF0000;
						internal_addr_reg = (basefile.get32be(offset+data_offset+i)>>16) & 0x1F;
						got_internal_addr_lo = true;
					}
					
					// get the address that the map drives function is called from
					// 4B FF C7 59   bl        MapInternalDrives
					if( do_map_patch &&
						got_internal_addr_lo &&
						!patched_map_flags &&
						(basefile.get32be(offset+data_offset+i) & 0xFC000001) == 0x48000001 )
					{
						// patch the individual calls to MapDebugDrive()
						u32 func_addr = (basefile.get32be(offset+data_offset+i) & 0x00FFFFFC);
						if(func_addr >= 0x800000)
							func_addr |= 0xFF000000;
						func_addr += (u32)(xex.getLoadAddress() + offset + data_offset + i);
						int func_offset = func_addr - xex.getLoadAddress();
						bool first_skipped = false;
						u32* ptr = (u32*)buff;
						for(int x = func_offset; basefile.get32be(x) != 0x4E800020; x+=4)
						{
							if((basefile.get32be(x) & 0xFFFF0000) == 0x38A00000)
							{
								if(first_skipped)
									basefile.set32be(0x38A00001, x);
								first_skipped = true;
							}
						}
						
						patched_map_flags = true;
					}
					
					// only keep searching if there are still patches to be found
					if( do_map_patch && (!got_internal_addr_lo || !patched_map_flags) )
						continue;
					if( do_testkit_patch && (!got_internal_addr_lo) )
						continue;
					break;
				}

				// get the high part of the address of the "g_fInternal" variable
				for(i=-i; i<80*4; i+=4)
				{
					// find the address of the "g_fInternal" variable
					if( (do_map_patch || do_testkit_patch) &&
						!got_internal_addr &&
						(basefile.get32be(offset+data_offset-i) & 0xFFFF0000) == (0x3C000000 | (internal_addr_reg<<(5+16))) )
					{
						internal_addr += (basefile.get32be(offset+data_offset-i)&0xFFFF) << 16;
						got_internal_addr = true;
						break;
					}
					
					// mflr      r12
					if( basefile.get32be(offset+data_offset-i) == 0x7D8802A6 )
						break;
				}
			}
			
			delete[] buff;
		}
		else if( !strcmp(name, ".data") )
		{
			u32 offset, sect_size;
			pe_parser.getSectionAddr(s_idx, offset);
			pe_parser.getSectionSize(s_idx, sect_size);
			
			// set the "g_fInternal" variable to 1
			if( (do_map_patch || do_testkit_patch) &&
				got_internal_addr )
			{
				u32 var_offset = internal_addr - (xex.getLoadAddress() + offset);
				if( var_offset + 4 <= sect_size )
				{
					basefile.set32be(1, var_offset + offset);
					patched_internal = true;
					break;
				}
			}
		}
	}
	
	// print the results of the patching
	if( do_map_patch )
	{
		if( patched_map_flags && patched_internal )
		{
			if(printStream) fprintf(printStream, "  patched allow access to extra drives in network neighbourhood\n");
			result = true;
		}
		else
			if(printStream) fprintf(printStream, "  error patching allow access to extra drives in network neighbourhood\n");
	}
	if( do_testkit_patch )
	{
		if( patched_internal )
		{
			if(printStream) fprintf(printStream, "  patched disable flagging of slim devkits as testkits\n");
			result = true;
		}
		else
			if(printStream) fprintf(printStream, "  error patching disable flagging of slim devkits as testkits\n");
	}
	
	// if at least one patch was successful, then write the patches to the file
	if( result )
		xex.setBasefile(basefile);
	return result;
}


// patches crypto routines that stop decryption from working on devkits
bool HarmonixCryptPatch(Xex& xex)
{
	// get PE access to basefile
	DataBlock basefile;
	xex.getBasefile(basefile);
	PEParser pe_parser(basefile);
	
	u32 XeKeysSetKey_addr = 0;
	u32 TCCrypt_addr = 0;
	u32 TCSetKey_addr = 0;
	u32 TCCryptData_addr = 0;
	u32 TCKey_addr = 0;
	s32 TCKey_count = 0;
	
	// convert XeKeysSetKey and XeKeysAesCbc imports
	// to XeCryptAesKey and XeCryptAesCbc
	// (these are found in section ".rdata")
	for(int s_idx=0; s_idx<pe_parser.getNumSections(); s_idx++)
	{
		char name[100];
		pe_parser.getSectionName(s_idx, name);
		
		// patch ".rdata" imports from XeKeysSetKey and XeKeysAesCbc to XeCryptAesKey and XeCryptAesCbc
		if( !strcmp(name, ".rdata") )
		{
			const u8 SEARCH_DATA1[8]= { 0x00, 0x01, 0x02, 0x4B, 0x00, 0x01, 0x02, 0x42 };
			const u8 PATCH_DATA1[8]	= { 0x00, 0x01, 0x01, 0x5B, 0x00, 0x01, 0x01, 0x59 };
			const u8 SEARCH_DATA2[8]= { 0x00, 0x01, 0x02, 0x42, 0x00, 0x01, 0x02, 0x4B };
			const u8 PATCH_DATA2[8]	= { 0x00, 0x01, 0x01, 0x59, 0x00, 0x01, 0x01, 0x5B };
			
			u32 offset, sect_size;
			pe_parser.getSectionAddr(s_idx, offset);
			pe_parser.getSectionSize(s_idx, sect_size);
			u8* buff = new u8[sect_size];
			basefile.get(buff, offset, sect_size);
			
			bool patched = false;
			for(int i=0; i<(int)sect_size-(int)sizeof(SEARCH_DATA1); i++)
			{
				if(buff[i] == SEARCH_DATA1[0])
				{
					if( !memcmp(buff+i, SEARCH_DATA1, sizeof(SEARCH_DATA1)) )
					{
						basefile.set((void*)PATCH_DATA1, offset+i, sizeof(PATCH_DATA1));
						patched = true;
						break;
					}
				}
				if(buff[i] == SEARCH_DATA2[0])
				{
					if( !memcmp(buff+i, SEARCH_DATA2, sizeof(SEARCH_DATA2)) )
					{
						basefile.set((void*)PATCH_DATA2, offset+i, sizeof(PATCH_DATA2));
						patched = true;
						break;
					}
				}
			}
			delete[] buff;
			if( !patched )
				return false;
		}
		
		// patch code to use normal aes routines
		else if( !strcmp(name, ".text") )
		{
			// patch ".text" functions XeKeysSetKey and XeKeysAesCbc to XeCryptAesKey and XeCryptAesCbc
			const u8 SEARCH_DATA1[32] = {
				0x01,0x01,0x02,0x42,0x02,0x01,0x02,0x42, 0x7D,0x69,0x03,0xA6,0x4E,0x80,0x04,0x20,
				0x01,0x01,0x02,0x4B,0x02,0x01,0x02,0x4B, 0x7D,0x69,0x03,0xA6,0x4E,0x80,0x04,0x20,
			};
			const u8 PATCH_DATA1[32] = {
				0x01,0x01,0x01,0x59,0x02,0x01,0x01,0x59, 0x7D,0x69,0x03,0xA6,0x4E,0x80,0x04,0x20,
				0x01,0x01,0x01,0x5B,0x02,0x01,0x01,0x5B, 0x7D,0x69,0x03,0xA6,0x4E,0x80,0x04,0x20,
			};
			const u8 SEARCH_DATA2[32] = {
				0x01,0x01,0x02,0x4B,0x02,0x01,0x02,0x4B, 0x7D,0x69,0x03,0xA6,0x4E,0x80,0x04,0x20,
				0x01,0x01,0x02,0x42,0x02,0x01,0x02,0x42, 0x7D,0x69,0x03,0xA6,0x4E,0x80,0x04,0x20,
			};
			const u8 PATCH_DATA2[32] = {
				0x01,0x01,0x01,0x5B,0x02,0x01,0x01,0x5B, 0x7D,0x69,0x03,0xA6,0x4E,0x80,0x04,0x20,
				0x01,0x01,0x01,0x59,0x02,0x01,0x01,0x59, 0x7D,0x69,0x03,0xA6,0x4E,0x80,0x04,0x20,
			};

			u32 offset, sect_size;
			pe_parser.getSectionAddr(s_idx, offset);
			pe_parser.getSectionSize(s_idx, sect_size);
			u8* buff = new u8[sect_size];
			u32* ptr = (u32*)buff;
			basefile.get(buff, offset, sect_size);
			
			for(int i=0; i<(int)sect_size-(int)sizeof(SEARCH_DATA1); i++)
			{
				if(buff[i] == SEARCH_DATA1[0])
				{
					if( !memcmp(buff+i, SEARCH_DATA1, sizeof(SEARCH_DATA1)) )
					{
						memcpy(buff+i, (void*)PATCH_DATA1, sizeof(PATCH_DATA1));
						XeKeysSetKey_addr = offset+i + pe_parser.getLoadAddress();
						break;
					}
				}
				if(buff[i] == SEARCH_DATA2[0])
				{
					if( !memcmp(buff+i, SEARCH_DATA2, sizeof(SEARCH_DATA2)) )
					{
						memcpy(buff+i, (void*)PATCH_DATA2, sizeof(PATCH_DATA2));
						XeKeysSetKey_addr = offset+i + pe_parser.getLoadAddress();
						break;
					}
				}
			}
			if( !XeKeysSetKey_addr )
			{
				delete[] buff;
				return false;
			}
			
			
			// find cross reference to XeKeysSetKey() to find TCSetKey()
			for(int i=0; i<(int)sect_size-4; i+=4)
			{
				u32 opcode = 0x48000001 | (XeKeysSetKey_addr - pe_parser.getLoadAddress() - offset - i);
				if(GET32BE(&ptr[i/4]) == opcode)
				{
					// found the call to XeKeysSetKey
					// now find the start of that function (mflr %r12)
					while( GET32BE(&ptr[i/4]) != 0x7D8802A6 ) i-=4;
					TCSetKey_addr = pe_parser.getLoadAddress() + offset + i;
					break;
				}
			}
			if( !TCSetKey_addr )
			{
				delete[] buff;
				return false;
			}

			// find cross reference to TCSetKey() to find TCCrypt()
			for(int i=0; i<(int)sect_size-4; i+=4)
			{
				u32 opcode = 0x48000001 | (TCSetKey_addr - pe_parser.getLoadAddress() - offset - i);
				if(GET32BE(&ptr[i/4]) == opcode)
				{
					int off;
					
					// found the call to TCSetKey
					// now find the start of that function (mflr %r12)
					for(off=i; GET32BE(&ptr[off/4])!=0x7D8802A6; off-=4);
					TCCrypt_addr = pe_parser.getLoadAddress() + offset + off;
					
					// now find the call to TCCryptData()
					for(off=i+4; off<(int)sect_size; off+=4)
					{
						if( (GET32BE(&ptr[off/4])&0xF8000000) == 0x48000000 )
						{
							TCCryptData_addr = (GET32BE(&ptr[off/4]) & 0x00FFFFFC);
							if(TCCryptData_addr >= 0x800000)
								TCCryptData_addr |= 0xFF000000;
							TCCryptData_addr += pe_parser.getLoadAddress() + offset + off;
							break;
						}
					}
					
					
					// now find the key address
					
					// find the address for a single key
					for(off=i; off+offset+pe_parser.getLoadAddress()>TCCrypt_addr; off-=4)
					{
						// load using r11
						// 3D 60 82 A1     lis %r11, ((g_key+0x10000)@h)
						// 38 8B 81 14     addi %r4, %r11, -0x7EEC # g_key
						if( (GET32BE(&ptr[off/4])&0xFFFF0000) == 0x388B0000 )
							TCKey_addr |= GET32BE(&ptr[off/4])&0x0000FFFF;
						if( (GET32BE(&ptr[off/4])&0xFFFF0000) == 0x3D600000 && TCKey_addr != 0)
						{
							if(TCKey_addr < 0x8000)
								TCKey_addr |= (GET32BE(&ptr[off/4])&0xFFFF)<<16;
							else
								TCKey_addr |= ((GET32BE(&ptr[off/4])&0xFFFF) - 1)<<16;
							break;
						}
					}
					TCKey_count = 1;
					if(TCKey_addr) break;
					
					// find the address for multiple keys (dance central "disc")
					// find the address for multiple keys (rb3 disc)
					// find the address for multiple keys (rb1-tu5)
					int bl_count = 0;
					for(off=i; off+offset+pe_parser.getLoadAddress()>TCCrypt_addr; off-=4)
					{
						if( (GET32BE(&ptr[off/4])&0xFC000000) == 0x48000000 )
							bl_count++;
						if( (GET32BE(&ptr[off/4])&0xFFFF0000) == 0x396B0000 )
							TCKey_addr |= GET32BE(&ptr[off/4])&0x0000FFFF;
						if( (GET32BE(&ptr[off/4])&0xFFFF0000) == 0x3D600000 && TCKey_addr != 0)
						{
							if(TCKey_addr < 0x8000)
								TCKey_addr |= (GET32BE(&ptr[off/4])&0xFFFF)<<16;
							else
								TCKey_addr |= ((GET32BE(&ptr[off/4])&0xFFFF) - 1)<<16;
						}
					}
					for( ; off+offset+pe_parser.getLoadAddress()>TCCrypt_addr; off-=4)
					{
						if( (GET32BE(&ptr[off/4])&0xFC000000) == 0x48000000 )
							bl_count++;
					}
					if(bl_count == 2)
					{
						// (dance central "disc")
						// find the address for multiple keys (rb3 disc)
						TCKey_count = 4;
						if(TCKey_addr) break;
					}
					else
					{
						// (rb1-tu5)
						TCKey_count = 2;
						if(TCKey_addr) break;
					}
					
					// find the address for multiple keys (rb1-tu5)
					for(off=i; off+offset+pe_parser.getLoadAddress()>TCCrypt_addr; off-=4)
					{
						if( (GET32BE(&ptr[off/4])&0xFFFF0000) == 0x396B0000 )
							TCKey_addr |= GET32BE(&ptr[off/4])&0x0000FFFF;
						if( (GET32BE(&ptr[off/4])&0xFFFF0000) == 0x3D600000 && TCKey_addr != 0)
						{
							if(TCKey_addr < 0x8000)
								TCKey_addr |= (GET32BE(&ptr[off/4])&0xFFFF)<<16;
							else
								TCKey_addr |= ((GET32BE(&ptr[off/4])&0xFFFF) - 1)<<16;
						}
					}
					TCKey_count = 2;
					if(TCKey_addr) break;
					
					// find the address for multiple keys (rb2-tu2)
					for(off=i; off+offset+pe_parser.getLoadAddress()>TCCrypt_addr; off-=4)
					{
						if( (GET32BE(&ptr[off/4])&0xFFFF0000) == 0x396A0000 )
							TCKey_addr |= GET32BE(&ptr[off/4])&0x0000FFFF;
						if( (GET32BE(&ptr[off/4])&0xFFFF0000) == 0x3D400000 && TCKey_addr != 0)
						{
							if(TCKey_addr < 0x8000)
								TCKey_addr |= (GET32BE(&ptr[off/4])&0xFFFF)<<16;
							else
								TCKey_addr |= ((GET32BE(&ptr[off/4])&0xFFFF) - 1)<<16;
						}
					}
					TCKey_count = 2;
					if(TCKey_addr) break;
					
					// find the address for multiple keys (rb2-tu5)
					for(off=i; off+offset+pe_parser.getLoadAddress()>TCCrypt_addr; off-=4)
					{
						if( (GET32BE(&ptr[off/4])&0xFFFF0000) == 0x394A0000 )
							TCKey_addr |= GET32BE(&ptr[off/4])&0x0000FFFF;
						if( (GET32BE(&ptr[off/4])&0xFFFF0000) == 0x3D400000  && TCKey_addr != 0)
						{
							if(TCKey_addr < 0x8000)
								TCKey_addr |= (GET32BE(&ptr[off/4])&0xFFFF)<<16;
							else
								TCKey_addr |= ((GET32BE(&ptr[off/4])&0xFFFF) - 1)<<16;
						}
					}
					TCKey_count = 3;
					if(TCKey_addr) break;
					
					// find the address for multiple keys (rb-lego)
					for(off=i; off+offset+pe_parser.getLoadAddress()>TCCrypt_addr; off-=4)
					{
						// 3D 20 82 76     lis %r9, ((g_key+0x10000)@h)
						// 39 69 EF 08     addi %r11, %r9, -0x10F8 # g_key
						if( (GET32BE(&ptr[off/4])&0xFFFF0000) == 0x39690000 )
							TCKey_addr |= GET32BE(&ptr[off/4])&0x0000FFFF;
						if( (GET32BE(&ptr[off/4])&0xFFFF0000) == 0x3D200000 && TCKey_addr != 0)
						{
							if(TCKey_addr < 0x8000)
								TCKey_addr |= (GET32BE(&ptr[off/4])&0xFFFF)<<16;
							else
								TCKey_addr |= ((GET32BE(&ptr[off/4])&0xFFFF) - 1)<<16;
						}
					}
					TCKey_count = 2;
					if(TCKey_addr) break;
					
					// no keys found!
					TCKey_count = 0;
					break;
				}
			}
			if( !TCKey_addr || !TCKey_count )
			{
				delete[] buff;
				return false;
			}
			
			
			// now all the addresses have been collected it time for the patching to begin!
			
			
			// patch TCCrypt() to use an sha context
			int off = TCCrypt_addr - pe_parser.getLoadAddress() - offset;
			while( (GET32BE(&ptr[off/4]) & 0xFFFF0000) != 0x94210000 ) off += 4;// stwu %sp, -0x70(%sp)
			int stack_size = ((GET32BE(&ptr[off/4])&0xFFFF) ^ 0xFFFF) + 1;
			SET32BE(&ptr[off/4], 0x94210000 | ((stack_size+0x190-1)^0xFFFF));	// patch sha-context onto the stack
			
			while( GET32BE(&ptr[off/4]) != 0x38600000 ) off += 4;				// li  %r3, 0
			SET32BE(&ptr[off/4], 0x38610000 | (stack_size-0x20));				// patch sha-context into arg0
			
			while( GET32BE(&ptr[off/4]) != 0x38600000 ) off += 4;				// li  %r3, 0
			SET32BE(&ptr[off/4], 0x38610000 | (stack_size-0x20));				// patch sha-context into arg0
			
			while( (GET32BE(&ptr[off/4]) & 0xFFFF0000) != 0x38210000 ) off += 4;// addi %sp, %sp, 0x70
			SET32BE(&ptr[off/4], 0x38210000 | (stack_size+0x190));				// patch sha-context onto the stack
			
			
			// patch TCSetKey() to use the sha-context
			off = TCSetKey_addr - pe_parser.getLoadAddress() - offset;
			while( GET32BE(&ptr[off/4]) != 0x4E800020 )			// blr
			{
				if(		GET32BE(&ptr[off/4]) == 0x386300E0 )	// addi %r3, %r3, 0xE0
					SET32BE(&ptr[off/4], 0x60000000);
				else if(GET32BE(&ptr[off/4]) == 0x2B030008 )	// cmplwi cr6, %r3, 8
				{
					SET32BE(&ptr[off/4], 0x60000000);	off += 4;
					SET32BE(&ptr[off/4], 0x48000000 | (GET32BE(&ptr[off/4])&0xFFFF));
				}
				off += 4;
			}
			
			
			// patch TCCryptData() to use the sha-context
			off = TCCryptData_addr - pe_parser.getLoadAddress() - offset;
			while( (GET32BE(&ptr[off/4]) & 0xFFFF0000) != 0x94210000 ) off += 4;// stwu %sp, -0x70(%sp)
			stack_size = ((GET32BE(&ptr[off/4])&0xFFFF) ^ 0xFFFF) + 1;
			SET32BE(&ptr[off/4], 0x94210000 | ((stack_size+0x10-1)^0xFFFF));	// patch IV onto the stack
			
			while( GET32BE(&ptr[off/4]) != 0x38E00000 )							// li  %r7, 0
			{
				if( GET32BE(&ptr[off/4]) == 0x2B030008 )						// cmplwi cr6, %r3, 8
				{
					SET32BE(&ptr[off/4], 0x60000000);	off += 4;				// nop
					SET32BE(&ptr[off/4], 0x48000000 | (GET32BE(&ptr[off/4])&0xFFFF));
				}
				off += 4;				// li  %r7, 0
			}
			SET32BE(&ptr[off/4], 0x38E10000 | (stack_size-0x10));	off += 4;	// put IV on stack
			SET32BE(&ptr[off/4], 0xF9070000);	off += 4;						// zero 1st half of IV
			SET32BE(&ptr[off/4], 0xF9070008);	off += 4;						// zero 2nd half of IV
			
			while( (GET32BE(&ptr[off/4])&0xFFFF0000) != 0x38210000 ) off += 4;	// addi %sp, %sp, 0x60
			SET32BE(&ptr[off/4], 0x38210000 | (stack_size+0x10));	off += 4;	// patch IV onto the stack
			
			
			// finish fixing ".text" section
			basefile.set(buff, offset, sect_size);
			delete[] buff;
		}
		
		// patch key to be a normal aes key
		else if( !strcmp(name, ".data") )
		{
			u32 offset, sect_size;
			pe_parser.getSectionAddr(s_idx, offset);
			pe_parser.getSectionSize(s_idx, sect_size);
			
			// get original key
			u8* game_aes_key = new u8[TCKey_count * 0x10];
			int text_offset = TCKey_addr-pe_parser.getLoadAddress();
			basefile.get(game_aes_key, text_offset, TCKey_count*0x10);
			
			// crypt keys
			
			for(int key_idx=0; key_idx<TCKey_count; key_idx++)
			{
				// 1) sha over game-key
				XECRYPT_HMAC_SHA_STATE sha_ctx;
				XeCryptHmacShaInit(&sha_ctx, game_aes_key+key_idx*0x10, 0x10);
				// 2) sha over "public key from 1bl" data
				XeCryptHmacShaUpdate(&sha_ctx, G_1BL_PUB_KEY, 0x110);
				// 3) sha over "other keys from 1bl" data
				XeCryptHmacShaUpdate(&sha_ctx, G_1BL_OTHER_KEYS, 0x1A);
				// 4) sha over "integrity_signature" data (offset 0x18 in HV)
				XeCryptHmacShaUpdate(&sha_ctx, G_INTEGRITY_SIG, 0x08);
				// 5) sha over "constant keys" data
				XeCryptHmacShaUpdate(&sha_ctx, G_CONSTANT_KEYS, 0x590);
				// 6) sha over "xex and dvd keys" data
				XeCryptHmacShaUpdate(&sha_ctx, G_XEX_DVD_KEYS, 0x20);
				// 7) sha over "XEKEY_ROAMABLE_OBFUSCATION_KEY" key19 from keyvault
				XeCryptHmacShaUpdate(&sha_ctx, G_ROAM_OBFUSC_KEY, 0x10);
				// 8) sha over "cross platform syslink key" data
				XeCryptHmacShaUpdate(&sha_ctx, G_CP_SYSLINK_KEY, 0x10);
				XeCryptHmacShaFinal(&sha_ctx, game_aes_key+key_idx*0x10, 0x10);
			}
			
			// set altered keys
			//for(int i=0; i<0x20; i++) printf("%02X ", game_aes_key[i]);
			basefile.set(game_aes_key, text_offset, TCKey_count*0x10);
		}
	}
	
	// all patches applied successfully
	xex.setBasefile(basefile);
	return true;
}


// patches crypto routines that stop dance central songs from working on devkits
bool DanceCryptPatch(Xex& xex, u32 patchFlags, FILE* printStream)
{
	if(patchFlags == 0)
	{
		if(printStream) fprintf(printStream, "  1 = patch dance central crypt to work on devkits\n");
		return true;
	}
	if((patchFlags & 1) == 0)
	{
		if(printStream) fprintf(printStream, "  unsupported patch flags\n");
		return false;
	}
	
	bool result = HarmonixCryptPatch(xex);
	if(result)
		if(printStream) fprintf(printStream, "  patched dance central crypt to work on devkits\n");
	else
		if(printStream) fprintf(printStream, "  error patching dance central crypt to work on devkits\n");
	return result;
}

// patches crypto routines that stop rockband2 songs from working on devkits
bool RockbandCryptPatch(Xex& xex, u32 patchFlags, FILE* printStream)
{
	if(patchFlags == 0)
	{
		if(printStream) fprintf(printStream, "  1 = patch rockband crypt to work on devkits\n");
		return true;
	}
	if((patchFlags & 1) == 0)
	{
		if(printStream) fprintf(printStream, "  unsupported patch flags\n");
		return false;
	}
	
	bool result = HarmonixCryptPatch(xex);
	if(result)
		if(printStream) fprintf(printStream, "  patched rockband crypt to work on devkits\n");
	else
		if(printStream) fprintf(printStream, "  error patching rockband crypt to work on devkits\n");
	return result;
}


// patch to fix where files are expected and to boot unsupported games
// and to load files from the harddrive not inside an update package
//   1 = allow unsupported xbox1 games
bool XboxPatches(Xex& xex, u32 patchFlags, FILE* printStream)
{
	if(patchFlags == 0)
	{
		if(printStream) fprintf(printStream, "  1 = allow unsupported xbox1 games\n");
		return true;
	}
	if((patchFlags & 1) == 0)
	{
		if(printStream) fprintf(printStream, "  unsupported patch flags\n");
		return false;
	}
	
	// get PE access to basefile
	DataBlock basefile;
	xex.getBasefile(basefile);
	PEParser pe_parser(basefile);
	
	// find "update:\" and convert to "X:\"
	// (this is found in section ".rdata")
	int patch_count = 0;
	bool result = false;
	for(int s_idx=0; s_idx<pe_parser.getNumSections(); s_idx++)
	{
		char name[100];
		pe_parser.getSectionName(s_idx, name);
		if( !strcmp(name, ".rdata") )
		{
			const char SEARCH_DATA[]= "update:\\";
			
			u32 offset, sect_size;
			pe_parser.getSectionAddr(s_idx, offset);
			pe_parser.getSectionSize(s_idx, sect_size);
			u8* buff = new u8[sect_size];
			basefile.get(buff, offset, sect_size);
//			rdata_offset = offset;
			
			for(int i=0; i<(int)sect_size-(int)sizeof(SEARCH_DATA); i++)
			{
				if(buff[i] == SEARCH_DATA[0])
				{
					if( !memcmp(buff+i, SEARCH_DATA, sizeof(SEARCH_DATA)-1) )
					{
						// string to patch has been found
						char tmp[260];
						strcpy(tmp, (char*)buff+i);
						tmp[5] = 'X';
						memset(buff+i, 0, strlen(tmp));
						strcpy((char*)buff+i, tmp+5);
						result = true;
					}
				}
			}
			basefile.set(buff, offset, sect_size);
			delete[] buff;
		}
		else if( !strcmp(name, ".text") )
		{
			const u32 SEARCH_DATA[]= { 0x3D608200, 0x7C6A1B78, 0x396B06F4, 0x39200005, 0x7D2903A6 };
			const u32 SEARCH_MASK[]= { 0xFFFF0000, 0xFFFFFFFF, 0xFFFF0000, 0xFFFFFFFF, 0xFFFFFFFF };
			
			u32 offset, sect_size;
			pe_parser.getSectionAddr(s_idx, offset);
			pe_parser.getSectionSize(s_idx, sect_size);
			u8* buff = new u8[sect_size];
			u32* ptr = (u32*)buff;
			basefile.get(buff, offset, sect_size);
			
			int data_offset = SearchDataMask((u32*)buff, sect_size, SEARCH_DATA, SEARCH_MASK, sizeof(SEARCH_DATA));
			if(data_offset >= 0)
			{
				// nop out "bne cr6, loc_82010D28"
				for( int off=data_offset; GET32BE(&ptr[off/4])!=0x7D8802A6; off-=4)	// mflr %r12
				{
					if( (GET32BE(&ptr[off/4])&0xFFFF0000) == 0x409A0000 )
					{
						SET32BE(&ptr[off/4], 0x60000000);
						break;
					}
				}
				// nop out "beq cr6, loc_82010D34"
				for( int off=data_offset; GET32BE(&ptr[off/4])!=0x7D8802A6; off-=4)	// mflr %r12
				{
					if( (GET32BE(&ptr[off/4])&0xFFFF0000) == 0x419A0000 )
					{
						SET32BE(&ptr[off/4], 0x60000000);
						break;
					}
				}
				// nop out "beq cr6, loc_82010D34"
				for( int off=data_offset; GET32BE(&ptr[off/4])!=0x4E800020; off+=4)	// blr
				{
					if( (GET32BE(&ptr[off/4])&0xFFFF0000) == 0x419A0000 )
					{
						SET32BE(&ptr[off/4], 0x60000000);
						break;
					}
				}

				// change options on default title from 0x80000002 to 0x00000000
				u32 options_addr = GET32BE(buff+data_offset + 8) & 0x0000FFFF;
				if(options_addr >= 0x8000)
					options_addr |= 0xFFFF0000;
				options_addr += (GET32BE(buff+data_offset) & 0x0000FFFF)<<16;
				int options_offset = options_addr - xex.getLoadAddress();
				basefile.set32be(0, options_offset + 12);
				result = true;
			}
			
			basefile.set(buff, offset, sect_size);
			delete[] buff;
		}
	}
	
	if(result)
		if(printStream) fprintf(printStream, "  patched allow unsupported xbox1 games\n");
	else
		if(printStream) fprintf(printStream, "  error patching allow unsupported xbox1 games\n");
	
	if(result)
		xex.setBasefile(basefile);
	return result;
}


// patches to remove all limits on running xb1 games
//    1 = fix retail/debug address xor handling
//    2 = remove emulator xbe signature check
//    4 = remove kernel xbe signature check
//    8 = remove kernel xbe region check
//   10 = remove kernel xbe media check
//   20 = remove kernel xbe hash check
bool XefuPatches(Xex& xex, u32 patchFlags, FILE* printStream)
{
	if(patchFlags == 0)
	{
		if(printStream) fprintf(printStream, "   1 = fix retail/debug address xor handling\n");
		if(printStream) fprintf(printStream, "   2 = remove emulator xbe signature check\n");
		if(printStream) fprintf(printStream, "   4 = remove kernel xbe signature check\n");
		if(printStream) fprintf(printStream, "   8 = remove kernel xbe region check\n");
		if(printStream) fprintf(printStream, "  10 = remove kernel xbe media check\n");
		if(printStream) fprintf(printStream, "  20 = remove kernel xbe hash check\n");
		return true;
	}
	if((patchFlags & 0xFF) == 0)
	{
		if(printStream) fprintf(printStream, "  unsupported patch flags\n");
		return false;
	}
	
	// get basefile data
	bool patched_all_ok = true;
	DataBlock basefile_block;
	xex.getBasefile(basefile_block);
	int basefile_size = basefile_block.size();
	u8* basefile_data = new u8[basefile_size];
	basefile_block.get(basefile_data, 0, basefile_size);
	
	// do patches here
	u32 base_address = xex.getLoadAddress();
	for(int patch_num=0; patch_num<NumXboxEmuPatches; patch_num++)
	{
		if(patchFlags & (2<<patch_num))
		{
			int offset = SearchDataMask(basefile_data, basefile_size,
					XboxEmuPatches[patch_num].searchData, XboxEmuPatches[patch_num].searchMask,
					XboxEmuPatches[patch_num].searchSize);
			
			if(offset > 0)
			{
				offset += XboxEmuPatches[patch_num].patchOffset;
//				if(printStream) fprintf(printStream, "  Patching %s at address %08X\n", XboxEmuPatches[patch_num].patchName, base_address+offset);
				if(printStream) fprintf(printStream, "  patched %s\n", XboxEmuPatches[patch_num].patchName);
				memcpy(basefile_data+offset, XboxEmuPatches[patch_num].patchData, XboxEmuPatches[patch_num].patchSize);
			}
			else
			{
//				if(printStream) fprintf(printStream, "  Error finding %s to patch!\n", XboxEmuPatches[patch_num].patchName);
				if(printStream) fprintf(printStream, "  error patching %s\n", XboxEmuPatches[patch_num].patchName);
				patched_all_ok = false;
			}
		}
	}
	
	// this last patch is tricky - so only one of these has to work
	// xbe debug/retail xor value inside the emu itself
	if(patchFlags & 1)
	{
		int num_xor_patches = 0;
		for(int patch_num=0; patch_num<NumXboxEmuXorPatches; patch_num++)
		{
			int offset = SearchDataMask(basefile_data, basefile_size,
					XboxEmuXorPatches[patch_num].searchData, XboxEmuXorPatches[patch_num].searchMask,
					XboxEmuXorPatches[patch_num].searchSize);
		
			if(offset > 0)
			{
				offset += XboxEmuXorPatches[patch_num].patchOffset;
//				if(printStream) fprintf(printStream, "  Patching %s at address %08X\n", XboxEmuXorPatches[patch_num].patchName, base_address+offset);
				if(printStream) fprintf(printStream, "  patched %s\n", XboxEmuXorPatches[patch_num].patchName);
				memcpy(basefile_data+offset, XboxEmuXorPatches[patch_num].patchData, XboxEmuXorPatches[patch_num].patchSize);
				num_xor_patches++;
			}
		}
		if( num_xor_patches != 1 )
		{
//			if(printStream) fprintf(printStream, "Error %d performing Emu Xor Patches!\n", num_xor_patches);
			if(printStream) fprintf(printStream, "  error patching %s\n", XboxEmuXorPatches[0].patchName);
			patched_all_ok = false;
		}
	}

	// now write back the results
	basefile_block.set(basefile_data, 0, basefile_size);
	xex.setBasefile(basefile_block);
	delete[] basefile_data;
	return patched_all_ok;
}


// patches to remove dash limits
//    1 = allow all region dvd videos
bool DashPatches(Xex& xex, u32 patchFlags, FILE* printStream)
{
	if(patchFlags == 0)
	{
		if(printStream) fprintf(printStream, "  1 = allow all region dvd videos\n");
		return true;
	}
	if((patchFlags & (1)) == 0)
	{
		if(printStream) fprintf(printStream, "  unsupported patch flags\n");
		return false;
	}
	
	bool do_all_region = (patchFlags & 1) != 0;
	
	bool patched_all_region = false;
	
	// get PE access to basefile
	DataBlock basefile;
	xex.getBasefile(basefile);
	PEParser pe_parser(basefile);
	
	// find dvd_region_check()
	// and nop out the region check
	// (this is found in section ".text")
	bool result = false;
	for(int s_idx=0; s_idx<pe_parser.getNumSections(); s_idx++)
	{
		char name[100];
		pe_parser.getSectionName(s_idx, name);
		if( !strcmp(name, ".text") )
		{
			u32 offset, sect_size;
			pe_parser.getSectionAddr(s_idx, offset);
			pe_parser.getSectionSize(s_idx, sect_size);
			// buffer is only used for searching?
			u8* buff = new u8[sect_size];
			basefile.get(buff, offset, sect_size);
			
			// all region dvd videos patch data
			const u32 ALL_REGION_DATA[]	= {
				0x3D60003F,		// lis       r11, 0x3F
				0x8123032C,		// lwz       r9, 0x32C(r3)
				0x81090018,		// lwz       r8, 0x18(r9)
				0x7D075838,		// and       r7, r8, r11
				0x2B070000,		// cmplwi    cr6, r7, 0
				0x409A0038,		// bne       cr6, ret_0_bad_region
			};
			const u32 ALL_REGION_MASK[]	= {
				0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xFC000001 };
			
			int data_offset = SearchDataMask((u32*)buff, sect_size, ALL_REGION_DATA, ALL_REGION_MASK, sizeof(ALL_REGION_DATA));
			if(data_offset >= 0)
			{
				basefile.set32be(0x60000000, offset+data_offset+0x14);
				patched_all_region = true;
			}
			
			delete[] buff;
		}
	}
	
	// print the results of the patching
	if( do_all_region )
	{
		if( patched_all_region )
		{
			if(printStream) fprintf(printStream, "  patched allow all region dvd videos\n");
			result = true;
		}
		else
			if(printStream) fprintf(printStream, "  error patching allow all region dvd videos\n");
	}
	
	// if at least one patch was successful, then write the patches to the file
	if( result )
		xex.setBasefile(basefile);
	return result;
}


