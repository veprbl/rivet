// -*- C++ -*-
#include "Rivet/Analysis.hh"
#include "Rivet/RivetAIDA.hh"
#include "Rivet/Tools/ParticleIdUtils.hh"
#include "Rivet/Projections/FinalState.hh"
#include "Rivet/Projections/ChargedFinalState.hh"
#include "Rivet/Projections/TotalVisibleMomentum.hh"

namespace Rivet {


  class UA1_1990_S2044935 : public Analysis {
  public:

    /// Constructor
    UA1_1990_S2044935() : Analysis("UA1_1990_S2044935") {
      setBeams(PROTON, ANTIPROTON);
      setNeedsCrossSection(true);
      _sumwTrig = 0;
      _sumwTrig08 = 0;
      _sumwTrig40 = 0;
      _sumwTrig80 = 0;
    }
 

    /// @name Analysis methods
    //@{

    /// Book projections and histograms
    void init() {
      addProjection(ChargedFinalState(-5.5, 5.5), "TriggerFS");
      addProjection(ChargedFinalState(-2.5, 2.5), "TrackFS");
      const FinalState calofs(-6.0, 6.0);
      addProjection(TotalVisibleMomentum(calofs), "Mom");

      if (fuzzyEquals(sqrtS()/GeV, 63)) {
        _hist_Pt = bookProfile1D(8,1,1);
      } else if (fuzzyEquals(sqrtS()/GeV, 200)) {
        _hist_Nch = bookHistogram1D(1,1,1);
        _hist_Esigd3p = bookHistogram1D(2,1,1);
        _hist_Pt = bookProfile1D(6,1,1);
        _hist_Et = bookHistogram1D(9,1,1);
        _hist_Etavg = bookProfile1D(12,1,1);
      } else if (fuzzyEquals(sqrtS()/GeV, 500)) {
        _hist_Nch = bookHistogram1D(1,1,2);
        _hist_Esigd3p = bookHistogram1D(2,1,2);
        _hist_Et = bookHistogram1D(10,1,1);
        _hist_Etavg = bookProfile1D(12,1,2);
      } else if (fuzzyEquals(sqrtS()/GeV, 900)) {
        _hist_Nch = bookHistogram1D(1,1,3);
        _hist_Esigd3p = bookHistogram1D(2,1,3);
        _hist_Pt = bookProfile1D(7,1,1);
        _hist_Et = bookHistogram1D(11,1,1);
        _hist_Etavg = bookProfile1D(12,1,3);
        _hist_Esigd3p08 = bookHistogram1D(3,1,1);
        _hist_Esigd3p40 = bookHistogram1D(4,1,1);
        _hist_Esigd3p80 = bookHistogram1D(5,1,1);
      }

    }
 

    void analyze(const Event& event) {
      // Trigger
      const FinalState& trigfs = applyProjection<FinalState>(event, "TriggerFS");
      unsigned int n_minus(0), n_plus(0);
      foreach (const Particle& p, trigfs.particles()) {
        const double eta = p.momentum().eta();
        if (inRange(eta, -5.5, -1.5)) n_minus++;
        else if (inRange(eta, 1.5, 5.5)) n_plus++;
      }
      getLog() << Log::DEBUG << "Trigger -: " << n_minus << ", Trigger +: " << n_plus << endl;
      if (n_plus == 0 || n_minus == 0) vetoEvent;
      const double weight = event.weight();
      _sumwTrig += weight;

      // Use good central detector tracks
      const FinalState& cfs = applyProjection<FinalState>(event, "TrackFS");
      const double Et = applyProjection<TotalVisibleMomentum>(event, "Mom").scalarET();
      const unsigned int nch = cfs.size();

      // Event level histos
      _hist_Nch->fill(nch, weight);
      _hist_Et->fill(Et/GeV, weight);
      /// @todo Doesn't fit data
      _hist_Etavg->fill(nch, Et/GeV, weight);

      // Particle/track level histos
      const double deta = 2 * 5.0;
      const double dphi = TWOPI;
      const double dnch_deta = nch/deta;
      foreach (const Particle& p, cfs.particles()) {
        /// @todo Use pion-mass trick (see CDF 2009) for eta -> y in d3sig/dp3?
        const double pt = p.momentum().pT();
        const double scaled_weight = weight/(deta*dphi*pt/GeV);
        _hist_Pt->fill(nch, pt/GeV, weight);
        if (!fuzzyEquals(sqrtS()/GeV, 63, 1E-3)) {
          _hist_Esigd3p->fill(pt/GeV, scaled_weight);
        }
        // Also fill for specific dn/deta ranges at 900 GeV
        /// @todo Doesn't fit data: are these really ranges in dNch/deta? Scale factors?
        if (fuzzyEquals(sqrtS()/GeV, 900, 1E-3)) {
          if (inRange(dnch_deta, 0.8, 4.0)) {
            _sumwTrig08 += weight;
            _hist_Esigd3p08->fill(pt/GeV, scaled_weight);
          } else if (inRange(dnch_deta, 4.0, 8.0)) {
            _sumwTrig40 += weight;
            _hist_Esigd3p40->fill(pt/GeV, scaled_weight);
          } else if(dnch_deta > 8.0) {
            _sumwTrig80 += weight;
            _hist_Esigd3p80->fill(pt/GeV, scaled_weight);
          }
        }
      }
   
    }
 
 
    void finalize() {
      if (_sumwTrig <= 0) {
        getLog() << Log::WARN << "No events passed the trigger!" << endl;
        return;
      }
      const double xsec = crossSectionPerEvent();
      scale(_hist_Nch, 2*xsec/millibarn); //< Factor of 2 for Nch bin widths?
      scale(_hist_Esigd3p, xsec/millibarn);
      scale(_hist_Et, xsec/millibarn);
      if (fuzzyEquals(sqrtS()/GeV, 900, 1E-3)) {
        scale(_hist_Esigd3p08, xsec/microbarn);
        scale(_hist_Esigd3p40, xsec/microbarn);
        scale(_hist_Esigd3p80, xsec/microbarn);
      }
    }
 
    //@}

 
  private:

    /// @name Weight counters
    //@{
    double _sumwTrig, _sumwTrig08, _sumwTrig40, _sumwTrig80;
    //@}

    /// @name Histogram collections
    //@{
    AIDA::IHistogram1D* _hist_Nch;
    AIDA::IHistogram1D* _hist_Esigd3p;
    AIDA::IHistogram1D* _hist_Esigd3p08;
    AIDA::IHistogram1D* _hist_Esigd3p40;
    AIDA::IHistogram1D* _hist_Esigd3p80;
    AIDA::IProfile1D* _hist_Pt;
    AIDA::IProfile1D* _hist_Etavg;
    AIDA::IHistogram1D* _hist_Et;
    //@}
 
  };



  // This global object acts as a hook for the plugin system
  AnalysisBuilder<UA1_1990_S2044935> plugin_UA1_1990_S2044935;

}
