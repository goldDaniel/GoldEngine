#pragma once

#include "Core.h"

enum class MouseButton
{
    INVALID = 0,

    LEFT = 1,
    MIDDLE = 2,
    RIGHT = 3,
};

enum class KeyCode
{
	UNKNOWN = 0,

	RETURN = '\r',
	ESCAPE = '\x1B',
	BACKSPACE = '\b',
	TAB = '\t',
	SPACE = ' ',
	EXCLAIM = '!',
	QUOTEDBL = '"',
	HASH = '#',
	PERCENT = '%',
	DOLLAR = '$',
	AMPERSAND = '&',
	QUOTE = '\'',
	LEFTPAREN = '(',
	RIGHTPAREN = ')',
	ASTERISK = '*',
	PLUS = '+',
	COMMA = ',',
	MINUS = '-',
	PERIOD = '.',
	SLASH = '/',
	NUM_0 = '0',
	NUM_1 = '1',
	NUM_2 = '2',
	NUM_3 = '3',
	NUM_4 = '4',
	NUM_5 = '5',
	NUM_6 = '6',
	NUM_7 = '7',
	NUM_8 = '8',
	NUM_9 = '9',
	COLON = ':',
	SEMICOLON = ';',
	LESS = '<',
	EQUALS = '=',
	GREATER = '>',
	QUESTION = '?',
	AT = '@',

	LEFTBRACKET = '[',
	BACKSLASH = '\\',
	RIGHTBRACKET = ']',
	CARET = '^',
	UNDERSCORE = '_',
	BACKQUOTE = '`',
	a = 'a',
	b = 'b',
	c = 'c',
	d = 'd',
	e = 'e',
	f = 'f',
	g = 'g',
	h = 'h',
	i = 'i',
	j = 'j',
	k = 'k',
	l = 'l',
	m = 'm',
	n = 'n',
	o = 'o',
	p = 'p',
	q = 'q',
	r = 'r',
	s = 's',
	t = 't',
	u = 'u',
	v = 'v',
	w = 'w',
	x = 'x',
	y = 'y',
	z = 'z',
};

namespace gold
{
    class Input
    {
    public:
        Input();

        
        void Update();

        
        bool IsKeyJustPressed(KeyCode code) const;

        bool IsKeyDown(KeyCode code) const;

        bool IsButtonDown(MouseButton button) const;
        bool IsButtonJustPressed(MouseButton button) const;
        
        //relative to top-left
        const glm::vec2 GetMousePos() const;

        void SetKeyState(KeyCode code, bool state);
        void SetMouseState(MouseButton button, bool state);
        void SetMousePosition(int x, int y);

            
    private:

        int mMouseX = 0;
        int mMouseY = 0;

        std::unordered_map<MouseButton, bool> mButtonDown;
        std::unordered_map<MouseButton, bool> mButtonDownPrev;

        std::unordered_map<KeyCode, bool> mKeyDown;
        std::unordered_map<KeyCode, bool> mKeyDownPrev;
    };
}
