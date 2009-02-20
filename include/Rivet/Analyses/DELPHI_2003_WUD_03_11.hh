// -*- C++ -*-
#ifndef RIVET_DELPHI_2003_WUD_03_11_HH
#define RIVET_DELPHI_2003_WUD_03_11_HH

#include "Rivet/Analysis.hh"
#include "Rivet/Projections/FastJets.hh"
#include "Rivet/Projections/FinalState.hh"
#include "Rivet/Projections/ChargedFinalState.hh"

namespace Rivet {

  /**
   * @brief DELPHI 4-jet angular distributions
   * @author Hendrik Hoeth
   *
   * This is Hendrik Hoeth's Diploma thesis, measuring the 4-jet angular
   * distributions at LEP-1.
   *
   *
   * @par Run conditions
   *
   * @arg LEP1 beam energy: \f$ \sqrt{s} = \$f 91.2 GeV
   * @arg Run with generic QCD events.
   * @arg No \f$ p_\perp^\text{min} \f$ cutoff is required
   */
  class DELPHI_2003_WUD_03_11 : public Analysis {

  public:

    /// @name Constructors etc.
    //@{

    /// Constructor
    DELPHI_2003_WUD_03_11() 
    {
      const ChargedFinalState cfs;
      addProjection(cfs, "FS");
      #ifdef HAVE_JADE
      addProjection(FastJets(cfs, FastJets::JADE, 0.7), "JadeJets");
      addProjection(FastJets(cfs, FastJets::DURHAM, 0.7), "DurhamJets");
      #endif
      _numdurjets = 0;
      _numjadejets = 0;
    }


    /// Factory method.
    static Analysis* create() { 
      return new DELPHI_2003_WUD_03_11(); 
    }

    //@}


    /// @name Publication metadata
    //@{
    
    /// Analysis name
    string name() const {
      return "DELPHI_2003_WUD_03_11";
    }
    /// SPIRES key (IRN)
    string spiresId() const {
      return "NONE";
    }
    /// A short description of the analysis.
    string summary() const {
      return "4-jet angular distributions at LEP";
    }
    /// A full description of the analysis.
    string description() const {
      ostringstream os;
      os << "The 4-jet angular distributions (Bengtsson-Zerwas,"
         << "K\"orner-Schierholz-Willrodt, Nachtmann-Reiter, and "
         << "$\\alpha_{34}$) have been measured with DELPHI at "
	 << "LEP 1 using Jade and Durham cluster algorithms. ";
      return os.str();
    }
    // Experiment which performed and published this analysis.
    string experiment() const {
     return "DELPHI";
    }
    // Collider on which the experiment was based.
    string collider() const {
     return "LEP 1";
    }
    /// When published.
    string year() const {
     return "2003 (note)";
    }
    /// Names & emails of paper/analysis authors.
    vector<string> authors() const {
      vector<string> ret;
      ret += "Hendrik Hoeth <hendrik.hoeth@cern.ch>";
      return ret;
    }
    /// Information about the events needed as input for this analysis.
    string runInfo() const {
      ostringstream os;
      os << "Hadronic Z decay events generated on the Z pole (sqrt(s) = 91.2 GeV)";
      return os.str();
    }
    string status() const {
      return "UNVALIDATED";
    }
    /// Publication reference
    vector<string> references() const {
      vector<string> ret;
      ret += "Diploma thesis WUD-03-11, University of Wuppertal";
      return ret;
    }

    //@}


    /// @name Analysis methods
    //@{
    virtual void init();
    virtual void analyze(const Event& event);
    virtual void finalize();
    double calc_BZ(std::vector<fastjet::PseudoJet> jets);
    double calc_KSW(std::vector<fastjet::PseudoJet> jets);
    double calc_NR(std::vector<fastjet::PseudoJet> jets);
    double calc_ALPHA34(std::vector<fastjet::PseudoJet> jets);
    //@}


  private:

    int _numdurjets;
    int _numjadejets;

    /// @name Histograms
    //@{
    AIDA::IHistogram1D *_histDurhamBZ;
    AIDA::IHistogram1D *_histDurhamKSW;
    AIDA::IHistogram1D *_histDurhamNR;
    AIDA::IHistogram1D *_histDurhamALPHA34;
    AIDA::IHistogram1D *_histJadeBZ;
    AIDA::IHistogram1D *_histJadeKSW;
    AIDA::IHistogram1D *_histJadeNR;
    AIDA::IHistogram1D *_histJadeALPHA34;
    //@}

  };

}

#endif
