#ifndef TABBAR_H
#define TABBAR_H

#include <QTabBar>
#include "coreplugin/id.h"

QT_BEGIN_NAMESPACE
class QShortcut;
QT_END_NAMESPACE

namespace Core {
class IEditor;
}

namespace TabbedEditor {
namespace Internal {

class TabBar : public QTabBar
{
    Q_OBJECT

    struct FullscreenState
    {
        bool modeSelectorVisible;
        bool rightPaneWidgetVisible;
        bool navigationWidgetVisible;
        Core::Id modeManagerId;
    };
public:
    explicit TabBar(QWidget *parent = 0);

private slots:
    void activateEditor(int index);

    void addEditorTab(Core::IEditor *editor);
    void removeEditorTabs(QList<Core::IEditor *> editors);
    void selectEditorTab(Core::IEditor *editor);

    void closeTab(int index);
    void fullscreenTab(int);

    void prevTabAction();
    void nextTabAction();

    void mouseWheelChange(bool checked);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void getFullscreenState();
    void resetFullscreenState();
    QList<Core::IEditor *> m_editors;
    QList<QShortcut *> m_shortcuts;
    FullscreenState m_state;
    bool m_wheel;
    bool m_fullscreen;
};

} // namespace Internal
} // namespace TabbedEditor

#endif // TABBAR_H
