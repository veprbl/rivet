// -*- C++ -*-
#include "Rivet/Analysis.hh"
#include "Rivet/RivetAIDA.hh"
#include "Rivet/Tools/Logging.hh"
#include "Rivet/Tools/ParticleIdUtils.hh"
#include "Rivet/Projections/ChargedFinalState.hh"
#include "Rivet/Projections/UnstableFinalState.hh"
#include "LWH/Histogram1D.h"
#include "HepMC/GenParticle.h"
#include "HepMC/GenVertex.h"

//using namespace std;
//using namespace Rivet;


namespace Rivet {

  class ATLAS_2011_I944826 : public Analysis {
  public:

    /// Constructor
    ATLAS_2011_I944826()
      : Analysis("ATLAS_2011_I944826")
    {
        setBeams(PROTON, PROTON);
        _sum_w_ks     = 0.0;
        _sum_w_lambda = 0.0;
        _sum_w_passed = 0.0;
    }


  public:

    /// Book histograms and initialise projections before the run
    void init() {

      UnstableFinalState ufs(-MAXRAPIDITY, MAXRAPIDITY, 100*MeV);
      addProjection(ufs, "UFS");
      
      ChargedFinalState  cfs(-2.5, 2.5, 100*MeV);
      addProjection(cfs, "CFS");

      if (fuzzyEquals(sqrtS()*GeV, 7000, 1E-3)) {
        _hist_Ks_pT   = bookHistogram1D(1,1,1);
        _hist_Ks_y    = bookHistogram1D(2,1,1);
        _hist_Ks_mult = bookHistogram1D(3,1,1);
        _hist_L_pT    = bookHistogram1D(7,1,1);
        _hist_L_y     = bookHistogram1D(8,1,1);
        _hist_L_mult  = bookHistogram1D(9,1,1);
        _hist_Ratio_v_y      = bookDataPointSet(13,1,1);
        _hist_Ratio_v_pT     = bookDataPointSet(14,1,1);
        _temp_lambda_v_y.reset(    new LWH::Histogram1D(10, 0.0, 2.5));
        _temp_lambdabar_v_y.reset( new LWH::Histogram1D(10, 0.0, 2.5));
        _temp_lambda_v_pT.reset(   new LWH::Histogram1D(18, 0.5, 4.1));
        _temp_lambdabar_v_pT.reset(new LWH::Histogram1D(18, 0.5, 4.1));
      } 
      else if (fuzzyEquals(sqrtS()*GeV, 900, 1E-3)) {
        _hist_Ks_pT   = bookHistogram1D(4,1,1);
        _hist_Ks_y    = bookHistogram1D(5,1,1);
        _hist_Ks_mult = bookHistogram1D(6,1,1);
        _hist_L_pT    = bookHistogram1D(10,1,1);
        _hist_L_y     = bookHistogram1D(11,1,1);
        _hist_L_mult  = bookHistogram1D(12,1,1);
        _hist_Ratio_v_y      = bookDataPointSet(15,1,1); 
        _hist_Ratio_v_pT     = bookDataPointSet(16,1,1);
        _temp_lambda_v_y.reset(    new LWH::Histogram1D(5, 0.0, 2.5));
        _temp_lambdabar_v_y.reset( new LWH::Histogram1D(5, 0.0, 2.5));
        _temp_lambda_v_pT.reset(   new LWH::Histogram1D(8, 0.5, 3.7));
        _temp_lambdabar_v_pT.reset(new LWH::Histogram1D(8, 0.5, 3.7));
      }
    }

    // This function is required to impose the flight time cuts on Kaons and Lambdas
    inline double getPerpFlightDistance(const Rivet::Particle& p) {
      const HepMC::GenParticle& genp = p.genParticle();
      HepMC::GenVertex* prodV = genp.production_vertex();
      HepMC::GenVertex* decV  = genp.end_vertex();
      const HepMC::ThreeVector prodPos = prodV->point3d();
      if (decV) {
        const HepMC::ThreeVector decPos =  decV->point3d();
        double dy = prodPos.y() - decPos.y();
        double dx = prodPos.x() - decPos.x();
        return sqrt(dx*dx + dy*dy);
      }
      else return 9999999.;
    }

    inline bool daughtersSurviveCuts(const Rivet::Particle& p) {
      // We require the Kshort or Lambda to decay into two charged 
      // particles with at least 100MeV pT inside acceptance region
      const HepMC::GenParticle& genp = p.genParticle();
      HepMC::GenVertex* decV  = genp.end_vertex();
      bool decision = true;

      if (!decV) return false;
      if (decV->particles_out_size() == 2) {
        std::vector<double> pTs;
        std::vector<int> charges;
        std::vector<double> etas;
        for (HepMC::GenVertex::particles_out_const_iterator pp = decV->particles_out_const_begin() ;
             pp != decV->particles_out_const_end() ; ++pp) {
          pTs.push_back((*pp)->momentum().perp());
          etas.push_back(fabs((*pp)->momentum().eta()));
          charges.push_back( Rivet::PID::threeCharge((*pp)->pdg_id()));
          //(*pp)->print();
        }
        if ( (pTs[0]/Rivet::GeV < 0.1) || (pTs[1]/Rivet::GeV < 0.1) ) {
          decision = false;
          MSG_DEBUG("Failed pT cut: " << pTs[0]/Rivet::GeV << " " << pTs[1]/Rivet::GeV);
        }
        if ( etas[0] > 2.5 || etas[1] > 2.5 ) {
          decision = false;
          MSG_DEBUG("Failed eta cut: " << etas[0] << " " << etas[1]);
        }
        if ( charges[0] * charges[1] >= 0 ) {
          decision = false;
          MSG_DEBUG("Failed opposite charge cut: " << charges[0] << " " << charges[1]);
        }
      }
      else {
        decision = false;
        MSG_DEBUG("Failed nDaughters cut: " << decV->particles_out_size());
      }

      return decision;
    }

    /// Perform the per-event analysis
    void analyze(const Event& event) {
      const double weight = event.weight();

      // ATLAS MBTS trigger requirement of at least one hit in either hemisphere
      const ChargedFinalState& cfs = applyProjection<ChargedFinalState>(event, "CFS");
      int n_mbts = 0;
      foreach (const Particle& p, cfs.particles()) {
        const double eta = fabs(p.momentum().eta());
        if (inRange(eta, 2.09, 3.84)) n_mbts++; 
      }

      if (n_mbts < 1) {
        MSG_DEBUG("Failed trigger cut");
        vetoEvent;
      }

      // Veto event also when we find less than 2 particles in the acceptance region of type 211,2212,11,13,321
      int n_stable = 0;
      foreach (const Particle& p, cfs.particles()) {
        const PdgId pid = fabs(p.pdgId());
        if (pid == 11 || pid == 13 || pid == 211 || pid == 321 || pid == 2212) n_stable++;
      }

      if (n_stable < 2) {
        MSG_DEBUG("Failed stable particle cut");
        vetoEvent;
      }
      _sum_w_passed += weight;

      // This ufs holds all the Kaons and Lambdas
      const UnstableFinalState& ufs = applyProjection<UnstableFinalState>(event, "UFS");
     
      // Some conters
      int n_KS0 = 0; 
      int n_LAMBDA = 0; 

      // Particle loop
      foreach (const Particle& p, ufs.particles()) {
       
        // General particle quantities
        const double pT = p.momentum().pT()*GeV;
        const double y = p.momentum().rapidity();
        const PdgId pid = fabs(p.pdgId());

        double flightd = 0.0;

        // Look for Kaons, Lambdas
        switch (pid) {
        
          case K0S:
            flightd = getPerpFlightDistance(p);
            if (!inRange(flightd, 4., 450.) ){
              MSG_DEBUG("Kaon failed flight distance cut:" << flightd);
              break;
            }
            if (daughtersSurviveCuts(p) ) {
              _hist_Ks_y ->fill(y,  weight);
              _hist_Ks_pT->fill(pT, weight);
              
              _sum_w_ks += weight;
              n_KS0++;
            }
          break;

          case LAMBDA:
            if (pT < 0.5) { // Lambdas have an additional pT cut of 500 MeV
              MSG_DEBUG("Lambda failed pT cut:" << pT);
              break; 
            }
            flightd = getPerpFlightDistance(p);
            if (!inRange(flightd, 17., 450.)) {
              MSG_DEBUG("Lambda failed flight distance cut:" << flightd);
              break;
            }
            if ( daughtersSurviveCuts(p) ) {
              if (p.pdgId() == 3122) {
                _temp_lambda_v_y    ->fill(fabs(y), weight);
                _temp_lambda_v_pT   ->fill(pT,      weight);
                
                _hist_L_y->fill( y,  weight);
                _hist_L_pT->fill(pT, weight);
               
                _sum_w_lambda += weight;
                n_LAMBDA++;
                }
              
              else if (p.pdgId() == -3122) {
                _temp_lambdabar_v_y ->fill(fabs(y), weight);
                _temp_lambdabar_v_pT->fill(pT,      weight);
              }
            }
          break;
      
        } // End of switch
      
      }// End of particle loop

      // Fill multiplicity histos
      _hist_Ks_mult->fill(n_KS0, weight);
      _hist_L_mult->fill(n_LAMBDA, weight);
    }
      


    /// Normalise histograms etc., after the run
    void finalize() {
      MSG_INFO("Events that pass the trigger: " << _sum_w_passed);
      MSG_INFO("Kshort events: " << _sum_w_ks);
      MSG_INFO("Lambda events: " << _sum_w_lambda);

      scale(_hist_Ks_pT,   1.0/_sum_w_ks);
      scale(_hist_Ks_y,    1.0/_sum_w_ks);
      scale(_hist_Ks_mult, 1.0/_sum_w_passed);

      scale(_hist_L_pT,   1.0/_sum_w_lambda);
      scale(_hist_L_y,    1.0/_sum_w_lambda);
      scale(_hist_L_mult, 1.0/_sum_w_passed);
     

      // Division of histograms to obtain lambdabar/lambda ratios
      if (fuzzyEquals(sqrtS()*GeV, 7000, 1E-3)) {
        histogramFactory().divide(histoPath("d13-x01-y01"),  *_temp_lambdabar_v_y, *_temp_lambda_v_y );
        histogramFactory().divide(histoPath("d14-x01-y01"), *_temp_lambdabar_v_pT, *_temp_lambda_v_pT);
      }                                                                                              
      else if (fuzzyEquals(sqrtS()*GeV, 900, 1E-3)) {                                                
        histogramFactory().divide(histoPath("d15-x01-y01"),  *_temp_lambdabar_v_y, *_temp_lambda_v_y );
        histogramFactory().divide(histoPath("d16-x01-y01"), *_temp_lambdabar_v_pT, *_temp_lambda_v_pT);
      }
    }



  private:

    // Data members like post-cuts event weight counters go here
    double _sum_w_ks    ;
    double _sum_w_lambda;
    double _sum_w_passed;

    /// @name Histograms
    AIDA::IHistogram1D *_hist_Ks_pT;
    AIDA::IHistogram1D *_hist_Ks_y;
    AIDA::IHistogram1D *_hist_Ks_mult;

    AIDA::IHistogram1D *_hist_L_pT;
    AIDA::IHistogram1D *_hist_L_y;
    AIDA::IHistogram1D *_hist_L_mult;

    AIDA::IDataPointSet *_hist_Ratio_v_pT;
    AIDA::IDataPointSet *_hist_Ratio_v_y;
    
    shared_ptr<LWH::Histogram1D> _temp_lambda_v_y,  _temp_lambdabar_v_y;
    shared_ptr<LWH::Histogram1D> _temp_lambda_v_pT, _temp_lambdabar_v_pT;
  };



  // The hook for the plugin system
  //DECLARE_RIVET_PLUGIN(ATLAS_2011_I944826);
  AnalysisBuilder<ATLAS_2011_I944826> plugin_ATLAS_2011_I944826;

}
