#ifndef PIRE_TEST_COMMON_H_INCLUDED
#define PIRE_TEST_COMMON_H_INCLUDED

#include <stdio.h>
#include <pire.h>
#include <stub/stl.h>
#include <stub/defaults.h>
#include <stub/lexical_cast.h>
#include "stub/cppunit.h"

using namespace Pire;

/*****************************************************************************
* Helpers
*****************************************************************************/

inline Pire::Fsm ParseRegexp(const char* str, const char* options = "", const Pire::Encoding** enc = 0)
{
	Pire::Lexer lexer;
	yvector<wchar32> ucs4;

	bool surround = true;
	for (; *options; ++options) {
		if (*options == 'i')
			lexer.AddFeature(Pire::Features::CaseInsensitive());
		else if (*options == 'u')
			lexer.SetEncoding(Pire::Encodings::Utf8());
		else if (*options == 'n')
			surround = false;
		else if (*options == 'a')
			lexer.AddFeature(Pire::Features::AndNotSupport());
		else
			throw std::invalid_argument("Unknown option: " + ystring(1, *options));
	}

	if (enc)
		*enc = &lexer.Encoding();

	lexer.Encoding().FromLocal(str, str + strlen(str), std::back_inserter(ucs4));
	lexer.Assign(ucs4.begin(), ucs4.end());

	Pire::Fsm fsm = lexer.Parse();
	if (surround)
		fsm.Surround();
	return fsm;
}

struct Scanners {
	Pire::Scanner fast;
	Pire::SimpleScanner simple;
	Pire::SlowScanner slow;

	Scanners(const Pire::Fsm& fsm)
		: fast(Pire::Fsm(fsm).Compile<Pire::Scanner>())
		, simple(Pire::Fsm(fsm).Compile<Pire::SimpleScanner>())
		, slow(Pire::Fsm(fsm).Compile<Pire::SlowScanner>())
	{}

	Scanners(const char* str, const char* options = "")
	{
		Pire::Fsm fsm = ParseRegexp(str, options);
		fast = Pire::Fsm(fsm).Compile<Pire::Scanner>();
		simple = Pire::Fsm(fsm).Compile<Pire::SimpleScanner>();
		slow = Pire::Fsm(fsm).Compile<Pire::SlowScanner>();
	}
};

#ifdef PIRE_DEBUG

inline ystring DbgState(const Pire::Scanner& scanner, Pire::Scanner::State state)
{
	return ToString(scanner.StateIndex(state)) + (scanner.Final(state) ? ystring(" [final]") : ystring());
}

inline ystring DbgState(const Pire::SimpleScanner& scanner, Pire::SimpleScanner::State state)
{
	return ToString(scanner.StateIndex(state)) + (scanner.Final(state) ? ystring(" [final]") : ystring());
}

inline ystring DbgState(const Pire::SlowScanner& scanner, const Pire::SlowScanner::State& state)
{
	return ystring("(") + Join(state.states.begin(), state.states.end(), ", ") + ystring(")") + (scanner.Final(state) ? ystring(" [final]") : ystring());
}

template<class Scanner>
void DbgRun(const Scanner& scanner, typename Scanner::State& state, const char* begin, const char* end)
{
	for (; begin != end; ++begin) {
		char tmp[8];
		if (*begin >= 32) {
			tmp[0] = *begin;
			tmp[1] = 0;
		} else
			snprintf(tmp, sizeof(tmp)-1, "\\%03o", (unsigned char) *begin);
		std::clog << DbgState(scanner, state) << " --[" << tmp << "]--> ";
		scanner.Next(state, (unsigned char) *begin);
		std::clog << DbgState(scanner, state) << "\n";
	}
}

#define Run DbgRun
#endif

template<class Scanner>
typename Scanner::State RunRegexp(const Scanner& scanner, const char* str)
{
	PIRE_IFDEBUG(std::clog << "--- checking against " << str << "\n");

	typename Scanner::State state;
	scanner.Initialize(state);
	scanner.Next(state, BeginMark);
	Run(scanner, state, str, str + strlen(str));
	scanner.Next(state, EndMark);
	return state;
}

template<class Scanner>
bool Matches(const Scanner& scanner, const char* str)
{
	return scanner.Final(RunRegexp(scanner, str));
}

#define SCANNER(fsm) for (Scanners m_scanners(fsm), *m_flag = &m_scanners; m_flag; m_flag = 0)
#define REGEXP(pattern) for (Scanners m_scanners(pattern), *m_flag = &m_scanners; m_flag; m_flag = 0)
#define REGEXP2(pattern,flags) for (Scanners m_scanners(pattern, flags), *m_flag = &m_scanners; m_flag; m_flag = 0)
#define ACCEPTS(str) \
	do {\
		UNIT_ASSERT(Matches(m_scanners.fast, str));\
		UNIT_ASSERT(Matches(m_scanners.simple, str));\
		UNIT_ASSERT(Matches(m_scanners.slow, str));\
	} while (false)

#define DENIES(str) \
	do {\
		UNIT_ASSERT(!Matches(m_scanners.fast, str));\
		UNIT_ASSERT(!Matches(m_scanners.simple, str));\
		UNIT_ASSERT(!Matches(m_scanners.slow, str));\
	} while (false)


#endif
