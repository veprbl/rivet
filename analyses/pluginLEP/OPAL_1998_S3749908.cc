// -*- C++ -*-
#include "Rivet/Analysis.hh"
#include "Rivet/Projections/Beam.hh"
#include "Rivet/Projections/FinalState.hh"
#include "Rivet/Projections/ChargedFinalState.hh"
#include "Rivet/Projections/UnstableFinalState.hh"

namespace Rivet {


  /// @brief OPAL photon/light meson paper
  /// @author Peter Richardson
  class OPAL_1998_S3749908 : public Analysis {
  public:

    /// Constructor
    OPAL_1998_S3749908()
      : Analysis("OPAL_1998_S3749908")
    {}


    /// @name Analysis methods
    //@{

    void init() {
      declare(Beam(), "Beams");
      declare(ChargedFinalState(), "FS");
      declare(UnstableFinalState(), "UFS");
      book(_histXePhoton   , 2, 1, 1);
      book(_histXiPhoton   , 3, 1, 1);
      book(_histXePi       , 4, 1, 1);
      book(_histXiPi       , 5, 1, 1);
      book(_histXeEta      , 6, 1, 1);
      book(_histXiEta      , 7, 1, 1);
      book(_histXeRho      , 8, 1, 1);
      book(_histXiRho      , 9, 1, 1);
      book(_histXeOmega    ,10, 1, 1);
      book(_histXiOmega    ,11, 1, 1);
      book(_histXeEtaPrime ,12, 1, 1);
      book(_histXiEtaPrime ,13, 1, 1);
      book(_histXeA0       ,14, 1, 1);
      book(_histXiA0       ,15, 1, 1);
    }


    void analyze(const Event& e) {
      // First, veto on leptonic events by requiring at least 4 charged FS particles
      const FinalState& fs = apply<FinalState>(e, "FS");
      const size_t numParticles = fs.particles().size();

      // Even if we only generate hadronic events, we still need a cut on numCharged >= 2.
      if (numParticles < 2) {
        MSG_DEBUG("Failed leptonic event cut");
        vetoEvent;
      }
      MSG_DEBUG("Passed leptonic event cut");

      // Get event weight for histo filling
      const double weight = 1.0;

      // Get beams and average beam momentum
      const ParticlePair& beams = apply<Beam>(e, "Beams").beams();
      const double meanBeamMom = ( beams.first.p3().mod() +
                                   beams.second.p3().mod() ) / 2.0;
      MSG_DEBUG("Avg beam momentum = " << meanBeamMom);

      // Final state of unstable particles to get particle spectra
      const UnstableFinalState& ufs = apply<UnstableFinalState>(e, "UFS");

      foreach (const Particle& p, ufs.particles()) {
        const int id = p.abspid();
        double xi = -log(p.p3().mod()/meanBeamMom);
        double xE = p.E()/meanBeamMom;
        switch (id) {
        case 22: // Photons
          _histXePhoton->fill(xE, weight);
          _histXiPhoton->fill(xi, weight);
          break;
        case 111: // Neutral pions
          _histXePi->fill(xE, weight);
          _histXiPi->fill(xi, weight);
          break;
        case 221: // eta
          _histXeEta->fill(xE, weight);
          _histXiEta->fill(xi, weight);
          break;
        case 213: // Charged rho (770)
          _histXeRho->fill(xE, weight);
          _histXiRho->fill(xi, weight);
          break;
        case 223: // omega (782)
          _histXeOmega->fill(xE, weight);
          _histXiOmega->fill(xi, weight);
          break;
        case 331: // eta' (958)
          _histXeEtaPrime->fill(xE, weight);
          _histXiEtaPrime->fill(xi, weight);
          break;
        case 9000211: // Charged a_0 (980)
          _histXeA0->fill(xE, weight);
          _histXiA0->fill(xi, weight);
          break;
        }
      }
    }


    /// Finalize
    void finalize() {
      scale(_histXePhoton  , 1./sumOfWeights());
      scale(_histXiPhoton  , 1./sumOfWeights());
      scale(_histXePi      , 1./sumOfWeights());
      scale(_histXiPi      , 1./sumOfWeights());
      scale(_histXeEta     , 1./sumOfWeights());
      scale(_histXiEta     , 1./sumOfWeights());
      scale(_histXeRho     , 1./sumOfWeights());
      scale(_histXiRho     , 1./sumOfWeights());
      scale(_histXeOmega   , 1./sumOfWeights());
      scale(_histXiOmega   , 1./sumOfWeights());
      scale(_histXeEtaPrime, 1./sumOfWeights());
      scale(_histXiEtaPrime, 1./sumOfWeights());
      scale(_histXeA0      , 1./sumOfWeights());
      scale(_histXiA0      , 1./sumOfWeights());
    }

    //@}


  private:

      Histo1DPtr _histXePhoton  ;
      Histo1DPtr _histXiPhoton  ;
      Histo1DPtr _histXePi      ;
      Histo1DPtr _histXiPi      ;
      Histo1DPtr _histXeEta     ;
      Histo1DPtr _histXiEta     ;
      Histo1DPtr _histXeRho     ;
      Histo1DPtr _histXiRho     ;
      Histo1DPtr _histXeOmega   ;
      Histo1DPtr _histXiOmega   ;
      Histo1DPtr _histXeEtaPrime;
      Histo1DPtr _histXiEtaPrime;
      Histo1DPtr _histXeA0      ;
      Histo1DPtr _histXiA0      ;
    //@}

  };

  // The hook for the plugin system
  DECLARE_RIVET_PLUGIN(OPAL_1998_S3749908);

}