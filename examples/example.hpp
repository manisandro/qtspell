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

/**
* @example example.hpp
* Simple example demonstrating the use of QtSpell::TextEditChecker
*/

#ifndef EXAMPLE_HPP
#define EXAMPLE_HPP

#include <QCheckBox>
#include <QLabel>
#include <QMainWindow>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QtSpell.hpp>

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	MainWindow()
		: QMainWindow()
	{
		setWindowTitle("QtSpell Example");

		QLabel* label = new QLabel("Type some text into the text box.\n"
								   "Try misspelling some words. Then right click them.");
		m_textEdit = new QTextEdit(this);
		QCheckBox* check = new QCheckBox("Enable spelling");
		check->setChecked(true);

		QWidget* widget = new QWidget(this);
		setCentralWidget(widget);

		QVBoxLayout* layout = new QVBoxLayout(widget);
		layout->addWidget(label);
		layout->addWidget(m_textEdit, 1);
		layout->addWidget(check);

		m_checker = new QtSpell::TextEditChecker(this);
		m_checker->setTextEdit(m_textEdit);
		m_checker->setDecodeLanguageCodes(true);

		connect(check, SIGNAL(clicked(bool)), this, SLOT(toggleSpelling(bool)));
	}

private slots:
	void toggleSpelling(bool enable){
		m_checker->setTextEdit(enable ? m_textEdit : 0);
	}

private:
	QTextEdit* m_textEdit;
	QtSpell::TextEditChecker* m_checker;
};

#endif // EXAMPLE_HPP
