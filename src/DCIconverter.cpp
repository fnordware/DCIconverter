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
// DCIconverter.cpp
//
// Classes to perform RGB to XYZ conversions
//
// ------------------------------------------------------------------------


#include "DCIconverter.h"

#include <assert.h>


DCIconverterBase::DCIconverterBase(ColorSpace color, ChromaticAdaptation adapt, int temperature) :
	_rgb2xyz_matrix( RGBtoXYZmatrix(color, adapt, temperature) )
{

}


DCIconverterBase::XYZvalue
DCIconverterBase::TemperatureToWhite(int temperature)
{
	// Convert a color temperature (i.e. 5900K) to an XYZ value.
	
    // taken from the Little CMS file cmswtpnt.c
    double x, y, Y;
    
    double T = temperature;
    double T2 = T * T;
    double T3 = T2 * T;
    
    if(T >= 4000.0 && T <= 7000.0)
    {
        x = -4.6070*(1E9/T3) + 2.9678*(1E6/T2) + 0.09911*(1E3/T) + 0.244063;
    }
    else if (T > 7000.0 && T <= 25000.0)
    {
        x = -2.0064*(1E9/T3) + 1.9018*(1E6/T2) + 0.24748*(1E3/T) + 0.237040;
    }
    else
        throw Iex::LogicExc("Invalid temperature");
    
    y = -3.000*(x*x) + 2.870*x - 0.275;
    
    Y = 1.0;
    
	
	XYZvalue wp;
	
    wp.x = (Y / y) * x;
    wp.y = Y;
    wp.z = (Y / y) * (1 - x - y);
	
	return wp;
}


DCIconverterBase::Matrix
DCIconverterBase::RGBtoXYZmatrix(ColorSpace color, const XYZvalue *endWhite)
{
	// This matrix is part of the sRGB standard.
	// http://en.wikipedia.org/wiki/SRGB
	// http://www.color.org/chardata/rgb/srgb.xalter
	
	// sRGB and Rec. 709 use the same color primaries,
	// so we use the same matrix to convert linear sRGB/Rec. 709
	// to linear XYZ.
	
	static const Matrix sRGBtoXYZ_spec
		(0.4124, 0.3576, 0.1805,
		 0.2126, 0.7152, 0.0722,
		 0.0193, 0.1192, 0.9505);
	

	// The ProPhoto RGB spec gives only the XYZ->RGB matrix.
	// Just remember to use the inverse of this for RGB->XYZ.
	// http://www.color.org/ROMMRGB.pdf
	
	static const Matrix XYZtoProPhotoRGB_spec
		(1.3460, -0.2556, -0.0511,
		-0.5446,  1.5082,  0.0205,
		 0.0000,  0.0000,  1.2123);
		 
	
	// DCI-P3 space (SMPTE RP 431-2, formerly DCI Spec Table 11)
	// Also:
	// http://www.hp.com/united-states/campaigns/workstations/pdfs/lp2480zx-dci--p3-emulation.pdf

	static const float P3_r_x = 0.680;	static const float P3_r_y = 0.320;
	static const float P3_g_x = 0.265;	static const float P3_g_y = 0.690;
	static const float P3_b_x = 0.150;	static const float P3_b_y = 0.060;
	static const float P3_w_x = 0.314;	static const float P3_w_y = 0.351;

	// chromaticities to RGBtoXYZ matrix steps taken from
	// http://www.ryanjuckett.com/programming/graphics/27-rgb-color-space-conversion?start=6
	
	static const float P3_r_z = 1.f - P3_r_x - P3_r_y;
	static const float P3_g_z = 1.f - P3_g_x - P3_g_y;
	static const float P3_b_z = 1.f - P3_b_x - P3_b_y;
	static const float P3_w_z = 1.f - P3_w_x - P3_w_y;
	
	static const XYZvalue P3_w_XYZ = (1.f / P3_w_y) * Imath::V3f(P3_w_x, P3_w_y, P3_w_z);
	
	
	static const Matrix P3_chromaticity_mat
		(P3_r_x, P3_g_x, P3_b_x,
		 P3_r_y, P3_g_y, P3_b_y,
		 P3_r_z, P3_g_z, P3_b_z);
	
		 
	static const Imath::V3f P3_RGB_XYZ_sum = P3_w_XYZ * P3_chromaticity_mat.transposed().inverse();
	
	static const Matrix P3_RGB_XYZ_sum_mat
		(P3_RGB_XYZ_sum[0], 0,        0,       
		 0,        P3_RGB_XYZ_sum[1], 0,       
		 0,        0,        P3_RGB_XYZ_sum[2]);
	
	
	static const Matrix P3toXYZ_spec = P3_chromaticity_mat * P3_RGB_XYZ_sum_mat;
	
	
	// To calculate the chromatic adaptation matrix, we use the Bradford method.
	// The Bradford method is also used by ICC profiles.
	// Here's the Bradford Cone Primary Matrix and its inverse.
	// http://www.brucelindbloom.com/index.html?Eqn_ChromAdapt.html
	
	static const Matrix bradfordCPM_spec
		(0.895100,  0.266400, -0.161400,
		-0.750200,  1.713500,  0.036700,
		 0.038900, -0.068500,  1.029600);
	
	static const Matrix inverseBradfordCPM_spec
		(0.986993, -0.147054,  0.159963,
		 0.432305,  0.518360,  0.049291,
		-0.008529,  0.040043,  0.968487);
	
	
	// Imath matrix math is column-major, so we use the transpose of these matrices.
	static const Matrix sRGBtoXYZ = sRGBtoXYZ_spec.transposed();
	static const Matrix ProPhotoRGBtoXYZ = XYZtoProPhotoRGB_spec.inverse().transposed();
	static const Matrix P3toXYZ = P3toXYZ_spec.transposed();
	static const Matrix bradfordCPM = bradfordCPM_spec.transposed();
	static const Matrix inverseBradfordCPM = inverseBradfordCPM_spec.transposed();
	
	
	const Matrix RGBtoXYZ = (color == ProPhotoRGB_ROMM ? ProPhotoRGBtoXYZ :
							 color == P3_RGB ? P3toXYZ :
							 sRGBtoXYZ );
	
	
	// if we're not doing chromatic adaptation, just return the RGBtoXYZ matrix
	if(endWhite == NULL)
		return RGBtoXYZ;
	
	
	// sRGB/Rec. 709 uses the D65 white point, ProPhoto RGB uses D50. P3 is roughly 5900K?
	// We convert to XYZ and use as our source white point.
	double wp_x, wp_y, wp_Y;
	
	if(color == ProPhotoRGB_ROMM)
	{
		// D50
		wp_x = 0.3457;
		wp_y = 0.3585;
		wp_Y = 1.0000;
	}
	else if(color == P3_RGB)
	{
		wp_x = P3_w_x;
		wp_y = P3_w_y;
		wp_Y = 1.0000;
	}
	else
	{
		// D65
		wp_x = 0.3127;
		wp_y = 0.3290;
		wp_Y = 1.0000;
	}
	
	XYZvalue white(	(wp_Y / wp_y) * wp_x,
					wp_Y,
					(wp_Y / wp_y) * (1 - wp_x - wp_y) );
	
	
	
	// Calculate the chromatic adaptation matrix using the Bradford method.
	// We must do this because sRGB and Rec. 709 use the D65 white point,
	// which is roughly a 6504K color temperature.  But many projector bulbs are
	// warmer/redder (which ironically means they have a lower color temperature).
	Imath::V3f ratio( (*endWhite * bradfordCPM) / (white * bradfordCPM) );

	Matrix ratioMat
		(ratio[0], 0,        0,       
		 0,        ratio[1], 0,       
		 0,        0,        ratio[2]);
	
	Matrix chromaticAdaptation = bradfordCPM * ratioMat * inverseBradfordCPM;
	
	
	// Our final sRGB/Rec. 709 to DCI XYZ matrix is the product of the
	// sRGB->XYZ and chromatic adaptation matrices.  That's what we return.
	
	return RGBtoXYZ * chromaticAdaptation;
}


DCIconverterBase::Matrix
DCIconverterBase::RGBtoXYZmatrix(ColorSpace color, ChromaticAdaptation adapt, int temperature)
{
	XYZvalue *whitePoint = NULL;
	
	XYZvalue whiteValue;
	
	if(adapt != None)
	{
		whitePoint = &whiteValue;
		
		if(adapt == Temp)
		{
			whiteValue = TemperatureToWhite(temperature);
		}
		else
		{
			float x, y, Y;
			
			// Some links for chromaticity values:
			// http://hackage.haskell.org/packages/archive/colour/2.3.3/doc/html/src/Data-Colour-CIE-Illuminant.html
			// https://en.wikipedia.org/wiki/Standard_illuminant#White_points_of_standard_illuminants
			// http://www.filmlight.ltd.uk/pdf/whitepapers/FL-TL-TN-0417-StdColourSpaces.pdf
			// http://nbviewer.ipython.org/github/colour-science/colour-ipython/blob/master/notebooks/colorimetry/illuminants.ipynb
			
			if(adapt == D50)
			{
				x = 0.3457;
				y = 0.3585;
				Y = 1.0000;
			}
			else if(adapt == D55)
			{
				x = 0.3324;
				y = 0.3474;
				Y = 1.0000;
			}
			else if(adapt == D60)
			{
				x = 0.3217;
				y = 0.3378; 
				Y = 1.0000;
			}
			else if(adapt == DCI)
			{
				x = 0.314;
				y = 0.351;
				Y = 1.000;
			}
			else
			{
				assert(adapt == D65);
			
				x = 0.3127;
				y = 0.3290;
				Y = 1.0000;
			}
			
			// convert xyY to XYZ
			whiteValue = XYZvalue(	(Y / y) * x,
									Y,
									(Y / y) * (1 - x - y) );
		}
	}
	
	return RGBtoXYZmatrix(color, whitePoint);
}


ForwardDCIconverter::ForwardDCIconverter(ResponseCurve curve, float gamma,
											ColorSpace color, ChromaticAdaptation adapt, int temperature,
											bool normalize, float xyz_gamma) :
	DCIconverterBase(color, adapt, temperature),
	_curve(curve),
	_gamma(gamma),
	_normalize(normalize),
	_xyz_gamma(xyz_gamma),
	_rgb2xyz_matrix( DCIconverterBase::_rgb2xyz_matrix )
{

}


static inline float
GammaFunc(float in, float gamma)
{
	return (in < 0.f ? -powf(-in, gamma) : powf(in, gamma) );
}


Pixel
ForwardDCIconverter::convert(const Pixel &pix) const
{
	Pixel rgb = pix;
	
	float &r = rgb[0];
	float &g = rgb[1];
	float &b = rgb[2];
	
	
	// Video RGB to linear RGB (R'G'B' to RGB)
	if(_curve == sRGB)
	{
		r = sRGBtoLin(r);
		g = sRGBtoLin(g);
		b = sRGBtoLin(b);
	}
	else if(_curve == Rec709)
	{
		r = Rec709toLin(r);
		g = Rec709toLin(g);
		b = Rec709toLin(b);
	}
	else if(_curve == ProPhotoRGB)
	{
		r = ProPhotoRGBtoLin(r);
		g = ProPhotoRGBtoLin(g);
		b = ProPhotoRGBtoLin(b);
	}
	else if(_curve == P3)
	{
		r = GammaFunc(r, 2.6f);
		g = GammaFunc(g, 2.6f);
		b = GammaFunc(b, 2.6f);
	}
	else if(_curve == Gamma)
	{
		r = GammaFunc(r, _gamma);
		g = GammaFunc(g, _gamma);
		b = GammaFunc(b, _gamma);
	}
	else
	{
		assert(_curve == Linear);
	}

	
	// Convert to XYZ
	Pixel xyz = rgb * _rgb2xyz_matrix;
	
	
	// normalize
	if(_normalize)
	{
		xyz.x *= 48.f / 52.37f;
		xyz.y *= 48.f / 52.37f;
		xyz.z *= 48.f / 52.37f;
	}
	
	
	// XYZ to X'Y'Z'
	xyz.x = GammaFunc(xyz.x, 1.f / _xyz_gamma);
	xyz.y = GammaFunc(xyz.y, 1.f / _xyz_gamma);
	xyz.z = GammaFunc(xyz.z, 1.f / _xyz_gamma);
	
	
	return xyz;
}


inline float
ForwardDCIconverter::sRGBtoLin(float in)
{
	// sRGB to linear transfer function
	// http://en.wikipedia.org/wiki/SRGB
	// http://www.color.org/chardata/rgb/srgb.xalter
	
	return (in <= 0.04045f ? (in / 12.92f) : powf( (in + 0.055f) / 1.055f, 2.4f));
}


inline float
ForwardDCIconverter::Rec709toLin(float in)
{
	// Rec. 709 to linear transfer function
	// http://en.wikipedia.org/wiki/Rec._709
	// http://www.poynton.com/notes/colour_and_gamma/GammaFAQ.html#gamma_correction
	
	return (in <= 0.081f ? (in / 4.5f) : powf( (in + 0.099f) / 1.099f, 1.0f / 0.45f));
}


inline float
ForwardDCIconverter::ProPhotoRGBtoLin(float in)
{
	// ProPhotoRGB to linear transfer function.  Inverse of LinToProPhotoRGB spec.
	// http://en.wikipedia.org/wiki/ProPhoto_RGB_color_space
	// http://www.color.org/ROMMRGB.pdf
	
	return (in < 0.031248f ? (in / 16.f) : powf(in, 1.8f));
}


ReverseDCIconverter::ReverseDCIconverter(ResponseCurve curve, float gamma,
											ColorSpace color, ChromaticAdaptation adapt, int temperature,
											bool normalize, float xyz_gamma) :
	DCIconverterBase(color, adapt, temperature),
	_curve(curve),
	_gamma(gamma),
	_normalize(normalize),
	_xyz_gamma(xyz_gamma),
	_xyz2rgb_matrix( DCIconverterBase::_rgb2xyz_matrix.inverse() )
{

}


Pixel
ReverseDCIconverter::convert(const Pixel &pix) const
{
	Pixel xyz = pix;


	// X'Y'Z' to XYZ
	xyz.x = GammaFunc(xyz.x, _xyz_gamma);
	xyz.y = GammaFunc(xyz.y, _xyz_gamma);
	xyz.z = GammaFunc(xyz.z, _xyz_gamma);
	
	
	// de-normalize
	if(_normalize)
	{
		xyz.x *= 52.37f / 48.f;
		xyz.y *= 52.37f / 48.f;
		xyz.z *= 52.37f / 48.f;
	}
	
	
	// Convert XYZ to RGB
	Pixel rgb = xyz * _xyz2rgb_matrix;
	
	
	float &r = rgb[0];
	float &g = rgb[1];
	float &b = rgb[2];
	
	
	// Linear RGB to Video RGB (RGB to R'G'B')
	if(_curve == sRGB)
	{
		r = LinTosRGB(r);
		g = LinTosRGB(g);
		b = LinTosRGB(b);
	}
	else if(_curve == Rec709)
	{
		r = LinToRec709(r);
		g = LinToRec709(g);
		b = LinToRec709(b);
	}
	else if(_curve == ProPhotoRGB)
	{
		r = LinToProPhotoRGB(r);
		g = LinToProPhotoRGB(g);
		b = LinToProPhotoRGB(b);
	}
	else if(_curve == P3)
	{
		r = GammaFunc(r, 1.f / 2.6f);
		g = GammaFunc(g, 1.f / 2.6f);
		b = GammaFunc(b, 1.f / 2.6f);
	}
	else if(_curve == Gamma)
	{
		r = GammaFunc(r, 1.f / _gamma);
		g = GammaFunc(g, 1.f / _gamma);
		b = GammaFunc(b, 1.f / _gamma);
	}
	else
	{
		assert(_curve == Linear);
	}
	
	
	return rgb;
}


inline float
ReverseDCIconverter::LinTosRGB(float in)
{
	// linear to sRGB transfer function
	
	return (in <= 0.0031308f ? (in * 12.92f) : 1.055f * powf(in, 1.f / 2.4f) - 0.055f);
}


inline float
ReverseDCIconverter::LinToRec709(float in)
{
	// linear to Rec. 709 transfer function
	
	return (in <= 0.018f ? (in * 4.5f) : 1.099f * powf(in, 0.45f) - 0.099f);
}


inline float
ReverseDCIconverter::LinToProPhotoRGB(float in)
{
	// linear to ProPhoto RGB transfer function
	
	return (in < 0.001953f ? (in * 16.f) : powf(in, 1.f / 1.8f));
}

