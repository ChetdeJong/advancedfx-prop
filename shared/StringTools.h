#pragma once

// Copyright (c) by advancedfx.org
//
// Last changes:
// 2010-05-19 by dominik.matrixstorm.com
//
// Firstchanges:
// 2010-03-17 by dominik.matrixstorm.com

#include <string>

bool WideStringToAnsiString(wchar_t const * wideChars, std::string & outAnsiString);

bool AnsiStringToWideString(char const * ansiChars, std::wstring & outWideString);


bool StringEndsWith(char const * target, char const * ending);

bool StringBeginsWith(char const * target, char const * beginning);


bool StringIsAlNum(char const * value);

bool StringIsEmpty(char const * value);

bool StringIsNull(char const * value);

/// <summary>
/// Match a string against a mask with wildcards.
/// The mask can contain \*, which means match any number of characters or \\ which means the char \.
/// <summary>
/// <param name="sz_Mask">The mask to match.</param>
/// <param name="sz_Target">The target string to match against the mask.</param>
/// <returns>Returns true if matched, false otherwise.</returns>
bool StringWildCard1Matched( const char * sz_Mask, const char * sz_Target );
