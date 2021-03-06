// -*- C++ -*-
#include "Rivet/Config/RivetCommon.hh"
#include "Rivet/AnalysisHandler.hh"
#include "Rivet/Analysis.hh"
#include "Rivet/Tools/ParticleName.hh"
#include "Rivet/Tools/BeamConstraint.hh"
#include "Rivet/Tools/Logging.hh"
#include "Rivet/Projections/Beam.hh"
#include "YODA/IO.h"
#include <iostream>

using std::cout;
using std::cerr;

namespace Rivet {


  AnalysisHandler::AnalysisHandler(const string& runname)
    : _runname(runname),
      _initialised(false), _ignoreBeams(false), 
      _skipWeights(false), _weightCap(0.),
      _defaultWeightIdx(0), _dumpPeriod(0), _dumping(false)
  {  }


  AnalysisHandler::~AnalysisHandler() {
      static bool printed = false;
    // Print out MCnet boilerplate
    if (!printed && getLog().getLevel() <= 20) {
      cout << endl;
      cout << "The MCnet usage guidelines apply to Rivet: see http://www.montecarlonet.org/GUIDELINES" << endl;
      cout << "Please acknowledge plots made with Rivet analyses, and cite arXiv:1003.0694 (http://arxiv.org/abs/1003.0694)" << endl;
      printed = true;
    }
  }


  Log& AnalysisHandler::getLog() const {
    return Log::getLog("Rivet.Analysis.Handler");
  }


  /// http://stackoverflow.com/questions/4654636/how-to-determine-if-a-string-is-a-number-with-c
  namespace {
    bool is_number(const std::string& s) {
      std::string::const_iterator it = s.begin();
      while (it != s.end() && std::isdigit(*it)) ++it;
      return !s.empty() && it == s.end();
    }
  }

  /// Check if any of the weightnames is not a number
  bool AnalysisHandler::haveNamedWeights() const {
    bool dec=false;
    for (unsigned int i=0;i<_weightNames.size();++i) {
      string s = _weightNames[i];
      if (!is_number(s)) {
        dec=true;
        break;
      }
    }
    return dec;
  }


  void AnalysisHandler::init(const GenEvent& ge) {
    if (_initialised)
      throw UserError("AnalysisHandler::init has already been called: cannot re-initialize!");

    /// @todo Should the Rivet analysis objects know about weight names?

    setRunBeams(Rivet::beams(ge));
    MSG_DEBUG("Initialising the analysis handler");
    _eventNumber = ge.event_number();

    setWeightNames(ge);
    if (_skipWeights)
        MSG_INFO("Only using nominal weight. Variation weights will be ignored.");
    else if (haveNamedWeights())
        MSG_INFO("Using named weights");
    else
        MSG_INFO("NOT using named weights. Using first weight as nominal weight");

    _eventCounter = CounterPtr(weightNames(), Counter("_EVTCOUNT"));

    // Set the cross section based on what is reported by this event.
    if ( ge.cross_section() ) setCrossSection(HepMCUtils::crossSection(ge));

    // Check that analyses are beam-compatible, and remove those that aren't
    const size_t num_anas_requested = analysisNames().size();
    vector<string> anamestodelete;
    for (const AnaHandle a : analyses()) {
      if (!_ignoreBeams && !a->isCompatible(beams())) {
        //MSG_DEBUG(a->name() << " requires beams " << a->requiredBeams() << " @ " << a->requiredEnergies() << " GeV");
        anamestodelete.push_back(a->name());
      }
    }
    for (const string& aname : anamestodelete) {
      MSG_WARNING("Analysis '" << aname << "' is incompatible with the provided beams: removing");
      removeAnalysis(aname);
    }
    if (num_anas_requested > 0 && analysisNames().empty()) {
      cerr << "All analyses were incompatible with the first event's beams\n"
           << "Exiting, since this probably wasn't intentional!" << endl;
      exit(1);
    }

    // Warn if any analysis' status is not unblemished
    for (const AnaHandle a : analyses()) {
      if ( a->info().preliminary() ) {
        MSG_WARNING("Analysis '" << a->name() << "' is preliminary: be careful, it may change and/or be renamed!");
      } else if ( a->info().obsolete() ) {
        MSG_WARNING("Analysis '" << a->name() << "' is obsolete: please update!");
      } else if (( a->info().unvalidated() ) ) {
        MSG_WARNING("Analysis '" << a->name() << "' is unvalidated: be careful, it may be broken!");
      }
    }

    // Initialize the remaining analyses
    _stage = Stage::INIT;
    for (AnaHandle a : analyses()) {
      MSG_DEBUG("Initialising analysis: " << a->name());
      try {
        // Allow projection registration in the init phase onwards
        a->_allowProjReg = true;
        a->init();
        //MSG_DEBUG("Checking consistency of analysis: " << a->name());
        //a->checkConsistency();
      } catch (const Error& err) {
        cerr << "Error in " << a->name() << "::init method: " << err.what() << endl;
        exit(1);
      }
      MSG_DEBUG("Done initialising analysis: " << a->name());
    }
    _stage = Stage::OTHER;
    _initialised = true;
    MSG_DEBUG("Analysis handler initialised");
  }

  void AnalysisHandler::setWeightNames(const GenEvent& ge) {
    if (!_skipWeights)  _weightNames = HepMCUtils::weightNames(ge);
    if ( _weightNames.empty() )  _weightNames.push_back("");
    for ( int i = 0, N = _weightNames.size(); i < N; ++i )
      if ( _weightNames[i] == "" ) _defaultWeightIdx = i;
  }

  void AnalysisHandler::analyze(const GenEvent& ge) {
    // Call init with event as template if not already initialised
    if (!_initialised) init(ge);
    assert(_initialised);

    // Ensure that beam details match those from the first event (if we're checking beams)
    if ( !_ignoreBeams ) {
      const PdgIdPair beams = Rivet::beamIds(ge);
      const double sqrts = Rivet::sqrtS(ge);
      if (!compatible(beams, _beams) || !fuzzyEquals(sqrts, sqrtS())) {
        cerr << "Event beams mismatch: "
             << PID::toBeamsString(beams) << " @ " << sqrts/GeV << " GeV" << " vs. first beams "
             << this->beams() << " @ " << this->sqrtS()/GeV << " GeV" << endl;
        exit(1);
      }
    }

    // Create the Rivet event wrapper
    /// @todo Filter/normalize the event here
    bool strip = ( getEnvParam("RIVET_STRIP_HEPMC", string("NOOOO") ) != "NOOOO" );
    Event event(ge, strip);

    // set the cross section based on what is reported by this event.
    // if no cross section
    if ( ge.cross_section() ) setCrossSection(HepMCUtils::crossSection(ge));

    // Won't happen for first event because _eventNumber is set in init()
    if (_eventNumber != ge.event_number()) {

      pushToPersistent();

      _eventNumber = ge.event_number();

    }


    MSG_TRACE("starting new sub event");
    _eventCounter.get()->newSubEvent();

    for (const AnaHandle& a : analyses()) {
        for (auto ao : a->analysisObjects()) {
            ao.get()->newSubEvent();
        }
    }

    _subEventWeights.push_back(event.weights());
    if (_weightCap != 0.) {
      MSG_DEBUG("Implementing weight cap using a maximum |weight| of " << _weightCap << ".");
      for (size_t i = 0; i < _subEventWeights.size(); ++i) {
        for (size_t j = 0; j < _subEventWeights[i].size(); ++j) {
          if (abs(_subEventWeights[i][j]) > _weightCap) {
            _subEventWeights[i][j] = sign(_subEventWeights[i][j]) * _weightCap;
          }
        }
      }
    }
    MSG_DEBUG("Analyzing subevent #" << _subEventWeights.size() - 1 << ".");

    _eventCounter->fill();
    // Run the analyses
    for (AnaHandle a : analyses()) {
      MSG_TRACE("About to run analysis " << a->name());
      try {
        a->analyze(event);
      } catch (const Error& err) {
        cerr << "Error in " << a->name() << "::analyze method: " << err.what() << endl;
        exit(1);
      }
      MSG_TRACE("Finished running analysis " << a->name());
    }

    if ( _dumpPeriod > 0 && numEvents() > 0 && numEvents()%_dumpPeriod == 0 ) {
      MSG_DEBUG("Dumping intermediate results to " << _dumpFile << ".");
      _dumping = numEvents()/_dumpPeriod;
      finalize();
      writeData(_dumpFile);
      _dumping = 0;
    }

  }


  void AnalysisHandler::analyze(const GenEvent* ge) {
    if (ge == nullptr) {
      MSG_ERROR("AnalysisHandler received null pointer to GenEvent");
      //throw Error("AnalysisHandler received null pointer to GenEvent");
    }
    analyze(*ge);
  }

  void AnalysisHandler::pushToPersistent() {
    if ( _subEventWeights.empty() ) return;
    MSG_TRACE("AnalysisHandler::analyze(): Pushing _eventCounter to persistent.");
    _eventCounter.get()->pushToPersistent(_subEventWeights);
    for (const AnaHandle& a : analyses()) {
      for (auto ao : a->analysisObjects()) {
        MSG_TRACE("AnalysisHandler::analyze(): Pushing " << a->name()
                  << "'s " << ao->name() << " to persistent.");
        ao.get()->pushToPersistent(_subEventWeights);
      }
      MSG_TRACE("AnalysisHandler::analyze(): finished pushing "
                << a->name() << "'s objects to persistent.");
    }
    _subEventWeights.clear();
  }

  void AnalysisHandler::finalize() {
    if (!_initialised) return;
    MSG_DEBUG("Finalising analyses");

    _stage = Stage::FINALIZE;

    // First push all analyses' objects to persistent and final
    MSG_TRACE("AnalysisHandler::finalize(): Pushing analysis objects to persistent.");
    pushToPersistent();

    // Copy all histos to finalize versions.
    _eventCounter.get()->pushToFinal();
    _xs.get()->pushToFinal();
    for (const AnaHandle& a : analyses())
      for (auto ao : a->analysisObjects())
        ao.get()->pushToFinal();

    for (AnaHandle a : analyses()) {
      if ( _dumping && !a->info().reentrant() )  {
        if ( _dumping == 1 )
          MSG_DEBUG("Skipping finalize in periodic dump of " << a->name() << " as it is not declared re-entrant.");
        continue;
      }
      for (size_t iW = 0; iW < numWeights(); iW++) {
        _eventCounter.get()->setActiveFinalWeightIdx(iW);
        _xs.get()->setActiveFinalWeightIdx(iW);
        for (auto ao : a->analysisObjects())
          ao.get()->setActiveFinalWeightIdx(iW);
        try {
          MSG_TRACE("running " << a->name() << "::finalize() for weight " << iW << ".");
          a->finalize();
        } catch (const Error& err) {
          cerr << "Error in " << a->name() << "::finalize method: " << err.what() << '\n';
          exit(1);
        }
      }
    }

    // Print out number of events processed
    if (!_dumping) {
      const int nevts = numEvents();
      MSG_DEBUG("Processed " << nevts << " event" << (nevts != 1 ? "s" : ""));
    }

    _stage = Stage::OTHER;

  }


  AnalysisHandler& AnalysisHandler::addAnalysis(const string& analysisname, std::map<string, string> pars) {
     // Make an option handle.
    std::string parHandle = "";
    for (map<string, string>::iterator par = pars.begin(); par != pars.end(); ++par) {
      parHandle +=":";
      parHandle += par->first + "=" + par->second;
    }
    return addAnalysis(analysisname + parHandle);
  }


  AnalysisHandler& AnalysisHandler::addAnalysis(const string& analysisname) {
    // Check for a duplicate analysis
    /// @todo Might we want to be able to run an analysis twice, with different params?
    ///       Requires avoiding histo tree clashes, i.e. storing the histos on the analysis objects.
    string ananame = analysisname;
    vector<string> anaopt = split(analysisname, ":");
    if ( anaopt.size() > 1 ) ananame = anaopt[0];
    AnaHandle analysis( AnalysisLoader::getAnalysis(ananame) );
    if (analysis.get() != 0) { // < Check for null analysis.
      MSG_DEBUG("Adding analysis '" << analysisname << "'");
      map<string,string> opts;
      for ( int i = 1, N = anaopt.size(); i < N; ++i ) {
        vector<string> opt = split(anaopt[i], "=");
        if ( opt.size() != 2 ) {
          MSG_WARNING("Error in option specification. Skipping analysis " << analysisname);
          return *this;
        }
        if ( !analysis->info().validOption(opt[0], opt[1]) )
          MSG_WARNING("Setting the option '" << opt[0] << "' to '"
                      << opt[1] << "' for " << analysisname
                      << " has not been declared in the info file "
                      << " and may be ignored in the analysis.");
        opts[opt[0]] = opt[1];
      }
      for ( auto opt: opts) {
        analysis->_options[opt.first] = opt.second;
        analysis->_optstring += ":" + opt.first + "=" + opt.second;
      }
      for (const AnaHandle& a : analyses()) {
        if (a->name() == analysis->name() ) {
          MSG_WARNING("Analysis '" << analysisname << "' already registered: skipping duplicate");
          return *this;
        }
      }
      analysis->_analysishandler = this;
      _analyses[analysisname] = analysis;
    } else {
      MSG_WARNING("Analysis '" << analysisname << "' not found.");
    }
    // MSG_WARNING(_analyses.size());
    // for (const AnaHandle& a : _analyses) MSG_WARNING(a->name());
    return *this;
  }


  AnalysisHandler& AnalysisHandler::removeAnalysis(const string& analysisname) {
    MSG_DEBUG("Removing analysis '" << analysisname << "'");
    if (_analyses.find(analysisname) != _analyses.end()) _analyses.erase(analysisname);
    // }
    return *this;
  }


  // void AnalysisHandler::addData(const std::vector<YODA::AnalysisObjectPtr>& aos) {
  //   for (const YODA::AnalysisObjectPtr ao : aos) {
  //     string path = ao->path();
  //     if ( path.substr(0, 5) != "/RAW/" ) {
  //       _orphanedPreloads.push_back(ao);
  //       continue;
  //     }

  //     path = path.substr(4);
  //     ao->setPath(path);
  //     if (path.size() > 1) { // path > "/"
  //       try {
  //         const string ananame =  ::split(path, "/")[0];
  //         AnaHandle a = analysis(ananame);
  //         /// @todo FIXXXXX
  //         //MultiweightAOPtr mao = ????; /// @todo generate right Multiweight object from ao
  //         //a->addAnalysisObject(mao); /// @todo Need to statistically merge...
  //       } catch (const Error& e) {
  //         MSG_TRACE("Adding analysis object " << path <<
  //                   " to the list of orphans.");
  //         _orphanedPreloads.push_back(ao);
  //       }
  //     }
  //   }
  // }


  void AnalysisHandler::stripOptions(YODA::AnalysisObjectPtr ao,
                                     const vector<string> & delopts) const {
    string path = ao->path();
    string ananame = split(path, "/")[0];
    vector<string> anaopts = split(ananame, ":");
    for ( int i = 1, N = anaopts.size(); i < N; ++i )
      for ( auto opt : delopts )
        if ( opt == "*" || anaopts[i].find(opt + "=") == 0 )
          path.replace(path.find(":" + anaopts[i]), (":" + anaopts[i]).length(), "");
    ao->setPath(path);
  }


  void AnalysisHandler::mergeYodas(const vector<string> & aofiles,
                                   const vector<string> & delopts, bool equiv) {

    // Convenient typedef;
    typedef multimap<string, YODA::AnalysisObjectPtr> AOMap;

    // Store all found weights here.
    set<string> foundWeightNames;

    // Stor all found analyses.
    set<string> foundAnalyses;

    // Store all analysis objects here.
    vector<AOMap> allaos;

    // Go through all files and collect information.
    for ( auto file : aofiles ) {
      allaos.push_back(AOMap());
      AOMap & aomap = allaos.back();
      vector<YODA::AnalysisObject*> aos_raw;
      try {
        YODA::read(file, aos_raw);
      }
      catch (...) { //< YODA::ReadError&
        throw UserError("Unexpected error in reading file: " + file);
      }
      for (YODA::AnalysisObject* aor : aos_raw) {
        YODA::AnalysisObjectPtr ao(aor);
        AOPath path(ao->path());
        if ( !path )
          throw UserError("Invalid path name in file: " + file);
        if ( !path.isRaw() ) continue;

        foundWeightNames.insert(path.weight());
        // Now check if any options should be removed.
        for ( string delopt : delopts )
          if ( path.hasOption(delopt) ) path.removeOption(delopt);
        path.setPath();
        if ( path.analysisWithOptions() != "" )
          foundAnalyses.insert(path.analysisWithOptions());
        aomap.insert(make_pair(path.path(), ao));
      }
    }

    // Now make analysis handler aware of the weight names present.
    _weightNames.clear();
    _defaultWeightIdx = 0;
    for ( string name : foundWeightNames ) _weightNames.push_back(name);

    // Then we create and initialize all analyses
    for ( string ananame : foundAnalyses ) addAnalysis(ananame);
    _stage = Stage::INIT;
    for (AnaHandle a : analyses() ) {
      MSG_TRACE("Initialising analysis: " << a->name());
      if ( !a->info().reentrant() )
        MSG_WARNING("Analysis " << a->name() << " has not been validated to have "
                    << "a reentrant finalize method. The merged result is unpredictable.");
      try {
        // Allow projection registration in the init phase onwards
        a->_allowProjReg = true;
        a->init();
      } catch (const Error& err) {
        cerr << "Error in " << a->name() << "::init method: " << err.what() << endl;
        exit(1);
      }
      MSG_TRACE("Done initialising analysis: " << a->name());
    }
    _stage = Stage::OTHER;
    _initialised = true;

    // Now get all booked analysis objects.
    vector<MultiweightAOPtr> raos;
    for (AnaHandle a : analyses()) {
      for (const auto & ao : a->analysisObjects()) {
        raos.push_back(ao);
      }
    }

    // Collect global weights and xcoss sections and fix scaling for
    // all files.
    _eventCounter = CounterPtr(weightNames(), Counter("_EVTCOUNT"));
    _xs = Scatter1DPtr(weightNames(), Scatter1D("_XSEC"));
    for (size_t iW = 0; iW < numWeights(); iW++) {
      _eventCounter.get()->setActiveWeightIdx(iW);
      _xs.get()->setActiveWeightIdx(iW);
      YODA::Counter & sumw = *_eventCounter;
      YODA::Scatter1D & xsec = *_xs;
      vector<YODA::Scatter1DPtr> xsecs;
      vector<YODA::CounterPtr> sows;
      for ( auto & aomap : allaos ) {
        auto xit = aomap.find(xsec.path());
        if ( xit != aomap.end() )
          xsecs.push_back(dynamic_pointer_cast<YODA::Scatter1D>(xit->second));
        else
          xsecs.push_back(YODA::Scatter1DPtr());
        xit = aomap.find(sumw.path());
        if ( xit != aomap.end() )
          sows.push_back(dynamic_pointer_cast<YODA::Counter>(xit->second));
        else
          sows.push_back(YODA::CounterPtr());
      }
      double xs = 0.0, xserr = 0.0;
      for ( int i = 0, N = sows.size(); i < N; ++i ) {
        if ( !sows[i] || !xsecs[i] ) continue;
        double xseci = xsecs[i]->point(0).x();
        double xsecerri = sqr(xsecs[i]->point(0).xErrAvg());
        sumw += *sows[i];
        double effnent = sows[i]->effNumEntries();
        xs += (equiv? effnent: 1.0)*xseci;
        xserr += (equiv? sqr(effnent): 1.0)*xsecerri;
      }
      vector<double> scales(sows.size(), 1.0);
      if ( equiv ) {
        xs /= sumw.effNumEntries();
        xserr = sqrt(xserr)/sumw.effNumEntries();
      } else {
        xserr = sqrt(xserr);
        for ( int i = 0, N = sows.size(); i < N; ++i )
          scales[i] = (sumw.sumW()/sows[i]->sumW())*
           (xsecs[i]->point(0).x()/xs);
      }
      xsec.reset();
      xsec.addPoint(Point1D(xs, xserr));

      // Go through alla analyses and add stuff to their analysis objects;
      for (AnaHandle a : analyses()) {
        for (const auto & ao : a->analysisObjects()) {
          ao.get()->setActiveWeightIdx(iW);
          YODA::AnalysisObjectPtr yao = ao.get()->activeYODAPtr();
          for ( int i = 0, N = sows.size(); i < N; ++i ) {
            if ( !sows[i] || !xsecs[i] ) continue;
            auto range = allaos[i].equal_range(yao->path());
            for ( auto aoit = range.first; aoit != range.second; ++aoit )
              if ( !addaos(yao, aoit->second, scales[i]) )
                MSG_WARNING("Cannot merge objects with path " << yao->path()
                            <<" of type " << yao->annotation("Type") );
          }
          ao.get()->unsetActiveWeight();
        }
      }
      _eventCounter.get()->unsetActiveWeight();
      _xs.get()->unsetActiveWeight();
    }

    // Finally we just have to finalize all analyses, leaving to the
    // controlling program to write it out some yoda-file.
    finalize();

  }

  void AnalysisHandler::readData(const string& filename) {
    try {
      /// @todo Use new YODA SFINAE to fill the smart ptr vector directly
      vector<YODA::AnalysisObject*> aos_raw;
      YODA::read(filename, aos_raw);
      for (YODA::AnalysisObject* aor : aos_raw)
        _preloads[aor->path()] = YODA::AnalysisObjectPtr(aor);
    } catch (...) { //< YODA::ReadError&
      throw UserError("Unexpected error in reading file: " + filename);
    }
  }

  vector<MultiweightAOPtr> AnalysisHandler::getRivetAOs() const {
      vector<MultiweightAOPtr> rtn;

      for (AnaHandle a : analyses()) {
          for (const auto & ao : a->analysisObjects()) {
              rtn.push_back(ao);
          }
      }

      rtn.push_back(_eventCounter);
      rtn.push_back(_xs);

      return rtn;
  }

  void AnalysisHandler::writeData(const string& filename) const {

    // This is where we store the OAs to be written.
    vector<YODA::AnalysisObjectPtr> output;

    // First get all multiwight AOs
    vector<MultiweightAOPtr> raos = getRivetAOs();
    output.reserve(raos.size()*2*numWeights());

    // Fix the oredering so that default weight is written out first.
    vector<size_t> order;
    if ( _defaultWeightIdx >= 0 && _defaultWeightIdx < numWeights() )
      order.push_back(_defaultWeightIdx);
    for ( size_t  i = 0; i < numWeights(); ++i )
      if ( i != _defaultWeightIdx ) order.push_back(i);

    // Then we go through all finalized AOs one weight at a time
    for (size_t iW : order ) {
      for ( auto rao : raos ) {
        rao.get()->setActiveFinalWeightIdx(iW);
        if ( rao->path().find("/TMP/") != string::npos ) continue;
        output.push_back(rao.get()->activeYODAPtr());
      }
    }

    // Finally the RAW objects.
    for (size_t iW : order ) {
      for ( auto rao : raos ) {
        rao.get()->setActiveWeightIdx(iW);
        output.push_back(rao.get()->activeYODAPtr());
      }
    }

    try {
      YODA::write(filename, output.begin(), output.end());
    } catch (...) { //< YODA::WriteError&
      throw UserError("Unexpected error in writing file: " + filename);
    }
  }


  string AnalysisHandler::runName() const { return _runname; }
  size_t AnalysisHandler::numEvents() const { return _eventCounter->numEntries(); }


  std::vector<std::string> AnalysisHandler::analysisNames() const {
    std::vector<std::string> rtn;
    for (AnaHandle a : analyses()) {
      rtn.push_back(a->name());
    }
    return rtn;
  }


  AnalysisHandler& AnalysisHandler::addAnalyses(const std::vector<std::string>& analysisnames) {
    for (const string& aname : analysisnames) {
      //MSG_DEBUG("Adding analysis '" << aname << "'");
      addAnalysis(aname);
    }
    return *this;
  }


  AnalysisHandler& AnalysisHandler::removeAnalyses(const std::vector<std::string>& analysisnames) {
    for (const string& aname : analysisnames) {
      removeAnalysis(aname);
    }
    return *this;
  }


  void AnalysisHandler::setCrossSection(pair<double,double> xsec) {
    _xs = Scatter1DPtr(weightNames(), Scatter1D("_XSEC"));
    _eventCounter.get()->setActiveWeightIdx(_defaultWeightIdx);
    double nomwgt = sumW();

    // The cross section of each weight variation is the nominal cross section
    // times the sumW(variation) / sumW(nominal).
    // This way the cross section will work correctly
    for (size_t iW = 0; iW < numWeights(); iW++) {
      _eventCounter.get()->setActiveWeightIdx(iW);
      double s = sumW() / nomwgt;
      _xs.get()->setActiveWeightIdx(iW);
      _xs->addPoint(xsec.first*s, xsec.second*s);
    }

    _eventCounter.get()->unsetActiveWeight();
    _xs.get()->unsetActiveWeight();
    return;
  }

  AnalysisHandler& AnalysisHandler::addAnalysis(Analysis* analysis) {
    analysis->_analysishandler = this;
    // _analyses.insert(AnaHandle(analysis));
    _analyses[analysis->name()] = AnaHandle(analysis);
    return *this;
  }

  PdgIdPair AnalysisHandler::beamIds() const {
    return Rivet::beamIds(beams());
  }


  double AnalysisHandler::sqrtS() const {
    return Rivet::sqrtS(beams());
  }

  void AnalysisHandler::setIgnoreBeams(bool ignore) {
    _ignoreBeams=ignore;
  }

  void AnalysisHandler::skipMultiWeights(bool ignore) {
    _skipWeights=ignore;
  }


}
