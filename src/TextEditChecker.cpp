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
#include "TextEditChecker_p.hpp"
#include "UndoRedoStack.hpp"

#include <QDebug>
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QTextBlock>

namespace QtSpell {

TextEditCheckerPrivate::TextEditCheckerPrivate()
	: CheckerPrivate()
{
}

TextEditCheckerPrivate::~TextEditCheckerPrivate()
{
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
	: Checker(*new TextEditCheckerPrivate(), parent)
{
}

TextEditChecker::~TextEditChecker()
{
	setTextEdit(static_cast<TextEditProxy*>(nullptr));
}

void TextEditChecker::setTextEdit(QTextEdit* textEdit)
{
	setTextEdit(textEdit ? new TextEditProxyT<QTextEdit>(textEdit) : static_cast<TextEditProxyT<QTextEdit>*>(nullptr));
}

void TextEditChecker::setTextEdit(QPlainTextEdit* textEdit)
{
	setTextEdit(textEdit ? new TextEditProxyT<QPlainTextEdit>(textEdit) : static_cast<TextEditProxyT<QPlainTextEdit>*>(nullptr));
}

void TextEditChecker::setTextEdit(TextEditProxy *textEdit)
{
	if(m_textEdit){
		disconnect(m_textEdit, &TextEditProxy::editDestroyed, this, &TextEditChecker::slotDetachTextEdit);
		disconnect(m_textEdit, &TextEditProxy::textChanged, this, &TextEditChecker::slotCheckDocumentChanged);
		disconnect(m_textEdit, &TextEditProxy::customContextMenuRequested, this, &TextEditChecker::slotShowContextMenu);
		disconnect(m_textEdit->document(), &QTextDocument::contentsChange, this, &TextEditChecker::slotCheckRange);
		m_textEdit->setContextMenuPolicy(m_oldContextMenuPolicy);
		m_textEdit->removeEventFilter(this);

		// Remove spelling format
		QTextCursor cursor = m_textEdit->textCursor();
		cursor.movePosition(QTextCursor::Start);
		cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
		QTextCharFormat fmt = cursor.charFormat();
		QTextCharFormat defaultFormat = QTextCharFormat();
		fmt.setFontUnderline(defaultFormat.fontUnderline());
		fmt.setUnderlineColor(defaultFormat.underlineColor());
		fmt.setUnderlineStyle(defaultFormat.underlineStyle());
		cursor.setCharFormat(fmt);
	}
	bool undoWasEnabled = m_undoRedoStack != nullptr;
	setUndoRedoEnabled(false);
	delete m_textEdit;
	m_document = nullptr;
	m_textEdit = textEdit;
	if(m_textEdit){
		m_document = m_textEdit->document();
		connect(m_textEdit, &TextEditProxy::editDestroyed, this, &TextEditChecker::slotDetachTextEdit);
		connect(m_textEdit, &TextEditProxy::textChanged, this, &TextEditChecker::slotCheckDocumentChanged);
		connect(m_textEdit, &TextEditProxy::customContextMenuRequested, this, &TextEditChecker::slotShowContextMenu);
		connect(m_textEdit->document(), &QTextDocument::contentsChange, this, &TextEditChecker::slotCheckRange);
		m_oldContextMenuPolicy = m_textEdit->contextMenuPolicy();
		setUndoRedoEnabled(undoWasEnabled);
		m_textEdit->setContextMenuPolicy(Qt::CustomContextMenu);
		m_textEdit->installEventFilter(this);
		checkSpelling();
	}
}

void TextEditChecker::setNoSpellingPropertyId(int propertyId)
{
	m_noSpellingProperty = propertyId;
}

int TextEditChecker::noSpellingPropertyId() const
{
	return m_noSpellingProperty;
}

bool TextEditChecker::eventFilter(QObject* obj, QEvent* event)
{
	if(event->type() == QEvent::KeyPress){
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
		if(keyEvent->key() == Qt::Key_Z && keyEvent->modifiers() == Qt::CTRL){
			undo();
			return true;
		}else if(keyEvent->key() == Qt::Key_Z && keyEvent->modifiers() == (Qt::CTRL | Qt::SHIFT)){
			redo();
			return true;
		}
	}
	return QObject::eventFilter(obj, event);
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
	errorFmt.setUnderlineStyle(QTextCharFormat::WaveUnderline);
	QTextCharFormat defaultFormat = QTextCharFormat();

	TextCursor cursor(m_textEdit->textCursor());
	cursor.beginEditBlock();
	cursor.setPosition(start);
	while(cursor.position() < end) {
		cursor.moveWordEnd(QTextCursor::KeepAnchor);
		bool correct;
		QString word = cursor.selectedText();
		if(noSpellingPropertySet(cursor)) {
			correct = true;
			qDebug() << "Skipping word:" << word << "(" << cursor.anchor() << "-" << cursor.position() << ")";
		} else {
			correct = checkWord(word);
			qDebug() << "Checking word:" << word << "(" << cursor.anchor() << "-" << cursor.position() << "), correct:" << correct;
		}
		if(!correct){
			cursor.mergeCharFormat(errorFmt);
		}else{
			QTextCharFormat fmt = cursor.charFormat();
			fmt.setFontUnderline(defaultFormat.fontUnderline());
			fmt.setUnderlineColor(defaultFormat.underlineColor());
			fmt.setUnderlineStyle(defaultFormat.underlineStyle());
			cursor.setCharFormat(fmt);
		}
		// Go to next word start
		while(cursor.position() < end && !cursor.isWordChar(cursor.nextChar())){
			cursor.movePosition(QTextCursor::NextCharacter);
		}
	}
	cursor.endEditBlock();

	m_textEdit->document()->blockSignals(false);
}

bool TextEditChecker::noSpellingPropertySet(const QTextCursor &cursor) const
{
	if(m_noSpellingProperty < QTextFormat::UserProperty) {
		return false;
	}
	if(cursor.charFormat().intProperty(m_noSpellingProperty) == 1) {
		return true;
	}
	const QVector<QTextLayout::FormatRange>& formats = cursor.block().layout()->formats();
	int pos = cursor.positionInBlock();
	foreach(const QTextLayout::FormatRange& range, formats) {
		if(pos > range.start && pos <= range.start + range.length && range.format.intProperty(m_noSpellingProperty) == 1) {
			return true;
		}
	}
	return false;
}

void TextEditChecker::clearUndoRedo()
{
	if(m_undoRedoStack){
		m_undoRedoStack->clear();
	}
}

void TextEditChecker::setUndoRedoEnabled(bool enabled)
{
	if(enabled == (m_undoRedoStack != nullptr)){
		return;
	}
	if(!enabled){
		delete m_undoRedoStack;
		m_undoRedoStack = nullptr;
		emit undoAvailable(false);
		emit redoAvailable(false);
	}else{
		m_undoRedoStack = new UndoRedoStack(m_textEdit);
		connect(m_undoRedoStack, &QtSpell::UndoRedoStack::undoAvailable, this, &TextEditChecker::undoAvailable);
		connect(m_undoRedoStack, &QtSpell::UndoRedoStack::redoAvailable, this, &TextEditChecker::redoAvailable);
	}
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
		bool undoWasEnabled = m_undoRedoStack != nullptr;
		setUndoRedoEnabled(false);
		if(m_document){
			disconnect(m_document, &QTextDocument::contentsChange, this, &TextEditChecker::slotCheckRange);
		}
		m_document = m_textEdit->document();
		connect(m_document, &QTextDocument::contentsChange, this, &TextEditChecker::slotCheckRange);
		setUndoRedoEnabled(undoWasEnabled);
	}
}

void TextEditChecker::slotDetachTextEdit()
{
	bool undoWasEnabled = m_undoRedoStack != nullptr;
	setUndoRedoEnabled(false);
	delete m_textEdit;
	m_textEdit = nullptr;
	m_document = nullptr;
	if(undoWasEnabled){
		// Crate dummy instance
		setUndoRedoEnabled(true);
	}
}

void TextEditChecker::slotCheckRange(int pos, int removed, int added)
{
	if(m_undoRedoStack != nullptr && !m_undoRedoInProgress){
		m_undoRedoStack->handleContentsChange(pos, removed, added);
	}

	// Qt Bug? Apparently, when contents is pasted at pos = 0, added and removed are too large by 1
	TextCursor c(m_textEdit->textCursor());
	c.movePosition(QTextCursor::End);
	int len = c.position();
	if(pos == 0 && added > len){
		--added;
	}

	// Set default format on inserted text
	c.beginEditBlock();
	c.setPosition(pos);
	c.moveWordStart();
	c.setPosition(pos + added, QTextCursor::KeepAnchor);
	c.moveWordEnd(QTextCursor::KeepAnchor);
	QTextCharFormat fmt = c.charFormat();
	QTextCharFormat defaultFormat = QTextCharFormat();
	fmt.setFontUnderline(defaultFormat.fontUnderline());
	fmt.setUnderlineColor(defaultFormat.underlineColor());
	fmt.setUnderlineStyle(defaultFormat.underlineStyle());
	c.setCharFormat(fmt);
	checkSpelling(c.anchor(), c.position());
	c.endEditBlock();
}

void TextEditChecker::undo()
{
	if(m_undoRedoStack != nullptr){
		m_undoRedoInProgress = true;
		m_undoRedoStack->undo();
		m_textEdit->ensureCursorVisible();
		m_undoRedoInProgress = false;
	}
}

void TextEditChecker::redo()
{
	if(m_undoRedoStack != nullptr){
		m_undoRedoInProgress = true;
		m_undoRedoStack->redo();
		m_textEdit->ensureCursorVisible();
		m_undoRedoInProgress = false;
	}
}

bool TextEditChecker::isAttached() const
{
	return m_textEdit != 0;
}

} // QtSpell
