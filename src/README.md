DCI Converter
=============

Some code for converting RGB images to XYZ and back again.

This all started because people were using my [**j2k**](http://www.fnordware.com/j2k) plug-in for Premiere Pro to write out [Digital Cinema](http://en.wikipedia.org/wiki/Digital_Cinema_Package) frames, which are JPEG 2000 images in XYZ color space. Some questioned the way I was doing my XYZ conversion, so I figured the best thing to do was make the code available for public scrutiny.

So far this project includes a program-independent converter class and an After Effects plug-in that also runs in Premiere Pro.


#####Written by

[Brendan Bolles](http://github.com/fnordware/)


#####Thanks

Lars Borg from Adobe


#####License

[BSD](http://opensource.org/licenses/BSD-2-Clause)


Installation
------------

Manually copy the plug-in to the version-specific shared Adobe plug-in folder.

**Mac:** /Library/Application\ Support/Adobe/Common/Plug-ins/CSX/MediaCore/

**Win:** C:\Program Files\Adobe\Common\Plug-ins\CSX\MediaCore\

*(replace "CSX" with the version you're using)*

#####Note

If using this plug-in with [j2k](http://www.fnordware.com/j2k) to write DCI files out of Premiere Pro, make sure to disable j2k's own XYZ conversion by checking the *Advanced* box and setting the XYZ conversion to *None*.


Color Science
-------------

These are the steps for converting an RGB image to [DCI-friendly](http://en.wikipedia.org/wiki/Digital_cinema#Technology) XYZ:

1. Convert from non-linear RGB (R'G'B') to linear RGB using a response curve
2. Convert from linear RGB to linear XYZ based on the chomaticities specified by the source RGB color space
3. Use chromatic adaptation to adjust between the illuminant white point specified by the source RGB color space and a target white point
4. Normalize the XYZ 
5. Convert from XYZ to non-linear XYZ (X'Y'Z') using a gamma curve


###Response Curve

This is the transfer function for converting between the non-linear RGB pixels that you view on your monitor and linear RGB pixels that are proportional to the amount of real-world light you see. This is often called your monitor's "gamma", but the transfer functions used in most standards are more complicated than a standard [gamma](http://en.wikipedia.org/wiki/Gamma_correction) function.

For example, sRGB is often said to use a 2.2 gamma, which would be this:

> out = in<sup>2.2</sup>

But the actual sRGB to linear curve is:

*for in <= 0.04045:*
> out = in / 12.92

*for in > 0.04045:*
> out = ((in + 0.099) / 1.099)<sup>2.4</sup>

Don't let that 2.4 term fool you; sRGB is still best approximated by a gamma of 2.2.

DCI Converter offers the following response curves:

* [**sRGB**](http://en.wikipedia.org/wiki/SRGB)
* [**Rec. 709**](http://www.poynton.com/notes/colour_and_gamma/GammaFAQ.html#gamma_correction)
* [**ProPhoto RGB**](http://en.wikipedia.org/wiki/ProPhoto_RGB_color_space)
* **DCI P3** (gamma 2.6)
* **Linear** (no response curve required, equivalent to gamma 1.0)
* **Gamma** (a regular power function using the exponent specified in the Gamma parameter)


###RGB Color Space

To convert to [XYZ color space](http://en.wikipedia.org/wiki/XYZ_color_space), we need to know the [color primaries](http://en.wikipedia.org/wiki/Primary_color) used in our RGB color space.

[**sRGB**](http://www.color.org/chardata/rgb/srgb.xalter) and [**Rec. 709**](http://en.wikipedia.org/wiki/Rec._709) use the same color primaries (but different transfer functions), so they are lumped together into one option.

[**ProPhoto RGB**](http://en.wikipedia.org/wiki/ProPhoto_RGB_color_space) (also known as [ROMM RGB](http://www.color.org/ROMMRGB.pdf)) uses different primaries, and has a much wider [gamut](http://en.wikipedia.org/wiki/Gamut).

[**DCI P3**](http://www.hp.com/united-states/campaigns/workstations/pdfs/lp2480zx-dci--p3-emulation.pdf) is an RGB space that represents the gamut of a 3-color DCI projector.

Your monitor is sRGB while a video camera shoots in Rec. 709. Actually, that's a gross oversimplification, but will have to do for this document. Getting your pixels into ProPhoto RGB usually involves [ICC Profiles](http://en.wikipedia.org/wiki/ICC_profile) in Photoshop or another program.


###Chromatic Adaptation

Your vision automatically adjusts for different lighting conditions using [chromatic adaptation](http://en.wikipedia.org/wiki/Chromatic_adaptation). An image projected in an office setting will appear more blue when viewed in a theater unless this is taken into account.

DCI Converter uses the [Bradford method](http://www.brucelindbloom.com/index.html?Eqn_ChromAdapt.html) to perform chromatic adaptation between the [illuminant](http://en.wikipedia.org/wiki/Standard_illuminant) named by your RGB Color Space and an illuminant or [**color temperature**](http://en.wikipedia.org/wiki/Color_temperature) you specify.

sRGB and Rec. 709 use [D65](http://en.wikipedia.org/wiki/Illuminant_D65) while ProPhoto RGB uses [D50](http://en.wikipedia.org/wiki/Standard_illuminant#Illuminant_series_D) as the source illuminant. P3 is roughly 5900K.

The DCI Converter plug-in defaults to using a Color Temperature of 5900K for chromatic adaptation because that's the setting used by the After Effects DCI profile, "DCDM X'Y'Z'(Gamma 2.6) 5900K (by Adobe)".


###Normalize XYZ

Conversion to XYZ for some white points, including D65, can produce XYZ values above 1.0. These would normally get clipped when converting to 12-bit. To prevent clipping, DCI scales values down by a factor of 48.0 / 52.37.

The 48.0 above comes from the DCI screen luminance level of 48 cd/m<sup>2</sup>. 52.37 is the "peak luminance," providing necessary headroom. For more details see the DCI Spec, SMPTE 428-1.


###XYZ Gamma

The DCI calls spec for its X'Y'Z' color space to have a simple 2.6 gamma.
