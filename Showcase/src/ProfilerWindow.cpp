#include "ProfilerWindow.h"

#include <QtWidgets/QColorDialog>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QPushButton>

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
    ui.profilerMetrics->setText(QString::fromStdString(m_status.mmeterMetrics));
}
