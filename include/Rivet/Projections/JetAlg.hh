// -*- C++ -*-
#ifndef RIVET_JetAlg_HH
#define RIVET_JetAlg_HH

#include "Rivet/Projection.hh"
#include "Rivet/Projections/FinalState.hh"
#include "Rivet/Particle.hh"
#include "Rivet/Jet.hh"

namespace Rivet {

  
  /// Abstract base class for projections which can return a set of {@link Jet}s.
  class JetAlg : public Projection {
    
  public:
    
    /// Destructor
    virtual ~JetAlg() { }

    /// Get the jets (unordered).
    virtual Jets getJets() const = 0;

    /// Get the jets, ordered by \f$ p_T \f$.
    //virtual Jets getJetsPt() const = 0;

    /// Get the jets, ordered by supplied sorting function object.
    //virtual Jets getJets(sortingFunction) const = 0;


  protected:   

    /// Perform the projection on the Event.
    virtual void project(const Event& e) = 0;

    /// Compare projections.
    virtual int compare(const Projection& p) const = 0;

  };

}

#endif
