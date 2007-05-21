// $Id: $

#include "Rivet/Rivet.hh"
#include "Rivet/HistoFormat.hh"
#include "Rivet/Analysis/Analysis.fhh"
#include "Rivet/Tools/Commandline.hh"
#include <tclap/CmdLine.h>
#include <fstream>
#include <iostream>

namespace Rivet {

  namespace Commandline {

    void addAnalysisArgs(TCLAP::CmdLine& cmd,
                         TCLAP::MultiArg<string>*& analysesArg,
                         TCLAP::SwitchArg*& analysesAllArg,
                         TCLAP::ValuesConstraint<string>*& anaNameConstraint) {
      vector<string> knownAnalyses = Rivet::getKnownAnalysisNames();
      vector<string> tmp = knownAnalyses;
      for (vector<string>::const_iterator i = tmp.begin(); i != tmp.end(); ++i) {
        knownAnalyses.push_back("~" + *i);
      }
      anaNameConstraint = new TCLAP::ValuesConstraint<string>(knownAnalyses);
      analysesArg = new TCLAP::MultiArg<string>("a", "analysis", "A Rivet analysis to be run. Prefix name with a ~ to disable instead", false, anaNameConstraint, cmd);
      analysesAllArg = new TCLAP::SwitchArg("A", "all_analyses", "Run all Rivet analyses (unless any are disabled)", cmd, false);
      /// @todo Can we use -A for removals and get the ordering right?
    }


    void addHistoArgs(TCLAP::CmdLine& cmd,
                      TCLAP::ValueArg<string>*& histoNameArg,
                      TCLAP::ValueArg<string>*& histoTypeArg,
                      TCLAP::ValuesConstraint<string>*& histoTypeConstraint) {
      histoNameArg = new TCLAP::ValueArg<string>("H", "histoname", "Base name of histogram file, with no extension ('Rivet' by default)", false, "Rivet", "name", cmd);

      vector<string> knownFormats = Rivet::getKnownHistoFormatNames();
      TCLAP::ValuesConstraint<string> allowedHistoTypes(knownFormats);
      histoTypeConstraint = new TCLAP::ValuesConstraint<string>(knownFormats);
      histoTypeArg = new TCLAP::ValueArg<string>("", "histotype", "Histogram output format (default is AIDA XML)", false, "AIDA", histoTypeConstraint, cmd);
    }


    void addLoggingArgs(TCLAP::CmdLine& cmd, 
                        TCLAP::MultiArg<string>*& logsArg) {
      string mesg;
      mesg += "Set log level in 'name:level' format. The levels are INFO, DEBUG and WARNING";
      logsArg = new TCLAP::MultiArg<string>("l", "loglevel", mesg, false, "name:level", cmd);
      //TCLAP::ValueArg<string> logFileArg("L", "logfile", "File to write logs to", false, "", "logfile", cmd);
    }

    
    void useAnalysisArgs(TCLAP::CmdLine& cmd,
                         TCLAP::MultiArg<string>* analysesArg,
                         TCLAP::SwitchArg* analysesAllArg,
                         set<AnalysisName>& cfgAnalyses) {
      // Do nothing if CLI pointer(s) are null
      if (!analysesArg || !analysesAllArg) return;

      // First handle the "enable all analyses" option...
      if (analysesAllArg->getValue()) {
        Rivet::AnalysisList alist = Rivet::getKnownAnalysisEnums();
        for (Rivet::AnalysisList::const_iterator a = alist.begin(); a != alist.end(); ++a) {
          cfgAnalyses.insert(*a);
        }
      }
      // ...then handle individuals, including negations
      for (vector<string>::const_iterator a = analysesArg->getValue().begin(); 
           a != analysesArg->getValue().end(); ++a) {
        Rivet::AnalysisMapR amapr = Rivet::getKnownAnalysesR();
        try {
          // Check for analysis disabling with ~ prefix
          if (a->rfind("~", 0) == string::npos) {
            cfgAnalyses.insert(amapr[*a]);
          } else {
            string aneg = a->substr(1, a->size() - 1);
            // Not really necessary since sets should have no duplicates...
            if (cfgAnalyses.find(amapr[aneg]) != cfgAnalyses.end()) {
              cfgAnalyses.erase(amapr[aneg]);
            }
          }
        } catch (std::exception& e) {
          throw std::runtime_error("Invalid analysis choice: " + *a);
        }
      }

      delete analysesArg;
      delete analysesAllArg;
    }
    

    void useHistoArgs(TCLAP::CmdLine& cmd,
                      TCLAP::ValueArg<string>* histoNameArg,
                      TCLAP::ValueArg<string>* histoTypeArg,
                      string& cfgHistoFileName,
                      HistoFormat& cfgHistoFormat) {
      // Do nothing if CLI pointers are null
      if (!histoNameArg || !histoTypeArg) return;

      // Get histogram filename
      cfgHistoFileName = histoNameArg->getValue();

      // Get histogram format
      if (histoTypeArg->getValue() == "AIDA") {
        cfgHistoFormat = AIDAML;
      } else if (histoTypeArg->getValue() == "FLAT") {
        cfgHistoFormat = FLAT;
      } else if (histoTypeArg->getValue() == "ROOT") {
	cfgHistoFormat = ROOT;
        //throw std::runtime_error("ROOT is not currently supported as an output histogram format");
        /// @todo Support ROOT histogram output (possibly via a ROOT AIDA reader).
      }


      delete histoNameArg;
      delete histoTypeArg;
    }


    void useLoggingArgs(TCLAP::CmdLine& cmd,
                        TCLAP::MultiArg<string>* logsArg,
                        Log::LevelMap& cfgLogLevels) {
      // Do nothing if CLI pointers are null
      if (!logsArg) return;

      for (vector<string>::const_iterator l = logsArg->getValue().begin(); 
           l != logsArg->getValue().end(); ++l) {
        unsigned int breakpos = l->find(":");
        if (breakpos != string::npos) {
          string key = l->substr(0, breakpos);
          string value = l->substr(breakpos + 1, l->size() - breakpos - 1);
          cfgLogLevels[key] = Log::getLevelFromName(value);
        } else {
          throw runtime_error("Invalid log setting format: " + *l);
        }
      }

      delete logsArg;
    }


  }  
}
