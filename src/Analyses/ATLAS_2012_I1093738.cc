// -*- C++ -*-
#include <iostream>
#include <sstream>
#include <string>

#include "Rivet/Analysis.hh"
#include "Rivet/RivetAIDA.hh"
#include "Rivet/Tools/Logging.hh"
#include "Rivet/Projections/FinalState.hh"
#include "Rivet/Projections/LeadingParticlesFinalState.hh"
#include "Rivet/Projections/VetoedFinalState.hh"
#include "Rivet/Jet.hh"
#include "Rivet/Projections/FastJets.hh"

#include "fastjet/internal/base.hh"
#include "fastjet/JetDefinition.hh"
#include "fastjet/AreaDefinition.hh"
#include "fastjet/ClusterSequence.hh"
#include "fastjet/ClusterSequenceArea.hh"
#include "fastjet/PseudoJet.hh"

#define MYDEBUG if(false) getLog() << __LINE__


namespace Rivet {

  /// @brief Measurement of isolated gamma + jet + X differential cross-sections
  ///
  /// Inclusive isolated gamma + jet cross-sections, differential in pT(gamma), for
  /// various photon and jet rapidity configurations.
  ///
  /// @author Giovanni Marchiori

  class ATLAS_2012_I1093738 : public Analysis {
  public:

    // Constructor
    ATLAS_2012_I1093738()
      : Analysis("ATLAS_2012_I1093738")
    {
      setBeams(PROTON, PROTON);
      setNeedsCrossSection(true);

      _eta_bins_ph.push_back(0.0);
      _eta_bins_ph.push_back(1.37);
      _eta_bins_ph.push_back(1.52);
      _eta_bins_ph.push_back(2.37);

      _eta_bins_jet.push_back(0.0);
      _eta_bins_jet.push_back(1.2);
      _eta_bins_jet.push_back(2.8);
      _eta_bins_jet.push_back(4.4);

      _eta_bins_areaoffset.push_back(0.0);
      _eta_bins_areaoffset.push_back(1.5);
      _eta_bins_areaoffset.push_back(3.0);
    }

  public:

    // Book histograms and initialise projections before the run
    void init() {
      MYDEBUG << "Entering init." << std::endl;

      // Final state
      FinalState fs;
      addProjection(fs, "FS");

      // Voronoi eta-phi tassellation with KT jets, for ambient energy density calculation
      FastJets fj(fs, FastJets::KT, 0.5);
      _area_def = new fastjet::AreaDefinition(fastjet::VoronoiAreaSpec());
      fj.useJetArea(_area_def);
      addProjection(fj, "KtJetsD05");

      // Leading photon
      LeadingParticlesFinalState photonfs(FinalState(-1.37, 1.37, 25.0*GeV));
      photonfs.addParticleId(PHOTON);
      addProjection(photonfs, "LeadingPhoton");

      // FS excluding the leading photon
      VetoedFinalState vfs(fs);
      vfs.addVetoOnThisFinalState(photonfs);
      addProjection(vfs, "JetFS");

      // Jets
      FastJets jetpro(vfs, FastJets::ANTIKT, 0.4);
      //FastJets jetpro(vfs, FastJets::ANTIKT, 0.6);
      jetpro.useInvisibles();
      addProjection(jetpro, "Jets");


      MYDEBUG << "\t... Booking Histograms " << std::endl;
      // use autobook option, histo d??-x??-y??
      _h_phbarrel_jetcentral_SS = bookHistogram1D(1, 1, 1);
      _h_phbarrel_jetmedium_SS = bookHistogram1D(2, 1, 1);
      _h_phbarrel_jetforward_SS = bookHistogram1D(3, 1, 1);

      _h_phbarrel_jetcentral_OS = bookHistogram1D(4, 1, 1);
      _h_phbarrel_jetmedium_OS = bookHistogram1D(5, 1, 1);
      _h_phbarrel_jetforward_OS = bookHistogram1D(6, 1, 1);

      MYDEBUG << "Exiting init." << std::endl;
    }


    int getEtaBin(double eta_w, int what) const {
      double eta = fabs(eta_w);

      int v_iter=0;
      if (what==0) {
	for(v_iter=0; v_iter < (int)_eta_bins_ph.size()-1; v_iter++){
	  if(eta >= _eta_bins_ph.at(v_iter) && eta < _eta_bins_ph.at(v_iter+1))
	    break;
	}
      }
      else if (what==1) {
	for (v_iter=0; v_iter < (int)_eta_bins_jet.size()-1; v_iter++){
	  if(eta >= _eta_bins_jet.at(v_iter) && eta < _eta_bins_jet.at(v_iter+1))
	    break;
	}
      }
      else {
	for(v_iter=0; v_iter < (int)_eta_bins_areaoffset.size()-1; v_iter++){
	  if(eta >= _eta_bins_areaoffset.at(v_iter) && eta < _eta_bins_areaoffset.at(v_iter+1))
	    break;
	}
      }

      return v_iter;
    }


    // Perform the per-event analysis
    void analyze(const Event& event) {

      MYDEBUG << "Entering Analyze." << std::endl;

      const double weight = event.weight();

      MYDEBUG << "...First projections." << std::endl;


      // Get the photon
      const FinalState& photonfs = applyProjection<FinalState>(event, "LeadingPhoton");
      if (photonfs.particles().size() < 1) {
	MYDEBUG << "...Going to veto event(1)." << std::endl;
        vetoEvent;
      }
      MYDEBUG << "...Didn't veto event(1)." << std::endl;

      const FourMomentum photon = photonfs.particles().front().momentum();
      double eta_P = photon.eta();
      double phi_P = photon.phi();

      // Get the jet
      Jets jets = applyProjection<FastJets>(event, "Jets").jetsByPt(20.0*GeV);
      if (jets.size()==0) {
	MYDEBUG << "...Going to veto event(3)." << std::endl;
        vetoEvent;
      }
      MYDEBUG << "...Didn't veto event(3)." << std::endl;
      FourMomentum leadingJet = jets[0].momentum();

      // Require jet separated from photon
      if (deltaR(eta_P, phi_P, leadingJet.eta(), leadingJet.phi())<1.0) {
	MYDEBUG << "...Going to veto event(4)." << std::endl;
        vetoEvent;
      }
      MYDEBUG << "...Didn't veto event(4)." << std::endl;

      // Veto if leading jet is outside plotted rapidity regions
      const double abs_y1 = fabs(leadingJet.rapidity());
      if (abs_y1 > 4.4) {
	MYDEBUG << "...Going to veto event(5)." << std::endl;
        vetoEvent;
      }
      MYDEBUG << "...Didn't veto event(5)." << std::endl;


      // compute the median event energy density
      const unsigned int skipnhardjets = 0; 
      _ptDensity.clear();
      _sigma.clear();
      _Njets.clear();
      std::vector< std::vector<double> > ptDensities;
      std::vector<double> emptyVec;
      ptDensities.assign(_eta_bins_areaoffset.size()-1,emptyVec);
      
      const fastjet::ClusterSequenceArea* clust_seq_area = applyProjection<FastJets>(event, "KtJetsD05").clusterSeqArea();
      foreach (const fastjet::PseudoJet& jet, applyProjection<FastJets>(event, "KtJetsD05").pseudoJets(0.0*GeV)) {
	double eta = fabs(jet.eta());
	double pt = fabs(jet.perp());
	
	/// get the cluster sequence
	double area = clust_seq_area->area(jet);
	
	if(area > 10e-4 && fabs(eta)<_eta_bins_areaoffset[_eta_bins_areaoffset.size()-1]){
	  ptDensities.at(getEtaBin(fabs(eta),2)).push_back(pt/area);
	}
      }
      
      for(int b=0; b<(int)_eta_bins_areaoffset.size()-1; b++){
	double median = 0.0;
	double sigma = 0.0;
	int Njets = 0;
	if(ptDensities[b].size() > skipnhardjets)
	  {
	    std::sort(ptDensities[b].begin(), ptDensities[b].end());
	    int nDens = ptDensities[b].size() - skipnhardjets;
	    if( nDens%2 == 0 )
	      median = (ptDensities[b][nDens/2]+ptDensities[b][(nDens-2)/2])/2;
	    else
	      median = ptDensities[b][(nDens-1)/2];
	    sigma = ptDensities[b][(int)(.15865*nDens)];
	    Njets = nDens;
	  }
	_ptDensity.push_back(median);
	_sigma.push_back(sigma);
	_Njets.push_back(Njets);
      }


      // compute photon isolation
	
      // std EtCone
      ParticleVector fs = applyProjection<FinalState>(event, "JetFS").particles();
      FourMomentum mom_in_EtCone;
      float iso_dR = 0.4;
      float cluster_eta_width = 0.25*7.0;
      float cluster_phi_width = (PI/128.)*5.0;
      foreach (const Particle& p, fs) {
	// check if it's in the cone of .4
	if (deltaR(eta_P, phi_P, p.momentum().eta(), p.momentum().phi()) >= iso_dR) continue;
	
	// check if it's in the 5x7 central core
	if (fabs(eta_P-p.momentum().eta()) < cluster_eta_width*0.5 && 
	    fabs(phi_P-p.momentum().phi()) < cluster_phi_width*0.5) continue;
	
	mom_in_EtCone += p.momentum();
      }
      MYDEBUG << "...Done with initial EtCone." << std::endl;
      
      // now figure out the correction (area*density)
      float EtCone_area = PI*iso_dR*iso_dR - cluster_eta_width*cluster_phi_width;
      float correction = _ptDensity[getEtaBin(eta_P,2)]*EtCone_area;
      MYDEBUG << "...Done with jet-area correction." << std::endl;

      // require photon to be isolated
      if(mom_in_EtCone.Et()-correction > 4.0*GeV){ 
	MYDEBUG << "...Going to veto event(6)." << std::endl;
        vetoEvent;
      }
      MYDEBUG << "...Didn't veto event(6)." << std::endl;


      // 
      int photon_jet_sign = sign( leadingJet.rapidity() * photon.rapidity() );

      // Fill histos
      float abs_jet_rapidity = fabs(leadingJet.rapidity());
      float photon_pt = photon.pT()/GeV;
      float abs_photon_eta = fabs(photon.eta());

      if (abs_photon_eta<1.37) {

	if (abs_jet_rapidity < 1.2) {

	  if (photon_jet_sign >= 1) {
	    _h_phbarrel_jetcentral_SS->fill(photon_pt, weight);
	  } else {
	    _h_phbarrel_jetcentral_OS->fill(photon_pt, weight);
	  }

	} else if (abs_jet_rapidity < 2.8) {

	  if (photon_jet_sign >= 1) {
	    _h_phbarrel_jetmedium_SS->fill(photon_pt, weight);
	  } else {
	    _h_phbarrel_jetmedium_OS->fill(photon_pt, weight);
	  }

	} else if (abs_jet_rapidity < 4.4) {

	  if (photon_jet_sign >= 1) {
	    _h_phbarrel_jetforward_SS->fill(photon_pt, weight);
	  } else {
	    _h_phbarrel_jetforward_OS->fill(photon_pt, weight);
	  }
	}

      }

      MYDEBUG << "...Done with analyze." << std::endl;
    }


    /// Normalise histograms etc., after the run
    void finalize() {

      MYDEBUG << "In finalize." << std::endl;

      scale(_h_phbarrel_jetcentral_SS, crossSection()/sumOfWeights());
      scale(_h_phbarrel_jetcentral_OS, crossSection()/sumOfWeights());
      scale(_h_phbarrel_jetmedium_SS, crossSection()/sumOfWeights());
      scale(_h_phbarrel_jetmedium_OS, crossSection()/sumOfWeights());
      scale(_h_phbarrel_jetforward_SS, crossSection()/sumOfWeights());
      scale(_h_phbarrel_jetforward_OS, crossSection()/sumOfWeights());

      MYDEBUG << "Done with finalize." << std::endl;
    }

  private:

    AIDA::IHistogram1D* _h_phbarrel_jetcentral_SS;
    AIDA::IHistogram1D* _h_phbarrel_jetmedium_SS;
    AIDA::IHistogram1D* _h_phbarrel_jetforward_SS;

    AIDA::IHistogram1D* _h_phbarrel_jetcentral_OS;
    AIDA::IHistogram1D* _h_phbarrel_jetmedium_OS;
    AIDA::IHistogram1D* _h_phbarrel_jetforward_OS;

    fastjet::AreaDefinition* _area_def;

    std::vector<float> _eta_bins_ph;
    std::vector<float> _eta_bins_jet;
    std::vector<float> _eta_bins_areaoffset;

    std::vector<float> _ptDensity;
    std::vector<float> _sigma;
    std::vector<float> _Njets;
  };



  // This global object acts as a hook for the plugin system
  AnalysisBuilder<ATLAS_2012_I1093738> plugin_ATLAS_2012_I1093738;
}