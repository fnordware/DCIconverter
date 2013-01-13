///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2013, Brendan Bolles
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////

// ------------------------------------------------------------------------
//
// DCIconverter_AE.h
//
// After Effects plug-in to perform RGB to XYZ conversions
//
// ------------------------------------------------------------------------


#ifndef INCLUDED_DCI_CONVERTER_AE_H
#define INCLUDED_DCI_CONVERTER_AE_H


#include "AEConfig.h"
#include "entry.h"
#include "SPTypes.h"
#include "PrSDKAESupport.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "AE_Effect.h"
#include "AE_EffectUI.h"
#include "AE_EffectCB.h"


#ifdef MSWindows
    #include <Windows.h>
#else 
    #ifndef __MACH__
        #include "string.h"
    #endif
#endif  


// Versioning information

#define NAME				"DCI Converter"
#define DESCRIPTION			"Convert RGB to XYZ"
#define RELEASE_DATE		__DATE__
#define AUTHOR				"by Brendan Bolles"
#define COPYRIGHT			"\xA9 2013 fnord"
#define WEBSITE				"www.fnordware.com"
#define	MAJOR_VERSION		0
#define	MINOR_VERSION		5
#define	BUG_VERSION			0
#define	STAGE_VERSION		PF_Stage_RELEASE
#define	BUILD_VERSION		0


// Paramater constants
enum {
    DCI_INPUT = 0,
	DCI_OPERATION,
	DCI_CURVE,
	DCI_GAMMA,
	DCI_RGB_COLOR_SPACE,
	DCI_ADAPTATION,
	DCI_TEMPERATURE,
	DCI_XYZ_GAMMA,
    
    DCI_NUM_PARAMS
};

enum {
	DCI_OPERATION_ID = 1,
	DCI_CURVE_ID,
	DCI_GAMMA_ID,
	DCI_ADAPTATION_ID,
	DCI_TEMPERATURE_ID,
	DCI_XYZ_GAMMA_ID,
	DCI_RGB_COLOR_SPACE_ID
};


// Menus
enum {
	OPERATION_RGB_TO_XYZ = 1,
	OPERATION_XYZ_TO_RGB,
	
	OPERATION_NUM_OPTIONS = OPERATION_XYZ_TO_RGB
};

#define OPERATION_MENU_STR "RGB to XYZ|XYZ to RGB"


enum {
	CURVE_sRGB = 1,
	CURVE_Rec709,
	CURVE_ProPhotoRGB,
	CURVE_Linear,
	CURVE_GAMMA,
	
	CURVE_NUM_OPTIONS = CURVE_GAMMA
};

#define CURVE_MENU_STR "sRGB|Rec. 709|ProPhoto RGB|Linear|Gamma"


enum {
	RGB_COLOR_SPACE_sRGB = 1,
	RGB_COLOR_SPACE_PROPHOTO,
	
	RGB_COLOR_SPACE_NUM_OPTIONS = RGB_COLOR_SPACE_PROPHOTO
};

#define RGB_COLOR_SPACE_MENU_STR "sRGB / Rec. 709|ProPhoto RGB / ROMM RGB"


enum {
	ADAPTATION_NONE = 1,
	ADAPTATION_D50,
	ADAPTATION_D55,
	ADAPTATION_D65,
	ADAPTATION_TEMPERATURE,
	
	ADAPTATION_NUM_OPTIONS = ADAPTATION_TEMPERATURE
};

#define ADAPTATION_MENU_STR "None|D50|D55|D65|Color Temperature"




#ifdef __cplusplus
	extern "C" {
#endif


DllExport	PF_Err 
PluginMain (	
	PF_Cmd			cmd,
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	void			*extra) ;


#ifdef __cplusplus
	}
#endif


#endif // INCLUDED_DCI_CONVERTER_AE_H