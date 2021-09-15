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

/*! \file autocall.hpp
    \brief Autocall on a single asset
*/

#ifndef quantlib_autocall_hpp
#define quantlib_autocall_hpp

#include <ql/instruments/oneassetoption.hpp>
#include <ql/instruments/payoffs.hpp>

namespace QuantLib {

    class GeneralizedBlackScholesProcess;

    //! %Autocall on a single asset.
    /*! The analytic pricing engine will be used if none if passed.

        \ingroup instruments
    */
    class Autocall : public OneAssetOption {
      public:
        class arguments;
        class engine;
        Autocall(   Real rebate,
                    Real coupon,
                    std::vector<Date> fixingDates,
                    Real kiBarrier,
                    Real koBarrier,
                    Real margin,
                    const ext::shared_ptr<StrikedTypePayoff>& kiPayoff,
                    const ext::shared_ptr<Exercise>& exercise);

        void setupArguments(PricingEngine::arguments*) const override;
        /*! \warning see VanillaOption for notes on implied-volatility
                     calculation.
        */
        // Volatility impliedVolatility(
        //      Real price,
        //      const ext::shared_ptr<GeneralizedBlackScholesProcess>& process,
        //      Real accuracy = 1.0e-4,
        //      Size maxEvaluations = 100,
        //      Volatility minVol = 1.0e-7,
        //      Volatility maxVol = 4.0) const;
      protected:
        // arguments
        Real rebate_;
        Real coupon_;
        std::vector<Date> fixingDates_;
        Real kiBarrier_;
        Real koBarrier_;
        Real margin_;
        ext::shared_ptr<StrikedTypePayoff> kiPayoff_;
        ext::shared_ptr<Exercise> exercise;
    };

    //! %Arguments for barrier option calculation
    class Autocall::arguments : public OneAssetOption::arguments {
      public:
        arguments();
        
        Real rebate;
        Real coupon;
        Real kiBarrier;
        Real koBarrier;
        Real margin;
        std::vector<Date> fixingDates;
        void validate() const override;
    };

    //! %Barrier-option %engine base class
    class Autocall::engine
        : public GenericEngine<Autocall::arguments,
                               Autocall::results> {
      protected:
        bool triggered(Real underlying) const;
    };

}

#endif
