// -*- C++ -*-
#ifndef RIVET_MissingMomentum_HH
#define RIVET_MissingMomentum_HH

#include "Rivet/Rivet.hh"
#include "Rivet/Projection.hh"
#include "Rivet/Projections/VisibleFinalState.hh"
#include "Rivet/Particle.hh"
#include "Rivet/Event.hh"

namespace Rivet {


  /// @brief Calculate missing \f$ E \f$, \f$ E_\perp \f$ etc.
  /// Project out the total visible energy vector, allowing missing
  /// \f E \f$, \f$ E_\perp \f$ etc. to be calculated. Final state 
  /// visibility restrictions are automatic.
  class MissingMomentum : public Projection { 
  public:

    /// Default constructor with uncritical FS.
    MissingMomentum()
    {
      setName("MissingMomentum");
      FinalState fs;
      addProjection(fs, "FS");
      addProjection(VisibleFinalState(fs), "VisibleFS");
    }

 
    /// Constructor.
    MissingMomentum(const FinalState& fs)
    {
      setName("MissingMomentum");
      addProjection(fs, "FS");
      addProjection(VisibleFinalState(fs), "VisibleFS");
    }


    /// Clone on the heap.
    virtual const Projection* clone() const {
      return new MissingMomentum(*this);
    }

 
  public:

    /// The projected four-momentum vector
    FourMomentum& momentum() { return _momentum; }

    /// The projected four-momentum vector
    const FourMomentum& momentum() const { return _momentum; }

    /// The projected scalar transverse energy
    double scalarET() const { return _set; }
 

  protected:
 
    /// Apply the projection to the event.
    void project(const Event& e);
 
    /// Compare projections.
    int compare(const Projection& p) const;
     

  public:

    /// Clear the projection results.
    void clear();


  private:
 
    /// The total visible momentum
    FourMomentum _momentum;
 
    /// Scalar transverse energy
    double _set;
 
  };

}


#endif
