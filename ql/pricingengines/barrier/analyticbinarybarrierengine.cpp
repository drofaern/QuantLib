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

#include <ql/exercise.hpp>
#include <ql/instruments/vanillaoption.hpp>
#include <ql/math/distributions/normaldistribution.hpp>
#include <ql/pricingengines/barrier/analyticbinarybarrierengine.hpp>
#include <ql/pricingengines/vanilla/analyticeuropeanengine.hpp>
#include <utility>
#include <iostream>

#define log_S_X   (std::log(spot()/strike()))
#define log_S_H   (std::log(spot()/barrier()))
#define log_H_S   (std::log(barrier()/spot()))
#define log_H2_SX (std::log(barrier()*barrier()/(spot()*strike())))
#define H_S_2mu   (std::pow(barrier()/spot(), 2*mu()))
#define H_S_2mu_1 (std::pow(barrier()/spot(), 2*(mu()+1)))



namespace QuantLib {
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

    AnalyticBinaryBarrierEngine::AnalyticBinaryBarrierEngine(
        ext::shared_ptr<GeneralizedBlackScholesProcess> process)
    : process_(std::move(process)) {
        registerWith(process_);
    }

    void AnalyticBinaryBarrierEngine::calculate() const {
        ext::shared_ptr<StrikedTypePayoff> payoff =
            ext::dynamic_pointer_cast<StrikedTypePayoff>(arguments_.payoff);
        QL_REQUIRE(payoff, "non-striked payoff given");

        Real spot = process_->stateVariable()->value();
        QL_REQUIRE(spot > 0.0, "negative or null underlying given");

        Real barrier = arguments_.barrier;
        QL_REQUIRE(barrier>0.0,
                   "positive barrier value required");
        Barrier::Type barrierType = arguments_.barrierType;

        // KO degenerate cases
        if ( (barrierType == Barrier::DownOut && spot <= barrier) ||
             (barrierType == Barrier::UpOut && spot >= barrier))
        {
            // knocked out, no value
            results_.value = 0;
            results_.delta = 0;
            results_.gamma = 0;
            results_.vega = 0;
            results_.theta = 0;
            results_.rho = 0;
            results_.dividendRho = 0;
            return;
        }

        // KI degenerate cases
        if ((barrierType == Barrier::DownIn && spot <= barrier) ||
           (barrierType == Barrier::UpIn && spot >= barrier)) {
            // knocked in - is a digital european
            ext::shared_ptr<Exercise> exercise(new EuropeanExercise(
                                             arguments_.exercise->lastDate()));

            ext::shared_ptr<PricingEngine> engine(
                                       new AnalyticEuropeanEngine(process_));

            VanillaOption opt(payoff, exercise);
            opt.setPricingEngine(engine);
            results_.value = opt.NPV();
            results_.delta = opt.delta();
            results_.gamma = opt.gamma();
            results_.vega = opt.vega();
            results_.theta = opt.theta();
            results_.rho = opt.rho();
            results_.dividendRho = opt.dividendRho();
            return;
        }        
        Option::Type type   = payoff->optionType();
        
        bool cashOrNothing = false;
        bool assetOrNothing = false;
        bool payAtExpiry = arguments_.payAtExpiry;

        ext::shared_ptr<CashOrNothingPayoff> coo =
            ext::dynamic_pointer_cast<CashOrNothingPayoff>(arguments_.payoff);
        if (coo != nullptr) cashOrNothing = true; else assetOrNothing = true;

        Real phi;
        Real eta;
        Real result;

        // case 1: Down-and-in cash-(at-hit)-or-nothing
        // case 2: Up-and-in cash-(at-hit)-or-nothing
        // case 3: Down-and-in asset-(at-hit)-or-nothing (K=H)
        // case 4: Up-and-in asset-(at-hit)-or-nothing (K=H)
        if (!payAtExpiry){
          if (barrierType == Barrier::DownIn)
            eta = 1;
          if (barrierType == Barrier::UpIn)
            eta = -1;
          result = A5(eta);
        }
        // following cases degenerate to call with zero strike vice versa
        // case 5: Down-and-in cash-or-nothing
        // case 6: Up-and-in cash-or-nothing
        // case 7: Down-and-in asset-or-nothing
        // case 8: Up-and-in asset-or-nothing
        // case 9: Down-and-out cash-or-nothing
        // case 10: Up-and-out cash-or-nothing
        // case 11: Down-and-out asset-or-nothing
        // case 12: Up-and-out asset-or-nothing

        // case 13: Down-and-in cash-or-nothing call
        else if (cashOrNothing && barrierType == Barrier::DownIn && type == Option::Call){
           if (strike() >= barrier) {
              phi = 1;
              result = B3(phi);
            } else {
              eta = 1; phi = 1;
              result = B1(phi) - B2(phi) + B4(eta);
            }
        }
        // case 14: Up-and-in cash-or-nothing call
        else if (cashOrNothing && barrierType == Barrier::UpIn && type == Option::Call){
           if (strike() >= barrier) {
              phi = 1;
              result = B1(phi); // wrong in VBA
            } else {
              eta = -1; phi = 1;
              result = B2(phi) - B3(eta) + B4(eta);
            }
        }
        // case 15: Down-and-in asset-or-nothing call
        else if (assetOrNothing && barrierType == Barrier::DownIn && type == Option::Call){
           if (strike() >= barrier) {
              eta = 1;
              result = A3(eta);
            } else {
              eta = 1; phi = 1;
              result = A1(phi) - A2(phi) + A4(eta);
            }
        }
        // case 16: Up-and-in asset-or-nothing call
        else if (assetOrNothing && barrierType == Barrier::UpIn && type == Option::Call){
           if (strike() >= barrier) {
              phi = 1;
              result = A1(phi);
            } else {
              eta = -1; phi = 1;
              result = A2(phi) - A3(eta) + A4(eta);
            }
        }
        // case 17: Down-and-in cash-or-nothing put
        else if (cashOrNothing && barrierType == Barrier::DownIn && type == Option::Put){
           if (strike() >= barrier) {
              eta = 1; phi = -1;
              result = B2(phi) - B3(eta) + B4(eta);
            } else {
              phi = -1;
              result = B1(phi);
            }
        }
        // case 18: Up-and-in cash-or-nothing put
        else if (cashOrNothing && barrierType == Barrier::UpIn && type == Option::Put){
           if (strike() >= barrier) {
              eta = -1; phi = -1;
              result = B1(phi) - B2(phi) + B4(eta);
            } else {
              phi = -1;
              result = B3(phi);
            }
        }
        // case 19: Down-and-in asset-or-nothing put
        else if (assetOrNothing && barrierType == Barrier::DownIn && type == Option::Put){
           if (strike() >= barrier) {
              eta = 1; phi = -1;
              result = A2(phi) - A3(eta) + A4(eta);
            } else {
              phi = -1;
              result = A1(phi);
            }
        }
        // case 20: Up-and-in asset-or-nothing put
        else if (assetOrNothing && barrierType == Barrier::UpIn && type == Option::Put){
           if (strike() >= barrier) {
              eta = -1; phi = -1;
              result = A1(phi) - A2(phi) + A4(eta);  // wrong in book and VBA?????
            } else {
              eta = -1;
              result = A3(eta);
            }
        }
        // case 21: Down-and-out cash-or-nothing call
        else if (cashOrNothing && barrierType == Barrier::DownOut && type == Option::Call){  
           if (strike() >= barrier) {
              eta = 1; phi = 1;
              result = B1(phi) - B3(eta);
            } else {
              eta = 1; phi = 1;
              result = B2(phi) - B4(eta);
            }
        }
        // case 22: Up-and-out cash-or-nothing call
        else if (cashOrNothing && barrierType == Barrier::UpOut && type == Option::Call){
           if (strike() >= barrier) {
              result = 0;
            } else {
              eta = -1; phi = 1;
              result = B1(phi) - B2(phi) + B3(eta) - B4(eta);
            }
        }
        // case 23: Down-and-out asset-or-nothing call
        else if (assetOrNothing && barrierType == Barrier::DownOut && type == Option::Call){
           if (strike() >= barrier) {
              eta = 1; phi = 1;
              result = A1(phi) - A3(eta);
            } else {
              eta = 1; phi = 1;
              result = A2(phi) - A4(eta);
            }
        }
        // case 24: Up-and-out asset-or-nothing call
        else if (assetOrNothing && barrierType == Barrier::UpOut && type == Option::Call){
           if (strike() >= barrier) {
              result = 0;
            } else {
              eta = -1; phi = 1;
              result = A1(phi) - A2(phi) + A3(eta) - A4(eta);
            }
        }
        // case 25: Down-and-out cash-or-nothing put
        else if (cashOrNothing && barrierType == Barrier::DownOut && type == Option::Put){
           if (strike() >= barrier) {
              eta = 1; phi = -1;
              result = B1(phi) - B2(phi) + B3(eta) - B4(eta);
            } else {
              result = 0;
            }
        }
        // case 26: Up-and-out cash-or-nothing put
        else if (cashOrNothing && barrierType == Barrier::UpOut && type == Option::Put){
           if (strike() >= barrier) {
              eta = -1; phi = -1;
              result = B2(phi) - B4(eta);
            } else {
              eta = -1; phi = -1;
              result = B1(phi) - B3(eta);
            }
        }
        // case 27: Down-and-out asset-or-nothing put
        else if (assetOrNothing && barrierType == Barrier::DownOut && type == Option::Put){
           if (strike() >= barrier) {
              eta = 1; phi = -1;
              result = A1(phi) - A2(phi) + A3(eta) - A4(eta);
            } else {
              result = 0;
            }
        }
        // case 28: Up-and-out asset-or-nothing put
        else if (assetOrNothing && barrierType == Barrier::UpOut && type == Option::Put){
           if (strike() >= barrier) {
              eta = -1; phi = -1;
              result = A2(phi) - A4(eta);
            } else {
              eta = -1; phi = -1;
              result = A1(phi) - A3(eta);
            }
        }
        else {
           QL_FAIL("Not supported");
        }

        results_.value = result;
    }

    Real AnalyticBinaryBarrierEngine::variance() const {
        return process_->blackVolatility()->blackVariance(arguments_.exercise->lastDate(),
            strike());
    }

    Real AnalyticBinaryBarrierEngine::stdDeviation() const {
        return std::sqrt(process_->blackVolatility()->blackVariance(
                        arguments_.exercise->lastDate(),
                        strike()));
    }

    Real AnalyticBinaryBarrierEngine::barrier() const {
        QL_REQUIRE(arguments_.barrier>0.0,
                   "positive barrier value required");
        return arguments_.barrier;
    }

    Real AnalyticBinaryBarrierEngine::strike() const {
         ext::shared_ptr<StrikedTypePayoff> payoff =
            ext::dynamic_pointer_cast<StrikedTypePayoff>(arguments_.payoff);
        QL_REQUIRE(payoff, "non-striked type payoff given");
        return payoff->strike();
    }

    Real AnalyticBinaryBarrierEngine::rebate() const {
        return arguments_.rebate;
    }

    DiscountFactor AnalyticBinaryBarrierEngine::riskFreeDiscount() const {
        return process_->riskFreeRate()->discount(
                    arguments_.exercise->lastDate());
    }

    DiscountFactor AnalyticBinaryBarrierEngine::dividendDiscount() const {
        return process_->dividendYield()->discount(
                    arguments_.exercise->lastDate());
    }

    Real AnalyticBinaryBarrierEngine::spot() const {
      return process_->x0();
    }

    Real AnalyticBinaryBarrierEngine::A1(Real phi) const{
         return spot() * dividendDiscount()*f_(phi*(log_S_X/stdDeviation() + (mu()+1)*stdDeviation()));
    }

    Real AnalyticBinaryBarrierEngine::A2(Real phi) const{
         return spot()* dividendDiscount()*f_(phi*(log_S_H/stdDeviation() + (mu()+1)*stdDeviation()));
    }
    
    Real AnalyticBinaryBarrierEngine::A3(Real eta) const{                
         return spot()* dividendDiscount()* H_S_2mu_1 * f_(eta * (log_H2_SX/stdDeviation() + (mu()+1)*stdDeviation()));
    }

    Real AnalyticBinaryBarrierEngine::A4(Real eta) const{
         return spot()* dividendDiscount()* H_S_2mu_1 * f_(eta*(log_H_S/stdDeviation() + (mu()+1)*stdDeviation()));
    }

    Real AnalyticBinaryBarrierEngine::A5(Real eta) const{
         ext::shared_ptr<CashOrNothingPayoff> coo =
            ext::dynamic_pointer_cast<CashOrNothingPayoff>(arguments_.payoff);
         Real K;
         if (coo != nullptr)
            K = coo->cashPayoff();
         else
            K = barrier();
         Real lambda = std::sqrt(mu() * mu() - 2 * std::log(riskFreeDiscount()) / (stdDeviation() * stdDeviation()));
         return K * (std::pow((barrier()/ spot()), mu() + lambda) * f_(eta*(log_H_S/stdDeviation() + lambda*stdDeviation())) +
              std::pow((barrier()/ spot()), mu() - lambda) * f_(eta*(log_H_S/stdDeviation() + lambda*stdDeviation()) - 2*eta*lambda*stdDeviation()));
    }

    Real AnalyticBinaryBarrierEngine::B1(Real phi) const {
         ext::shared_ptr<CashOrNothingPayoff> coo =
            ext::dynamic_pointer_cast<CashOrNothingPayoff>(arguments_.payoff);
         QL_REQUIRE(coo, "non-cash-or-nothing payoff given");
         return coo->cashPayoff()* riskFreeDiscount()*f_(phi*(log_S_X/stdDeviation() + mu()*stdDeviation()));
    }

    Real AnalyticBinaryBarrierEngine::B2(Real phi) const {
         ext::shared_ptr<CashOrNothingPayoff> coo =
            ext::dynamic_pointer_cast<CashOrNothingPayoff>(arguments_.payoff);
         QL_REQUIRE(coo, "non-cash-or-nothing payoff given");
         return coo->cashPayoff()* riskFreeDiscount()*f_(phi*(log_S_H/stdDeviation() + mu()*stdDeviation()));
    }
    
    Real AnalyticBinaryBarrierEngine::B3(Real eta) const {
         ext::shared_ptr<CashOrNothingPayoff> coo =
            ext::dynamic_pointer_cast<CashOrNothingPayoff>(arguments_.payoff);
         QL_REQUIRE(coo, "non-cash-or-nothing payoff given");
         return coo->cashPayoff()* riskFreeDiscount()* H_S_2mu * f_(eta * (log_H2_SX/stdDeviation() + mu()*stdDeviation()));
    }
    
    Real AnalyticBinaryBarrierEngine::B4(Real eta) const {
         ext::shared_ptr<CashOrNothingPayoff> coo =
            ext::dynamic_pointer_cast<CashOrNothingPayoff>(arguments_.payoff);
         QL_REQUIRE(coo, "non-cash-or-nothing payoff given");
         return coo->cashPayoff()* riskFreeDiscount()* H_S_2mu * f_(eta*(log_H_S/stdDeviation() + mu()*stdDeviation()));
    }

    Rate AnalyticBinaryBarrierEngine::mu() const {
        return std::log(dividendDiscount()/riskFreeDiscount())/variance() - 0.5;
    }
}

