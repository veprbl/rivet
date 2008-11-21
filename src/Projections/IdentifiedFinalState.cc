// -*- C++ -*-
#include "Rivet/Rivet.hh"
#include "Rivet/Projections/IdentifiedFinalState.hh"
#include "Rivet/Cmp.hh"
#include "Rivet/Tools/Utils.hh"
#include <algorithm>

namespace Rivet {


  int IdentifiedFinalState::compare(const Projection& p) const {
    const PCmp fscmp = mkNamedPCmp(p, "FS");
    if (fscmp != PCmp::EQUIVALENT) return fscmp;

    const IdentifiedFinalState& other = dynamic_cast<const IdentifiedFinalState&>(p);
    int pidssize = cmp(_pids.size(), other._pids.size());
    if (pidssize != PCmp::EQUIVALENT) return pidssize;
    return cmp(_pids, other._pids);
  }


  void IdentifiedFinalState::project(const Event& e) {
    const FinalState& fs = applyProjection<FinalState>(e, "FS");
    _theParticles.clear();
    _theParticles.reserve(fs.particles().size());
    foreach (const Particle& p, fs.particles()) {
      if (acceptedIds().find(p.pdgId()) != acceptedIds().end()) {
        _theParticles.push_back(p);
      }
    }
  }
  

}
