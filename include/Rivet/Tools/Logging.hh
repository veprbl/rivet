#ifndef RIVET_LOGGING_HH 
#define RIVET_LOGGING_HH

#include "Rivet/Rivet.hh"

namespace Rivet {


  class Log {
  public:

    /// Log priority levels.
    enum Level {
      TRACE = 0, DEBUG = 10, INFO = 20, WARN = 30, ERROR = 40
    };

    /// Typedef for a collection of named logs.
    typedef map<const string, Log*> LogMap;

    /// Typedef for a collection of named log levels.
    typedef map<const string, int> LevelMap;

    /// Typedef for a collection of shell color codes, accessed by log level.
    typedef map<int, string> ColorCodes;

  private:
    /// A static map of existing logs: we don't make more loggers than necessary.
    static LogMap existingLogs;

    /// A static map of default log levels.
    static LevelMap defaultLevels;

    /// A static map of shell color codes for the log levels.
    static ColorCodes colorCodes;

    /// Shell color code for the end of the log levels.
    static string endColorCode;

    /// Show timestamp?
    static bool showTimestamp;

    /// Show log level?
    static bool showLogLevel;

    /// Show logger name?
    static bool showLoggerName;

    /// Use shell colour escape codes?
    static bool useShellColors;

  public:
    /// Set the log levels
    static void setLevels(const string& name, int level);
    static void setLevels(const LevelMap& logLevels);

    static void setShowTimestamp(const bool showTime=true) {
      showTimestamp = showTime;
    }

    static void setShowLevel(const bool showLevel=true) {
      showLogLevel = showLevel;
    }

    static void setShowLoggerName(const bool showName=true) {
      showLoggerName = showName;
    }

    static void setUseColors(const bool useColors=true) {
      useShellColors = useColors;
    }

  protected:
    /// @name Hidden constructors etc.
    //@{
    /// Constructor 1
    Log(const string& name);

    /// Constructor 2
    Log(const string& name, int level);

    /// Copy constructor
    //Log(const Log&);

    /// Copy assignment operator
    //Log& operator=(const Log&);
    //@}

    static string getColorCode(int level);

  public:
    /// Get a logger with the given name. The level will be taken from the 
    /// "requestedLevels" static map or will be INFO by default.
    static Log& getLog(const string& name);

  public:
    /// Get the priority level of this logger.
    int getLevel() const {
      return _level;
    }

    /// Set the priority level of this logger.
    Log& setLevel(int level) {
      _level = level;
      return *this;
    }

    /// Get a log level enum from a string.
    static Level getLevelFromName(const string& level);

    /// Get the string representation of a log level.
    static string getLevelName(int level);

    /// Get the name of this logger.
    string getName() const {
      return _name;
    }

    /// Set the name of this logger.
    Log& setName(const string& name) {
      _name = name;
      return *this;
    }

    /// Will this log level produce output on this logger at the moment?
    bool isActive(int level) const {
      return (level >= _level);
    }

    /// @name Explicit log methods
    //@{
    void trace(const string& message) { log(TRACE, message); }

    void debug(const string& message) { log(DEBUG, message); }
    
    void info(const string& message) { log(INFO, message); }

    void warn(const string& message) { log(WARN, message); }

    void error(const string& message) { log(ERROR, message); }
    //@}

  private:
    /// This logger's name
    string _name;
    
    /// Threshold level for this logger.
    int _level;
    
  protected:
    /// Write a message at a particular level.
    void log(int level, const string& message);

    /// Turn a message string into the current log format.
    string formatMessage(int level, const string& message);

  public:

    /// A null output stream, used for piping discarded output to nowhere.
    /// @todo Hide this...
    ostream* const _nostream;

    /// The streaming operator can use Log's internals.
    friend ostream& operator<<(Log& log, int level);

  };
  
  /// Streaming output to a logger must have a Log::Level/int as its first argument.
  ostream& operator<<(Log& log, int level);
  
}


#endif
