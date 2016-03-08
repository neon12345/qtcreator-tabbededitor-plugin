#include "tabbar.h"

#include "constants.h"

#include <coreplugin/findplaceholder.h>
#include <coreplugin/rightpane.h>
#include <coreplugin/outputpane.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/navigationwidget.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/idocument.h>
#include <coreplugin/imode.h>
#include <projectexplorer/session.h>

#include <QMenu>
#include <QMouseEvent>
#include <QShortcut>
#include <QTabBar>

using namespace Core::Internal;

using namespace TabbedEditor::Internal;

/// TODO: Use Core::DocumentModel for everything

TabBar::TabBar(QWidget *parent) :
    QTabBar(parent)
    , m_wheel(false)
    , m_fullscreen(false)
{
    setExpanding(false);
    setMovable(true);
    setTabsClosable(true);
    setUsesScrollButtons(true);

    QSizePolicy sp(QSizePolicy::Preferred, QSizePolicy::Fixed);
    sp.setHorizontalStretch(1);
    sp.setVerticalStretch(0);
    sp.setHeightForWidth(sizePolicy().hasHeightForWidth());
    setSizePolicy(sp);

    connect(this, &QTabBar::tabMoved, [this](int from, int to) {
        m_editors.move(from, to);
    });

    Core::EditorManager *em = Core::EditorManager::instance();

    connect(em, &Core::EditorManager::editorOpened, this, &TabBar::addEditorTab);
    connect(em, &Core::EditorManager::editorsClosed, this, &TabBar::removeEditorTabs);
    connect(em, SIGNAL(currentEditorChanged(Core::IEditor*)), SLOT(selectEditorTab(Core::IEditor*)));

    connect(this, &QTabBar::currentChanged, this, &TabBar::activateEditor);
    connect(this, &QTabBar::tabCloseRequested, this, &TabBar::closeTab);
    connect(this, &QTabBar::tabBarDoubleClicked, this, &TabBar::fullscreenTab);

    ProjectExplorer::SessionManager *sm = ProjectExplorer::SessionManager::instance();
    connect(sm, &ProjectExplorer::SessionManager::sessionLoaded, [this, em]() {
        foreach (Core::DocumentModel::Entry *entry, Core::DocumentModel::entries())
            em->activateEditorForEntry(entry, Core::EditorManager::DoNotChangeCurrentEditor);
    });

    const QString shortCutSequence = QStringLiteral("Ctrl+Alt+%1");
    for (int i = 1; i <= 10; ++i) {
        QShortcut *shortcut = new QShortcut(shortCutSequence.arg(i % 10), this);
        connect(shortcut, &QShortcut::activated, [this, shortcut]() {
            setCurrentIndex(m_shortcuts.indexOf(shortcut));
        });
        m_shortcuts.append(shortcut);
    }

    QAction *prevTabAction = new QAction(tr("Switch to previous tab"), this);
    Core::Command *prevTabCommand
            = Core::ActionManager::registerAction(prevTabAction,
                                                  TabbedEditor::Constants::PREV_TAB_ID,
                                                  Core::Context(Core::Constants::C_GLOBAL));
    prevTabCommand->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+J")));
    connect(prevTabAction, SIGNAL(triggered()), this, SLOT(prevTabAction()));

    QAction *nextTabAction = new QAction(tr("Switch to next tab"), this);
    Core::Command *nextTabCommand
            = Core::ActionManager::registerAction(nextTabAction,
                                                  TabbedEditor::Constants::NEXT_TAB_ID,
                                                  Core::Context(Core::Constants::C_GLOBAL));
    nextTabCommand->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+K")));
    connect(nextTabAction, SIGNAL(triggered()), this, SLOT(nextTabAction()));

    getFullscreenState();
}

void TabBar::activateEditor(int index)
{
    if (index < 0 || index >= m_editors.size())
        return;

    Core::EditorManager::instance()->activateEditor(m_editors[index]);
}

void TabBar::addEditorTab(Core::IEditor *editor)
{
    Core::IDocument *document = editor->document();

    const int index = addTab(document->displayName());
    setTabIcon(index, Core::FileIconProvider::icon(document->filePath().toFileInfo()));
    setTabToolTip(index, document->filePath().toString());

    m_editors.append(editor);

    connect(document, &Core::IDocument::changed, [this, editor, document]() {
        const int index = m_editors.indexOf(editor);
        if (index == -1)
            return;
        QString tabText = document->displayName();
        if (document->isModified())
            tabText += QLatin1Char('*');
        setTabText(index, tabText);
    });
}

void TabBar::removeEditorTabs(QList<Core::IEditor *> editors)
{
    blockSignals(true); // Avoid calling activateEditor()
    foreach (Core::IEditor *editor, editors) {
        const int index = m_editors.indexOf(editor);
        if (index == -1)
            continue;
        m_editors.removeAt(index);
        removeTab(index);
    }
    blockSignals(false);
}

void TabBar::selectEditorTab(Core::IEditor *editor)
{
    const int index = m_editors.indexOf(editor);
    if (index == -1)
        return;
    setCurrentIndex(index);
}

void TabBar::closeTab(int index)
{
    if (index < 0 || index >= m_editors.size())
        return;

    Core::EditorManager::instance()->closeEditor(m_editors.takeAt(index));
    removeTab(index);
}

void TabBar::getFullscreenState()
{
    Core::NavigationWidget* nw = Core::NavigationWidget::instance();
    Core::RightPaneWidget* rw = Core::RightPaneWidget::instance();
    Core::ModeManager* mm = Core::ModeManager::instance();

    m_state.modeSelectorVisible = mm->isModeSelectorVisible();
    m_state.rightPaneWidgetVisible = rw->isShown();
    m_state.navigationWidgetVisible = nw->isShown();
    m_state.modeManagerId = Core::ModeManager::currentMode()->id();
}

void TabBar::resetFullscreenState()
{
    Core::NavigationWidget* nw = Core::NavigationWidget::instance();
    Core::RightPaneWidget* rw = Core::RightPaneWidget::instance();
    Core::ModeManager* mm = Core::ModeManager::instance();

    mm->setModeSelectorVisible(m_state.modeSelectorVisible);
    rw->setShown(m_state.rightPaneWidgetVisible);
    nw->setShown(m_state.navigationWidgetVisible);
    Core::ModeManager::activateMode(m_state.modeManagerId);
}

void TabBar::fullscreenTab(int)
{
    Core::NavigationWidget* nw = Core::NavigationWidget::instance();
    Core::RightPaneWidget* rw = Core::RightPaneWidget::instance();
    Core::ModeManager* mm = Core::ModeManager::instance();

    if(m_fullscreen)
    {
        resetFullscreenState();
    }
    else
    {
        getFullscreenState();
        mm->setModeSelectorVisible(false);
        rw->setShown(false);
        nw->setShown(false);
        Core::ModeManager::activateMode(Core::Id(Core::Constants::MODE_EDIT));

        Core::FindToolBarPlaceHolder* findPane = Core::FindToolBarPlaceHolder::getCurrent();
        if (findPane && findPane->isVisible())
            findPane->hide();

        Core::OutputPanePlaceHolder* outputPane = Core::OutputPanePlaceHolder::getCurrent();
        if(outputPane && outputPane->isVisible())
            emit outputPane->hide();
    }

    m_fullscreen = !m_fullscreen;
}

void TabBar::prevTabAction()
{
    int index = currentIndex();
    if (index >= 1)
        setCurrentIndex(index - 1);
    else
        setCurrentIndex(count() - 1);
}

void TabBar::nextTabAction()
{
    int index = currentIndex();
    if (index < count() - 1)
        setCurrentIndex(index + 1);
    else
        setCurrentIndex(0);
}

void TabBar::contextMenuEvent(QContextMenuEvent *event)
{
    const int index = tabAt(event->pos());
    if (index == -1)
        return;

    QScopedPointer<QMenu> menu(new QMenu());

    Core::IEditor *editor = m_editors[index];
    Core::DocumentModel::Entry *entry = Core::DocumentModel::entryForDocument(editor->document());
    Core::EditorManager::addSaveAndCloseEditorActions(menu.data(), entry, editor);
    menu->addSeparator();
    Core::EditorManager::addNativeDirAndOpenWithActions(menu.data(), entry);

    QAction* mouse_wheel = new QAction(tr("Mouse Wheel?"), this);
    mouse_wheel->setCheckable(true);
    mouse_wheel->setChecked(m_wheel);
    menu->addAction(mouse_wheel);

    connect(mouse_wheel, SIGNAL(triggered(bool)),
                   this, SLOT(mouseWheelChange(bool)));

    menu->exec(mapToGlobal(event->pos()));
}

void TabBar::mouseWheelChange(bool checked)
{
    m_wheel = checked;
}

void TabBar::wheelEvent(QWheelEvent *event)
{
    if(m_wheel)
        QTabBar::wheelEvent(event);
}

void TabBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton)
        closeTab(tabAt(event->pos()));
    QTabBar::mousePressEvent(event);
}
