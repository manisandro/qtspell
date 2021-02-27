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

#ifndef QTSPELL_TEXTEDITCHECKER_P_HPP
#define QTSPELL_TEXTEDITCHECKER_P_HPP

#include "QtSpell.hpp"
#include "Checker_p.hpp"

#include <QTextCursor>

class QMenu;

namespace QtSpell {

class TextEditChecker;

class TextEditCheckerPrivate : public CheckerPrivate
{
public:
	TextEditCheckerPrivate();
	virtual ~TextEditCheckerPrivate();

	Q_DECLARE_PUBLIC(TextEditChecker)
};

/**
 * @brief An enhanced QTextCursor
 */
class TextCursor : public QTextCursor
{
public:
	TextCursor()
		: QTextCursor(), m_wordRegEx(QRegExp("^\\w$")) {}
	TextCursor(QTextDocument* document)
		: QTextCursor(document), m_wordRegEx(QRegExp("^\\w$")) {}
	TextCursor(const QTextBlock& block)
		: QTextCursor(block), m_wordRegEx(QRegExp("^\\w$")) {}
	TextCursor(const QTextCursor& cursor)
		: QTextCursor(cursor), m_wordRegEx(QRegExp("^\\w$")) {}

	/**
	 * @brief Retreive the num-th next character.
	 * @param num Which character to retreive.
	 * @return A string containing the character, might be empty.
	 */
	QString nextChar(int num = 1) const;

	/**
	 * @brief Retreive the num-th previous character.
	 * @param num Which character to retreive.
	 * @return A string containing the character, might be empty.
	 */
	QString prevChar(int num = 1) const;

	/**
	 * @brief Move the cursor to the start of the current word. Cursor must be
	 *        inside a word. This method correctly honours apostrophes.
	 * @param moveMode The move mode, see QTextCursor::MoveMode.
	 */
	void moveWordStart(MoveMode moveMode = MoveAnchor);

	/**
	 * @brief Move the cursor to the end of the current word. Cursor must be
	 *        inside a word. This method correctly honours apostrophes.
	 * @param moveMode The move mode, see QTextCursor::MoveMode.
	 */
	void moveWordEnd(MoveMode moveMode = MoveAnchor);

	/**
	 * @brief Returns whether the cursor is inside a word.
	 * @return Whether the cursor is inside a word.
	 */
	bool isInsideWord() const{
		return nextChar().contains(m_wordRegEx) || prevChar().contains(m_wordRegEx);
	}

	/**
	 * @brief Returns whether the specified character is a word character.
	 * @return Whether the specified character is a word character.
	 */
	bool isWordChar(const QString& character) const{
		return character.contains(m_wordRegEx);
	}

private:
	QRegExp m_wordRegEx;
};

///////////////////////////////////////////////////////////////////////////////

class TextEditProxy : public QObject {
	Q_OBJECT
public:
	TextEditProxy(QObject* parent = nullptr) : QObject(parent) {}
	virtual QTextCursor textCursor() const = 0;
	virtual QTextDocument* document() const = 0;
	virtual QPoint mapToGlobal(const QPoint& pos) const = 0;
	virtual QMenu* createStandardContextMenu() = 0;
	virtual QTextCursor cursorForPosition(const QPoint& pos) const = 0;
	virtual void setContextMenuPolicy(Qt::ContextMenuPolicy policy) = 0;
	virtual void setTextCursor(const QTextCursor& cursor) = 0;
	virtual Qt::ContextMenuPolicy contextMenuPolicy() const = 0;
	virtual void installEventFilter(QObject* filterObj) = 0;
	virtual void removeEventFilter(QObject* filterObj) = 0;
	virtual void ensureCursorVisible() = 0;

signals:
	void customContextMenuRequested(const QPoint& pos);
	void textChanged();
	void editDestroyed();
};

template<class T>
class TextEditProxyT : public TextEditProxy {
public:
	TextEditProxyT(T* textEdit, QObject* parent = nullptr) : TextEditProxy(parent), m_textEdit(textEdit) {
		connect(textEdit, &T::customContextMenuRequested, this, &TextEditProxy::customContextMenuRequested);
		connect(textEdit, &T::textChanged, this, &TextEditProxy::textChanged);
		connect(textEdit, &T::destroyed, this, &TextEditProxy::editDestroyed);
	}
	QTextCursor textCursor() const{ return m_textEdit->textCursor(); }
	QTextDocument* document() const{ return m_textEdit->document(); }
	QPoint mapToGlobal(const QPoint& pos) const{ return m_textEdit->mapToGlobal(pos); }
	QMenu* createStandardContextMenu(){ return m_textEdit->createStandardContextMenu(); }
	QTextCursor cursorForPosition(const QPoint& pos) const{ return m_textEdit->cursorForPosition(pos); }
	void setContextMenuPolicy(Qt::ContextMenuPolicy policy){ m_textEdit->setContextMenuPolicy(policy); }
	void setTextCursor(const QTextCursor& cursor){ m_textEdit->setTextCursor(cursor); }
	Qt::ContextMenuPolicy contextMenuPolicy() const{ return m_textEdit->contextMenuPolicy(); }
	void installEventFilter(QObject* filterObj){ m_textEdit->installEventFilter(filterObj); }
	void removeEventFilter(QObject* filterObj){ m_textEdit->removeEventFilter(filterObj); }
	void ensureCursorVisible() { m_textEdit->ensureCursorVisible(); }

private:
	T* m_textEdit = nullptr;
};

} // QtSpell

#endif // QTSPELL_TEXTEDITCHECKER_P_HPP
