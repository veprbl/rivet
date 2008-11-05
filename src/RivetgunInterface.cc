// -*- C++ -*-

#include "Rivet/Rivet.hh"
#include "Rivet/Tools/Configuration.hh"
#include "Rivet/Tools/Logging.hh"
using namespace Rivet;

#ifdef HAVE_AGILE
#include "AGILe/AGILe.hh"
#endif

#ifdef FC_DUMMY_MAIN
extern "C" int F77_DUMMY_MAIN() { return 1; }
#endif


////////////////////////////////////////////////


// Forward declare the function that actually runs 
// the event loop and analysis, which is isolated from 
// the administration and param parsing code in main().
namespace Rivet {
  void generate(Configuration& cfg, Log& log);
}


// The main function of the rivetgun executable.
int main(int argc, char* argv[]) {

  // Debug initial gen/analysis scan (before proper command line log levels are set)
  for (int argi = 0; argi < argc; ++argi) {
    string arg(argv[argi]);
    if (arg.find("Rivet.Loader=TRACE") != string::npos ||
        arg.find("Rivet=TRACE") != string::npos) {
      Rivet::Log::setLevel("Rivet.Loader", Log::TRACE);
    }
    #ifdef HAVE_AGILE
    if (arg.find("AGILe.Loader=TRACE") != string::npos ||
        arg.find("AGILe=TRACE") != string::npos) {
      AGILe::Log::setLevel("AGILe.Loader", Log::TRACE);
    }
    #endif
  }

  // Parse command line into a configuration object.
  Configuration cfg;
  try {
    cfg = Commandline::parseArgs(argc, argv);
  } catch (Error& e) { 
    cerr << "Error: " << e.what() << endl; 
    return EXIT_FAILURE;
  }


  // Set log levels from command line and get a logger
  Rivet::Log::setLevels(cfg.logLevels);
  Rivet::Log::setUseColors(cfg.useLogColors);

  #ifdef HAVE_AGILE
  AGILe::Log::setLevels(cfg.logLevels);
  AGILe::Log::setUseColors(cfg.useLogColors);
  #endif

  Log& log = Rivet::Log::getLog("RivetGun.Main");
  stringstream cmd;
  cmd << "rivetgun ";
  for (int i = 1; i < argc; ++i) cmd << argv[i] << " ";
  log << Log::INFO << "Called with command line: " << cmd.str() << endl;


  try {
    // Configure and run the generator.
    generate(cfg, log);
  } 
  
  // Main loop exception handling.
  catch (Error& e) { 
    cerr << "Error: " << e.what() << endl; 
    return EXIT_FAILURE;
  }


  return EXIT_SUCCESS;
}
