///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2013, Brendan Bolles
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *	   Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *	   Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *	   Neither the name of Industrial Light & Magic nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission. 
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
// DCIconverter_AE.cpp
//
// After Effects plug-in to perform RGB to XYZ conversions
//
// ------------------------------------------------------------------------


#include "DCIconverter_AE.h"

#include "DCIconverter.h"


#include "AEGP_SuiteHandler.h"


static PF_Err 
About(	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_SPRINTF( out_data->return_msg, 
				"%s - %s\r\rwritten by %s\r\rv%d.%d - %s\r\r%s\r%s",
				NAME,
				DESCRIPTION,
				AUTHOR, 
				MAJOR_VERSION, 
				MINOR_VERSION,
				RELEASE_DATE,
				COPYRIGHT,
				WEBSITE);
				
	return PF_Err_NONE;
}


static PF_Err 
GlobalSetup (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	out_data->my_version	=	PF_VERSION( MAJOR_VERSION, 
											MINOR_VERSION,
											BUG_VERSION, 
											STAGE_VERSION, 
											BUILD_VERSION);

	out_data->out_flags		=	PF_OutFlag_DEEP_COLOR_AWARE		|
								PF_OutFlag_PIX_INDEPENDENT		|
								PF_OutFlag_USE_OUTPUT_EXTENT	|
								PF_OutFlag_SEND_UPDATE_PARAMS_UI;

	out_data->out_flags2	=	PF_OutFlag2_SUPPORTS_SMART_RENDER	|
								PF_OutFlag2_FLOAT_COLOR_AWARE;
	
	if(in_data->appl_id == 'PrMr')
	{
		PF_PixelFormatSuite1 *pfS = NULL;
		
		in_data->pica_basicP->AcquireSuite(kPFPixelFormatSuite,
											kPFPixelFormatSuiteVersion1,
											(const void **)&pfS);
											
		if(pfS)
		{
			pfS->ClearSupportedPixelFormats(in_data->effect_ref);
			
			pfS->AddSupportedPixelFormat(in_data->effect_ref, PrPixelFormat_BGRA_4444_8u);
			pfS->AddSupportedPixelFormat(in_data->effect_ref, PrPixelFormat_BGRA_4444_16u);
			pfS->AddSupportedPixelFormat(in_data->effect_ref, PrPixelFormat_BGRA_4444_32f);
			
			in_data->pica_basicP->ReleaseSuite(kPFPixelFormatSuite,
												kPFPixelFormatSuiteVersion1);
		}
	}
	
	return PF_Err_NONE;
}


static PF_Err
ParamsSetup(
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output)
{
	PF_Err			err = PF_Err_NONE;
	
	PF_ParamDef		def;


	AEFX_CLR_STRUCT(def);
	PF_ADD_POPUP(	"Operation",
					OPERATION_NUM_OPTIONS, //number of choices
					OPERATION_RGB_TO_XYZ, //default
					OPERATION_MENU_STR,
					DCI_OPERATION_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_POPUP(	"Response Curve",
					CURVE_NUM_OPTIONS,
					CURVE_Rec709,
					CURVE_MENU_STR,
					DCI_CURVE_ID);
					
	AEFX_CLR_STRUCT(def);
	def.flags = PF_ParamFlag_COLLAPSE_TWIRLY;
	PF_ADD_FLOAT_SLIDER("Gamma",
					0.01, 5.0, 0.01, 5.0, // ranges
					0, // curve tolderance?
					2.2, // default
					2, 0, 0, // precision, display flags, want phase
					DCI_GAMMA_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_POPUP(	"Chromatic Adaptation",
					ADAPTATION_NUM_OPTIONS,
					ADAPTATION_TEMPERATURE,
					ADAPTATION_MENU_STR,
					DCI_ADAPTATION_ID);
					
	AEFX_CLR_STRUCT(def);
	def.flags = PF_ParamFlag_COLLAPSE_TWIRLY;
	PF_ADD_SLIDER(	"Color Temperature (K)",
					4000, 25000,
					4000, 25000,
					5900,
					DCI_TEMPERATURE_ID);
	
	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDER("XYZ Gamma",
					0.01, 5.0, 0.01, 5.0,
					0,
					2.6,
					2, 0, 0,
					DCI_XYZ_GAMMA_ID);
	
	
	out_data->num_params = DCI_NUM_PARAMS;
	
	return err;
}


static PF_Err
UpdateParamsUI(
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output)
{
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	
	
	PF_ParamValue curveP		= params[DCI_CURVE]->u.pd.value;
	PF_ParamValue adaptationP	= params[DCI_ADAPTATION]->u.pd.value;
	
	bool gamma_visible = (curveP == CURVE_GAMMA);
	bool gamma_is_visible = !(params[DCI_GAMMA]->ui_flags & PF_PUI_DISABLED);
	
	bool temp_visible = (adaptationP == ADAPTATION_TEMPERATURE);
	bool temp_is_visible = !(params[DCI_TEMPERATURE]->ui_flags & PF_PUI_DISABLED);
	
	
	if(gamma_visible != gamma_is_visible)
	{
		if(gamma_visible)
			params[DCI_GAMMA]->ui_flags &= ~PF_PUI_DISABLED;
		else
			params[DCI_GAMMA]->ui_flags |= PF_PUI_DISABLED;
			
		suites.ParamUtilsSuite1()->PF_UpdateParamUI(in_data->effect_ref, DCI_GAMMA, params[DCI_GAMMA]);
	}

	if(temp_visible != temp_is_visible)
	{
		if(temp_visible)
			params[DCI_TEMPERATURE]->ui_flags &= ~PF_PUI_DISABLED;
		else
			params[DCI_TEMPERATURE]->ui_flags |= PF_PUI_DISABLED;
			
		suites.ParamUtilsSuite1()->PF_UpdateParamUI(in_data->effect_ref, DCI_TEMPERATURE, params[DCI_TEMPERATURE]);
	}


	return PF_Err_NONE;
}


static PF_Boolean IsEmptyRect(const PF_LRect *r){
	return (r->left >= r->right) || (r->top >= r->bottom);
}

#ifndef mmin
	#define mmin(a,b) ((a) < (b) ? (a) : (b))
	#define mmax(a,b) ((a) > (b) ? (a) : (b))
#endif

static void UnionLRect(const PF_LRect *src, PF_LRect *dst)
{
	if (IsEmptyRect(dst)) {
		*dst = *src;
	} else if (!IsEmptyRect(src)) {
		dst->left	= mmin(dst->left, src->left);
		dst->top	= mmin(dst->top, src->top);
		dst->right	= mmax(dst->right, src->right);
		dst->bottom = mmax(dst->bottom, src->bottom);
	}
}

static PF_Err PreRender(
	PF_InData				*in_data,
	PF_OutData				*out_data,
	PF_PreRenderExtra		*extra)
{
	PF_Err err = PF_Err_NONE;
	PF_RenderRequest req = extra->input->output_request;
	PF_CheckoutResult in_result;
	
	req.preserve_rgb_of_zero_alpha = TRUE;

	ERR(extra->cb->checkout_layer(	in_data->effect_ref,
									DCI_INPUT,
									DCI_INPUT,
									&req,
									in_data->current_time,
									in_data->time_step,
									in_data->time_scale,
									&in_result));


	UnionLRect(&in_result.result_rect,		&extra->output->result_rect);
	UnionLRect(&in_result.max_result_rect,	&extra->output->max_result_rect);	
	
	return err;
}


template <typename CHAN_TYPE>
static inline float ConvertToFloat(CHAN_TYPE in);

template <>
static inline float ConvertToFloat(A_u_char in)
{
	return (float)in / (float)PF_MAX_CHAN8;
}

template <>
static inline float ConvertToFloat(A_u_short in)
{
	return (float)in / (float)PF_MAX_CHAN16;
}

template <>
static inline float ConvertToFloat(PF_FpShort in)
{
	return in;
}


static inline float Clamp(float in)
{
	return (in >= 1.f ? 1.f : in <= 0.f ? 0.f : in);
}

template <typename CHAN_TYPE>
static inline CHAN_TYPE ConvertToAE(float in);

template <>
static inline A_u_char ConvertToAE(float in)
{
	return ( Clamp(in) * (float)PF_MAX_CHAN8 ) + 0.5f;
}

template <>
static inline A_u_short ConvertToAE(float in)
{
	return ( Clamp(in) * (float)PF_MAX_CHAN16 ) + 0.5f;
}

template <>
static inline PF_FpShort ConvertToAE(float in)
{
	return in;
}


template <typename T>
struct PremierePixel {
	T blue;
	T green;
	T red;
	T alpha;
};


typedef struct {
	A_long				width;
	DCIconverterBase	*converter;
} ProcessData;


template <typename AE_PIXTYPE, typename WP_PIXTYPE, typename CHAN_TYPE>
static PF_Err
ProcessRow(
	void			*refcon, 
	A_long			x, 
	A_long			y, 
	AE_PIXTYPE		*inP, 
	AE_PIXTYPE		*outP)
{
	ProcessData *p_data = (ProcessData *)refcon;
	
	WP_PIXTYPE *in = (WP_PIXTYPE *)inP;
	WP_PIXTYPE *out = (WP_PIXTYPE *)outP;
	
	for(int x=0; x < p_data->width; x++)
	{
		Pixel inpix;
		
		inpix[0] = ConvertToFloat( in->red );
		inpix[1] = ConvertToFloat( in->green );
		inpix[2] = ConvertToFloat( in->blue );
		
		
		Pixel outpix = p_data->converter->convert(inpix);
		
		
		out->red   = ConvertToAE<CHAN_TYPE>( outpix[0] );
		out->green = ConvertToAE<CHAN_TYPE>( outpix[1] );
		out->blue  = ConvertToAE<CHAN_TYPE>( outpix[2] );
		
		out->alpha = in->alpha;
		
		in++;
		out++;
	}

	return PF_Err_NONE;
}



static PF_Err DoRender(
	PF_InData		*in_data,
	PF_EffectWorld	*input,
	PF_ParamDef		*DCI_operation,
	PF_ParamDef		*DCI_curve,
	PF_ParamDef		*DCI_gamma,
	PF_ParamDef		*DCI_adaptation,
	PF_ParamDef		*DCI_temperature,
	PF_ParamDef		*DCI_xyz_gamma,
	PF_OutData		*out_data,
	PF_EffectWorld	*output)
{
	PF_Err				err		= PF_Err_NONE;

	AEGP_SuiteHandler suites(in_data->pica_basicP);
	
	
	PF_PixelFormatSuite1 *pfS = NULL;
	PF_WorldSuite2 *wsP = NULL;
	
	// Premiere pixel info
	in_data->pica_basicP->AcquireSuite(kPFPixelFormatSuite, kPFPixelFormatSuiteVersion1, (const void **)&pfS);
  
	ERR( in_data->pica_basicP->AcquireSuite(kPFWorldSuite, kPFWorldSuiteVersion2, (const void **)&wsP) );
	
	
	// Get the pixel format
	PF_PixelFormat format = PF_PixelFormat_INVALID;			
			
	ERR( wsP->PF_GetPixelFormat(output, &format) );
	
	if(in_data->appl_id == 'PrMr' && pfS)
	{
		// the regular world suite function will give a bogus value for Premiere
		ERR( pfS->GetPixelFormat(output, (PrPixelFormat *)&format) );
	}
	
	
	
	if(!err)
	{
		try
		{
			PF_Point			origin;
			PF_Rect				src_rect, areaR;
			
			origin.h = output->origin_x;
			origin.v = output->origin_y;

			src_rect.left	= -in_data->output_origin_x;
			src_rect.top	= -in_data->output_origin_y;
			src_rect.bottom = src_rect.top + output->height;
			src_rect.right	= src_rect.left + output->width;

			areaR.top		= 0;
			areaR.left		= 0;
			areaR.right		= 1;
			areaR.bottom	= output->height;
			
			
			PF_ParamValue operation		= DCI_operation->u.pd.value;
			PF_ParamValue curveP		= DCI_curve->u.pd.value;
			PF_FpLong gamma				= DCI_gamma->u.fs_d.value;
			PF_ParamValue adaptationP	= DCI_adaptation->u.pd.value;
			PF_ParamValue temperature	= DCI_temperature->u.sd.value;
			PF_FpLong xyz_gamma			= DCI_xyz_gamma->u.fs_d.value;
			
			
			DCIconverterBase::ResponseCurve curve = curveP == CURVE_sRGB ? DCIconverterBase::sRGB :
													curveP == CURVE_Rec709 ? DCIconverterBase::Rec709 :
													DCIconverterBase::Gamma;
													
			DCIconverterBase::ChromaticAdaptation adaptation =	adaptationP == ADAPTATION_NONE ? DCIconverterBase::None :
																adaptationP == ADAPTATION_D55 ? DCIconverterBase::D55 :
																adaptationP == ADAPTATION_D65 ? DCIconverterBase::D65 :
																DCIconverterBase::Temp;
			
			DCIconverterBase *converter = NULL;
			
			if(operation == OPERATION_XYZ_TO_RGB)
			{
				converter = new ReverseDCIconverter(curve, gamma, adaptation, temperature, xyz_gamma);
			}
			else
			{
				converter = new ForwardDCIconverter(curve, gamma, adaptation, temperature, xyz_gamma);
			}
			
			
			if(converter)
			{
				ProcessData p_data = { output->width, converter };
				
			
				if(format == PF_PixelFormat_ARGB32)
				{
					err = suites.Iterate8Suite1()->iterate_origin(in_data,
																	0,
																	output->height,
																	input,
																	&areaR,
																	&origin,
																	&p_data,
																	ProcessRow<PF_Pixel, PF_Pixel, A_u_char>,
																	output);
				}
				else if(format == PF_PixelFormat_ARGB64)
				{
					err = suites.Iterate16Suite1()->iterate_origin(in_data,
																	0,
																	output->height,
																	input,
																	&areaR,
																	&origin,
																	&p_data,
																	ProcessRow<PF_Pixel16, PF_Pixel16, A_u_short>,
																	output);
				}
				else if(format == PF_PixelFormat_ARGB128)
				{
					err = suites.IterateFloatSuite1()->iterate_origin(in_data,
																	0,
																	output->height,
																	input,
																	&areaR,
																	&origin,
																	&p_data,
																	ProcessRow<PF_Pixel32, PF_Pixel32, PF_FpShort>,
																	output);
				}
				if(format == PrPixelFormat_BGRA_4444_8u)
				{
					err = suites.Iterate8Suite1()->iterate_origin(in_data,
																	0,
																	output->height,
																	input,
																	&areaR,
																	&origin,
																	&p_data,
																	ProcessRow<PF_Pixel, PremierePixel<A_u_char>, A_u_char>,
																	output);
				}
				else if(format == PrPixelFormat_BGRA_4444_16u)
				{
					err = suites.Iterate16Suite1()->iterate_origin(in_data,
																	0,
																	output->height,
																	input,
																	&areaR,
																	&origin,
																	&p_data,
																	ProcessRow<PF_Pixel16, PremierePixel<A_u_short>, A_u_short>,
																	output);
				}
				else if(format == PrPixelFormat_BGRA_4444_32f)
				{
					err = suites.IterateFloatSuite1()->iterate_origin(in_data,
																	0,
																	output->height,
																	input,
																	&areaR,
																	&origin,
																	&p_data,
																	ProcessRow<PF_Pixel32, PremierePixel<PF_FpShort>, PF_FpShort>,
																	output);
				}
				
				
				delete converter;
			}
		}
		catch(...) { err = PF_Err_INTERNAL_STRUCT_DAMAGED; }
	}
	
	
	if(pfS)
		in_data->pica_basicP->ReleaseSuite(kPFPixelFormatSuite, kPFPixelFormatSuiteVersion1);
	
	if(wsP)
		in_data->pica_basicP->ReleaseSuite(kPFWorldSuite, kPFWorldSuiteVersion2);
		

	return err;
}


static PF_Err
SmartRender(
	PF_InData				*in_data,
	PF_OutData				*out_data,
	PF_SmartRenderExtra		*extra)
{

	PF_Err			err		= PF_Err_NONE,
					err2	= PF_Err_NONE;
					
	PF_EffectWorld *input, *output;
	
	PF_ParamDef DCI_operation,
				DCI_curve,
				DCI_gamma,
				DCI_adaptation,
				DCI_temperature,
				DCI_xyz_gamma;

	// zero-out parameters
	AEFX_CLR_STRUCT(DCI_operation);
	AEFX_CLR_STRUCT(DCI_curve);
	AEFX_CLR_STRUCT(DCI_adaptation);
	AEFX_CLR_STRUCT(DCI_gamma);
	AEFX_CLR_STRUCT(DCI_temperature);
	AEFX_CLR_STRUCT(DCI_xyz_gamma);
	

	// checkout input & output buffers.
	ERR(	extra->cb->checkout_layer_pixels( in_data->effect_ref, DCI_INPUT, &input)	);
	ERR(	extra->cb->checkout_output( in_data->effect_ref, &output)	);


	// bail before param checkout
	if(err)
		return err;

#define PF_CHECKOUT_PARAM_NOW( PARAM, DEST )	PF_CHECKOUT_PARAM(	in_data, (PARAM), in_data->current_time, in_data->time_step, in_data->time_scale, DEST )

	// checkout the required params
	ERR(	PF_CHECKOUT_PARAM_NOW( DCI_OPERATION,		&DCI_operation )	);
	ERR(	PF_CHECKOUT_PARAM_NOW( DCI_CURVE,			&DCI_curve )	);
	ERR(	PF_CHECKOUT_PARAM_NOW( DCI_GAMMA,			&DCI_gamma )	);
	ERR(	PF_CHECKOUT_PARAM_NOW( DCI_ADAPTATION,		&DCI_adaptation )	);
	ERR(	PF_CHECKOUT_PARAM_NOW( DCI_TEMPERATURE,		&DCI_temperature )	);
	ERR(	PF_CHECKOUT_PARAM_NOW( DCI_XYZ_GAMMA,		&DCI_xyz_gamma )	);

	ERR(	DoRender(	in_data, 
						input, 
						&DCI_operation, 
						&DCI_curve,
						&DCI_gamma,
						&DCI_adaptation,
						&DCI_temperature,
						&DCI_xyz_gamma,
						out_data, 
						output) );

	// Always check in, no matter what the error condition!
	ERR2(	PF_CHECKIN_PARAM(in_data, &DCI_operation )	);
	ERR2(	PF_CHECKIN_PARAM(in_data, &DCI_curve )	);
	ERR2(	PF_CHECKIN_PARAM(in_data, &DCI_gamma )	);
	ERR2(	PF_CHECKIN_PARAM(in_data, &DCI_adaptation ) );
	ERR2(	PF_CHECKIN_PARAM(in_data, &DCI_temperature )	);
	ERR2(	PF_CHECKIN_PARAM(in_data, &DCI_xyz_gamma )	);


	return err;
}


static PF_Err Render(
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	return DoRender(in_data,
					&params[DCI_INPUT]->u.ld,
					params[DCI_OPERATION],
					params[DCI_CURVE],
					params[DCI_GAMMA],
					params[DCI_ADAPTATION],
					params[DCI_TEMPERATURE],
					params[DCI_XYZ_GAMMA],
					out_data,
					output);
}


DllExport	
PF_Err 
PluginMain (	
	PF_Cmd			cmd,
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	void			*extra)
{
	PF_Err		err = PF_Err_NONE;
	
	try {
		switch (cmd) {
			case PF_Cmd_ABOUT:
				err = About(in_data, out_data, params, output);
				break;
			case PF_Cmd_GLOBAL_SETUP:
				err = GlobalSetup(in_data, out_data, params, output);
				break;
			case PF_Cmd_PARAMS_SETUP:
				err = ParamsSetup(in_data, out_data, params, output);
				break;
			case PF_Cmd_UPDATE_PARAMS_UI:
				err = UpdateParamsUI(in_data, out_data, params, output);
				break;
			case PF_Cmd_SMART_PRE_RENDER:
				err = PreRender(in_data, out_data, (PF_PreRenderExtra*)extra);
				break;
			case PF_Cmd_SMART_RENDER:
				err = SmartRender(in_data, out_data, (PF_SmartRenderExtra*)extra);
				break;
			case PF_Cmd_RENDER:
				err = Render(in_data, out_data, params, output);
				break;
		}
	}
	catch(PF_Err &thrown_err) { err = thrown_err; }
	catch(...) { err = PF_Err_INTERNAL_STRUCT_DAMAGED; }
	
	return err;
}

