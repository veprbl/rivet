
#include "Rivet/Tools/Logging.hh"
#include <ctime>

using namespace std;


namespace Rivet {

  Log::LogMap Log::existingLogs;
  Log::LevelMap Log::defaultLevels;
  bool Log::showTimestamp = false;
  bool Log::showLogLevel = true;
  bool Log::showLoggerName = true;

  Log::Log(const string& name) 
    : _name(name), _level(INFO), _nostream(new ostream(0)) { }


  Log::Log(const string& name, const Level& level)
    : _name(name), _level(level), _nostream(new ostream(0)) { }


  Log& Log::getLog(const string& name) {
    if (existingLogs.find(name) == existingLogs.end()) {
      Level level = INFO;
      // Try running through all parent classes to find an existing level
      string tmpname = name;
      bool triedAllParents = false;
      while (! triedAllParents) {
        // Is there a default level?
        if (defaultLevels.find(tmpname) != defaultLevels.end()) {
          level = defaultLevels.find(tmpname)->second;
          break;
        }
        // Is there already such a logger?
        if (existingLogs.find(tmpname) != existingLogs.end()) {
          level = existingLogs.find(tmpname)->second->getLevel();
          break;
        }
        // Crop the string back to the next parent level
        size_t lastDot = tmpname.find_last_of(".");
        if (lastDot == string::npos) {
          triedAllParents = true;
        } else {
          tmpname = tmpname.substr(0, lastDot);
        }
      }
      existingLogs[name] = new Log(name, level);
    }
    return *existingLogs[name];
  }


  Log& Log::getLog(const string& name, const Level& level) {
    //cout << "Log::getLog(name, level): are we really using this? Does it make sense?" << endl;
    if (existingLogs.find(name) == existingLogs.end()) {
      existingLogs[name] = new Log(name, level);
    }
    return *existingLogs[name];
  }


  string Log::getLevelName(const Level& level) {
    switch(level) {
    case TRACE:
      return "TRACE";
    case DEBUG:
      return "DEBUG";
    case INFO:
      return "INFO";
    case WARN:
      return "WARN";
    case ERROR:
      return "ERROR";
    }
    throw runtime_error("Enum value was not a valid log level. How did that happen?");
  }


  Log::Level Log::getLevelFromName(const string& level) {
    if (level == "TRACE") return TRACE;
    if (level == "DEBUG") return DEBUG;
    if (level == "INFO") return INFO;
    if (level == "WARN") return WARN;
    if (level == "ERROR") return ERROR;
    throw runtime_error("Couldn't create a log level from string '" + level + "'");
  }


  string Log::formatMessage(const Level& level, const std::string& message) {
    string out;
    if (Log::showLoggerName) {
      out += getName();
      out += ": ";
    }

    if (Log::showLogLevel) {
      out += Log::getLevelName(level);
      out += " ";
    }

    if (Log::showTimestamp) {
      time_t rawtime;
      time(&rawtime);
      char* timestr = ctime(&rawtime);
      timestr[24] = ' ';
      out += timestr;
      out += " ";
    }

    out += message;
    return out;
  }


  void Log::log(const Level& level, const std::string& message) {
    if (isActive(level)) {
      cout << formatMessage(level, message) << endl;
    }
  }


  ostream& operator<<(Log& log, const Log::Level& level) {
    if (log.isActive(level)) {
      cout << log.formatMessage(level, "");
      return cout;
    } else {
      return *(log._nostream);
    }
  }

}
