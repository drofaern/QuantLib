/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*!
 Copyright (C) 2005, 2006, 2007, 2009 StatPro Italia srl

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

#include <ql/qldefines.hpp>
#ifdef BOOST_MSVC
#  include <ql/auto_link.hpp>
#endif
#include <ql/instruments/autocall/autocall.hpp>

#include <ql/quotes/simplequote.hpp>
#include <ql/termstructures/yield/flatforward.hpp>
#include <ql/termstructures/volatility/equityfx/blackconstantvol.hpp>
#include <ql/pricingengines/autocall/mcautocallengine.hpp>
#include <ql/time/calendars/china.hpp>
#include <ql/time/schedule.hpp>
#include <ql/utilities/dataformatters.hpp>

#include <iostream>
#include <iomanip>

using namespace QuantLib;

#if defined(QL_ENABLE_SESSIONS)
namespace QuantLib {

    ThreadKey sessionId() { return {}; }

}
#endif


int main(int, char* []) {

    try {

        std::cout << std::endl;
        
        // set up dates
        Calendar calendar = China();
        Date todaysDate = Date::todaysDate();
        Date settlementDate = Date::todaysDate();
        Settings::instance().evaluationDate() = todaysDate;

        // our options

        Real rebate = 16;
        Real coupon = 16;
        Real kiBarrier = 75;
        Real koBarrier = 103;
        Real underlying = 100;
        Real strike = 100;
        Real margin = 100;
        Spread dividendYield = 0.00;
        Rate riskFreeRate = 0.03;
        Volatility volatility = 0.40;
        Frequency frequency = Monthly;
        int t = 1;
        
        BusinessDayConvention convention = Following;
        Date maturity = calendar.advance(settlementDate, t, Years, convention);
        DayCounter dayCounter = Actual365Fixed();

        Schedule fixingDates(settlementDate,maturity,Period(frequency),
                               calendar,convention,convention,
                               DateGeneration::Forward,false);

        std::cout << "Maturity = "        << maturity << std::endl;
        std::cout << "Underlying price = "        << underlying << std::endl;
        std::cout << "Strike = "                  << strike << std::endl;
        std::cout << "Risk-free interest rate = " << io::rate(riskFreeRate)
                  << std::endl;
        std::cout << "Dividend yield = " << io::rate(dividendYield)
                  << std::endl;
        std::cout << "Volatility = " << io::volatility(volatility)
                  << std::endl;
        std::cout << std::endl;
        std::string method;
        std::cout << std::endl ;

        // write column headings
        Size widths[] = { 35, 14, 14, 14 };
        // std::cout << std::setw(widths[0]) << std::left << "Method"
        //           << std::setw(widths[1]) << std::left << "European"
        //           << std::setw(widths[2]) << std::left << "Bermudan"
        //           << std::setw(widths[3]) << std::left << "American"
        //           << std::endl;

        ext::shared_ptr<Exercise> europeanExercise(
                                         new EuropeanExercise(maturity));

        Handle<Quote> underlyingH(
            ext::shared_ptr<Quote>(new SimpleQuote(underlying)));

        // bootstrap the yield/dividend/vol curves
        Handle<YieldTermStructure> flatTermStructure(
            ext::shared_ptr<YieldTermStructure>(
                new FlatForward(settlementDate, riskFreeRate, dayCounter)));
        Handle<YieldTermStructure> flatDividendTS(
            ext::shared_ptr<YieldTermStructure>(
                new FlatForward(settlementDate, dividendYield, dayCounter)));
        Handle<BlackVolTermStructure> flatVolTS(
            ext::shared_ptr<BlackVolTermStructure>(
                new BlackConstantVol(settlementDate, calendar, volatility,
                                     dayCounter)));
        ext::shared_ptr<StrikedTypePayoff> payoff(
                                        new PlainVanillaPayoff(Option::Type::Put, strike));
        ext::shared_ptr<BlackScholesMertonProcess> bsmProcess(
                 new BlackScholesMertonProcess(underlyingH, flatDividendTS,
                                               flatTermStructure, flatVolTS));   

        // options        
        Autocall autocall(rebate,
                    coupon,
                    fixingDates.dates(),
                    kiBarrier,
                    koBarrier,
                    margin,
                    payoff,
                    europeanExercise);

        // Analytic formulas:

        // Monte Carlo for autocall
        method = "Monte Carlo";
        ext::shared_ptr<PricingEngine> mcengine;
        mcengine = MakeMCAutocallEngine<PseudoRandom, GeneralStatistics>(bsmProcess)
                    .withStepsPerYear(365)
                    .withBrownianBridge()
                    //.withSamples(5000) // 2^17-1
                    //.withMaxSamples(1048575) // 2^20-1
                    //.withAntitheticVariate()
                    .withAbsoluteTolerance(0.01)
                    .withSeed(8);
        autocall.setPricingEngine(mcengine);

        std::cout << std::setw(widths[0]) << std::left << method
                  << std::fixed
                  << std::setw(widths[1]) << std::left << autocall.NPV()
                  << std::setw(widths[2]) << std::left << "N/A"
                  << std::setw(widths[3]) << std::left << "N/A"
                  << std::endl;

        // End test
        return 0;

    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "unknown error" << std::endl;
        return 1;
    }
}
