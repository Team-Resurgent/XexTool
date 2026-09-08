//
// Small shims over XeCrypt.
//
// XexTool passes const buffers to functions this XeCrypt declares as taking
// mutable ones. Rather than casting at each call site, provide const-qualified
// overloads that forward.
//
#pragma once

#include "XeCrypt.h"

inline void XeCryptRotSumSha(const unsigned char* pbInp1, unsigned int cbInp1,
                             const unsigned char* pbInp2, unsigned int cbInp2,
                             unsigned char* pbOut, unsigned int cbOut)
{
	XeCryptRotSumSha(const_cast<unsigned char*>(pbInp1), cbInp1,
	                 const_cast<unsigned char*>(pbInp2), cbInp2,
	                 pbOut, cbOut);
}

inline void XeCryptShaUpdate(PXECRYPT_SHA_STATE pShaState,
                             const unsigned char* pbInp, unsigned int cbInp)
{
	XeCryptShaUpdate(pShaState, const_cast<unsigned char*>(pbInp), cbInp);
}
