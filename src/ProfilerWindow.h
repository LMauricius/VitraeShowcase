#pragma once

#include "ui_Profiler.h"

#include <QtWidgets/QMainWindow>

#include "Status.hpp"
#include "assetCollection.hpp"

class ProfilerWindow : public QMainWindow
{
  public:
    ProfilerWindow(Status &status);
    virtual ~ProfilerWindow();

    void updateValues();

  private:
    Ui::Profiler ui;

    Status &m_status;
};