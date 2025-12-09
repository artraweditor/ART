/*  -*- C++ -*-
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
#pragma once

#include "../rtengine/procparams.h"
#include "../rtengine/rtengine.h"
#include "mydiagonalcurve.h"
#include "myflatcurve.h"
#include "paramsedited.h"
#include <vector>

class Clipboard {
private:
    bool _hasIPTC;
    rtengine::procparams::IPTCPairs iptc;
    std::unique_ptr<rtengine::procparams::ProcParams> pparams;
    ParamsEdited pedited;
    bool has_pparams_;
    bool has_pedited_;
    DiagonalCurveType hasDiagonalCurveDataType;
    FlatCurveType hasFlatCurveDataType;
    std::vector<double> diagonalCurve;
    std::vector<double> flatCurve;
    rtengine::procparams::AreaMask areaMask;
    rtengine::procparams::DrawnMask drawnMask;
    rtengine::procparams::Mask mask;

public:
    void setIPTC(const rtengine::procparams::IPTCPairs &iptcc)
    {
        iptc = iptcc;
        _hasIPTC = true;
    }

    const rtengine::procparams::IPTCPairs &getIPTC() { return iptc; }

    bool hasIPTC() { return _hasIPTC; }

    void setProcParams(const rtengine::procparams::ProcParams &pp)
    {
        if (!pparams) {
            pparams.reset(new rtengine::procparams::ProcParams());
        }
        *pparams = pp;
        has_pparams_ = true;
    }

    const rtengine::procparams::ProcParams &getProcParams()
    {
        if (!pparams) {
            pparams.reset(new rtengine::procparams::ProcParams());
        }
        return *pparams;
    }

    void setParamsEdited(const ParamsEdited &pe)
    {
        pedited = pe;
        has_pedited_ = true;
    }

    const ParamsEdited &getParamsEdited() { return pedited; }

    bool hasProcParams() { return has_pparams_; }

    bool hasPEdited() { return has_pedited_; }

    void setDiagonalCurveData(std::vector<double> &p, DiagonalCurveType type)
    {
        diagonalCurve = p;
        hasDiagonalCurveDataType = type;
        return;
    }

    const std::vector<double> &getDiagonalCurveData() { return diagonalCurve; }

    DiagonalCurveType hasDiagonalCurveData()
    {
        return hasDiagonalCurveDataType;
    }

    void setFlatCurveData(std::vector<double> &p, FlatCurveType type)
    {
        flatCurve = p;
        hasFlatCurveDataType = type;
        return;
    }

    const std::vector<double> &getFlatCurveData() { return flatCurve; }

    FlatCurveType hasFlatCurveData() { return hasFlatCurveDataType; }

    bool hasAreaMask() { return !areaMask.isTrivial(); }

    bool hasDrawnMask() { return !drawnMask.isTrivial(); }

    const rtengine::procparams::AreaMask &getAreaMask() { return areaMask; }

    const rtengine::procparams::DrawnMask &getDrawnMask() { return drawnMask; }

    void setAreaMask(const rtengine::procparams::AreaMask &am)
    {
        areaMask = am;
    }

    void setDrawnMask(const rtengine::procparams::DrawnMask &dm)
    {
        drawnMask = dm;
    }

    bool hasMask() const { return mask != rtengine::procparams::Mask(); }

    const rtengine::procparams::Mask &getMask() { return mask; }

    void setMask(const rtengine::procparams::Mask &m) { mask = m; }

    Clipboard();
    ~Clipboard();
};

extern Clipboard clipboard;
