#pragma once

#if BUILD_WITH_EASY_PROFILER && BUILD_WITH_OPTICK
#	error Cannot enable both profilers at once. Just pick one.
#endif // BUILD_WITH_EASY_PROFILER && BUILD_WITH_OPTICK

#if !BUILD_WITH_EASY_PROFILER && !BUILD_WITH_OPTICK
#	define EASY_FUNCTION(...)
#	define EASY_BLOCK(...)
#	define EASY_END_BLOCK
#	define EASY_THREAD_SCOPE(...)
#	define EASY_PROFILER_ENABLE
#	define EASY_MAIN_THREAD
#	define PROFILER_FRAME(...)
#	define PROFILER_DUMP(fileName)
#endif // !BUILD_WITH_EASY_PROFILER && !BUILD_WITH_OPTICK

#if BUILD_WITH_EASY_PROFILER
#	include "easy/profiler.h"

#define PROFILER_FUNCTION(...) EASY_FUNCTION(__VA_ARGS__)
#define PROFILER_BLOCK(name, ...) EASY_BLOCK(name, __VA_ARGS__)
#define PROFILER_END_BLOCK }; EASY_END_BLOCK
#define PROFILER_THREAD_SCOPE(...) EASY_THREAD_SCOPE(__VA_ARGS__)

#define PROFILER_START() EASY_PROFILER_ENABLE
#define PROFILER_MAIN_THREAD() EASY_THREAD("MainThread")
#define PROFILER_THREAD(name) EASY_THREAD(name)
#define PROFILER_FRAME(...)
#define PROFILER_DUMP(fileName) profiler::dumpBlocksToFile(fileName);

#define PROFILER_CATEGORY(text, type) EASY_BLOCK(text)

#endif // BUILD_WITH_EASY_PROFILER


#if BUILD_WITH_OPTICK

#include "optick.h"
#define PROFILER_FUNCTION(...) OPTICK_EVENT()
#define PROFILER_BLOCK(name, ...) { OptickScopeWrapper Wrapper(name);
#define PROFILER_END_BLOCK };
#define PROFILER_THREAD_SCOPE(...) OPTICK_START_THREAD(__VA_ARGS__)
#define PROFILER_START() OPTICK_START_CAPTURE()
#define PROFILER_MAIN_THREAD() OPTICK_THREAD( "MainThread" )
#define PROFILER_THREAD(name) OPTICK_THREAD( name )
#define PROFILER_FRAME(name) OPTICK_FRAME(name)
#define PROFILER_DUMP(fileName) OPTICK_STOP_CAPTURE(); OPTICK_SAVE_CAPTURE(fileName, true);
#define PROFILER_CATEGORY(text, type) OPTICK_CATEGORY(text, Optick::Category:: type)

namespace profiler
{
	namespace colors
	{
		const int32_t Magenta = Optick::Color::Magenta;
		const int32_t Green = Optick::Color::Green;
		const int32_t Red = Optick::Color::Red;
	} // namespace colors
} // namespace profiler

class OptickScopeWrapper
{
public:
	OptickScopeWrapper( const char* name )
	{
		OPTICK_PUSH( name );
	}
	~OptickScopeWrapper()
	{
		OPTICK_POP();
	}
};
#endif // BUILD_WITH_OPTICK
