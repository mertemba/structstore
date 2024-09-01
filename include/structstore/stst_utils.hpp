#ifndef STST_UTILS_HPP
#define STST_UTILS_HPP

#include <sstream>

#define try_with_info(info_stream, stmt)                                       \
  do {                                                                         \
    try {                                                                      \
      stmt                                                                     \
    } catch (std::runtime_error _e) {                                          \
      std::ostringstream _str;                                                 \
      _str << info_stream << _e.what();                                        \
      throw std::runtime_error(_str.str().c_str());                            \
    }                                                                          \
  } while (0)

#endif