/*
    Copyright 2016 - 2017 Benjamin Vedder	benjamin@vedder.se

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

#include "detectfochall.h"
#include "ui_detectfochall.h"
#include "helpdialog.h"
#include <QMessageBox>
#include "utility.h"

DetectFocHall::DetectFocHall(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DetectFocHall)
{
    ui->setupUi(this);

    ui->helpButton->setIcon(Utility::getIcon("icons/Help-96.png"));
    ui->startButton->setIcon(Utility::getIcon("icons/Circled Play-96.png"));
    ui->applyButton->setIcon(Utility::getIcon("icons/apply.png"));

    if (ui->writeConfigButton) {
        ui->writeConfigButton->setVisible(false);
    }

    layout()->setContentsMargins(0, 0, 0, 0);
    mVesc = nullptr;

    setReferenceMode(false);
}

DetectFocHall::~DetectFocHall()
{
    delete ui;
}

void DetectFocHall::on_helpButton_clicked()
{
    if (mVesc) {
        HelpDialog::showHelp(this, mVesc->infoConfig(), "help_foc_hall_detect");
    }
}


void DetectFocHall::on_startButton_clicked()
{
    if (mVesc) {
        if (!mSkipConfirmation) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::warning(this,
                                         tr("Detect FOC Hall Sensor Parameters"),
                                         tr("This is going to turn the motor slowly. Make "
                                            "sure that nothing is in the way."),
                                         QMessageBox::Ok | QMessageBox::Cancel);

            if (reply != QMessageBox::Ok) {
                return;
            }
        }

        mVesc->commands()->measureHallFoc(ui->currentBox->value());
        mRunning = mVesc->isPortConnected();
    }
}
void DetectFocHall::on_applyButton_clicked()
{
    if (mVesc) {
        mVesc->mcConfig()->updateParamInt("foc_hall_table__0", ui->hall0Box->value());
        mVesc->mcConfig()->updateParamInt("foc_hall_table__1", ui->hall1Box->value());
        mVesc->mcConfig()->updateParamInt("foc_hall_table__2", ui->hall2Box->value());
        mVesc->mcConfig()->updateParamInt("foc_hall_table__3", ui->hall3Box->value());
        mVesc->mcConfig()->updateParamInt("foc_hall_table__4", ui->hall4Box->value());
        mVesc->mcConfig()->updateParamInt("foc_hall_table__5", ui->hall5Box->value());
        mVesc->mcConfig()->updateParamInt("foc_hall_table__6", ui->hall6Box->value());
        mVesc->mcConfig()->updateParamInt("foc_hall_table__7", ui->hall7Box->value());
        mVesc->emitStatusMessage(tr("Hall Sensor Parameters Applied"), true);
    }
}

void DetectFocHall::on_writeConfigButton_clicked()
{
    if (!mVesc) {
        return;
    }

    on_applyButton_clicked();

    if (mVesc->isPortConnected()) {
        mVesc->commands()->setMcconf(!mSkipConfirmation);
    }
}

VescInterface *DetectFocHall::vesc() const
{
    return mVesc;
}

bool DetectFocHall::isRunning() const
{
    return mRunning;
}

bool DetectFocHall::startMeasureHall()
{
    on_startButton_clicked();
    return mRunning;
}

bool DetectFocHall::applyAndWriteHallConfig()
{
    on_writeConfigButton_clicked();
    return mVesc && mVesc->isPortConnected();
}

void DetectFocHall::cancelHallAutomation()
{
    mRunning = false;
}

void DetectFocHall::setVesc(VescInterface *vesc)
{
    mVesc = vesc;

    if (mVesc) {
        connect(mVesc->commands(), SIGNAL(focHallTableReceived(QVector<int>,int)),
                this, SLOT(focHallTableReceived(QVector<int>,int)));
        connect(mVesc->commands(), SIGNAL(ackReceived(QString)),
                this, SLOT(ackReceived(QString)));

        connect(mVesc->mcConfig(), &ConfigParams::paramChangedInt,
                this, &DetectFocHall::mcParamChangedInt);
        connect(mVesc->mcConfig(), &ConfigParams::savingXml,
                this, &DetectFocHall::mcSavingXml);
        connect(mVesc, &VescInterface::configurationChanged,
                this, [this]() { ensureReferenceParams(); });

        connect(mVesc, &VescInterface::portConnectedChanged, this, [this]() {
            if (!mVesc->isPortConnected()) {
                mRunning = false;
                resetValues();
            }
        });

        ensureReferenceParams();
    }
}

void DetectFocHall::setWriteConfigEnabled(bool enabled)
{
    if (ui->writeConfigButton) {
        ui->writeConfigButton->setVisible(enabled);
    }
}

void DetectFocHall::setHideApplyButton(bool hide)
{
    if (ui->applyButton) {
        ui->applyButton->setVisible(!hide);
    }
}

void DetectFocHall::setSkipConfirmation(bool skip)
{
    mSkipConfirmation = skip;
}

void DetectFocHall::focHallTableReceived(QVector<int> hall_table, int res)
{
    mRunning = false;
    bool err = (res != 0);

    if (err) {
        mVesc->emitStatusMessage(tr("Bad FOC Hall Detection Result Received"), false);
    } else {
        mVesc->emitStatusMessage(tr("FOC Hall Result Received"), true);
        ui->hall0Box->setValue(hall_table.at(0));
        ui->hall1Box->setValue(hall_table.at(1));
        ui->hall2Box->setValue(hall_table.at(2));
        ui->hall3Box->setValue(hall_table.at(3));
        ui->hall4Box->setValue(hall_table.at(4));
        ui->hall5Box->setValue(hall_table.at(5));
        ui->hall6Box->setValue(hall_table.at(6));
        ui->hall7Box->setValue(hall_table.at(7));
    }

    emit hallMeasurementDone(!err);
}

void DetectFocHall::ackReceived(QString ackType)
{
    if (ackType == "Motor config write OK") {
        emit hallConfigWriteDone(true);
    }
}

void DetectFocHall::setResetBoxValues(bool reset)
{
    resetBoxValues = reset;
}

void DetectFocHall::setReferenceMode(bool enabled)
{
    ui->refWidget->setVisible(enabled);
}

void DetectFocHall::ensureReferenceParams()
{
    if (!mVesc) {
        return;
    }

    ConfigParams *mc = mVesc->mcConfig();
    if (!mc) {
        return;
    }

    for (int i = 0; i < 8; ++i) {
        ConfigParam minParam;
        minParam.setInt(-1, 255, -1, 1);
        minParam.longName = QString("Hall Table %1 Reference Min").arg(i);
        minParam.transmittable = false;
        QString minName = QString("foc_hall_table_ref_min__%1").arg(i);
        if (!mc->hasParam(minName)) {
            mc->addParam(minName, minParam);
        }

        ConfigParam maxParam;
        maxParam.setInt(-1, 255, -1, 1);
        maxParam.longName = QString("Hall Table %1 Reference Max").arg(i);
        maxParam.transmittable = false;
        QString maxName = QString("foc_hall_table_ref_max__%1").arg(i);
        if (!mc->hasParam(maxName)) {
            mc->addParam(maxName, maxParam);
        }
    }

    loadReferenceValues();
}

void DetectFocHall::loadReferenceValues()
{
    if (!mVesc) {
        return;
    }

    ConfigParams *mc = mVesc->mcConfig();
    if (!mc) {
        return;
    }

    for (int i = 0; i < 8; ++i) {
        QSpinBox *minBox = findChild<QSpinBox*>(QString("refMin%1Box").arg(i));
        QSpinBox *maxBox = findChild<QSpinBox*>(QString("refMax%1Box").arg(i));

        if (minBox) {
            minBox->setValue(mc->getParamInt(QString("foc_hall_table_ref_min__%1").arg(i)));
        }
        if (maxBox) {
            maxBox->setValue(mc->getParamInt(QString("foc_hall_table_ref_max__%1").arg(i)));
        }
    }
}

void DetectFocHall::mcParamChangedInt(QObject *src, const QString &name, int newParam)
{
    if (src == this || !mVesc) {
        return;
    }

    int index = -1;
    QString prefix;

    if (name.startsWith("foc_hall_table_ref_min__")) {
        prefix = "refMin";
        index = name.mid(24).toInt();
    } else if (name.startsWith("foc_hall_table_ref_max__")) {
        prefix = "refMax";
        index = name.mid(24).toInt();
    }

    if (index < 0 || index > 7) {
        return;
    }

    QSpinBox *box = findChild<QSpinBox*>(QString("%1%2Box").arg(prefix).arg(index));
    if (box) {
        box->setValue(newParam);
    }
}

void DetectFocHall::mcSavingXml()
{
    if (!mVesc) {
        return;
    }

    ConfigParams *mc = mVesc->mcConfig();
    if (!mc) {
        return;
    }

    for (int i = 0; i < 8; ++i) {
        QSpinBox *minBox = findChild<QSpinBox*>(QString("refMin%1Box").arg(i));
        QSpinBox *maxBox = findChild<QSpinBox*>(QString("refMax%1Box").arg(i));

        if (minBox) {
            mc->updateParamInt(QString("foc_hall_table_ref_min__%1").arg(i), minBox->value(), this);
        }
        if (maxBox) {
            mc->updateParamInt(QString("foc_hall_table_ref_max__%1").arg(i), maxBox->value(), this);
        }
    }
}

void DetectFocHall::resetReferenceValues()
{
    if (!mVesc) {
        return;
    }

    ConfigParams *mc = mVesc->mcConfig();
    if (!mc) {
        return;
    }

    for (int i = 0; i < 8; ++i) {
        mc->updateParamInt(QString("foc_hall_table_ref_min__%1").arg(i), -1, this);
        mc->updateParamInt(QString("foc_hall_table_ref_max__%1").arg(i), -1, this);

        QSpinBox *minBox = findChild<QSpinBox*>(QString("refMin%1Box").arg(i));
        QSpinBox *maxBox = findChild<QSpinBox*>(QString("refMax%1Box").arg(i));

        if (minBox) {
            minBox->setValue(-1);
        }
        if (maxBox) {
            maxBox->setValue(-1);
        }
    }
}

void DetectFocHall::resetValues()
{
    if (resetBoxValues) {
        ui->hall0Box->setValue(0);
        ui->hall1Box->setValue(0);
        ui->hall2Box->setValue(0);
        ui->hall3Box->setValue(0);
        ui->hall4Box->setValue(0);
        ui->hall5Box->setValue(0);
        ui->hall6Box->setValue(0);
        ui->hall7Box->setValue(0);
    }
}