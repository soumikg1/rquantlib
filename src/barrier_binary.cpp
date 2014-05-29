// -*- mode: C++; c-indent-level: 4; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
//  RQuantLib -- R interface to the QuantLib libraries
//
//  Copyright (C) 2002 - 2014  Dirk Eddelbuettel <edd@debian.org>
//
//  This file is part of RQuantLib.
//
//  RQuantLib is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 2 of the License, or
//  (at your option) any later version.
//
//  RQuantLib is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with RQuantLib.  If not, see <http://www.gnu.org/licenses/>.

#include "rquantlib.h"

// [[Rcpp::export]]
Rcpp::List binaryOptionEngine(std::string binType,
                              std::string type,
                              std::string excType,
                              double underlying,
                              double strike,
                              double dividendYield, 
                              double riskFreeRate,
                              double maturity,
                              double volatility,
                              double cashPayoff) {

    int length = int(maturity*360 + 0.5); // FIXME: this could be better, but same rounding in QL
    QuantLib::Option::Type optionType = getOptionType(type);

    // new QuantLib 0.3.5 framework: digitals, updated for 0.3.7
    // updated again for QuantLib 0.9.0, 
    // cf QuantLib-0.9.0/test-suite/digitaloption.cpp
    QuantLib::Date today = QuantLib::Date::todaysDate();
    QuantLib::Settings::instance().evaluationDate() = today;

    QuantLib::DayCounter dc = QuantLib::Actual360();
    boost::shared_ptr<QuantLib::SimpleQuote> spot(new QuantLib::SimpleQuote(underlying));
    boost::shared_ptr<QuantLib::SimpleQuote> qRate(new QuantLib::SimpleQuote(dividendYield));
    boost::shared_ptr<QuantLib::YieldTermStructure> qTS = flatRate(today,qRate,dc);
    boost::shared_ptr<QuantLib::SimpleQuote> rRate(new QuantLib::SimpleQuote(riskFreeRate));
    boost::shared_ptr<QuantLib::YieldTermStructure> rTS = flatRate(today,rRate,dc);
    boost::shared_ptr<QuantLib::SimpleQuote> vol(new QuantLib::SimpleQuote(volatility));
    boost::shared_ptr<QuantLib::BlackVolTermStructure> volTS = flatVol(today, vol, dc);

    boost::shared_ptr<QuantLib::StrikedTypePayoff> payoff;
    if (binType=="cash") {
        boost::shared_ptr<QuantLib::StrikedTypePayoff> con(new QuantLib::CashOrNothingPayoff(optionType, strike, cashPayoff));
        payoff = con;
    } else if (binType=="asset") {
        boost::shared_ptr<QuantLib::StrikedTypePayoff> aon(new QuantLib::AssetOrNothingPayoff(optionType, strike));
        payoff = aon;
    } else if (binType=="gap") {
        boost::shared_ptr<QuantLib::StrikedTypePayoff> gap(new QuantLib::GapPayoff(optionType, strike, cashPayoff));
        payoff = gap;
    } else {
        throw std::range_error("Unknown binary option type " + binType);
    }

    QuantLib::Date exDate = today + length;
    boost::shared_ptr<QuantLib::Exercise> exercise;
    if (excType=="american") {
        boost::shared_ptr<QuantLib::Exercise> amEx(new QuantLib::AmericanExercise(today, exDate));
        exercise = amEx;
    } else if (excType=="european") {
        boost::shared_ptr<QuantLib::Exercise> euEx(new QuantLib::EuropeanExercise(exDate));
        exercise = euEx;
    } else {
        throw std::range_error("Unknown binary exercise type " + excType);
    }

    boost::shared_ptr<QuantLib::BlackScholesMertonProcess> 
        stochProcess(new QuantLib::BlackScholesMertonProcess(QuantLib::Handle<QuantLib::Quote>(spot),
                                                             QuantLib::Handle<QuantLib::YieldTermStructure>(qTS),
                                                             QuantLib::Handle<QuantLib::YieldTermStructure>(rTS),
                                                             QuantLib::Handle<QuantLib::BlackVolTermStructure>(volTS)));

    boost::shared_ptr<QuantLib::PricingEngine> engine;
    if (excType=="american") {
        boost::shared_ptr<QuantLib::PricingEngine> amEng(new QuantLib::AnalyticDigitalAmericanEngine(stochProcess));
        engine = amEng;
    } else if (excType=="european") {
        boost::shared_ptr<QuantLib::PricingEngine> euEng(new QuantLib::AnalyticEuropeanEngine(stochProcess));
        engine = euEng;
    } else {
        throw std::range_error("Unknown binary exercise type " + excType);
    }

    QuantLib::VanillaOption opt(payoff, exercise);
    opt.setPricingEngine(engine);

    Rcpp::List rl = Rcpp::List::create(Rcpp::Named("value") = opt.NPV(),
                                       Rcpp::Named("delta") = opt.delta(),
                                       Rcpp::Named("gamma") = opt.gamma(),
                                       Rcpp::Named("vega") = (excType=="european") ? opt.vega() : R_NaN,
                                       Rcpp::Named("theta") = (excType=="european") ? opt.theta() : R_NaN,
                                       Rcpp::Named("rho") = (excType=="european") ? opt.rho() : R_NaN,
                                       Rcpp::Named("divRho") = (excType=="european") ? opt.dividendRho() : R_NaN);
    return rl;
}

// dumped core when we tried last
// no longer under 0.3.10 and g++ 4.0.1 (Aug 2005)
// [[Rcpp::export]]
double binaryOptionImpliedVolatilityEngine(std::string type,
                                           double value,
                                           double underlying,
                                           double strike,
                                           double dividendYield, 
                                           double riskFreeRate,
                                           double maturity,
                                           double volatility,
                                           double cashPayoff) {

    //RcppExport SEXP BinaryOptionImpliedVolatility(SEXP optionParameters) {
    int length = int(maturity*360 + 0.5); // FIXME: this could be better

    QuantLib::Option::Type optionType = getOptionType(type);

    // updated again for QuantLib 0.9.0, 
    // cf QuantLib-0.9.0/test-suite/digitaloption.cpp
    QuantLib::Date today = QuantLib::Date::todaysDate();
    QuantLib::Settings::instance().evaluationDate() = today;
    QuantLib::DayCounter dc = QuantLib::Actual360();
    boost::shared_ptr<QuantLib::SimpleQuote> spot(new QuantLib::SimpleQuote(underlying));
    boost::shared_ptr<QuantLib::SimpleQuote> qRate(new QuantLib::SimpleQuote(dividendYield));
    boost::shared_ptr<QuantLib::YieldTermStructure> qTS = flatRate(today, qRate, dc);
    boost::shared_ptr<QuantLib::SimpleQuote> rRate(new QuantLib::SimpleQuote(riskFreeRate));
    boost::shared_ptr<QuantLib::YieldTermStructure> rTS = flatRate(today, rRate, dc);
    boost::shared_ptr<QuantLib::SimpleQuote> vol(new QuantLib::SimpleQuote(volatility));
    boost::shared_ptr<QuantLib::BlackVolTermStructure> volTS = flatVol(today, vol, dc);
    
    boost::shared_ptr<QuantLib::StrikedTypePayoff> 
        payoff(new QuantLib::CashOrNothingPayoff(optionType, strike, cashPayoff));

    QuantLib::Date exDate = today + length;
    boost::shared_ptr<QuantLib::Exercise> exercise(new QuantLib::EuropeanExercise(exDate));

    boost::shared_ptr<QuantLib::BlackScholesMertonProcess> 
        stochProcess(new QuantLib::BlackScholesMertonProcess(QuantLib::Handle<QuantLib::Quote>(spot),
                                                             QuantLib::Handle<QuantLib::YieldTermStructure>(qTS),
                                                             QuantLib::Handle<QuantLib::YieldTermStructure>(rTS),
                                                             QuantLib::Handle<QuantLib::BlackVolTermStructure>(volTS)));
    //boost::shared_ptr<PricingEngine> engine(new AnalyticEuropeanEngine(stochProcess));
    boost::shared_ptr<QuantLib::PricingEngine> engine(new QuantLib::AnalyticBarrierEngine(stochProcess));

    QuantLib::VanillaOption opt(payoff, exercise);
    opt.setPricingEngine(engine);

    return opt.impliedVolatility(value, stochProcess);
}

// [[Rcpp::export]]
Rcpp::List barrierOptionEngine(std::string barrType,
                               std::string type,
                               double underlying,
                               double strike,
                               double dividendYield, 
                               double riskFreeRate,
                               double maturity,
                               double volatility,
                               double barrier, 
                               double rebate) {

    int length = int(maturity*360 + 0.5); // FIXME: this could be better
        
    QuantLib::Barrier::Type barrierType = QuantLib::Barrier::DownIn;
    if (barrType=="downin") {
        barrierType = QuantLib::Barrier::DownIn;
    } else if (barrType=="upin") {
        barrierType = QuantLib::Barrier::UpIn;
    } else if (barrType=="downout") {
        barrierType = QuantLib::Barrier::DownOut;
    } else if (barrType=="upout") {
        barrierType = QuantLib::Barrier::UpOut;
    } else {
        throw std::range_error("Unknown barrier type " + type);
    }

    QuantLib::Option::Type optionType = getOptionType(type);

    // new QuantLib 0.3.5 framework, updated for 0.3.7
    // updated again for QuantLib 0.9.0, 
    // cf QuantLib-0.9.0/test-suite/barrieroption.cpp
    QuantLib::Date today = QuantLib::Date::todaysDate();
    QuantLib::Settings::instance().evaluationDate() = today;
    QuantLib::DayCounter dc = QuantLib::Actual360();
    boost::shared_ptr<QuantLib::SimpleQuote> spot(new QuantLib::SimpleQuote(underlying));
    boost::shared_ptr<QuantLib::SimpleQuote> qRate(new QuantLib::SimpleQuote(dividendYield));
    boost::shared_ptr<QuantLib::YieldTermStructure> qTS = flatRate(today, qRate, dc);
    boost::shared_ptr<QuantLib::SimpleQuote> rRate(new QuantLib::SimpleQuote(riskFreeRate));
    boost::shared_ptr<QuantLib::YieldTermStructure> rTS = flatRate(today,rRate,dc);
    boost::shared_ptr<QuantLib::SimpleQuote> vol(new QuantLib::SimpleQuote(volatility));
    boost::shared_ptr<QuantLib::BlackVolTermStructure> volTS = flatVol(today, vol, dc);

    QuantLib::Date exDate = today + length;
    boost::shared_ptr<QuantLib::Exercise> exercise(new QuantLib::EuropeanExercise(exDate));
        
    boost::shared_ptr<QuantLib::StrikedTypePayoff> payoff(new QuantLib::PlainVanillaPayoff(optionType, strike));
    
    boost::shared_ptr<QuantLib::BlackScholesMertonProcess> 
        stochProcess(new QuantLib::BlackScholesMertonProcess(QuantLib::Handle<QuantLib::Quote>(spot),
                                                             QuantLib::Handle<QuantLib::YieldTermStructure>(qTS),
                                                             QuantLib::Handle<QuantLib::YieldTermStructure>(rTS),
                                                             QuantLib::Handle<QuantLib::BlackVolTermStructure>(volTS)));

    // Size timeSteps = 1;
    // bool antitheticVariate = false;
    // bool controlVariate = false;
    // Size requiredSamples = 10000;
    // double requiredTolerance = 0.02;
    // Size maxSamples = 1000000;
    // bool isBiased = false;

    boost::shared_ptr<QuantLib::PricingEngine> engine(new QuantLib::AnalyticBarrierEngine(stochProcess));

    // need to explicitly reference BarrierOption from QuantLib here
    QuantLib::BarrierOption barrierOption(barrierType,
                                          barrier,
                                          rebate,
                                          payoff,
                                          exercise);
    barrierOption.setPricingEngine(engine);

    Rcpp::List rl = Rcpp::List::create(Rcpp::Named("value") = barrierOption.NPV(),
                                       Rcpp::Named("delta") = R_NaReal,
                                       Rcpp::Named("gamma") = R_NaReal,
                                       Rcpp::Named("vega") = R_NaReal,
                                       Rcpp::Named("theta") = R_NaReal,
                                       Rcpp::Named("rho") = R_NaReal,
                                       Rcpp::Named("divRho") = R_NaReal);
    return rl;
}

