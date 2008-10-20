// -*- C++ -*-
#ifndef RIVET_D0_2001_S4674421_HH
#define RIVET_D0_2001_S4674421_HH

#include "Rivet/Analysis.hh"
#include "Rivet/Projections/D0ILConeJets.hh"
#include "Rivet/Projections/PVertex.hh"
#include "Rivet/Projections/LeadingParticlesFinalState.hh"
#include "Rivet/Projections/InvMassFinalState.hh"
#include "Rivet/Projections/VetoedFinalState.hh"

namespace Rivet {  


  /// @brief D0 Run I differential W/Z boson cross-section analysis
  /// @author Lars Sonnenschein
  class D0_2001_S4674421 : public Analysis {

  public:

    /// @name Constructors etc.
    //@{

    /// Constructor.
    ///  - @c _mwmz = ratio of \f$ mW/mZ \f$ used in the publication analysis
    ///  - @c _brwenu = ratio of \f$ BR(W->e,nu) \f$ used in the publication analysis
    ///  - @c _brzee = ratio of \f$ BR(Z->ee) \f$ used in the publication analysis
    ///  - @c _mZmin = lower Z mass cut used in the publication analysis
    ///  - @c _mZmax = upper Z mass cut used in the publication analysis

    D0_2001_S4674421()
      : _mwmz(0.8820), _brwenu(0.1073), _brzee(0.033632), _mZmin(75.*GeV), _mZmax(105.*GeV)
    { 

      //cout << "D0_2001_Sbla constructor entered!" << endl;

      setBeams(PROTON, ANTIPROTON);
      setNeedsCrossSection(true);
      //const FinalState fs(-3.0, 3.0); 
      FinalState fs(-5.0, 5.0); //corrected for detector acceptance
      addProjection(fs, "FS");

      LeadingParticlesFinalState eeFS(fs, -2.5, 2.5, 0.); //20.);
      eeFS.addParticleId(11).addParticleId(-11); //Z -> e- e+
      addProjection(eeFS, "eeFS");
      
      LeadingParticlesFinalState enuFS(fs, -2.5, 2.5, 0.); //25.);
      enuFS.addParticleId(11).addParticleId(-12); //W- -> e- nub
      addProjection(enuFS, "enuFS");
      
      LeadingParticlesFinalState enubFS(fs, -2.5, 2.5, 0.); //25.);
      enubFS.addParticleId(-11).addParticleId(12); //W+ -> e+ nu
      addProjection(enubFS, "enubFS");

      InvMassFinalState eFromZ(eeFS, make_pair(11, -11), 0., 900., -5.0, 5.0, 0.); //20.);
      addProjection(eFromZ,"eFromZ");

      InvMassFinalState enuFromW(enuFS, make_pair(11, -12), 0., 900., -5.0, 5.0, 0.); //25.);
      addProjection(eFromZ,"enuFromW");

      InvMassFinalState enubFromW(enubFS, make_pair(-11, 12), 0., 900., -5.0, 5.0, 0.); //25.);
      addProjection(eFromZ,"enubFromW");

      //remove neutrinos for isolation final state particles
      VetoedFinalState vfs(fs);
      vfs.addVetoPairId(12).addVetoPairId(14).addVetoPairId(16);
      addProjection(vfs, "VFS");

    }    
    
    
    /// Factory method
    static Analysis* create() { 
      return new D0_2001_S4674421(); 
    }
    //@}
    
    
    /// @name Publication metadata
    //@{
    /// Get a description of the analysis.
    string getSpiresId() const {
      return "4674421";
    }
    /// Get a description of the analysis.
    string getDescription() const {
      return "Run I differential W/Z boson cross-section analysis";
    }
    /// Experiment which performed and published this analysis.
    string getExpt() const {
      return "D0";
    }
    /// When published (preprint year according to SPIRES).
    string getYear() const {
      return "2001";
    }
    /// Journal, and preprint references.
    vector<string> getReferences() const {
      vector<string> ret;
      ret.push_back("arXiv:hep-ex/0107012");
      return ret;
    }
    //@}
    
    
    /// @name Analysis methods
    //@{
    void init();  
    void isol(const ParticleVector& pvec, const VetoedFinalState& vfs, vector<double>& isolfrac, double rad);
    void analyze(const Event& event);
    void finalize();
    //@}
    
  private:
    
    /// Analysis used ratio of mW/mZ 
    const double _mwmz;
    
    /// Ratio of \f$ BR(W->e,nu) \f$ used in the publication analysis
    const double _brwenu;
    
    /// Ratio of \f$ BR(Z->ee) \f$ used in the publication analysis
    const double _brzee;
    
    ///invariant mass cuts for Z boson candidate (75.<mZ<105.)
    const double _mZmin, _mZmax;


    // Event counters for cross section normalizations
    double _eventsFilledW;
    double _eventsFilledZ;
    
    //@{
    /// Histograms
    AIDA::IHistogram1D* _h_dsigdpt_w;
    AIDA::IHistogram1D* _h_dsigdpt_z;
    AIDA::IHistogram1D* _h_dsigdpt_scaled_z;
    AIDA::IHistogram1D* _h_temp;
    AIDA::IDataPointSet* _dset_dsigpt_wz_rat;
   //@}    

    /// Hide copy assignment operator.
    D0_2001_S4674421& operator=(const D0_2001_S4674421&);
  };

}

#endif
