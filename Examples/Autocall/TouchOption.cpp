/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2014 Thema Consulting SA

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

#include <iostream>


#include "util.hpp"
#include <ql/time/calendars/nullcalendar.hpp>
#include <ql/time/calendars/target.hpp>
#include <ql/time/daycounters/actual360.hpp>
#include <ql/instruments/autocall/touchoption.hpp>
#include <ql/models/equity/hestonmodel.hpp>
#include <ql/pricingengines/autocall/fdblackscholestouchengine.hpp>
#include <ql/termstructures/yield/flatforward.hpp>
#include <ql/termstructures/volatility/equityfx/blackconstantvol.hpp>
#include <ql/termstructures/volatility/equityfx/blackvariancecurve.hpp>
#include <ql/termstructures/volatility/equityfx/blackvariancesurface.hpp>
#include <ql/utilities/dataformatters.hpp>

using namespace QuantLib;

#undef REPORT
#define REPORT(greekName, exercise, touchType, barrier, payAtExpiry, s, q,\
                        r, today, v, expected, calculated, error, tolerance) \
    std::cout << touchTypeToString(touchType) << " barrier type:\n" \
               << "    barrier:          " << barrier << "\n" \
               << "    payAtExpiry:      " << (payAtExpiry ? "True":"False") << "\n" \
               << "    spot value:       " << s << "\n" \
               << "    dividend yield:   " << io::rate(q) << "\n" \
               << "    risk-free rate:   " << io::rate(r) << "\n" \
               << "    reference date:   " << today << "\n" \
               << "    maturity:         " << exercise->lastDate() << "\n" \
               << "    volatility:       " << io::volatility(v) << "\n" \
               << "    expected   " << greekName << ": " << expected << "\n" \
               << "    calculated " << greekName << ": " << calculated << "\n\n";               

std::string touchTypeToString(Touch::Type type) {
    switch(type){
      case Touch::OneTouchUp:
        return std::string("One-touch-up");
      case Touch::OneTouchDown:
        return std::string("One-touch-down");
      case Touch::NoTouchUp:
        return std::string("No-touch-up");
      case Touch::NoTouchDown:
        return std::string("No-touch-down");
      case Touch::DoubleOneTouch:
        return std::string("Double-one-touch");
      case Touch::DoubleNoTouch:
        return std::string("Double-no-touch");
      default:
        QL_FAIL("unknown touch type");
    }
}

struct TouchOptionData {
    Touch::Type touchType;
    std::vector<Real> barrierHigh;
    std::vector<Real> barrierLow;
    std::vector<Real> rebateHigh;
    std::vector<Real> rebateLow;
    bool payAtExpiry;
    Real strike;
    Real s;        // spot
    Rate q;        // dividend
    Rate r;        // risk-free rate
    Time t;        // time to maturity
    Volatility v;  // volatility
    Real result;   // expected result
    Real tol;      // tolerance
};


int main(int, char* []) {

    try {
        TouchOptionData values[] = {
            /* The data below are from
              "Option pricing formulas 2nd Ed.", E.G. Haug, McGraw-Hill 2007 pag. 180 - cases 13,14,17,18,21,22,25,26
              Note:
                q is the dividend rate, while the book gives b, the cost of carry (q=r-b)
            */
            //    barrierType, barrier,  cash,         type, strike,   spot,    q,    r,   t,  vol,   value, tol
            
            { Touch::OneTouchUp,   {103.00}, {}, {2.00}, {}, false, 100.00, 100.00, 0.00, 0.025, 1, 0.10,  0.0000, 1e-4 },
            { Touch::OneTouchUp,   {103.00}, {}, {2.00}, {}, false, 100.00, 100.00, 0.00, 0.025, 2, 0.10,  0.0000, 1e-4 },            
            { Touch::OneTouchUp,   {103.00}, {}, {2.00}, {}, true, 100.00, 100.00, 0.00, 0.025, 1, 0.10,  0.0000, 1e-4 }
            // { Barrier::DownIn,  100.00, 15.00, Option::Call, 102.00, 105.00, 0.00, 0.10, 0.5, 0.20,  4.9289, 1e-4 },
            // { Barrier::DownIn,  100.00, 15.00, Option::Call,  98.00, 105.00, 0.00, 0.10, 0.5, 0.20,  6.2150, 1e-4 },
            // // following value is wrong in book. 
            // { Barrier::UpIn,    100.00, 15.00, Option::Call, 102.00,  95.00, 0.00, 0.10, 0.5, 0.20,  5.8926, 1e-4 },
            // { Barrier::UpIn,    100.00, 15.00, Option::Call,  98.00,  95.00, 0.00, 0.10, 0.5, 0.20,  7.4519, 1e-4 },
            // // 17,18
            // { Barrier::DownIn,  100.00, 15.00, Option::Put,  102.00, 105.00, 0.00, 0.10, 0.5, 0.20,  4.4314, 1e-4 },
            // { Barrier::DownIn,  100.00, 15.00, Option::Put,   98.00, 105.00, 0.00, 0.10, 0.5, 0.20,  3.1454, 1e-4 },
            // { Barrier::UpIn,    100.00, 15.00, Option::Put,  102.00,  95.00, 0.00, 0.10, 0.5, 0.20,  5.3297, 1e-4 },
            // { Barrier::UpIn,    100.00, 15.00, Option::Put,   98.00,  95.00, 0.00, 0.10, 0.5, 0.20,  3.7704, 1e-4 },
            // // 21,22
            // { Barrier::DownOut, 100.00, 15.00, Option::Call, 102.00, 105.00, 0.00, 0.10, 0.5, 0.20,  4.8758, 1e-4 },
            // { Barrier::DownOut, 100.00, 15.00, Option::Call,  98.00, 105.00, 0.00, 0.10, 0.5, 0.20,  4.9081, 1e-4 },
            // { Barrier::UpOut,   100.00, 15.00, Option::Call, 102.00,  95.00, 0.00, 0.10, 0.5, 0.20,  0.0000, 1e-4 },
            // { Barrier::UpOut,   100.00, 15.00, Option::Call,  98.00,  95.00, 0.00, 0.10, 0.5, 0.20,  0.0407, 1e-4 },
            // // 25,26
            // { Barrier::DownOut, 100.00, 15.00, Option::Put,  102.00, 105.00, 0.00, 0.10, 0.5, 0.20,  0.0323, 1e-4 },
            // { Barrier::DownOut, 100.00, 15.00, Option::Put,   98.00, 105.00, 0.00, 0.10, 0.5, 0.20,  0.0000, 1e-4 },
            // { Barrier::UpOut,   100.00, 15.00, Option::Put,  102.00,  95.00, 0.00, 0.10, 0.5, 0.20,  3.0461, 1e-4 },
            // { Barrier::UpOut,   100.00, 15.00, Option::Put,   98.00,  95.00, 0.00, 0.10, 0.5, 0.20,  3.0054, 1e-4 },

            // // other values calculated with book vba
            // { Barrier::UpIn,    100.00, 15.00, Option::Call, 102.00,  95.00,-0.14, 0.10, 0.5, 0.20,  8.6806, 1e-4 },
            // { Barrier::UpIn,    100.00, 15.00, Option::Call, 102.00,  95.00, 0.03, 0.10, 0.5, 0.20,  5.3112, 1e-4 },
            // // degenerate conditions (barrier touched)
            // { Barrier::DownIn,  100.00, 15.00, Option::Call,  98.00,  95.00, 0.00, 0.10, 0.5, 0.20,  7.4926, 1e-4 },
            // { Barrier::UpIn,    100.00, 15.00, Option::Call,  98.00, 105.00, 0.00, 0.10, 0.5, 0.20, 11.1231, 1e-4 },
            // // 17,18
            // { Barrier::DownIn,  100.00, 15.00, Option::Put,  102.00,  98.00, 0.00, 0.10, 0.5, 0.20,  7.1344, 1e-4 },
            // { Barrier::UpIn,    100.00, 15.00, Option::Put,  102.00, 101.00, 0.00, 0.10, 0.5, 0.20,  5.9299, 1e-4 },
            // // 21,22
            // { Barrier::DownOut, 100.00, 15.00, Option::Call,  98.00,  99.00, 0.00, 0.10, 0.5, 0.20,  0.0000, 1e-4 },
            // { Barrier::UpOut,   100.00, 15.00, Option::Call,  98.00, 101.00, 0.00, 0.10, 0.5, 0.20,  0.0000, 1e-4 },
            // // 25,26
            // { Barrier::DownOut, 100.00, 15.00, Option::Put,   98.00,  99.00, 0.00, 0.10, 0.5, 0.20,  0.0000, 1e-4 },
            // { Barrier::UpOut,   100.00, 15.00, Option::Put,   98.00, 101.00, 0.00, 0.10, 0.5, 0.20,  0.0000, 1e-4 },
            
        };

        DayCounter dc = Actual360();
        Date today = Date::todaysDate();

        ext::shared_ptr<SimpleQuote> spot(new SimpleQuote(0.0));
        ext::shared_ptr<SimpleQuote> qRate(new SimpleQuote(0.00));
        ext::shared_ptr<YieldTermStructure> qTS = flatRate(today, qRate, dc);
        ext::shared_ptr<SimpleQuote> rRate(new SimpleQuote(0.00));
        ext::shared_ptr<YieldTermStructure> rTS = flatRate(today, rRate, dc);
        ext::shared_ptr<SimpleQuote> vol(new SimpleQuote(0.0));
        ext::shared_ptr<BlackVolTermStructure> volTS = flatVol(today, vol, dc);

        for (auto& value : values) {
            Date exDate = today + timeToDays(value.t);
            ext::shared_ptr<Exercise> amExercise(new AmericanExercise(today,
                                                                        exDate,
                                                                        true));

            spot->setValue(value.s);
            qRate->setValue(value.q);
            rRate->setValue(value.r);
            vol->setValue(value.v);

            ext::shared_ptr<BlackScholesMertonProcess> stochProcess(new
                BlackScholesMertonProcess(Handle<Quote>(spot),
                                          Handle<YieldTermStructure>(qTS),
                                          Handle<YieldTermStructure>(rTS),
                                          Handle<BlackVolTermStructure>(volTS)));
            ext::shared_ptr<PricingEngine> engine(
                                 new FdBlackScholesTouchEngine(stochProcess, Size(360), Size(500)));

            
            TouchOption opt(value.touchType, value.barrierHigh, value.barrierLow, value.rebateHigh, value.rebateLow, value.payAtExpiry, amExercise);
            
            opt.setPricingEngine(engine);

            Real calculated = opt.NPV();
            Real error = std::fabs(calculated - value.result);
            REPORT("value", amExercise, value.touchType, value.barrierHigh[0], value.payAtExpiry, value.s,
                     value.q, value.r, today, value.v, value.result, calculated, error,
                     value.tol);
            
        }
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "unknown error" << std::endl;
        return 1;
    }
}

