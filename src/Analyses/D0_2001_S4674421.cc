// -*- C++ -*-
#include "Rivet/Tools/Logging.hh"
#include "Rivet/RivetAIDA.hh"
#include "Rivet/Tools/ParticleIDMethods.hh"
#include "Rivet/Analyses/D0_2001_S4674421.hh" 

#include "AIDA/IDataPoint.h"

namespace Rivet {


  void D0_2001_S4674421::init() {	
    getLog() << Log::TRACE << "D0_2001_S4674421::init(): processing" << endl;
    _eventsFilledW = 0.0;
    _eventsFilledZ = 0.0;
    _h_dsigdpt_w = bookHistogram1D(1, 1, 1, "dsigma/dpT(W)");
    _h_dsigdpt_z = bookHistogram1D(1, 1, 2, "dsigma/dpT(Z)");

    const double binning[] = {0., 2., 4., 6., 8., 10., 12., 14., 16., 18., 20., 
			     25., 30., 35., 40., 50., 60., 70., 80., 100., 120., 160., 200.};
    vector<double> bins(23);
    for (int i=0; i<bins.size(); ++i)
      bins[i] = binning[i];

    _h_dsigdpt_scaled_z = bookHistogram1D("d01-x01-y03","dsigma/d(pT(Z)*mW/mZ)", bins);

    _h_temp = bookHistogram1D("temp", "dsigma/dpT(W) / dsigma/d(pT(Z)*mW/mZ)", bins);

  }
  

  void D0_2001_S4674421::analyze(const Event & event) {
      const double weight = event.weight();
      const WZandh& WZbosons = applyProjection<WZandh>(event, "WZ");

      // Fill W pT distributions
      const ParticleVector& Wens = WZbosons.Wens();
      for (ParticleVector::const_iterator p = Wens.begin(); p != Wens.end(); ++p) {
        FourMomentum pmom = p->momentum();
        _h_dsigdpt_w->fill(pmom.pT()/GeV, weight);
        _eventsFilledW += weight;
      }

      // Fill Z pT distributions
      size_t Zcount = 0;      
      const ParticleVector& Zees = WZbosons.Zees();
      for (ParticleVector::const_iterator p = Zees.begin(); p != Zees.end(); ++p) {
        FourMomentum pmom = p->momentum();
        if (pmom.mass()/GeV > _mZmin && pmom.mass()/GeV < _mZmax) {
          Zcount += 1;
          _eventsFilledZ += weight;
          getLog() << Log::DEBUG << "Z #" << Zcount << " pmom.pT() = " << pmom.pT()/GeV << endl;
          _h_dsigdpt_z->fill(pmom.pT()/GeV, weight);
          _h_dsigdpt_scaled_z->fill(pmom.pT()*_mwmz/GeV, weight);
        }
      }
  }


  void D0_2001_S4674421::finalize() { 
    // Get cross-section per event (i.e. per unit weight) from generator
    const double xSecPerEvent = crossSection()/picobarn / sumOfWeights();

    // Correct W pT distribution to W cross-section
    const double xSecW = xSecPerEvent * _eventsFilledW;

    // Correct Z pT distribution to Z cross-section
    const double xSecZ = xSecPerEvent * _eventsFilledZ;


    //_h_dsigdpt_wz_rat = histogramFactory().divide(getName() + "/d02-x01-y01", *_h_dsigdpt_w, *_h_dsigdpt_scaled_z);
    _h_temp = histogramFactory().divide(getName() + "/temp", *_h_dsigdpt_w, *_h_dsigdpt_scaled_z);


    const double wpt_integral = integral(_h_dsigdpt_w);
    normalize(_h_dsigdpt_w, xSecW);


    normalize(_h_dsigdpt_z, xSecZ);


    const double zpt_scaled_integral = integral(_h_dsigdpt_scaled_z);
    normalize(_h_dsigdpt_scaled_z, xSecZ);


    _h_temp->scale( (xSecW / wpt_integral) / (xSecZ / zpt_scaled_integral)  *  _brzee / _brwenu);
    
    std::vector<double> _x, _y, _ex, _ey;
    for ( int i = 0, N = _h_temp->axis().bins(); i < N; ++i ) {
      _x.push_back((_h_temp->axis().binLowerEdge(i) + _h_temp->axis().binUpperEdge(i))/2.0);
      _ex.push_back(_h_temp->axis().binWidth(i)/2.0);
      _y.push_back(_h_temp->binHeight(i)); 
      _ey.push_back(_h_temp->binError(i)); 
    }
    _dset_dsigpt_wz_rat = datapointsetFactory().createXY(getName() + "/d02-x01-y01", _h_temp->title(), _x, _y, _ex, _ey);
    

    //delete _h_temp;
    //tree.rm(getName() + "/temp");
    

  }

}
