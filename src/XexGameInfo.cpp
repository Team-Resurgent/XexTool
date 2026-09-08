#include "XexGameInfo.h"
#include "XexResourceDumper.h"
#include "stdlib.h"

XexGameInfo::XexGameInfo()
{
}

XexGameInfo::~XexGameInfo()
{
}


bool XexGameInfo::GetSPA(const Xex& xex, DataBlock& data) const
{
	ExecutionId exec_id;
	if( !xex.getExecutionId(exec_id) )
		return false;
	char title_id_str[32];
	sprintf(title_id_str, "%08X", exec_id.titleId);
	
	XexResourceDumper res_dumper(xex);
	s32 index = res_dumper.indexFromName(title_id_str);
	if(index < 0)
		return false;
	
	u8* res_data = NULL;
	int res_size = 0;
	if( !res_dumper.dump(index, res_data, res_size) )
		return false;
	
	data.set(res_data, 0, res_size);
	delete[] res_data;
	return true;
}


// SPA Decryption code by elsemi, thanks mate!
#pragma pack(push,1)
typedef struct {
	u16 ChunkType;
	u32 Unk;
	u32 ID;		//Data: Name, Icon: ID, String: Locale
	s32 offset;
	s32 size;
} DataChunk;
#pragma pack(pop)

bool XexGameInfo::SPAGetGameName(const DataBlock& spaData, char* gameName, int gameNameLen) const
{
	strncpy(gameName, "", gameNameLen);
	
	int num_entries = spaData.get32be(8);
	
	// Extract the icon and string from the chunk table
	DataChunk strings_chunk;
	DataChunk icon_chunk;
	icon_chunk.offset = 0;
	strings_chunk.offset = 0;
	for(int i=0; i<num_entries; i++)
	{
		DataChunk chunk;
		chunk.ChunkType	= spaData.get16be( 0x18 + i*sizeof(DataChunk) + offsetof(DataChunk, ChunkType) );
		chunk.Unk		= spaData.get32be( 0x18 + i*sizeof(DataChunk) + offsetof(DataChunk, Unk) );
		chunk.ID		= spaData.get32be( 0x18 + i*sizeof(DataChunk) + offsetof(DataChunk, ID) );
		chunk.offset	= spaData.get32be( 0x18 + i*sizeof(DataChunk) + offsetof(DataChunk, offset) );
		chunk.size		= spaData.get32be( 0x18 + i*sizeof(DataChunk) + offsetof(DataChunk, size) );
		
		switch(chunk.ChunkType)
		{
//		case 0x01:
//			printf("Data Chunk %c%c%c%c. Offset 0x%x size 0x%x\n",(Data.ID>>24)&0xFF,(Data.ID>>16)&0xFF,(Data.ID>>8)&0xFF,Data.ID&0xFF,Data.offset,Data.size);
//			break;
		case 0x02:
//			printf("Icon %X. Offset 0x%x size 0x%x\n",Data.ID,Data.offset,Data.size);
			if(chunk.ID == 0x8000)	//Main Icon
				icon_chunk = chunk;
			break;
		case 0x03:
//			printf("String Locale %d. Offset 0x%x size 0x%x\n",Data.ID,Data.offset,Data.size);
			if(chunk.ID == 0x01)	//English Locals
				strings_chunk = chunk;
			break;
		default:
//			printf("unknown chunk\n");
			break;
		}
	}

	// Find the base offset;
	s32 base = 0;
	for(int i=num_entries*sizeof(DataChunk); i<spaData.size()-4; i++)
	{
		u32 data = spaData.get32be(i);
		if(data == 0x58544844) // "XTHD"
		{
			base = i;
			break;
		}
	}
	if(base == 0)
		return false;
	
	// Now Extract the game name
	if(strings_chunk.offset == 0)
		return false;
	for(int i=0; i<strings_chunk.size-4; i++)
	{
		u32 data = spaData.get32be(base+strings_chunk.offset+i);
		if(data == 0x58535452) // "XSTR"
		{
			s32 str_offset = base+strings_chunk.offset+i+12;
			s32 num_strings = spaData.get16be(str_offset);
			str_offset += 2;
			for(int str_idx=0; str_idx<num_strings; str_idx++)
			{
				s32 str_code = spaData.get16be(str_offset+0);
				s32 str_size = spaData.get16be(str_offset+2);
				if(str_code == 0x8000)
				{
					int name_size = ((gameNameLen-1) < (str_size) ? (gameNameLen-1) : (str_size));
					spaData.get(gameName, str_offset+4, name_size);
					gameName[name_size] = '\0';
					break;
				}
				str_offset += str_size + 4;
			}
			break;
		}
	}
	
	return true;
}

bool XexGameInfo::SPAGetGameIcon(const DataBlock& spaData, DataBlock& gameIcon) const
{
	int num_entries = spaData.get32be(8);
	
	// Extract the icon and string from the chunk table
	DataChunk strings_chunk;
	DataChunk icon_chunk;
	icon_chunk.offset = 0;
	strings_chunk.offset = 0;
	for(int i=0; i<num_entries; i++)
	{
		DataChunk chunk;
		chunk.ChunkType	= spaData.get16be( 0x18 + i*sizeof(DataChunk) + offsetof(DataChunk, ChunkType) );
		chunk.Unk		= spaData.get32be( 0x18 + i*sizeof(DataChunk) + offsetof(DataChunk, Unk) );
		chunk.ID		= spaData.get32be( 0x18 + i*sizeof(DataChunk) + offsetof(DataChunk, ID) );
		chunk.offset	= spaData.get32be( 0x18 + i*sizeof(DataChunk) + offsetof(DataChunk, offset) );
		chunk.size		= spaData.get32be( 0x18 + i*sizeof(DataChunk) + offsetof(DataChunk, size) );
		
		switch(chunk.ChunkType)
		{
//		case 0x01:
//			printf("Data Chunk %c%c%c%c. Offset 0x%x size 0x%x\n",(Data.ID>>24)&0xFF,(Data.ID>>16)&0xFF,(Data.ID>>8)&0xFF,Data.ID&0xFF,Data.offset,Data.size);
//			break;
		case 0x02:
//			printf("Icon %X. Offset 0x%x size 0x%x\n",Data.ID,Data.offset,Data.size);
			if(chunk.ID == 0x8000)	//Main Icon
				icon_chunk = chunk;
			break;
		case 0x03:
//			printf("String Locale %d. Offset 0x%x size 0x%x\n",Data.ID,Data.offset,Data.size);
			if(chunk.ID == 0x01)	//English Locals
				strings_chunk = chunk;
			break;
		default:
//			printf("unknown chunk\n");
			break;
		}
	}

	// Find the base offset;
	s32 base = 0;
	for(int i=num_entries*sizeof(DataChunk); i<spaData.size()-4; i++)
	{
		u32 data = spaData.get32be(i);
		if(data == 0x58544844) // "XTHD"
		{
			base = i;
			break;
		}
	}
	if(base == 0)
		return false;
	
	// Extract main icon
	if(icon_chunk.offset == 0)
		return false;
	gameIcon.set(spaData, base+icon_chunk.offset, 0, icon_chunk.size);
	return true;
}

bool XexGameInfo::GameName(const Xex& xex, char* name, int nameLen) const
{
	DataBlock spa_data;
	if( !GetSPA(xex, spa_data) ||
		!SPAGetGameName(spa_data, name, nameLen) )
		return false;
	return true;
}
bool XexGameInfo::GameIcon(const Xex& xex, DataBlock& gameIcon) const
{
	DataBlock spa_data;
	if( !GetSPA(xex, spa_data) ||
		!SPAGetGameIcon(spa_data, gameIcon) )
		return false;
	return true;
}
	

