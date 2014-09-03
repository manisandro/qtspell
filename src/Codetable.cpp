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

#include "Codetable.hpp"
#include <QDir>
#include <QFile>
#include <QXmlStreamReader>
#include <libintl.h>

#define ISO_639_DOMAIN  "iso_639"
#define ISO_3166_DOMAIN "iso_3166"

namespace QtSpell {

Codetable* Codetable::instance()
{
	static Codetable codetable;
	return &codetable;
}

void Codetable::lookup(const QString &language_code, QString &language_name, QString &country_name) const
{
	QStringList parts = language_code.split("_");
	if(!parts.size() == 2) {
		language_name = language_code;
		country_name = "";
		return;
	}

	language_name = m_languageTable.contains(parts[0]) ? m_languageTable.value(parts[0]) : parts[0];
	country_name = m_countryTable.contains(parts[1]) ? m_countryTable.value(parts[1]) : parts[1];
}

Codetable::Codetable()
{
	bindtextdomain (ISO_639_DOMAIN, ISO_CODES_PREFIX "/share/locale");
	bind_textdomain_codeset (ISO_639_DOMAIN, "UTF-8");

	bindtextdomain (ISO_3166_DOMAIN, ISO_CODES_PREFIX "/share/locale");
	bind_textdomain_codeset (ISO_3166_DOMAIN, "UTF-8");

	parse("iso_639.xml", parseIso639Elements, m_languageTable);
	parse("iso_3166.xml", parseIso3166Elements, m_countryTable);
}

void Codetable::parseIso639Elements(const QXmlStreamReader &xml, QMap<QString, QString> &table)
{
	if(xml.name() == "iso_639_entry" ){
		QString name = xml.attributes().value("name").toString();
		QString code = xml.attributes().value("iso_639_1_code").toString();
		if(!name.isEmpty() && !code.isEmpty()){
			name = QString::fromUtf8(dgettext(ISO_639_DOMAIN, name.toLatin1().data()));
			table.insert(code, name);
		}
	}
}

void Codetable::parseIso3166Elements(const QXmlStreamReader &xml, QMap<QString, QString> &table)
{
	if(xml.name() == "iso_3166_entry" ){
		QString name = xml.attributes().value("name").toString();
		QString code = xml.attributes().value("alpha_2_code").toString();
		if(!name.isEmpty() && !code.isEmpty()){
			name = QString::fromUtf8(dgettext(ISO_3166_DOMAIN, name.toLatin1().data()));;
			table.insert(code, name);
		}
	}
}

void Codetable::parse(const QString& basename, const parser_t& parser, QMap<QString, QString>& table)
{
	QString filename = QDir(ISO_CODES_PREFIX "/share/xml/iso-codes").absoluteFilePath(basename);
	QFile file(filename);
	if(!file.open(QIODevice::ReadOnly)){
		qWarning("Failed to open %s for reading", file.fileName().toLatin1().data());
		return;
	}

	QXmlStreamReader xml(&file);
	while(!xml.atEnd() && !xml.hasError()){
		if(xml.readNext() == QXmlStreamReader::StartElement){
			parser(xml, table);
		}
	}
}

} // QtSpell
