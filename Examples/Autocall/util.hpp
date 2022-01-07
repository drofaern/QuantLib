#ifndef util_hpp
#define util_hpp

#include <ql/instruments/payoffs.hpp>
#include <ql/exercise.hpp>
#include <ql/termstructures/yieldtermstructure.hpp>
#include <ql/termstructures/volatility/equityfx/blackvoltermstructure.hpp>
#include <ql/termstructures/yield/flatforward.hpp>
#include <ql/termstructures/volatility/equityfx/blackconstantvol.hpp>
#include <ql/time/calendars/nullcalendar.hpp>
#include <ql/quote.hpp>
#include <ql/patterns/observable.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <ql/functional.hpp>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

// This makes it easier to use array literals (for new code, use std::vector though)
#define LENGTH(a) (sizeof(a)/sizeof(a[0]))

#define CHECK_DOWNCAST(Derived,Description) { \
    ext::shared_ptr<Derived> hd = ext::dynamic_pointer_cast<Derived>(h); \
    if (hd) \
        return Description; \
}

namespace QuantLib {

    std::string payoffTypeToString(const ext::shared_ptr<Payoff>& h) {

        CHECK_DOWNCAST(PlainVanillaPayoff, "plain-vanilla");
        CHECK_DOWNCAST(CashOrNothingPayoff, "cash-or-nothing");
        CHECK_DOWNCAST(AssetOrNothingPayoff, "asset-or-nothing");
        CHECK_DOWNCAST(SuperSharePayoff, "super-share");
        CHECK_DOWNCAST(SuperFundPayoff, "super-fund");
        CHECK_DOWNCAST(PercentageStrikePayoff, "percentage-strike");
        CHECK_DOWNCAST(GapPayoff, "gap");
        CHECK_DOWNCAST(FloatingTypePayoff, "floating-type");

        QL_FAIL("unknown payoff type");
    }


    std::string exerciseTypeToString(const ext::shared_ptr<Exercise>& h) {

        CHECK_DOWNCAST(EuropeanExercise, "European");
        CHECK_DOWNCAST(AmericanExercise, "American");
        CHECK_DOWNCAST(BermudanExercise, "Bermudan");

        QL_FAIL("unknown exercise type");
    }


    ext::shared_ptr<YieldTermStructure>
    flatRate(const Date& today,
             const ext::shared_ptr<Quote>& forward,
             const DayCounter& dc) {
        return ext::shared_ptr<YieldTermStructure>(
                          new FlatForward(today, Handle<Quote>(forward), dc));
    }

    ext::shared_ptr<YieldTermStructure>
    flatRate(const Date& today, Rate forward, const DayCounter& dc) {
        return flatRate(
               today, ext::shared_ptr<Quote>(new SimpleQuote(forward)), dc);
    }

    ext::shared_ptr<YieldTermStructure>
    flatRate(const ext::shared_ptr<Quote>& forward,
             const DayCounter& dc) {
        return ext::shared_ptr<YieldTermStructure>(
              new FlatForward(0, NullCalendar(), Handle<Quote>(forward), dc));
    }

    ext::shared_ptr<YieldTermStructure>
    flatRate(Rate forward, const DayCounter& dc) {
        return flatRate(ext::shared_ptr<Quote>(new SimpleQuote(forward)),
                        dc);
    }


    ext::shared_ptr<BlackVolTermStructure>
    flatVol(const Date& today,
            const ext::shared_ptr<Quote>& vol,
            const DayCounter& dc) {
        return ext::shared_ptr<BlackVolTermStructure>(new
            BlackConstantVol(today, NullCalendar(), Handle<Quote>(vol), dc));
    }

    ext::shared_ptr<BlackVolTermStructure>
    flatVol(const Date& today,
            const Calendar cal,
            const ext::shared_ptr<Quote>& vol,
            const DayCounter& dc) {
        return ext::shared_ptr<BlackVolTermStructure>(new
            BlackConstantVol(today, cal, Handle<Quote>(vol), dc));
    }

    ext::shared_ptr<BlackVolTermStructure>
    flatVol(const Date& today, Volatility vol,
            const DayCounter& dc) {
        return flatVol(today,
                       ext::shared_ptr<Quote>(new SimpleQuote(vol)),
                       dc);
    }

    ext::shared_ptr<BlackVolTermStructure>
    flatVol(const ext::shared_ptr<Quote>& vol,
            const DayCounter& dc) {
        return ext::shared_ptr<BlackVolTermStructure>(new
            BlackConstantVol(0, NullCalendar(), Handle<Quote>(vol), dc));
    }

    ext::shared_ptr<BlackVolTermStructure>
    flatVol(Volatility vol,
            const DayCounter& dc) {
        return flatVol(ext::shared_ptr<Quote>(new SimpleQuote(vol)), dc);
    }

    //Real relativeError(Real x1, Real x2, Real reference);

    //bool checkAbsError(Real x1, Real x2, Real tolerance){
    //    return std::fabs(x1 - x2) < tolerance;
    //};

    template<class Iterator>
    Real norm(const Iterator& begin, const Iterator& end, Real h) {
        // squared values
        std::vector<Real> f2(end-begin);
        std::transform(begin,end,begin,f2.begin(),
                       std::multiplies<Real>());
        // numeric integral of f^2
        Real I = h * (std::accumulate(f2.begin(),f2.end(),Real(0.0))
                      - 0.5*f2.front() - 0.5*f2.back());
        return std::sqrt(I);
    }


    inline Integer timeToDays(Time t, Integer daysPerYear = 360) {
        return Integer(std::lround(t * daysPerYear));
    }


    // this cleans up index-fixing histories when destroyed
    class IndexHistoryCleaner {
      public:
        IndexHistoryCleaner() = default;
        ~IndexHistoryCleaner();
    };


    // Allow streaming vectors to error messages.

    // The standard forbids defining new overloads in the std
    // namespace, so we have to use a wrapper instead of overloading
    // operator<< to send a vector to the stream directly.
    // Defining the overload outside the std namespace wouldn't work
    // with Boost streams because of ADT name lookup rules.

    template <class T>
    struct vector_streamer {
        explicit vector_streamer(std::vector<T> v) : v(std::move(v)) {}
        std::vector<T> v;
    };

    template <class T>
    vector_streamer<T> to_stream(const std::vector<T>& v) {
        return vector_streamer<T>(v);
    }

    template <class T>
    std::ostream& operator<<(std::ostream& out, const vector_streamer<T>& s) {
        out << "{ ";
        if (!s.v.empty()) {
            for (size_t n=0; n<s.v.size()-1; ++n)
                out << s.v[n] << ", ";
            out << s.v.back();
        }
        out << " }";
        return out;
    }


}


#endif
