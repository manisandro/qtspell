/* QtSpell - Spell checking for Qt text widgets.
 * Copyright (c) 2014-2022 Sandro Mani
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

#ifndef QTSPELL_UNDOREDOSTACK_HPP
#define QTSPELL_UNDOREDOSTACK_HPP

#include <QObject>
#include <QStack>


namespace QtSpell {

class TextEditProxy;

class UndoRedoStack : public QObject
{
	Q_OBJECT
public:
	UndoRedoStack(TextEditProxy* textEdit);
	void handleContentsChange(int pos, int removed, int added);
	void clear();

public slots:
	void undo();
	void redo();

signals:
	void undoAvailable(bool);
	void redoAvailable(bool);

private:
	struct Action;
	struct UndoableInsert;
	struct UndoableDelete;

	bool m_actionInProgress = false;
	TextEditProxy* m_textEdit = nullptr;
	QStack<Action*> m_undoStack;
	QStack<Action*> m_redoStack;

	bool insertMergeable(const UndoableInsert* prev, const UndoableInsert* cur) const;
	bool deleteMergeable(const UndoableDelete* prev, const UndoableDelete* cur) const;
};

} // QtSpell

#endif // QTSPELL_UNDOREDOSTACK_HPP
