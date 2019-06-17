// -*- C++ -*-
#include "Rivet/Analysis.hh"
#include "Rivet/Projections/FastJets.hh"
#include "Rivet/Projections/FinalState.hh"
#include "Rivet/Projections/PromptFinalState.hh"
#include "Rivet/Projections/DressedLeptons.hh"


namespace Rivet {

  /// @brief leptoquark search at 13 TeV 
  /// @note This base class contains a "mode" variable to specify lepton channel
  class ATLAS_2019_I1718132 : public Analysis {
    public:

      /// Constructor
      DEFAULT_RIVET_ANALYSIS_CTOR(ATLAS_2019_I1718132);
      //@}

      /// Book histograms and initialise projections before the run
      void init() {

        // default to widest cut, electrons and muons.
        _mode = 3;      
        if ( getOption("LMODE") == "ELEL" )  _mode = 1;
        if ( getOption("LMODE") == "MUMU" )  _mode = 2;
        if ( getOption("LMODE") == "ELMU" )  _mode = 3;

        _useST = false;
        if ( getOption("STMODE") == "STRICT" )     _useST = true;
        if ( getOption("STMODE") == "INCLUSIVE" )  _useST = false;

        // Lepton cuts
        Cut baseline_lep_cuts = Cuts::abseta < 2.5 && Cuts::pT >=  40*GeV;

        // All final state particles
        const FinalState fs;

        // Get photons to dress leptons
        FinalState photons(Cuts::abspid == PID::PHOTON);

        // Find and dress the electrons and muons
        PromptFinalState bare_leps(Cuts::abspid == PID::ELECTRON || Cuts::abspid == PID::MUON);
        DressedLeptons dressed_leps(photons, bare_leps, 0.1, baseline_lep_cuts, true);
        declare(dressed_leps, "leptons");

        //and finally the jets:
        FastJets jets(fs, FastJets::ANTIKT, 0.4, JetAlg::Muons::NONE);
        declare(jets, "jets");

        size_t offset = _mode + (_useST? 3 : 0);
        book(_h["JetPt_leading"],     1 + offset, 1, 1);
        book(_h["JetPt_subleading"],  7 + offset, 1, 1); 
        book(_h["minDeltaPhiJ0_L"],  13 + offset, 1, 1);
        book(_h["minDeltaPhiJ1_L"],  19 + offset, 1, 1);
        book(_h["DeltaEtaJJ"],       25 + offset, 1, 1);
        book(_h["DeltaPhiJJ"],       31 + offset, 1, 1);
        book(_h["DeltaPhiLL"],       37 + offset, 1, 1);
        book(_h["DiJetMass"],        43 + offset, 1, 1);
        book(_h["DiLepPt"],          49 + offset, 1, 1);
        book(_h["Ht"],               55 + offset, 1, 1);
        book(_h["St"],               61 + offset, 1, 1);

      }

      /// Perform the per-event analysis
      void analyze(const Event& event) {

        // Get the selected leptons:
        vector<DressedLepton> leptons = apply<DressedLeptons>(event, "leptons").dressedLeptons();

        // get the selected jets:
        Jets jets = apply<JetAlg>(event, "jets").jetsByPt(Cuts::pT > 60*GeV && Cuts::absrap < 2.5);

        // exclude jets which are actually electrons
        idiscardIfAny(jets, leptons, [](const Jet& jet, const DressedLepton& lep) { 
          return lep.abspid() == PID::ELECTRON and deltaR(jet, lep) < 0.2; 
        });

        // remove cases where muons are too close to a jet 
        if (_mode == 3) { // el-mu case
          idiscardIfAny(leptons, jets, [](const DressedLepton& lep, const Jet& jet) { 
            return lep.abspid() == PID::MUON and deltaR(jet, lep) < 0.4;
          });
        }

        // make sure we have the right number of jets
        if (jets.size() < 2)  vetoEvent;

        // make sure we have right number of leptons
        size_t requiredMuons = 0, requiredElecs = 0;
        if (_mode == 1)  requiredElecs = 2;
        if (_mode == 2)  requiredMuons = 2;
        if (_mode == 3)  requiredMuons = requiredElecs = 1;

        size_t nEl = count(leptons, [](const DressedLepton& lep) { return  lep.abspid() == PID::ELECTRON; });
        if (nEl != requiredElecs)   vetoEvent; 

        size_t nMu = count(leptons, [](const DressedLepton& lep) { return  lep.abspid() == PID::MUON; });
        if (nMu != requiredMuons)  vetoEvent;  

        // make sure leptons are in right order (should be OK byt better safe than sorry!)
        std::sort(leptons.begin(), leptons.end(), cmpMomByPt);

        // calculate all observables and store in dict
        const double jetpt_leading    = jets[0].pT()/GeV;
        const double jetpt_subleading = jets[1].pT()/GeV;
        const double mll              = (leptons[0].momentum() + leptons[1].momentum()).mass()/GeV;
        const double st               = (leptons[0].pT() + leptons[1].pT() + jets[0].pT() + jets[1].pT())/GeV;
        const double ht               = (jets[0].pT() + jets[1].pT())/GeV;
        const double dijetmass        = (jets[0].mom() + jets[1].mom()).mass()/GeV;
        const double dileppt          = (leptons[0].momentum() + leptons[1].momentum()).pT()/GeV;
        const double deltaphijj       = deltaPhi(jets[0], jets[1]);
        const double deltaetajj       = deltaEta(jets[0], jets[1]);
        const double deltaphill       = deltaPhi(leptons[0], leptons[1]);
        const double mindeltaphij0_l  = min(deltaPhi(jets[0], leptons[0]), deltaPhi(jets[0], leptons[1]));
        const double mindeltaphij1_l  = min(deltaPhi(jets[1], leptons[0]), deltaPhi(jets[1], leptons[1]));

        // add Z-mass window cut if needed
        bool addMllCut = _mode == 1 || _mode == 2;
        if (addMllCut && (mll < 70. || 110. < mll))  vetoEvent;

        // add "extreme" ST cut if needed
        if (_useST && st < 600.)  vetoEvent;

        // fill output histos
        _h["JetPt_leading"]->fill(jetpt_leading); 
        _h["JetPt_subleading"]->fill(jetpt_subleading); 
        _h["St"]->fill(st);
        _h["Ht"]->fill(ht);
        _h["DiJetMass"]->fill(dijetmass);
        _h["DiLepPt"]->fill(dileppt);
        _h["DeltaPhiJJ"]->fill(deltaphijj);
        _h["DeltaEtaJJ"]->fill(deltaetajj);
        _h["DeltaPhiLL"]->fill(deltaphill);
        _h["minDeltaPhiJ0_L"]->fill(mindeltaphij0_l);
        _h["minDeltaPhiJ1_L"]->fill(mindeltaphij1_l);
      }



      void finalize() {
        const double sf = crossSectionPerEvent();
        for (auto hist : _h) { scale(hist.second, sf); }
      }

      //@}


    protected:

      size_t _mode;
      bool _useST;


    private:

      map<string, Histo1DPtr> _h;

    };

  DECLARE_RIVET_PLUGIN(ATLAS_2019_I1718132);

}
