/************************************************************************//**
 * ProgBar: Draw progress bars for command line applications.
 *
 * Drawn progress bar has the following appearance:
 * <Some text> [========>        ] 50%
 * Initial text is optional. The bar is auto adjusted to the specified line
 * width. This function must be called for each bar iteration.
 *
 * \note It is recommended to hide the cursor (e.g. calling curs_set(0) if
 *       using ncurses) when using this module.
 * 
 * \author Jesus Alonso (doragasu)
 * \date   2015
 ****************************************************************************/
#include "progbar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/************************************************************************//**
 * Draws the progress bar.
 *
 * \param[in] pos Position (relative to max).
 * \param[in] max Maximum position (pos) value.
 * \param[in] width Line width. Drawn bar will fill a complete line.
 * \param[in] text  Text drawn at the beginning of the line (NULL for none).
 ****************************************************************************/
void ProgBarDraw(unsigned int pos, unsigned int max, unsigned int width,
		char text[]) {

	size_t textLen = 0;
	unsigned int progChars = 0;
	unsigned int barWidth;
	unsigned char progPercent = 0;
	char *t = text;
	unsigned int i = 0;

	// Minimum drawable bar takes 8 characters plus text length ([==]100%).
	// Text length is the length of the text string plus 1 (a space). Also
	// a space if left at the end of the bar to avoid cursor jumping to the
	// next line.
	// Obtain text length. Cut if necessary
	if (text) {
		textLen = strlen(text);
		// If cutting, copy text to avoid modifying the original parameter
		if (textLen >= (width - 9)) {
			t = (char*) malloc(textLen + 1);
			memcpy(t, text, textLen);
			t[textLen] = '\0';
		}
	}

	// Obtain progress in percent and chars forms
	barWidth = width - 6;
	barWidth = textLen?barWidth - textLen - 1:barWidth;
	pos = pos > max?max:pos;	// Ensure pos <= max.
	progChars = barWidth * pos / max;
	progPercent = 100 * pos / max;

	// Jump to the beginning of the line
	printf("\r");
	// Draw text (if any)
	if (textLen) printf("%s ", t);
	// Draw start of the bar
	putchar('[');
	// Draw progress
	if (progChars) {
		for (; i < (progChars - 1); i++) putchar('=');
		// Unless progress is 100%, print '>' head
		if (progChars < barWidth) putchar('>');
		else putchar('=');
		i++;
	}
	// Fill line with blanks
	for (; i < barWidth; i++) putchar(' ');
	// Print tail with completion percent
	printf("]%3d%%", progPercent);
	fflush(stdout);

	// If we copied the text, free memory
	if (text && (t != text)) free(t);
}
