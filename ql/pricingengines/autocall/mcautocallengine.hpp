/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2003, 2004 Neil Firth
 Copyright (C) 2003, 2004 Ferdinando Ametrano
 Copyright (C) 2003, 2004, 2005, 2007, 2008 StatPro Italia srl

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

/*! \file mcautocallengine.hpp
    \brief Monte Carlo autocall engines
*/

#ifndef quantlib_mc_autocall_engines_hpp
#define quantlib_mc_autocall_engines_hpp

#include <ql/exercise.hpp>
#include <ql/instruments/autocall/autocall.hpp>
#include <ql/pricingengines/mcsimulation.hpp>
#include <ql/processes/blackscholesprocess.hpp>
#include <utility>

namespace QuantLib {

    //! Pricing engine for autocall using Monte Carlo simulation

    template <class RNG = PseudoRandom, class S = Statistics>
    class MCAutocallEngine : public Autocall::engine,
                            public McSimulation<SingleVariate,RNG,S> {
      public:
        typedef
        typename McSimulation<SingleVariate,RNG,S>::path_generator_type
            path_generator_type;
        typedef typename McSimulation<SingleVariate,RNG,S>::path_pricer_type
            path_pricer_type;
        typedef typename McSimulation<SingleVariate,RNG,S>::stats_type
            stats_type;
        // constructor
        MCAutocallEngine(ext::shared_ptr<GeneralizedBlackScholesProcess> process,
                        Size timeSteps,
                        Size timeStepsPerYear,
                        bool brownianBridge,
                        bool antitheticVariate,
                        Size requiredSamples,
                        Real requiredTolerance,
                        Size maxSamples,
                        bool isBiased,
                        BigNatural seed);
        void calculate() const override {
            Real spot = process_->x0();
            QL_REQUIRE(spot >= 0.0, "negative or null underlying given");
            //QL_REQUIRE(!triggered(spot), "barrier touched");
            McSimulation<SingleVariate,RNG,S>::calculate(requiredTolerance_,
                                                         requiredSamples_,
                                                         maxSamples_);
            results_.value = this->mcModel_->sampleAccumulator().mean();
            if (RNG::allowsErrorEstimate)
            results_.errorEstimate =
                this->mcModel_->sampleAccumulator().errorEstimate();
        }

      protected:
        // McSimulation implementation
        TimeGrid timeGrid() const override;
        ext::shared_ptr<path_generator_type> pathGenerator() const override {
            TimeGrid grid = timeGrid();
            typename RNG::rsg_type gen =
                RNG::make_sequence_generator(grid.size()-1,seed_);
            return ext::shared_ptr<path_generator_type>(
                         new path_generator_type(process_,
                                                 grid, gen, brownianBridge_));
        }
        ext::shared_ptr<path_pricer_type> pathPricer() const override;
        // data members
        ext::shared_ptr<GeneralizedBlackScholesProcess> process_;
        Size timeSteps_, timeStepsPerYear_;
        Size requiredSamples_, maxSamples_;
        Real requiredTolerance_;
        bool isBiased_;
        bool brownianBridge_;
        BigNatural seed_;
    };


    //! Monte Carlo autocall engine factory
    template <class RNG = PseudoRandom, class S = Statistics>
    class MakeMCAutocallEngine {
      public:
        MakeMCAutocallEngine(ext::shared_ptr<GeneralizedBlackScholesProcess>);
        // named parameters
        MakeMCAutocallEngine& withSteps(Size steps);
        MakeMCAutocallEngine& withStepsPerYear(Size steps);
        MakeMCAutocallEngine& withBrownianBridge(bool b = true);
        MakeMCAutocallEngine& withAntitheticVariate(bool b = true);
        MakeMCAutocallEngine& withSamples(Size samples);
        MakeMCAutocallEngine& withAbsoluteTolerance(Real tolerance);
        MakeMCAutocallEngine& withMaxSamples(Size samples);
        MakeMCAutocallEngine& withBias(bool b = true);
        MakeMCAutocallEngine& withSeed(BigNatural seed);
        // conversion to pricing engine
        operator ext::shared_ptr<PricingEngine>() const;
      private:
        ext::shared_ptr<GeneralizedBlackScholesProcess> process_;
        bool brownianBridge_, antithetic_, biased_;
        Size steps_, stepsPerYear_, samples_, maxSamples_;
        Real tolerance_;
        BigNatural seed_;
    };


    class AutocallPathPricer : public PathPricer<Path> {
      public:
        AutocallPathPricer( Real rebate,
                            Real coupon,
                            std::vector<Real> fixings,
                            Real kiBarrier,
                            Real koBarrier,
                            Real strike,
                            Real margin,
                            std::vector<DiscountFactor> discounts,
                            ext::shared_ptr<StochasticProcess1D> diffProcess,
                            PseudoRandom::ursg_type sequenceGen);
        Real operator()(const Path& path) const override;

      private:
        Real rebate_;
        Real coupon_;
        std::vector<Real> fixings_;
        Real kiBarrier_;
        Real koBarrier_;        
        Real margin_;
        std::vector<DiscountFactor> discounts_;
        ext::shared_ptr<StochasticProcess1D> diffProcess_;
        PseudoRandom::ursg_type sequenceGen_;
        PlainVanillaPayoff payoff_;
    };

    // template definitions

    template <class RNG, class S>
    inline MCAutocallEngine<RNG, S>::MCAutocallEngine(
        ext::shared_ptr<GeneralizedBlackScholesProcess> process,
        Size timeSteps,
        Size timeStepsPerYear,
        bool brownianBridge,
        bool antitheticVariate,
        Size requiredSamples,
        Real requiredTolerance,
        Size maxSamples,
        bool isBiased,
        BigNatural seed)
    : McSimulation<SingleVariate, RNG, S>(antitheticVariate, false), process_(std::move(process)),
      timeSteps_(timeSteps), timeStepsPerYear_(timeStepsPerYear), requiredSamples_(requiredSamples),
      maxSamples_(maxSamples), requiredTolerance_(requiredTolerance), isBiased_(isBiased),
      brownianBridge_(brownianBridge), seed_(seed) {
        QL_REQUIRE(timeSteps != Null<Size>() ||
                   timeStepsPerYear != Null<Size>(),
                   "no time steps provided");
        QL_REQUIRE(timeSteps == Null<Size>() ||
                   timeStepsPerYear == Null<Size>(),
                   "both time steps and time steps per year were provided");
        QL_REQUIRE(timeSteps != 0,
                   "timeSteps must be positive, " << timeSteps <<
                   " not allowed");
        QL_REQUIRE(timeStepsPerYear != 0,
                   "timeStepsPerYear must be positive, " << timeStepsPerYear <<
                   " not allowed");
        registerWith(process_);
    }

    template <class RNG, class S>
    inline TimeGrid MCAutocallEngine<RNG,S>::timeGrid() const {
        Time residualTime = process_->time(arguments_.exercise->lastDate());
        if (timeSteps_ != Null<Size>()) {
            return TimeGrid(residualTime, timeSteps_);
        } else if (timeStepsPerYear_ != Null<Size>()) {
            Size steps = static_cast<Size>(timeStepsPerYear_*residualTime);
            return TimeGrid(residualTime, std::max<Size>(steps, 1));
        } else {
            QL_FAIL("time steps not specified");
        }
    }

    template <class RNG, class S>
    inline
    ext::shared_ptr<typename MCAutocallEngine<RNG,S>::path_pricer_type>
    MCAutocallEngine<RNG,S>::pathPricer() const {
        ext::shared_ptr<PlainVanillaPayoff> payoff =
            ext::dynamic_pointer_cast<PlainVanillaPayoff>(arguments_.payoff);
        QL_REQUIRE(payoff, "non-plain payoff given");

        TimeGrid grid = timeGrid();
        std::vector<DiscountFactor> discounts(grid.size());
        for (Size i=0; i<grid.size(); i++)
            discounts[i] = process_->riskFreeRate()->discount(grid[i]);

        std::vector<Real> fixings(arguments_.fixingDates.size());
         for (int i = 1; i < arguments_.fixingDates.size(); i++)
            fixings[i] = process_->time(arguments_.fixingDates[i]);

        // do this with template parameters?

        PseudoRandom::ursg_type sequenceGen(grid.size()-1,
                                            PseudoRandom::urng_type(5));
        return ext::shared_ptr<
                    typename MCAutocallEngine<RNG,S>::path_pricer_type>(
            new AutocallPathPricer(
                arguments_.rebate,
                arguments_.coupon,
                fixings,
                arguments_.kiBarrier,
                arguments_.koBarrier,
                payoff->strike(),
                arguments_.margin,
                discounts,
                process_,
                sequenceGen));
    }


    template <class RNG, class S>
    inline MakeMCAutocallEngine<RNG, S>::MakeMCAutocallEngine(
        ext::shared_ptr<GeneralizedBlackScholesProcess> process)
    : process_(std::move(process)), brownianBridge_(false), antithetic_(false), biased_(false),
      steps_(Null<Size>()), stepsPerYear_(Null<Size>()), samples_(Null<Size>()),
      maxSamples_(Null<Size>()), tolerance_(Null<Real>()), seed_(0) {}

    template <class RNG, class S>
    inline MakeMCAutocallEngine<RNG,S>&
    MakeMCAutocallEngine<RNG,S>::withSteps(Size steps) {
        steps_ = steps;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCAutocallEngine<RNG,S>&
    MakeMCAutocallEngine<RNG,S>::withStepsPerYear(Size steps) {
        stepsPerYear_ = steps;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCAutocallEngine<RNG,S>&
    MakeMCAutocallEngine<RNG,S>::withBrownianBridge(bool brownianBridge) {
        brownianBridge_ = brownianBridge;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCAutocallEngine<RNG,S>&
    MakeMCAutocallEngine<RNG,S>::withAntitheticVariate(bool b) {
        antithetic_ = b;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCAutocallEngine<RNG,S>&
    MakeMCAutocallEngine<RNG,S>::withSamples(Size samples) {
        QL_REQUIRE(tolerance_ == Null<Real>(),
                   "tolerance already set");
        samples_ = samples;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCAutocallEngine<RNG,S>&
    MakeMCAutocallEngine<RNG,S>::withAbsoluteTolerance(Real tolerance) {
        QL_REQUIRE(samples_ == Null<Size>(),
                   "number of samples already set");
        QL_REQUIRE(RNG::allowsErrorEstimate,
                   "chosen random generator policy "
                   "does not allow an error estimate");
        tolerance_ = tolerance;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCAutocallEngine<RNG,S>&
    MakeMCAutocallEngine<RNG,S>::withMaxSamples(Size samples) {
        maxSamples_ = samples;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCAutocallEngine<RNG,S>&
    MakeMCAutocallEngine<RNG,S>::withBias(bool biased) {
        biased_ = biased;
        return *this;
    }

    template <class RNG, class S>
    inline MakeMCAutocallEngine<RNG,S>&
    MakeMCAutocallEngine<RNG,S>::withSeed(BigNatural seed) {
        seed_ = seed;
        return *this;
    }

    template <class RNG, class S>
    inline
    MakeMCAutocallEngine<RNG,S>::operator ext::shared_ptr<PricingEngine>()
                                                                      const {
        QL_REQUIRE(steps_ != Null<Size>() || stepsPerYear_ != Null<Size>(),
                   "number of steps not given");
        QL_REQUIRE(steps_ == Null<Size>() || stepsPerYear_ == Null<Size>(),
                   "number of steps overspecified");
        return ext::shared_ptr<PricingEngine>(new
            MCAutocallEngine<RNG,S>(process_,
                                   steps_,
                                   stepsPerYear_,
                                   brownianBridge_,
                                   antithetic_,
                                   samples_, tolerance_,
                                   maxSamples_,
                                   biased_,
                                   seed_));
    }

}

#endif
