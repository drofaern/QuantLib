/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2003 Neil Firth
 Copyright (C) 2003 Ferdinando Ametrano
 Copyright (C) 2007 StatPro Italia srl

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

#include <ql/instruments/autocall/touchoption.hpp>
#include <ql/instruments/impliedvolatility.hpp>
#include <ql/pricingengines/autocall/fdblackscholestouchengine.hpp>
#include <memory>

namespace QuantLib {
    TouchOption::TouchOption(Touch::Type touchType,
                    std::vector<Real> barrierHigh,
                    std::vector<Real> barrierLow,
                    std::vector<Real> rebateHigh,
                    std::vector<Real> rebateLow,
                    bool payoffAtExpiry,
                    const ext::shared_ptr<Exercise>& exercise)
    : OneAssetOption(ext::shared_ptr<StrikedTypePayoff>(new PlainVanillaPayoff(Option::Type::Call, 1e9)), exercise),
      touchType_(touchType), barrierHigh_(std::move(barrierHigh)), barrierLow_(std::move(barrierLow)),
      rebateHigh_(std::move(rebateHigh)), rebateLow_(std::move(rebateLow)), payoffAtExpiry_(payoffAtExpiry){}

    void TouchOption::setupArguments(PricingEngine::arguments* args) const {

        OneAssetOption::setupArguments(args);

        auto* moreArgs = dynamic_cast<TouchOption::arguments*>(args);
        QL_REQUIRE(moreArgs != nullptr, "wrong argument type");
        moreArgs->touchType = touchType_;
        moreArgs->barrierHigh = std::move(barrierHigh_);
        moreArgs->barrierLow = std::move(barrierLow_);
        moreArgs->rebateHigh = std::move(rebateHigh_);
        moreArgs->rebateLow = std::move(rebateLow_);
        moreArgs->payoffAtExpiry = payoffAtExpiry_;
    }


    // Volatility TouchOption::impliedVolatility(
    //          Real targetValue,
    //          const ext::shared_ptr<GeneralizedBlackScholesProcess>& process,
    //          Real accuracy,
    //          Size maxEvaluations,
    //          Volatility minVol,
    //          Volatility maxVol) const {

    //     QL_REQUIRE(!isExpired(), "option expired");

    //     ext::shared_ptr<SimpleQuote> volQuote(new SimpleQuote);

    //     ext::shared_ptr<GeneralizedBlackScholesProcess> newProcess =
    //         detail::ImpliedVolatilityHelper::clone(process, volQuote);

    //     // engines are built-in for the time being
    //     std::unique_ptr<PricingEngine> engine;
    //     switch (exercise_->type()) {
    //       case Exercise::European:
    //         engine.reset(new AnalyticBarrierEngine(newProcess));
    //         break;
    //       case Exercise::American:
    //       case Exercise::Bermudan:
    //         QL_FAIL("engine not available for non-European barrier option");
    //         break;
    //       default:
    //         QL_FAIL("unknown exercise type");
    //     }

    //     return detail::ImpliedVolatilityHelper::calculate(*this,
    //                                                       *engine,
    //                                                       *volQuote,
    //                                                       targetValue,
    //                                                       accuracy,
    //                                                       maxEvaluations,
    //                                                       minVol, maxVol);
    // }


    TouchOption::arguments::arguments()
    : touchType(Touch::Type(-1)) {}

    void TouchOption::arguments::validate() const {
        // OneAssetOption::arguments::validate();

        // switch (barrierType) {
        //   case Barrier::DownIn:
        //   case Barrier::UpIn:
        //   case Barrier::DownOut:
        //   case Barrier::UpOut:
        //     break;
        //   default:
        //     QL_FAIL("unknown type");
        // }

        // QL_REQUIRE(barrier != Null<Real>(), "no barrier given");
        // QL_REQUIRE(rebate != Null<Real>(), "no rebate given");
    }

    bool TouchOption::engine::triggered(Real underlying) const {
        // switch (arguments_.barrierType) {
        //   case Barrier::DownIn:
        //   case Barrier::DownOut:
        //     return underlying < arguments_.barrier;
        //   case Barrier::UpIn:
        //   case Barrier::UpOut:
        //     return underlying > arguments_.barrier;
        //   default:
        //     QL_FAIL("unknown type");
        // }
    }

}

