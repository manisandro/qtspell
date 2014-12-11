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

#include "UndoRedoStack.hpp"
#include "TextEditChecker_p.hpp"
#include <QTextDocument>

namespace QtSpell {

struct UndoRedoStack::Action {
	virtual ~Action(){}
};

struct UndoRedoStack::UndoableInsert : public UndoRedoStack::Action {
	QString text;
	int pos;
	bool isWhitespace;
	bool isMergeable;

	UndoableInsert(int _pos, const QString& _text){
		pos = _pos;
		text = _text;
		isWhitespace = text.length() == 1 && text[0].isSpace();
		isMergeable = (text.length() == 1);
	}
};

struct UndoRedoStack::UndoableDelete : public UndoRedoStack::Action {
	QString text;
	int start, end;
	bool deleteKeyUsed;
	bool isWhitespace;
	bool isMergeable;

	UndoableDelete(int _start, int _end, const QString& _text, bool _deleteKeyUsed){
		start = _start;
		end = _end;
		text = _text;
		deleteKeyUsed = _deleteKeyUsed;
		isWhitespace = text.length() == 1 && text[0].isSpace();
		isMergeable = (text.length() == 1);
	}
};

UndoRedoStack::UndoRedoStack(TextEditProxy* textEdit)
	: m_actionInProgress(false), m_textEdit(textEdit)
{
	// We need to keep undo/redo enabled to retreive the deleted text in onContentsChange...
	m_textEdit->document()->setUndoRedoEnabled(true);
}

void UndoRedoStack::clear()
{
	qDeleteAll(m_undoStack);
	qDeleteAll(m_redoStack);
	m_undoStack.clear();
	m_redoStack.clear();
	emit undoAvailable(false);
	emit redoAvailable(false);
}

void UndoRedoStack::handleContentsChange(int pos, int removed, int added)
{
	if(m_actionInProgress || (added == 0 && removed == 0)){
		return;
	}
	qDeleteAll(m_redoStack);
	m_redoStack.clear();
	if(removed > 0){
		m_textEdit->document()->undo();
		QTextCursor c(m_textEdit->textCursor());
		bool deleteWasUsed = (c.anchor() == c.position() && c.position() == pos);
		c.setPosition(pos);
		c.setPosition(pos + removed, QTextCursor::KeepAnchor);
		UndoableDelete* undoAction = new UndoableDelete(pos, pos + removed, c.selectedText(), deleteWasUsed);
		m_textEdit->document()->redo();
		if(m_undoStack.empty() || !dynamic_cast<UndoableDelete*>(m_undoStack.top())){
			m_undoStack.push(undoAction);
		}else{
			UndoableDelete* prevDelete = static_cast<UndoableDelete*>(m_undoStack.top());
			if(deleteMergeable(prevDelete, undoAction)){
				if(prevDelete->start == undoAction->start){ // Delete key used
					prevDelete->text += undoAction->text;
					prevDelete->end += (undoAction->end - undoAction->start);
				}else{ // Backspace used
					prevDelete->text = undoAction->text + prevDelete->text;
					prevDelete->start = undoAction->start;
				}
			}else{
				m_undoStack.push(undoAction);
			}
		}
	}
	if(added > 0){
		QTextCursor c(m_textEdit->textCursor());
		c.setPosition(pos);
		c.setPosition(pos + added, QTextCursor::KeepAnchor);
		UndoableInsert* undoAction = new UndoableInsert(pos, c.selectedText());
		if(m_undoStack.empty() || !dynamic_cast<UndoableInsert*>(m_undoStack.top())){
			m_undoStack.push(undoAction);
		}else{
			UndoableInsert* prevInsert = static_cast<UndoableInsert*>(m_undoStack.top());
			if(insertMergeable(prevInsert, undoAction)){
				prevInsert->text += undoAction->text;
			}else{
				m_undoStack.push(undoAction);
			}
		}
	}
	// We are only interested in the previous step for delete, no point in storing the rest
	if(added > 0 || removed > 0){
		m_textEdit->document()->clearUndoRedoStacks();
	}
	emit redoAvailable(false);
	emit undoAvailable(true);
}

void UndoRedoStack::undo()
{
	if(m_undoStack.empty()){
		return;
	}
	m_actionInProgress = true;
	Action* undoAction = m_undoStack.pop();
	m_redoStack.push(undoAction);
	QTextCursor c(m_textEdit->textCursor());
	if(dynamic_cast<UndoableInsert*>(undoAction)){
		UndoableInsert* insertAction = static_cast<UndoableInsert*>(undoAction);
		c.setPosition(insertAction->pos);
		c.setPosition(insertAction->pos + insertAction->text.length(), QTextCursor::KeepAnchor);
		c.removeSelectedText();
		if(!m_undoStack.empty() && isReplace(dynamic_cast<UndoableDelete*>(m_undoStack.top()), insertAction)){
			undo();
		}
	}else{
		UndoableDelete* deleteAction = static_cast<UndoableDelete*>(undoAction);
		c.setPosition(deleteAction->start);
		c.insertText(deleteAction->text);
		if(deleteAction->deleteKeyUsed){
			c.setPosition(deleteAction->start);
		}
	}
	m_textEdit->setTextCursor(c);
	emit undoAvailable(!m_undoStack.empty());
	emit redoAvailable(!m_redoStack.empty());
	m_actionInProgress = false;
}

void UndoRedoStack::redo()
{
	if(m_redoStack.empty()){
		return;
	}
	m_actionInProgress = true;
	Action* redoAction = m_redoStack.top();
	m_redoStack.pop();
	m_undoStack.push(redoAction);
	QTextCursor c(m_textEdit->textCursor());
	if(dynamic_cast<UndoableInsert*>(redoAction)){
		UndoableInsert* insertAction = static_cast<UndoableInsert*>(redoAction);
		c.setPosition(insertAction->pos);
		c.insertText(insertAction->text);
	}else{
		UndoableDelete* deleteAction = static_cast<UndoableDelete*>(redoAction);
		c.setPosition(deleteAction->start);
		c.setPosition(deleteAction->end, QTextCursor::KeepAnchor);
		c.removeSelectedText();
		if(!m_redoStack.empty() && isReplace(deleteAction, dynamic_cast<UndoableInsert*>(m_redoStack.top()))){
			redo();
		}
	}
	m_textEdit->setTextCursor(c);
	emit undoAvailable(!m_undoStack.empty());
	emit redoAvailable(!m_redoStack.empty());
	m_actionInProgress = false;
}

bool UndoRedoStack::insertMergeable(const UndoableInsert* prev, const UndoableInsert* cur) const
{
	return (cur->pos == prev->pos + prev->text.length()) &&
		   (cur->isWhitespace == prev->isWhitespace) &&
		   (cur->isMergeable && prev->isMergeable);
}

bool UndoRedoStack::deleteMergeable(const UndoableDelete* prev, const UndoableDelete* cur) const
{
	return (prev->deleteKeyUsed == cur->deleteKeyUsed) &&
		   (cur->isWhitespace == prev->isWhitespace) &&
		   (cur->isMergeable && prev->isMergeable) &&
		   (prev->start == cur->start || prev->start == cur->end);
}

bool UndoRedoStack::isReplace(const UndoableDelete* del, const UndoableInsert* ins) const
{
	return del && ins && del->start == ins->pos;
}


} // QtSpell
