// -*- C++ -*-
#include "Rivet/Rivet.hh"
#include "Rivet/Tools/Logging.hh"
#include "Rivet/Projections/FastJets.hh"

#include "fastjet/SISConePlugin.hh"
#include "fastjet/ATLASConePlugin.hh"
#include "fastjet/CMSIterativeConePlugin.hh"
#include "fastjet/CDFJetCluPlugin.hh"
#include "fastjet/CDFMidPointPlugin.hh"
#include "fastjet/D0RunIIConePlugin.hh"
#include "fastjet/TrackJetPlugin.hh"
#include "fastjet/JadePlugin.hh"
//#include "fastjet/PxConePlugin.hh"

namespace Rivet {


  FastJets::FastJets(const FinalState& fsp, JetAlgName alg, double rparameter, double seed_threshold)
    : JetAlg(fsp), _adef(0)
  {
    setName("FastJets");
    MSG_DEBUG("R parameter = " << rparameter);
    MSG_DEBUG("Seed threshold = " << seed_threshold);
    if (alg == KT) {
      _jdef = fastjet::JetDefinition(fastjet::kt_algorithm, rparameter, fastjet::E_scheme);
    } else if (alg == CAM) {
      _jdef = fastjet::JetDefinition(fastjet::cambridge_algorithm, rparameter, fastjet::E_scheme);
    } else if (alg == ANTIKT) {
      _jdef = fastjet::JetDefinition(fastjet::antikt_algorithm, rparameter, fastjet::E_scheme);
    } else if (alg == DURHAM) {
      _jdef = fastjet::JetDefinition(fastjet::ee_kt_algorithm, fastjet::E_scheme);
    } else {
      // Plugins:
      if (alg == SISCONE) {
        const double OVERLAP_THRESHOLD = 0.75;
        _plugin.reset(new fastjet::SISConePlugin(rparameter, OVERLAP_THRESHOLD));
      } else if (alg == PXCONE) {
        string msg = "PxCone currently not supported, since FastJet doesn't install it by default. ";
        msg += "Please notify the Rivet authors if this behaviour should be changed.";
        throw Error(msg);
        //_plugin.reset(new fastjet::PxConePlugin(rparameter));
      } else if (alg == ATLASCONE) {
        const double OVERLAP_THRESHOLD = 0.5;
        _plugin.reset(new fastjet::ATLASConePlugin(rparameter, seed_threshold, OVERLAP_THRESHOLD));
      } else if (alg == CMSCONE) {
        _plugin.reset(new fastjet::CMSIterativeConePlugin(rparameter, seed_threshold));
      } else if (alg == CDFJETCLU) {
        const double OVERLAP_THRESHOLD = 0.75;
        _plugin.reset(new fastjet::CDFJetCluPlugin(rparameter, OVERLAP_THRESHOLD, seed_threshold));
      } else if (alg == CDFMIDPOINT) {
        const double OVERLAP_THRESHOLD = 0.5;
        _plugin.reset(new fastjet::CDFMidPointPlugin(rparameter, OVERLAP_THRESHOLD, seed_threshold));
      } else if (alg == D0ILCONE) {
        const double min_jet_Et = 6.0;
        _plugin.reset(new fastjet::D0RunIIConePlugin(rparameter, min_jet_Et));
      } else if (alg == JADE) {
        _plugin.reset(new fastjet::JadePlugin());
      } else if (alg == TRACKJET) {
        _plugin.reset(new fastjet::TrackJetPlugin(rparameter));
      }
      _jdef = fastjet::JetDefinition(_plugin.get());
    }
  }


  FastJets::FastJets(const FinalState& fsp, fastjet::JetAlgorithm type,
                     fastjet::RecombinationScheme recom, double rparameter)
    : JetAlg(fsp), _adef(0)
  {
    setName("FastJets");
    _jdef = fastjet::JetDefinition(type, rparameter, recom);
  }


  FastJets::FastJets(const FinalState& fsp,
                     fastjet::JetDefinition::Plugin& plugin)
    : JetAlg(fsp), _adef(0)
  {
    setName("FastJets");
    /// @todo Should we be copying the plugin?
    _plugin.reset(&plugin);
    _jdef = fastjet::JetDefinition(_plugin.get());
  }


  int FastJets::compare(const Projection& p) const {
    const FastJets& other = dynamic_cast<const FastJets&>(p);
    // cout << "CMP " << _useInvisibles << endl;
    return \
      (_useInvisibles ? mkNamedPCmp(other, "FS") : mkNamedPCmp(other, "VFS")) ||
      cmp(_jdef.jet_algorithm(), other._jdef.jet_algorithm()) ||
      cmp(_jdef.recombination_scheme(), other._jdef.recombination_scheme()) ||
      cmp(_jdef.plugin(), other._jdef.plugin()) ||
      cmp(_jdef.R(), other._jdef.R()) ||
      cmp(_adef, other._adef);
  }



  void FastJets::project(const Event& e) {
    ParticleVector particles;
    if (_useInvisibles) {
      particles = applyProjection<FinalState>(e, "FS").particles();
    } else {
      particles = applyProjection<FinalState>(e, "VFS").particles();
    }
    calc(particles);
  }


  void FastJets::calc(const ParticleVector& ps) {
    _particles.clear();
    vector<fastjet::PseudoJet> vecs;
    // Store 4 vector data about each particle into vecs
    int counter = 1;
    foreach (const Particle& p, ps) {
      const FourMomentum fv = p.momentum();
      fastjet::PseudoJet pJet(fv.px(), fv.py(), fv.pz(), fv.E());
      pJet.set_user_index(counter);
      vecs.push_back(pJet);
      _particles[counter] = p;
      ++counter;
    }
    MSG_DEBUG("Running FastJet ClusterSequence construction");

    // Choose CSeq as basic or area-calculating depending on whether _adef pointer is non-null.
    if (_adef == 0) {
      _cseq.reset(new fastjet::ClusterSequence(vecs, _jdef));
    } else {
      _cseq.reset(new fastjet::ClusterSequenceArea(vecs, _jdef, *_adef));
    }
  }


  Jets FastJets::_pseudojetsToJets(const PseudoJets& pjets) const {
    Jets rtn;
    foreach (const fastjet::PseudoJet& pj, pjets) {
      Jet j;
      assert(clusterSeq());
      const PseudoJets parts = clusterSeq()->constituents(pj);
      foreach (const fastjet::PseudoJet& p, parts) {
        map<int, Particle>::const_iterator found = _particles.find(p.user_index());
        assert(found != _particles.end());
        j.addParticle(found->second);
      }
      rtn.push_back(j);
    }
    return rtn;
  }


  void FastJets::reset() {
    _yscales.clear();
    _particles.clear();
    /// @todo _cseq = fastjet::ClusterSequence();
  }


  size_t FastJets::numJets(double ptmin) const {
    if (_cseq.get() != 0) {
      return _cseq->inclusive_jets(ptmin).size();
    } else {
      return 0;
    }
  }


  Jets FastJets::_jets(double ptmin) const {
    Jets rtn = _pseudojetsToJets(pseudoJets(ptmin));
    return rtn;
  }


  // Jets FastJets::jetsByPt(double ptmin) const {
  //   return _pseudojetsToJets(pseudoJetsByPt(ptmin));
  // }


  // Jets FastJets::jetsByE(double ptmin) const {
  //   return _pseudojetsToJets(pseudoJetsByE(ptmin));
  // }


  // Jets FastJets::jetsByRapidity(double ptmin) const {
  //   return _pseudojetsToJets(pseudoJetsByRapidity(ptmin));
  // }


  PseudoJets FastJets::pseudoJets(double ptmin) const {
    if (_cseq.get() != 0) {
      return _cseq->inclusive_jets(ptmin);
    } else {
      return PseudoJets();
    }
  }


  vector<double> FastJets::ySubJet(const fastjet::PseudoJet& jet) const {
    assert(clusterSeq());
    map<int,vector<double> >::iterator iter = _yscales.find(jet.cluster_hist_index());
    fastjet::ClusterSequence subjet_cseq(clusterSeq()->constituents(jet), _jdef);
    vector<double> yMergeVals;
    for (int i = 1; i < 4; ++i) {
      // Multiply the dmerge value by R^2 so that it corresponds to a
      // relative k_T (fastjet has 1/R^2 in the d_ij distance by default)
      const double ktmerge = subjet_cseq.exclusive_dmerge(i) * _jdef.R()*_jdef.R();
      yMergeVals.push_back(ktmerge/jet.perp2());
    }
    _yscales.insert(make_pair( jet.cluster_hist_index(), yMergeVals ));
    return yMergeVals;
  }



  fastjet::PseudoJet FastJets::splitJet(fastjet::PseudoJet jet, double& last_R) const {
    // Sanity cuts
    if (jet.E() <= 0 || _cseq->constituents(jet).size() <= 1) {
      return jet;
    }

    // Build a new cluster sequence just using the consituents of this jet.
    assert(clusterSeq());
    fastjet::ClusterSequence cs(clusterSeq()->constituents(jet), _jdef);

    // Get the jet back again
    fastjet::PseudoJet remadeJet = cs.inclusive_jets()[0];
    MSG_DEBUG("Jet2:" << remadeJet.m() << "," << remadeJet.e());

    fastjet::PseudoJet parent1, parent2;
    fastjet::PseudoJet split(0.0, 0.0, 0.0, 0.0);
    while (cs.has_parents(remadeJet, parent1, parent2)) {
      MSG_DEBUG("Parents:" << parent1.m() << "," << parent2.m());
      if (parent1.m2() < parent2.m2()) {
        fastjet::PseudoJet tmp;
        tmp = parent1; parent1 = parent2; parent2 = tmp;
      }

      double ktdist = parent1.kt_distance(parent2);
      double rtycut2 = 0.3*0.3;
      if (parent1.m() < ((2.0*remadeJet.m())/3.0) && ktdist > rtycut2*remadeJet.m2()) {
        break;
      } else {
        remadeJet = parent1;
      }
    }

    last_R = 0.5 * sqrt(parent1.squared_distance(parent2));
    split.reset(remadeJet.px(), remadeJet.py(), remadeJet.pz(), remadeJet.E());
    return split;
  }



  fastjet::PseudoJet FastJets::filterJet(fastjet::PseudoJet jet,
                                         double& stingy_R, const double def_R) const {
    assert(clusterSeq());

    if (jet.E() <= 0.0 || clusterSeq()->constituents(jet).size() == 0) {
      return jet;
    }
    if (stingy_R == 0.0) {
      stingy_R = def_R;
    }

    stingy_R = def_R < stingy_R ? def_R : stingy_R;
    fastjet::JetDefinition stingy_jet_def(fastjet::cambridge_algorithm, stingy_R);

    //FlavourRecombiner recom;
    //stingy_jet_def.set_recombiner(&recom);
    fastjet::ClusterSequence scs(clusterSeq()->constituents(jet), stingy_jet_def);
    std::vector<fastjet::PseudoJet> stingy_jets = sorted_by_pt(scs.inclusive_jets());

    fastjet::PseudoJet reconst_jet(0.0, 0.0, 0.0, 0.0);

    for (unsigned isj = 0; isj < std::min(3U, (unsigned int) stingy_jets.size()); ++isj) {
      reconst_jet += stingy_jets[isj];
    }
    return reconst_jet;
  }

}
