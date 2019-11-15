/**
 * Miscellanous functions for dealing with enviroment varables.
 */

#ifndef ENV_H_
#define ENV_H_

#include <exception>
#include <string>
#include <sstream>
#include <ios>

#include <string.h>

namespace env {

namespace detail {
template <typename T>
T parse_from_string(const std::string& str) {
   std::istringstream ss(str);
   T result;
   ss >> result;
   return result;
}
}

struct envvar_not_found : public std::runtime_error {
    envvar_not_found(std::string msg) : runtime_error(std::move(msg)) {}
};

/**
 * Gets the value of the given environment variable, throws an
 * exception if not found. Usually you'll want the version that
 * takes a default value.
 */
template <typename T>
T getenv_generic(const std::string &name) {
    const char *val = getenv(name.c_str());
    if (val) {
        return detail::parse_from_string<T>(val);
    } else {
        throw envvar_not_found{"env var " + name + " not found "};
    }
}

/**
 * Gets the converted value of the environment variable with the
 * given name, or returns the given default_value value if not found.
 */
template <typename T>
T getenv_generic(const std::string &name, const T& default_value) {
    try {
        return getenv_generic<T>(name);
    } catch (const envvar_not_found &) {
        return default_value;
    }
}

int getenv_int(const char *var, int def) {
    const char *val = getenv(var);
    return val ? atoi(val) : def;
}

long long getenv_longlong(const char *var, long long def) {
    const char *val = getenv(var);
    return val ? atoll(val) : def;
}



bool getenv_bool(const char *var) {
    return getenv_generic<bool>(var, false);
}

}

#endif
