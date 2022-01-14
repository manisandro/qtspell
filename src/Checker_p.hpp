/* QtSpell - Spell checking for Qt text widgets.
 * Copyright (c) 2014-2022 Sandro Mani
 * Copyright (c) 2021 Pino Toscano
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef QTSPELL_CHECKER_P_HPP
#define QTSPELL_CHECKER_P_HPP

#include <QString>

namespace enchant { class Dict; }

namespace QtSpell {

class Checker;

class CheckerPrivate
{
public:
	CheckerPrivate();
	virtual ~CheckerPrivate();

	void init();
	bool setLanguageInternal(const QString& newLang);

	Checker* q_ptr = nullptr;
	enchant::Dict* speller = nullptr;
	QString lang;
	bool decodeCodes = false;
	bool spellingCheckbox = false;
	bool spellingEnabled = true;

	Q_DECLARE_PUBLIC(Checker)
};

} // QtSpell

#endif // QTSPELL_CHECKER_P_HPP
