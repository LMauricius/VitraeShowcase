#include "ProfilerWindow.h"

#include <QtWidgets/QColorDialog>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollBar>

#include <mutex>

ProfilerWindow::ProfilerWindow(Status &status) : QMainWindow(), ui(), m_status(status)
{
    ui.setupUi(this);

    ui.profilerMetrics->verticalScrollBar()->setTracking(true);

    updateValues();
}

ProfilerWindow::~ProfilerWindow() {}

void ProfilerWindow::updateValues()
{

    int pos = ui.profilerMetrics->verticalScrollBar()->value();
    int minPos = ui.profilerMetrics->verticalScrollBar()->minimum();
    int maxPos = ui.profilerMetrics->verticalScrollBar()->maximum();
    QString statusMetrics;
    {
        std::unique_lock lock1(m_status.accessMutex);
        statusMetrics = QString::fromStdString(m_status.mmeterMetrics);
    }
    ui.profilerMetrics->setPlainText(statusMetrics);
    int newMinPos = ui.profilerMetrics->verticalScrollBar()->minimum();
    int newMaxPos = ui.profilerMetrics->verticalScrollBar()->maximum();
    ui.profilerMetrics->verticalScrollBar()->setValue(
        int(double(pos - minPos) * (newMaxPos - newMinPos) / (maxPos - minPos)) + newMinPos);
}
