// 
// XexTool
// 
// tool to manage xex files
// 

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <direct.h>
#include "XexTool.h"
#include "XGetopt.h"
#include "tinyxml.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iterator>
#include <vector>
#include <cstdlib>
#include <cstring>
using namespace std;

const char G_TITLE[]="XexTool v6.7  -  xorloser 2006-2017 (Build " __TIMESTAMP__ ")";
const char G_USAGE[]="Usage:    XexTool <options> <xex filename>\n"
					"Options:\n"
					"          -l = print extended info list about xex file\n"
					"          -p <xexp filename> = patch xex with xexp\n"
					"          -b <base filename> = dump basefile from xex\n"
					"          -i <idc filename>  = dump basefile info to idc file\n"
					"          -d <res. dirname>  = dump all resources to a dir (can be '.')\n"
					"          -o <xex filename>  = output altered xex to a new file\n"
					"          -a <bounding path> = add bounding path to xex location\n"
					"          -u = fix patch updated xex to not require separate patch file\n"
					"          -s = do special xex specific patches (0/1/2/4/8/10/.../80000000)\n"
					"               bitflags are used to select one or more patches at a time\n"
					"               0 = list all patches for an xex\n"
					"              -1 = do all possible patches\n"
					"               1 = patch #1\n"
					"               2 = patch #2\n"
					"               4 = patch #3 (yes #3, not 4, because its a bitflag)\n"
					"          -r = remove xex limitations (a/m/r/b/d/i/y/v/k/l/c/z)\n"
					"               a = remove all limits (same as \"mrbdiyvklcz\")\n"
					"               m = remove media limits (all media)\n"
					"               r = remove region limits (all regions)\n"
					"               b = remove bounding pathname\n"
					"               d = remove bounding device id\n"
					"               i = remove console id restriction\n"
					"               y = remove dates restriction\n"
					"               v = remove keyvault privileges restriction\n"
					"               k = remove signed keyvault only limitation\n"
					"               l = remove minimum library version limitations\n"
					"               c = remove required revocation check\n"
					"               s = remove discswap checks\n"
					"               z = zero the media id\n"
					"          -m = force output xex machine format (d/r) (0=d, 1=r)\n"
					"               d = force output xex to be devkit\n"
					"               r = force output xex to be retail\n"
					"          -c = force output xex compression format (u/c/b) (0=u, 1=c)\n"
					"               u = force output xex to be uncompressed (no zeroed data)\n"
					"               c = force output xex to be compressed\n"
					"               b = force output xex to be binary (has zeroed data)\n"
					"          -e = force output xex encryption format (u/e) (0=u, 1=e)\n"
					"               u = force output xex to be uncrypted\n"
					"               e = force output xex to be encrypted\n"
					"          -x = xml output options (a/b/d/i/m/n/p/r/t/x)\n"
					"               a = extract everything\n"
					"               b = extract basefile type (ie dll, exe, patch, other)\n"
					"               d = extract media id\n"
					"               i = extract game icon\n"
					"               m = extract game supported medias\n"
					"               n = extract game name\n"
					"               p = extract bounding path\n"
					"               r = extract game supported regions\n"
					"               t = extract title id\n"
					"               x = extract xex machine format (retail or devkit xbox360)\n"
					"           -z = get and set xex info (g/s)\n"
					"               g <filename> = get info from file\n"
					"               s <filename> = set info into file\n"
					"\n"
					"If \"-o\" is not used, the original xex file will be altered.\n"
					"Multiple options can be given at once, eg: \"-m d -r mrl\".\n"
					"If no options are given, a shortened xex info list will be printed.\n";


// returns: number of bytes gotten from string
int get_data_from_hex_string(const char* pHexStr, u8* pDataOut, int dataSize)
{
	if( pHexStr==NULL || (strlen(pHexStr)&1)!=0 || pDataOut==NULL || dataSize<=0 )
	{
		if( pHexStr )
			printf("Error getting data from hex string: %s\n", pHexStr);
		else
			printf("Error getting data from hex string: %s\n", "<NULL>");
		return 0;
	}
	int hex_str_len = strlen(pHexStr);
/*	if( hex_str_len/2 != dataSize )
	{
		printf("HexString is bad size %d vs %d\n", hex_str_len/2, dataSize);
		return 0;
	}
*/
	int i;
	for(i=0; i<dataSize; i++)
	{
		if( i >= hex_str_len/2 )
			break;
		char tmp[3];
		tmp[0] = pHexStr[i*2+0];
		tmp[1] = pHexStr[i*2+1];
		tmp[2] = '\0';
		pDataOut[i] = (u8)strtoul(tmp, NULL, 16);
	}
	return i;
}


int main(int argc, char* argv[])
{
	if(argc == 1)
	{
		printf("%s\n", G_TITLE);
		printf("%s\n", G_USAGE);
		return 1;
	}
	
	// filenames
	char xex_filename[260];
	char xexp_filename[260];
	char base_filename[260];
	char idc_filename[260];
	char res_dirname[260];
	char out_filename[260];
	char bounding_path[260];
	char info_path[260];
	u32  special_patch_num = 0;
	
	// determine which to options to do
	bool do_showlogo = true;
	bool do_print_extended = false;
	bool do_print_shortened = false;
	bool do_xexp_patch = false;
	bool do_dump_basefile = false;
	bool do_dump_idc = false;
	bool do_dump_resources = false;
	bool do_output_new_exe = false;
	bool do_update_patches = false;
	bool do_special_patches = false;
	bool do_add_bounding_path = false;
	bool do_remove_media_limits = false;
	bool do_remove_region_limits = false;
	bool do_remove_bounding_path = false;
	bool do_remove_bounding_device_id = false;
	bool do_remove_restrict_console_id = false;
	bool do_remove_restrict_dates = false;
	bool do_remove_restrict_keyvault = false;
	bool do_remove_signedkv_limit = false;
	bool do_remove_library_version_limit = false;
	bool do_remove_revocation_check = false;
	bool do_remove_discswap_checks = false;
	bool do_remove_media_id_limit = false;
	bool do_force_retail = false;
	bool do_force_devkit = false;
	bool do_force_compressed = false;
	bool do_force_uncompressed = false;
	bool do_force_binary = false;
	bool do_force_decrypted = false;
	bool do_force_encrypted = false;
	bool do_xml_output = false;
	bool do_xml_basefile = false;
	bool do_xml_mediaid = false;
	bool do_xml_icon = false;
	bool do_xml_medias = false;
	bool do_xml_name = false;
	bool do_xml_boundpath = false;
	bool do_xml_regions = false;
	bool do_xml_titleid = false;
	bool do_xml_machine = false;
	bool do_info_get = false;
	bool do_info_set = false;
	bool xex_was_altered = false;
	
	// parse options
	int getopt_result;
	while(argc > 2 &&
		(getopt_result = getopt(argc, argv, "lp:b:i:d:o:a:us:r:m:c:e:x:z:")) != -1)
	{
		switch(getopt_result)
		{
		// print extended info
		case 'l':
			do_print_extended = true;
			break;
		// patch xex with xexp
		case 'p':
			do_xexp_patch = true;
			xex_was_altered = true;
			strcpy(xexp_filename, optarg);
			break;
		// dump basefile
		case 'b':
			do_dump_basefile = true;
			strcpy(base_filename, optarg);
			break;
		// dump idc
		case 'i':
			do_dump_idc = true;
			strcpy(idc_filename, optarg);
			break;
		// dump resources
		case 'd':
			do_dump_resources = true;
			strcpy(res_dirname, optarg);
			if(strlen(res_dirname) > 0 &&
				res_dirname[strlen(res_dirname)-1] == '\\')
				res_dirname[strlen(res_dirname)-1] = 0;
			break;
		// output xex filename
		case 'o':
			do_output_new_exe = true;
			xex_was_altered = true;
			strcpy(out_filename, optarg);
			break;
		// add bounding path
		case 'a':
			do_add_bounding_path = true;
			xex_was_altered = true;
			strcpy(bounding_path, optarg);
			break;
		// update patches
		case 'u':
			do_update_patches = true;
			break;
		// special patches
		case 's':
			do_special_patches = true;
			special_patch_num = strtol(optarg, 0, 16);
			break;
		// remove limitations
		case 'r':
			if(strstr(optarg, "a"))
			{
				do_remove_media_limits = true;
				do_remove_region_limits = true;
				do_remove_bounding_path = true;
				do_remove_bounding_device_id = true;
				do_remove_restrict_console_id = true;
				do_remove_restrict_dates = true;
				do_remove_restrict_keyvault = true;
				do_remove_signedkv_limit = true;
				do_remove_library_version_limit = true;
				do_remove_revocation_check = true;
				do_remove_discswap_checks = true;
				do_remove_media_id_limit = true;
			}
			if(strstr(optarg, "m")) do_remove_media_limits = true;
			if(strstr(optarg, "r")) do_remove_region_limits = true;
			if(strstr(optarg, "b")) do_remove_bounding_path = true;
			if(strstr(optarg, "d")) do_remove_bounding_device_id = true;
			if(strstr(optarg, "i")) do_remove_restrict_console_id = true;
			if(strstr(optarg, "y")) do_remove_restrict_dates = true;
			if(strstr(optarg, "v")) do_remove_restrict_keyvault = true;
			if(strstr(optarg, "k")) do_remove_signedkv_limit = true;
			if(strstr(optarg, "l")) do_remove_library_version_limit = true;
			if(strstr(optarg, "c")) do_remove_revocation_check = true;
			if(strstr(optarg, "s")) do_remove_discswap_checks = true;
			if(strstr(optarg, "z")) do_remove_media_id_limit = true;
			xex_was_altered = true;
			break;
		// force machine format
		case 'm':
			if(strstr(optarg, "d")) do_force_devkit = true;
			if(strstr(optarg, "r")) do_force_retail = true;
			if(strstr(optarg, "0")) do_force_devkit = true;
			if(strstr(optarg, "1")) do_force_retail = true;
			if(do_force_retail && do_force_devkit)
			{
				printf("%s\n", G_TITLE);
				printf("Error trying to force to more than one machine format\n");
				return 1;
			}
			xex_was_altered = true;
			break;
		// force compression format
		case 'c':
			if(strstr(optarg, "u")) do_force_uncompressed = true;
			if(strstr(optarg, "c")) do_force_compressed = true;
			if(strstr(optarg, "b")) do_force_binary = true;
			if(strstr(optarg, "0")) do_force_uncompressed = true;
			if(strstr(optarg, "1")) do_force_compressed = true;
			if(	(do_force_compressed && do_force_uncompressed) ||
				(do_force_compressed && do_force_binary) ||
				(do_force_uncompressed && do_force_binary) )
			{
				printf("%s\n", G_TITLE);
				printf("Error trying to force to more than one compression format\n");
				return 1;
			}
			xex_was_altered = true;
			break;
		// force encryption format
		case 'e':
			if(strstr(optarg, "d")) do_force_decrypted = true;
			if(strstr(optarg, "u")) do_force_decrypted = true;
			if(strstr(optarg, "e")) do_force_encrypted = true;
			if(strstr(optarg, "0")) do_force_decrypted = true;
			if(strstr(optarg, "1")) do_force_encrypted = true;
			if(do_force_decrypted && do_force_encrypted)
			{
				printf("%s\n", G_TITLE);
				printf("Error trying to force to more than one encryption format\n");
				return 1;
			}
			xex_was_altered = true;
			break;
		// xml output options
		case 'x':
			if(strstr(optarg, "b")) do_xml_basefile = true;
			if(strstr(optarg, "d")) do_xml_mediaid = true;
			if(strstr(optarg, "i")) do_xml_icon = true;
			if(strstr(optarg, "m")) do_xml_medias = true;
			if(strstr(optarg, "n")) do_xml_name = true;
			if(strstr(optarg, "p")) do_xml_boundpath = true;
			if(strstr(optarg, "r")) do_xml_regions = true;
			if(strstr(optarg, "t")) do_xml_titleid = true;
			if(strstr(optarg, "x")) do_xml_machine = true;
			if(strstr(optarg, "a"))
			{
				do_xml_basefile = true;
				do_xml_mediaid = true;
				do_xml_icon = true;
				do_xml_medias = true;
				do_xml_name = true;
				do_xml_boundpath = true;
				do_xml_regions = true;
				do_xml_titleid = true;
				do_xml_machine = true;
			}
			do_xml_output = true;
			do_showlogo = false;
			break;
		// get/set info
		case 'z':
			if(		strlen(optarg) >= 1 && optarg[0] == 'g' )
			{
				do_info_get = true;
			}
			else if(strlen(optarg) >= 1 && optarg[0] == 's' )
			{
				do_info_set = true;
				xex_was_altered = true;
			}
			else
			{
				printf("%s\n", G_TITLE);
				printf("Error unknown info param\n");
				return 1;
			}
			
			if( strlen(optarg) > 1 )
				strcpy(info_path, optarg+1);
			else if( optind+1 < argc )
			{
				strcpy(info_path, argv[optind]);
				optind++;
			}
			else
			{
				printf("Error getting info filename\n");
				return 1;
			}
			break;
		// this should catch all unknown options
		case '?':
			printf("%s\n", G_TITLE);
			printf("Error unknown option \"%s\"\n", argv[optind-1]);
			return 1;
		}
	}
	
	if( do_showlogo )
		printf("%s\n", G_TITLE);
	
	// get xex filename
	// if only 2 args are given - the print shortened info for the given xex file
	if(argc == 2)
	{
		do_print_shortened = true;
		strcpy(xex_filename, argv[1]);
	}
	else if(optind >= argc)
	{
		printf("No xex filename specified\n");
		return 1;
	}
	else if(optind+1 < argc)
	{
		printf("Extra params specified at the end of the command line\n");
		return 1;
	}
	else
	{
		strcpy(xex_filename, argv[optind]);
	}
	
	// xex file that will hold all our xex info
	Xex xex;
	
	// all xex options require first reading in an xex file
	XexReader xex_reader;
	if( do_showlogo )
		printf("Reading and parsing input xex file...\n");
	if( !xex_reader.read(xex, xex_filename) )
	{
		printf("Error reading xex file %s\n", xex_filename);
//		if(xex.isRetail() && xex.isEncrypted())
//		{
//			printf(	"\"%s\" is an encrypted retail xex.\n"
//					"The required decryption key for retail xex files is missing\n\n", xex_filename);
//		}
//		else
		if(xex.isDeltaCompressed())
		{
			printf(	"\"%s\" is a delta compressed patch file\n"
					"It's contents only make sense when used with the required xex file!\n\n", xex_filename);
		}
		else
			return 2;
	}
	
	// if patching with xexp, do this first!
	if( do_xexp_patch )
	{
		// xex to store patch info
		Xex xexp;
		
		// read in patch file
		XexReader xexp_reader;
		if( !xexp_reader.read(xexp, xexp_filename) )
		{
			printf("Error reading patch file %s\n", xexp_filename);
			return 2;
		}
		if( xex.isPatchModule() )
		{
			printf("Error can't patch a patch file! (%s is a patch file)\n", xex_filename);
			return 3;
		}
		if( !xexp.isPatchModule() )
		{
			printf("%s is not a patch file!\n", xexp_filename);
			return 3;
		}
		
		// patch xex file with xexp
		Xex target_xex;
		XexPatcher patcher;
		if( !patcher.isCorrectPatch(xex, xexp) )
		{
			printf("%s is not the correct patch file for %s\n", xexp_filename, xex_filename);
			return 3;
		}
		if( !patcher.patch(target_xex, xex, xexp) )
		{
			printf(	"Error patching %s with %s\n", xex_filename, xexp_filename);
			return 3;
		}
		xex = target_xex;
	}
	
	// add bounding path
	if( do_add_bounding_path )
	{
		xex.setBoundingPath(bounding_path);
		printf("  added bounding path: %s\n", bounding_path);
	}
	
	// fix updated-patched xex to work
	if( do_update_patches )
	{
		if( DoUpdatePatchFix(xex) )
			xex_was_altered = true;
	}

	// special patches
	if( do_special_patches )
	{
		if( special_patch_num == 0 )
			printf("Special patches available for this file are:\n");
		if( DoSpecialPatches(xex, special_patch_num, stdout) &&
			special_patch_num != 0 )
			xex_was_altered = true;
	}

	// remove limits
	if( do_remove_media_limits )
	{
		xex.setAllMediaTypes();
		xex.setXGD2Only(false);
		printf("  removed media limit\n");
	}
	if( do_remove_region_limits )
	{
		xex.setAllRegions();
		printf("  removed region limit\n");
	}
	if( do_remove_bounding_path )
	{
		xex.clearBoundingPath();
		printf("  removed bounding path limit\n");
	}
	if( do_remove_bounding_device_id )
	{
		xex.clearBoundingDeviceId();
		printf("  removed bounding device id limit\n");
	}
	if( do_remove_restrict_console_id )
	{
		xex.clearRestrictConsoleIds();
		printf("  removed console id restriction\n");
	}
	if( do_remove_restrict_dates )
	{
		xex.clearRestrictDates();
		printf("  removed date restriction\n");
	}
	if( do_remove_restrict_keyvault )
	{
		xex.clearRestrictKVPrivs();
		printf("  removed keyvault privilege restriction\n");
	}
	if( do_remove_signedkv_limit )
	{
		xex.setSignedKeyvaultRestricted(false);
		printf("  removed signed keyvault only limit\n");
	}
	if( do_remove_library_version_limit )
	{
		for(int lib_num=0; lib_num<xex.numImportLibraries(); lib_num++)
		{
			char name[64];
			XexVersion32 version;
			XexVersion32 min_version;
			DataBlock addresses;
			u32 module_number;
			u8  module_index;
			xex.getImportLibrary(lib_num, name, version, min_version, addresses, module_number, module_index);
			min_version.build = 0;
			xex.setImportLibrary(lib_num, name, version, min_version, addresses, module_number, module_index);
		}
		printf("  removed library version limit\n");
	}
	if( do_remove_revocation_check )
	{
		xex.setRequiredRevocationCheck(false);
		printf("  removed required revocation check\n");
	}
	if( do_remove_discswap_checks )
	{
		if( DoDiscSwapChecksFix(xex) )
		{
			printf("  removed discswap checks\n");
			xex_was_altered = true;
		}
	}
	if( do_remove_media_id_limit )
	{
		MediaId media_id;
		memset(&media_id, 0, sizeof(media_id));
		xex.setMediaId(media_id);
		printf("  removed media id limit\n");
	}
	
	// machine format
	if( do_force_devkit )
	{
		xex.setDebug();
	}
	if( do_force_retail )
	{
		xex.setRetail();
	}
	
	// compression format
	if( do_force_uncompressed )
	{
		xex.setRaw();
	}
	if( do_force_binary )
	{
		xex.setBinary();
	}
	if( do_force_compressed )
	{
		xex.setCompressed();
	}
	
	// encryption format
	if( do_force_decrypted )
	{
		xex.setEncrypted(false);
	}
	if( do_force_encrypted )
	{
		xex.setEncrypted(true);
	}
	
	// print info on xex
	if( do_print_extended ||
		do_print_shortened )
	{
		XexPrinter xex_printer;
		printf("\n");
		if( do_print_extended ) xex_printer.printAll(xex);
		if( do_print_shortened ) xex_printer.printShort(xex);
		printf("\n");
	}
	
	// dump idc info
	if( do_dump_idc )
	{
		XexIdcCreator idc_creator;
		if( idc_creator.dump(xex, idc_filename) )
		{
			printf("Successfully dumped basefile idc to %s\n", idc_filename);
		}
		else
			printf("Error dumping basefile idc to %s\n", idc_filename);
	}
	
	// dump basefile
	if( do_dump_basefile )
	{
		XexBasefileDumper basefile_dumper;
		if( basefile_dumper.dump(xex, base_filename) )
		{
			printf("Successfully dumped basefile to %s\n", base_filename);
			if( xex.isBasefilePE() )
			{
				printf(	"\nLoad basefile into IDA with the following details\n"
						"DO NOT load as a PE or EXE file as the format is not valid\n"
						"File Type:       Binary file\n"
						"Processor Type:  PowerPC: ppc\n"
						"Load Address:    0x%08X\n"
						"Entry Point:     0x%08X\n", xex.getLoadAddress(), xex.getEntryPoint());
			}
		}
		else
			printf("Error dumping basefile to %s\n", base_filename);
	}
	
	// dump resources
	if( do_dump_resources )
	{
		// ensure the dir to dump to exists
		_mkdir(res_dirname);
		
		// dump resources
		XexResourceDumper res_creator(xex);
		if( xex.numResources() == 0 )
		{
			printf("File contains no resources\n");
		}
		else if( res_creator.dump(res_dirname) )
		{
			printf("Successfully dumped %d resource%s to %s\n", xex.numResources(), (xex.numResources()>1)?"s":"", res_dirname);
		}
		else
			printf("Error dumping resources to %s\n", res_dirname);
	}
	
	// xml extract / get info
	if( do_xml_output )
	{
		XexGameInfo game_info;
		std::string info;
		char tmp[1024];
		
		info += "<XexInfo>\n";
		if(do_xml_basefile)
		{
			info += "    <BasefileType format=\"string\">";
			if( xex.isTitleModule() )			info += "Exe";
			else if( xex.isDllModule() )		info += "Dll";
			else if( xex.isPatchModule() )		info += "Patch";
			else								info += "Other";
			info += "</BasefileType>\n";
		}
		if(do_xml_mediaid)
		{
			info += "    <MediaId format=\"hex\">";
			MediaId media_id;
			xex.getMediaId(media_id);
			for(int i=0; i<sizeof(MediaId); i++)
			{
				sprintf(tmp, "%02X", media_id.data[i]);
				info += tmp;
			}
			info += "</MediaId>\n";
		}
		if(do_xml_icon)
		{
			DataBlock gameicon;
			if( game_info.GameIcon(xex, gameicon) )
			{
				info += "    <GameIcon format=\"base64\">\n";
				int tmp_size = gameicon.size();
				u8* tmp_data = new u8[tmp_size];
				gameicon.get(tmp_data, 0, tmp_size);
				//base64::encode((char*)tmp_data, (char*)tmp_data+tmp_size, ostream_iterator<char>(std::cout));
				std::ostringstream buf;
				base64::encode((char*)tmp_data, (char*)tmp_data+tmp_size, ostream_iterator<char>(buf));
				delete[] tmp_data;
				info += buf.str();
				info += "\n    </GameIcon>\n";
			}
		}
		if(do_xml_medias)
		{
			info += "    <GameMedias>\n";
			if( !xex.isXGD2Only() && xex.isAllMediaTypes() )
			{
				info += "        <Media format=\"string\">All</Media>\n";
			}
			else if( xex.isXGD2Only() && xex.isMediaDvdCd() )
			{
				info += "        <Media format=\"string\">DVD-XGD2 (Xbox360 Original Disc)</Media>\n";
			}
			else if( xex.isXGD2Only() && !xex.isMediaDvdCd() )
			{
				info += "        <Media format=\"string\">Updated DVD-XGD2 (Updated version of Xbox360 Original Disc)</Media>\n";
			}
			else
			{
				if( xex.isMediaHardDisk() )				info += "        <Media format=\"string\">Hard Disk</Media>\n";
				if( xex.isMediaDvdX2() )				info += "        <Media format=\"string\">DVD-X2 (Xbox1 Original Disc)</Media>\n";
				if( xex.isMediaDvdCd() )				info += "        <Media format=\"string\">DVD / CD</Media>\n";
				if( xex.isMediaDvd5() )					info += "        <Media format=\"string\">DVD5</Media>\n";
				if( xex.isMediaDvd9() )					info += "        <Media format=\"string\">DVD9</Media>\n";
				if( xex.isMediaSystemFlash() )			info += "        <Media format=\"string\">System Flash</Media>\n";
				if( xex.isMediaMemoryUnit() )			info += "        <Media format=\"string\">Memory Unit</Media>\n";
				if( xex.isMediaMassStorage() )			info += "        <Media format=\"string\">Usb Mass Storage</Media>\n";
				if( xex.isMediaSMB() )					info += "        <Media format=\"string\">Networked SMB Share</Media>\n";
				if( xex.isMediaRam() )					info += "        <Media format=\"string\">Direct from Ram</Media>\n";
				if( xex.isMediaRamDrive() )				info += "        <Media format=\"string\">Ram Drive</Media>\n";
				if( xex.isMediaSecureVirtOD() )			info += "        <Media format=\"string\">Secure Virtual Optical Device</Media>\n";
				if( xex.isMediaWirelessNStorage() )		info += "        <Media format=\"string\">Wireless N Storage</Media>\n";
				if( xex.isMediaSystemExtPartition() )	info += "        <Media format=\"string\">System Extended Partition</Media>\n";
				if( xex.isMediaSystemAuxPartition() )	info += "        <Media format=\"string\">System Auxillary Partition</Media>\n";
				if( xex.isMediaInsecurePackage() )		info += "        <Media format=\"string\">Insecure Package (\"CONS\")</Media>\n";
				if( xex.isMediaSavegamePackage() )		info += "        <Media format=\"string\">Savegame Package (\"CONS\")</Media>\n";
				if( xex.isMediaLocallySignedPackage() )	info += "        <Media format=\"string\">Locally Signed Package (\"CONS\")</Media>\n";
				if( xex.isMediaLiveSignedPackage() )	info += "        <Media format=\"string\">Live Signed Package (\"LIVE\")</Media>\n";
				if( xex.isMediaXboxPackage() )			info += "        <Media format=\"string\">Xbox Package (\"PIRS\")</Media>\n";
				if( xex.getUnknownMediaTypes() )
				{
					sprintf(tmp, "        <Media format=\"string\">Unknown Media: %08X</Media>\n", xex.getUnknownMediaTypes());
					info += tmp;
				}
			}
			info += "    </GameMedias>\n";
		}
		if(do_xml_name)
		{
			char gamename[1024];
			if( game_info.GameName(xex, gamename, sizeof(gamename)) )
			{
				info += "    <GameName format=\"string\">";
				for(char* ptr = gamename; *ptr; ptr++)
				{
					int ch = *ptr;
					if(	(ch >= ' ' && ch < 127) &&
						(ch != '&' && ch != '<' && ch != '>' && ch != '\'' && ch != '"') )
					{
						//putchar(ch);
						info += ch;
					}
					else
					{
						sprintf(tmp, "&#%d;", ch);
						info += tmp;
					}
				}
				info += "</GameName>\n";
			}
		}
		if(do_xml_boundpath)
		{
			char bound_path[1024];
			if( xex.getBoundingPath(bound_path, sizeof(bound_path)) )
			{
				info += "    <BoundingPath>";
				sprintf(tmp, "%s", bound_path);
				info += tmp;
				info += "    </BoundingPath>\n";
			}
		}
		if(do_xml_regions)
		{
			info += "    <GameRegions>\n";
			if( xex.isAllRegions() )
			{
				info += "        <Region format=\"string\">All</Region>\n";
			}
			else
			{
				if( xex.isRegionNorthAmerica() )	info += "        <Region format=\"string\">North America</Region>\n";
				if( xex.isRegionJapan() )			info += "        <Region format=\"string\">Japan</Region>\n";
				if( xex.isRegionChina() )			info += "        <Region format=\"string\">China</Region>\n";
				if( xex.isRegionRestOfAsia() )		info += "        <Region format=\"string\">Rest of Asia</Region>\n";
				if( xex.isRegionAustNZ() )			info += "        <Region format=\"string\">Australia / New Zealand</Region>\n";
				if( xex.isRegionRestOfEurope() )	info += "        <Region format=\"string\">Rest of Europe</Region>\n";
				if( xex.isRegionRestOfWorld() )		info += "        <Region format=\"string\">Rest of the World</Region>\n";
			}
			info += "    </GameRegions>\n";
		}
		if(do_xml_titleid)
		{
			if( xex.hasExecutionId() )
			{
				info += "    <TitleId format=\"hex\">";
				ExecutionId exec_id;
				xex.getExecutionId(exec_id);
				for(int i=sizeof(exec_id.titleId)-1; i>=0; i--)
				{
					sprintf(tmp, "%02X", ((u8*)&exec_id.titleId)[i]);
					info += tmp;
				}
				info += "</TitleId>\n";
			}
		}
		if(do_xml_machine)
		{
			info += "    <MachineFormat format=\"string\">";
			if( xex.isDebug() )				info += "Devkit";
			else if( xex.isRetail() )		info += "Retail";
			else							info += "Unknown";
			info += "</MachineFormat>\n";
		}
		info += "</XexInfo>\n";
		
		if( do_xml_output )
			printf("%s", info.c_str());
	}
	

	// get info
	if( do_info_get )
	{
		std::string info;
		char tmp[1024];

		info += "<xex>\n";
		sprintf(tmp, "  <moduleflags  flags=\"%08X\"/>\n", xex.getModuleFlags());
		info += tmp;
		sprintf(tmp, "  <imageflags   flags=\"%08X\"/>\n", xex.getImageFlags());
		info += tmp;
		sprintf(tmp, "  <regions      flags=\"%08X\"/>\n", xex.getRegions());
		info += tmp;
		sprintf(tmp, "  <mediatypes   flags=\"%08X\"/>\n", xex.getMediaTypes());
		info += tmp;
		sprintf(tmp, "  <systemflags  flags=\"%08X\"/>\n", xex.getSystemFlags());
		info += tmp;
		sprintf(tmp, "  <systemflags2 flags=\"%08X\"/>\n", xex.getSystemFlags2());
		info += tmp;
		sprintf(tmp, "  <machine type=\"%s\"/>\n", xex.isDebug() ? "debug" : "retail");
		info += tmp;
		
		// media id
		info += "  <mediaid id=\"";
		MediaId media_id;
		xex.getMediaId(media_id);
		for(int i=0; i<sizeof(MediaId); i++)
		{
			sprintf(tmp, "%02X", media_id.data[i]);
			info += tmp;
		}
		info += "\"/>\n";
		// MultidiscMediaIds
		for(int i=0; i<xex.numMultidiscMediaIds(); i++)
		{
			info += "  <altmediaid id=\"";
			MediaId media_id;
			xex.getMultidiscMediaId(i, media_id);
			for(int i=0; i<sizeof(MediaId); i++)
			{
				sprintf(tmp, "%02X", media_id.data[i]);
				info += tmp;
			}
			info += "\"/>\n";
		}

		// title id, etc
		if( xex.hasExecutionId() )
		{
			ExecutionId exec_id = {0};
			xex.getExecutionId(exec_id);
			sprintf(tmp, "  <mediaid32 id=\"%08X\"/>\n", exec_id.mediaId);
			info += tmp;
			sprintf(tmp, "  <version ver32=\"%08X\"/>\n", exec_id.version.dword);
			info += tmp;
			sprintf(tmp, "  <baseversion ver32=\"%08X\"/>\n", exec_id.baseVersion.dword);
			info += tmp;
			sprintf(tmp, "  <titleid id=\"%08X\"/>\n", exec_id.titleId);
			info += tmp;
			sprintf(tmp, "  <execother id=\"%02X%02X%02X%02X\"/>\n",
				exec_id.platform, exec_id.execType, exec_id.numDisc, exec_id.maxDiscs);
			info += tmp;
			sprintf(tmp, "  <savegameid id=\"%08X\"/>\n", exec_id.saveGameId);
			info += tmp;
		}
		// AltTitleIds
		for(int i=0; i<xex.numMultidiscMediaIds(); i++)
		{
			u32 title_id = 0;
			xex.getAltTitleId(i, title_id);
			sprintf(tmp, "  <alttitleid id=\"%08X\"/>\n", title_id);
			info += tmp;
		}

		// DiscProfileId
		if( xex.hasDiscProfileId() )
		{
			info += "  <discprofileid id=\"";
			DiscProfileId profile_id;
			xex.getDiscProfileId(profile_id);
			for(int i=0; i<sizeof(DiscProfileId); i++)
			{
				sprintf(tmp, "%02X", profile_id.data[i]);
				info += tmp;
			}
			info += "\"/>\n";
		}
		
		// LANKey
		if( xex.hasLANKey() )
		{
			info += "  <lankey id=\"";
			LANKey lan_key;
			xex.getLANKey(lan_key);
			for(int i=0; i<sizeof(LANKey); i++)
			{
				sprintf(tmp, "%02X", lan_key.data[i]);
				info += tmp;
			}
			info += "\"/>\n";
		}
		
		// LogoData
		if( xex.hasLogoData() )
		{
			DataBlock logo_data;
			xex.getLogoData(logo_data);
			info += "  <logodata data=\"";
			for(int i=0; i<logo_data.size(); i++)
			{
				sprintf(tmp, "%02X", logo_data.get8(i));
				info += tmp;
			}
			info += "\"/>\n";
		}
		
		// BoundingPath
		if( xex.hasBoundingPath() )
		{
			char path[260] = "";
			xex.getBoundingPath(path, sizeof(path));
			sprintf(tmp, "  <boundingpath path=\"%s\"/>\n", path);
			info += tmp;
		}

		// BoundingDeviceId
		if( xex.hasBoundingDeviceId() )
		{
			info += "  <boundingdeviceid id=\"";
			u8 id[20];
			xex.getBoundingDeviceId(id);
			for(int i=0; i<sizeof(id); i++)
			{
				sprintf(tmp, "%02X", id[i]);
				info += tmp;
			}
			info += "\"/>\n";
		}
		
		// game ratings
		if( xex.hasGameRatings() )
		{
			GameRatings ratings;
			xex.getGameRatings(ratings);
			info += "  <gameratings data=\"";
			for(int i=0; i<sizeof(ratings); i++)
			{
				sprintf(tmp, "%02X", ((u8*)&ratings)[i]);
				info += tmp;
			}
			info += "\"/>\n";
		}

		// SPA resource
		if( xex.numResources() > 0 &&  xex.hasExecutionId() )
		{
			ExecutionId exec_id = {0};
			xex.getExecutionId(exec_id);
			char title_id_str[32];
			sprintf(title_id_str, "%08X", exec_id.titleId);
			for(int i=0; i<xex.numResources(); i++)
			{
				u32 addr = 0;
				s32 size = 0;
				char name[260] = "";
				xex.getResource(i, addr, size, name);
				if( strcmp(title_id_str, name)==0 )
				{
					u8* p_res_data = NULL;
					s32 res_size = 0;
					XexResourceDumper res_dump(xex);
					res_dump.dump(i, p_res_data, res_size);

					sprintf(tmp, "  <spa name=\"%s\" data=\"", name);
					info += tmp;
					for(int i=0; i<res_size; i++)
					{
						sprintf(tmp, "%02X", p_res_data[i]);
						info += tmp;
					}
					info += "/>\n";

					break;
				}
			}
		}
		
		info += "</xex>\n";

		FILE* fd = fopen(info_path, "wb");
		if( fd == NULL )
		{
			printf("Error creating info file %s\n", info_path);
		}
		else
		{
			fwrite(info.c_str(), 1, info.size(), fd);
			fclose(fd);
			printf("Wrote info to %s\n", info_path);
		}
	}
	
	// set info from info file into xex
	if( do_info_set )
	{
		// read in xml file
		TiXmlDocument xml_doc;
		if( !xml_doc.LoadFile(info_path) )
		{
			printf("Error reading and parsing info file: %s\n", info_path);
			return 1;
		}
		
		// parse xml file
		TiXmlElement* p_root = xml_doc.RootElement();
		if( p_root == NULL || p_root->Value() == NULL )
		{
			printf("Info file is invalid or corrupted\n");
			return 1;
		}
		if( _stricmp(p_root->Value(), "xex")!=0 )
		{
			printf("Info file has unsupported root node: %s\n", p_root->Value());
			return 1;
		}

		// clear all data types that are optional or have multiples
		// so that the only ones that exist wil lbe the ones specifically set in below.
		xex.clearMultidiscMediaIds();
		xex.clearExecutionId();
		xex.clearAltTitleIds();
		xex.clearDiscProfileId();
		xex.clearLANKey();
		xex.clearBoundingPath();
		xex.clearBoundingDeviceId();
		xex.clearGameRatings();

		// scan through and parse each entry
		for(TiXmlElement* p_elem=p_root->FirstChildElement(); p_elem; p_elem=p_elem->NextSiblingElement())
		{
			if(		 _stricmp(p_elem->Value(), "moduleflags") == 0 )
			{
				u32 flags = strtoul(p_elem->Attribute("flags"), 0, 0);
				xex.setModuleFlags(flags);
			}
			else if( _stricmp(p_elem->Value(), "imageflags") == 0 )
			{
				u32 flags = strtoul(p_elem->Attribute("flags"), 0, 0);
				xex.setImageFlags(flags);
			}
			else if( _stricmp(p_elem->Value(), "regions") == 0 )
			{
				u32 flags = strtoul(p_elem->Attribute("flags"), 0, 0);
				xex.setRegions(flags);
			}
			else if( _stricmp(p_elem->Value(), "mediatypes") == 0 )
			{
				u32 flags = strtoul(p_elem->Attribute("flags"), 0, 0);
				xex.setMediaTypes(flags);
			}
			else if( _stricmp(p_elem->Value(), "systemflags") == 0 )
			{
				u32 flags = strtoul(p_elem->Attribute("flags"), 0, 0);
				xex.setSystemFlags(flags);
			}
			else if( _stricmp(p_elem->Value(), "systemflags2") == 0 )
			{
				u32 flags = strtoul(p_elem->Attribute("flags"), 0, 0);
				xex.setSystemFlags2(flags);
			}
			else if( _stricmp(p_elem->Value(), "machine") == 0 )
			{
				bool is_retail = _stricmp(p_elem->Attribute("type"), "retail")==0;
				bool is_debug  = _stricmp(p_elem->Attribute("type"), "debug" )==0;
				if( is_retail )
					xex.setRetail();
				else if( is_debug )
					xex.setDebug();
				else
				{
					printf("Unknonwn machine type in info file: %s\n", p_elem->Attribute("type"));
					return 1;
				}
			}
			else if( _stricmp(p_elem->Value(), "mediaid") == 0 )
			{
				MediaId media_id = {0};
				int media_id_len = get_data_from_hex_string( p_elem->Attribute("id"), media_id.data, sizeof(media_id));
				if( media_id_len != sizeof(media_id) )
				{
					printf("Error parsing info file: Invalid MediaId length for %s\n", p_elem->Attribute("id"));
					return 1;
				}
				xex.setMediaId(media_id);
			}
			else if( _stricmp(p_elem->Value(), "multidiscmediaid") == 0 )
			{
				MediaId media_id = {0};
				int media_id_len = get_data_from_hex_string( p_elem->Attribute("id"), media_id.data, sizeof(media_id));
				if( media_id_len != sizeof(media_id) )
				{
					printf("Error parsing info file: Invalid MultidiscMediaId length for %s\n", p_elem->Attribute("id"));
					return 1;
				}
				xex.addMultidiscMediaId(media_id);
			}
			else if( _stricmp(p_elem->Value(), "mediaid32") == 0 )
			{
				ExecutionId exec_id = {0};
				xex.getExecutionId(exec_id);
				exec_id.mediaId = strtoul(p_elem->Attribute("id"), 0, 0);
				xex.setExecutionId(exec_id);
			}
			else if( _stricmp(p_elem->Value(), "version") == 0 )
			{
				ExecutionId exec_id = {0};
				xex.getExecutionId(exec_id);
				exec_id.version.dword = strtoul(p_elem->Attribute("ver32"), 0, 0);
				xex.setExecutionId(exec_id);
			}
			else if( _stricmp(p_elem->Value(), "baseversion") == 0 )
			{
				ExecutionId exec_id = {0};
				xex.getExecutionId(exec_id);
				exec_id.baseVersion.dword = strtoul(p_elem->Attribute("ver32"), 0, 0);
				xex.setExecutionId(exec_id);
			}
			else if( _stricmp(p_elem->Value(), "titleid") == 0 )
			{
				ExecutionId exec_id = {0};
				xex.getExecutionId(exec_id);
				exec_id.titleId = strtoul(p_elem->Attribute("id"), 0, 0);
				xex.setExecutionId(exec_id);
			}
			else if( _stricmp(p_elem->Value(), "execother") == 0 )
			{
				ExecutionId exec_id = {0};
				xex.getExecutionId(exec_id);
				u32 other_ids = strtoul(p_elem->Attribute("id"), 0, 0);
				exec_id.platform = (other_ids >> 24) & 0xFF;
				exec_id.execType = (other_ids >> 16) & 0xFF;
				exec_id.numDisc  = (other_ids >>  8) & 0xFF;
				exec_id.maxDiscs = (other_ids >>  0) & 0xFF;
				xex.setExecutionId(exec_id);
			}
			else if( _stricmp(p_elem->Value(), "savegameid") == 0 )
			{
				ExecutionId exec_id = {0};
				xex.getExecutionId(exec_id);
				exec_id.saveGameId = strtoul(p_elem->Attribute("id"), 0, 0);
				xex.setExecutionId(exec_id);
			}
			else if( _stricmp(p_elem->Value(), "alttitleid") == 0 )
			{
				u32 title_id = strtoul(p_elem->Attribute("id"), 0, 0);
				xex.addAltTitleId(title_id);
			}
			else if( _stricmp(p_elem->Value(), "discprofileid") == 0 )
			{
				DiscProfileId id = {0};
				int id_len = get_data_from_hex_string( p_elem->Attribute("id"), id.data, sizeof(id));
				if( id_len != sizeof(id) )
				{
					printf("Error parsing info file: Invalid DiscProfileId length for %s\n", p_elem->Attribute("id"));
					return 1;
				}
				xex.setDiscProfileId(id);
			}
			else if( _stricmp(p_elem->Value(), "lankey") == 0 )
			{
				LANKey key = {0};
				int key_len = get_data_from_hex_string( p_elem->Attribute("id"), key.data, sizeof(key));
				if( key_len != sizeof(key) )
				{
					printf("Error parsing info file: Invalid LANKey length for %s\n", p_elem->Attribute("id"));
					return 1;
				}
				xex.setLANKey(key);
			}
			else if( _stricmp(p_elem->Value(), "logodata") == 0 )
			{
				const char* p_data_str = p_elem->Attribute("id");
				int data_str_len = strlen(p_data_str);
				int data_buff_len = data_str_len/2;
				u8* p_data_buff = new u8[data_buff_len];
				int data_len = get_data_from_hex_string( p_data_str, p_data_buff, data_buff_len );
				if( data_len != data_buff_len )
				{
					printf("Error parsing info file: Invalid LogoData length\n");
					delete[] p_data_buff;
					return 1;
				}
				DataBlock logo_data;
				logo_data.set(p_data_buff, 0, data_len);
				xex.setLogoData(logo_data);
				delete[] p_data_buff;
			}
			else if( _stricmp(p_elem->Value(), "boundingpath") == 0 )
			{
				char bound_path[260] = {0};
				strcpy(bound_path, p_elem->Attribute("path"));
				xex.setBoundingPath(bound_path);
			}
			else if( _stricmp(p_elem->Value(), "boundingdeviceid") == 0 )
			{
				u8 bound_id[20] = {0};
				int id_len = get_data_from_hex_string( p_elem->Attribute("id"), bound_id, sizeof(bound_id) );
				if( id_len != sizeof(bound_id) )
				{
					printf("Error parsing info file: Invalid BoundingDeviceId length for %s\n", p_elem->Attribute("id"));
					return 1;
				}
				xex.setBoundingDeviceId(bound_id);
			}
			else if( _stricmp(p_elem->Value(), "gameratings") == 0 )
			{
				GameRatings ratings = {0};
				int ratings_len = get_data_from_hex_string( p_elem->Attribute("data"), (u8*)&ratings, sizeof(ratings) );
				if( ratings_len != sizeof(ratings) )
				{
					printf("Error parsing info file: Invalid GameRatings length for %s\n", p_elem->Attribute("data"));
					return 1;
				}
				xex.setGameRatings(ratings);
			}
			else if( _stricmp(p_elem->Value(), "spa") == 0 )
			{
				const char* p_name = p_elem->Attribute("name");
				if( p_name == NULL )
				{
					printf("Error getting SPA name\n");
					return 1;
				}
				const char* p_data_str = p_elem->Attribute("data");
				int data_str_len = strlen(p_data_str);
				int data_buff_len = data_str_len/2;
				u8* p_data_buff = new u8[data_buff_len];
				int data_len = get_data_from_hex_string( p_data_str, p_data_buff, data_buff_len );
				if( data_len != data_buff_len )
				{
					printf("Error parsing info file: Invalid SPA data length\n");
					delete[] p_data_buff;
					return 1;
				}
				DataBlock logo_data;
				logo_data.set(p_data_buff, 0, data_len);
				delete[] p_data_buff;
				
				// how do we add this resource in though?! it wants an address to load to.
//				xex.addResource(spa_addr, spa_size, spa_name, logo_data);
			}
		}
		
		// info successfully set into xex
		printf("Info was successfully set into xex file\n");
	}
	
	
	// write out result of changes to xex if required
	if( xex_was_altered )
	{
		char* filename = xex_filename;
		if( do_output_new_exe )
			filename = out_filename;
		XexWriter xex_writer;
		printf("Creating and writing output xex file...\n");
		if( xex_writer.write(xex, filename) )
		{
			printf("Successfully wrote altered xex to %s\n", filename);
			printf("\n%s is %s %s %s.\n", filename,
				xex.isRetail()?"retail":"devkit",
				xex.isEncrypted()?"encrypted":"unencrypted",
				xex.isCompressed()?"compressed":(xex.isRaw()?"uncompressed":"binary"));
		}
		else
			printf("Error writing altered xex to %s\n", filename);
	}
	
	return 0;
}


