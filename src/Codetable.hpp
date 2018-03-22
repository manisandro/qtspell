/* QtSpell - Spell checking for Qt text widgets.
 * Copyright (c) 2014 Sandro Mani
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

#ifndef QTSPELL_CODETABLE_HPP
#define QTSPELL_CODETABLE_HPP

#include <QDir>
#include <QMap>
#include <QString>

class QXmlStreamReader;

namespace QtSpell {

/**
 * @brief Class for translating locale identifiers into human readable strings
 */
class Codetable
{
public:
	/**
	 * @brief Get codetable instance
	 * @return The codetable singleton
	 */
	static Codetable* instance();

	/**
	 * @brief Looks up the language and country name for the specified
	 *        language code. If no matching entries are found, language_name
	 *        and country_name will simply contain the parts of the language
	 *        code (i.e. "en" and "US").
	 * @param language_code The language locale identifier (i.e. "en_US")
	 * @param language_name The language name (i.e. "English")
	 * @param country_name The country name (i.e. "United States")
	 */
	void lookup(const QString& language_code, QString& language_name, QString& country_name, QString& extra) const;

private:
	typedef void (*parser_t)(const QXmlStreamReader&, QMap<QString, QString>&);
	QMap<QString, QString> m_languageTable;
	QMap<QString, QString> m_countryTable;

	Codetable();
	void parse(const QDir& dataDir, const QString& basename, const parser_t& parser, QMap<QString, QString>& table);
	static void parseIso3166Elements(const QXmlStreamReader& xml, QMap<QString, QString> & table);
	static void parseIso639Elements(const QXmlStreamReader& xml, QMap<QString, QString> & table);
};

} // QtSpell

#endif // QTSPELL_CODETABLE_HPP
