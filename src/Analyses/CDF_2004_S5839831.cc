// -*- C++ -*-
// Underlying event analysis at CDF.

#include "Rivet/Rivet.hh"
#include "Rivet/Tools/Logging.hh"
#include "Rivet/Analyses/CDF_2004_S5839831.hh"
#include "Rivet/RivetAIDA.hh"

namespace Rivet {


  // Book histograms
  void CDF_2004_S5839831::init() {
    _pt90Max1800 = bookProfile1D(2, 1, 1, "pTmax vs ET at sqrt{s} = 1800 GeV");
    _pt90Min1800 = bookProfile1D(2, 1, 2, "pTmin vs ET at sqrt{s} = 1800 GeV");
    _pt90Diff1800 = bookProfile1D(2, 1, 3, "pTdiff vs ET at sqrt{s} = 1800 GeV");
    _pt90Dbn1800Et40 = bookHistogram1D(3, 1, 1, "pT distribution in MAX+MIN transverse cones for 40 < ET < 80 GeV at sqrt{s} = 1800 GeV");
    _pt90Dbn1800Et80 = bookHistogram1D(3, 1, 2, "pT distribution in MAX+MIN transverse cones for 80 < ET < 120 GeV at sqrt{s} = 1800 GeV");
    _pt90Dbn1800Et120 = bookHistogram1D(3, 1, 3, "pT distribution in MAX+MIN transverse cones for 120 < ET < 160 GeV at sqrt{s} = 1800 GeV");
    _pt90Dbn1800Et160 = bookHistogram1D(3, 1, 4, "pT distribution in MAX+MIN transverse cones for 160 < ET < 200 GeV at sqrt{s} = 1800 GeV");
    _pt90Dbn1800Et200 = bookHistogram1D(3, 1, 5, "pT distribution in MAX+MIN transverse cones for 200 < ET < 270 GeV at sqrt{s} = 1800 GeV");
    _num90Max1800 = bookProfile1D(4, 1, 1, "Nmax vs ET at sqrt{s} = 1800 GeV");
    _num90Min1800 = bookProfile1D(4, 1, 2, "Nmin vs ET at sqrt{s} = 1800 GeV");    
    _numTracksDbn1800 = bookHistogram1D(5, 1, 1, "Track multiplicity distribution at sqrt{s} = 1800 GeV");
    _ptDbn1800 = bookHistogram1D(6, 1, 1, "pT distribution at sqrt{s} = 1800 GeV");
    _pTSum1800_2Jet = bookProfile1D(7, 1, 1, "pTsum vs ET (for removal of 2 jets) at sqrt{s} = 1800 GeV");
    _pTSum1800_3Jet = bookProfile1D(7, 1, 2, "pTsum vs ET (for removal of 3 jets) at sqrt{s} = 1800 GeV");            
    _pt90Max630 = bookProfile1D(8, 1, 1, "pTmax vs ET at sqrt{s} = 630 GeV");
    _pt90Min630 = bookProfile1D(8, 1, 2, "pTmin vs ET at sqrt{s} = 630 GeV");
    /// @todo Problem with HepData indexes?
    //_pt90Diff630 = bookProfile1D(8, 1, 3, "pTdiff vs ET at sqrt{s} = 630 GeV");
    _pTSum630_2Jet = bookProfile1D(9, 1, 1, "pTsum vs ET (for removal of 2 jets) at sqrt{s} = 630 GeV");
    _pTSum630_3Jet = bookProfile1D(9, 1, 2, "pTsum vs ET (for removal of 3 jets) at sqrt{s} = 630 GeV");
  }


  bool cmpJetsByEt(const Jet& a, const Jet& b) {
    /// @todo Use Et instead... definition?
    return a.getPtSum() < b.getPtSum();
  }


  // Do the analysis
  void CDF_2004_S5839831::analyze(const Event& event) {
    const double sqrtS = applyProjection<Beam>(event, "Beam").getSqrtS();
    const ParticleVector tracks = applyProjection<FinalState>(event, "FS").particles();
    if (tracks.empty()) vetoEvent(event);
    /// @todo Make FastJets handle "no particles" events nicely
    const FastJets& jetproj = applyProjection<FastJets>(event, "Jets");
    vector<Jet> jets = jetproj.getJets();
    if (jets.empty()) vetoEvent(event);
    sort(jets.begin(), jets.end(), cmpJetsByEt);

    /// @todo Ensure there is only one well-defined primary vertex, i.e. no pileup. 
    /// Should be automatic as long as generator is run sensibly.

    /// @todo Need to implement track pT > 0.4 GeV/c and within 5.0/0.5cm of pp vertex (nominal?)

    // NB. Charged track reconstruction efficiency has already been corrected.

    // Leading jet must be in central |eta| < 0.5 region.
    /// @todo Sure it isn't |eta| < 1.0?
    const Jet leadingjet = jets.front();
    const double etaLead = leadingjet.vector().pseudorapidity();
    if (fabs(etaLead) > 0.5) vetoEvent(event);
    
    // Get Et of the leading jet: used to bin histograms.
    const double ETlead = leadingjet.getEtSum();

    // Get azimuthal angle of leading jet, to determine transverse regions
    const double phiLead = leadingjet.vector().azimuthalAngle();
    const double phiTransPlus = mapAngleMPiToPi(phiLead + PI/2.0);
    const double phiTransMinus = mapAngleMPiToPi(phiLead - PI/2.0);

    // Get the event weight
    const double weight = event.weight();

    // Fill total track multiplicity histo
    if (fuzzyEquals(sqrtS/GeV, 1800)) {
      _numTracksDbn1800->fill(tracks.size(), weight);
    }

    // Run over all charged tracks
    double numPlus(0), numMinus(0);
    double ptPlus(0), ptMinus(0);
    double ptSumSub2(0), ptSumSub3(0);
    for (ParticleVector::const_iterator t = tracks.begin(); t != tracks.end(); ++t) {
      FourMomentum trackMom = t->getMomentum();
      const double pt = trackMom.pT();

      // Plot total pT distribution for min bias at sqrt(s) = 1800 GeV
      if (fuzzyEquals(sqrtS/GeV, 1800)) {
        _ptDbn1800->fill(pt, weight);
      }

      // Find if track mom is in either transverse cone
      if (deltaR(trackMom, etaLead, phiTransPlus) < 0.7) {
        ptPlus += pt;
        numPlus += 1;
      } else if (deltaR(trackMom, etaLead, phiTransMinus) < 0.7) {
        ptMinus += pt;
        numMinus += 1;
      }

      // Construct "Swiss Cheese" pT distributions, with pT contributions from
      // tracks within R = 0.7 of the 1st, 2nd (and 3rd) jets being ignored.
      if (ETlead/GeV > 5.0 &&
          jets.size() > 1 && 
          mod(jets[1].vector())/GeV > 5.0) {
        const double eta2 = jets[1].vector().pseudorapidity();
        const double phi2 = jets[1].vector().azimuthalAngle();
        if (deltaR(trackMom, etaLead, phiLead) > 0.7 &&
            deltaR(trackMom, eta2, phi2) > 0.7) {
          ptSumSub2 += pt;
          if (jets.size() > 2 &&
              mod(jets[2].vector())/GeV > 5.0) {
            const double eta3 = jets[2].vector().pseudorapidity();
            const double phi3 = jets[2].vector().azimuthalAngle();
            if (deltaR(trackMom, eta3, phi3) > 0.7) {
              ptSumSub3 += pt;
            }
          }
        }
      }
    }
    // Assign pT_{min,max} from pT_{plus,minus}
    const double ptMin = min(ptPlus, ptMinus);
    const double ptMax = max(ptPlus, ptMinus);
    const double ptDiff = fabs(ptMax - ptMin);

    /// @todo Filling histos from different kinematic cut regions.

    // Multiplicity, pT and Swiss Cheese distributions for 
    // sqrt(s) = 630 GeV, 1800 GeV
    if (fuzzyEquals(sqrtS/GeV, 630)) {
      _pt90Max630->fill(ETlead/GeV, ptMax/GeV, weight);
      _pt90Min630->fill(ETlead/GeV, ptMin/GeV, weight);
      /// @todo Reinstate when HepData contains data for this histo
      //_pt90Diff630->fill(ETlead/GeV, ptDiff/GeV, weight);
      // Swiss Cheese sub 2,3 jets
      _pTSum630_2Jet->fill(ETlead/GeV, ptSumSub2/GeV, weight);
      _pTSum630_3Jet->fill(ETlead/GeV, ptSumSub3/GeV, weight);
    } else if (fuzzyEquals(sqrtS/GeV, 1800)) {
      const unsigned int numMax = (ptPlus >= ptMinus) ? numPlus : numMinus;
      const unsigned int numMin = (ptPlus >= ptMinus) ? numMinus : numPlus;
      _num90Max1800->fill(ETlead/GeV, numMax, weight);
      _num90Min1800->fill(ETlead/GeV, numMin, weight);
      _pt90Max1800->fill(ETlead/GeV, ptMax/GeV, weight);
      _pt90Min1800->fill(ETlead/GeV, ptMin/GeV, weight);
      _pt90Diff1800->fill(ETlead/GeV, ptDiff/GeV, weight);
      // Swiss Cheese sub 2,3 jet: *_pTSum1800_2Jet, *_pTSum1800_3Jet;
      _pTSum1800_2Jet->fill(ETlead/GeV, ptSumSub2/GeV, weight);
      _pTSum1800_3Jet->fill(ETlead/GeV, ptSumSub3/GeV, weight);
      // pT distributions
      const double ptTransTotal = ptMax + ptMin;
      if (inRange(ETlead/GeV, 40, 80)) {
        _pt90Dbn1800Et40->fill(ptTransTotal/GeV, weight);
      } else if (inRange(ETlead/GeV, 80, 120)) {
        _pt90Dbn1800Et80->fill(ptTransTotal/GeV, weight);
      } else if (inRange(ETlead/GeV, 120, 160)) {
        _pt90Dbn1800Et120->fill(ptTransTotal/GeV, weight);
      } else if (inRange(ETlead/GeV, 160, 200)) {
        _pt90Dbn1800Et160->fill(ptTransTotal/GeV, weight);
      } else if (inRange(ETlead/GeV, 200, 270)) {
        _pt90Dbn1800Et200->fill(ptTransTotal/GeV, weight);
      }
    }
      
  }

  
  void CDF_2004_S5839831::finalize() { 
    // Normalize to actual number of entries in data histos
    normalize(_pt90Dbn1800Et40,  1656.75);
    normalize(_pt90Dbn1800Et80,  4657.5);
    normalize(_pt90Dbn1800Et120, 5395.5);
    normalize(_pt90Dbn1800Et160, 7248.75);
    normalize(_pt90Dbn1800Et200, 2442.0);
  }
