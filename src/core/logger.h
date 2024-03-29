#ifndef BARBU_CORE_LOGGER_H_
#define BARBU_CORE_LOGGER_H_

#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

#include "glm/gtx/string_cast.hpp" // glm::to_string

#include "utils/singleton.h"


// ----------------------------------------------------------------------------

//
// A colored logger that could be used inside loops to print messages once.
//
//  Type of logs :
//    * Message    : white, not hashed (will be repeated).
//    * Info       : blue, hashed (will not be repeated).
//    * Warning    : yellow, hashed, used in stats.
//    * Error      : bold red, hashed, display file and line, used in stats.
//    * FatalError : flashing red, not hashed, exit program instantly. 
//
class Logger : public Singleton<Logger> {
  friend class Singleton<Logger>;

 public:
  static std::string TrimFilename(std::string const& filename) {
    return filename.substr(filename.find_last_of("/\\") + 1);
  }

  static std::string TrimFilename(std::string_view filename) {
    return TrimFilename(std::string(filename));
  }

  enum class LogType {
    Message,
    Info,
    Warning,
    Error,
    FatalError
  };

  ~Logger() {
    displayStats();
  }

  template<typename T, typename ... Args>
  bool log(char const* file, char const* fn, int line, bool useHash, LogType type, T first, Args ... args) {
    // Clear the local stream and retrieve the full current message.
    out_.str(std::string());
    subLog(first, args ...);

    // Trim filename for display.
    std::string filename(file);
    filename = filename.substr(filename.find_last_of("/\\") + 1);

    // Check the message has not been registered yet.
    if (useHash) {
      std::string const key = /*filename + std::to_string(line) +*/ out_.str();
      if (0u < error_log_.count(key)) {
        return false;
      }
      error_log_[key] = true;
    }

    // Prefix.
    switch (type) {
      case LogType::Message:
        std::cerr << "\x1b[0;29m"
                  //<< "[Message] "
                  ;
      break;

      case LogType::Info:
        std::cerr << "\x1b[0;36m"
                  //<< "[Info] "
                  ;
      break;

      case LogType::Warning:
        std::cerr << "\x1b[3;33m" 
                  //<< "[Warning] "
                  ;
        ++warning_count_;
      break;

      case LogType::Error:
        std::cerr << "\x1b[1;31m" 
                  << "[Error] "
                  ;
        ++error_count_;
      break;

      case LogType::FatalError:
        std::cerr << "\x1b[5;31m[Fatal Error]\x1b[0m\n" 
                  << "\x1b[0;31m"
                  ;
      break;
    }

    // Log message.
    std::cerr << out_.str();

    // Suffix.
    switch (type) {
      case LogType::Message:
      case LogType::Info:
      case LogType::Warning:
      break;

      case LogType::Error:
      case LogType::FatalError:
        std::cerr << std::endl <<  "(" << filename << " " << fn << " L." << line << ")" << std::endl;
      break;
    }

    // Closing.
    std::cerr << "\x1b[0m" << std::endl;
      
    return true;
  }


  template<typename T, typename ... Args>
  void message(char const* file, char const* fn, int line, T first, Args ... args) {
    log(file, fn, line, false, LogType::Message, first, args...);
  }

  template<typename T, typename ... Args>
  void info(char const* file, char const* fn, int line, T first, Args ... args) {
    log(file, fn, line, true, LogType::Info, first, args...);
  }

  template<typename T, typename ... Args>
  void warning(char const* file, char const* fn, int line, T first, Args ... args) {
    log(file, fn, line, true, LogType::Warning, first, args...);
  }

  template<typename T, typename ... Args>
  void error(char const* file, char const* fn, int line, T first, Args ... args) {
    log(file, fn, line, true, LogType::Error, first, args...);
  }

  template<typename T, typename ... Args>
  void fatal_error(char const* file, char const* fn, int line, T first, Args ... args) {
    log(file, fn, line, false, LogType::FatalError, first, args...);
    exit(EXIT_FAILURE);
  }

 private:
  template<typename T>
  void subLog(T first) {
    out_ << first;
  }

  template<typename T, typename ... Args>
  void subLog(T first, Args ... args) {
    out_ << first << " ";
    subLog(args ...);
  }

  void displayStats() {
#ifndef NDEBUG
    if (warning_count_ > 0 || error_count_ > 0) {
      std::cerr << std::endl <<
        "\x1b[7;38m================= Logger stats =================\x1b[0m\n" \
        " * Warnings : " << warning_count_ << std::endl <<
        " * Errors   : " << error_count_ << std::endl <<
        "\x1b[7;38m================================================\x1b[0m\n" \
        << std::endl;
    }
#endif // NDEBUG
  }

  std::stringstream out_;
  std::unordered_map<std::string, bool> error_log_;
  int32_t warning_count_ = 0;
  int32_t error_count_   = 0;
};

// ----------------------------------------------------------------------------

#define LOG_MESSAGE( ... )      Logger::Get().message     ( __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_INFO( ... )         Logger::Get().info        ( __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_WARNING( ... )      Logger::Get().warning     ( __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_ERROR( ... )        Logger::Get().error       ( __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_FATAL_ERROR( ... )  Logger::Get().fatal_error ( __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

#ifdef BARBU_ENABLE_DEBUG_LOG
  #define LOG_DEBUG( ... )        Logger::Get().message   ( __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
  #define LOG_DEBUG_INFO( ... )   Logger::Get().info      ( __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#else
  #define LOG_DEBUG( ... )        
  #define LOG_DEBUG_INFO( ... )
#endif

#ifndef NDEBUG
  #define LOG_CHECK( x )          if (!(x)) Logger::Get().warning( __FILE__, __FUNCTION__, __LINE__, #x, " test fails")
#else
  #define LOG_CHECK( x )          if (!(x)) {}
#endif

// ----------------------------------------------------------------------------

// #ifdef __ANDROID__

// #include <android/log.h>
// #define LOG_TAG    "Barbu-droid"

// #define LOG_MESSAGE( ... )      __android_log_print( ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
// #define LOG_INFO( ... )         __android_log_print( ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
// #define LOG_WARNING( ... )      __android_log_print( ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
// #define LOG_ERROR( ... )        __android_log_print( ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
// #define LOG_FATAL_ERROR( ... )  __android_log_print( ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
// #define LOG_DEBUG( ... )
// #define LOG_DEBUG_INFO( ... )

// #endif

// ----------------------------------------------------------------------------

#endif  // BARBU_CORE_LOGGER_H_
