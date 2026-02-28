/****************************************************************************
 * SaveAsCSV TDX Export DLL
 ****************************************************************************/
#pragma once

#ifndef __LINKTDX_H__
#define __LINKTDX_H__

#include <windows.h>
#include "FxIndicator.h"
 

#define MATHLIBRARY_EXPORTS
#ifdef MATHLIBRARY_EXPORTS
#define MATHLIBRARY_API __declspec(dllexport)
#else
#define MATHLIBRARY_API __declspec(dllimport)
#endif

extern "C" MATHLIBRARY_API void Func1(int nCount, float *pOut, float *pIn1, float *pIn2, float *pIn3);
extern "C" MATHLIBRARY_API void Func2(int nCount, float *pOut, float *pIn1, float *pIn2, float *pIn3);
extern "C" MATHLIBRARY_API void Func3(int nCount, float *pOut, float *pIn1, float *pIn2, float *pIn3);
extern "C" MATHLIBRARY_API void Func4(int nCount, float *pOut, float *pIn1, float *pIn2, float *pIn3);
extern "C" MATHLIBRARY_API void Func5(int nCount, float *pOut, float *pIn1, float *pIn2, float *pIn3);
extern "C" MATHLIBRARY_API void Func6(int nCount, float *pOut, float *pIn1, float *pIn2, float *pIn3);
extern "C" MATHLIBRARY_API void Func7(int nCount, float *pOut, float *pIn1, float *pIn2, float *pIn3);
extern "C" MATHLIBRARY_API void Func8(int nCount, float *pOut, float *pIn1, float *pIn2, float *pIn3);

#endif
