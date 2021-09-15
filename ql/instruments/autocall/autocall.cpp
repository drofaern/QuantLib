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

#include <ql/instruments/autocall/autocall.hpp>
#include <ql/instruments/impliedvolatility.hpp>
#include <ql/pricingengines/autocall/mcautocallengine.hpp>
#include <ql/exercise.hpp>
#include <memory>

namespace QuantLib {

    Autocall::Autocall(
        Real rebate,
        Real coupon,
        std::vector<Date> fixingDates,
        Real kiBarrier,
        Real koBarrier,
        Real margin,
        const ext::shared_ptr<StrikedTypePayoff>& kiPayoff,
        const ext::shared_ptr<Exercise>& exercise)
    : OneAssetOption(kiPayoff, exercise),
      rebate_(rebate), coupon_(coupon), fixingDates_(std::move(fixingDates)), kiBarrier_(kiBarrier), koBarrier_(koBarrier), margin_(margin){}

    void Autocall::setupArguments(PricingEngine::arguments* args) const {

        OneAssetOption::setupArguments(args);

        auto* moreArgs = dynamic_cast<Autocall::arguments*>(args);
        QL_REQUIRE(moreArgs != nullptr, "wrong argument type");
        moreArgs->rebate = rebate_;
        moreArgs->coupon = coupon_;
        moreArgs->kiBarrier = kiBarrier_;
        moreArgs->koBarrier = koBarrier_;
        moreArgs->fixingDates = fixingDates_;
        moreArgs->margin = margin_;
    }

    Autocall::arguments::arguments()
    : rebate(Null<Real>()),coupon(Null<Real>()),kiBarrier(Null<Real>()),koBarrier(Null<Real>()) {}

    void Autocall::arguments::validate() const {
        OneAssetOption::arguments::validate();

        QL_REQUIRE(rebate != Null<Real>(), "no rebate given");
        QL_REQUIRE(coupon != Null<Real>(), "no coupon given");
        QL_REQUIRE(kiBarrier != Null<Real>(), "no kiBarrier given");
        QL_REQUIRE(koBarrier != Null<Real>(), "no koBarrier given");
    }

    bool Autocall::engine::triggered(Real underlying) const {
        //TODO
    }
}

