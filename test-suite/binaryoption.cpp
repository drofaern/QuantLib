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

#include "binaryoption.hpp"
#include "utilities.hpp"
#include <ql/time/calendars/nullcalendar.hpp>
#include <ql/time/calendars/target.hpp>
#include <ql/time/daycounters/actual360.hpp>
#include <ql/math/interpolations/bicubicsplineinterpolation.hpp>
#include <ql/instruments/barrieroption.hpp>
#include <ql/models/equity/hestonmodel.hpp>
#include <ql/pricingengines/barrier/analyticbinarybarrierengine.hpp>
#include <ql/termstructures/yield/zerocurve.hpp>
#include <ql/termstructures/yield/flatforward.hpp>
#include <ql/termstructures/volatility/equityfx/blackconstantvol.hpp>
#include <ql/termstructures/volatility/equityfx/blackvariancecurve.hpp>
#include <ql/termstructures/volatility/equityfx/blackvariancesurface.hpp>
#include <ql/utilities/dataformatters.hpp>

using namespace QuantLib;
using namespace boost::unit_test_framework;

#undef REPORT_FAILURE
#define REPORT_FAILURE(greekName, payoff, exercise, barrierType, barrier, s, q,\
                        r, today, v, expected, calculated, error, tolerance) \
    BOOST_FAIL(payoff->optionType() << " option with " \
               << barrierTypeToString(barrierType) << " barrier type:\n" \
               << "    barrier:          " << barrier << "\n" \
               << payoffTypeToString(payoff) << " payoff:\n" \
               << "    spot value:       " << s << "\n" \
               << "    strike:           " << payoff->strike() << "\n" \
               << "    dividend yield:   " << io::rate(q) << "\n" \
               << "    risk-free rate:   " << io::rate(r) << "\n" \
               << "    reference date:   " << today << "\n" \
               << "    maturity:         " << exercise->lastDate() << "\n" \
               << "    volatility:       " << io::volatility(v) << "\n\n" \
               << "    expected   " << greekName << ": " << expected << "\n" \
               << "    calculated " << greekName << ": " << calculated << "\n"\
               << "    error:            " << error << "\n" \
               << "    tolerance:        " << tolerance << "\n");

namespace binary_option_test {

    std::string barrierTypeToString(Barrier::Type type) {
        switch(type){
          case Barrier::DownIn:
            return std::string("Down-and-in");
          case Barrier::UpIn:
            return std::string("Up-and-in");
          case Barrier::DownOut:
            return std::string("Down-and-out");
          case Barrier::UpOut:
            return std::string("Up-and-out");
          default:
            QL_FAIL("unknown exercise type");
        }
    }

    struct BinaryOptionData {
        bool payOnExpiry;
        Barrier::Type barrierType;
        Real barrier;
        Real cash;     // cash payoff for cash-or-nothing
        Option::Type type;
        Real strike;
        Real s;        // spot
        Rate q;        // dividend
        Rate r;        // risk-free rate
        Time t;        // time to maturity
        Volatility v;  // volatility
        Real result;   // expected result
        Real tol;      // tolerance
    };
}


void BinaryOptionTest::testAnalyticBinaryBarrierEngineValues() {

    BOOST_TEST_MESSAGE("Testing cash-or-nothing barrier options against Haug's values...");

    using namespace binary_option_test;

    BinaryOptionData values[] = {
        /* The data below are from
          "Option pricing formulas 2nd Ed.", E.G. Haug, McGraw-Hill 2007 pag. 180 - cases 13,14,17,18,21,22,25,26
          Note:
            q is the dividend rate, while the book gives b, the cost of carry (q=r-b)
        */
        //     poe,      barrierType, barrier,  cash,         type, strike,   spot,    q,    r,   t,  vol,   value, tol
             // cash-at-hit
            {  false, Barrier::DownIn,  100.00, 15.00, Option::Call, 102.00, 105.00, 0.00, 0.10, 0.5, 0.20,  9.7264, 1e-4 }, // #1 
            {  false, Barrier::DownIn,  100.00, 15.00, Option::Call,  98.00, 105.00, 0.00, 0.10, 0.5, 0.20,  9.7264, 1e-4 }, // #1
            {  false, Barrier::UpIn,    100.00, 15.00, Option::Call, 102.00,  95.00, 0.00, 0.10, 0.5, 0.20, 11.6553, 1e-4 }, // #2
            {  false, Barrier::UpIn,    100.00, 15.00, Option::Call,  98.00,  95.00, 0.00, 0.10, 0.5, 0.20, 11.6553, 1e-4 }, // #2
            {  false, Barrier::DownIn,  100.00,  0.00, Option::Call, 102.00, 105.00, 0.00, 0.10, 0.5, 0.20, 68.0848, 1e-4 }, // #3
            {  false, Barrier::DownIn,  100.00,  0.00, Option::Call,  98.00, 105.00, 0.00, 0.10, 0.5, 0.20, 68.0848, 1e-4 }, // #3
            {  false, Barrier::UpIn,    100.00,  0.00, Option::Call, 102.00,  95.00, 0.00, 0.10, 0.5, 0.20, 11.6553, 1e-4 }, // #4
            {  false, Barrier::UpIn,    100.00,  0.00, Option::Call,  98.00,  95.00, 0.00, 0.10, 0.5, 0.20, 11.6553, 1e-4 }, // #4

             // cash-at-expiry
            {  true, Barrier::DownIn,  100.00, 15.00, Option::Call, 102.00, 105.00, 0.00, 0.10, 0.5, 0.20,  4.9289, 1e-4 }, // #13 
            {  true, Barrier::DownIn,  100.00, 15.00, Option::Call,  98.00, 105.00, 0.00, 0.10, 0.5, 0.20,  6.2150, 1e-4 }, // #13
            {  true, Barrier::UpIn,    100.00, 15.00, Option::Call, 102.00,  95.00, 0.00, 0.10, 0.5, 0.20,  5.8926, 1e-4 }, // #14 wrong in book table 4-22
            {  true, Barrier::UpIn,    100.00, 15.00, Option::Call,  98.00,  95.00, 0.00, 0.10, 0.5, 0.20,  7.4519, 1e-4 }, // #14
            {  true, Barrier::DownIn,  100.00,  0.00, Option::Call, 102.00, 105.00, 0.00, 0.10, 0.5, 0.20, 37.2782, 1e-4 }, // #15
            {  true, Barrier::DownIn,  100.00,  0.00, Option::Call,  98.00, 105.00, 0.00, 0.10, 0.5, 0.20, 45.8530, 1e-4 }, // #15
            {  true, Barrier::UpIn,    100.00,  0.00, Option::Call, 102.00,  95.00, 0.00, 0.10, 0.5, 0.20, 44.5294, 1e-4 }, // #16
            {  true, Barrier::UpIn,    100.00,  0.00, Option::Call,  98.00,  95.00, 0.00, 0.10, 0.5, 0.20, 54.9262, 1e-4 }, // #16
            {  true, Barrier::DownIn,  100.00, 15.00, Option::Put,  102.00, 105.00, 0.00, 0.10, 0.5, 0.20,  4.4314, 1e-4 }, // #17
            {  true, Barrier::DownIn,  100.00, 15.00, Option::Put,   98.00, 105.00, 0.00, 0.10, 0.5, 0.20,  3.1454, 1e-4 }, // #17
            {  true, Barrier::UpIn,    100.00, 15.00, Option::Put,  102.00,  95.00, 0.00, 0.10, 0.5, 0.20,  5.3297, 1e-4 }, // #18
            {  true, Barrier::UpIn,    100.00, 15.00, Option::Put,   98.00,  95.00, 0.00, 0.10, 0.5, 0.20,  3.7704, 1e-4 }, // #18
            {  true, Barrier::DownIn,  100.00,  0.00, Option::Put,  102.00, 105.00, 0.00, 0.10, 0.5, 0.20, 27.5644, 1e-4 }, // #19
            {  true, Barrier::DownIn,  100.00,  0.00, Option::Put,   98.00, 105.00, 0.00, 0.10, 0.5, 0.20, 18.9896, 1e-4 }, // #19
            {  true, Barrier::UpIn,    100.00,  0.00, Option::Put,  102.00,  95.00, 0.00, 0.10, 0.5, 0.20, 33.1723, 1e-4 }, // #20 wrong in book table 4-22
            {  true, Barrier::UpIn,    100.00,  0.00, Option::Put,   98.00,  95.00, 0.00, 0.10, 0.5, 0.20, 22.7755, 1e-4 }, // #20
            {  true, Barrier::DownOut, 100.00, 15.00, Option::Call, 102.00, 105.00, 0.00, 0.10, 0.5, 0.20,  4.8758, 1e-4 }, // #21
            {  true, Barrier::DownOut, 100.00, 15.00, Option::Call,  98.00, 105.00, 0.00, 0.10, 0.5, 0.20,  4.9081, 1e-4 }, // #21
            {  true, Barrier::UpOut,   100.00, 15.00, Option::Call, 102.00,  95.00, 0.00, 0.10, 0.5, 0.20,  0.0000, 1e-4 }, // #22
            {  true, Barrier::UpOut,   100.00, 15.00, Option::Call,  98.00,  95.00, 0.00, 0.10, 0.5, 0.20,  0.0407, 1e-4 }, // #22
            {  true, Barrier::DownOut, 100.00,  0.00, Option::Call, 102.00, 105.00, 0.00, 0.10, 0.5, 0.20, 39.9391, 1e-4 }, // #23
            {  true, Barrier::DownOut, 100.00,  0.00, Option::Call,  98.00, 105.00, 0.00, 0.10, 0.5, 0.20, 40.1574, 1e-4 }, // #23
            {  true, Barrier::UpOut,   100.00,  0.00, Option::Call, 102.00,  95.00, 0.00, 0.10, 0.5, 0.20,  0.0000, 1e-4 }, // #24
            {  true, Barrier::UpOut,   100.00,  0.00, Option::Call,  98.00,  95.00, 0.00, 0.10, 0.5, 0.20,  0.2676, 1e-4 }, // #24
            {  true, Barrier::DownOut, 100.00, 15.00, Option::Put,  102.00, 105.00, 0.00, 0.10, 0.5, 0.20,  0.0323, 1e-4 }, // #25
            {  true, Barrier::DownOut, 100.00, 15.00, Option::Put,   98.00, 105.00, 0.00, 0.10, 0.5, 0.20,  0.0000, 1e-4 }, // #25
            {  true, Barrier::UpOut,   100.00, 15.00, Option::Put,  102.00,  95.00, 0.00, 0.10, 0.5, 0.20,  3.0461, 1e-4 }, // #26
            {  true, Barrier::UpOut,   100.00, 15.00, Option::Put,   98.00,  95.00, 0.00, 0.10, 0.5, 0.20,  3.0054, 1e-4 }, // #26
            {  true, Barrier::DownOut, 100.00,  0.00, Option::Put,  102.00, 105.00, 0.00, 0.10, 0.5, 0.20,  0.2183, 1e-4 }, // #27
            {  true, Barrier::DownOut, 100.00,  0.00, Option::Put,   98.00, 105.00, 0.00, 0.10, 0.5, 0.20,  0.0000, 1e-4 }, // #27
            {  true, Barrier::UpOut,   100.00,  0.00, Option::Put,  102.00,  95.00, 0.00, 0.10, 0.5, 0.20, 17.2983, 1e-4 }, // #28
            {  true, Barrier::UpOut,   100.00,  0.00, Option::Put,   98.00,  95.00, 0.00, 0.10, 0.5, 0.20, 17.0306, 1e-4 }, // #28
            {  true, Barrier::UpIn,    100.00, 15.00, Option::Call, 102.00,  95.00,-0.14, 0.10, 0.5, 0.20,  8.6806, 1e-4 }, // #
            {  true, Barrier::UpIn,    100.00, 15.00, Option::Call, 102.00,  95.00, 0.03, 0.10, 0.5, 0.20,  5.3112, 1e-4 }, // #
            {  true, Barrier::DownIn,  100.00, 15.00, Option::Call,  98.00,  95.00, 0.00, 0.10, 0.5, 0.20,  7.4926, 1e-4 }, // #
            {  true, Barrier::UpIn,    100.00, 15.00, Option::Call,  98.00, 105.00, 0.00, 0.10, 0.5, 0.20, 11.1231, 1e-4 }, // #
            {  true, Barrier::DownIn,  100.00, 15.00, Option::Put,  102.00,  98.00, 0.00, 0.10, 0.5, 0.20,  7.1344, 1e-4 }, // #
            {  true, Barrier::UpIn,    100.00, 15.00, Option::Put,  102.00, 101.00, 0.00, 0.10, 0.5, 0.20,  5.9299, 1e-4 }, // #
            {  true, Barrier::DownOut, 100.00, 15.00, Option::Call,  98.00,  99.00, 0.00, 0.10, 0.5, 0.20,  0.0000, 1e-4 }, // #
            {  true, Barrier::UpOut,   100.00, 15.00, Option::Call,  98.00, 101.00, 0.00, 0.10, 0.5, 0.20,  0.0000, 1e-4 }, // #
            {  true, Barrier::DownOut, 100.00, 15.00, Option::Put,   98.00,  99.00, 0.00, 0.10, 0.5, 0.20,  0.0000, 1e-4 }, // #
            {  true, Barrier::UpOut,   100.00, 15.00, Option::Put,   98.00, 101.00, 0.00, 0.10, 0.5, 0.20,  0.0000, 1e-4 }, // #
    };

    DayCounter dc = Actual360();
    Date today = Date::todaysDate();

    ext::shared_ptr<SimpleQuote> spot(new SimpleQuote(100.0));
    ext::shared_ptr<SimpleQuote> qRate(new SimpleQuote(0.04));
    ext::shared_ptr<YieldTermStructure> qTS = flatRate(today, qRate, dc);
    ext::shared_ptr<SimpleQuote> rRate(new SimpleQuote(0.01));
    ext::shared_ptr<YieldTermStructure> rTS = flatRate(today, rRate, dc);
    ext::shared_ptr<SimpleQuote> vol(new SimpleQuote(0.25));
    ext::shared_ptr<BlackVolTermStructure> volTS = flatVol(today, vol, dc);

    for (auto& value : values) {


        ext::shared_ptr<StrikedTypePayoff> payoff;
        if (value.cash != 0 ){
            payoff = ext::make_shared<CashOrNothingPayoff>(value.type, value.strike, value.cash);
        }else{
            payoff = ext::make_shared<AssetOrNothingPayoff>(value.type, value.strike);
        }

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
                             new AnalyticBinaryBarrierEngine(stochProcess));

        BarrierOption opt(value.barrierType, value.barrier, 0, payoff, amExercise, value.payOnExpiry);

        opt.setPricingEngine(engine);

        Real calculated = opt.NPV();
        Real error = std::fabs(calculated - value.result);
        if (error > value.tol) {
            REPORT_FAILURE("value", payoff, amExercise, value.barrierType, value.barrier, value.s,
                           value.q, value.r, today, value.v, value.result, calculated, error,
                           value.tol);
        }
    }
}

test_suite* BinaryOptionTest::suite() {
    auto* suite = BOOST_TEST_SUITE("Binary");
    suite->add(QUANTLIB_TEST_CASE(&BinaryOptionTest::testAnalyticBinaryBarrierEngineValues));
    return suite;
}
