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

#include "QtSpell.hpp"
#include "Codetable.hpp"

#include <enchant++.h>
#include <QApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QMenu>
#include <QTranslator>

static void dict_describe_cb(const char* const lang_tag,
							 const char* const /*provider_name*/,
							 const char* const /*provider_desc*/,
							 const char* const /*provider_file*/,
							 void* user_data)
{
	QList<QString>* languages = static_cast<QList<QString>*>(user_data);
	languages->append(lang_tag);
}


class TranslationsInit {
public:
	TranslationsInit(){
		QString translationsPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
#ifdef Q_OS_WIN
		QDir packageDir = QDir(QString("%1/../").arg(QApplication::applicationDirPath()));
		translationsPath = packageDir.absolutePath() + translationsPath.mid(QLibraryInfo::location(QLibraryInfo::PrefixPath).length());
#endif
		spellTranslator.load("QtSpell_" + QLocale::system().name(), translationsPath);
		QApplication::instance()->installTranslator(&spellTranslator);
	}
private:
	QTranslator spellTranslator;
};


namespace QtSpell {

Checker::Checker(QObject* parent)
	: QObject(parent),
	  m_speller(0),
	  m_decodeCodes(false),
	  m_spellingCheckbox(false),
	  m_spellingEnabled(true),
	  m_wordRegEx("^[A-Za-z0-9']+$")
{
	static TranslationsInit tsInit;
	Q_UNUSED(tsInit);

	// setLanguageInternal: setLanguage is virtual and cannot be called in the constructor
	setLanguageInternal("");
}

Checker::~Checker()
{
	delete m_speller;
}

bool Checker::setLanguage(const QString &lang)
{
	bool success = setLanguageInternal(lang);
	if(isAttached()){
		checkSpelling();
	}
	return success;
}

bool Checker::setLanguageInternal(const QString &lang)
{
	delete m_speller;
	m_speller = 0;
	m_lang = lang;

	// Determine language from system locale
	if(m_lang.isEmpty()){
		m_lang = QLocale::system().name();
		if(m_lang.toLower() == "c" || m_lang.isEmpty()){
			qWarning("Cannot use system locale %s", m_lang.toLatin1().data());
			m_lang = QString::null;
			return false;
		}
	}

	// Request dictionary
	try {
		m_speller = enchant::Broker::instance()->request_dict(m_lang.toStdString());
	} catch(enchant::Exception& e) {
		qWarning("Failed to load dictionary: %s", e.what());
		m_lang = QString::null;
		return false;
	}

	return true;
}

void Checker::addWordToDictionary(const QString &word)
{
	if(m_speller){
		m_speller->add(word.toUtf8().data());
	}
}

bool Checker::checkWord(const QString &word) const
{
	if(!m_speller || !m_spellingEnabled){
		return true;
	}
	// Skip empty strings and single characters
	if(word.length() < 2){
		return true;
	}
	// Don't check non-word blocks
	if(!word.contains(m_wordRegEx)){
		return true;
	}
	return m_speller->check(word.toUtf8().data());
}

void Checker::ignoreWord(const QString &word) const
{
	m_speller->add_to_session(word.toUtf8().data());
}

QList<QString> Checker::getSpellingSuggestions(const QString& word) const
{
	QList<QString> list;
	if(m_speller){
		std::vector<std::string> suggestions;
		m_speller->suggest(word.toUtf8().data(), suggestions);
		for(std::size_t i = 0, n = suggestions.size(); i < n; ++i){
			list.append(QString::fromStdString(suggestions[i]));
		}
	}
	return list;
}

QList<QString> Checker::getLanguageList()
{
	enchant::Broker* broker = enchant::Broker::instance();
	QList<QString> languages;
	broker->list_dicts(dict_describe_cb, &languages);
	qSort(languages);
	return languages;
}

QString Checker::decodeLanguageCode(const QString &lang)
{
	QString language, country;
	Codetable::instance()->lookup(lang, language, country);
	if(!country.isEmpty()){
		return QString("%1 (%2)").arg(language, country);
	}else{
		return language;
	}
}

void Checker::showContextMenu(QMenu* menu, const QPoint& pos, int wordPos)
{
	QAction* insertPos = menu->actions().first();
	if(m_speller && m_spellingEnabled){
		QString word = getWord(wordPos);

		if(!checkWord(word)) {
			QList<QString> suggestions = getSpellingSuggestions(word);
			if(!suggestions.isEmpty()){
				for(int i = 0, n = qMin(10, suggestions.length()); i < n; ++i){
					QAction* action = new QAction(suggestions[i], menu);
					action->setData(wordPos);
					connect(action, SIGNAL(triggered()), this, SLOT(slotReplaceWord()));
					menu->insertAction(insertPos, action);
				}
				if(suggestions.length() > 10) {
					QMenu* moreMenu = new QMenu();
					for(int i = 10, n = suggestions.length(); i < n; ++i){
						QAction* action = new QAction(suggestions[i], moreMenu);
						action->setData(wordPos);
						connect(action, SIGNAL(triggered()), this, SLOT(slotReplaceWord()));
						moreMenu->addAction(action);
					}
					QAction* action = new QAction(tr("More..."), menu);
					menu->insertAction(insertPos, action);
					action->setMenu(moreMenu);
				}
				menu->insertSeparator(insertPos);
			}

			QAction* addAction = new QAction(tr("Add \"%1\" to dictionary").arg(word), menu);
			addAction->setData(wordPos);
			connect(addAction, SIGNAL(triggered()), this, SLOT(slotAddWord()));
			menu->insertAction(insertPos, addAction);

			QAction* ignoreAction = new QAction(tr("Ignore \"%1\"").arg(word), menu);
			ignoreAction->setData(wordPos);
			connect(ignoreAction, SIGNAL(triggered()), this, SLOT(slotIgnoreWord()));
			menu->insertAction(insertPos, ignoreAction);
			menu->insertSeparator(insertPos);
		}
	}
	if(m_spellingCheckbox){
		QAction* action = new QAction(tr("Check spelling"), menu);
		action->setCheckable(true);
		action->setChecked(m_spellingEnabled);
		connect(action, SIGNAL(toggled(bool)), this, SLOT(setSpellingEnabled(bool)));
		menu->insertAction(insertPos, action);
	}
	if(m_speller && m_spellingEnabled){
		QMenu* languagesMenu = new QMenu();
		QActionGroup* actionGroup = new QActionGroup(languagesMenu);
		foreach(const QString& lang, getLanguageList()){
			QString text = getDecodeLanguageCodes() ? decodeLanguageCode(lang) : lang;
			QAction* action = new QAction(text, languagesMenu);
			action->setData(lang);
			action->setCheckable(true);
			action->setChecked(lang == getLanguage());
			connect(action, SIGNAL(triggered(bool)), this, SLOT(slotSetLanguage(bool)));
			languagesMenu->addAction(action);
			actionGroup->addAction(action);
		}
		QAction* langsAction = new QAction(tr("Languages"), menu);
		langsAction->setMenu(languagesMenu);
		menu->insertAction(insertPos, langsAction);
		menu->insertSeparator(insertPos);
	}

	menu->exec(pos);
	delete menu;
}

void Checker::slotAddWord()
{
	int wordPos = qobject_cast<QAction*>(QObject::sender())->data().toInt();
	int start, end;
	addWordToDictionary(getWord(wordPos, &start, &end));
	checkSpelling(start, end);
}

void Checker::slotIgnoreWord()
{
	int wordPos = qobject_cast<QAction*>(QObject::sender())->data().toInt();
	int start, end;
	ignoreWord(getWord(wordPos, &start, &end));
	checkSpelling(start, end);
}

void Checker::slotReplaceWord()
{
	int wordPos = qobject_cast<QAction*>(QObject::sender())->data().toInt();
	int start, end;
	getWord(wordPos, &start, &end);
	insertWord(start, end, qobject_cast<QAction*>(QObject::sender())->text());
}

void Checker::slotSetLanguage(bool checked)
{
	if(checked) {
		QAction* action = qobject_cast<QAction*>(QObject::sender());
		QString lang = action->data().toString();
		if(!setLanguage(lang)){
			action->setChecked(false);
			lang = "";
		}
		emit languageChanged(lang);
	}
}

} // QtSpell
