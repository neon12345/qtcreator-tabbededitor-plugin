#ifndef TABBAR_H
#define TABBAR_H

#include <QTabBar>

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
public:
    explicit TabBar(QWidget *parent = 0);

private slots:
    void activateEditor(int index);

    void addEditorTab(Core::IEditor *editor);
    void removeEditorTabs(QList<Core::IEditor *> editors);
    void selectEditorTab(Core::IEditor *editor);

    void closeTab(int index);

    void prevTabAction();
    void nextTabAction();

    void mouseWheelChange(bool checked);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    QList<Core::IEditor *> m_editors;
    QList<QShortcut *> m_shortcuts;
    bool m_wheel;
};

} // namespace Internal
} // namespace TabbedEditor

#endif // TABBAR_H
