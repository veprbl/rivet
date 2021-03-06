// -*- C++ -*-
#include "Rivet/Analysis.hh"

namespace Rivet {

  /// @brief Analysis for the generated cross section
  class MC_WEIGHTS : public Analysis {
  public:

    /// @name Constructors etc.
    //@{

    /// Constructor
    DEFAULT_RIVET_ANALYSIS_CTOR(MC_WEIGHTS);

    //@}


  public:

    /// @name Analysis methods
    //@{

    /// Book histograms and initialise projections before the run
    void init() {
      /// @todo Convert to Scatter1D or Counter
      book(_h_weight_100, "weight_100", 200, -100.0, 100.0);
      book(_h_weight_10,  "weight_10",  200,  -10.0,  10.0);
      book(_h_logweight_pos, "logweight_pos", logspace(100, 0.1, 10000.0));
      book(_h_logweight_neg, "logweight_neg", logspace(100, 0.1, 10000.0));

      book(_h_xsfraction_neg, "xsfraction_neg");
    }


    /// Perform the per-event analysis
    void analyze(const Event& event) { 

      for (size_t m = 0; m < event.weights().size(); ++m) {
        const double weight = event.weights()[m];
        _h_weight_100.get()->_getPersistent(m)->fill(weight, 1.0);
        _h_weight_10.get()->_getPersistent(m)->fill(weight, 1.0);
        if (weight < 0.) {
          _h_logweight_neg.get()->_getPersistent(m)->fill(fabs(weight), 1.0);
        } else {
          _h_logweight_pos.get()->_getPersistent(m)->fill(weight, 1.0);
        }
      }
    }


    /// Normalise histograms etc., after the run
    void finalize() {
      const double sf = 1.0 / numEvents();
      scale(_h_weight_100, sf);
      scale(_h_weight_10, sf);
      scale(_h_logweight_pos, sf);
      scale(_h_logweight_neg, sf);

      const double totalSumW  = _h_logweight_neg->sumW() + _h_logweight_pos->sumW();
      const double totalSumW2 = _h_logweight_neg->sumW2() + _h_logweight_pos->sumW2();
      const double negFrac = _h_logweight_neg->sumW() / totalSumW;
      const double negFracErr = negFrac * totalSumW / sqrt(totalSumW2);
      _h_xsfraction_neg->addPoint(0, negFrac, 0.5, negFracErr);
    }

    //@}


  private:

    /// @name Histograms
    //@{
    Scatter2DPtr _h_xsfraction_neg;
    Histo1DPtr _h_weight_100, _h_weight_10, _h_logweight_pos, _h_logweight_neg;
    //@}

  };

  // The hook for the plugin system
  DECLARE_RIVET_PLUGIN(MC_WEIGHTS);
}
