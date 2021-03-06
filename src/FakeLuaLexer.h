/*
 * Copyright (C) 2019 Me and My Shadow
 *
 * This file is part of Me and My Shadow.
 *
 * Me and My Shadow is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Me and My Shadow is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Me and My Shadow.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FAKELUALEXER_H
#define FAKELUALEXER_H

#include "ITreeStorage.h"
#include <string>

// A simple and fake Lua lexer.
class FakeLuaLexer {
public:
	enum TokenType {
		EndOfFile,
		Identifier,
		Operator,
		StringLiteral,
		NumberLiteral,
		Comment,
	};

	const char* buf; // The input buffer, will be advanced after get next token
	ITreeStorageBuilder::FilePosition posStart; // The start position of the token
	ITreeStorageBuilder::FilePosition pos; // The position, will be advanced after get next token (usually the end position of the token)
	TokenType tokenType; // The token type
	std::string token; // The (unescaped) string containing the contents of the token

	std::string error; // The error message if it is not empty.

	bool storedByPOASerializer; // True if it's a string stored in a file generated by POASerializer.

	FakeLuaLexer();

	// Get next token, returns true if it succeded, false if it's EOF or an error occured.
	bool getNextToken();

private:
	bool parseNumber();
	bool parseShortString(char delim);
	bool parseLongString(int level);
	bool parseComment();
	int checkOpeningLongBracket(int offset = 0);
	int checkClosingLongBracket(int offset = 0);
	int checkDigit(bool isHex, int offset = 0);
	void advanceByCharacter(char c);
};

#endif

