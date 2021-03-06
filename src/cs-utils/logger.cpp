////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "logger.hpp"

#include <VistaBase/VistaStreamUtils.h>
#include <spdlog/sinks/base_sink.h>
#include <sstream>

namespace cs::utils::logger {

namespace {

// This class is used to intercept Vista messages.
template <spdlog::level::level_enum level>
class SpdlogBuffer : public std::streambuf {
 public:
  SpdlogBuffer(std::shared_ptr<spdlog::logger> const& logger)
      : mLogger(logger) {
  }

 private:
  // Whenever vista prints a '\n', a new log message is emitted.
  int_type overflow(int_type c) override {
    char_type ch = traits_type::to_char_type(c);
    if (ch == '\n') {
      mLogger->log(level, "{}", mLine);
      mLine = "";
    } else {
      mLine += ch;
    }

    return 0;
  }

  std::shared_ptr<spdlog::logger> mLogger;
  std::string                     mLine;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

class SignalSink : public spdlog::sinks::base_sink<std::mutex> {
 public:
  Signal<std::string, spdlog::level::level_enum, std::string> onMessage;

 protected:
  void sink_it_(const spdlog::details::log_msg& msg) override {
    onMessage.emit(std::string(msg.logger_name.begin(), msg.logger_name.end()), msg.level,
        std::string(msg.payload.begin(), msg.payload.end()));
  }

  void flush_() override {
  }
};

////////////////////////////////////////////////////////////////////////////////////////////////////

// Setup our sinks. The logger will print to the console and store it's messages in a file called
// cosmoscout.log.
auto signalSink = std::make_shared<SignalSink>();
auto fileSink   = std::make_shared<spdlog::sinks::basic_file_sink_mt>("cosmoscout.log", true);
auto coutSink   = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
std::vector<spdlog::sink_ptr> sinks = {signalSink, coutSink, fileSink};

////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace

////////////////////////////////////////////////////////////////////////////////////////////////////

void init() {
  spdlog::set_default_logger(createLogger("cs-utils"));

  // This logger will be used by vista.
  static auto vistaLogger = createLogger("vista");

  // Assign a custom log stream for vista's debug messages.
  static SpdlogBuffer<spdlog::level::debug> debugBuffer(vistaLogger);
  static std::ostream                       debugStream(&debugBuffer);
  vstr::SetDebugStream(&debugStream);

  // Assign a custom log stream for vista's info messages.
  static SpdlogBuffer<spdlog::level::info> infoBuffer(vistaLogger);
  static std::ostream                      infoStream(&infoBuffer);
  vstr::SetOutStream(&infoStream);

  // Assign a custom log stream for vista's warnings.
  static SpdlogBuffer<spdlog::level::warn> warnBuffer(vistaLogger);
  static std::ostream                      warnStream(&warnBuffer);
  vstr::SetWarnStream(&warnStream);

  // Assign a custom log stream for vista's errors.
  static SpdlogBuffer<spdlog::level::err> errBuffer(vistaLogger);
  static std::ostream                     errStream(&errBuffer);
  vstr::SetErrStream(&errStream);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Signal<std::string, spdlog::level::level_enum, std::string> const& onMessage() {
  return signalSink->onMessage;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<spdlog::logger> createLogger(std::string const& name) {

  // Append some ... to the name of the logger to make the output more readable.
  std::string paddedName = name + " ";
  while (paddedName.length() < 20) {
    paddedName += ".";
  }
  paddedName.back() = ' ';

  auto logger = std::make_shared<spdlog::logger>(paddedName, sinks.begin(), sinks.end());

  // See https://github.com/gabime/spdlog/wiki/3.-Custom-formatting for formatting options.
  logger->set_pattern("%^[%L] %n%$%v");
  logger->set_level(spdlog::level::trace);

  return logger;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void setFileLogLevel(spdlog::level::level_enum level) {
  fileSink->set_level(level);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void setCoutLogLevel(spdlog::level::level_enum level) {
  coutSink->set_level(level);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void setSignalLogLevel(spdlog::level::level_enum level) {
  signalSink->set_level(level);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace cs::utils::logger
