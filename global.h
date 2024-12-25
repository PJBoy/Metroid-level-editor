#pragma once

import global;
import debug;

import std;

using namespace std::literals;

#define STRINGIFY(x) #x
#define APPLY(f, x) f(x)

#define LOG_INFO __FILE__ ":" APPLY(STRINGIFY, __LINE__) " - "s

#define LOG_RETHROW \
    catch (const std::exception& e) \
    { \
        DebugFile(DebugFile::error) << LOG_INFO "Exception thrown: "s << e.what() << '\n'; \
        throw; \
    }

#define LOG_IGNORE(e) \
    DebugFile(DebugFile::info) << LOG_INFO "Ignoring exception: " << (e).what() << '\n';
