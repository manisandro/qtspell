QtSpell - Spell checking for Qt text widgets
============================================

Introduction
------------
QtSpell adds spell-checking functionality to Qt's text widgets, using the
enchant spell-checking library.


Usage
-----
### Simple
To check spelling in a `QTextEdit` or `QPlainTextEdit`, proceed as follows:
```c++
// create a QtSpell::TextEdit instance
QtSpell::TextEditChecker checker;

// optional: set the language (by default, the system locale is used)
checker.setLanguage("de_CH");

// attach to a QTextEdit or QPlainTextEdit
QTextEdit textEdit = new QTextEdit();
checker.setTextEdit(textEdit);
```
If you wish to use `undo` and `redo` on a `Q{Plain}TextEdit` with an attached
`QtSpell::TextEditChecker`, use the undo and redo functionality provided by
`QtSpell::TextEditChecker`, since the corresponding `Q{Plain}TextEdit` methods
do not work correctly when spell checking is enabled.

### Advanced
`QtSpell::TextEditChecker` inherits from the abstract `QtSpell::Checker` class.
You can derive from the `QtSpell::Checker` class, implementing the interface
methods

- `QtSpell::Checker::checkSpelling`
- `QtSpell::Checker::getWord`
- `QtSpell::Checker::insertWord`
- `QtSpell::Checker::isAttached`

to create a spell checker for any other widget.


Build instructions
------------------
You need to have the enchant, as well as either or both the qt4 and qt5-qtbase
development files installed. If you want to build the documentation, you need
Doxygen.
QtSpell uses CMake as the build system. From withing the QtSpell source directory:
```shell
mkdir build
cd build
cmake ..
make
sudo make install
```
By default, QtSpell is built against Qt5. If you want to build against Qt4, pass `-DUSE_QT5=OFF` to `cmake`.

Author
------
Sandro Mani <manisandro@gmail.com>
