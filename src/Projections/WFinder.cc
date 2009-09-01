// -*- C++ -*-
#include "Rivet/Projections/WFinder.hh"
#include "Rivet/Projections/InvMassFinalState.hh"
#include "Rivet/Projections/ClusteredPhotons.hh"
#include "Rivet/Projections/VetoedFinalState.hh"
#include "Rivet/Tools/Logging.hh"
#include "Rivet/Cmp.hh"

namespace Rivet {


  WFinder::WFinder(const FinalState& fs,
                   PdgId pid,
                   double m2_min, double m2_max,
                   double dRmax) {
    _init(fs, pid, m2_min, m2_max, dRmax);
  }


  WFinder::WFinder(double etaMin, double etaMax,
                   double pTmin,
                   PdgId pid,
                   double m2_min, double m2_max,
                   double dRmax) {
    vector<pair<double, double> > etaRanges;
    etaRanges += std::make_pair(etaMin, etaMax);
    _init(etaRanges, pTmin, pid, m2_min, m2_max, dRmax);
  }


  WFinder::WFinder(const std::vector<std::pair<double, double> >& etaRanges,
                   double pTmin,
                   PdgId pid,
                   double m2_min, const double m2_max,
                   double dRmax) {
    _init(etaRanges, pTmin, pid, m2_min, m2_max, dRmax);
  }


  void WFinder::_init(const std::vector<std::pair<double, double> >& etaRanges,
                      double pTmin,  PdgId pid,
                      double m2_min, double m2_max,
                      double dRmax) {
    FinalState fs(etaRanges, pTmin);
    _init(fs, pid, m2_min, m2_max, dRmax);
  }


  void WFinder::_init(const FinalState& fs,
                      PdgId pid,
                      double m2_min, double m2_max,
                      double dRmax)
  {
    setName("WFinder");

    addProjection(fs, "FS");

    assert(abs(pid) == ELECTRON || abs(pid) == MUON || abs(pid) == TAU);
    PdgId nu_pid = -(abs(pid) + 1);
    assert(abs(nu_pid) == NU_E || abs(nu_pid) == NU_MU || abs(nu_pid) == NU_TAU);
    std::vector<std::pair<long, long> > l_nu_ids;
    l_nu_ids += std::make_pair(abs(pid), -abs(nu_pid));
    l_nu_ids += std::make_pair(-abs(pid), abs(nu_pid));
    InvMassFinalState imfs(fs, l_nu_ids, m2_min, m2_max);
    addProjection(imfs, "IMFS");
    
    ClusteredPhotons cphotons(FinalState(), imfs, dRmax);
    addProjection(cphotons, "CPhotons");

    VetoedFinalState remainingFS;
    remainingFS.addVetoOnThisFinalState(imfs);
    remainingFS.addVetoOnThisFinalState(cphotons);
    addProjection(remainingFS, "RFS");
  }


  /////////////////////////////////////////////////////


  const FinalState& WFinder::remainingFinalState() const
  {
    return getProjection<FinalState>("RFS");
  }


  const FinalState& WFinder::constituentsFinalState() const
  {
    return getProjection<FinalState>("IMFS");
  }


  int WFinder::compare(const Projection& p) const {
    PCmp cmp = mkNamedPCmp(p, "IMFS");
    if (cmp != PCmp::EQUIVALENT) return cmp;

    cmp = mkNamedPCmp(p, "CPhotons");
    if (cmp != PCmp::EQUIVALENT) return cmp;

    return PCmp::EQUIVALENT;
  } 
  

  void WFinder::project(const Event& e) {
    _theParticles.clear();

    const FinalState& imfs = applyProjection<FinalState>(e, "IMFS");
    if (imfs.particles().size() != 2) return;

    const FinalState& photons = applyProjection<FinalState>(e, "CPhotons");
    
    getLog() << Log::DEBUG << "Z reconstructed out of: " << endl
        << "  " << imfs.particles()[0].momentum() << " " << imfs.particles()[0].pdgId() << endl
        << " +" << imfs.particles()[1].momentum() << " " << imfs.particles()[1].pdgId() << endl;

    Particle W;
    FourMomentum pW = imfs.particles()[0].momentum() + imfs.particles()[1].momentum();
    foreach (const Particle& photon, photons.particles()) {
      getLog() << Log::DEBUG << " + " << photon.momentum() << " " << photon.pdgId() << endl;
      pW += photon.momentum();
    }
    getLog() << Log::DEBUG << " = " << W.momentum() << endl;
    W.setMomentum(pW);

    _theParticles.push_back(W);
    getLog() << Log::DEBUG << name() << " found " << _theParticles.size()
             << " W candidates." << endl;
  }
 
 
}
