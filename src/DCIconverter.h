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
// *       Neither the name of Industrial Light & Magic nor the names of
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
// DCIconverter.h
//
// Classes to perform RGB to XYZ conversions
//
// ------------------------------------------------------------------------

#ifndef INCLUDED_DCI_CONVERTER_H
#define INCLUDED_DCI_CONVERTER_H


#include "ImathMatrix.h"


typedef Imath::V3f Pixel;


class DCIconverterBase
{
  public:
	typedef enum {
		sRGB_Rec709,
		ProPhotoRGB_ROMM
	} ColorSpace;
  
	typedef enum {
		sRGB,
		Rec709,
		ProPhotoRGB,
		Gamma
	} ResponseCurve;
	
	typedef enum {
		None,
		D50,
		D55,
		D65,
		Temp
	} ChromaticAdaptation;
  
	DCIconverterBase(ColorSpace color, ChromaticAdaptation adapt, int temperature);
						
	virtual ~DCIconverterBase() {}
	
	
	virtual Pixel convert(const Pixel &pix) const = 0;
	
	
  protected:
	typedef Imath::M33f Matrix;
	Matrix _rgb2xyz_matrix;


  private:
	typedef Imath::V3f XYZvalue;
	static XYZvalue TemperatureToWhite(int temperature);
	
	static Matrix RGBtoXYZmatrix(ColorSpace color, const XYZvalue *endWhite);
	static Matrix RGBtoXYZmatrix(ColorSpace color, ChromaticAdaptation adapt, int temperature);
};


class ForwardDCIconverter : public DCIconverterBase
{
  public:
	ForwardDCIconverter(ResponseCurve curve, float gamma,
						ColorSpace color, ChromaticAdaptation adapt, int temperature,
						float xyz_gamma);
						
	virtual ~ForwardDCIconverter() {}
	
	virtual Pixel convert(const Pixel &pix) const;
	
  private:
	ResponseCurve _curve;
	float _gamma;
	float _xyz_gamma;
	Matrix _rgb2xyz_matrix;
	
  private:
	static inline float sRGBtoLin(float in);
	static inline float Rec709toLin(float in);
	static inline float ProPhotoRGBtoLin(float in);
};


class ReverseDCIconverter : public DCIconverterBase
{
  public:
	ReverseDCIconverter(ResponseCurve curve, float gamma,
						ColorSpace color, ChromaticAdaptation adapt, int temperature,
						float xyz_gamma);
						
	virtual ~ReverseDCIconverter() {}
	
	virtual Pixel convert(const Pixel &pix) const;
	
  private:
	ResponseCurve _curve;
	float _gamma;
	float _xyz_gamma;
	Matrix _xyz2rgb_matrix;
	
  private:
	static inline float LinTosRGB(float in);
	static inline float LinToRec709(float in);
	static inline float LinToProPhotoRGB(float in);
};


#endif // INCLUDED_DCI_CONVERTER_H
