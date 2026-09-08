// 
// special xex patches for particular games and xexs
// 

#ifndef _SPECIAL_PATCHES_H_
#define _SPECIAL_PATCHES_H_

#include "Xex.h"


// perform special patches on the xex
bool DoSpecialPatches(Xex& xex, u32 patchNum, FILE* printStream=NULL);

// perform a fix upon an update-patched xex
bool DoUpdatePatchFix(Xex& xex, FILE* printStream=NULL);

// remove discswap checks
bool DoDiscSwapChecksFix(Xex& xex, FILE* printStream=NULL);


#endif // _SPECIAL_PATCHES_H_

