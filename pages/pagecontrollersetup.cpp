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

#include "pagecontrollersetup.h"
#include "ui_pagecontrollersetup.h"
#include <QWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QSettings>
#include <QPushButton>
#include <QFileInfo>
#include <QVBoxLayout>

PageControllerSetup::PageControllerSetup(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PageControllerSetup)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    mVesc = nullptr;

    mConfigWriteTimeout.setSingleShot(true);
    connect(&mConfigWriteTimeout, &QTimer::timeout,
            this, &PageControllerSetup::configWriteTimeout);

    loadConfigHistory();
    updateHistoryButtons(ConfigMotor);
    updateHistoryButtons(ConfigApp);
}

PageControllerSetup::~PageControllerSetup()
{
    delete ui;
}

VescInterface *PageControllerSetup::vesc() const
{
    return mVesc;
}

void PageControllerSetup::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    if (mVesc) {
        ui->detectFoc->setVesc(mVesc);
        ui->detectFoc->setHideApplyButton(true);
        ui->detectFoc->setSkipConfirmation(true);
        ui->detectFoc->setResetBoxValues(true);

        ui->detectFocHall->setVesc(mVesc);
        ui->detectFocHall->setWriteConfigEnabled(true);
        ui->detectFocHall->setHideApplyButton(true);
        ui->detectFocHall->setSkipConfirmation(true);
        ui->detectFocHall->setResetBoxValues(true);

        connect(mVesc->commands(), SIGNAL(valuesReceived(MC_VALUES,uint)),
                this, SLOT(valuesReceived(MC_VALUES,uint)));

        connect(ui->detectFoc, &DetectFoc::measureRLDone,
                this, &PageControllerSetup::rlMeasurementDone);
        connect(ui->detectFoc, &DetectFoc::measureLambdaDone,
                this, &PageControllerSetup::lambdaMeasurementDone);
        connect(ui->detectFoc, &DetectFoc::configWriteDone,
                this, &PageControllerSetup::configWritten);
        connect(mVesc, &VescInterface::portConnectedChanged,
                this, &PageControllerSetup::portConnectedChanged);

        // Request initial values to display temperatures
        mVesc->commands()->getValues();
        reloadParams();
    }
}

void PageControllerSetup::reloadParams()
{
    if (mVesc) {
        ui->generalTab->clearParams();
        ui->generalTab->addParamSubgroup(mVesc->mcConfig(), "foc", "general");
        if (ui->hallTab) {
            ui->hallTab->clearParams();
            ui->hallTab->addParamSubgroup(mVesc->mcConfig(), "foc", "hall sensors");
        }
        // App Settings - General tab
        if (ui->appGeneralTab) {
            ui->appGeneralTab->clearParams();
            ui->appGeneralTab->addParamSubgroup(mVesc->appConfig(), "general", "general");
        }
    }
}

void PageControllerSetup::valuesReceived(MC_VALUES values, unsigned int mask)
{
    (void)mask;
    ui->tempGauge->setValues(values);
}

void PageControllerSetup::on_runSetupButton_clicked()
{
    if (mSetupStep != StepIdle) {
        finishSetup(tr("Full setup aborted."), false);
        return;
    }

    if (!mVesc || !mVesc->isPortConnected()) {
        QMessageBox::warning(this, tr("Full Setup"),
                             tr("The VESC is not connected. Please connect it."));
        return;
    }

    if (ui->detectFoc->isRunning() || ui->detectFocHall->isRunning()) {
        QMessageBox::warning(this, tr("Full Setup"),
                             tr("A detection is already running. Please wait for it to finish."));
        return;
    }

    ui->runSetupButton->setText(tr("Abort Setup"));

    for (QPushButton *b : ui->detectFoc->findChildren<QPushButton*>()) {
        b->setEnabled(false);
    }

    for (QPushButton *b : ui->detectFocHall->findChildren<QPushButton*>()) {
        b->setEnabled(false);
    }

    ui->loadMotorConfButton->setEnabled(false);
    ui->loadAppConfButton->setEnabled(false);

    runNextStep();
}

void PageControllerSetup::rlMeasurementDone(bool ok)
{
    if (mSetupStep != StepMeasureRL) {
        return;
    }
    if (ok) {
        runNextStep();
    } else {
        finishSetup(tr("R & L measurement failed."), false);
    }
}

void PageControllerSetup::lambdaMeasurementDone(bool ok)
{
    if (mSetupStep != StepMeasureLambda) {
        return;
    }
    if (ok) {
        runNextStep();
    } else {
        finishSetup(tr("Flux linkage measurement failed."), false);
    }
}

void PageControllerSetup::configWritten(bool ok)
{
    if (mSetupStep != StepWriteConfig) {
        return;
    }
    mConfigWriteTimeout.stop();
    if (ok) {
        runNextStep();
    } else {
        finishSetup(tr("Motor configuration write failed."), false);
    }
}

void PageControllerSetup::configWriteTimeout()
{
    if (mSetupStep != StepWriteConfig) {
        return;
    }

    finishSetup(tr("Full setup stopped: no \"Motor config write OK\" response received."), false);
    QMessageBox::information(this, tr("Full Setup"),
                             tr("Full setup stopped: no \"Motor config write OK\" response received."));
}

void PageControllerSetup::portConnectedChanged()
{
    if (mVesc && !mVesc->isPortConnected() && mSetupStep != StepIdle) {
        finishSetup(tr("Full setup aborted: connection lost."), false);
    }
}

void PageControllerSetup::runNextStep()
{
    switch (mLastSetupStep) {
    case StepIdle:
        mLastSetupStep = StepMeasureRL;
        mSetupStep = StepMeasureRL;
        if (!ui->detectFoc->startMeasureRL()) {
            finishSetup(tr("Could not start R & L measurement."), false);
        }
        break;

    case StepMeasureRL:
        mLastSetupStep = StepMeasureLambda;
        mSetupStep = StepMeasureLambda;
        if (!ui->detectFoc->startMeasureLambda()) {
            finishSetup(tr("Could not start flux linkage measurement."), false);
        }
        break;

    case StepMeasureLambda:
        mLastSetupStep = StepWriteConfig;
        mSetupStep = StepWriteConfig;
        if (!ui->detectFoc->applyAndWriteConfig()) {
            finishSetup(tr("Could not apply parameters and write motor configuration."), false);
        } else {
            mConfigWriteTimeout.start(30000);
        }
        break;

    case StepWriteConfig:
        mSetupStep = StepDone;
        finishSetup(tr("Full setup completed."), true);
        break;

    default:
        break;
    }
}

void PageControllerSetup::finishSetup(const QString &message, bool success)
{
    if (!success && mVesc &&
            (mSetupStep == StepMeasureRL || mSetupStep == StepMeasureLambda)) {
        mVesc->commands()->setCurrent(0);
        int estopMs = QSettings().value("estop_ms", 5000).toInt();
        if (estopMs > 0) {
            mVesc->commands()->motorEstop(estopMs);
        }
    }

    mConfigWriteTimeout.stop();
    ui->detectFoc->cancelAutomation();
    mSetupStep = StepIdle;
    mLastSetupStep = StepIdle;
    ui->runSetupButton->setText(tr("Run Full Setup"));

    for (QPushButton *b : ui->detectFoc->findChildren<QPushButton*>()) {
        b->setEnabled(true);
    }

    for (QPushButton *b : ui->detectFocHall->findChildren<QPushButton*>()) {
        b->setEnabled(true);
    }

    ui->loadMotorConfButton->setEnabled(true);
    ui->loadAppConfButton->setEnabled(true);

    if (mVesc) {
        mVesc->emitStatusMessage(message, success);
    }
}

void PageControllerSetup::loadConfigHistory()
{
    QSettings settings;
    settings.beginGroup("PageControllerSetup");
    mMotorConfigHistory = settings.value("motorConfigHistory", QStringList()).toStringList();
    mAppConfigHistory = settings.value("appConfigHistory", QStringList()).toStringList();
    settings.endGroup();
}

void PageControllerSetup::addToHistory(ConfigType type, const QString &path)
{
    QStringList &hist = historyFor(type);
    hist.removeAll(path);
    hist.prepend(path);
    while (hist.size() > 2) {
        hist.removeLast();
    }

    saveHistory(type);
    updateHistoryButtons(type);
}

void PageControllerSetup::saveHistory(ConfigType type)
{
    QSettings settings;
    settings.beginGroup("PageControllerSetup");
    settings.setValue(type == ConfigMotor ? "motorConfigHistory" : "appConfigHistory", historyFor(type));
    settings.endGroup();
}

void PageControllerSetup::removeFromHistory(ConfigType type, const QString &path)
{
    historyFor(type).removeAll(path);
    saveHistory(type);
    updateHistoryButtons(type);
}

void PageControllerSetup::updateHistoryButtons(ConfigType type)
{
    QVBoxLayout *layout = historyLayoutFor(type);
    if (!layout) {
        return;
    }

    // Clear existing buttons (including nested layouts)
    while (QLayoutItem *item = layout->takeAt(0)) {
        if (QLayout *row = item->layout()) {
            while (QLayoutItem *sub = row->takeAt(0)) {
                if (QWidget *w = sub->widget()) {
                    w->hide();
                    w->deleteLater();
                }
                delete sub;
            }
        } else if (QWidget *w = item->widget()) {
            w->hide();
            w->deleteLater();
        }
        delete item;
    }

    const QStringList &hist = historyFor(type);
    for (int i = 0; i < hist.size(); ++i) {
        const QString &path = hist[i];
        QString fileName = QFileInfo(path).fileName();

        QPushButton *btn = new QPushButton(fileName, ui->scrollAreaWidgetContents);
        btn->setToolTip(path);
        btn->setIcon(QIcon(":/res/icons/Download-96.png"));
        btn->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

        connect(btn, &QPushButton::clicked, this, [this, type, path]() {
            onHistoryClicked(type, path);
        });

        // Create row with history button and delete button
        QHBoxLayout *row = new QHBoxLayout();
        row->setSpacing(4);
        row->setContentsMargins(0, 0, 0, 0);
        row->addWidget(btn, 1);

        QPushButton *rm = new QPushButton(QStringLiteral("\u00d7"), ui->scrollAreaWidgetContents);
        rm->setToolTip(tr("Remove from history"));
        rm->setCursor(Qt::PointingHandCursor);
        rm->setFixedWidth(28);
        connect(rm, &QPushButton::clicked, this, [this, type, path]() {
            removeFromHistory(type, path);
        });
        row->addWidget(rm, 0);

        layout->addLayout(row);
    }
}

QStringList &PageControllerSetup::historyFor(ConfigType type)
{
    return type == ConfigMotor ? mMotorConfigHistory : mAppConfigHistory;
}

QVBoxLayout *PageControllerSetup::historyLayoutFor(ConfigType type)
{
    return type == ConfigMotor ? ui->motorHistoryLayout : ui->appHistoryLayout;
}

void PageControllerSetup::onHistoryClicked(ConfigType type, const QString &path)
{
    if (mSetupStep != StepIdle) {
        return;
    }

    if (!QFileInfo::exists(path)) {
        QMessageBox::warning(this, tr("Load Configuration"),
                             tr("File not found:<BR>%1").arg(path));
        return;
    }

    const QString filePath = path;

    if (type == ConfigMotor) {
        loadMotorConfig(filePath);
    } else {
        loadAppConfig(filePath);
    }
}

void PageControllerSetup::on_loadMotorConfButton_clicked()
{
    if (!mVesc) {
        QMessageBox::warning(this, tr("Load Motor Configuration"),
                             tr("Not connected to the VESC."));
        return;
    }

    QString path = QFileDialog::getOpenFileName(this,
                                                tr("Choose motor configuration file to load"),
                                                QString(),
                                                tr("Xml files (*.xml)"));

    if (path.isNull()) {
        return;
    }

    addToHistory(ConfigMotor, path);
    loadMotorConfig(path);
}

void PageControllerSetup::on_loadAppConfButton_clicked()
{
    if (!mVesc) {
        QMessageBox::warning(this, tr("Load App Configuration"),
                             tr("Not connected to the VESC."));
        return;
    }

    QString path = QFileDialog::getOpenFileName(this,
                                                tr("Choose app configuration file to load"),
                                                QString(),
                                                tr("Xml files (*.xml)"));

    if (path.isNull()) {
        return;
    }

    addToHistory(ConfigApp, path);
    loadAppConfig(path);
}

void PageControllerSetup::loadMotorConfig(const QString &path)
{
    if (!mVesc) {
        QMessageBox::warning(this, tr("Load Motor Configuration"),
                             tr("Not connected to the VESC."));
        return;
    }

    bool res = mVesc->mcConfig()->loadXml(path, "MCConfiguration");

    if (res) {
        mVesc->emitStatusMessage(tr("Loaded motor configuration"), true);
        mVesc->commands()->setMcconf(false);
    } else {
        QMessageBox::warning(this, tr("Load motor configuration"),
                             tr("Could not load motor configuration:<BR>"
                                "%1").arg(mVesc->mcConfig()->xmlStatus()));
    }
}

void PageControllerSetup::loadAppConfig(const QString &path)
{
    if (!mVesc) {
        QMessageBox::warning(this, tr("Load App Configuration"),
                             tr("Not connected to the VESC."));
        return;
    }

    bool res = mVesc->appConfig()->loadXml(path, "APPConfiguration");

    if (res) {
        mVesc->emitStatusMessage(tr("Loaded app configuration"), true);
        mVesc->commands()->setAppConf();
    } else {
        QMessageBox::warning(this, tr("Load app configuration"),
                             tr("Could not load app configuration:<BR>"
                                "%1").arg(mVesc->appConfig()->xmlStatus()));
    }
}
