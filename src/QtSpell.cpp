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
#include "QtSpell_p.hpp"
#include "Codetable.hpp"

#include <enchant++.h>
#include <QContextMenuEvent>
#include <QDebug>
#include <QLocale>
#include <QMenu>
#include <QPlainTextEdit>
#include <QTextEdit>

static void dict_describe_cb(const char* const lang_tag,
							 const char* const /*provider_name*/,
							 const char* const /*provider_desc*/,
							 const char* const /*provider_file*/,
							 void* user_data)
{
	QList<QString>* languages = static_cast<QList<QString>*>(user_data);
	languages->append(lang_tag);
}

namespace QtSpell {

Checker::Checker(QObject* parent)
	: QObject(parent),
	  m_speller(0),
	  m_wordRegEx("^[A-Za-z0-9']+$"),
	  m_decodeCodes(false),
	  m_spellingCheckbox(false),
	  m_spellingEnabled(true)
{
	// setLanguageInternal: setLanguage is virtual and cannot be called in the constructor
	setLanguageInternal("");
}

Checker::~Checker()
{
	delete m_speller;
}

bool Checker::setLanguage(const QString &lang)
{
	if(setLanguageInternal(lang))
	{
		if(isAttached()){
			checkSpelling();
		}
		return true;
	}
	return false;
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
///////////////////////////////////////////////////////////////////////////////

QString TextCursor::nextChar(int num) const
{
	TextCursor testCursor(*this);
	if(num > 1)
		testCursor.movePosition(NextCharacter, MoveAnchor, num - 1);
	else
		testCursor.setPosition(testCursor.position());
	testCursor.movePosition(NextCharacter, KeepAnchor);
	return testCursor.selectedText();
}

QString TextCursor::prevChar(int num) const
{
	TextCursor testCursor(*this);
	if(num > 1)
		testCursor.movePosition(PreviousCharacter, MoveAnchor, num - 1);
	else
		testCursor.setPosition(testCursor.position());
	testCursor.movePosition(PreviousCharacter, KeepAnchor);
	return testCursor.selectedText();
}

void TextCursor::moveWordStart(MoveMode moveMode)
{
	movePosition(StartOfWord, moveMode);
	qDebug() << "Start: " << position() << ": " << prevChar(2) << prevChar() << "|" << nextChar();
	// If we are in front of a quote...
	if(nextChar() == "'"){
		// If the previous char is alphanumeric, move left one word, otherwise move right one char
		if(prevChar().contains(m_wordRegEx)){
			movePosition(WordLeft, moveMode);
		}else{
			movePosition(NextCharacter, moveMode);
		}
	}
	// If the previous char is a quote, and the char before that is alphanumeric, move left one word
	else if(prevChar() == "'" && prevChar(2).contains(m_wordRegEx)){
		movePosition(WordLeft, moveMode, 2); // 2: because quote counts as a word boundary
	}
}

void TextCursor::moveWordEnd(MoveMode moveMode)
{
	movePosition(EndOfWord, moveMode);
	qDebug() << "End: " << position() << ": " << prevChar() << " | " << nextChar() << "|" << nextChar(2);
	// If we are in behind of a quote...
	if(prevChar() == "'"){
		// If the next char is alphanumeric, move right one word, otherwise move left one char
		if(nextChar().contains(m_wordRegEx)){
			movePosition(WordRight, moveMode);
		}else{
			movePosition(PreviousCharacter, moveMode);
		}
	}
	// If the next char is a quote, and the char after that is alphanumeric, move right one word
	else if(nextChar() == "'" && nextChar(2).contains(m_wordRegEx)){
		movePosition(WordRight, moveMode, 2); // 2: because quote counts as a word boundary
	}
}

///////////////////////////////////////////////////////////////////////////////

TextEditChecker::TextEditChecker(QObject* parent)
	: Checker(parent)
{
	m_textEdit = 0;
	m_document = 0;
}

TextEditChecker::~TextEditChecker()
{
	setTextEdit(reinterpret_cast<TextEditProxy*>(0));
}

void TextEditChecker::setTextEdit(QTextEdit* textEdit)
{
	setTextEdit(textEdit ? new TextEditProxyT<QTextEdit>(textEdit) : reinterpret_cast<TextEditProxyT<QTextEdit>*>(0));
}

void TextEditChecker::setTextEdit(QPlainTextEdit* textEdit)
{
	setTextEdit(textEdit ? new TextEditProxyT<QPlainTextEdit>(textEdit) : reinterpret_cast<TextEditProxyT<QPlainTextEdit>*>(0));
}

void TextEditChecker::setTextEdit(TextEditProxy *textEdit)
{
	if(!textEdit && m_textEdit){
		disconnect(m_textEdit->object(), SIGNAL(destroyed()), this, SLOT(slotDetachTextEdit()));
		disconnect(m_textEdit->object(), SIGNAL(textChanged()), this, SLOT(slotCheckDocumentChanged()));
		disconnect(m_textEdit->object(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotShowContextMenu(QPoint)));
		disconnect(m_textEdit->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(slotCheckRange(int,int,int)));
		m_textEdit->setContextMenuPolicy(m_oldContextMenuPolicy);

		// Remove spelling format
		QTextCursor cursor = m_textEdit->textCursor();
		cursor.movePosition(QTextCursor::Start);
		cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
		cursor.setCharFormat(QTextCharFormat());
	}
	delete m_textEdit;
	m_document = 0;
	m_textEdit = textEdit;
	if(m_textEdit){
		m_document = m_textEdit->document();
		connect(m_textEdit->object(), SIGNAL(destroyed()), this, SLOT(slotDetachTextEdit()));
		connect(m_textEdit->object(), SIGNAL(textChanged()), this, SLOT(slotCheckDocumentChanged()));
		connect(m_textEdit->object(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotShowContextMenu(QPoint)));
		connect(m_textEdit->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(slotCheckRange(int,int,int)));
		m_oldContextMenuPolicy = m_textEdit->contextMenuPolicy();
		m_textEdit->setContextMenuPolicy(Qt::CustomContextMenu);
		checkSpelling();
	}
}

void TextEditChecker::checkSpelling(int start, int end)
{
	if(end == -1){
		QTextCursor tmpCursor(m_textEdit->textCursor());
		tmpCursor.movePosition(QTextCursor::End);
		end = tmpCursor.position();
	}

	// stop contentsChange signals from being emitted due to changed charFormats
	m_textEdit->document()->blockSignals(true);

	qDebug() << "Checking range " << start << " - " << end;

	QTextCharFormat errorFmt;
	errorFmt.setFontUnderline(true);
	errorFmt.setUnderlineColor(Qt::red);
	errorFmt.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
	QTextCharFormat defaultFmt;

	TextCursor cursor(m_textEdit->textCursor());
	cursor.setPosition(start);
	while(cursor.position() < end) {
		cursor.moveWordEnd(QTextCursor::KeepAnchor);
		QString word = cursor.selectedText();
		bool correct = checkWord(word);
		qDebug() << "Checking word:" << word << "(" << cursor.anchor() << "-" << cursor.position() << "), correct:" << correct;
		if(!correct){
			cursor.setCharFormat(errorFmt);
		}else{
			cursor.setCharFormat(defaultFmt);
		}
		// Go to next word start
		while(cursor.position() < end && !cursor.isWordChar(cursor.nextChar())){
			cursor.movePosition(QTextCursor::NextCharacter);
		}
	}

	m_textEdit->document()->blockSignals(false);
}

QString TextEditChecker::getWord(int pos, int* start, int* end) const
{
	TextCursor cursor(m_textEdit->textCursor());
	cursor.setPosition(pos);
	cursor.moveWordStart();
	cursor.moveWordEnd(QTextCursor::KeepAnchor);
	if(start)
		*start = cursor.anchor();
	if(end)
		*end = cursor.position();
	return cursor.selectedText();
}

void TextEditChecker::insertWord(int start, int end, const QString &word)
{
	QTextCursor cursor(m_textEdit->textCursor());
	cursor.setPosition(start);
	cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, end - start);
	cursor.insertText(word);
}

void TextEditChecker::slotShowContextMenu(const QPoint &pos)
{
	QPoint globalPos = m_textEdit->mapToGlobal(pos);
	QMenu* menu = m_textEdit->createStandardContextMenu();
	int wordPos = m_textEdit->cursorForPosition(pos).position();
	showContextMenu(menu, globalPos, wordPos);
}

void TextEditChecker::slotCheckDocumentChanged()
{
	if(m_document != m_textEdit->document()) {
		if(m_document){
			disconnect(m_document, SIGNAL(contentsChange(int,int,int)), this, SLOT(slotCheckRange(int,int,int)));
		}
		m_document = m_textEdit->document();
		connect(m_document, SIGNAL(contentsChange(int,int,int)), this, SLOT(slotCheckRange(int,int,int)));
	}
}

void TextEditChecker::slotDetachTextEdit()
{
	// Signals are disconnected when objects are deleted
	delete m_textEdit;
	m_textEdit = 0;
	m_document = 0;
}

void TextEditChecker::slotCheckRange(int pos, int /*removed*/, int added)
{
	// Set default format on inserted text
	QTextCursor tmp = m_textEdit->textCursor();
	tmp.setPosition(pos);
	tmp.setPosition(pos + added, QTextCursor::KeepAnchor);
	tmp.setCharFormat(QTextCharFormat());

	// We are in a word if one (or both) of the previous and next characters is a word character
	TextCursor cursor(m_textEdit->textCursor());
	if(!cursor.isInsideWord()){
		return;
	}
	cursor.setPosition(pos);
	cursor.moveWordStart();
	cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, added);
	cursor.moveWordEnd(QTextCursor::KeepAnchor);

	checkSpelling(cursor.anchor(), cursor.position());
}

} // QtSpell
