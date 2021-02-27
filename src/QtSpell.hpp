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

#ifndef QTSPELL_HPP
#define QTSPELL_HPP

#include "QtSpellExport.hpp"

#include <QObject>

class QMenu;
class QPlainTextEdit;
class QPoint;
class QTextEdit;

/**
 * @brief QtSpell namespace
 */
namespace QtSpell {

class CheckerPrivate;
class TextEditCheckerPrivate;

/**
 * @brief Check whether the dictionary for a language is installed
 */
bool QTSPELL_API checkLanguageInstalled(const QString& lang);

///////////////////////////////////////////////////////////////////////////////

/**
 * @brief An abstract class providing spell checking support.
 */
class QTSPELL_API Checker : public QObject
{
	Q_OBJECT
public:
	/**
	 * @brief QtSpell::Checker object constructor.
	 */
	Checker(QObject* parent = 0);

	/**
	 * @brief QtSpell::Checker object destructor.
	 */
	virtual ~Checker();

	/**
	 * @brief Check the spelling.
	 * @param start The start position within the buffer.
	 * @param end The end position within the buffer (-1 for the buffer end).
	 */
	virtual void checkSpelling(int start = 0, int end = -1) = 0;

	/**
	 * @brief Set the spell checking language.
	 * @param lang The language, as a locale specifier (i.e. "en_US"), or an
	 *             empty string to attempt to use the system locale.
	 * @return true on success, false on failure.
	 */
	bool setLanguage(const QString& lang);

	/**
	 * @brief Retreive the current spelling language.
	 * @return The current spelling language.
	 */
	QString getLanguage() const;

	/**
	 * @brief Set whether to decode language codes in the UI.
	 * @note Requres the iso-codes package.
	 * @param decode Whether to decode the language codes.
	 */
	void setDecodeLanguageCodes(bool decode);

	/**
	 * @brief Return whether langauge codes are decoded in the UI.
	 * @return Whether langauge codes are decoded in the UI.
	 */
	bool getDecodeLanguageCodes() const;

	/**
	 * @brief Set whether to display an "Check spelling" checkbox in the UI.
	 * @param show Whether to display an "Check spelling" checkbox in the UI.
	 */
	void setShowCheckSpellingCheckbox(bool show);

	/**
	 * @brief Return whether a "Check spelling" checkbox is displayed in the UI.
	 * @return Whether a "Check spelling" checkbox is displayed in the UI.
	 */
	bool getShowCheckSpellingCheckbox() const;

	/**
	 * @brief Return whether spellchecking is performed.
	 * @return Whether spellchecking is performed.
	 */
	bool getSpellingEnabled() const;

	/**
	 * @brief Add the specified word to the user dictionary
	 * @param word The word to add to the dictionary
	 */
	void addWordToDictionary(const QString& word);

	/**
	 * @brief Check the specified word.
	 * @param word A word.
	 * @return Whether the word is correct.
	 */
	bool checkWord(const QString& word) const;

	/**
	 * @brief Ignore a word for the current session.
	 * @param word The word to ignore.
	 */
	void ignoreWord(const QString& word) const;

	/**
	 * @brief Retreive a list of spelling suggestions for the misspelled word.
	 * @param word The misspelled word.
	 * @return A list of spelling suggestions.
	 */
	QList<QString> getSpellingSuggestions(const QString& word) const;


	/**
	 * @brief Requests the list of languages available for spell checking.
	 * @return A list of languages available for spell checking.
	 */
	static QList<QString> getLanguageList();

	/**
	 * @brief Translates a language code to a human readable format
	 *        (i.e. "en_US" -> "English (United States)").
	 * @param lang The language locale specifier.
	 * @note If the iso-codes package is unavailable, the unchanged code is
	 *       returned.
	 * @return The human readable language description.
	 */
	static QString decodeLanguageCode(const QString& lang);

public slots:
	/**
	 * @brief Set whether spell checking should be performed.
	 * @param enabled True if spell checking should be performed.
	 */
	void setSpellingEnabled(bool enabled);

signals:
	/**
	 * @brief This signal is emitted when the user selects a new language from
	 *        the spellchecker UI.
	 * @param newLang The new language, as a locale specifier.
	 */
	void languageChanged(const QString& newLang);

protected:
	void showContextMenu(QMenu* menu, const QPoint& pos, int wordPos);

private slots:
	void slotAddWord();
	void slotIgnoreWord();
	void slotReplaceWord();
	void slotSetLanguage(bool checked);

private:
	/**
	 * @brief Get the word at the specified cursor position.
	 * @param pos The cursor position.
	 * @param start If not 0, will contain the start position of the word.
	 * @param end If not 0, will contain the end position of the word.
	 * @return The word.
	 */
	virtual QString getWord(int pos, int* start = 0, int* end = 0) const = 0;

	/**
	 * @brief Replaces the specified range with the specified word.
	 * @param start The start position.
	 * @param end The end position.
	 * @param word The word to insert.
	 */
	virtual void insertWord(int start, int end, const QString& word) = 0;

	/**
	 * @brief Returns whether a widget is attached to the checker.
	 * @return Whether a widget is attached to the checker.
	 */
	virtual bool isAttached() const = 0;

protected:
	Checker(CheckerPrivate& dd, QObject* parent = 0);
	CheckerPrivate* d_ptr;

private:
	Q_DECLARE_PRIVATE(Checker)
};

///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Checker class for QTextEdit widgets.
 * @details Sample usage: @include example.hpp
 */
class QTSPELL_API TextEditChecker : public Checker
{
	Q_OBJECT
public:
	/**
	 * @brief TextEditChecker object constructor.
	 */
	TextEditChecker(QObject* parent = 0);

	/**
	  * @brief TextEditChecker object destructor.
	  */
	~TextEditChecker();

	/**
	 * @brief Set the QTextEdit to check.
	 * @param textEdit The QTextEdit to check, or 0 to detach.
	 */
	void setTextEdit(QTextEdit* textEdit);

	/**
	 * @brief Set the QPlainTextEdit to check.
	 * @param textEdit The QPlainTextEdit to check, or 0 to detach.
	 */
	void setTextEdit(QPlainTextEdit* textEdit);

	/**
	 * @brief Set the QTextCharFormat property identifier which marks whether
	 *        a word ought to be spell-checked.
	 * @param propertyId By default this is -1, meaning that no such property
	 *        is set. To enable, pass a value above QTextFormat::UserProperty.
	 * @note If the value returned by QTextFormat::intProperty is 1, spelling
	 *       is skipped. To work correctly, this property needs to be set for
	 *       the entire word.
	 */
	void setNoSpellingPropertyId(int propertyId);

	/**
	 * @brief Returns the current QTextCharFormat property identifier which
	 *        marks whether a word ought to be spell-checked.
	 * @return The no-spelling QTextCharFormat property identifier.
	 */
	int noSpellingPropertyId() const;

	void checkSpelling(int start = 0, int end = -1);

	/**
	 * @brief Sets whether undo/redo functionality is enabled.
	 * @param enabled Whether undo/redo is enabled.
	 * @note QtSpell::TextEditChecker reimplements the undo/redo functionality
	 *       since the one provided by QTextDocument also tracks text format
	 *       changes (i.e. underlining of spelling errors) which is undesirable.
	 */
	void setUndoRedoEnabled(bool enabled);

public slots:
	/**
	 * @brief Undo the last edit operation.
	 * @note QtSpell::TextEditChecker reimplements the undo/redo functionality
	 *       since the one provided by QTextDocument also tracks text format
	 *       changes (i.e. underlining of spelling errors) which is undesirable.
	 */

	void undo();
	/**
	 * @brief Redo the last edit operation.
	 * @note QtSpell::TextEditChecker reimplements the undo/redo functionality
	 *       since the one provided by QTextDocument also tracks text format
	 *       changes (i.e. underlining of spelling errors) which is undesirable.
	 */
	void redo();

	/**
	 * @brief Clears the undo/redo stack.
	 * @note QtSpell::TextEditChecker reimplements the undo/redo functionality
	 *       since the one provided by QTextDocument also tracks text format
	 *       changes (i.e. underlining of spelling errors) which is undesirable.
	 */
	void clearUndoRedo();

signals:
	/**
	 * @brief Emitted when the undo stack changes.
	 * @param available Whether undo steps are available.
	 * @note QtSpell::TextEditChecker reimplements the undo/redo functionality
	 *       since the one provided by QTextDocument also tracks text format
	 *       changes (i.e. underlining of spelling errors) which is undesirable.
	 */
	void undoAvailable(bool available);

	/**
	 * @brief Emitted when the redo stak changes.
	 * @param available Whether redo steps are available.
	 * @note QtSpell::TextEditChecker reimplements the undo/redo functionality
	 *       since the one provided by QTextDocument also tracks text format
	 *       changes (i.e. underlining of spelling errors) which is undesirable.
	 */
	void redoAvailable(bool available);

private:
	QString getWord(int pos, int* start = 0, int* end = 0) const;
	void insertWord(int start, int end, const QString& word);
	bool isAttached() const;
	bool eventFilter(QObject *obj, QEvent *event);

private slots:
	void slotShowContextMenu(const QPoint& pos);
	void slotCheckDocumentChanged();
	void slotDetachTextEdit();
	void slotCheckRange(int pos, int removed, int added);

private:
	Q_DECLARE_PRIVATE(TextEditChecker)
};

} // QtSpell

#endif // QTSPELL_HPP
