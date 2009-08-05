// -*- C++ -*-
#include "Rivet/Tools/Logging.hh"
#include "Rivet/RivetAIDA.hh"
#include "Rivet/Analyses/ALEPH_1991_S2435284.hh"
#include "Rivet/Projections/ChargedFinalState.hh"
#include "Rivet/Projections/Multiplicity.hh"

namespace Rivet {


  ALEPH_1991_S2435284::ALEPH_1991_S2435284() 
    : Analysis("ALEPH_1991_S2435284")
  {
    setBeams(ELECTRON, POSITRON); 
    const ChargedFinalState cfs;
    addProjection(cfs, "FS");
    addProjection(Multiplicity(cfs), "Mult");
  }
  
  
  void ALEPH_1991_S2435284::init() {
    // Book histogram
    _histChTot = bookHistogram1D(1, 1, 1);
  }


  // Do the analysis
  void ALEPH_1991_S2435284::analyze(const Event& event) {
    const Multiplicity& m = applyProjection<Multiplicity>(event, "Mult");
    getLog() << Log::DEBUG << "Total charged multiplicity = " << m.totalMultiplicity() << endl;
    _histChTot->fill(m.totalMultiplicity(), event.weight());
  }


  // Finalize
  void ALEPH_1991_S2435284::finalize() { 
    // Normalize the histogram
    scale(_histChTot, 2.0/sumOfWeights()); // same as in ALEPH 1996
  }


}
