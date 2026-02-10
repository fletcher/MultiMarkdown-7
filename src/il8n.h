/**

	libMultiMarkdown7 -- Lightweight markup processor to produce HTML, LaTeX, and more.

	@file il8n.h

	@brief


	@author	Fletcher T. Penney
	@bug

**/

/*

	MIT License

	Copyright (c) 2024-2026 Fletcher T. Penney

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.

*/


#ifndef IL8N_LIBMULTIMARKDOWN7_H
#define IL8N_LIBMULTIMARKDOWN7_H


#define kNumberOfLanguages 7
#define kNumberOfStrings


static const char * il8n[kNumberOfStrings][kNumberOfLanguages] = {
	{
		"return to body",				// English
		"Regresar al texto",			// Español
		"Zum Haupttext",				// Deutsch
		"Retour au texte principal",	// Français
		"Ga terug naar tekstlichaam",	// Nederlands
		"Återgå till textkroppen",		// Svenska
		"חזור/י לגוף הטקסט",			// Hebrew - עברית
	},
	{
		"see footnote",					// English
		"Ver nota a pie de página",		// Español
		"Siehe Fußnote",				// Deutsch
		"Voir note de bas de page",		// Français
		"Zie vootnot",					// Nederlands
		"Se fotnot",					// Svenska
		"ראה/י הערה",					// Hebrew - עברית
	},
	{
		"see citation",					// English
		"Ver referencia",				// Español
		"Siehe Zitat",					// Deutsch
		"Voir citation",				// Français
		"Zie citaat",					// Nederlands
		"Se citat",						// Svenska
		"ראה/י ציטוט",					// Hebrew - עברית
	},
	{
		"see glossary",					// English
		"Ver glosario",					// Español
		"Siehe Glossar",				// Deutsch
		"Voir glossaire",				// Français
		"Zie woordenlijst",				// Nederlands
		"Se ordlista",					// Svenska
		"ראה/י מילון מונחים",			// Hebrew - עברית
	}
};


#endif
