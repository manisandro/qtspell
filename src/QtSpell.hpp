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

#if defined(QTSPELL_LIBRARY)
#  define QTSPELL_API Q_DECL_EXPORT
#else
#  define QTSPELL_API Q_DECL_IMPORT
#endif

#include <QObject>
#include <QRegExp>

class QLineEdit;
class QMenu;
class QPlainTextEdit;
class QPoint;
class QTextDocument;
class QTextEdit;

namespace enchant { class Dict; }

namespace QtSpell {

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
	const QString& getLanguage() const{ return m_lang; }

	/**
	 * @brief Set whether to decode language codes in the UI.
	 * @note Requres the iso-codes package.
	 * @param decode Whether to decode the language codes.
	 */
	void setDecodeLanguageCodes(bool decode){ m_decodeCodes = decode; }

	/**
	 * @brief Return whether langauge codes are decoded in the UI.
	 * @return Whether langauge codes are decoded in the UI.
	 */
	bool getDecodeLanguageCodes() const{ return m_decodeCodes; }

	/**
	 * @brief Set whether to display an "Check spelling" checkbox in the UI.
	 * @param show Whether to display an "Check spelling" checkbox in the UI.
	 */
	void setShowCheckSpellingCheckbox(bool show) { m_spellingCheckbox = show; }

	/**
	 * @brief Return whether a "Check spelling" checkbox is displayed in the UI.
	 * @return Whether a "Check spelling" checkbox is displayed in the UI.
	 */
	bool getShowCheckSpellingCheckbox() const{ return m_spellingCheckbox; }

	/**
	 * @brief Return whether spellchecking is performed.
	 * @return Whether spellchecking is performed.
	 */
	bool getSpellingEnabled() const{ return m_spellingEnabled; }

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
	void setSpellingEnabled(bool enabled) { m_spellingEnabled = enabled; checkSpelling(); }

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
	enchant::Dict* m_speller;
	QString m_lang;
	bool m_decodeCodes;
	bool m_spellingCheckbox;
	bool m_spellingEnabled;
	QRegExp m_wordRegEx;

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
	bool setLanguageInternal(const QString& lang);
};

///////////////////////////////////////////////////////////////////////////////

class TextEditProxy;
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

	void checkSpelling(int start = 0, int end = -1);

public slots:
	/**
	 * @brief Undo the last edit operation.
	 * @note Use this function instead of Q(Plain)TextEdit::undo, since the
	 *       latter does not work correctly together with spell checking.
	 */

	void undo();
	/**
	 * @brief Redo the last edit operation.
	 * @note Use this function instead of Q(Plain)TextEdit::redo, since the
	 *       latter does not work correctly together with spell checking.
	 */
	void redo();

private:
	TextEditProxy* m_textEdit;
	QTextDocument* m_document;
	Qt::ContextMenuPolicy m_oldContextMenuPolicy;
	bool m_undoInProgress;
	bool m_redoInProgress;

	QString getWord(int pos, int* start = 0, int* end = 0) const;
	void insertWord(int start, int end, const QString& word);
	bool isAttached() const{ return m_textEdit != 0; }
	void setTextEdit(TextEditProxy* textEdit);

private slots:
	void slotShowContextMenu(const QPoint& pos);
	void slotCheckDocumentChanged();
	void slotDetachTextEdit();
	void slotCheckRange(int pos, int removed, int added);
};

} // QtSpell

#endif // QTSPELL_HPP
