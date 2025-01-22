#include "ProfilerWindow.h"

#include <QtWidgets/QColorDialog>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollBar>

#include <mutex>

ProfilerWindow::ProfilerWindow(AssetCollection &assetCollection, Status &status)
    : QMainWindow(), ui(), m_assetCollection(assetCollection), m_status(status)
{
    ui.setupUi(this);

    updateValues();
}

ProfilerWindow::~ProfilerWindow() {}

void ProfilerWindow::updateValues()
{
    int pos = ui.profilerMetrics->verticalScrollBar()->value();
    ui.profilerMetrics->setText(QString::fromStdString(m_status.mmeterMetrics));
    ui.profilerMetrics->setMaximumHeight(5000000);
    ui.profilerMetrics->verticalScrollBar()->setValue(pos);
}
