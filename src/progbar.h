/************************************************************************//**
 * \brief Draw progress bars for command line applications.
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
 *
 * \defgroup ProgBar progbar
 * \{
 ****************************************************************************/

#ifndef _PROGBAR_H_
#define _PROGBAR_H_

/************************************************************************//**
 * Draws the progress bar.
 *
 * \param[in] pos Position (relative to max).
 * \param[in] max Maximum position (pos) value.
 * \param[in] width Line width. Drawn bar will fill a complete line.
 * \param[in] text  Text drawn at the beginning of the line (NULL for none).
 ****************************************************************************/
void ProgBarDraw(unsigned int pos, unsigned int max, unsigned int width,
		char text[]);

#endif /*_PROGBAR_H_*/

/** \} */

