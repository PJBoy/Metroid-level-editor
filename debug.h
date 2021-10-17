#pragma once

import debug_m;

import std.core;
import std.filesystem;
import std.memory;
import std.regex;
import std.threading;

using namespace std::literals;

// `APPLY` and `STRINGIFY` are defined in global.h
#define LOG_INFO __FILE__ ":" APPLY(STRINGIFY, __LINE__) " - "s

#define LOG_RETHROW \
    catch (const std::exception& e) \
    { \
        DebugFile(DebugFile::error) << LOG_INFO "Exception thrown: "s << e.what() << '\n'; \
        throw; \
    }

#define LOG_IGNORE(e) \
    DebugFile(DebugFile::info) << LOG_INFO "Ignoring exception: " << (e).what() << '\n';
