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
#include <QDialogButtonBox>
#include <QPushButton>

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
		QTextEdit* textEdit = new QTextEdit(this);
		textEdit->setUndoRedoEnabled(true);

		QDialogButtonBox* bbox = new QDialogButtonBox(this);

		QPushButton* buttonUndo = bbox->addButton("Undo", QDialogButtonBox::ActionRole);
		buttonUndo->setEnabled(false);

		QPushButton* buttonRedo = bbox->addButton("Redo", QDialogButtonBox::ActionRole);
		buttonRedo->setEnabled(false);

		QWidget* widget = new QWidget(this);
		setCentralWidget(widget);

		QVBoxLayout* layout = new QVBoxLayout(widget);
		layout->addWidget(label);
		layout->addWidget(textEdit, 1);
		layout->addWidget(bbox);


		QtSpell::TextEditChecker* checker = new QtSpell::TextEditChecker(this);
		checker->setTextEdit(textEdit);
		checker->setDecodeLanguageCodes(true);
		checker->setShowCheckSpellingCheckbox(true);

		connect(textEdit, SIGNAL(undoAvailable(bool)), buttonUndo, SLOT(setEnabled(bool)));
		connect(textEdit, SIGNAL(redoAvailable(bool)), buttonRedo, SLOT(setEnabled(bool)));
		connect(buttonUndo, SIGNAL(clicked()), checker, SLOT(undo()));
		connect(buttonRedo, SIGNAL(clicked()), checker, SLOT(redo()));
	}
};

#endif // EXAMPLE_HPP
