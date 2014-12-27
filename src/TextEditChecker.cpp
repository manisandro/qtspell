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

namespace QtSpell {

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
	m_undoRedoStack = 0;
	m_undoRedoInProgress = false;
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
	bool undoWasEnabled = m_undoRedoStack != 0;
	setUndoRedoEnabled(false);
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
		setUndoRedoEnabled(undoWasEnabled);
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
	cursor.beginEditBlock();
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
	cursor.endEditBlock();

	m_textEdit->document()->blockSignals(false);
}

void TextEditChecker::clearUndoRedo()
{
	if(m_undoRedoStack){
		m_undoRedoStack->clear();
	}
}

void TextEditChecker::setUndoRedoEnabled(bool enabled)
{
	if(enabled == (m_undoRedoStack != 0)){
		return;
	}
	if(!enabled){
		delete m_undoRedoStack;
		m_undoRedoStack = 0;
		emit undoAvailable(false);
		emit redoAvailable(false);
	}else{
		m_undoRedoStack = new UndoRedoStack(m_textEdit);
		connect(m_undoRedoStack, SIGNAL(undoAvailable(bool)), this, SIGNAL(undoAvailable(bool)));
		connect(m_undoRedoStack, SIGNAL(redoAvailable(bool)), this, SIGNAL(redoAvailable(bool)));
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
		bool undoWasEnabled = m_undoRedoStack != 0;
		setUndoRedoEnabled(false);
		if(m_document){
			disconnect(m_document, SIGNAL(contentsChange(int,int,int)), this, SLOT(slotCheckRange(int,int,int)));
		}
		m_document = m_textEdit->document();
		connect(m_document, SIGNAL(contentsChange(int,int,int)), this, SLOT(slotCheckRange(int,int,int)));
		setUndoRedoEnabled(undoWasEnabled);
	}
}

void TextEditChecker::slotDetachTextEdit()
{
	bool undoWasEnabled = m_undoRedoStack != 0;
	setUndoRedoEnabled(false);
	// Signals are disconnected when objects are deleted
	delete m_textEdit;
	m_textEdit = 0;
	m_document = 0;
	if(undoWasEnabled){
		// Crate dummy instance
		setUndoRedoEnabled(true);
	}
}

void TextEditChecker::slotCheckRange(int pos, int removed, int added)
{
	if(m_undoRedoStack != 0 && !m_undoRedoInProgress){
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
	c.setCharFormat(QTextCharFormat());
	checkSpelling(c.anchor(), c.position());
	c.endEditBlock();
}

void TextEditChecker::undo()
{
	if(m_undoRedoStack != 0){
		m_undoRedoInProgress = true;
		m_undoRedoStack->undo();
		m_undoRedoInProgress = false;
	}
}

void TextEditChecker::redo()
{
	if(m_undoRedoStack != 0){
		m_undoRedoInProgress = true;
		m_undoRedoStack->redo();
		m_undoRedoInProgress = false;
	}
}

} // QtSpell
