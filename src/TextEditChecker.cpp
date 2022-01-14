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
	Q_D(TextEditChecker);
	d->setTextEdit(nullptr);
}

void TextEditChecker::setTextEdit(QTextEdit* textEdit)
{
	Q_D(TextEditChecker);
	d->setTextEdit(textEdit ? new TextEditProxyT<QTextEdit>(textEdit) : nullptr);
}

void TextEditChecker::setTextEdit(QPlainTextEdit* textEdit)
{
	Q_D(TextEditChecker);
	d->setTextEdit(textEdit ? new TextEditProxyT<QPlainTextEdit>(textEdit) : nullptr);
}

void TextEditCheckerPrivate::setTextEdit(TextEditProxy *newTextEdit)
{
	Q_Q(TextEditChecker);
	if(textEdit){
		QObject::disconnect(textEdit, &TextEditProxy::editDestroyed, q, &TextEditChecker::slotDetachTextEdit);
		QObject::disconnect(textEdit, &TextEditProxy::textChanged, q, &TextEditChecker::slotCheckDocumentChanged);
		QObject::disconnect(textEdit, &TextEditProxy::customContextMenuRequested, q, &TextEditChecker::slotShowContextMenu);
		QObject::disconnect(textEdit->document(), &QTextDocument::contentsChange, q, &TextEditChecker::slotCheckRange);
		textEdit->setContextMenuPolicy(oldContextMenuPolicy);
		textEdit->removeEventFilter(q);

		// Remove spelling format
		QTextCursor cursor = textEdit->textCursor();
		cursor.movePosition(QTextCursor::Start);
		cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
		QTextCharFormat fmt = cursor.charFormat();
		QTextCharFormat defaultFormat = QTextCharFormat();
		fmt.setFontUnderline(defaultFormat.fontUnderline());
		fmt.setUnderlineColor(defaultFormat.underlineColor());
		fmt.setUnderlineStyle(defaultFormat.underlineStyle());
		cursor.setCharFormat(fmt);
	}
	bool undoWasEnabled = undoRedoStack != nullptr;
	q->setUndoRedoEnabled(false);
	delete textEdit;
	document = nullptr;
	textEdit = newTextEdit;
	if(textEdit){
		bool wasModified = textEdit->document()->isModified();
		document = textEdit->document();
		QObject::connect(textEdit, &TextEditProxy::editDestroyed, q, &TextEditChecker::slotDetachTextEdit);
		QObject::connect(textEdit, &TextEditProxy::textChanged, q, &TextEditChecker::slotCheckDocumentChanged);
		QObject::connect(textEdit, &TextEditProxy::customContextMenuRequested, q, &TextEditChecker::slotShowContextMenu);
		QObject::connect(textEdit->document(), &QTextDocument::contentsChange, q, &TextEditChecker::slotCheckRange);
		oldContextMenuPolicy = textEdit->contextMenuPolicy();
		q->setUndoRedoEnabled(undoWasEnabled);
		textEdit->setContextMenuPolicy(Qt::CustomContextMenu);
		textEdit->installEventFilter(q);
		q->checkSpelling();
		textEdit->document()->setModified(wasModified);
        } else {
                if(undoWasEnabled){
                        // Crate dummy instance
                        q->setUndoRedoEnabled(true);
                }
	}
}

void TextEditChecker::setNoSpellingPropertyId(int propertyId)
{
	Q_D(TextEditChecker);
	d->noSpellingProperty = propertyId;
}

int TextEditChecker::noSpellingPropertyId() const
{
	Q_D(const TextEditChecker);
	return d->noSpellingProperty;
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
	Q_D(TextEditChecker);
	if(end == -1){
		QTextCursor tmpCursor(d->textEdit->textCursor());
		tmpCursor.movePosition(QTextCursor::End);
		end = tmpCursor.position();
	}

	// stop contentsChange signals from being emitted due to changed charFormats
	d->textEdit->document()->blockSignals(true);

	qDebug() << "Checking range " << start << " - " << end;

	QTextCharFormat errorFmt;
	errorFmt.setFontUnderline(true);
	errorFmt.setUnderlineColor(Qt::red);
	errorFmt.setUnderlineStyle(QTextCharFormat::WaveUnderline);
	QTextCharFormat defaultFormat = QTextCharFormat();

	TextCursor cursor(d->textEdit->textCursor());
	cursor.beginEditBlock();
	cursor.setPosition(start);
	while(cursor.position() < end) {
		cursor.moveWordEnd(QTextCursor::KeepAnchor);
		bool correct;
		QString word = cursor.selectedText();
		if(d->noSpellingPropertySet(cursor)) {
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

	d->textEdit->document()->blockSignals(false);
}

bool TextEditCheckerPrivate::noSpellingPropertySet(const QTextCursor &cursor) const
{
	if(noSpellingProperty < QTextFormat::UserProperty) {
		return false;
	}
	if(cursor.charFormat().intProperty(noSpellingProperty) == 1) {
		return true;
	}
	const QVector<QTextLayout::FormatRange>& formats = cursor.block().layout()->formats();
	int pos = cursor.positionInBlock();
	foreach(const QTextLayout::FormatRange& range, formats) {
		if(pos > range.start && pos <= range.start + range.length && range.format.intProperty(noSpellingProperty) == 1) {
			return true;
		}
	}
	return false;
}

void TextEditChecker::clearUndoRedo()
{
	Q_D(TextEditChecker);
	if(d->undoRedoStack){
		d->undoRedoStack->clear();
	}
}

void TextEditChecker::setUndoRedoEnabled(bool enabled)
{
	Q_D(TextEditChecker);
	if(enabled == (d->undoRedoStack != nullptr)){
		return;
	}
	if(!enabled){
		delete d->undoRedoStack;
		d->undoRedoStack = nullptr;
		emit undoAvailable(false);
		emit redoAvailable(false);
	}else{
		d->undoRedoStack = new UndoRedoStack(d->textEdit);
		connect(d->undoRedoStack, &QtSpell::UndoRedoStack::undoAvailable, this, &TextEditChecker::undoAvailable);
		connect(d->undoRedoStack, &QtSpell::UndoRedoStack::redoAvailable, this, &TextEditChecker::redoAvailable);
	}
}

QString TextEditChecker::getWord(int pos, int* start, int* end) const
{
	Q_D(const TextEditChecker);
	TextCursor cursor(d->textEdit->textCursor());
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
	Q_D(TextEditChecker);
	QTextCursor cursor(d->textEdit->textCursor());
	cursor.setPosition(start);
	cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, end - start);
	cursor.insertText(word);
}

void TextEditChecker::slotShowContextMenu(const QPoint &pos)
{
	Q_D(TextEditChecker);
	QPoint globalPos = d->textEdit->mapToGlobal(pos);
	QMenu* menu = d->textEdit->createStandardContextMenu();
	int wordPos = d->textEdit->cursorForPosition(pos).position();
	showContextMenu(menu, globalPos, wordPos);
}

void TextEditChecker::slotCheckDocumentChanged()
{
	Q_D(TextEditChecker);
	if(d->document != d->textEdit->document()) {
		bool undoWasEnabled = d->undoRedoStack != nullptr;
		setUndoRedoEnabled(false);
		if(d->document){
			disconnect(d->document, &QTextDocument::contentsChange, this, &TextEditChecker::slotCheckRange);
		}
		d->document = d->textEdit->document();
		connect(d->document, &QTextDocument::contentsChange, this, &TextEditChecker::slotCheckRange);
		setUndoRedoEnabled(undoWasEnabled);
	}
}

void TextEditChecker::slotDetachTextEdit()
{
	Q_D(TextEditChecker);
	bool undoWasEnabled = d->undoRedoStack != nullptr;
	setUndoRedoEnabled(false);
	delete d->textEdit;
	d->textEdit = nullptr;
	d->document = nullptr;
	if(undoWasEnabled){
		// Crate dummy instance
		setUndoRedoEnabled(true);
	}
}

void TextEditChecker::slotCheckRange(int pos, int removed, int added)
{
	Q_D(TextEditChecker);
	if(d->undoRedoStack != nullptr && !d->undoRedoInProgress){
		d->undoRedoStack->handleContentsChange(pos, removed, added);
	}

	// Qt Bug? Apparently, when contents is pasted at pos = 0, added and removed are too large by 1
	TextCursor c(d->textEdit->textCursor());
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
	Q_D(TextEditChecker);
	if(d->undoRedoStack != nullptr){
		d->undoRedoInProgress = true;
		d->undoRedoStack->undo();
		d->textEdit->ensureCursorVisible();
		d->undoRedoInProgress = false;
	}
}

void TextEditChecker::redo()
{
	Q_D(TextEditChecker);
	if(d->undoRedoStack != nullptr){
		d->undoRedoInProgress = true;
		d->undoRedoStack->redo();
		d->textEdit->ensureCursorVisible();
		d->undoRedoInProgress = false;
	}
}

bool TextEditChecker::isAttached() const
{
	Q_D(const TextEditChecker);
	return d->textEdit != 0;
}

} // QtSpell
