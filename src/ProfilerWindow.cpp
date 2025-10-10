#include "ProfilerWindow.h"

#include <QtCore/QTextStream>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollBar>

#include <mutex>

ProfilerWindow::ProfilerWindow(Status &status) : QMainWindow(), ui(), m_status(status)
{
    ui.setupUi(this);

    ui.profilerMetrics->verticalScrollBar()->setTracking(true);

    connect(ui.resetButton, &QPushButton::clicked, [&] {
        std::unique_lock lock1(m_status.accessMutex);
        m_status.aggregateTree.reset();
    });

    connect(ui.saveButton, &QPushButton::clicked, [&] {
        QString currentText;
        {
            std::unique_lock lock1(m_status.accessMutex);
            currentText = QString::fromStdString(m_status.mmeterMetrics);
        }

        // Open the save dialog
        QString fileName =
            QFileDialog::getSaveFileName(this, "Save Vitrae Metrics", "", "Text Files (*.txt)");
        if (!fileName.isEmpty()) {
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out << currentText;
                file.close();
            }
        }
    });

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
