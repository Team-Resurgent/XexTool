// 
// handles extraction of special info from an xex
// 

#ifndef _XEX_GAME_INFO_H_
#define _XEX_GAME_INFO_H_

#include "Xex.h"


class XexGameInfo
{
public:
	XexGameInfo();
	~XexGameInfo();
	
	bool GameName(const Xex& xex, char* name, int nameLen) const;
	bool GameIcon(const Xex& xex, DataBlock& data) const;
	
private:
	bool GetSPA(const Xex& xex, DataBlock& data) const;
	bool SPAGetGameIcon(const DataBlock& spaData, DataBlock& gameIcon) const;
	bool SPAGetGameName(const DataBlock& spaData, char* gameName, int gameNameLen) const;
};


#endif // _XEX_GAME_INFO_H_

