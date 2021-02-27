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
#include "Checker_p.hpp"
#include "Codetable.hpp"

#include <enchant++.h>
#include <QApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QMenu>
#include <QTranslator>
#include <QtDebug>

static void dict_describe_cb(const char* const lang_tag,
							 const char* const /*provider_name*/,
							 const char* const /*provider_desc*/,
							 const char* const /*provider_file*/,
							 void* user_data)
{
	QList<QString>* languages = static_cast<QList<QString>*>(user_data);
	languages->append(lang_tag);
}

static enchant::Broker* get_enchant_broker() {
#ifdef QTSPELL_ENCHANT2
    static enchant::Broker broker;
    return &broker;
#else
    return enchant::Broker::instance();
#endif
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

CheckerPrivate::CheckerPrivate()
{
}

CheckerPrivate::~CheckerPrivate()
{
}

void CheckerPrivate::init()
{
	Q_Q(Checker);

	static TranslationsInit tsInit;
	Q_UNUSED(tsInit);

	q->setLanguageInternal("");
}

bool checkLanguageInstalled(const QString &lang)
{
	return get_enchant_broker()->dict_exists(lang.toStdString());
}

Checker::Checker(QObject* parent)
	: QObject(parent)
	, d_ptr(new CheckerPrivate())
{
	d_ptr->q_ptr = this;
	d_ptr->init();
}

Checker::Checker(CheckerPrivate& dd, QObject* parent)
	: QObject(parent)
	, d_ptr(&dd)
{
	d_ptr->q_ptr = this;
	d_ptr->init();
}

Checker::~Checker()
{
	delete m_speller;
	delete d_ptr;
}

bool Checker::setLanguage(const QString &lang)
{
	bool success = setLanguageInternal(lang);
	if(isAttached()){
		checkSpelling();
	}
	return success;
}

QString Checker::getLanguage() const
{
	return m_lang;
}

bool Checker::setLanguageInternal(const QString &lang)
{
	delete m_speller;
	m_speller = nullptr;
	m_lang = lang;

	// Determine language from system locale
	if(m_lang.isEmpty()){
		m_lang = QLocale::system().name();
		if(m_lang.toLower() == "c" || m_lang.isEmpty()){
			qWarning() << "Cannot use system locale " << m_lang;
			m_lang = QString();
			return false;
		}
	}

	// Request dictionary
	try {
		m_speller = get_enchant_broker()->request_dict(m_lang.toStdString());
	} catch(enchant::Exception& e) {
		qWarning() << "Failed to load dictionary: " << e.what();
		m_lang = QString();
		return false;
	}

	return true;
}

void Checker::setDecodeLanguageCodes(bool decode)
{
	m_decodeCodes = decode;
}

bool Checker::getDecodeLanguageCodes() const
{
	return m_decodeCodes;
}

void Checker::setShowCheckSpellingCheckbox(bool show)
{
	m_spellingCheckbox = show;
}

bool Checker::getShowCheckSpellingCheckbox() const
{
	return m_spellingCheckbox;
}

bool Checker::getSpellingEnabled() const
{
	return m_spellingEnabled;
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
	try{
		return m_speller->check(word.toUtf8().data());
	}catch(const enchant::Exception&){
		return true;
	}
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
			list.append(QString::fromUtf8(suggestions[i].c_str()));
		}
	}
	return list;
}

QList<QString> Checker::getLanguageList()
{
	enchant::Broker* broker = get_enchant_broker();
	QList<QString> languages;
	broker->list_dicts(dict_describe_cb, &languages);
	std::sort(languages.begin(), languages.end());
	return languages;
}

QString Checker::decodeLanguageCode(const QString &lang)
{
	QString language, country, extra;
	Codetable::instance()->lookup(lang, language, country, extra);
	if(!country.isEmpty()){
		QString decoded = QString("%1 (%2)").arg(language, country);
		if(!extra.isEmpty()) {
			decoded += QString(" [%1]").arg(extra);
		}
		return decoded;
	}else{
		return language;
	}
}

void Checker::setSpellingEnabled(bool enabled)
{
	m_spellingEnabled = enabled;
	checkSpelling();
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
					action->setProperty("wordPos", wordPos);
					action->setProperty("suggestion", suggestions[i]);
					connect(action, &QAction::triggered, this, &Checker::slotReplaceWord);
					menu->insertAction(insertPos, action);
				}
				if(suggestions.length() > 10) {
					QMenu* moreMenu = new QMenu();
					for(int i = 10, n = suggestions.length(); i < n; ++i){
						QAction* action = new QAction(suggestions[i], moreMenu);
						action->setProperty("wordPos", wordPos);
						action->setProperty("suggestion", suggestions[i]);
						connect(action, &QAction::triggered, this, &Checker::slotReplaceWord);
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
			connect(addAction, &QAction::triggered, this, &Checker::slotAddWord);
			menu->insertAction(insertPos, addAction);

			QAction* ignoreAction = new QAction(tr("Ignore \"%1\"").arg(word), menu);
			ignoreAction->setData(wordPos);
			connect(ignoreAction, &QAction::triggered, this, &Checker::slotIgnoreWord);
			menu->insertAction(insertPos, ignoreAction);
			menu->insertSeparator(insertPos);
		}
	}
	if(m_spellingCheckbox){
		QAction* action = new QAction(tr("Check spelling"), menu);
		action->setCheckable(true);
		action->setChecked(m_spellingEnabled);
		connect(action, &QAction::toggled, this, &Checker::setSpellingEnabled);
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
			connect(action, &QAction::triggered, this, &Checker::slotSetLanguage);
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
	QAction* action = qobject_cast<QAction*>(QObject::sender());
	int wordPos = action->property("wordPos").toInt();
	int start, end;
	getWord(wordPos, &start, &end);
	insertWord(start, end, action->property("suggestion").toString());
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
