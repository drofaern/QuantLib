/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2003, 2004 Neil Firth
 Copyright (C) 2003, 2004 Ferdinando Ametrano
 Copyright (C) 2003, 2004, 2007 StatPro Italia srl

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it
 under the terms of the QuantLib license.  You should have received a
 copy of the license along with this program; if not, please email
 <quantlib-dev@lists.sf.net>. The license is also available online at
 <http://quantlib.org/license.shtml>.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

/*! \file touchoption.hpp
    \brief touch option on a single asset
*/

#ifndef quantlib_touch_option_hpp
#define quantlib_touch_option_hpp

#include <ql/instruments/oneassetoption.hpp>
#include <ql/instruments/payoffs.hpp>
#include <ql/exercise.hpp>

namespace QuantLib {

    class GeneralizedBlackScholesProcess;

    struct Touch {
        enum Type { OneTouchUp, OneTouchDown, NoTouchUp, NoTouchDown, DoubleOneTouch, DoubleNoTouch };
    };
    //! %Touch option on a single asset.
    /*! The finite difference pricing engine will be used if none is passed.

        \ingroup instruments
    */


    class TouchOption : public OneAssetOption {
      public:
        class arguments;
        class engine;
        TouchOption(Touch::Type touchType,
                    std::vector<Real> barrierHigh,
                    std::vector<Real> barrierLow,
                    std::vector<Real> rebateHigh,
                    std::vector<Real> rebateLow,
                    bool payoffAtExpiry,
                    const ext::shared_ptr<Exercise>& exercise);
        void setupArguments(PricingEngine::arguments*) const override;
      protected:
        // arguments
        Touch::Type touchType_;
        std::vector<Real> barrierHigh_;
        std::vector<Real> barrierLow_;
        std::vector<Real> rebateHigh_;
        std::vector<Real> rebateLow_;
        bool payoffAtExpiry_;

    };

    //! %Arguments for touch option calculation
    class TouchOption::arguments : public OneAssetOption::arguments {
      public:
        arguments();
        Touch::Type touchType;
        std::vector<Real> barrierHigh;
        std::vector<Real> barrierLow;
        std::vector<Real> rebateHigh;
        std::vector<Real> rebateLow;
        bool payoffAtExpiry;
        void validate() const override;
    };

    //! %Touch-option %engine base class
    class TouchOption::engine
        : public GenericEngine<TouchOption::arguments,
                               TouchOption::results> {
      protected:
        bool triggered(Real underlying) const;
    };

}

#endif
