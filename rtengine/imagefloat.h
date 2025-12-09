/* -*- C++ -*-
 *
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RawTherapee is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RawTherapee.  If not, see <http://www.gnu.org/licenses/>.
 */
//
// A class representing a 16 bit rgb image with separate planes and 16 byte
// aligned data
//
#ifndef _IMAGEFLOAT_
#define _IMAGEFLOAT_

#include "imageio.h"
#include "labimage.h"
#include "rtengine.h"

namespace rtengine {
using namespace procparams;

class Image8;
class Image16;

/*
 * Image type used by most tools; expected range: [0.0 ; 65535.0]
 */
class Imagefloat: public IImagefloat, public ImageIO {
public:
    Imagefloat();
    Imagefloat(int width, int height, const Imagefloat *state_from = nullptr);
    ~Imagefloat() override;

    Imagefloat *copy() const;
    void copyTo(Imagefloat *dst) const;

    Image8 *to8() const;
    Image16 *to16() const;

    void getStdImage(const ColorTemp &ctemp, int tran, Imagefloat *image,
                     PreviewProps pp) const override;

    const char *getType() const override { return sImagefloat; }

    int getBPS() const override { return 8 * sizeof(float); }

    void getScanline(int row, unsigned char *buffer, int bps,
                     bool isFloat = false) const override;
    void setScanline(int row, unsigned char *buffer, int bps,
                     unsigned int numSamples) override;

    // functions inherited from IImagefloat:
    MyMutex &getMutex() override { return mutex(); }
    cmsHPROFILE getProfile() const override { return getEmbeddedProfile(); }
    int getBitsPerPixel() const override { return 8 * sizeof(float); }
    int saveToFile(const Glib::ustring &fname) const override
    {
        return save(fname);
    }
    int saveAsPNG(const Glib::ustring &fname, int bps = -1,
                  bool uncompressed = false) const override
    {
        return savePNG(fname, bps, uncompressed);
    }
    int saveAsJPEG(const Glib::ustring &fname, int quality = 100,
                   int subSamp = 3) const override
    {
        return saveJPEG(fname, quality, subSamp);
    }
    int saveAsTIFF(const Glib::ustring &fname, int bps = -1,
                   bool isFloat = false,
                   bool uncompressed = false) const override
    {
        return saveTIFF(fname, bps, isFloat, uncompressed);
    }
    void setSaveProgressListener(ProgressListener *pl) override
    {
        setProgressListener(pl);
    }
    void free() override { delete this; }

    void multiply(float factor, bool multithread);
    void normalizeFloatTo1(bool multithread = true);
    void normalizeFloatTo65535(bool multithread = true);
    void calcCroppedHistogram(const ProcParams &params, float scale,
                              LUTu &hist);
    void ExecCMSTransform(cmsHTRANSFORM hTransform, bool multithread);
    void ExecCMSTransform(cmsHTRANSFORM hTransform, const Imagefloat *img,
                          bool multithread);

    enum class Mode {
        RGB = 0, // r = red, g = green, b = blue
        XYZ,     // r = X, g = Y, b = Z
        YUV,     // g = Y, b = U, r = V; NOTE: U and V are not normalized wrt Y
        LAB      // g = L, r = A, b = B
    };

    Mode mode() const { return mode_; }
    const Glib::ustring colorSpace() const { return color_space_; }

    void assignColorSpace(const Glib::ustring &space);
    void setMode(Mode mode, bool multithread);
    void assignMode(Mode mode) { mode_ = mode; }

    void copyState(Imagefloat *to) const;

    void toLab(LabImage &dst, bool multithread);
    void getLab(int y, int x, float &L, float &a, float &b);

private:
    void rgb_to_xyz(bool multithread);
    void rgb_to_yuv(bool multithread);
    void rgb_to_lab(bool multithread);
    void xyz_to_rgb(bool multithread);
    void xyz_to_yuv(bool multithread);
    void xyz_to_lab(bool multithread);
    void yuv_to_rgb(bool multithread);
    void yuv_to_xyz(bool multithread);
    void yuv_to_lab(bool multithread);
    void lab_to_rgb(bool multithread);
    void lab_to_xyz(bool multithread);
    void lab_to_yuv(bool multithread);
    void rgb_to_lab(int y, int x, float &L, float &a, float &b);
    void xyz_to_lab(int y, int x, float &L, float &a, float &b);
    void yuv_to_lab(int y, int x, float &L, float &a, float &b);
    void get_ws();

    Glib::ustring color_space_;
    Mode mode_;
    float ws_[3][3];
    float iws_[3][3];
#ifdef __SSE2__
    vfloat vws_[3][3];
    vfloat viws_[3][3];
#endif
};

} // namespace rtengine
#endif
