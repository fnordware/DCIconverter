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
// DCIconverter.cpp
//
// Classes to perform RGB to XYZ conversions
//
// ------------------------------------------------------------------------


#include "DCIconverter.h"


DCIconverterBase::DCIconverterBase(ChromaticAdaptation adapt, int temperature) :
	_rgb2xyz_matrix( RGBtoXYZmatrix(adapt, temperature) )
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
DCIconverterBase::RGBtoXYZmatrix(const XYZvalue *endWhite)
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
	static const Matrix bradfordCPM = bradfordCPM_spec.transposed();
	static const Matrix inverseBradfordCPM = inverseBradfordCPM_spec.transposed();
	
	
	// if we're not doing chromatic adaptation, just return the sRGBtoXYZ matrix
	if(endWhite == NULL)
		return sRGBtoXYZ;
	
	
	// sRGB/Rec. 709 uses the D65 white point, which has these xyY coordinates
	// which we convert to XYZ and use as our source white point.
	static const double D65wp_x = 0.3127;
	static const double D65wp_y = 0.3290;
	static const double D65wp_Y = 1.0000;
	
	XYZvalue D65white(	(D65wp_Y / D65wp_y) * D65wp_x,
						D65wp_Y,
						(D65wp_Y / D65wp_y) * (1 - D65wp_x - D65wp_y) );
	
	
	
	// Calculate the chromatic adaptation matrix using the Bradford method.
	// We must do this because sRGB and Rec. 709 use the D65 white point,
	// which is roughly a 6504K color temperature.  But many projector bulbs are
	// warmer/redder (which ironically means they have a lower color temperature).
	Imath::V3f ratio( (*endWhite * bradfordCPM) / (D65white * bradfordCPM) );

	Matrix ratioMat
		(ratio[0], 0,        0,       
		 0,        ratio[1], 0,       
		 0,        0,        ratio[2]);
	
	Matrix chromaticAdaptation = bradfordCPM * ratioMat * inverseBradfordCPM;
	
	
	// Our final sRGB/Rec. 709 to DCI XYZ matrix is the product of the
	// sRGB->XYZ and chromatic adaptation matrices.  That's what we return.
	
	return sRGBtoXYZ * chromaticAdaptation;
}


DCIconverterBase::Matrix
DCIconverterBase::RGBtoXYZmatrix(ChromaticAdaptation adapt, int temperature)
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
			
			if(adapt == D55)
			{
				x = 0.3324;
				y = 0.3474;
				Y = 1.0000;
			}
			else if(adapt == D65)
			{
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
	
	return RGBtoXYZmatrix(whitePoint);
}


ForwardDCIconverter::ForwardDCIconverter(ResponseCurve curve, float gamma,
											ChromaticAdaptation adapt, int temperature,
											float xyz_gamma) :
	DCIconverterBase(adapt, temperature),
	_curve(curve),
	_gamma(gamma),
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
	else if(_curve == Gamma)
	{
		r = GammaFunc(r, _gamma);
		g = GammaFunc(g, _gamma);
		b = GammaFunc(b, _gamma);
	}

	
	// Convert to XYZ
	Pixel xyz = rgb * _rgb2xyz_matrix;
	
	
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


ReverseDCIconverter::ReverseDCIconverter(ResponseCurve curve, float gamma,
											ChromaticAdaptation adapt, int temperature,
											float xyz_gamma) :
	DCIconverterBase(adapt, temperature),
	_curve(curve),
	_gamma(gamma),
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
	else if(_curve == Gamma)
	{
		r = GammaFunc(r, 1.f / _gamma);
		g = GammaFunc(g, 1.f / _gamma);
		b = GammaFunc(b, 1.f / _gamma);
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
