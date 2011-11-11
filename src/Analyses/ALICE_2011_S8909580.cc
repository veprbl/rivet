#include "Rivet/Analysis.hh"
#include "Rivet/RivetAIDA.hh"
#include "Rivet/Tools/Logging.hh"
#include "Rivet/Projections/FinalState.hh"
#include "Rivet/Projections/UnstableFinalState.hh"

namespace Rivet {


  class ALICE_2011_S8909580 : public Analysis {
  public:

    ALICE_2011_S8909580()
      : Analysis("ALICE_2011_S8909580")
    {}


  public:
    void init() {
      const UnstableFinalState ufs(-15, 15);
      addProjection(ufs, "UFS");

      _histPtK0s        = bookHistogram1D("d01-x01-y01");
      _histPtLambda     = bookHistogram1D("d02-x01-y01");
      _histPtAntiLambda = bookHistogram1D("d03-x01-y01");
      _histPtXi         = bookHistogram1D("d04-x01-y01");
      _histPtPhi        = bookHistogram1D("d05-x01-y01");
      _temp_h_Lambdas   = bookHistogram1D("temp_h_Lambdas", binEdges("d06-x01-y01"));
      _temp_h_Kzeros    = bookHistogram1D("temp_h_Kzeros",  binEdges("d06-x01-y01"));
    }

    void analyze(const Event& event) {
      const double weight = event.weight();
      const UnstableFinalState& ufs = applyProjection<UnstableFinalState>(event, "UFS");

      foreach (const Particle& p, ufs.particles()) {
        const double absrap = fabs(p.momentum().rapidity());
        const double pT = p.momentum().pT()/GeV;

        if (absrap<0.8) {
          switch(p.pdgId()) {
            case 3312:
            case -3312:
              if ( !( p.hasAncestor(3334) || p.hasAncestor(-3334) ) ) {
                _histPtXi->fill(pT, weight);
              }
              break;
            if (absrap<0.75) {
              case 310:
                _histPtK0s->fill(pT, weight);
                _temp_h_Kzeros->fill(pT, 2*weight);
                break;
              case 3122:
                if ( !( p.hasAncestor(3322) || p.hasAncestor(-3322) ||
                        p.hasAncestor(3312) || p.hasAncestor(-3312) ||
                        p.hasAncestor(3334) || p.hasAncestor(-3334) ) ) {
                  _histPtLambda->fill(pT, weight);
                  _temp_h_Lambdas->fill(pT, weight);
                }
                break;
              case -3122:
                if ( !( p.hasAncestor(3322) || p.hasAncestor(-3322) ||
                        p.hasAncestor(3312) || p.hasAncestor(-3312) ||
                        p.hasAncestor(3334) || p.hasAncestor(-3334) ) ) {
                  _histPtAntiLambda->fill(pT, weight);
                  _temp_h_Lambdas->fill(pT, weight);
                }
                break;
            }
            if (absrap<0.6) {
              case 333:
                _histPtPhi->fill(pT, weight);
                break;
            }
          }
        }
      }
    }

    void finalize() {
      scale(_histPtK0s,        1./(1.5*sumOfWeights()));
      scale(_histPtLambda,     1./(1.5*sumOfWeights()));
      scale(_histPtAntiLambda, 1./(1.5*sumOfWeights()));
      scale(_histPtXi,         1./(1.6*sumOfWeights()));
      scale(_histPtPhi,        1./(1.2*sumOfWeights()));

      histogramFactory().divide(histoPath("d06-x01-y01"), *_temp_h_Lambdas, *_temp_h_Kzeros);
      histogramFactory().destroy(_temp_h_Lambdas);
      histogramFactory().destroy(_temp_h_Kzeros);
    }


  private:

    AIDA::IHistogram1D *_histPtK0s;
    AIDA::IHistogram1D *_histPtLambda;
    AIDA::IHistogram1D *_histPtAntiLambda;
    AIDA::IHistogram1D *_histPtXi;
    AIDA::IHistogram1D *_histPtPhi;
    AIDA::IHistogram1D *_temp_h_Lambdas;
    AIDA::IHistogram1D *_temp_h_Kzeros;

  };



  // The hook for the plugin system
  DECLARE_RIVET_PLUGIN(ALICE_2011_S8909580);
}