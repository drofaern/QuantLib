/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2003 Neil Firth
 Copyright (C) 2003 Ferdinando Ametrano
 Copyright (C) 2003, 2004, 2005 StatPro Italia srl

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

#include <ql/pricingengines/autocall/mcautocallengine.hpp>
#include <utility>

namespace QuantLib {
    AutocallPathPricer::AutocallPathPricer(  Real rebate,
                                             Real coupon,
                                             std::vector<Real> fixings,
                                             Real kiBarrier,
                                             Real koBarrier,
                                             Real strike,
                                             Real margin,
                                             std::vector<DiscountFactor> discounts,
                                             ext::shared_ptr<StochasticProcess1D> diffProcess,
                                             PseudoRandom::ursg_type sequenceGen)

                                         
    : rebate_(rebate), coupon_(coupon), fixings_(std::move(fixings)),
      kiBarrier_(kiBarrier), koBarrier_(koBarrier), margin_(margin),
      diffProcess_(std::move(diffProcess)), sequenceGen_(std::move(sequenceGen)),
      payoff_(Option::Type::Put, strike), discounts_(std::move(discounts)) {
        QL_REQUIRE(strike>=0.0,
                   "strike less than zero not allowed");
        QL_REQUIRE(kiBarrier>0.0,
                   "kibarrier less/equal zero not allowed");
        QL_REQUIRE(koBarrier>0.0,
                   "kobarrier less/equal zero not allowed");
    }


    Real AutocallPathPricer::operator()(const Path& path) const {
        static Size null = Null<Size>();
        Size n = path.length();
        QL_REQUIRE(n>1, "the path cannot be empty");

        bool isOptionActive = false;
        bool isKnockOut = false;
        int ko = 0;
        Size knockNode = 0;
        Real asset_price = path.front();
        Real new_asset_price;
        Real x, y;
        Volatility vol;
        const TimeGrid& timeGrid = path.timeGrid();
        Time dt;
        std::vector<Real> u = sequenceGen_.nextSequence().value;
        Size i;

        for (i = 0; i < n - 1; i++)
        {
           new_asset_price = path[i+1];
           // terminal or initial vol?
           vol = diffProcess_->diffusion(timeGrid[i], asset_price);
           dt = timeGrid.dt(i);

           x = std::log(new_asset_price / asset_price);
           y = 0.5 * (x + std::sqrt(x * x - 2 * vol * vol * dt * std::log((1 - u[i]))));
           y = asset_price * std::exp(y);
           if (timeGrid[i] > fixings_[knockNode] && timeGrid[i] <= fixings_[knockNode + 1])
           {
              if (y >= koBarrier_)
              {
                 isKnockOut = true;
                 ko = i;
                 break;
              }
              if (!isKnockOut)
                 knockNode = knockNode + 1;
           }
           if (y <= kiBarrier_)
           {
              isOptionActive = true;
           }
           asset_price = new_asset_price;
        }

        if (!isKnockOut && !isOptionActive)
           return (coupon_ * timeGrid.back() + margin_) * discounts_.back();        
        if (isKnockOut)
           return (rebate_ * timeGrid[(int)ko] + margin_) * discounts_[(int)ko];
        if (isOptionActive)           
           return (margin_ - payoff_(asset_price)) * discounts_.back();
        return 0;
    }
}