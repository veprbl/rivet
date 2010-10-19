// -*- C++ -*-
#include "Rivet/Projections/WFinder.hh"
#include "Rivet/Projections/InvMassFinalState.hh"
#include "Rivet/Projections/MissingMomentum.hh"
#include "Rivet/Projections/MergedFinalState.hh"
#include "Rivet/Projections/ClusteredPhotons.hh"
#include "Rivet/Projections/VetoedFinalState.hh"
#include "Rivet/Tools/ParticleIdUtils.hh"
#include "Rivet/Tools/Logging.hh"
#include "Rivet/Cmp.hh"

namespace Rivet {


  WFinder::WFinder(const ChargedFinalState& fs_l,
                   PdgId pid,
                   double m2_min, double m2_max,
                   double missingET,
                   double dRmax) {
    _init(fs_l, pid, m2_min, m2_max, missingET, dRmax);
  }


  WFinder::WFinder(double etaMin, double etaMax,
                   double pTmin,
                   PdgId pid,
                   double m2_min, double m2_max,
                   double missingET,
                   double dRmax) {
    vector<pair<double, double> > etaRanges;
    etaRanges += std::make_pair(etaMin, etaMax);
    _init(etaRanges, pTmin, pid, m2_min, m2_max, missingET, dRmax);
  }


  WFinder::WFinder(const std::vector<std::pair<double, double> >& etaRanges,
                   double pTmin,
                   PdgId pid,
                   double m2_min, double m2_max,
                   double missingET,
                   double dRmax) {
    _init(etaRanges, pTmin, pid, m2_min, m2_max, missingET, dRmax);
  }


  void WFinder::_init(const std::vector<std::pair<double, double> >& etaRanges,
                      double pTmin,
                      PdgId pid,
                      double m2_min, double m2_max,
                      double missingET,
                      double dRmax) {
    ChargedFinalState fs_l(etaRanges, pTmin);
    _init(fs_l, pid, m2_min, m2_max, missingET, dRmax);
  }


  void WFinder::_init(const ChargedFinalState& fs_l,
                      PdgId pid,
                      double m2_min, double m2_max,
                      double missingET,
                      double dRmax)
  {
    setName("WFinder");

    // Check that the arguments are legal
    assert(abs(pid) == ELECTRON || abs(pid) == MUON);
    PdgId nu_pid = abs(pid) + 1;
    assert(abs(nu_pid) == NU_E || abs(nu_pid) == NU_MU);

    // Don't make pT or eta cuts on the neutrino
    IdentifiedFinalState fs_nu;
    fs_nu.acceptNeutrinos();

    // Make a merged final state projection for charged and neutral leptons
    MergedFinalState mergedFS(fs_l, fs_nu);

    // Mass range
    _m2_min = m2_min;
    _m2_max = m2_max;

    // Set ETmiss
    _etMiss = missingET;

    // Make and register an invariant mass final state for the W decay leptons
    vector<pair<PdgId, PdgId> > l_nu_ids;
    l_nu_ids += make_pair(abs(pid), -abs(nu_pid));
    l_nu_ids += make_pair(-abs(pid), abs(nu_pid));
    InvMassFinalState imfs(mergedFS, l_nu_ids, m2_min, m2_max);
    addProjection(imfs, "IMFS");

    // A projection for clustering photons on to the charged lepton
    ClusteredPhotons cphotons(FinalState(), imfs, dRmax);
    addProjection(cphotons, "CPhotons");

    // Projection for all signal constituents
    MergedFinalState signalFS(imfs, cphotons);
    addProjection(signalFS, "SignalParticles");

    // Add MissingMomentum proj to calc MET
    MissingMomentum vismom(signalFS);
    addProjection(vismom, "MissingET");

    // FS for non-signal bits of the event
    VetoedFinalState remainingFS;
    remainingFS.addVetoOnThisFinalState(signalFS);
    addProjection(remainingFS, "RFS");
  }


  /////////////////////////////////////////////////////


  const FinalState& WFinder::remainingFinalState() const {
    return getProjection<FinalState>("RFS");
  }


  const FinalState& WFinder::constituentsFinalState() const {
    return getProjection<FinalState>("SignalParticles");
  }


  const FinalState& WFinder::constituentLeptonsFinalState() const {
    return getProjection<FinalState>("IMFS");
  }


  const FinalState& WFinder::clusteredPhotonsFinalState() const
  {
    return getProjection<FinalState>("CPhotons");
  }


  int WFinder::compare(const Projection& p) const {
    PCmp cmp = mkNamedPCmp(p, "IMFS");
    if (cmp != EQUIVALENT) return cmp;

    cmp = mkNamedPCmp(p, "CPhotons");
    if (cmp != EQUIVALENT) return cmp;

    return EQUIVALENT;
  }


  void WFinder::clear() {
    _theParticles.clear();
  }


  void WFinder::project(const Event& e) {
    clear();
    const InvMassFinalState& imfs = applyProjection<InvMassFinalState>(e, "IMFS");

    Particle p1,p2;
    if(imfs.particles().size() == 2) {
      p1 = imfs.particles()[0];
      p2 = imfs.particles()[1];
    }
    else {
      // no candiate, return
      if(imfs.particles().empty()) {
	getLog() << Log::DEBUG << "No W+- candidates found" << " "
		 << imfs.particles().size() << " " << endl;
	return;
      }
      // more than one, pick the one nearer the W mass
      double deltaM = 1e30;
      for(unsigned int ix=0;ix<imfs.particlePairs().size();++ix) {
	FourMomentum pW = imfs.particlePairs()[ix].first .momentum()+
	  imfs.particlePairs()[ix].second.momentum();
	double mW = pW.mass();
	if(abs(mW-80.403)<deltaM) {
	  deltaM = abs(mW-80.4);
	  p1 = imfs.particlePairs()[ix].first ;
	  p2 = imfs.particlePairs()[ix].second;
	}
      }
    }

    FourMomentum pW = p1.momentum() + p2.momentum();
    const int w3charge = PID::threeCharge(p1) + PID::threeCharge(p2);
    assert(abs(w3charge) == 3);
    const int wcharge = w3charge/3;

    stringstream msg;
    string wsign = (wcharge == 1) ? "+" : "-";
    string wstr = "W" + wsign;
    msg << wstr << " reconstructed from: " << endl
        << "   " << p1.momentum() << " " << p1.pdgId() << endl
        << " + " << p2.momentum() << " " << p2.pdgId() << endl;

    // Add in clustered photons
    const FinalState& photons = applyProjection<FinalState>(e, "CPhotons");
    foreach (const Particle& photon, photons.particles()) {
      msg << " + " << photon.momentum() << " " << photon.pdgId() << endl;
      pW += photon.momentum();
    }
    msg << " = " << pW;

    // Check missing ET
    const MissingMomentum& vismom = applyProjection<MissingMomentum>(e, "MissingET");
    /// @todo Restrict missing momentum eta range? Use vectorET()?
    if (vismom.scalarET() < _etMiss) {
      getLog() << Log::DEBUG << "Not enough missing ET: " << vismom.scalarET()/GeV
               << " GeV vs. " << _etMiss/GeV << " GeV" << endl;
      return;
    }

    // Check mass range again
    if (!inRange(pW.mass()/GeV, _m2_min, _m2_max)) return;
    getLog() << Log::DEBUG << msg.str() << endl;

    // Make W Particle and insert into particles list
    const PdgId wpid = (wcharge == 1) ? WPLUSBOSON : WMINUSBOSON;
    _theParticles.push_back(Particle(wpid, pW));
  }


}
