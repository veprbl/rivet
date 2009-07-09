// -*- C++ -*-
#include "Rivet/Analyses/MC_TVT1960_ZJETS.hh"
#include "Rivet/Tools/Logging.hh"
#include "Rivet/Projections/ZFinder.hh"
#include "Rivet/Projections/FastJets.hh"
#include "Rivet/RivetAIDA.hh"

namespace Rivet {


  MC_TVT1960_ZJETS::MC_TVT1960_ZJETS()
    : Analysis("MC_TVT1960_ZJETS")
  {
    setBeams(PROTON, ANTIPROTON);
    
    ZFinder zfinder(-2.5, 2.5, 25.0*GeV, ELECTRON, 65.0*GeV, 115.0*GeV, 0.2);
    addProjection(zfinder, "ZFinder");
    FastJets jetpro(zfinder.remainingFinalState(), FastJets::KT, 0.7);
    addProjection(jetpro, "Jets");
  } 



  // Book histograms
  void MC_TVT1960_ZJETS::init() {
    _h_Z_mass = bookHistogram1D
      ("Z_mass", "Z mass", "$m_{\\text{Z}}$ [GeV]",
       "$1/\\sigma \\text{d}\\sigma/\\text{d}m_{\\text{Z}}$", 50, 66.0, 116.0);
    _h_jet1_pT = bookHistogram1D
      ("jet1_pT", "pT of 1st jet", "$p_{\\perp}^{\\text{1st jet}}$ [GeV]",
       "$1/\\sigma \\text{d}\\sigma/\\text{d}p_{\\perp}^{\\text{1st jet}}$", 50, 0.0, 500.0);
    _h_jet2_pT = bookHistogram1D
      ("jet2_pT", "pT of 2nd jet", "$p_{\\perp}^{\\text{2nd jet}}$ [GeV]",
       "$1/\\sigma \\text{d}\\sigma/\\text{d}p_{\\perp}^{\\text{2nd jet}}$", 30, 0.0, 300.0);
    _h_jet3_pT = bookHistogram1D
      ("jet3_pT", "pT of 3rd jet", "$p_{\\perp}^{\\text{3rd jet}}$ [GeV]",
       "$1/\\sigma \\text{d}\\sigma/\\text{d}p_{\\perp}^{\\text{3rd jet}}$", 20, 0.0, 200.0);
    _h_jet4_pT = bookHistogram1D
      ("jet4_pT", "pT of 4th jet", "$p_{\\perp}^{\\text{4th jet}}$ [GeV]",
       "$1/\\sigma \\text{d}\\sigma/\\text{d}p_{\\perp}^{\\text{4th jet}}$", 10, 0.0, 100.0);
    _h_jet20_multi_exclusive = bookHistogram1D
      ("jet20_multi_exclusive", "Exclusive jet multiplicity", "$N_{\\text{jet(}\\geq 20 \\text{GeV)}}$",
       "$\\sigma(N_{\\text{jet}})/\\sigma(N_{\\text{jet}}=0)$", 10, -0.5, 9.5);
    _h_jet20_multi_inclusive = bookHistogram1D
      ("jet20_multi_inclusive", "Inclusive jet multiplicity", "$N_{\\text{jet(}\\geq 20 \\text{GeV)}}$",
       "$\\sigma(\\geq N_{\\text{jet}})/\\sigma(\\text{inclusive})$", 10, -0.5, 9.5);
    _h_jet20_multi_ratio = bookDataPointSet
      ("jet20_multi_ratio", "Ratio of jet multiplicity", "$N_{\\text{jet(}\\geq 20 \\text{GeV)}}$",
       "$\\sigma(\\geq N_{\\text{jet}})/\\sigma(\\geq N_{\\text{jet}}-1)$", 9, 0.5, 9.5);
    _h_jet10_multi_exclusive = bookHistogram1D
      ("jet10_multi_exclusive", "Exclusive jet multiplicity", "$N_{\\text{jet(}\\geq 10 \\text{GeV)}}$",
       "$\\sigma(N_{\\text{jet}})/\\sigma(N_{\\text{jet}}=0)$", 10, -0.5, 9.5);
    _h_jet10_multi_inclusive = bookHistogram1D
      ("jet10_multi_inclusive", "Inclusive jet multiplicity", "$N_{\\text{jet(}\\geq 10 \\text{GeV)}}$",
       "$\\sigma(\\geq N_{\\text{jet}})/\\sigma(\\text{inclusive})$", 10, -0.5, 9.5);
    _h_jet10_multi_ratio = bookDataPointSet
      ("jet10_multi_ratio", "Ratio of jet multiplicity", "$N_{\\text{jet(}\\geq 10 \\text{GeV)}}$",
       "$\\sigma(\\geq N_{\\text{jet}})/\\sigma(\\geq N_{\\text{jet}}-1)$", 9, 0.5, 9.5);
    _h_deta_Z_jet1 = bookHistogram1D
      ("deta_Z_jet2", "", "$|\\Delta{\\eta}(\\text{Z, 1st jet})|$",
       "$1/\\sigma \\text{d}\\sigma/\\text{d}|\\Delta{\\eta}(\\text{Z, 1st jet})|$", 20, 0.0, 5.0);
    _h_dR_jet2_jet3 = bookHistogram1D
      ("dR_jet2_jet3", "", "$|\\Delta{R}(\\text{2nd jet, 3rd jet})|$",
       "$1/\\sigma \\text{d}\\sigma/\\text{d}|\\Delta{R}(\\text{2nd jet, 3rd jet})|$", 20, 0.0, 5.0);
    for (size_t i=0; i<4; ++i) {
      stringstream name, title, xtitle, ytitle;
      name<<"log10_d_"<<i<<i+1;
      title<<"$\\log_{10}$($k_\\perp$ jet resolution $"<<i<<" \\to "<<i+1<<"$ [GeV])";
      xtitle<<"$\\log_{10}(d_{"<<i<<i+1<<"}/\\text{GeV})$";
      ytitle<<"$\\text{d}\\sigma/\\text{d}\\log_{10}(d_{"<<i<<i+1<<"})$";
      _h_log10_d[i] = bookHistogram1D(name.str(), title.str(), xtitle.str(), ytitle.str(), 50, 0.2, 2.6);
    }
    for (size_t i=0; i<5; ++i) {
      stringstream name, title, xtitle, ytitle;
      name<<"log10_R_"<<i;
      title<<"$\\log_{10}$(Integrated $"<<i<<"$ jet rate in $k_\\perp$ [GeV])";
      xtitle<<"$\\log_{10}(d_{\\text{cut}}/\\text{GeV})$";
      if (i==4) ytitle<<"$R_{\\geq"<<i<<"}$";
      else ytitle<<"$R_{"<<i<<"}$";
      _h_log10_R[i] = bookDataPointSet(name.str(), title.str(), xtitle.str(), ytitle.str(), 50, 0.2, 2.6);
    }
  }



  // Do the analysis 
  void MC_TVT1960_ZJETS::analyze(const Event & e) {
    double weight = e.weight();
    
    const ZFinder& zfinder = applyProjection<ZFinder>(e, "ZFinder");
    if (zfinder.particles().size()!=1) {
      vetoEvent;
    }
    const FastJets& jetpro = applyProjection<FastJets>(e, "Jets");

    // jet resolutions and integrated jet rates
    const fastjet::ClusterSequence* seq = jetpro.clusterSeq();
    if (seq!=NULL) {
      double d_01=log10(sqrt(seq->exclusive_dmerge_max(0)));
      double d_12=log10(sqrt(seq->exclusive_dmerge_max(1)));
      double d_23=log10(sqrt(seq->exclusive_dmerge_max(2)));
      double d_34=log10(sqrt(seq->exclusive_dmerge_max(3)));

      _h_log10_d[0]->fill(d_01, weight);
      _h_log10_d[1]->fill(d_12, weight);
      _h_log10_d[2]->fill(d_23, weight);
      _h_log10_d[3]->fill(d_34, weight);

      /// @todo: can't this be calculated in the finalize method?
      for (int i=0; i<_h_log10_R[0]->size(); ++i) {
        IDataPoint* dp=_h_log10_R[0]->point(i);
        if (d_01 < dp->coordinate(0)->value()) {
          dp->coordinate(1)->setValue(dp->coordinate(1)->value()+weight);
        }
      }
      for (int i=0; i<_h_log10_R[1]->size(); ++i) {
        IDataPoint* dp=_h_log10_R[1]->point(i);
        double dcut=dp->coordinate(0)->value();
        if (d_12<dcut && d_01>dcut) {
          dp->coordinate(1)->setValue(dp->coordinate(1)->value()+weight);
        }
      }
      for (int i=0; i<_h_log10_R[2]->size(); ++i) {
        IDataPoint* dp=_h_log10_R[2]->point(i);
        double dcut=dp->coordinate(0)->value();
        if (d_23<dcut && d_12>dcut) {
          dp->coordinate(1)->setValue(dp->coordinate(1)->value()+weight);
        }
      }
      for (int i=0; i<_h_log10_R[3]->size(); ++i) {
        IDataPoint* dp=_h_log10_R[3]->point(i);
        double dcut=dp->coordinate(0)->value();
        if (d_34<dcut && d_23>dcut) {
          dp->coordinate(1)->setValue(dp->coordinate(1)->value()+weight);
        }
      }
      for (int i=0; i<_h_log10_R[4]->size(); ++i) {
        IDataPoint* dp=_h_log10_R[4]->point(i);
        double dcut=dp->coordinate(0)->value();
        if (d_34>dcut) {
          dp->coordinate(1)->setValue(dp->coordinate(1)->value()+weight);
        }
      }
    }

    const Jets& jets = jetpro.jetsByPt(20.0*GeV);
    Jets jets_cut;
    foreach (const Jet& j, jets) {
      if (fabs(j.momentum().pseudorapidity()) < 2.0) {
        jets_cut.push_back(j);
      }
    }
    
    // fill jet multi
    _h_jet20_multi_exclusive->fill(jets_cut.size(), weight);
    _h_jet20_multi_inclusive->fill(0, weight);

    FourMomentum zmom(zfinder.particles()[0].momentum());
    _h_Z_mass->fill(zmom.mass(),weight);
    if (jets_cut.size()>0) {
      _h_jet1_pT->fill(jets_cut[0].momentum().pT(), weight);
      double deta=fabs(zmom.pseudorapidity()-jets_cut[0].momentum().pseudorapidity());
      _h_deta_Z_jet1->fill(deta, weight);
      _h_jet20_multi_inclusive->fill(1, weight);
    }
    if (jets_cut.size()>1) {
      _h_jet2_pT->fill(jets_cut[1].momentum().pT(), weight);
      _h_jet20_multi_inclusive->fill(2, weight);
    }
    if (jets_cut.size()>2) {
      _h_jet3_pT->fill(jets_cut[2].momentum().pT(), weight);
      double dR23=deltaR(jets_cut[1].momentum().pseudorapidity(), jets_cut[1].momentum().azimuthalAngle(),
                         jets_cut[2].momentum().pseudorapidity(), jets_cut[2].momentum().azimuthalAngle());
      _h_dR_jet2_jet3->fill(dR23, weight);
      _h_jet20_multi_inclusive->fill(3, weight);
    }
    if (jets_cut.size()>3) {
      _h_jet4_pT->fill(jets_cut[3].momentum().pT(), weight);
      _h_jet20_multi_inclusive->fill(4, weight);
    }
    if (jets_cut.size()>4) {
      _h_jet20_multi_inclusive->fill(5, weight);
    }
    if (jets_cut.size()>5) {
      _h_jet20_multi_inclusive->fill(6, weight);
    }
    if (jets_cut.size()>6) {
      _h_jet20_multi_inclusive->fill(7, weight);
    }
    if (jets_cut.size()>7) {
      _h_jet20_multi_inclusive->fill(8, weight);
    }
    if (jets_cut.size()>8) {
      _h_jet20_multi_inclusive->fill(9, weight);
    }
    if (jets_cut.size()>9) {
      _h_jet20_multi_inclusive->fill(10, weight); // for overflow
    }

    // do the multis also for jets > 10 GeV
    const Jets& jets10 = jetpro.jetsByPt(10.0*GeV);
    Jets jets10_cut;
    foreach (const Jet& j, jets10) {
      if (fabs(j.momentum().pseudorapidity()) < 2.0) {
        jets10_cut.push_back(j);
      }
    }

    // fill jet multi
    _h_jet10_multi_exclusive->fill(jets10_cut.size(), weight);
    _h_jet10_multi_inclusive->fill(0, weight);

    if (jets10_cut.size()>0) {
      _h_jet10_multi_inclusive->fill(1, weight);
    }
    if (jets10_cut.size()>1) {
      _h_jet10_multi_inclusive->fill(2, weight);
    }
    if (jets10_cut.size()>2) {
      _h_jet10_multi_inclusive->fill(3, weight);
    }
    if (jets10_cut.size()>3) {
      _h_jet10_multi_inclusive->fill(4, weight);
    }
    if (jets10_cut.size()>4) {
      _h_jet10_multi_inclusive->fill(5, weight);
    }
    if (jets10_cut.size()>5) {
      _h_jet10_multi_inclusive->fill(6, weight);
    }
    if (jets10_cut.size()>6) {
      _h_jet10_multi_inclusive->fill(7, weight);
    }
    if (jets10_cut.size()>7) {
      _h_jet10_multi_inclusive->fill(8, weight);
    }
    if (jets10_cut.size()>8) {
      _h_jet10_multi_inclusive->fill(9, weight);
    }
    if (jets10_cut.size()>9) {
      _h_jet10_multi_inclusive->fill(10, weight);
    }
  }


  // Finalize
  void MC_TVT1960_ZJETS::finalize() {
    normalize(_h_Z_mass,1.0);
    normalize(_h_jet1_pT,1.0);
    normalize(_h_jet2_pT,1.0);
    normalize(_h_jet3_pT,1.0);
    normalize(_h_jet4_pT,1.0);

    normalize(_h_deta_Z_jet1,1.0);
    normalize(_h_dR_jet2_jet3,1.0);
    for (size_t i=0; i<4; ++i) {
      scale(_h_log10_d[i],1.0/sumOfWeights());
    }
    for (size_t n=0; n<5; ++n) {
      /// scale integrated jet rates to 1
      for (int i=0; i<_h_log10_R[n]->size(); ++i) {
        IDataPoint* dp=_h_log10_R[n]->point(i);
        dp->coordinate(1)->setValue(dp->coordinate(1)->value()*1.0/sumOfWeights());
      }
    }

    // fill inclusive jet multi ratio
    int Nbins=_h_jet20_multi_inclusive->axis().bins();
    std::vector<double> ratio(Nbins-1, 0.0);
    std::vector<double> err(Nbins-1, 0.0);
    for (int i=0; i<Nbins-1; ++i) {
      if (_h_jet20_multi_inclusive->binHeight(i)>0.0 && _h_jet20_multi_inclusive->binHeight(i+1)>0.0) {
        ratio[i]=_h_jet20_multi_inclusive->binHeight(i+1)/_h_jet20_multi_inclusive->binHeight(i);
        double relerr_i=_h_jet20_multi_inclusive->binError(i)/_h_jet20_multi_inclusive->binHeight(i);
        double relerr_j=_h_jet20_multi_inclusive->binError(i+1)/_h_jet20_multi_inclusive->binHeight(i+1);
        err[i]=ratio[i]*(relerr_i+relerr_j);
      }
    }
    _h_jet20_multi_ratio->setCoordinate(1, ratio, err);

    // fill inclusive jet10 multi ratio
    for (int i=0; i<Nbins-1; ++i) {
      if (_h_jet10_multi_inclusive->binHeight(i)>0.0 && _h_jet10_multi_inclusive->binHeight(i+1)>0.0) {
        ratio[i]=_h_jet10_multi_inclusive->binHeight(i+1)/_h_jet10_multi_inclusive->binHeight(i);
        double relerr_i=_h_jet10_multi_inclusive->binError(i)/_h_jet10_multi_inclusive->binHeight(i);
        double relerr_j=_h_jet10_multi_inclusive->binError(i+1)/_h_jet10_multi_inclusive->binHeight(i+1);
        err[i]=ratio[i]*(relerr_i+relerr_j);
      }
    }
    _h_jet10_multi_ratio->setCoordinate(1, ratio, err);

    // scale exclusive and inclusive jet_multi to first bin = 1.0
    scale(_h_jet20_multi_exclusive, 1.0/_h_jet20_multi_exclusive->binHeight(0));
    scale(_h_jet20_multi_inclusive, 1.0/_h_jet20_multi_inclusive->binHeight(0));
    scale(_h_jet10_multi_exclusive, 1.0/_h_jet10_multi_exclusive->binHeight(0));
    scale(_h_jet10_multi_inclusive, 1.0/_h_jet10_multi_inclusive->binHeight(0));
  }

}
