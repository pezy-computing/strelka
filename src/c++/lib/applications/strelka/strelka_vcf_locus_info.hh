// -*- mode: c++; indent-tabs-mode: nil; -*-
//
// Strelka - Small Variant Caller
// Copyright (c) 2009-2016 Illumina, Inc.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//

///
/// \author Chris Saunders
///

#pragma once

#include "strelkaScoringFeatures.hh"
#include "calibration/VariantScoringModel.hh"

#include <cassert>

#include <array>
#include <bitset>
#include <iosfwd>



namespace STRELKA_VCF_FILTERS
{

enum index_t
{
    // SNVs and indels:
    HighDepth,
    LowEVS,
    // SNVs only:
    BCNoise,
    SpanDel,
    QSS_ref,
    // indels only:
    Repeat,
    iHpol,
    IndelBCNoise,
    QSI_ref,
    Nonref,
    SIZE
};

inline
const char*
get_label(const unsigned idx)
{
    switch (idx)
    {
    case HighDepth:
        return "HighDepth";
    //return "DP"; // old strelka workflow name
    case BCNoise:
        return "BCNoise";
    case SpanDel:
        return "SpanDel";
    case QSS_ref:
        return "QSS_ref";
    case Repeat:
        return "Repeat";
    case iHpol:
        return "iHpol";
    case IndelBCNoise:
        return "BCNoise";
    case QSI_ref:
        return "QSI_ref";
    case LowEVS:
        return "LowEVS";
    case Nonref:
        return "Nonref";
    default:
        assert(false && "Unknown vcf filter id");
        return nullptr;
    }
}
}


struct strelka_filter_keeper
{
    strelka_filter_keeper()
    {
        clear();
    }

    void
    set(const STRELKA_VCF_FILTERS::index_t i)
    {
        if (_filters.test(i))
        {
            assert(false && "Setting filter twice");
        }
        _filters.set(i);
    }

    void
    write(
        std::ostream& os) const
    {
        if (_filters.none())
        {
            os << "PASS";
            return;
        }

        bool is_sep(false);
        for (unsigned i(0); i<STRELKA_VCF_FILTERS::SIZE; ++i)
        {
            if (! _filters.test(i)) continue;

            if (is_sep)
            {
                os << ";";
            }
            else
            {
                is_sep=true;
            }
            os << STRELKA_VCF_FILTERS::get_label(i);
        }
    }

    void
    clear()
    {
        _filters.reset();
    }

private:
    std::bitset<STRELKA_VCF_FILTERS::SIZE> _filters;
};


/// proposed simplified design to handle all featukres, for now just being used for
/// development features
///
/// better b/c
/// (1) doesn't mix up features with other tracking info
/// (2) generates no system calls after initialization
///
template<typename FEATURESET>
struct strelka_feature_keeper
{
    strelka_feature_keeper()
    {
        clear();
    }

    void
    set(const typename FEATURESET::index_t i,double val)
    {
        if (test(i))
        {
            assert(false && "Set scoring feature twice");
        }
        _featureVal[i] = val;
        _isFeatureSet.set(i);
    }

    double
    get(const typename FEATURESET::index_t i) const
    {
        if(! test(i))
        {
            assert(false && "Requesting undefined feature");
        }
        return _featureVal[i];
    }

    bool
    test(const typename FEATURESET::index_t i) const
    {
        return _isFeatureSet.test(i);
    }

    void
    write(
        std::ostream& os) const
    {
        const unsigned featureSize(FEATURESET::SIZE);
        for (unsigned featureIndex(0); featureIndex<featureSize; ++featureIndex)
        {
            if (featureIndex > 0) os << ',';
            os << FEATURESET::get_feature_label(featureIndex) << ":" << get(featureIndex);
        }
    }

    void
    clear()
    {
        _isFeatureSet.reset();
    }

private:
    std::bitset<FEATURESET::SIZE> _isFeatureSet;
    std::array<double,FEATURESET::SIZE> _featureVal;
};



template<typename _evs_featureset = STRELKA_SNV_SCORING_FEATURES,
         typename _evs_dev_featureset = STRELKA_SNV_SCORING_DEVELOPMENT_FEATURES>
struct strelka_shared_modifiers
{
    void
    set_feature(const typename _evs_featureset::index_t i,double val)
    {
        if (_isFeatureSet.test(i))
        {
            assert(false && "Set scoring feature twice");
        }
        _featureVal[i] = val;
        _isFeatureSet.set(i);
    }

    double
    get_feature(const typename _evs_featureset::index_t i) const
    {
        assert(_isFeatureSet.test(i));
        return this->_featureVal.at(i);
    }

    bool
    test_feature(const typename _evs_featureset::index_t i) const
    {
        return _isFeatureSet[i];
    }

    const feature_type&
    get_features() const
    {
        return _featureVal;
    }

    /// write features to INFO field
    void
    write_features(
        std::ostream& os) const
    {
        int ix = 0;
        for (auto const& val : _featureVal)
        {
            if (ix > 0)
            {
                os << ",";
            }
            os << _evs_featureset::get_feature_label(val.first) << ":" << val.second;
            ++ix;
        }
    }

    bool isEVS = false;
    double EVS = 0;
    strelka_filter_keeper filters;

    strelka_feature_keeper<_evs_dev_featureset> dfeatures;

private:
    std::bitset<_evs_featureset::SIZE> _isFeatureSet;
    feature_type _featureVal; // holds scoring features
};


template<class _evs_featureset>
std::ostream&
operator<<(
    std::ostream& os,
    const strelka_shared_modifiers<_evs_featureset>& shmod)
{
    os << " filters: ";
    shmod.write_filters(os);

    return os;
}

typedef strelka_shared_modifiers<STRELKA_SNV_SCORING_FEATURES, STRELKA_SNV_SCORING_DEVELOPMENT_FEATURES> strelka_shared_modifiers_snv;
typedef strelka_shared_modifiers<STRELKA_INDEL_SCORING_FEATURES, STRELKA_INDEL_SCORING_DEVELOPMENT_FEATURES> strelka_shared_modifiers_indel;

