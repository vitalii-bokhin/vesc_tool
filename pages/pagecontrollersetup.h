/*
    Copyright 2016 - 2025 Benjamin Vedder	benjamin@vedder.se

    This file is part of VESC Tool.

    VESC Tool is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VESC Tool is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

#ifndef PAGECONTROLLERSETUP_H
#define PAGECONTROLLERSETUP_H

#include <QWidget>
#include <QTimer>
#include "vescinterface.h"

class QVBoxLayout;

namespace Ui {
class PageControllerSetup;
}

class PageControllerSetup : public QWidget
{
    Q_OBJECT

public:
    explicit PageControllerSetup(QWidget *parent = nullptr);
    ~PageControllerSetup();

    VescInterface *vesc() const;
    void setVesc(VescInterface *vesc);

    void reloadParams();

private slots:
    void on_loadMotorConfButton_clicked();
    void on_loadAppConfButton_clicked();
    void on_runSetupButton_clicked();
    void valuesReceived(MC_VALUES values, unsigned int mask);
    void rlMeasurementDone(bool ok);
    void lambdaMeasurementDone(bool ok);
    void configWritten(bool ok);
    void configWriteTimeout();
    void hallMeasurementDone(bool ok);
    void hallConfigWriteDone(bool ok);
    void portConnectedChanged();

private:
    enum ConfigType { ConfigMotor, ConfigApp };
    enum SetupStep { StepIdle, StepMeasureRL, StepMeasureLambda, StepWriteConfig, StepMeasureHall, StepWriteHallConfig, StepDone };

    Ui::PageControllerSetup *ui;
    VescInterface *mVesc;
    QStringList mMotorConfigHistory;
    QStringList mAppConfigHistory;
    SetupStep mSetupStep = StepIdle;
    QTimer mConfigWriteTimeout;

    void loadConfigHistory();
    void addToHistory(ConfigType type, const QString &path);
    void saveHistory(ConfigType type);
    void removeFromHistory(ConfigType type, const QString &path);
    void updateHistoryButtons(ConfigType type);
    void onHistoryClicked(ConfigType type, const QString &path);

    QStringList &historyFor(ConfigType type);
    QVBoxLayout *historyLayoutFor(ConfigType type);
    void loadMotorConfig(const QString &path);
    void loadAppConfig(const QString &path);
    static SetupStep nextSetupStep(SetupStep s);
    void runNextStep();
    void finishSetup(const QString &message, bool success);
};

#endif // PAGECONTROLLERSETUP_H
